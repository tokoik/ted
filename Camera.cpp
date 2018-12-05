//
// �J�����֘A�̏���
//
#include "Camera.h"

#if defined(_WIN32)
#  define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#  if defined(_DEBUG)
#    define CV_EXT_STR "d.lib"
#  else
#    define CV_EXT_STR ".lib"
#  endif
#  pragma comment(lib, "opencv_world" CV_VERSION_STR CV_EXT_STR)
#endif

// �R���X�g���N�^
Camera::Camera()
{
  // ��Ɨp�̃������̈�
  recvbuf = sendbuf = nullptr;

  // �L���v�`�������摜�̃t�H�[�}�b�g
  format = GL_BGR;

  // ���k�ݒ�
  param.push_back(CV_IMWRITE_JPEG_QUALITY);
  param.push_back(defaults.remote_texture_quality);

  for (int cam = 0; cam < camCount; ++cam)
  {
    // �摜���܂��擾����Ă��Ȃ����Ƃ��L�^���Ă���
    buffer[cam] = nullptr;

    // �X���b�h����~��Ԃł��邱�Ƃ��L�^���Ă���
    run[cam] = false;
  }
}

// �f�X�g���N�^
Camera::~Camera()
{
  // ��Ɨp�̃��������J������
  delete[] recvbuf, sendbuf;
  recvbuf = sendbuf = nullptr;
}

// �X���b�h���~����
void Camera::stop()
{
  // �L���v�`���X���b�h���~�߂�
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (run[cam])
    {
      // �L���v�`���X���b�h�̃��[�v���~�߂�
      captureMutex[cam].lock();
      run[cam] = false;
      captureMutex[cam].unlock();

      // �L���v�`���X���b�h���I������̂�҂�
      if (captureThread[cam].joinable()) captureThread[cam].join();
    }
  }

  // �l�b�g���[�N�X���b�h���~�߂�
  if (useNetwork())
  {
    // ��M�X���b�h���I������̂�҂�
    if (recvThread.joinable()) recvThread.join();

    // ���M�X���b�h���I������̂�҂�
    if (sendThread.joinable()) sendThread.join();
  }
}

// �J���������b�N���ĉ摜���e�N�X�`���ɓ]������
bool Camera::transmit(int cam, GLuint texture, const GLsizei *size)
{
  // �J�����̃��b�N�����݂�
  if (captureMutex[cam].try_lock())
  {
    // �V�����f�[�^���������Ă�����
    if (buffer[cam])
    {
      // �f�[�^���e�N�X�`���ɓ]������
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size[0], size[1], format, GL_UNSIGNED_BYTE, buffer[cam]);

      // �f�[�^�̓]���������L�^����
      buffer[cam] = nullptr;
    }

    // ���J�����̃��b�N����������
    captureMutex[cam].unlock();

    // �e�N�X�`����]������
    return true;
  }

  // �e�N�X�`����]�����Ă��Ȃ�
  return false;
}

// �����[�g�̎p������M����
void Camera::recv()
{
  // �L���v�`���Ԋu
  const double capture_interval(defaults.capture_fps > 0.0 ? 1.0 / defaults.capture_fps : 30.0);

  // ���O�̃t���[���̎�M����
  double last(glfwGetTime());

  // �X���b�h�����s�̊�
  while (run[camL])
  {
    // �p���f�[�^����M����
    const int ret(network.recvData(recvbuf, maxFrameSize));

    // �T�C�Y�� 0 �Ȃ�I������
    if (ret == 0) return;

    // �G���[���Ȃ���΃f�[�^��ǂݍ���
    if (ret > 0 && network.checkRemote())
    {
      // �w�b�_�̃t�H�[�}�b�g
      unsigned int *const head(reinterpret_cast<unsigned int *>(recvbuf));

      // �ϊ��s��̕ۑ���
      GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + camCount + 1));

      // �ϊ��s��𕜋A����
      remoteMatrix->store(body, 0, head[camCount]);
    }

    // ���ݎ���
    const double now(glfwGetTime());

    // ���̃t���[���̑��M�����܂ł̎c�莞��
    const double delay(last + capture_interval - now);

#if DEBUG
    std::cerr << "recv delay = " << delay << '\n';
#endif
    // �c�莞�Ԃ������
    if (delay > 0.0)
    {
      // ���̃t���[���̑��M�����܂ő҂�
      std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(delay)));
    }

    // ���O�̃t���[���̑��M�������X�V����
    last = now;
  }
}

// ���[�J���̉f���Ǝp���𑗐M����
void Camera::send()
{
  // �L���v�`���Ԋu
  const double capture_interval(defaults.capture_fps > 0.0 ? 1.0 / defaults.capture_fps : 30.0);

  // ���O�̃t���[���̑��M����
  double last(glfwGetTime());

  // �J�����X���b�h�����s�̊�
  while (run[camL])
  {
    // �w�b�_�̃t�H�[�}�b�g
    unsigned int *const head(reinterpret_cast<unsigned int *>(sendbuf));

    // ���E�t���[���̃T�C�Y�� 0 �ɂ��Ă���
    head[camL] = head[camR] = 0;

    // �ϊ��s��̐���ۑ�����
    head[camCount] = localMatrix->getUsed();

    // �ϊ��s��̕ۑ���
    GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + camCount + 1));

    // �ϊ��s���ۑ�����
    localMatrix->load(body);

    // ���t���[���̕ۑ��� (�ϊ��s��̍Ō�)
    uchar *data(reinterpret_cast<uchar *>(body + head[camCount]));

    // ���ɐV�����t���[�����������Ă����
    if (!encoded[camL].empty())
    {
      // ���L���v�`���f�o�C�X�����b�N����
      captureMutex[camL].lock();

      // ���t���[���̃T�C�Y��ۑ�����
      head[camL] = static_cast<unsigned int>(encoded[camL].size());

      // ���t���[���̃f�[�^���R�s�[����
      memcpy(data, encoded[camL].data(), head[camL]);

      // ���t���[���̃f�[�^����ɂ���
      encoded[camL].clear();

      // ���t���[���̓]������������΃��b�N����������
      captureMutex[camL].unlock();

      // �E�t���[���̕ۑ��� (���t���[���̍Ō�)
      data += head[camL];

      // �E�L���v�`���f�o�C�X�����삵�Ă����
      if (run[camR])
      {
        // �E�L���v�`���f�o�C�X�����b�N����
        captureMutex[camR].lock();

        // �E�t���[���̃T�C�Y��ۑ�����
        head[camR] = static_cast<unsigned int>(encoded[camR].size());

        // �E�t���[���̃f�[�^�����t���[���̃f�[�^�̌��ɃR�s�[����
        memcpy(data, encoded[camR].data(), head[camR]);

        // �E�t���[���̃f�[�^����ɂ���
        encoded[camR].clear();

        // �t���[���̓]������������΃��b�N����������
        captureMutex[camR].unlock();

        // �E�t���[���̍Ō�
        data += head[camR];
      }

      // �t���[���𑗐M����
      network.sendData(sendbuf, static_cast<unsigned int>(data - sendbuf));
    }

    // ���ݎ���
    const double now(glfwGetTime());

    // ���̃t���[���̑��M�����܂ł̎c�莞��
    const double delay(last + capture_interval - now);

#if DEBUG
    std::cerr << "send delay = " << delay << '\n';
#endif
    // �c�莞�Ԃ������
    if (delay > 0.0)
    {
      // ���̃t���[���̑��M�����܂ő҂�
      std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(delay)));
    }

    // ���O�̃t���[���̑��M�������X�V����
    last = now;
  }

  // ���[�v�𔲂���Ƃ��� EOF �𑗐M����
  network.sendEof();
}

// ��ƎҒʐM�X���b�h�N��
int Camera::startWorker(unsigned short port, const char *address)
{
  // ���łɊm�ۂ���Ă����Ɨp��������j������
  delete[] recvbuf, sendbuf;
  recvbuf = sendbuf = nullptr;

  // ��Ǝ҂Ƃ��ď���������
  const int ret(network.initialize(2, port, address));
  if (ret > 0) return ret;

  // ��Ɨp�̃��������m�ۂ���i����� Camera �̃f�X�g���N�^�� delete ����j
  recvbuf = new uchar[maxFrameSize];
  sendbuf = new uchar[maxFrameSize];

  // �ʐM�X���b�h���J�n����
  run[camL] = true;
  recvThread = std::thread([this]() { recv(); });
  sendThread = std::thread([this]() { send(); });

  return 0;
}

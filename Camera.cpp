//
// �J�����֘A�̏���
//
#include "Camera.h"

// �V�[���O���t
#include "Scene.h"

// OpenCV �̃��C�u�����̃����N
#if defined(_WIN32)
#  define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#  if defined(_DEBUG)
#    define CV_EXT_STR "d.lib"
#  else
#    define CV_EXT_STR ".lib"
#  endif
#  pragma comment(lib, "opencv_core" CV_VERSION_STR CV_EXT_STR)
#  pragma comment(lib, "opencv_highgui" CV_VERSION_STR CV_EXT_STR)
#  pragma comment(lib, "opencv_imgcodecs" CV_VERSION_STR CV_EXT_STR)
#  pragma comment(lib, "opencv_videoio" CV_VERSION_STR CV_EXT_STR)
#endif

// �R���X�g���N�^
Camera::Camera()
{
  // ��Ɨp�̃������̈�
  recvbuf = sendbuf = nullptr;

  // �L���v�`�������摜�̃t�H�[�}�b�g
  format = GL_BGR;

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

// ���k�ݒ�
void Camera::setQuality(int quality)
{
  // ���k�ݒ�
  param.push_back(cv::IMWRITE_JPEG_QUALITY);
  param.push_back(quality);
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

      // ��M�����ϊ��s��̊i�[�ꏊ
      GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + headLength));

      // �ϊ��s������L�������Ɋi�[���� (head[camCount] �ɂ͕ϊ��s��̐��������Ă���)
      remoteAttitude->store(body, head[camCount]);
    }

    // ���̃X���b�h�����\�[�X�ɃA�N�Z�X���邽�߂ɏ����҂�
    std::this_thread::sleep_for(std::chrono::milliseconds(minDelay));
  }
}

// ���[�J���̉f���Ǝp���𑗐M����
void Camera::send()
{
  // ���O�̃t���[���̑��M����
  double last(glfwGetTime());

  // �J�����X���b�h�����s�̊�
  while (run[camL])
  {
    // �w�b�_�̃t�H�[�}�b�g
    unsigned int *const head(reinterpret_cast<unsigned int *>(sendbuf));

    // ���E�t���[���̃T�C�Y�� 0 �ɂ��Ă���
    head[camL] = head[camR] = 0;

    // ���E�̃t���[���T�C�Y�̎��ɕϊ��s��̐���ۑ�����
    head[camCount] = localAttitude->getSize();

    // ���M����ϊ��s��̊i�[�ꏊ
    GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + headLength));

    // �ϊ��s������L������������o��
    localAttitude->load(body, head[camCount]);

    // ���t���[���̕ۑ��� (�ϊ��s��̍Ō�)
    uchar *data(reinterpret_cast<uchar *>(body + head[camCount]));

    // ���̃t���[���̒x������
    long long delay(minDelay);

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

      // ���ݎ���
      const double now(glfwGetTime());

      // ���̃t���[���̑��M�����܂ł̎c�莞��
      const long long remain(static_cast<long long>(last + capture_interval - now));

#if defined(DEBUG)
      std::cerr << "send remain = " << remain << '\n';
#endif

      // �c�莞�ԕ��x��������
      if (remain > delay) delay = remain;

      // ���O�̃t���[���̑��M�������X�V����
      last = now;
    }

    // ���̃t���[���̑��M�����܂ő҂�
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
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

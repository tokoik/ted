//
// �J�����֘A�̏���
//
#include "Camera.h"

#ifdef _WIN32
#  define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#  ifdef _DEBUG
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
  sendbuf = recvbuf = nullptr;

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
  delete[] sendbuf, recvbuf;
  sendbuf = recvbuf = nullptr;
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
    // ���M�X���b�h���I������̂�҂�
    if (sendThread.joinable()) sendThread.join();

    // ��M�X���b�h���I������̂�҂�
    if (recvThread.joinable()) recvThread.join();
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

// �f�[�^���M
void Camera::send()
{
  // �J�����X���b�h�����s�̊�
  while (run[camL])
  {
    // �L���v�`���f�o�C�X�����b�N����
    captureMutex[camL].lock();

    // �w�b�_�̃t�H�[�}�b�g
    unsigned int *const head(reinterpret_cast<unsigned int *>(sendbuf));

    // ���t���[���̃T�C�Y��ۑ�����
    head[camL] = static_cast<unsigned int>(encoded[camL].size());

    // �E�t���[���̃T�C�Y�� 0 �ɂ��Ă���
    head[camR] = 0;

    // �ϊ��s��̐���ۑ�����
    head[camCount] = localMatrix->getUsed();

    // �ϊ��s��̕ۑ���
    GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + camCount + 1));

    // �ϊ��s���ۑ�����
    localMatrix->load(body);

    // ���t���[���̕ۑ��� (�ϊ��s��̍Ō�)
    uchar *data(reinterpret_cast<uchar *>(body + head[camCount]));

    // ���t���[���̃f�[�^���R�s�[����
    memcpy(data, encoded[camL].data(), head[camL]);

    // �t���[���̓]������������΃��b�N����������
    captureMutex[camL].unlock();

    // ���t���[���ɉ摜�������
    if (head[camL] > 0)
    {
      // �E�t���[���̕ۑ��� (���t���[���̍Ō�)
      data += head[camL];

      if (run[camR])
      {
        // �L���v�`���f�o�C�X�����b�N����
        captureMutex[camR].lock();

        // �E�t���[���̃T�C�Y��ۑ�����
        head[camR] = static_cast<unsigned int>(encoded[camR].size());

        // �E�t���[���̃f�[�^�����t���[���̃f�[�^�̌��ɃR�s�[����
        memcpy(data, encoded[camR].data(), head[camR]);

        // �t���[���̓]������������΃��b�N����������
        captureMutex[camR].unlock();

        // �E�t���[���̍Ō�
        data += head[camR];
      }
    }

    // �t���[���𑗐M����
    network.sendFrame(sendbuf, static_cast<unsigned int>(data - sendbuf));

    // ���̃X���b�h�����\�[�X�ɃA�N�Z�X���邽�߂ɏ����҂�
    std::this_thread::sleep_for(std::chrono::milliseconds(10L));
  }
}

// �f�[�^��M
void Camera::recv()
{
  // �X���b�h�����s�̊�
  while (run[camL])
  {
    // �p���f�[�^����M����
    if (network.recvFrame(recvbuf, maxFrameSize) != SOCKET_ERROR)
    {
      // �w�b�_�̃t�H�[�}�b�g
      unsigned int *const head(reinterpret_cast<unsigned int *>(recvbuf));

      // �ϊ��s��̕ۑ���
      GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + camCount + 1));

      // �ϊ��s��𕜋A����
      remoteMatrix->store(body, 0, head[camCount]);
    }

    // ���̃X���b�h�����\�[�X�ɃA�N�Z�X���邽�߂ɏ����҂�
    std::this_thread::sleep_for(std::chrono::milliseconds(10L));
  }
}

// ��ƎҒʐM�X���b�h�N��
void Camera::startWorker(unsigned short port, const char *address)
{
  // ��Ǝ҂Ƃ��ď���������
  if (network.initialize(2, port, address)) return;

  // ��Ɨp�̃��������m�ۂ���
  delete[] sendbuf, recvbuf;
  sendbuf = new uchar[maxFrameSize];
  recvbuf = new uchar[maxFrameSize];

  // ���M�X���b�h���J�n����
  sendThread = std::thread([this]() { send(); });

  // ��M�X���b�h���J�n����
  recvThread = std::thread([this]() { recv(); });
}

//
// �����[�g�J��������L���v�`��
//
#include "CamRemote.h"

// �V�[���O���t
#include "Scene.h"

// ���L������
#include "SharedMemory.h"

// �R���X�g���N�^
CamRemote::CamRemote(double width, double height, bool reshape)
  : reshape{ reshape }
{
  if (reshape)
  {
    // �w�i�摜�̑傫��
    size[camL] = size[camR] = cv::Size(static_cast<int>(width), static_cast<int>(height));

    // �w�i�摜�̕ό`�Ɏg���t���[���o�b�t�@�I�u�W�F�N�g
    glGenFramebuffers(1, &fb);

    // �����[�g����擾�����t���[���̃T���v�����O�Ɏg���e�N�X�`��
    glGenTextures(camCount, resample);

    // ����摜�ɕό`����V�F�[�_
    shader = ggLoadShader("mesh.vert", "mesh.frag");
    gapLoc = glGetUniformLocation(shader, "gap");
    screenLoc = glGetUniformLocation(shader, "screen");
    rotationLoc = glGetUniformLocation(shader, "rotation");
    imageLoc = glGetUniformLocation(shader, "image");
  }
}

// �f�X�g���N�^
CamRemote::~CamRemote()
{
  // �X���b�h���~����
  stop();

  if (reshape)
  {
    // �w�i�摜�̃^�C�����O�Ɏg���t���[���o�b�t�@�I�u�W�F�N�g
    glDeleteFramebuffers(1, &fb);

    // �����[�g����擾�����t���[���̃T���v�����O�Ɏg���e�N�X�`��
    glDeleteTextures(camCount, resample);

    // ����摜�ɕό`����V�F�[�_
    glDeleteProgram(shader);
  }
}

// ���c�ґ��̋N��
int CamRemote::open(unsigned short port, const char* address, const GLfloat* fov, int samples)
{
  // ���łɊm�ۂ���Ă����Ɨp��������j������
  delete[] sendbuf, recvbuf;
  sendbuf = recvbuf = nullptr;

  // ���c�҂Ƃ��ď���������
  const int ret(network.initialize(1, port, address));
  if (ret != 0) return ret;

  // ��Ɨp�̃��������m�ۂ���i����� Camera �̃f�X�g���N�^�� delete ����j
  sendbuf = new uchar[maxFrameSize];
  recvbuf = new uchar[maxFrameSize];

  // �w�b�_�̃t�H�[�}�b�g
  unsigned int* const head(reinterpret_cast<unsigned int*>(recvbuf));

  // 1 �t���[���󂯎����
  for (int i = 0;;)
  {
    const int ret(network.recvData(recvbuf, maxFrameSize));
#if defined(DEBUG)
    std::cerr << "CamRemote open:" << ret << ',' << head[camL] << ',' << head[camR] << '\n';
#endif
    if (ret > 0 && head[camL] > 0) break;
    if (++i > receiveRetry) return ret;
  }

  // ��M�����ϊ��s��̊i�[�ꏊ
  GgMatrix* const body(reinterpret_cast<GgMatrix*>(head + headLength));

  // �ϊ��s������L�������Ɋi�[����
  Scene::storeRemoteAttitude(body, head[camCount]);

  // ���t���[���̕ۑ��� (�ϊ��s��̍Ō�)
  uchar* const data(reinterpret_cast<uchar*>(body + head[camCount]));

  // ���������ꂽ�f�[�^�̈ꎞ�ۑ���
  std::vector<GLubyte> encoded;

  // ���t���[���f�[�^�� vector �ɕϊ�����
  encoded.assign(data, data + head[camL]);

  // ���t���[�����f�R�[�h����
  remote[camL] = cv::imdecode(cv::Mat(encoded), 1);

  // �����[�g����擾�����t���[���̃T�C�Y
  cv::Size rsize[camCount];

  // ���t���[���̃T�C�Y�����߂�
  rsize[camL] = remote[camL].size();

  // �E�t���[�������݂����
  if (head[camR] > 0)
  {
    // �E�t���[���f�[�^�� vector �ɕϊ�����
    encoded.assign(data + head[camL], data + head[camL] + head[camR]);

    // �E�t���[�����f�R�[�h����
    remote[camR] = cv::imdecode(cv::Mat(encoded), 1);

    // �E�t���[���̃T�C�Y�����߂�
    rsize[camR] = remote[camR].size();
  }
  else
  {
    // �E�t���[���͍��Ɠ����ɂ���
    remote[camR] = remote[camL];

    // �E�t���[���̃T�C�Y�͍��t���[���Ɠ����ɂ���
    rsize[camR] = rsize[camL];
  }

  if (reshape)
  {
    // �w�i�摜�̕ό`�Ɏg�����b�V���̏c���̊i�q�_��������
    const GLfloat aspect(static_cast<GLfloat>(size[camL].width) / static_cast<GLfloat>(size[camL].height));
    slices = static_cast<GLsizei>(sqrt(aspect * static_cast<GLfloat>(samples)));
    stacks = samples / slices;

    // �w�i�摜�̕ό`�Ɏg�����b�V���̏c���̊i�q�Ԋu�����߂�
    gap[0] = 2.0f / static_cast<GLfloat>(slices - 1);
    gap[1] = 2.0f / static_cast<GLfloat>(stacks - 1);

    // �w�i�摜���擾���郊���[�g�J�����̉�p
    screen[0] = tan(fov[0]);
    screen[1] = tan(fov[1]);

    for (int cam = 0; cam < camCount; ++cam)
    {
      // �����[�g����擾�����t���[���̃T���v�����O�Ɏg���e�N�X�`������������
      glBindTexture(GL_TEXTURE_2D, resample[cam]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, rsize[cam].width, rsize[cam].height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    }
  }
  else
  {
    // �h�[���摜�ɕό`���Ȃ��Ƃ��͔w�i�e�N�X�`���̃T�C�Y�ɂ���
    size[camL] = rsize[camL];
    size[camR] = rsize[camR];
  }

  // �ʐM�X���b�h���J�n����
  run[camL] = true;
  sendThread = std::thread([this]() { send(); });
  recvThread = std::thread([this]() { recv(); });

  return 0;
}

// ���J���������b�N���ĉ摜���e�N�X�`���ɓ]������
bool CamRemote::transmit(int cam, GLuint texture, const GLsizei* size)
{
  // �摜�̃T�C�Y
  const GLsizei fsize[] = { remote[cam].cols, remote[cam].rows };

  if (!reshape) return Camera::transmit(cam, texture, fsize);

  if (Camera::transmit(cam, resample[cam], fsize))
  {
    // �e�N�X�`���ό`�p�̃V�F�[�_
    glUseProgram(shader);
    glUniform2fv(gapLoc, 1, gap);
    glUniform2fv(screenLoc, 1, screen);
    glUniform1i(imageLoc, 0);
    glViewport(0, 0, size[0], size[1]);

    // �w�i�摜�̕ό`�Ɏg���t���[���o�b�t�@�I�u�W�F�N�g�ɐ؂�ւ���
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

    // �����[�g����擾�����t���[���̃T���v�����O�Ɏg���e�N�X�`�����w�肷��
    glBindTexture(GL_TEXTURE_2D, resample[cam]);

    // �����[�g�̃w�b�h�g���b�L���O����ݒ肵�ă����_�����O
    glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, Scene::getRemoteAttitude(cam).get());
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, slices * 2, stacks - 1);

    // �����_�����O���ʏ�̃t���[���o�b�t�@�ɖ߂�
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // �t���[���o�b�t�@�I�u�W�F�N�g�̃J���[�o�b�t�@�Ɏg�����e�N�X�`�����w�肷��
    glBindTexture(GL_TEXTURE_2D, texture);

    return true;
  }

  return false;
}

// �����[�g�̉f���Ǝp������M����
void CamRemote::recv()
{
  // �X���b�h�����s�̊�
  while (run[camL])
  {
    // �p���f�[�^�Ɖ摜�f�[�^����M����
    const int ret{ network.recvData(recvbuf, maxFrameSize) };

#if defined(DEBUG)
    std::cerr << "CamRemote recv:" << ret << '\n';
#endif

    // �T�C�Y�� 0 �Ȃ�I������
    if (ret == 0) return;

    // �G���[���Ȃ���΃f�[�^��ǂݍ���
    if (ret > 0 && network.checkRemote())
    {
      // �w�b�_�̃t�H�[�}�b�g
      const auto head{ reinterpret_cast<unsigned int*>(recvbuf) };

      // ��M�����ϊ��s��̊i�[�ꏊ
      const auto body{ reinterpret_cast<GgMatrix*>(head + headLength) };

      // �ϊ��s������L�������Ɋi�[����
      Scene::storeRemoteAttitude(body, head[camCount]);

      // ���t���[���̕ۑ��� (�ϊ��s��̍Ō�)
      const auto data{ reinterpret_cast<uchar*>(body + head[camCount]) };

      // �����[�g����擾�����t���[���̃T�C�Y
      cv::Size rsize[camCount];

      // ���������ꂽ�f�[�^�̈ꎞ�ۑ���
      std::vector<GLubyte> encoded;

      // ���o�b�t�@����̂Ƃ����t���[���������Ă��Ă����
      if (!captured[camL] && head[camL] > 0)
      {
        // ���t���[���f�[�^�� vector �ɕϊ�����
        encoded.assign(data, data + head[camL]);

        // ���t���[�����f�R�[�h���ĕۑ���
        remote[camL] = cv::imdecode(cv::Mat(encoded), 1);

        // ���t���[���̃T�C�Y�����߂Ă�����
        rsize[camL] = remote[camL].size();

        // ���摜�����b�N��
        captureMutex[camL].lock();

        // ���摜���X�V������
        image[camL] = remote[camL];

        // ���t���[���̎擾�̊������L�^����
        captured[camL] = true;

        // ���摜�̃��b�N����������
        captureMutex[camL].unlock();
      }

      // �E�o�b�t�@����̂Ƃ��E�t���[���������Ă��Ă����
      if (!captured[camR] && head[camR] > 0)
      {
        // �E�t���[���f�[�^�� vector �ɕϊ�����
        encoded.assign(data + head[camL], data + head[camL] + head[camR]);

        // �E�t���[�����f�R�[�h���ĕۑ���
        remote[camR] = cv::imdecode(cv::Mat(encoded), 1);

        // �E�t���[���̃T�C�Y�����߂Ă�����
        rsize[camR] = remote[camR].size();

        // �E�摜�����b�N��
        captureMutex[camR].lock();

        // �E�摜���X�V������
        image[camR] = remote[camR];

        // �E�t���[���̎擾�̊������L�^����
        captured[camR] = true;

        // �E�摜�̃��b�N����������
        captureMutex[camR].unlock();
      }

      // �E�t���[�����ۑ�����Ă��Ȃ����
      if (remote[camR].empty())
      {
        // �E�t���[���̃T�C�Y�͍��t���[���Ɠ����ɂ���
        rsize[camR] = rsize[camL];

        // �E�摜�����b�N��
        captureMutex[camR].lock();

        // �E�摜�͍��t���[���Ɠ����ɂ���
        image[camR] = remote[camL];

        // �E�t���[���̎擾�̊������L�^����
        captured[camR] = true;

        // �E�摜�̃��b�N����������
        captureMutex[camR].unlock();
      }
    }

    // ���̃X���b�h�����\�[�X�ɃA�N�Z�X���邽�߂ɏ����҂�
    std::this_thread::sleep_for(std::chrono::milliseconds(minDelay));
  }
}

// ���[�J���̎p���𑗐M����
void CamRemote::send()
{
  // ���O�̃t���[���̑��M����
  auto last{ glfwGetTime() };

  // �J�����X���b�h�����s�̊�
  while (run[camL])
  {
    // �w�b�_�̃t�H�[�}�b�g
    const auto head{ reinterpret_cast<unsigned int*>(sendbuf) };

    // ���E�̃t���[���̃T�C�Y�� 0 �ɂ���
    head[camL] = head[camR] = 0;

    // �ϊ��s��̐���ۑ�����
    head[camCount] = Scene::getLocalAttitudeSize();

    // ���M����ϊ��s��̊i�[�ꏊ
    const auto body{ reinterpret_cast<GgMatrix*>(head + headLength) };

    // �ϊ��s������L������������o��
    Scene::loadLocalAttitude(body, head[camCount]);

    // ���t���[���̕ۑ��� (�ϊ��s��̍Ō�)
    const auto data{ reinterpret_cast<uchar*>(body + head[camCount]) };

    // �t���[���𑗐M����
    network.sendData(sendbuf, static_cast<unsigned int>(data - sendbuf));

    // ���ݎ���
    const auto now{ glfwGetTime() };

    // ���̃t���[���̑��M�����܂ł̎c�莞��
    const auto remain{ static_cast<long long>(last + capture_interval - now) };

#if defined(DEBUG)
    std::cerr << "send remain = " << remain << '\n';
#endif

    // ���O�̃t���[���̑��M�������X�V����
    last = now;

    // �c�莞�ԕ��x��������
    const auto delay{ remain > minDelay ? remain : minDelay };

    // ���̃t���[���̑��M�����܂ő҂�
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
  }

  // ���[�v�𔲂���Ƃ��� EOF �𑗐M����
  network.sendEof();
}

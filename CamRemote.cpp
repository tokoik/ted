//
// �����[�g�J��������L���v�`��
//
#include "CamRemote.h"

// ���g���C��
const int retry(3);

// �R���X�g���N�^
CamRemote::CamRemote(bool reshape)
  : reshape(reshape)
{
  if (reshape)
  {
    // �w�i�摜�̃T�C�Y
    size[camR][0] = size[camL][0] = static_cast<GLsizei>(defaults.capture_width);
    size[camR][1] = size[camL][1] = static_cast<GLsizei>(defaults.capture_height);

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
int CamRemote::open(unsigned short port, const char *address, const GLuint *texture)
{
  // ��Ɨp�̃��������m�ۂ���i����� Camera �̃f�X�g���N�^�� delete ����j
  delete sendbuf, recvbuf;
  sendbuf = new uchar[maxFrameSize];
  recvbuf = new uchar[maxFrameSize];

  // ���c�҂Ƃ��ď���������
  network.initialize(1, port, address);

  // �w�b�_�̃t�H�[�}�b�g
  unsigned int *const head(reinterpret_cast<unsigned int *>(recvbuf));

  // 1 �t���[���󂯎����
  for (int i = 0;;)
  {
    const int ret(network.recvFrame(recvbuf, maxFrameSize));
    if (ret > 0 && head[camL] > 0) break;
    if (++i > retry) return ret;
  }

  // �ϊ��s��̕ۑ���
  GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + camCount + 1));

  // �ϊ��s��𕜋A����
  remoteMatrix->store(body, 0, head[camCount]);

  // ���t���[���̕ۑ��� (�ϊ��s��̍Ō�)
  uchar *const data(reinterpret_cast<uchar *>(body + head[camCount]));

  // ���t���[���f�[�^�� vector �ɕϊ�����
  encoded[camL].assign(data, data + head[camL]);

  // ���t���[�����f�R�[�h����
  remote[camL] = cv::imdecode(cv::Mat(encoded[camL]), 1);

  // �����[�g����擾�����t���[���̃T�C�Y
  GLsizei rsize[camCount][2];

  // ���t���[���̃T�C�Y�����߂�
  rsize[camL][0] = remote[camL].cols;
  rsize[camL][1] = remote[camL].rows;

  // �E�t���[�������݂����
  if (head[camR] > 0)
  {
    // �E�t���[���f�[�^�� vector �ɕϊ�����
    encoded[camR].assign(data + head[camL], data + head[camL] + head[camR]);

    // �E�t���[�����f�R�[�h����
    remote[camR] = cv::imdecode(cv::Mat(encoded[camR]), 1);

    // �E�t���[���̃T�C�Y�����߂�
    rsize[camR][0] = remote[camR].cols;
    rsize[camR][1] = remote[camR].rows;
  }
  else
  {
    // �E�t���[���͍��Ɠ����ɂ���
    remote[camR] = remote[camL];

    // �E�t���[���̃T�C�Y�͍��t���[���Ɠ����ɂ���
    rsize[camR][0] = rsize[camL][0];
    rsize[camR][1] = rsize[camL][1];
  }

  if (reshape)
  {
    // �w�i�摜�̕ό`�Ɏg�����b�V���̏c���̊i�q�_��������
    const GLfloat aspect(static_cast<GLfloat>(size[camL][0]) / static_cast<GLfloat>(size[camL][1]));
    slices = static_cast<GLsizei>(sqrt(aspect * static_cast<GLfloat>(defaults.remote_texture_samples)));
    stacks = defaults.remote_texture_samples / slices;

    // �w�i�摜�̕ό`�Ɏg�����b�V���̏c���̊i�q�Ԋu�����߂�
    gap[0] = 2.0f / static_cast<GLfloat>(slices - 1);
    gap[1] = 2.0f / static_cast<GLfloat>(stacks - 1);

    // �w�i�摜���擾���郊���[�g�J�����̉�p
    screen[0] = tan(defaults.remote_fov_x);
    screen[1] = tan(defaults.remote_fov_y);

    for (int cam = 0; cam < camCount; ++cam)
    {
      // �����[�g����擾�����t���[���̃T���v�����O�Ɏg���e�N�X�`������������
      glBindTexture(GL_TEXTURE_2D, resample[cam]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, rsize[cam][0], rsize[cam][1], 0,
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
    size[camL][0] = rsize[camL][0];
    size[camL][1] = rsize[camL][1];
    size[camR][0] = rsize[camR][0];
    size[camR][1] = rsize[camR][1];
  }

  // �ʐM�X���b�h���J�n����
  run[camL] = true;
  sendThread = std::thread([this]() { send(); });
  recvThread = std::thread([this]() { recv(); });

  return 0;
}

// ���J���������b�N���ĉ摜���e�N�X�`���ɓ]������
bool CamRemote::transmit(int cam, GLuint texture, const GLsizei *size)
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
    glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, getRemoteAttitude(cam).transpose().get());
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, slices * 2, stacks - 1);

    // �����_�����O���ʏ�̃t���[���o�b�t�@�ɖ߂�
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // �t���[���o�b�t�@�I�u�W�F�N�g�̃J���[�o�b�t�@�Ɏg�����e�N�X�`�����w�肷��
    glBindTexture(GL_TEXTURE_2D, texture);

    return true;
  }

  return false;
}

// ���[�J���̎p���𑗐M����
void CamRemote::send()
{
  // �J�����X���b�h�����s�̊�
  while (run[camL])
  {
    // �w�b�_�̃t�H�[�}�b�g
    unsigned int *const head(reinterpret_cast<unsigned int *>(sendbuf));

    // ���E�̃t���[���̃T�C�Y�� 0 �ɂ���
    head[camL] = head[camR] = 0;

    // �ϊ��s��̐���ۑ�����
    head[camCount] = localMatrix->getUsed();

    // �ϊ��s��̕ۑ���
    GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + camCount + 1));

    // �ϊ��s���ۑ�����
    localMatrix->load(body);

    // ���t���[���̕ۑ��� (�ϊ��s��̍Ō�)
    uchar *const data(reinterpret_cast<uchar *>(body + head[camCount]));

    // �t���[���𑗐M����
    network.sendFrame(sendbuf, static_cast<unsigned int>(data - sendbuf));

    // ���̃X���b�h�����\�[�X�ɃA�N�Z�X���邽�߂ɏ����҂�
    std::this_thread::sleep_for(std::chrono::milliseconds(10L));
  }
}

// �����[�g�̃t���[���Ǝp������M����
void CamRemote::recv()
{
  // �X���b�h�����s�̊�
  while (run[camL])
  {
    // �w�b�_�̃t�H�[�}�b�g
    unsigned int *const head(reinterpret_cast<unsigned int *>(recvbuf));

    // 1 �t���[���󂯎����
    for (int i = 0;;)
    {
      const int ret(network.recvFrame(recvbuf, maxFrameSize));
      if (ret > 0 && head[camL] > 0) break;
      if (++i > retry) return;
    }

    // �ϊ��s��̕ۑ���
    GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + camCount + 1));

    // �ϊ��s��𕜋A����
    remoteMatrix->store(body, 0, head[camCount]);

    // �J�����̎p���𕜋A����
    queueRemoteAttitude(camL, body[camL]);
    queueRemoteAttitude(camR, body[camR]);

    // ���t���[���̕ۑ��� (�ϊ��s��̍Ō�)
    uchar *const data(reinterpret_cast<uchar *>(body + head[camCount]));

    // �����[�g����擾�����t���[���̃T�C�Y
    GLsizei rsize[camCount][2];

    // ���t���[���������Ă��Ă��č��o�b�t�@����̂Ƃ�
    if (head[camL] > 0 && !buffer[camL])
    {
      // ���t���[���f�[�^�� vector �ɕϊ�����
      encoded[camL].assign(data, data + head[camL]);

      // ���t���[�������b�N����
      captureMutex[camL].lock();

      // ���t���[�����f�R�[�h����
      remote[camL] = cv::imdecode(cv::Mat(encoded[camL]), 1);

      // ���摜���X�V��
      buffer[camL] = remote[camL].data;

      // ���t���[���̃T�C�Y�����߂�
      rsize[camL][0] = remote[camL].cols;
      rsize[camL][1] = remote[camL].rows;

      // ���t���[���̓]������������΃��b�N����������
      captureMutex[camL].unlock();
    }

    // �E�t���[���������Ă��Ă��ĉE�o�b�t�@����̂Ƃ�
    if (head[camR] > 0 && !buffer[camR])
    {
      // ���t���[�������݂����
      if (head[camL] > 0)
      {
        // �E�t���[���f�[�^�� vector �ɕϊ�����
        encoded[camR].assign(data + head[camL], data + head[camL] + head[camR]);

        // �E�t���[�������b�N����
        captureMutex[camR].lock();

        // �E�t���[�����f�R�[�h����
        remote[camR] = cv::imdecode(cv::Mat(encoded[camR]), 1);

        // �E�摜���X�V��
        buffer[camR] = remote[camR].data;

        // �E�t���[���̃T�C�Y�����߂�
        rsize[camR][0] = remote[camR].cols;
        rsize[camR][1] = remote[camR].rows;

        // �t���[���̓]������������΃��b�N����������
        captureMutex[camR].unlock();
      }
      else
      {
        // �E�t���[�������b�N����
        captureMutex[camR].lock();

        // �E�摜�͍��摜�Ɠ����ɂ���
        buffer[camR] = remote[camL].data;

        // �E�t���[���̃T�C�Y�͍��t���[���Ɠ����ɂ���
        rsize[camR][0] = rsize[camL][0];
        rsize[camR][1] = rsize[camL][1];

        // �t���[���̓]������������΃��b�N����������
        captureMutex[camR].unlock();
      }
    }
  }
}

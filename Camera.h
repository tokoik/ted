#pragma once

//
// �J�����֘A�̏���
//

// �e��ݒ�
#include "config.h"

// �l�b�g���[�N�֘A�̏���
#include "Network.h"

// OpenCV
#include <opencv2/highgui/highgui.hpp>

// �L���v�`����񓯊��ōs��
#include <thread>
#include <mutex>

// �L���[
#include <queue>

// �J�����̐��Ǝ��ʎq
const int camCount(2), camL(0), camR(1);

// ��Ɨp�������̃T�C�Y
const int workingMemorySize(1024 * 1024);

//
// �J�����֘A�̏�����S������N���X
//
class Camera
{
  // �R�s�[�R���X�g���N�^�𕕂���
  Camera(const Camera &c);

  // ����𕕂���
  Camera &operator=(const Camera &w);

protected:

  // �J�����X���b�h
  std::thread captureThread[camCount];

  // �~���[�e�b�N�X
  std::mutex captureMutex[camCount];

  // ���s���
  bool run[camCount];

  // �X���b�h���~����
  void stop();

  // �L���v�`�������摜
  const GLubyte *buffer[camCount];

  // �L���v�`�������摜�̃T�C�Y
  GLsizei size[camCount][2];

  // �L���v�`������摜�̃t�H�[�}�b�g
  GLenum format;

  // �I�o�Ɨ���
  int exposure, gain;

public:

  // �R���X�g���N�^
  Camera();

  // �f�X�g���N�^
  virtual ~Camera();

  // �摜�̕��𓾂�
  int getWidth(int cam) const
  {
    return size[cam][0];
  }

  // �摜�̍����𓾂�
  int getHeight(int cam) const
  {
    return size[cam][1];
  }

  // Ovrvision Pro �̘I�o���グ��
  virtual void increaseExposure() {};

  // Ovrvision Pro �̘I�o��������
  virtual void decreaseExposure() {};

  // Ovrvision Pro �̗������グ��
  virtual void increaseGain() {};

  // Ovrvision Pro �̗�����������
  virtual void decreaseGain() {};

  // �J���������b�N���ĉ摜���e�N�X�`���ɓ]������
  virtual bool transmit(int cam, GLuint texture, const GLsizei *size);

  //
  // �ʐM�֘A
  //

private:

  // �f�[�^���M
  void send();

  // �f�[�^��M
  void recv();

protected:

  // ���M�X���b�h
  std::thread sendThread;

  // ��M�X���b�h
  std::thread recvThread;

  // �J�����̎p��
  struct Attitude
  {
    // �R���X�g���N�^
    Attitude()
    {
    }

    // �R���X�g���N�^
    Attitude(GLfloat p0, GLfloat p1, GLfloat p2, GLfloat p3, const GgQuaternion &q)
    {
      position[0] = p0;
      position[1] = p1;
      position[2] = p2;
      position[3] = p3;
      orientation = q;
    }

    // �R���X�g���N�^
    Attitude(const GLfloat *p, const GgQuaternion &q)
      : Attitude(p[0], p[1], p[2], p[3], q) {}

    // ���_�̈ʒu
    GLfloat position[4];

    // �����̌���
    GgQuaternion orientation;
  };

  // ���[�J���J�����̎p��
  Attitude attitude[camCount];

  // ���[�J���J�����̃g���b�L���O����ۑ�����ۂɎg���~���[�e�b�N�X
  std::mutex attitudeMutex[camCount];

  // �����[�g�J�����̎p���̃^�C�~���O���t���[���ɍ��킹�Ēx�点�邽�߂̃L���[
  std::queue<Attitude> fifo[camCount];

  // �G���R�[�h�̃p�����[�^
  std::vector<int> param;

  // �G���R�[�h�����摜
  std::vector<uchar> encoded[camCount];

  // �t���[���̃��C�A�E�g
  struct Frame
  {
    // ���̃t���[���B�e���̃J�����̎p��
    Attitude attitude[camCount];

    // �t���[���f�[�^�̒���
    unsigned int length[camCount];

    // ��Ɨp�̃�����
    uchar data[workingMemorySize];
  };

  // ��Ɨp�̃�����
  Frame *frame;

  // �ʐM�f�[�^
  Network network;

public:

  // ���[�J���� Oculus Rift �̃g���b�L���O����ۑ�����
  void storeLocalAttitude(int eye, const Attitude &new_attitude)
  {
    if (attitudeMutex[eye].try_lock())
    {
      attitude[eye] = new_attitude;
      attitudeMutex[eye].unlock();
    }
  }

  // ���[�J���� Oculus Rift �̃g���b�L���O����ۑ�����
  void storeLocalAttitude(int eye, const GLfloat *position, const GgQuaternion &orientation)
  {
    storeLocalAttitude(eye, Attitude(position, orientation));
  }

  // ���[�J���� Oculus Rift �̃w�b�h���b�L���O�ɂ��ړ��𓾂�
  const Attitude &getLocalAttitude(int eye)
  {
    std::lock_guard<std::mutex> lock(attitudeMutex[eye]);
    return attitude[eye];
  }

  // �����[�g�� Oculus Rift �̃g���b�L���O����x��������
  void queueRemoteAttitude(int eye, const Attitude &new_attitude)
  {
    // �V�����g���b�L���O�f�[�^��ǉ�����
    fifo[eye].push(new_attitude);

    // �L���[�̒������x��������t���[������蒷����΃L���[��i�߂�
    if (fifo[eye].size() > defaults.tracking_delay[eye] + 1) fifo[eye].pop();
  }

  // �����[�g�� Oculus Rift �̃g���b�L���O����x��������
  void queueRemoteAttitude(int eye, const GLfloat *position, const GgQuaternion &orientation)
  {
    queueRemoteAttitude(eye, Attitude(position, orientation));
  }

  // �����[�g�� Oculus Rift �̃w�b�h���b�L���O�ɂ��ړ��𓾂�
  const Attitude &getRemoteAttitude(int eye)
  {
    static Attitude initialAttitude(0.0f, 0.0f, 0.0f, 1.0f, ggIdentityQuaternion());
    if (fifo[eye].empty()) return initialAttitude;
    return fifo[eye].front();
  }

  // ��ƎҒʐM�X���b�h�N��
  void startWorker(unsigned short port, const char *address);

  // �l�b�g���[�N���g���Ă��邩�ǂ���
  bool useNetwork() const
  {
    return network.running();
  }

  // ��Ǝ҂��ǂ���
  bool isWorker() const
  {
    return useNetwork() && network.isSender();
  }

  // ���c�҂����ǂ���
  bool isOperator() const
  {
    return useNetwork() && !network.isSender();
  }
};

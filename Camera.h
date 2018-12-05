#pragma once

//
// �J�����֘A�̏���
//

// �e��ݒ�
#include "config.h"

// �l�b�g���[�N�֘A�̏���
#include "Network.h"

// ���L������
#include "SharedMemory.h"

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
const int maxFrameSize(1024 * 1024);

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

  // �����[�g�J�����̎p���̃^�C�~���O���t���[���ɍ��킹�Ēx�点�邽�߂̃L���[
  std::queue<GgMatrix> fifo[camCount];

  // �G���R�[�h�̃p�����[�^
  std::vector<int> param;

  // �G���R�[�h�����摜
  std::vector<uchar> encoded[camCount];

  // �ϊ��s��̃e�[�u��
  SharedMemory *localMatrix, *remoteMatrix;

  // �f�[�^���M�p�̃�����
  char *sendbuf;
  
  // �f�[�^��M�p�̃�����
  char *recvbuf;

  // �ʐM�f�[�^
  Network network;

public:

  // ���f���ϊ��s��̃e�[�u����I������
  void selectTable(SharedMemory *local, SharedMemory *remote)
  {
    localMatrix = local;
    remoteMatrix = remote;
  }

  // �����[�g�� Oculus Rift �̃g���b�L���O����x��������
  void queueRemoteAttitude(int eye, const GgMatrix &new_attitude)
  {
    // �V�����g���b�L���O�f�[�^��ǉ�����
    fifo[eye].push(new_attitude);

    // �L���[�̒������x��������t���[������蒷����΃L���[��i�߂�
    if (fifo[eye].size() > defaults.tracking_delay[eye] + 1) fifo[eye].pop();
  }

  // �����[�g�� Oculus Rift �̃w�b�h���b�L���O�ɂ��ړ��𓾂�
  const GgMatrix &getRemoteAttitude(int eye)
  {
    if (fifo[eye].empty()) fifo[eye].push(ggIdentity());
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

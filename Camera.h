#pragma once

//
// �J�����֘A�̏���
//

// �e��ݒ�
#include "config.h"

// �l�b�g���[�N�֘A�̏���
#include "Network.h"

// OpenCV
#include <opencv2/opencv.hpp>

// �W�����C�u����
#include <thread>
#include <mutex>

// �w�b�_�̒���
constexpr int headLength{ camCount + 1 };

// ��Ɨp�������̃T�C�Y
constexpr int maxFrameSize{ 1024 * 1024 };

//
// �J�����֘A�̏�����S������N���X
//
class Camera
{
protected:

  // �J�����X���b�h
  std::thread captureThread[camCount];

  // �~���[�e�b�N�X
  std::mutex captureMutex[camCount];

  // ���s���
  bool run[camCount];

  // �X���b�h���~����
  void stop();

  // �L���v�`���f�o�C�X����擾�����摜
  cv::Mat image[camCount];

  // �L���v�`�������Ȃ� true
  bool captured[camCount];

  // ���҂���L���v�`���Ԋu
  double capture_interval;

  // ���ۂ̃L���v�`���Ԋu
  double interval[camCount];

  // �L���v�`������摜�̃t�H�[�}�b�g
  GLenum format;

  // �I�o�Ɨ���
  int exposure, gain;

public:

  // �R���X�g���N�^
  Camera(int quality = 95);

  // �R�s�[�R���X�g���N�^�𕕂���
  Camera(const Camera& c) = delete;

  // ����𕕂���
  Camera& operator=(const Camera& w) = delete;

  // �f�X�g���N�^
  virtual ~Camera();

  // �摜�̕��𓾂�
  int getWidth(int cam) const
  {
    return image[cam].cols;
  }

  // �摜�̍����𓾂�
  int getHeight(int cam) const
  {
    return image[cam].rows;
  }

  // �t���[�����[�g����L���v�`���Ԋu��ݒ肷��
  void setInterval(double fps)
  {
  	capture_interval = fps > 0.0 ? 1000.0 / fps : minDelay;
  }

  // ���k�ݒ�
  void setQuality(int quality);

  // Ovrvision Pro �̘I�o���グ��
  virtual void increaseExposure() {};

  // Ovrvision Pro �̘I�o��������
  virtual void decreaseExposure() {};

  // Ovrvision Pro �̗������グ��
  virtual void increaseGain() {};

  // Ovrvision Pro �̗�����������
  virtual void decreaseGain() {};

  // �J���������b�N���ĉ摜���e�N�X�`���ɓ]������
  virtual bool transmit(int cam, GLuint texture, const GLsizei* size);

  //
  // �ʐM�֘A
  //

private:

  // �����[�g�̎p������M����
  void recv();

  // ���[�J���̉f���Ǝp���𑗐M����
  void send();

protected:

  // ��M�X���b�h
  std::thread recvThread;

  // ���M�X���b�h
  std::thread sendThread;

  // �G���R�[�h�̃p�����[�^
  std::vector<int> param;

  // �G���R�[�h�����摜
  std::vector<uchar> encoded[camCount];

  // �f����M�p�̃�����
  uchar* recvbuf;

  // �f�����M�p�̃�����
  uchar* sendbuf;

  // �ʐM�f�[�^
  Network network;

public:

  // ��ƎҒʐM�X���b�h�N��
  int startWorker(unsigned short port, const char* address);

  // �l�b�g���[�N���g���Ă��邩�ǂ���
  bool useNetwork() const
  {
    return network.running();
  }

  // ��Ǝ҂��ǂ���
  bool isWorker() const
  {
    return network.isWorker();
  }

  // �w���҂����ǂ���
  bool isInstructor() const
  {
    return network.isInstructor();
  }
};

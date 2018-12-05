#pragma once

//
// OpenCV ���g�����L���v�`��
//

// �J�����֘A�̏���
#include "Camera.h"

// OpenCV ���g���ăL���v�`������N���X
class CamCv
  : public Camera
{
  // OpenCV �̃L���v�`���f�o�C�X
  cv::VideoCapture camera[camCount];

  // OpenCV �̃L���v�`���f�o�C�X����擾�����摜
  cv::Mat image[camCount];

  // ���݂̃t���[���̎���
  double frameTime[camCount];

  // �L���v�`���f�o�C�X���J�n����
  bool start(int cam);

  // �t���[�����L���v�`������
  void capture(int cam);

public:

  // �R���X�g���N�^
  CamCv();

  // �f�X�g���N�^
  virtual ~CamCv();

  // �J����������͂���
  bool open(int device, int cam)
  {
    // �J�������J���ăL���v�`�����J�n����
    return camera[cam].open(device) && start(cam);
  }

  // �t�@�C���^�l�b�g���[�N����L���v�`�����J�n����
  bool open(const std::string &file, int cam)
  {
    // �t�@�C���^�l�b�g���[�N���J��
    return camera[cam].open(file) && start(cam);
  }

  // �J�������g�p�\�����肷��
  bool opened(int cam)
  {
    return camera[cam].isOpened();
  }

  // �I�o���グ��
  virtual void increaseExposure();

  // �I�o��������
  virtual void decreaseExposure();

  // �������グ��
  virtual void increaseGain();

  // ������������
  virtual void decreaseGain();
};

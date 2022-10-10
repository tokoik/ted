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

  // �L���v�`���f�o�C�X����������
  void setup(int cam, const std::array<char, 4>& codec, const std::array<int, 2>& size, double fps);

  // �L���v�`���f�o�C�X���J�n����
  bool start(int cam);

  // �ŏ��̃t���[���̎���
  double startTime[camCount];

  // �t���[�����L���v�`������
  void capture(int cam);

public:

  // �R���X�g���N�^
  CamCv();

  // �f�X�g���N�^
  virtual ~CamCv();

  // �J����������͂���
  bool open(int device, int cam);

  // �t�@�C���^�l�b�g���[�N����L���v�`�����J�n����
  bool open(const std::string& file, int cam);

  // �J�������g�p�\�����肷��
  bool opened(int cam);

  // �I�o���グ��
  virtual void increaseExposure();

  // �I�o��������
  virtual void decreaseExposure();

  // �������グ��
  virtual void increaseGain();

  // ������������
  virtual void decreaseGain();
};

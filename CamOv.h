#pragma once

//
// Ovrvision Pro ���g�����L���v�`��
//

// �J�����֘A�̏���
#include "Camera.h"

// Ovrvision Pro
#include "ovrvision_pro.h"

// Ovrvision ���g���ăL���v�`������N���X
class CamOv
  : public Camera
{
  // Ovrvision Pro
  static OVR::OvrvisionPro* ovrvision_pro;

  // Ovrvision Pro �̔ԍ�
  int device;

  // �ڑ�����Ă��� Ovrvision Pro �̑䐔
  static int count;

  // Ovrvision Pro ������͂���
  void capture();

public:

  // �R���X�g���N�^
  CamOv();

  // �f�X�g���N�^
  virtual ~CamOv();

  // �J�������N������
  bool open(OVR::Camprop ovrvision_property);

  // �I�o���グ��
  virtual void increaseExposure();

  // �I�o��������
  virtual void decreaseExposure();

  // �������グ��
  virtual void increaseGain();

  // ������������
  virtual void decreaseGain();
};

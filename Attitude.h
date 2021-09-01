#pragma once
//
// �p�����
//

// �e��ݒ�
#include "config.h"

class Attitude
{
  // �V�[���̊�ʒu
  GgVector position;

  // �V�[���̏����ʒu
  GgVector initialPosition;

  // �V�[���̊�p��
  GgQuaternion orientation;

  // �V�[���̏����p��
  GgQuaternion initialOrientation;

  // �J�������Ƃ̎p���̕␳�l
  GgQuaternion qa[camCount];

  // �J���������̕␳�X�e�b�v
  static GgQuaternion qrStep[2];

  // �V�[���ɑ΂���œ_�����ƒ��S�ʒu�̕␳�l
  GgVector foreAdjust;

  // �w�i�ɑ΂���œ_�����ƒ��S�ʒu�̕␳�l
  GgVector backAdjust;

  // ����
  GLfloat parallax;

  // �����̏����l
  GLfloat initialParallax;
};


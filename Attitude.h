#pragma once
//
// �p�����
//

// �e��ݒ�
#include "Window.h"

// �p��
struct Attitude
{
  // �V�[���̊�ʒu
  GgVector position;

  // �V�[���̏����ʒu
  GgVector initialPosition;

  // �V�[���̊�p��
  GgTrackball orientation;

  // �V�[���̏����p��
  GgTrackball initialOrientation;

  // �V�[���ɑ΂���œ_�����ƒ��S�ʒu�̕␳�l
  //   0: focal length, 1: aspect ratio
  //   2: center in x,  3: center in y
  GgVector foreIntrinsic;

  // �V�[���ɑ΂���œ_�����ƒ��S�ʒu�̕␳�l�̏����l
  GgVector initialForeIntrinsic;

  // �w�i�ɑ΂���œ_�����ƒ��S�ʒu�̕␳�l
  //   0: focal length, 1: aspect ratio
  //   2: center in x,  3: center in y
  GgVector backIntrinsic;

  // �w�i�ɑ΂���œ_�����ƒ��S�ʒu�̕␳�l�̏����l
  GgVector initialBackIntrinsic;

  // ����
  GLfloat parallax;

  // �����̏����l
  GLfloat initialParallax;

  // �J�������Ƃ̎p���̕␳�l
  GgQuaternion eyeOrientation[camCount];

  // �J�������Ƃ̎p���̕␳�l�̏����l
  GgQuaternion initialEyeOrientation[camCount];

  // �J���������̕␳�X�e�b�v
  static GgQuaternion qrStep[2];

  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu
  GgVector circle;

  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu�̏����l
  GgVector initialCircle;

  // �X�N���[���̊Ԋu
  GLfloat offset;

  // �X�N���[���̊Ԋu�̏����l
  GLfloat initialOffset;

  // �R���X�g���N�^
  Attitude();

  // �f�X�g���N�^
  ~Attitude();

  // �p���̐ݒ�t�@�C����
  std::string attitude_file;

  // �p���� JSON �f�[�^�̓ǂݎ��
  bool read(picojson::value &v);

  // �p���̐ݒ�t�@�C���̓ǂݍ���
  bool load(const std::string &file);

  // �p���̐ݒ�t�@�C���̏�������
  bool save(const std::string &file) const;
};

// �p���f�[�^
extern Attitude attitude;

#pragma once
//
// �p�����
//

// �e��ݒ�
#include "Window.h"

// �p��
struct Attitude
{
  // �O�i�̊�ʒu
  GgVector position;

  // �O�i�̏����ʒu
  GgVector initialPosition;

  // �O�i�̊�p��
  GgTrackball orientation;

  // �O�i�̏����p��
  GgTrackball initialOrientation;

  // �J�������Ƃ̎p���̕␳�l
  GgQuaternion eyeOrientation[camCount];

  // �J�������Ƃ̎p���̕␳�l�̏����l
  GgQuaternion initialEyeOrientation[camCount];

  // �J���������̕␳�X�e�b�v
  static GgQuaternion eyeOrientationStep[2];

  // �����̕␳�l
  int parallax;

  // �����̕␳�l�̏����l
  int initialParallax;

  // �O�i�ɑ΂���œ_�����ƒ��S�ʒu�̕␳�l
  //   0: focal length, 1: aspect ratio
  //   2: center in x,  3: center in y
  std::array<int, 4> foreAdjust;

  // �O�i�ɑ΂���œ_�����ƒ��S�ʒu�̕␳�l�̏����l
  std::array<int, 4> initialForeAdjust;

  // �w�i�ɑ΂���œ_�����ƒ��S�ʒu�̕␳�l
  //   0: focal length, 1: aspect ratio
  //   2: center in x,  3: center in y
  std::array<int, 4> backAdjust;

  // �w�i�ɑ΂���œ_�����ƒ��S�ʒu�̕␳�l�̏����l
  std::array<int, 4> initialBackAdjust;

  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu�̕␳�l
  //   0: fov in x,     1: fov in y
  //   2: center in x,  3: center in y
  std::array<int, 4> circleAdjust;

  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu�̕␳�l�̏����l
  std::array<int, 4> initialCircleAdjust;

  // �X�N���[���̊Ԋu
  int offset;

  // �X�N���[���̊Ԋu�̏����l
  int initialOffset;

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

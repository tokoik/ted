#pragma once

//
// Leap Motion �֘A�̏���
//

// Leap Motion SDK
#include <LeapC.h>

// �⏕�v���O����
#include "gg.h"
using namespace gg;

// �W�����C�u����
#include <thread>
#include <mutex>

// �t���[���̕��
#undef LEAP_INTERPORATE_FRAME

// �֐߂̐�
constexpr int jointCount((2 + 5 * 4) * 2);

struct LeapListener
{
  // �R���X�g���N�^
  LeapListener();

  // �f�X�g���N�^
  virtual ~LeapListener();

  // Leap Motion �Ɛڑ�����
  const LEAP_CONNECTION *openConnection();

  // Leap Motion ����ؒf����
  void closeConnection();

  // Leap Motion �Ƃ̐ڑ���j������
  void destroyConnection();

#if defined(LEAP_INTERPORATE_FRAME)
  // Leap Motion �� CPU �̓������Ƃ�
  void synchronize();
#endif

  // �֐߂̕ϊ��s��̃e�[�u���ɒl���擾����
  void getHandPose(GgMatrix *matrix) const;

  // ���̎p���̕ϊ��s��̃e�[�u���ɒl���擾����
  void getHeadPose(GgMatrix *matrix) const;
};

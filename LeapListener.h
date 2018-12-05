#pragma once

// Leap Motion
#include "Leap.h"

// ���L������
#include "SharedMemory.h"

// �W�����C�u����
#include <array>

class LeapListener
  : public Leap::Listener
{
  // �ϊ��s��̃e�[�u��
  SharedMemory *const matrix;

  // �ϊ��e�[�u���̊i�[��̐擪
  const unsigned int begin;

  // �ϊ��s��̃C���f�b�N�X�̃e�[�u��
  std::array<GgMatrix, (2 + 5 * 4) * 2> jointMatrix;

  // Leap Motion �Ƃ̃C���^�t�F�[�X
  virtual void onInit(const Leap::Controller &controller);
  virtual void onConnect(const Leap::Controller &controller);
  virtual void onDisconnect(const Leap::Controller &controller);
  virtual void onExit(const Leap::Controller &controller);
  virtual void onFrame(const Leap::Controller &controller);
  virtual void onFocusGained(const Leap::Controller &controller);
  virtual void onFocusLost(const Leap::Controller &controller);
  virtual void onDeviceChange(const Leap::Controller &controller);
  virtual void onServiceConnect(const Leap::Controller &controller);
  virtual void onServiceDisconnect(const Leap::Controller &controller);
  virtual void onServiceChange(const Leap::Controller &controller);
  virtual void onDeviceFailure(const Leap::Controller &controller);
  virtual void onLogMessage(const Leap::Controller &controller,
    Leap::MessageSeverity severity, int64_t timestamp, const char *msg);

public:

  // �R���X�g���N�^
  LeapListener(SharedMemory *matrix);

  // �f�X�g���N�^
  virtual ~LeapListener();
};

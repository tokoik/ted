#pragma once

// Leap Motion
#include "Leap.h"

// 共有メモリ
#include "SharedMemory.h"

// 標準ライブラリ
#include <array>

class LeapListener
  : public Leap::Listener
{
  // 変換行列のテーブル
  SharedMemory *const matrix;

  // 変換行列のインデックスのテーブル
  std::array<unsigned int, (2 + 5 * 4) * 2> jointIndex;

  // Leap Motion とのインタフェース
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

  // コンストラクタ
  LeapListener(SharedMemory *matrix)
    : matrix(matrix)
  {
    for (auto &index : jointIndex) index = matrix->push(ggIdentity());
  }

  // デストラクタ
  virtual ~LeapListener() {}
};

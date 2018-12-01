#pragma once

//
// Leap Motion 関連の処理
//

// Leap Motion SDK
#include <LeapC.h>

// 補助プログラム
#include "gg.h"
using namespace gg;

// 標準ライブラリ
#include <thread>
#include <mutex>

// フレームの補間
#undef LEAP_INTERPORATE_FRAME

// 関節の数
constexpr int jointCount((2 + 5 * 4) * 2);

struct LeapListener
{
  // コンストラクタ
  LeapListener();

  // デストラクタ
  virtual ~LeapListener();

  // Leap Motion と接続する
  const LEAP_CONNECTION *openConnection();

  // Leap Motion から切断する
  void closeConnection();

  // Leap Motion との接続を破棄する
  void destroyConnection();

#if defined(LEAP_INTERPORATE_FRAME)
  // Leap Motion と CPU の同期をとる
  void synchronize();
#endif

  // 関節の変換行列のテーブルに値を取得する
  void getHandPose(GgMatrix *matrix) const;

  // 頭の姿勢の変換行列のテーブルに値を取得する
  void getHeadPose(GgMatrix *matrix) const;
};

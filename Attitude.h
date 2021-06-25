#pragma once
//
// 姿勢情報
//

// 各種設定
#include "config.h"

class Attitude
{
  // シーンの基準位置
  GgVector position;

  // シーンの初期位置
  GgVector initialPosition;

  // シーンの基準姿勢
  GgQuaternion orientation;

  // シーンの初期姿勢
  GgQuaternion initialOrientation;

  // カメラごとの姿勢の補正値
  GgQuaternion qa[camCount];

  // カメラ方向の補正ステップ
  static GgQuaternion qrStep[2];

  // シーンに対する焦点距離と中心位置の補正値
  GgVector foreAdjust;

  // 背景に対する焦点距離と中心位置の補正値
  GgVector backAdjust;

  // 視差
  GLfloat parallax;

  // 視差の初期値
  GLfloat initialParallax;
};


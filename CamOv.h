#pragma once

//
// Ovrvision Pro を使ったキャプチャ
//

// カメラ関連の処理
#include "Camera.h"

// Ovrvision Pro
#include "ovrvision_pro.h"

// Ovrvision を使ってキャプチャするクラス
class CamOv
  : public Camera
{
  // Ovrvision Pro
  static OVR::OvrvisionPro* ovrvision_pro;

  // Ovrvision Pro の番号
  int device;

  // 接続されている Ovrvision Pro の台数
  static int count;

  // Ovrvision Pro から入力する
  void capture();

public:

  // コンストラクタ
  CamOv();

  // デストラクタ
  virtual ~CamOv();

  // カメラを起動する
  bool open(OVR::Camprop ovrvision_property);

  // 露出を上げる
  virtual void increaseExposure();

  // 露出を下げる
  virtual void decreaseExposure();

  // 利得を上げる
  virtual void increaseGain();

  // 利得を下げる
  virtual void decreaseGain();
};

#pragma once

///
/// Ovrvision を使ってキャプチャするクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///

// カメラ関連の処理
#include "Camera.h"

// Ovrvision Pro
#include "ovrvision_pro.h"

///
/// Ovrvision を使ってキャプチャするクラス
///
class CamOv
  : public Camera
{
  /// Ovrvision Pro
  static OVR::OvrvisionPro* ovrvision_pro;

  /// Ovrvision Pro の番号
  int device;

  /// 接続されている Ovrvision Pro の台数
  static int count;

  /// Ovrvision Pro から入力する
  void capture();

public:

  ///
  /// コンストラクタ
  ///
  CamOv();

  ///
  /// デストラクタ
  ///
  virtual ~CamOv();

  ///
  /// Ovrvision Pro を起動する
  ///
  /// @param ovrvision_property Ovrvision Pro のプロパティ
  /// @return 成功した場合は true
  ///
  bool open(OVR::Camprop ovrvision_property);

  ///
  /// Ovrvision Pro の露出を上げる
  ///
  virtual void increaseExposure();

  ///
  /// Ovrvision Pro の露出を下げる
  ///
  virtual void decreaseExposure();

  ///
  /// Ovrvision Pro の利得を上げる
  ///
  virtual void increaseGain();

  ///
  /// Ovrvision Pro の利得を下げる
  ///
  virtual void decreaseGain();
};

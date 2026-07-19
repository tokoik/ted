#pragma once

///
/// 姿勢情報クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 197, 2026
///

// 各種設定
#include "Config.h"

using namespace gg;

///
/// 姿勢情報を保持する構造体
/// 
/// @details
/// 実行中に調整する視点・投影補正と、そのリセット先となる初期値を対で保持する。
/// attitude.json に保存し、通常表示と OpenXR 表示の双方で同じ補正値を使用する。
///
struct Attitude
{
  /// 前景の基準位置
  GgVector position{ 0.0f, 0.0f, 0.0f, 1.0f };

  // 前景の初期位置
  GgVector initialPosition{ 0.0f, 0.0f, 0.0f, 1.0f };

  /// 前景の基準姿勢
  GgTrackball orientation;

  // 前景の初期姿勢
  GgTrackball initialOrientation;

  /// カメラごとの姿勢の補正値
  GgQuaternion eyeOrientation[camCount]
  {
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
  };

  // カメラごとの姿勢の補正値の初期値
  GgQuaternion initialEyeOrientation[camCount]
  {
    { 0.0f, 0.0f, 0.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
  };

  /// カメラ方向の補正ステップ
  static GgQuaternion eyeOrientationStep[2];

  /// 視差の補正値
  int parallax{ 0 };

  /// 視差の補正値の初期値
  int initialParallax{ 0 };

  /// 前景に対する焦点距離と中心位置の補正値
  ///
  /// @details
  /// 0: focal length, 1: aspect ratio
  /// 2: center in x,  3: center in y
  ///
  std::array<int, 4> foreAdjust{ 0, 0, 0, 0 };

  /// 前景に対する焦点距離と中心位置の補正値の初期値
  ///
  /// @details
  /// 0: focal length, 1: aspect ratio
  /// 2: center in x,  3: center in y
  ///
  std::array<int, 4> initialForeAdjust{ 0, 0, 0, 0 };

  /// 背景に対する焦点距離と中心位置の補正値
  ///
  /// @details
  /// 0: focal length, 1: aspect ratio
  /// 2: center in x,  3: center in y
  ///
  std::array<int, 4> backAdjust{ 0, 0, 0, 0 };

  /// 背景に対する焦点距離と中心位置の補正値の初期値
  ///
  /// @details
  /// 0: focal length, 1: aspect ratio
  /// 2: center in x,  3: center in y
  ///
  std::array<int, 4> initialBackAdjust{ 0, 0, 0, 0 };

  /// 背景テクスチャの半径と中心位置の補正値
  ///
  /// @details
  /// 0: fov in x,     1: fov in y
  /// 2: center in x,  3: center in y
  ///
  std::array<int, 4> circleAdjust{ 0, 0, 0, 0 };

  /// 背景テクスチャの半径と中心位置の補正値の初期値
  ///
  /// @details
  /// 0: fov in x,     1: fov in y
  /// 2: center in x,  3: center in y
  ///
  std::array<int, 4> initialCircleAdjust{ 0, 0, 0, 0 };

  /// スクリーンの間隔
  int offset{ 0 };

  /// スクリーンの間隔の初期値
  int initialOffset{ 0 };

  /// 姿勢の設定ファイル名
  std::string attitude_file;

  ///
  /// コンストラクタ
  ///
  Attitude();

  ///
  /// デストラクタ
  ///
  ~Attitude();

  ///
  /// 姿勢の JSON データの読み取り
  ///
  /// @param v JSON データを格納する picojson::value 型の変数
  /// @return JSON データの読み取りに成功した場合は true
  ///
  bool read(picojson::value &v);

  ///
  /// 姿勢の設定ファイルの読み込み
  ///
  /// @param file 姿勢の設定ファイル名
  /// @return 姿勢の設定ファイルの読み込みに成功した場合は true
  ///
  bool load(const std::string &file);

  ///
  /// 姿勢の設定ファイルの書き込み
  ///
  /// @param file 姿勢の設定ファイル名
  /// @return 姿勢の設定ファイルの書き込みに成功した場合は true
  ///
  bool save(const std::string &file) const;
};

/// 姿勢データ
extern Attitude attitude;

#pragma once
//
// 姿勢情報
//

// 各種設定
#include "Window.h"

// 姿勢
struct Attitude
{
  // 前景の基準位置
  GgVector position;

  // 前景の初期位置
  GgVector initialPosition;

  // 前景の基準姿勢
  GgTrackball orientation;

  // 前景の初期姿勢
  GgTrackball initialOrientation;

  // カメラごとの姿勢の補正値
  GgQuaternion eyeOrientation[camCount];

  // カメラごとの姿勢の補正値の初期値
  GgQuaternion initialEyeOrientation[camCount];

  // カメラ方向の補正ステップ
  static GgQuaternion eyeOrientationStep[2];

  // 視差の補正値
  int parallax;

  // 視差の補正値の初期値
  int initialParallax;

  // 前景に対する焦点距離と中心位置の補正値
  //   0: focal length, 1: aspect ratio
  //   2: center in x,  3: center in y
  std::array<int, 4> foreAdjust;

  // 前景に対する焦点距離と中心位置の補正値の初期値
  std::array<int, 4> initialForeAdjust;

  // 背景に対する焦点距離と中心位置の補正値
  //   0: focal length, 1: aspect ratio
  //   2: center in x,  3: center in y
  std::array<int, 4> backAdjust;

  // 背景に対する焦点距離と中心位置の補正値の初期値
  std::array<int, 4> initialBackAdjust;

  // 背景テクスチャの半径と中心位置の補正値
  //   0: fov in x,     1: fov in y
  //   2: center in x,  3: center in y
  std::array<int, 4> circleAdjust;

  // 背景テクスチャの半径と中心位置の補正値の初期値
  std::array<int, 4> initialCircleAdjust;

  // スクリーンの間隔
  int offset;

  // スクリーンの間隔の初期値
  int initialOffset;

  // コンストラクタ
  Attitude();

  // デストラクタ
  ~Attitude();

  // 姿勢の設定ファイル名
  std::string attitude_file;

  // 姿勢の JSON データの読み取り
  bool read(picojson::value &v);

  // 姿勢の設定ファイルの読み込み
  bool load(const std::string &file);

  // 姿勢の設定ファイルの書き込み
  bool save(const std::string &file) const;
};

// 姿勢データ
extern Attitude attitude;

#pragma once
//
// 姿勢情報
//

// 各種設定
#include "Window.h"

// 姿勢
struct Attitude
{
  // シーンの基準位置
  GgVector position;

  // シーンの初期位置
  GgVector initialPosition;

  // シーンの基準姿勢
  GgTrackball orientation;

  // シーンの初期姿勢
  GgTrackball initialOrientation;

  // シーンに対する焦点距離と中心位置の補正値
  //   0: focal length, 1: aspect ratio
  //   2: center in x,  3: center in y
  GgVector foreIntrinsic;

  // シーンに対する焦点距離と中心位置の補正値の初期値
  GgVector initialForeIntrinsic;

  // 背景に対する焦点距離と中心位置の補正値
  //   0: focal length, 1: aspect ratio
  //   2: center in x,  3: center in y
  GgVector backIntrinsic;

  // 背景に対する焦点距離と中心位置の補正値の初期値
  GgVector initialBackIntrinsic;

  // 視差
  GLfloat parallax;

  // 視差の初期値
  GLfloat initialParallax;

  // カメラごとの姿勢の補正値
  GgQuaternion eyeOrientation[camCount];

  // カメラごとの姿勢の補正値の初期値
  GgQuaternion initialEyeOrientation[camCount];

  // カメラ方向の補正ステップ
  static GgQuaternion qrStep[2];

  // 背景テクスチャの半径と中心位置
  GgVector circle;

  // 背景テクスチャの半径と中心位置の初期値
  GgVector initialCircle;

  // スクリーンの間隔
  GLfloat offset;

  // スクリーンの間隔の初期値
  GLfloat initialOffset;

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

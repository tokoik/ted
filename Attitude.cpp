///
/// 姿勢情報クラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date July 197, 2026
///
#include "Attitude.h"

// 構成ファイルの読み取り補助
#include "parseconfig.h"

// 標準ライブラリ
#include <fstream>

// 姿勢データ
Attitude attitude;

// カメラ方向の補正ステップ
GgQuaternion Attitude::eyeOrientationStep[2];

//
// コンストラクタ
//
Attitude::Attitude()
{
  // カメラ方向の補正ステップ
  static bool firstTime{ true };
  if (firstTime)
  {
    // カメラ方向の調整ステップを求める
    eyeOrientationStep[0].loadRotate(0.0f, 1.0f, 0.0f, 0.001f);
    eyeOrientationStep[1].loadRotate(1.0f, 0.0f, 0.0f, 0.001f);
    firstTime = false;
  }
}

//
// 姿勢の JSON データの読み取り
//
bool Attitude::read(picojson::value &v)
{
  // 設定内容のパース
  picojson::object &o(v.get<picojson::object>());
  if (o.empty()) return false;

  // 初期位置
  getVector(o, "position", initialPosition);

  // 初期姿勢
  getVector(o, "orientation", initialOrientation);

  // カメラ方向の補正値
  const auto &v_parallax_offset{ o.find("parallax_offset") };
  if (v_parallax_offset != o.end() && v_parallax_offset->second.is<picojson::array>())
  {
    picojson::array &a(v_parallax_offset->second.get<picojson::array>());
    for (int eye = 0; eye < camCount; ++eye)
    {
      std::size_t count{std::min(a.size(), initialEyeOrientation[eye].size()) };
      for (std::size_t i = 0; i < count; ++i)
        initialEyeOrientation[eye][i] = static_cast<GLfloat>(a[eye * 4 + i].get<double>());
    }
  }

  // 視差の初期値
  getValue(o, "parallax", initialParallax);

  // 前景の焦点距離・縦横比・中心位置の初期値
  getValue(o, "fore_intrinsic", initialForeAdjust);

  // 背景の焦点距離・縦横比・中心位置の初期値
  getValue(o, "back_intrinsic", initialBackAdjust);

  // 背景テクスチャの半径と中心位置の初期値
  getValue(o, "circle", initialCircleAdjust);

  // スクリーンの間隔の初期値
  getValue(o, "offset", initialOffset);

  return true;
}

//
// 姿勢の設定ファイルの読み込み
//
bool Attitude::load(const std::string &file)
{
  // 読み込んだ設定ファイル名を覚えておく
  attitude_file = file;

  // 設定ファイルを開く
  std::ifstream attitude(file);
  if (!attitude) return false;

  // 設定ファイルを読み込む
  picojson::value v;
  attitude >> v;
  attitude.close();

  // 設定を解析する
  return read(v);
}

// 姿勢の設定ファイルの書き込み
bool Attitude::save(const std::string &file) const
{
  // 設定値を保存する
  std::ofstream attitude(file);
  if (!attitude) return false;

  // オブジェクト
  picojson::object o;

  // 位置
  setVector(o, "position", position);

  // 姿勢
  setVector(o, "orientation", initialOrientation);

  // 前景に対する焦点距離と中心位置
  setValue(o, "fore_intrinsic", foreAdjust);

  // 背景に対する焦点距離と中心位置
  setValue(o, "back_intrinsic", backAdjust);

  // 視差
  setValue(o, "parallax", parallax);

  // カメラ方向の補正値
  picojson::array e;
  for (int eye = 0; eye < camCount; ++eye)
    for (int i = 0; i < 4; ++i)
      e.push_back(picojson::value(static_cast<double>(initialEyeOrientation[eye].data()[i])));
  o.insert(std::make_pair("parallax_offset", picojson::value(e)));

  // 背景テクスチャの半径と中心位置
  setValue(o, "circle", circleAdjust);

  // スクリーンの間隔
  setValue(o, "offset", offset);

  // 設定内容をシリアライズして保存
  picojson::value v(o);
  attitude << v.serialize(true);
  attitude.close();

  return true;
}

//
// デストラクタ
//
Attitude::~Attitude()
{
}

//
// 姿勢情報
//
#include "Attitude.h"

// 標準ライブラリ
#include <fstream>

// 姿勢データ
Attitude attitude;

//
// コンストラクタ
//
Attitude::Attitude()
  : position{ 0.0f, 0.0f, 0.0f, 1.0f }
  , initialPosition{ 0.0f, 0.0f, 0.0f, 1.0f }
  , parallax{ 0 }
  , initialParallax{ 0 }
  , foreAdjust{ 0, 0, 0, 0 }
  , initialForeAdjust{ 0, 0, 0, 0 }
  , backAdjust{ 0, 0, 0, 0 }
  , initialBackAdjust{ 0, 0, 0, 0 }
  , circleAdjust{ 0, 0, 0, 0 }
  , initialCircleAdjust{ 0, 0, 0, 0 }
  , offset{ 0 }
  , initialOffset{ 0 }
{
  // カメラごとの姿勢の補正値
  for (auto &o : eyeOrientation)
  {
    o[0] = o[1] = o[2] = 0.0f;
    o[3] = 1.0f;
  }

  // カメラごとの姿勢の補正値の初期値
  for (auto &o : initialEyeOrientation)
  {
    o[0] = o[1] = o[2] = 0.0f;
    o[3] = 1.0f;
  }

  // カメラ方向の補正ステップ
  static bool firstTime{ true };
  if (firstTime)
  {
    // カメラ方向の調整ステップを求める
    eyeOrientationStep[0].loadRotate(0.0f, 1.0f, 0.0f, 0.001f);
    eyeOrientationStep[1].loadRotate(1.0f, 0.0f, 0.0f, 0.001f);
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
  const auto &v_position(o.find("position"));
  if (v_position != o.end() && v_position->second.is<picojson::array>())
  {
    picojson::array &p(v_position->second.get<picojson::array>());
    std::size_t count{ std::min(p.size(), initialPosition.size()) };
    for (std::size_t i = 0; i < count; ++i)
      initialPosition[i] = static_cast<GLfloat>(p[i].get<double>());
  }

  // 初期姿勢
  const auto &v_orientation(o.find("orientation"));
  if (v_orientation != o.end() && v_orientation->second.is<picojson::array>())
  {
    picojson::array &a(v_orientation->second.get<picojson::array>());
    std::size_t count{ std::min(a.size(), initialOrientation.size()) };
    for (std::size_t i = 0; i < count; ++i)
      initialOrientation[i] = static_cast<GLfloat>(a[i].get<double>());
  }

  // カメラ方向の補正値
  const auto &v_parallax_offset(o.find("parallax_offset"));
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
  const auto &v_parallax(o.find("parallax"));
  if (v_parallax != o.end() && v_parallax->second.is<double>())
    initialParallax = static_cast<int>(v_parallax->second.get<double>());

  // 前景の焦点距離・縦横比・中心位置の初期値
  const auto &v_fore_intrisic(o.find("fore_intrinsic"));
  if (v_fore_intrisic != o.end() && v_fore_intrisic->second.is<picojson::array>())
  {
    picojson::array &c(v_fore_intrisic->second.get<picojson::array>());
    std::size_t count{ std::min(c.size(), initialForeAdjust.size()) };
    for (std::size_t i = 0; i < count; ++i)
      initialForeAdjust[i] = static_cast<int>(c[i].get<double>());
  }

  // 背景の焦点距離・縦横比・中心位置の初期値
  const auto &v_back_intrisic(o.find("back_intrinsic"));
  if (v_back_intrisic != o.end() && v_back_intrisic->second.is<picojson::array>())
  {
    picojson::array &c(v_back_intrisic->second.get<picojson::array>());
    std::size_t count{ std::min(c.size(), initialBackAdjust.size()) };
    for (std::size_t i = 0; i < count; ++i) 
      initialBackAdjust[i] = static_cast<int>(c[i].get<double>());
  }

  // 背景テクスチャの半径と中心位置の初期値
  const auto &v_circle(o.find("circle"));
  if (v_circle != o.end() && v_circle->second.is<picojson::array>())
  {
    picojson::array &c(v_circle->second.get<picojson::array>());
    std::size_t count{ std::min(c.size(), initialCircleAdjust.size()) };
    for (std::size_t i = 0; i < count; ++i)
      initialCircleAdjust[i] = static_cast<int>(c[i].get<double>());
  }

  // スクリーンの間隔の初期値
  const auto &v_offset(o.find("offset"));
  if (v_offset != o.end() && v_offset->second.is<double>())
    initialOffset = static_cast<int>(v_offset->second.get<double>());

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
  picojson::array p;
  for (int i = 0; i < 4; ++i) p.push_back(picojson::value(static_cast<double>(position[i])));
  o.insert(std::make_pair("position", picojson::value(p)));

  // 姿勢
  picojson::array a;
  for (int i = 0; i < 4; ++i) a.push_back(picojson::value(static_cast<double>(initialOrientation[i])));
  o.insert(std::make_pair("orientation", picojson::value(a)));

  // 前景に対する焦点距離と中心位置
  picojson::array f;
  for (int i = 0; i < 4; ++i) f.push_back(picojson::value(static_cast<double>(foreAdjust[i])));
  o.insert(std::make_pair("fore_intrinsic", picojson::value(f)));

  // 背景に対する焦点距離と中心位置
  picojson::array b;
  for (int i = 0; i < 4; ++i) b.push_back(picojson::value(static_cast<double>(backAdjust[i])));
  o.insert(std::make_pair("back_intrinsic", picojson::value(b)));

  // 視差
  o.insert(std::make_pair("parallax", picojson::value(static_cast<double>(parallax))));

  // カメラ方向の補正値
  picojson::array e;
  for (int eye = 0; eye < camCount; ++eye)
    for (int i = 0; i < 4; ++i)
      e.push_back(picojson::value(static_cast<double>(initialEyeOrientation[eye].data()[i])));
  o.insert(std::make_pair("parallax_offset", picojson::value(e)));

  // 背景テクスチャの半径と中心位置
  picojson::array c;
  for (int i = 0; i < 4; ++i) c.push_back(picojson::value(static_cast<double>(circleAdjust[i])));
  o.insert(std::make_pair("circle", picojson::value(c)));

  // スクリーンの間隔
  o.insert(std::make_pair("offset", picojson::value(static_cast<double>(offset))));

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

// カメラ方向の補正ステップ
GgQuaternion Attitude::eyeOrientationStep[2];

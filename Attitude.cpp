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
  , foreIntrinsic{ 1.0f, 1.0f, 0.0f, 0.0f }
  , initialForeIntrinsic{ 1.0f, 1.0f, 0.0f, 0.0f }
  , backIntrinsic{ 1.0f, 1.0f, 0.0f, 0.0f }
  , initialBackIntrinsic{ 1.0f, 1.0f, 0.0f, 0.0f }
  , parallax{ 0.065f }
  , initialParallax{ 0.065f }
{
  // カメラごとの姿勢の補正値の初期値
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
    qrStep[0].loadRotate(0.0f, 1.0f, 0.0f, 0.001f);
    qrStep[1].loadRotate(1.0f, 0.0f, 0.0f, 0.001f);
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
    for (int i = 0; i < 4; ++i) initialPosition[i] = static_cast<GLfloat>(p[i].get<double>());
  }

  // 初期姿勢
  const auto &v_orientation(o.find("orientation"));
  if (v_orientation != o.end() && v_orientation->second.is<picojson::array>())
  {
    picojson::array &a(v_orientation->second.get<picojson::array>());
    for (int i = 0; i < 4; ++i) initialOrientation[i] = static_cast<GLfloat>(a[i].get<double>());
  }

  // シーンの焦点距離・縦横比・中心位置の初期値
  const auto &v_fore_intrisic(o.find("fore_intrinsic"));
  if (v_fore_intrisic != o.end() && v_fore_intrisic->second.is<picojson::array>())
  {
    picojson::array &c(v_fore_intrisic->second.get<picojson::array>());
    for (int i = 0; i < 4; ++i) initialForeIntrinsic[i] = static_cast<GLfloat>(c[i].get<double>());
  }

  // 背景の焦点距離・縦横比・中心位置の初期値
  const auto &v_back_intrisic(o.find("back_intrinsic"));
  if (v_back_intrisic != o.end() && v_back_intrisic->second.is<picojson::array>())
  {
    picojson::array &c(v_back_intrisic->second.get<picojson::array>());
    for (int i = 0; i < 4; ++i) initialBackIntrinsic[i] = static_cast<GLfloat>(c[i].get<double>());
  }

  // 視差の初期値
  const auto &v_parallax(o.find("parallax"));
  if (v_parallax != o.end() && v_parallax->second.is<double>())
    initialParallax = static_cast<GLfloat>(v_parallax->second.get<double>());

  // カメラ方向の補正値
  const auto &v_parallax_offset(o.find("parallax_offset"));
  if (v_parallax_offset != o.end() && v_parallax_offset->second.is<picojson::array>())
  {
    picojson::array &a(v_parallax_offset->second.get<picojson::array>());
    for (int eye = 0; eye < camCount; ++eye)
    {
      GLfloat q[4];
      for (int i = 0; i < 4; ++i) q[i] = static_cast<GLfloat>(a[eye * 4 + i].get<double>());
      initialEyeOrientation[eye] = GgQuaternion(q);
    }
  }

  // 背景テクスチャの半径と中心位置の初期値
  const auto &v_circle(o.find("circle"));
  if (v_circle != o.end() && v_circle->second.is<picojson::array>())
  {
    picojson::array &c(v_circle->second.get<picojson::array>());
    for (int i = 0; i < 4; ++i) initialCircle[i] = static_cast<GLfloat>(c[i].get<double>());
  }

  // スクリーンの間隔の初期値
  const auto &v_offset(o.find("offset"));
  if (v_offset != o.end() && v_offset->second.is<double>())
    initialOffset = static_cast<GLfloat>(v_offset->second.get<double>());

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
  for (int i = 0; i < 4; ++i) p.push_back(picojson::value(position[i]));
  o.insert(std::make_pair("position", picojson::value(p)));

  // 姿勢
  picojson::array a;
  for (int i = 0; i < 4; ++i) a.push_back(picojson::value(initialOrientation[i]));
  o.insert(std::make_pair("orientation", picojson::value(a)));

  // シーンに対する焦点距離と中心位置
  picojson::array f;
  for (int i = 0; i < 4; ++i) f.push_back(picojson::value(foreIntrinsic[i]));
  o.insert(std::make_pair("fore_intrinsic", picojson::value(f)));

  // 背景に対する焦点距離と中心位置
  picojson::array b;
  for (int i = 0; i < 4; ++i) f.push_back(picojson::value(backIntrinsic[i]));
  o.insert(std::make_pair("back_intrinsic", picojson::value(b)));

  // 視差
  o.insert(std::make_pair("parallax", picojson::value(static_cast<double>(parallax))));

  // カメラ方向の補正値
  picojson::array e;
  for (int eye = 0; eye < camCount; ++eye)
    for (int i = 0; i < 4; ++i)
      e.push_back(picojson::value(static_cast<double>(initialEyeOrientation[eye].get()[i])));
  o.insert(std::make_pair("parallax_offset", picojson::value(e)));

  // 背景テクスチャの半径と中心位置
  picojson::array c;
  for (int i = 0; i < 4; ++i) c.push_back(picojson::value(static_cast<double>(circle[i])));
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
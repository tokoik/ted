#pragma once

//
// 各種設定
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// JSON
#include "picojson.h"

// ウィンドウのタイトル
constexpr char windowTitle[]{ "TED" };

// ウィンドウモード時のウィンドウサイズの初期値
constexpr int defaultWindowWidth{ 960 };
constexpr int defaultWindowHeight{ 540 };

// ナビゲーションの速度調整
constexpr GLfloat speedScale{ 0.005f };     // フレームあたりの移動速度係数
constexpr GLfloat angleScale{ -0.05f };     // フレームあたりの回転速度係数
constexpr GLfloat wheelXStep{ 0.005f };     // マウスホイールの X 方向の係数
constexpr GLfloat wheelYStep{ 0.005f };     // マウスホイールの Y 方向の係数
constexpr GLfloat axesSpeedScale{ 0.01f };  // ゲームパッドのスティックの速度の係数
constexpr GLfloat axesAngleScale{ 0.01f };  // ゲームパッドのスティックの角速度の係数
constexpr GLfloat btnsScale{ 0.02f };       // ゲームパッドのボタンの係数

// ズーム率の変更ステップ
constexpr GLfloat zoomStep{ 0.01f };

// 視差のデフォルト値
constexpr GLfloat defaultParallax{ 0.032f };

// 視差の変更ステップ (単位 m)
constexpr GLfloat parallaxStep{ 0.001f };

// 前景に対する焦点距離の変更ステップ
constexpr GLfloat foreFocalStep{ 0.001f };

// 前景に対する縦横比の変更ステップ
constexpr GLfloat foreAspectStep{ 0.001f };

// 背景に対する焦点距離の変更ステップ
constexpr GLfloat backFocalStep{ 0.001f };

// 背景に対する縦横比の変更ステップ
constexpr GLfloat backAspectStep{ 0.001f };

// レンズの画角の変更ステップ
constexpr GLfloat fovStep{ 0.001f };

// レンズの位置の変更ステップ
constexpr GLfloat shiftStep{ 0.001f };

// スクリーンの間隔のデフォルト値
constexpr GLfloat offsetDefault{ 0.0f };

// スクリーンの間隔の変更ステップ
constexpr GLfloat offsetStep{ 0.001f };

// マルチサンプリングのサンプル数 (Oculus Rift)
constexpr int backBufferMultisample{ 0 };

// 光源
constexpr GgSimpleShader::Light lightData
{
  { 0.2f, 0.2f, 0.2f, 1.0f },               // 環境光成分
  { 1.0f, 1.0f, 1.0f, 0.0f },               // 拡散反射光成分
  { 1.0f, 1.0f, 1.0f, 0.0f },               // 鏡面光成分
  { 0.0f, 0.0f, 1.0f, 1.0f }                // 位置
};

// テクスチャの境界色
constexpr GLfloat borderColor[]{ 0.0f, 0.0f, 0.0f, 1.0f };

// エンコード方法
constexpr char encoderType[]{ ".jpg" };

// 受信リトライ回数
constexpr int receiveRetry{ 30 };

// 読み飛ばすパケットの最大数
constexpr int maxDropPackets{ 1000 };

// フレーム送信の最小間隔
constexpr long long minDelay{ 10 };

// カメラの識別子と数
enum CameraId { camL = 0, camR, camCount };

// リモートカメラの数
constexpr int remoteCamCount{ camCount };

// 目の数
constexpr int eyeCount{ camCount };

// ローカルの共有メモリのサイズ
constexpr int localShareSize{ 64 };

// リモートの共有メモリのサイズ
constexpr int remoteShareSize{ 64 };

// ファイルマッピングオブジェクト名
constexpr wchar_t* localMutexName{ L"TED_LOCAL_MUTEX" };
constexpr wchar_t* localShareName{ L"TED_LOCAL_SHARE" };
constexpr wchar_t* remoteMutexName{ L"TED_REMOTE_MUTEX" };
constexpr wchar_t* remoteShareName{ L"TED_REMOTE_SHARE" };

// 設定ファイル名
constexpr char defaultConfig[]{ "config.json" };

// 姿勢ファイル名
constexpr char defaultAttitude[]{ "attitude.json" };

// 表示の設定
enum DisplayMode
{
  MONOCULAR = 0,            // 単眼視
  INTERLACE,                // インターレース（未実装）
  TOP_AND_BOTTOM,           // 上下２分割
  SIDE_BY_SIDE,             // 左右２分割
  QUADBUFFER,               // クワッドバッファステレオ
  OCULUS                    // Oculus Rift (HMD)
};

// 入力の設定
enum InputMode
{
  IMAGE = 0,                // 静止画
  MOVIE,                    // 動画
  CAMERA,                   // Web カメラ
  OVRVISION,                // Ovrvision Pro
  REALSENSE,                // RealSense
  REMOTE                    // リモートの TED
};

// 役割
enum Role
{
  STANDALONE = 0,           // 単独
  INSTRUCTOR,               // 指示者
  WORKER                    // 作業者
};

// 設定値
struct Config
{
  // 画面表示のモード
  int display_mode;

  // クワッドバッファステレオ表示を行うとき true
  bool display_quadbuffer;

  // フルスクリーン表示を行うとき true
  bool display_fullscreen;

  // フルスクリーン表示するディスプレイの番号
  int display_secondary;

  // 画面の解像度
  std::array<int, 2> display_size;

  // 画面の縦横比
  GLfloat display_aspect;

  // 画面の中心の高さ
  GLfloat display_center;

  // 画面までの距離
  GLfloat display_distance;

  // 視点から前方面までの距離
  GLfloat display_near;

  // 視点から後方面までの距離
  GLfloat display_far;

  // 入力デバイスのモード
  int input_mode;

  // カメラの番号
  std::array<int, camCount> camera_id;

  // カメラの代わりに使う静止画
  std::array<std::string, camCount> camera_image;

  // カメラの代わりに使う動画
  std::array<std::string, camCount> camera_movie;

  // 背景画像をマッピングするときのメッシュの分割数
  int camera_texture_samples;

  // 背景画像を繰り返しでマッピングするとき true
  bool camera_texture_repeat;

  // 背景画像をヘッドトラッキングに追従させるとき true
  bool camera_tracking;

  // カメラの解像度
  std::array<int, 2> camera_size;

  // カメラのフレームレート
  double camera_fps;

  // カメラの４文字コーデック
  std::array<char, 5> camera_fourcc;

  // 魚眼カメラの中心位置
  GLfloat camera_center_x;
  GLfloat camera_center_y;

  // 魚眼カメラの画角
  GLfloat camera_fov_x;
  GLfloat camera_fov_y;

  // Ovrvision Pro の設定
  int ovrvision_property;

  // ゲームコントローラの使用
  bool use_controller;

  // Leap Motion の使用
  bool use_leap_motion;

  // バーテックスシェーダのソースプログラム
  std::string vertex_shader;

  // フラグメントシェーダのソースプログラム
  std::string fragment_shader;

  // 役割
  int role;

  // 通信に使うポート番号
  int port;

  // 相手先の IP アドレス
  std::string address;

  // 相手先の映像を安定化するとき true
  bool remote_stabilize;

  // 相手先の映像を変形するとき true
  bool remote_texture_reshape;

  // 相手先の表示に加える遅延
  std::array<unsigned int, 2> remote_delay;

  // 送信する画像の品質
  int remote_texture_quality;

  // 受信した画像をマッピングするときのメッシュの分割数
  int remote_texture_samples;

  // 相手先のレンズの画角
  GLfloat remote_fov_x;
  GLfloat remote_fov_y;

  // 送信に用いる共有メモリのブロック数
  int local_share_size;

  // 受信に用いる共有メモリのブロック数
  int remote_share_size;

  // シーンファイルの入れ子の深さの上限
  int max_level;

  // シーングラフ
  picojson::value scene;

  // 設定ファイルのファイル名
  std::string config_file;

  // コンストラクタ
  Config();

  // デストラクタ
  virtual ~Config();

  // 設定の解析
  bool read(picojson::value &v);

  // 設定の読み込み
  bool load(const std::string &file);

  // 設定の書き込み
  bool save(const std::string &file) const;
};

// デフォルト値
extern Config defaults;

//
// 値の取得
//
template <typename T>
static bool getValue(T& scalar, const picojson::object& object, const char* name)
{
  const auto&& value{ object.find(name) };
  if (value == object.end() || !value->second.is<double>()) return false;
  scalar = static_cast<T>(value->second.get<double>());
  return true;
}

//
// 値の設定
//
template <typename T>
static void setValue(const T& scalar, picojson::object& object, const char* name)
{
  object.insert(std::make_pair(name, picojson::value(static_cast<double>(scalar))));
}

//
// ベクトルの取得
//
template <typename T, std::size_t U>
static bool getVector(std::array<T, U>& vector, const picojson::object& object, const char* name)
{
  const auto&& value{ object.find(name) };
  if (value == object.end() || !value->second.is<picojson::array>()) return false;

  // 配列を取り出す
  const auto& array{ value->second.get<picojson::array>() };

  // 配列の要素数の上限
  const auto limit{ std::min(static_cast<decltype(array.size())>(U), array.size()) };

  // 配列のすべての要素について
  for (decltype(array.size()) i = 0; i < limit; ++i)
  {
    // 要素が数値なら保存する
    if (array[i].is<double>()) vector[i] = static_cast<T>(array[i].get<double>());
  }

  return true;
}

//
// ベクトルの設定
//
template <typename T, std::size_t U>
static void setVector(const std::array<T, U>& vector, picojson::object& object, const char* name)
{
  // picojson の配列
  picojson::array array;

  // 配列のすべての要素について
  for (std::size_t i = 0; i < U; ++i)
  {
    // 要素を picojson::array に追加する
    array.emplace_back(picojson::value(static_cast<double>(vector[i])));
  }

  // オブジェクトに追加する
  object.insert(std::make_pair(name, array));
}

//
// 文字列の取得
//
static bool getString(std::string& string, const picojson::object& object, const char* name)
{
  const auto&& value{ object.find(name) };
  if (value == object.end() || !value->second.is<std::string>()) return false;
  string = value->second.get<std::string>();
  return true;
}

//
// 文字列の設定
//
static void setString(const std::string& string, picojson::object& object, const char* name)
{
  object.insert(std::make_pair(name, picojson::value(string)));
}

//
// 文字列のベクトルの取得
//
template <std::size_t U>
static bool getSource(std::array<std::string, U>& source, const picojson::object& object, const char* name)
{
  const auto& v_shader{ object.find(name) };
  if (v_shader == object.end() || !v_shader->second.is<picojson::array>()) return false;

  // 配列を取り出す
  const auto& array{ v_shader->second.get<picojson::array>() };

  // 配列の要素数の上限
  const auto limit{ std::min(static_cast<decltype(array.size())>(U), array.size()) };

  // 配列のすべての要素について
  for (decltype(array.size()) i = 0; i < limit; ++i)
  {
    // 要素が文字列なら保存する
    if (array[i].is<std::string>()) source[i] = array[i].get<std::string>();
  }

  return true;
}

//
// 文字列のベクトルの設定
//
template <std::size_t U>
static void setSource(const std::array<std::string, U>& source, picojson::object& object, const char* name)
{
  // picojson の配列
  picojson::array array;

  // 配列のすべての要素について
  for (std::size_t i = 0; i < U; ++i)
  {
    // 要素を picojson::array に追加する
    array.emplace_back(picojson::value(source[i]));
  }

  // オブジェクトに追加する
  object.insert(std::make_pair(name, array));
}

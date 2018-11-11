#pragma once

//
// 各種設定
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// JSON
#include "picojson.h"

// 立体視の設定
enum StereoMode
{
  MONO = 0,                                             // 単眼視
  LINEBYLINE,                                           // インターレース（未実装）
  TOPANDBOTTOM,                                         // 上下２分割
  SIDEBYSIDE,                                           // 左右２分割
  QUADBUFFER,                                           // クワッドバッファステレオ
  OCULUS                                                // Oculus Rift (HMD)
};

// 役割
enum HostRole
{
  STANDALONE = 0,                                       // 単独
  OPERATOR,                                             // 操縦者
  WORKER                                                // 作業者
};

// 設定値
struct config
{
  int camera_left;
  std::string camera_left_image;
  std::string camera_left_movie;
  int camera_right;
  std::string camera_right_image;
  std::string camera_right_movie;
  int camera_texture_samples;
  bool camera_texture_repeat;
  bool camera_tracking;
  double capture_width;
  double capture_height;
  double capture_fps;
  GLfloat fisheye_center_x;
  GLfloat fisheye_center_y;
  GLfloat fisheye_fov_x;
  GLfloat fisheye_fov_y;
  int ovrvision_property;
  int display_mode;
  int display_secondary;
  bool display_fullscreen;
  int display_width;
  int display_height;
  GLfloat display_aspect;
  GLfloat display_center;
  GLfloat display_distance;
  GLfloat display_near;
  GLfloat display_far;
  GLfloat display_zoom;
  std::string vertex_shader;
  std::string fragment_shader;
  int role;
  int port;
  std::string address;
  bool remote_stabilize;
  bool remote_texture_reshape;
  unsigned int remote_delay[2];
  int remote_texture_quality;
  int remote_texture_samples;
  GLfloat remote_fov_x;
  GLfloat remote_fov_y;
  int local_share_size;
  int remote_share_size;
  int max_level;
  picojson::value scene;
  static bool read(const std::string &file, picojson::value &v);
  bool load(const std::string &file);
  bool save(const std::string &file) const;
};

// デフォルト値
extern config defaults;

// ナビゲーションの速度調整
constexpr GLfloat zoomStep(0.01f);                          // 物体のズーム率調整のステップ
constexpr GLfloat shiftStep(0.001f);                        // 背景テクスチャのシフト量調整のステップ
constexpr GLfloat focalStep(50.0f);                         // 背景テクスチャのスケール調整のステップ
constexpr GLfloat speedScale(0.005f);                       // フレームあたりの移動速度係数
constexpr GLfloat angleScale(-0.05f);                       // フレームあたりの回転速度係数
constexpr GLfloat wheelXStep(0.005f);                       // マウスホイールの X 方向の係数
constexpr GLfloat wheelYStep(0.005f);                       // マウスホイールの Y 方向の係数
constexpr GLfloat axesSpeedScale(0.01f);                    // ゲームパッドのスティックの速度の係数
constexpr GLfloat axesAngleScale(0.01f);                    // ゲームパッドのスティックの角速度の係数
constexpr GLfloat btnsScale(0.02f);                         // ゲームパッドのボタンの係数

// 視差の変更ステップ (単位 m)
constexpr GLfloat parallaxStep(0.001f);

// マルチサンプリングのサンプル数 (Oculus Rift)
constexpr int backBufferMultisample(0);

// 光源
constexpr GgSimpleShader::Light lightData =
{
  { 0.2f, 0.2f, 0.2f, 0.4f },                           // 環境光成分
  { 1.0f, 1.0f, 1.0f, 0.0f },                           // 拡散反射光成分
  { 1.0f, 1.0f, 1.0f, 0.0f },                           // 鏡面光成分
  { 0.0f, 0.0f, 1.0f, 1.0f }                            // 位置
};

// 材質
constexpr GgSimpleShader::Material materialData =
{
  { 0.8f, 0.8f, 0.8f, 1.0f },                           // 環境光の反射係数
  { 0.8f, 0.8f, 0.8f, 1.0f },                           // 拡散反射係数
  { 0.2f, 0.2f, 0.2f, 1.0f },                           // 鏡面反射係数
  50.0f                                                 // 輝き係数
};

// テクスチャの境界色
constexpr GLfloat borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

// エンコード方法
constexpr char encoderType[] = ".jpg";

// 受信リトライ回数
constexpr int receiveRetry(30);

// 読み飛ばすパケットの最大数
constexpr int maxDropPackets(1000);

// フレーム送信の最小間隔
constexpr long long minDelay(10);

// カメラの数と識別子
constexpr int camCount(2), camL(0), camR(1);

// リモートカメラの数
constexpr int remoteCamCount(2);

// ヘッダの長さ
constexpr int headLength(camCount + 1);

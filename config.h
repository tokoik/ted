#pragma once

//
// 各種設定
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// JSON
#include "picojson.h"

// 標準ライブラリ
#include <string>

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
  GLfloat display_center;
  GLfloat display_distance;
  GLfloat display_near;
  GLfloat display_far;
  GLfloat display_zoom;
  std::string vertex_shader;
  std::string fragment_shader;
  int port;
  std::string address;
  int role;
  unsigned int tracking_delay[2];
  int remote_texture_quality;
  bool remote_texture_reshape;
  int remote_texture_samples;
  GLfloat remote_fov_x;
  GLfloat remote_fov_y;
  int local_share_size;
  int remote_share_size;
  int max_level;
  picojson::value scene;
  picojson::value target;
  picojson::value remote;
  static bool read(const std::string &file, picojson::value &v);
  bool load(const std::string &file);
  bool save(const std::string &file) const;
};

// デフォルト値
extern config defaults;

// ナビゲーションの速度調整
const double zoomStep(0.01);                            // 物体のズーム率調整のステップ
const GLfloat shiftStep(0.001f);                        // 背景テクスチャのシフト量調整のステップ
const GLfloat focalStep(50.0f);                         // 背景テクスチャのスケール調整のステップ
const GLfloat speedScale(0.005f);                       // フレームあたりの移動速度係数
const GLfloat angleScale(-0.05f);                       // フレームあたりの回転速度係数
const GLfloat wheelXStep(0.005f);                       // マウスホイールの X 方向の係数
const GLfloat wheelYStep(0.005f);                       // マウスホイールの Y 方向の係数
const GLfloat axesSpeedScale(0.01f);                    // ゲームパッドのスティックの速度の係数
const GLfloat axesAngleScale(0.01f);                    // ゲームパッドのスティックの角速度の係数
const GLfloat btnsScale(0.02f);                         // ゲームパッドのボタンの係数

// 視差の変更ステップ (単位 m)
const GLfloat parallaxStep(0.001f);

// マルチサンプリングのサンプル数 (Oculus Rift)
const int backBufferMultisample(0);

// 光源
const GgSimpleLight lightData =
{
  { 0.2f, 0.2f, 0.2f, 0.4f },                           // 環境光成分
  { 1.0f, 1.0f, 1.0f, 0.0f },                           // 拡散反射光成分
  { 1.0f, 1.0f, 1.0f, 0.0f },                           // 鏡面光成分
  { 0.0f, 0.0f, 1.0f, 1.0f }                            // 位置
};

// 材質
const GgSimpleMaterial materialData =
{
  { 0.8f, 0.8f, 0.8f, 1.0f },                           // 環境光の反射係数
  { 0.8f, 0.8f, 0.8f, 1.0f },                           // 拡散反射係数
  { 0.2f, 0.2f, 0.2f, 1.0f },                           // 鏡面反射係数
  50.0f                                                 // 輝き係数
};

// テクスチャの境界色
const GLfloat borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

// エンコード方法
const char coder[] = ".jpg";

// カメラの数と識別子
const int camCount(2), camL(0), camR(1);

// リモートカメラの数
const int remoteCamCount(2);

// デバッグモード
#if defined(_DEBUG)
const bool debug(true);
#else
const bool debug(false);
#endif

//
// 初期設定関連の処理
//

// 各種設定
#include "Config.h"

// Ovrvision Pro
#include "ovrvision_pro.h"

// 標準ライブラリ
#include <fstream>

// 初期設定
Config defaults;

//
// ※1 表示の設定 (DisplayMode)
//
//    MONOCULAR = 0,            // 単眼視
//    INTERLACE,                // インターレース（未実装）
//    TOP_AND_BOTTOM,           // 上下２分割
//    SIDE_BY_SIDE,             // 左右２分割
//    QUADBUFFER,               // クワッドバッファステレオ
//    OCULUS                    // Oculus Rift (HMD)
//

//
// ※2 入力の設定 (InputMode)
//
//    IMAGE = 0,                // 静止画
//    MOVIE,                    // 動画
//    CAMERA,                   // Web カメラ
//    OVRVISON,                 // Ovrvision Pro
//    REMOTE                    // リモートの TED
//

//
// ※3 Ovrvision Pro のプロパティ (OVR::Camprop)
//
//    OV_CAM5MP_FULL = 0,	      // 2560x1920 @15fps x2
//    OV_CAM5MP_FHD,			      // 1920x1080 @30fps x2
//    OV_CAMHD_FULL,			      // 1280x960  @45fps x2
//    OV_CAMVR_FULL, 			      // 960x950   @60fps x2
//    OV_CAMVR_WIDE,			      // 1280x800  @60fps x2
//    OV_CAMVR_VGA,			        // 640x480   @90fps x2
//    OV_CAMVR_QVGA,			      // 320x240   @120fps x2
//    OV_CAM20HD_FULL,		      // 1280x960  @15fps x2 Only USB2.0 connection
//    OV_CAM20VR_VGA,           // 640x480   @30fps x2 Only USB2.0 connection
//

//
// ※4 ホストの役割
//
//    STANDALONE = 0,           // 単独
//    INSTRUCTOR,               // 指示者
//    WORKER                    // 作業者
//

//
// コンストラクタ
//
Config::Config()
  : display_mode{ MONOCULAR }                 // 画面表示のモード (※1)
  , display_quadbuffer{ false }               // クワッドバッファステレオ表示を行うとき true
  , display_fullscreen{ false }               // フルスクリーン表示を行うとき true
  , display_secondary{ 0 }                    // フルスクリーン表示するディスプレイの番号
  , display_size{ 960, 540 }                  // 画面（ミラー表示）の画素数
  , display_aspect{ 0.0f }                    // 画面の縦横比
  , display_center{ 0.5f }                    // 画面の中心の高さ
  , display_distance{ 1.5f }                  // 画面までの距離
  , display_near{ 0.1f }                      // 視点から前方面までの距離
  , display_far{ 5.0f }                       // 視点から後方面までの距離
  , input_mode{ IMAGE }                      // 入力モード (※2)
  , camera_id{ -1, -1 }                       // カメラの番号
  , camera_image{ "left.jpg", "right.jpg" }   // カメラの代わりに使う静止画
  , camera_movie{ "", "" }                    // カメラの代わりに使う動画
  , camera_texture_samples{ 1271 }            // 背景画像をマッピングするときのメッシュの分割数
  , camera_texture_repeat{ false }            // 背景画像を繰り返しでマッピングするとき true
  , camera_tracking{ true }                   // 背景画像をヘッドトラッキングに追従させるとき true
  , camera_size{ 0, 0 }                       // カメラの横の画素数
  , camera_fps{ 0.0 }                         // カメラのフレームレート
  , camera_fourcc{ '\0', '\0', '\0', '\0' }   // カメラの４文字コーデック
  , camera_center_x{ 0.0 }                    // 魚眼カメラの横の中心位置
  , camera_center_y{ 0.0 }                    // 魚眼カメラの横の中心位置
  , camera_fov_x{ 1.0 }                       // 魚眼カメラの横の画角
  , camera_fov_y{ 1.0 }                       // 魚眼カメラの縦の画角
  , ovrvision_property{ OVR::OV_CAMVR_FULL }  // Ovrvision Pro の設定 (※3)
  , use_controller{ false }                   // ゲームコントローラの使用
  , use_leap_motion{ false }                  // Leap Motion の使用
  , vertex_shader{ "fixed.vert" }             // バーテックスシェーダのソースプログラム
  , fragment_shader{ "normal.frag" }          // フラグメントシェーダのソースプログラム
  , role{ STANDALONE }                        // ホストの役割 (※4)
  , port{ 0 }                                 // 通信に使うポート番号
  , address{ "" }                             // 相手先の IP アドレス
  , remote_stabilize{ true }                  // 相手先の映像を安定化するとき true
  , remote_texture_reshape{ false }           // 相手先の映像を変形するとき true
  , remote_delay{ 0, 0 }                      // 相手先の表示に加える遅延
  , remote_texture_quality{ 50 }              // 送信する画像の品質
  , remote_texture_samples{ 1372 }            // 受信した画像をマッピングするときのメッシュの分割数
  , remote_fov_x{ 1.0 }                       // 相手先のレンズの横の画角
  , remote_fov_y{ 1.0 }                       // 相手先のレンズの縦の画角
  , local_share_size{ localShareSize }        // 送信に用いる共有メモリのブロック数
  , remote_share_size{ remoteShareSize }      // 受信に用いる共有メモリのブロック数
  , max_level{ 10 }                           // シーンファイルの入れ子の深さの上限
  , scene{}                                   // シーングラフ
  , config_file{ "" }                         // 設定ファイルのファイル名
{
}

//
// デストラクタ
//
Config::~Config()
{
}

//
// JSON の読み取り
//
bool Config::read(picojson::value& v)
{
  // 設定内容のパース
  const auto& o{ v.get<picojson::object>() };
  if (o.empty()) return false;

  // 立体視の方式
  getValue(display_mode, o, "stereo");

  // クアッドバッファステレオ表示
  getValue(display_quadbuffer, o, "quadbuffer");

  // フルスクリーン表示
  getValue(display_fullscreen, o, "fullscreen");

  // フルスクリーン表示するディスプレイの番号
  getValue(display_secondary, o, "use_secondary");

  // ディスプレイの横の画素数
  getValue(display_size[0], o, "display_width");

  // ディスプレイの縦の画素数
  getValue(display_size[1], o, "display_height");

  // ディスプレイの縦横比
  getValue(display_aspect, o, "display_aspect");

  // ディスプレイの中心の高さ
  getValue(display_center, o, "display_center");

  // 視点からディスプレイまでの距離
  getValue(display_distance, o, "display_distance");

  // 視点から前方面までの距離 (焦点距離)
  getValue(display_near, o, "depth_near");

  // 視点から後方面までの距離
  getValue(display_far, o, "depth_far");

  // ニュー力ソース
  getValue(input_mode, o, "input_mode");

  // 左目のキャプチャデバイスのデバイス番号
  getValue(camera_id[camL], o, "left_camera");

  // 左目のムービーファイル
  getString(camera_movie[camL], o, "left_movie");

  // 左目のキャプチャデバイス不使用時に表示する静止画像
  getString(camera_image[camL], o, "left_image");

  // 右目のキャプチャデバイスのデバイス番号
  getValue(camera_id[camR], o, "right_camera");

  // 右目のムービーファイル
  getString(camera_movie[camR], o, "right_movie");

  // 右目のキャプチャデバイス不使用時に表示する静止画像
  getString(camera_image[camR], o, "right_image");

  // スクリーンのサンプル数
  getValue(camera_texture_samples, o, "screen_samples");

  // 正距円筒図法の場合はテクスチャを繰り返す
  getValue(camera_texture_repeat, o, "texture_repeat");

  // ヘッドトラッキング
  getValue(camera_tracking, o, "tracking");

  // 安定化処理
  getValue(remote_stabilize, o, "stabilize");

  // カメラの横の画素数
  getValue(camera_size[0], o, "capture_width");

  // カメラの縦の画素数
  getValue(camera_size[1], o, "capture_height");

  // カメラのフレームレート
  getValue(camera_fps, o, "capture_fps");

  // カメラのコーデック
  const auto& v_camera_fourcc{ o.find("capture_codec") };
  if (v_camera_fourcc != o.end() && v_camera_fourcc->second.is<std::string>())
  {
    // コーデックの文字列
    const auto& codec{ v_camera_fourcc->second.get<std::string>() };

    // コーデックの文字列の長さと格納先の要素数 - 1 の小さいほう
    const auto limit{ std::min(codec.length(), camera_fourcc.size() - 1) };

    // 格納先の先頭からコーデックの文字を格納する
    std::size_t i{ 0 };
    for (; i < limit; ++i) camera_fourcc[i] = toupper(codec[i]);

    // 格納先の残りの要素を 0 で埋める
    std::fill(camera_fourcc.begin() + i, camera_fourcc.end(), '\0');
  }

  // 魚眼レンズの横の中心位置
  getValue(camera_center_x, o, "fisheye_center_x");

  // 魚眼レンズの縦の中心位置
  getValue(camera_center_y, o, "fisheye_center_y");

  // 魚眼レンズの横の画角
  getValue(camera_fov_x, o, "fisheye_fov_x");

  // 魚眼レンズの縦の画角
  getValue(camera_fov_y, o, "fisheye_fov_y");

  // Ovrvision Pro のモード
  getValue(ovrvision_property, o, "ovrvision_property");

  // ゲームコントローラの使用
  getValue(use_controller, o, "leap_motion");

  // Leap Motion の使用
  getValue(use_leap_motion, o, "leap_motion");

  // バーテックスシェーダのソースファイル名
  getString(vertex_shader, o, "vertex_shader");

  // フラグメントシェーダのソースファイル名
  getString(fragment_shader, o, "fragment_shader");

  // リモート表示に使うポート
  getValue(port, o, "port");

  // リモート表示の送信先の IP アドレス
  getString(address, o, "host");

  // ホストの役割
  getValue(role, o, "role");

  // カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  if (getValue(remote_delay[0], o, "role")) remote_delay[1] = remote_delay[0];

  // 左カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  getValue(remote_delay[0], o, "tracking_delay_left");

  // 右カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  getValue(remote_delay[1], o, "tracking_delay_right");

  // 伝送画像の品質
  getValue(remote_texture_quality, o, "texture_quality");

  // 安定化処理（ドーム画像への変形）を行う
  getValue(remote_texture_reshape, o, "texture_reshape");

  // 安定化処理（ドーム画像への変形）に用いるテクスチャのサンプル数
  getValue(remote_texture_samples, o, "texture_samples");

  // リモートカメラの横の画角
  getValue(remote_fov_x, o, "remote_fov_x");

  // リモートカメラの縦の画角
  getValue(remote_fov_y, o, "remote_fov_y");

  // ローカルの姿勢変換行列の最大数
  getValue(local_share_size, o, "local_share_size");

  // リモートの姿勢変換行列の最大数
  getValue(remote_share_size, o, "remote_share_size");

  // シーングラフの最大の深さ
  getValue(max_level, o, "max_level");

  // ワールド座標に固定するシーングラフ
  const auto& v_scene{ o.find("scene") };
  if (v_scene != o.end())
    scene = v_scene->second;

  return true;
}

//
// 設定ファイルの読み込み
//
bool Config::load(const std::string& file)
{
  // 読み込んだ設定ファイル名を覚えておく
  config_file = file;

  // 設定ファイルを開く
  std::ifstream config{ file };
  if (!config) return false;

  // 設定ファイルを読み込む
  picojson::value v;
  config >> v;
  config.close();

  // 設定を解析する
  return read(v);
}

//
// 設定ファイルの書き込み
//
bool Config::save(const std::string& file) const
{
  // 設定値を保存する
  std::ofstream config{ file };
  if (!config) return false;

  // オブジェクト
  picojson::object o;

  // 立体視の方式
  setValue(display_mode, o, "stereo");

  // クアッドバッファステレオ表示
  setValue(display_quadbuffer, o, "quadbuffer");

  // フルスクリーン表示
  setValue(display_fullscreen, o, "fullscreen");

  // フルスクリーン表示するディスプレイの番号
  setValue(display_secondary, o, "use_secondary");

  // ディスプレイの横の画素数
  setValue(display_size[0], o, "display_width");

  // ディスプレイの縦の画素数
  setValue(display_size[1], o, "display_height");

  // ディスプレイの縦横比
  setValue(display_aspect, o, "display_aspect");

  // ディスプレイの中心の高さ
  setValue(display_center, o, "display_center");

  // 視点からディスプレイまでの距離
  setValue(display_distance, o, "display_distance");

  // 視点から前方面までの距離 (焦点距離)
  setValue(display_near, o, "depth_near");

  // 視点から後方面までの距離
  setValue(display_far, o, "depth_far");

  // 左目のキャプチャデバイスのデバイス番号
  setValue(camera_id[camL], o, "left_camera");

  // 左目のムービーファイル
  setString(camera_movie[camL], o, "left_movie");

  // 左目のキャプチャデバイス不使用時に表示する静止画像
  setString(camera_image[camL], o, "left_image");

  // 右目のキャプチャデバイスのデバイス番号
  setValue(camera_id[camR], o, "right_camera");

  // 右目のムービーファイル
  setString(camera_movie[camL], o, "right_movie");

  // 右目のキャプチャデバイス不使用時に表示する静止画像
  setString(camera_image[camR], o, "right_image");

  // スクリーンのサンプル数
  setValue(camera_texture_samples, o, "screen_samples");

  // 正距円筒図法の場合はテクスチャを繰り返す
  setValue(camera_texture_repeat, o, "texture_repeat");

  // ヘッドトラッキング
  setValue(camera_tracking, o, "tracking");

  // 安定化処理
  setValue(remote_stabilize, o, "stabilize");

  // カメラの横の画素数
  setValue(camera_size[0], o, "capture_width");

  // カメラの縦の画素数
  setValue(camera_size[1], o, "capture_height");

  // カメラのフレームレート
  setValue(camera_fps, o, "capture_fps");

  // カメラのコーデック
  setString(std::string(camera_fourcc.data(), camera_fourcc.size()), o, "capture_codec");

  // 魚眼レンズの横の中心位置
  setValue(camera_center_x, o, "fisheye_center_x");

  // 魚眼レンズの縦の中心位置
  setValue(camera_center_y, o, "fisheye_center_y");

  // 魚眼レンズの横の画角
  setValue(camera_fov_x, o, "fisheye_fov_x");

  // 魚眼レンズの縦の画角
  setValue(camera_fov_y, o, "fisheye_fov_y");

  // Ovrvision Pro のモード
  setValue(ovrvision_property, o, "ovrvision_property");

  // コントローラの使用
  setValue(use_controller, o, "controller");

  // Leap Motion の使用
  setValue(use_leap_motion, o, "leap_motion");

  // バーテックスシェーダのソースファイル名
  setString(vertex_shader, o, "vertex_shader");

  // フラグメントシェーダのソースファイル名
  setString(fragment_shader, o, "fragment_shader");

  // リモート表示に使うポート
  setValue(port, o, "port");

  // リモート表示の送信先の IP アドレス
  setString(address, o, "host");

  // ホストの役割
  setValue(role, o, "role");

  // 左カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  setValue(remote_delay[0], o, "tracking_delay_left");

  // 右カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  setValue(remote_delay[1], o, "tracking_delay_right");

  // 伝送画像の品質
  setValue(remote_texture_quality, o, "texture_quality");

  // 安定化処理（ドーム画像への変形）を行う
  setValue(remote_texture_reshape, o, "texture_reshape");

  // 安定化処理（ドーム画像への変形）に用いるテクスチャのサンプル数
  setValue(remote_texture_samples, o, "texture_samples");

  // リモートカメラの横の画角
  setValue(remote_fov_x, o, "remote_fov_x");

  // リモートカメラの縦の画角
  setValue(remote_fov_y, o, "remote_fov_y");

  // ローカルの姿勢変換行列の最大数
  setValue(local_share_size, o, "local_share_size");

  // リモートの姿勢変換行列の最大数
  setValue(remote_share_size, o, "remote_share_size");

  // シーングラフの最大の深さ
  setValue(max_level, o, "max_level");

  // ワールド座標に固定するシーングラフ
  o.insert(std::make_pair("scene", scene));

  // 設定内容をシリアライズして保存
  picojson::value v(o);
  config << v.serialize(true);
  config.close();

  return true;
}

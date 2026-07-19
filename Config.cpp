///
/// 各種設定クラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date July 197, 2026
///
#include "Config.h"

// 構成ファイルの読み取り補助
#include "parseconfig.h"

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
//    TOP_AND_BOTTOM,           // 上下２分割
//    SIDE_BY_SIDE,             // 左右２分割
//    OVERLAY,                  // 左右２分割を重ねて表示
//    QUADBUFFER,               // クワッドバッファステレオ
//    OPENXR                    // OpenXR (HMD)
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
//    INSTRUCTOR,               // 指導者
//    WORKER                    // 作業者
//

//
// JSON の読み取り
//
bool Config::read(picojson::value& v)
{
  // 設定内容のパース
  const auto& o{ v.get<picojson::object>() };
  if (o.empty()) return false;

  // 立体視の方式
  getValue(o, "stereo", display_mode);

  // クアッドバッファステレオ表示
  getValue(o, "quadbuffer", display_quadbuffer);

  // フルスクリーン表示
  getValue(o, "fullscreen", display_fullscreen);

  // フルスクリーン表示するディスプレイの番号
  getValue(o, "use_secondary", display_secondary);

  // ディスプレイの横の画素数
  getValue(o, "display_width", display_size[0]);

  // ディスプレイの縦の画素数
  getValue(o, "display_height", display_size[1]);

  // ディスプレイの縦横比
  getValue(o, "display_aspect", display_aspect);

  // ディスプレイの中心の高さ
  getValue(o, "display_center", display_center);

  // 視点からディスプレイまでの距離
  getValue(o, "display_distance", display_distance);

  // 視点から前方面までの距離 (焦点距離)
  getValue(o, "depth_near", display_near);

  // 視点から後方面までの距離
  getValue(o, "depth_far", display_far);

  // ニュー力ソース
  getValue(o, "input_mode", input_mode);

  // 左目のキャプチャデバイスのデバイス番号
  getValue(o, "left_camera", camera_id[camL]);

  // 左目のムービーファイル
  getString(o, "left_movie", camera_movie[camL]);

  // 左目のキャプチャデバイス不使用時に表示する静止画像
  getString(o, "left_image", camera_image[camL]);

  // 右目のキャプチャデバイスのデバイス番号
  getValue(o, "right_camera", camera_id[camR]);

  // 右目のムービーファイル
  getString(o, "right_movie", camera_movie[camR]);

  // 右目のキャプチャデバイス不使用時に表示する静止画像
  getString(o, "right_image", camera_image[camR]);

  // スクリーンのサンプル数
  getValue(o, "screen_samples", camera_texture_samples);

  // 正距円筒図法の場合はテクスチャを繰り返す
  getValue(o, "texture_repeat", camera_texture_repeat);

  // ヘッドトラッキング
  getValue(o, "tracking", camera_tracking);

  // 安定化処理
  getValue(o, "stabilize", remote_stabilize);

  // カメラの横の画素数
  getValue(o, "capture_width", camera_size[0]);

  // カメラの縦の画素数
  getValue(o, "capture_height", camera_size[1]);

  // カメラのフレームレート
  getValue(o, "capture_fps", camera_fps);

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

  // 左右カメラの個別コーデックと解像度
  if (!getString(o, "left_capture_codec", camera_codec[camL]))
  {
    if (v_camera_fourcc != o.end() && v_camera_fourcc->second.is<std::string>())
    {
      camera_codec[camL] = v_camera_fourcc->second.get<std::string>();
    }
    else
    {
      camera_codec[camL] = "MJPG";
    }
  }

  if (!getString(o, "right_capture_codec", camera_codec[camR]))
  {
    camera_codec[camR] = camera_codec[camL];
  }

  if (!getString(o, "left_capture_resolution", camera_resolution[camL]))
  {
    if (camera_size[0] > 0 && camera_size[1] > 0)
    {
      char buf[32];
      sprintf_s(buf, "%d x %d", camera_size[0], camera_size[1]);
      camera_resolution[camL] = buf;
    }
    else
    {
      camera_resolution[camL] = "1280 x 720";
    }
  }

  if (!getString(o, "right_capture_resolution", camera_resolution[camR]))
  {
    camera_resolution[camR] = camera_resolution[camL];
  }

  // 魚眼レンズの横の中心位置
  getValue(o, "fisheye_center_x", camera_center_x);

  // 魚眼レンズの縦の中心位置
  getValue(o, "fisheye_center_y", camera_center_y);

  // 魚眼レンズの横の画角
  getValue(o, "fisheye_fov_x", camera_fov_x);

  // 魚眼レンズの縦の画角
  getValue(o, "fisheye_fov_y", camera_fov_y);

  // Ovrvision Pro のモード
  getValue(o, "ovrvision_property", ovrvision_property);

  // ゲームコントローラの使用
  getValue(o, "leap_motion", use_controller);

  // Leap Motion の使用
  getValue(o, "leap_motion", use_leap_motion);

  // バーテックスシェーダのソースファイル名
  getString(o, "vertex_shader", vertex_shader);

  // フラグメントシェーダのソースファイル名
  getString(o, "fragment_shader", fragment_shader);

  // リモート表示に使うポート
  getValue(o, "port", port);

  // リモート表示の送信先の IP アドレス
  getString(o, "host", address);

  // ホストの役割
  getValue(o, "role", role);

  // カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  if (getValue(o, "tracking_delay", remote_delay[0])) remote_delay[1] = remote_delay[0];

  // 左カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  getValue(o, "tracking_delay_left", remote_delay[0]);

  // 右カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  getValue(o, "tracking_delay_right", remote_delay[1]);

  // 伝送画像の品質
  getValue(o, "texture_quality", remote_texture_quality);

  // 安定化処理（ドーム画像への変形）を行う
  getValue(o, "texture_reshape", remote_texture_reshape);

  // 安定化処理（ドーム画像への変形）に用いるテクスチャのサンプル数
  getValue(o, "texture_samples", remote_texture_samples);

  // リモートカメラの横の画角
  getValue(o, "remote_fov_x", remote_fov_x);

  // リモートカメラの縦の画角
  getValue(o, "remote_fov_y", remote_fov_y);

  // ローカルの姿勢変換行列の最大数
  getValue(o, "local_share_size", local_share_size);

  // リモートの姿勢変換行列の最大数
  getValue(o, "remote_share_size", remote_share_size);

  // シーングラフの最大の深さ
  getValue(o, "max_level", max_level);

  // ワールド座標に固定するシーングラフ
  const auto& v_scene{ o.find("scene") };
  if (v_scene != o.end())
    scene = v_scene->second;

  // メニューフォント
  getString(o, "menu_font", menu_font);

  // メニューフォントのサイズ
  getValue(o, "menu_font_size", menu_font_size);

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
  setValue(o, "stereo", display_mode);

  // クアッドバッファステレオ表示
  setValue(o, "quadbuffer", display_quadbuffer);

  // フルスクリーン表示
  setValue(o, "fullscreen", display_fullscreen);

  // フルスクリーン表示するディスプレイの番号
  setValue(o, "use_secondary", display_secondary);

  // ディスプレイの横の画素数
  setValue(o, "display_width", display_size[0]);

  // ディスプレイの縦の画素数
  setValue(o, "display_height", display_size[1]);

  // ディスプレイの縦横比
  setValue(o, "display_aspect", display_aspect);

  // ディスプレイの中心の高さ
  setValue(o, "display_center", display_center);

  // 視点からディスプレイまでの距離
  setValue(o, "display_distance", display_distance);

  // 視点から前方面までの距離 (焦点距離)
  setValue(o, "depth_near", display_near);

  // 視点から後方面までの距離
  setValue(o, "depth_far", display_far);

  // 左目のキャプチャデバイスのデバイス番号
  setValue(o, "left_camera", camera_id[camL]);

  // 左目のムービーファイル
  setString(o, "left_movie", camera_movie[camL]);

  // 左目のキャプチャデバイス不使用時に表示する静止画像
  setString(o, "left_image", camera_image[camL]);

  // 右目のキャプチャデバイスのデバイス番号
  setValue(o, "right_camera", camera_id[camR]);

  // 右目のムービーファイル
  setString(o, "right_movie", camera_movie[camR]);

  // 右目のキャプチャデバイス不使用時に表示する静止画像
  setString(o, "right_image", camera_image[camR]);

  // スクリーンのサンプル数
  setValue(o, "screen_samples", camera_texture_samples);

  // 正距円筒図法の場合はテクスチャを繰り返す
  setValue(o, "texture_repeat", camera_texture_repeat);

  // ヘッドトラッキング
  setValue(o, "tracking", camera_tracking);

  // 安定化処理
  setValue(o, "stabilize", remote_stabilize);

  // 左右個別のコーデックと解像度を保存
  setString(o, "left_capture_codec", camera_codec[camL]);
  setString(o, "left_capture_resolution", camera_resolution[camL]);
  setString(o, "right_capture_codec", camera_codec[camR]);
  setString(o, "right_capture_resolution", camera_resolution[camR]);

  // カメラの横の画素数
  int w = 0, h = 0;
  sscanf_s(camera_resolution[camL].c_str(), "%d x %d", &w, &h);
  setValue(o, "capture_width", w);

  // カメラの縦の画素数
  setValue(o, "capture_height", h);

  // カメラのフレームレート
  setValue(o, "capture_fps", camera_fps);

  // カメラのコーデック (互換性のため左カメラの設定を保存)
  setString(o, "capture_codec", camera_codec[camL]);

  // 魚眼レンズの横の中心位置
  setValue(o, "fisheye_center_x", camera_center_x);

  // 魚眼レンズの縦の中心位置
  setValue(o, "fisheye_center_y", camera_center_y);

  // 魚眼レンズの横の画角
  setValue(o, "fisheye_fov_x", camera_fov_x);

  // 魚眼レンズの縦の画角
  setValue(o, "fisheye_fov_y", camera_fov_y);

  // Ovrvision Pro のモード
  setValue(o, "ovrvision_property", ovrvision_property);

  // コントローラの使用
  setValue(o, "controller", use_controller);

  // Leap Motion の使用
  setValue(o, "leap_motion", use_leap_motion);

  // バーテックスシェーダのソースファイル名
  setString(o, "vertex_shader", vertex_shader);

  // フラグメントシェーダのソースファイル名
  setString(o, "fragment_shader", fragment_shader);

  // リモート表示に使うポート
  setValue(o, "port", port);

  // リモート表示の送信先の IP アドレス
  setString(o, "host", address);

  // ホストの役割
  setValue(o, "role", role);

  // 左カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  setValue(o, "tracking_delay_left", remote_delay[0]);

  // 右カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  setValue(o, "tracking_delay_right", remote_delay[1]);

  // 伝送画像の品質
  setValue(o, "texture_quality", remote_texture_quality);

  // 安定化処理（ドーム画像への変形）を行う
  setValue(o, "texture_reshape", remote_texture_reshape);

  // 安定化処理（ドーム画像への変形）に用いるテクスチャのサンプル数
  setValue(o, "texture_samples", remote_texture_samples);

  // リモートカメラの横の画角
  setValue(o, "remote_fov_x", remote_fov_x);

  // リモートカメラの縦の画角
  setValue(o, "remote_fov_y", remote_fov_y);

  // ローカルの姿勢変換行列の最大数
  setValue(o, "local_share_size", local_share_size);

  // リモートの姿勢変換行列の最大数
  setValue(o, "remote_share_size", remote_share_size);

  // シーングラフの最大の深さ
  setValue(o, "max_level", max_level);

  // ワールド座標に固定するシーングラフ
  o.insert(std::make_pair("scene", scene));

  // メニューフォント
  setString(o, "menu_font", menu_font);

  // メニューフォントのサイズ
  setValue(o, "menu_font_size", menu_font_size);

  // 設定内容をシリアライズして保存
  picojson::value v(o);
  config << v.serialize(true);
  config.close();

  return true;
}

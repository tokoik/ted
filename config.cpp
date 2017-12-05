//
// 初期設定関連の処理
//

// 各種設定
#include "config.h"

// 標準ライブラリ
#include <fstream>

// Ovrvision Pro
#if defined(APIENTRY)
#  undef APIENTRY
#endif
#include "ovrvision_pro.h"

config defaults =
{
  -1,                           // int camera_left; (※2)
  "normal_left.jpg",            // std::string camera_left_image;
  "",                           // std::string camera_left_movie;
  -1,                           // int camera_right; (※2)
  "normal_right.jpg",           // std::string camera_right_image;
  "",                           // std::string camera_right_movie;
  1271,                         // int camera_texture_samples;
  true,                         // bool camera_texture_repeat;
  true,                         // bool camera_tracking;
  true,                         // bool remote_stabilize;
  0.0,                          // double capture_width; (0 ならカメラから取得)
  0.0,                          // double capture_height; (0 ならカメラから取得)
  0.0,                          // double capture_fps; (0 ならカメラから取得)
  0.0,                          // GLfloat fisheye_center_x;
  0.0,                          // GLfloat fisheye_center_y;
  1.0,                          // GLfloat fisheye_fov_x;
  1.0,                          // GLfloat fisheye_fov_y;
  OVR::OV_CAM5MP_FHD,           // int ovrvision_propaty; (※3)
  MONO,                         // StereoMode display_mode; (※1)
  1,                            // int display_secondary;
  0.5f,                         // GLfloat display_center;
  1.5f,                         // GLfloat display_distance;
  0.1f,                         // GLfloat display_near;
  5.0f,                         // GLfloat display_far;
  1.0f,                         // GLfloat display_zoom;
  "fixed.vert",                 // std::string vertex_shader;
  "normal.frag",                // std::string fragment_shader;
  0,                            // int port;
  "",                           // std::string address;
  STANDALONE,                   // int role (※4)
  { 0, 0 },                     // int tracking_delay_left, tracking_delay_right;
  50,                           // int texture_quality;
  false,                        // bool texture_reshape;
  1271,                         // int texture_samples;
  1.0,                          // GLfloat remote_fov_x;
  1.0,                          // GLfloat remote_fov_y;
  50,                           // int local_share_size;
  50,                           // int remote_share_size;
  10                            // int max_level;
};

//
// ※1 立体視の設定 (StereoMode)
//
//    NONE = 0,                 // 単眼視
//    LINEBYLINE,               // インターレース（未実装）
//    TOPANDBOTTOM,             // 上下２分割
//    SIDEBYSIDE,               // 左右２分割
//    QUADBUFFER,               // クワッドバッファステレオ
//    OCULUS                    // Oculus Rift (HMD)
//

//
// ※2 カメラ番号の設定
//
//    left_camera <  0 && right_camera <  0 : 画像ファイルを使う
//    left_camera >= 0 && right_camera <  0 : 左カメラだけを使う (単眼)
//    left_camera <  0 && right_camera >= 0 : Ovrvision Pro を使う
//    left_camera >  0 && right_camera >  0 : 左右のカメラを使う
//
//    注: (left_camera == right_camera) >= 0 にしないでください
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
//    OPERATOR,                 // 操縦者
//    WORKER                    // 作業者
//

//
// JSON ファイルの読み込み
//
bool config::read(const std::string &file, picojson::value &v)
{
  std::ifstream config(file);
  if (!config) return false;
  config >> v;
  config.close();
  return true;
}

//
// 設定ファイルの読み込み
//
bool config::load(const std::string &file)
{
  // 設定内容の読み込み
  picojson::value v;
  if (!read(file, v)) return false;

  // 設定内容のパース
  const auto &o(v.get<picojson::object>());
  if (o.empty()) return false;

  // 左目のキャプチャデバイスのデバイス番号もしくはムービーファイル
  const auto &v_left_camera(o.find("left_camera"));
  if (v_left_camera != o.end())
  {
    if (v_left_camera->second.is<std::string>())
      camera_left_movie = v_left_camera->second.get<std::string>();
    else if (v_left_camera->second.is<double>())
      camera_left = static_cast<int>(v_left_camera->second.get<double>());
  }

  // 左目のキャプチャデバイス不使用時に表示する静止画像
  const auto &v_left_image(o.find("left_image"));
  if (v_left_image != o.end() && v_left_image->second.is<std::string>())
    camera_left_image = v_left_image->second.get<std::string>();

  // 右目のキャプチャデバイスのデバイス番号もしくはムービーファイル
  const auto &v_right_camera(o.find("right_camera"));
  if (v_right_camera != o.end())
  {
    if (v_right_camera->second.is<std::string>())
      camera_right_movie = v_right_camera->second.get<std::string>();
    else if (v_right_camera->second.is<double>())
      camera_right = static_cast<int>(v_right_camera->second.get<double>());
  }

  // 右目のキャプチャデバイス不使用時に表示する静止画像
  const auto &v_right_image(o.find("right_image"));
  if (v_right_image != o.end() && v_right_image->second.is<std::string>())
    camera_right_image = v_right_image->second.get<std::string>();

  // スクリーンのサンプル数
  const auto &v_screen_samples(o.find("screen_samples"));
  if (v_screen_samples != o.end() && v_screen_samples->second.is<double>())
    camera_texture_samples = static_cast<int>(v_screen_samples->second.get<double>());

  // 正距円筒図法の場合はテクスチャを繰り返す
  const auto &v_texture_repeat(o.find("texture_repeat"));
  if (v_texture_repeat != o.end() && v_texture_repeat->second.is<bool>())
    camera_texture_repeat = v_texture_repeat->second.get<bool>();

  // ヘッドトラッキング
  const auto &v_tracking(o.find("tracking"));
  if (v_tracking != o.end() && v_tracking->second.is<bool>())
    camera_tracking = v_tracking->second.get<bool>();

  // 安定化処理
  const auto &v_stabilize(o.find("stabilize"));
  if (v_stabilize != o.end() && v_stabilize->second.is<bool>())
    remote_stabilize = v_stabilize->second.get<bool>();

  // カメラの横の画素数
  const auto &v_capture_width(o.find("capture_width"));
  if (v_capture_width != o.end() && v_capture_width->second.is<double>())
    capture_width = v_capture_width->second.get<double>();

  // カメラの縦の画素数
  const auto &v_capture_height(o.find("capture_height"));
  if (v_capture_height != o.end() && v_capture_height->second.is<double>())
    capture_height = v_capture_height->second.get<double>();

  // カメラのフレームレート
  const auto &v_capture_fps(o.find("capture_fps"));
  if (v_capture_fps != o.end() && v_capture_fps->second.is<double>())
    capture_fps = v_capture_fps->second.get<double>();

  // 魚眼レンズの横の中心位置
  const auto &v_fisheye_center_x(o.find("fisheye_center_x"));
  if (v_fisheye_center_x != o.end() && v_fisheye_center_x->second.is<double>())
    fisheye_center_x = static_cast<GLfloat>(v_fisheye_center_x->second.get<double>());

  // 魚眼レンズの縦の中心位置
  const auto &v_fisheye_center_y(o.find("fisheye_center_y"));
  if (v_fisheye_center_y != o.end() && v_fisheye_center_y->second.is<double>())
    fisheye_center_y = static_cast<GLfloat>(v_fisheye_center_y->second.get<double>());

  // 魚眼レンズの横の画角
  const auto &v_fisheye_fov_x(o.find("fisheye_fov_x"));
  if (v_fisheye_fov_x != o.end() && v_fisheye_fov_x->second.is<double>())
    fisheye_fov_x = static_cast<GLfloat>(v_fisheye_fov_x->second.get<double>());

  // 魚眼レンズの縦の画角
  const auto &v_fisheye_fov_y(o.find("fisheye_fov_y"));
  if (v_fisheye_fov_y != o.end() && v_fisheye_fov_y->second.is<double>())
    fisheye_fov_y = static_cast<GLfloat>(v_fisheye_fov_y->second.get<double>());

  // Ovrvision Pro のモード
  const auto &v_ovrvision_property(o.find("ovrvision_property"));
  if (v_ovrvision_property != o.end() && v_ovrvision_property->second.is<double>())
    ovrvision_property = static_cast<int>(v_ovrvision_property->second.get<double>());

  // 立体視の方式
  const auto &v_stereo(o.find("stereo"));
  if (v_stereo != o.end() && v_stereo->second.is<double>())
    display_mode = static_cast<int>(v_stereo->second.get<double>());

  // セカンダリディスプレイの使用
  const auto &v_use_secondary(o.find("use_secondary"));
  if (v_use_secondary != o.end() && v_use_secondary->second.is<double>())
    display_secondary = static_cast<int>(v_use_secondary->second.get<double>());

  // ディスプレイの中心の高さ
  const auto &v_display_center(o.find("display_center"));
  if (v_display_center != o.end() && v_display_center->second.is<double>())
    display_center = static_cast<GLfloat>(v_display_center->second.get<double>());

  // 視点からディスプレイまでの距離
  const auto &v_display_distance(o.find("display_distance"));
  if (v_display_distance != o.end() && v_display_distance->second.is<double>())
    display_distance = static_cast<GLfloat>(v_display_distance->second.get<double>());

  // 視点から前方面までの距離 (焦点距離)
  const auto &v_depth_near(o.find("depth_near"));
  if (v_depth_near != o.end() && v_depth_near->second.is<double>())
    display_near = static_cast<GLfloat>(v_depth_near->second.get<double>());

  // 視点から後方面までの距離
  const auto &v_depth_far(o.find("depth_far"));
  if (v_depth_far != o.end() && v_depth_far->second.is<double>())
    display_far = static_cast<GLfloat>(v_depth_far->second.get<double>());

  // シーンに対するズーム率
  const auto &v_zoom(o.find("zoom"));
  if (v_zoom != o.end() && v_zoom->second.is<double>())
    display_zoom = static_cast<GLfloat>(v_zoom->second.get<double>());

  // バーテックスシェーダのソースファイル名
  const auto &v_vertex_shader(o.find("vertex_shader"));
  if (v_vertex_shader != o.end() && v_vertex_shader->second.is<std::string>())
    vertex_shader = v_vertex_shader->second.get<std::string>();

  // フラグメントシェーダのソースファイル名
  const auto &v_fragment_shader(o.find("fragment_shader"));
  if (v_fragment_shader != o.end() && v_fragment_shader->second.is<std::string>())
    fragment_shader = v_fragment_shader->second.get<std::string>();

  // リモート表示に使うポート
  const auto &v_port(o.find("port"));
  if (v_port != o.end())
    port = static_cast<int>(v_port->second.get<double>());

  // リモート表示の送信先の IP アドレス
  const auto &v_host(o.find("host"));
  if (v_host != o.end())
    address = v_host->second.get<std::string>();

  // ホストの役割
  const auto &v_role(o.find("role"));
  if (v_role != o.end())
    role = static_cast<int>(v_role->second.get<double>());

  // カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  const auto &v_tracking_delay(o.find("tracking_delay"));
  if (v_tracking_delay != o.end())
    tracking_delay[0] = tracking_delay[1] = static_cast<int>(v_tracking_delay->second.get<double>());

  // 左カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  const auto &v_tracking_delay_left(o.find("tracking_delay_left"));
  if (v_tracking_delay_left != o.end())
    tracking_delay[0] = static_cast<int>(v_tracking_delay_left->second.get<double>());

  // 右カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  const auto &v_tracking_delay_right(o.find("tracking_delay_right"));
  if (v_tracking_delay_right != o.end())
    tracking_delay[1] = static_cast<int>(v_tracking_delay_right->second.get<double>());

  // 伝送画像の品質
  const auto &v_texture_quality(o.find("texture_quality"));
  if (v_texture_quality != o.end())
    remote_texture_quality = static_cast<int>(v_texture_quality->second.get<double>());

  // 安定化処理（ドーム画像への変形）を行う
  const auto &v_texture_reshape(o.find("texture_reshape"));
  if (v_texture_reshape != o.end() && v_texture_reshape->second.is<bool>())
    remote_texture_reshape = v_texture_reshape->second.get<bool>();

  // 安定化処理（ドーム画像への変形）に用いるテクスチャのサンプル数
  const auto &v_texture_samples(o.find("texture_samples"));
  if (v_texture_samples != o.end())
    remote_texture_samples = static_cast<int>(v_texture_samples->second.get<double>());

  // リモートカメラの横の画角
  const auto &v_remote_fov_x(o.find("remote_fov_x"));
  if (v_remote_fov_x != o.end() && v_remote_fov_x->second.is<double>())
    remote_fov_x = static_cast<GLfloat>(v_remote_fov_x->second.get<double>());

  // リモートカメラの縦の画角
  const auto &v_remote_fov_y(o.find("remote_fov_y"));
  if (v_remote_fov_y != o.end() && v_remote_fov_y->second.is<double>())
    remote_fov_y = static_cast<GLfloat>(v_remote_fov_y->second.get<double>());

  // ローカルの姿勢変換行列の最大数
  const auto &v_local_share_size(o.find("local_share_size"));
  if (v_local_share_size != o.end())
    local_share_size = static_cast<int>(v_local_share_size->second.get<double>());

  // リモートの姿勢変換行列の最大数
  const auto &v_remote_share_size(o.find("remote_share_size"));
  if (v_remote_share_size != o.end())
    remote_share_size = static_cast<int>(v_remote_share_size->second.get<double>());

  // シーングラフの最大の深さ
  const auto &v_max_level(o.find("max_level"));
  if (v_max_level != o.end())
    max_level = static_cast<int>(v_max_level->second.get<double>());

  // ワールド座標に固定するシーングラフ
  const auto &v_scene(o.find("scene"));
  if (v_scene != o.end())
    scene = v_scene->second;

  return true;
}

//
// 設定ファイルの書き込み
//
bool config::save(const std::string &file) const
{
  // 設定値を保存する
  std::ofstream config(file);
  if (!config) return false;

  // オブジェクト
  picojson::object o;

  // 左目のキャプチャデバイスのデバイス番号もしくはムービーファイル
  o.insert(std::make_pair("left_camera", camera_left_movie.empty()
    ? picojson::value(static_cast<double>(camera_left))
    : picojson::value(camera_left_movie)));

  // 左目のキャプチャデバイス不使用時に表示する静止画像
  o.insert(std::make_pair("left_image", picojson::value(camera_left_image)));

  // 右目のキャプチャデバイスのデバイス番号もしくはムービーファイル
  o.insert(std::make_pair("right_camera", camera_right_movie.empty()
    ? picojson::value(static_cast<double>(camera_right))
    : picojson::value(camera_left_movie)));

  // 右目のキャプチャデバイス不使用時に表示する静止画像
  o.insert(std::make_pair("right_image", picojson::value(camera_right_image)));

  // スクリーンのサンプル数
  o.insert(std::make_pair("screen_samples", picojson::value(static_cast<double>(camera_texture_samples))));

  // 正距円筒図法の場合はテクスチャを繰り返す
  o.insert(std::make_pair("texture_repeat", picojson::value(camera_texture_repeat)));

  // ヘッドトラッキング
  o.insert(std::make_pair("tracking", picojson::value(camera_tracking)));

  // 安定化処理
  o.insert(std::make_pair("stabilize", picojson::value(remote_stabilize)));

  // カメラの横の画素数
  o.insert(std::make_pair("capture_width", picojson::value(capture_width)));

  // カメラの縦の画素数
  o.insert(std::make_pair("capture_height", picojson::value(capture_height)));

  // カメラのフレームレート
  o.insert(std::make_pair("capture_fps", picojson::value(capture_fps)));

  // 魚眼レンズの横の中心位置
  o.insert(std::make_pair("fisheye_center_x", picojson::value(static_cast<double>(fisheye_center_x))));

  // 魚眼レンズの縦の中心位置
  o.insert(std::make_pair("fisheye_center_y", picojson::value(static_cast<double>(fisheye_center_y))));

  // 魚眼レンズの横の画角
  o.insert(std::make_pair("fisheye_fov_x", picojson::value(static_cast<double>(fisheye_fov_x))));

  // 魚眼レンズの縦の画角
  o.insert(std::make_pair("fisheye_fov_y", picojson::value(static_cast<double>(fisheye_fov_y))));

  // Ovrvision Pro のモード
  o.insert(std::make_pair("ovrvision_property", picojson::value(static_cast<double>(ovrvision_property))));

  // 立体視の方式
  o.insert(std::make_pair("stereo", picojson::value(static_cast<double>(display_mode))));

  // セカンダリディスプレイの使用
  o.insert(std::make_pair("use_secondary", picojson::value(static_cast<double>(display_secondary))));

  // ディスプレイの中心の高さ
  o.insert(std::make_pair("display_center", picojson::value(static_cast<double>(display_center))));

  // 視点からディスプレイまでの距離
  o.insert(std::make_pair("display_distance", picojson::value(static_cast<double>(display_distance))));

  // 視点から前方面までの距離 (焦点距離)
  o.insert(std::make_pair("depth_near", picojson::value(static_cast<double>(display_near))));

  // 視点から後方面までの距離
  o.insert(std::make_pair("depth_far", picojson::value(static_cast<double>(display_far))));

  // シーンに対するズーム率
  o.insert(std::make_pair("zoom", picojson::value(static_cast<double>(display_zoom))));

  // バーテックスシェーダのソースファイル名
  o.insert(std::make_pair("vertex_shader", picojson::value(vertex_shader)));

  // フラグメントシェーダのソースファイル名
  o.insert(std::make_pair("fragment_shader", picojson::value(fragment_shader)));

  // リモート表示に使うポート
  o.insert(std::make_pair("port", picojson::value(static_cast<double>(port))));

  // リモート表示の送信先の IP アドレス
  o.insert(std::make_pair("host", picojson::value(address)));

  // ホストの役割
  o.insert(std::make_pair("role", picojson::value(static_cast<double>(role))));

  // 左カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  o.insert(std::make_pair("tracking_delay_left", picojson::value(static_cast<double>(tracking_delay[0]))));

  // 右カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  o.insert(std::make_pair("tracking_delay_right", picojson::value(static_cast<double>(tracking_delay[1]))));

  // 伝送画像の品質
  o.insert(std::make_pair("texture_quality", picojson::value(static_cast<double>(remote_texture_quality))));

  // 安定化処理（ドーム画像への変形）を行う
  o.insert(std::make_pair("texture_reshape", picojson::value(remote_texture_reshape)));

  // 安定化処理（ドーム画像への変形）に用いるテクスチャのサンプル数
  o.insert(std::make_pair("texture_samples", picojson::value(static_cast<double>(remote_texture_samples))));

  // リモートカメラの横の画角
  o.insert(std::make_pair("remote_fov_x", picojson::value(static_cast<double>(remote_fov_x))));

  // リモートカメラの縦の画角
  o.insert(std::make_pair("remote_fov_y", picojson::value(static_cast<double>(remote_fov_y))));

  // ローカルの姿勢変換行列の最大数
  o.insert(std::make_pair("local_share_size", picojson::value(static_cast<double>(local_share_size))));

  // リモートの姿勢変換行列の最大数
  o.insert(std::make_pair("remote_share_size", picojson::value(static_cast<double>(remote_share_size))));

  // シーングラフの最大の深さ
  o.insert(std::make_pair("max_level", picojson::value(static_cast<double>(max_level))));

  // ワールド座標に固定するシーングラフ
  o.insert(std::make_pair("scene", scene));

  // 設定内容をシリアライズして保存
  picojson::value v(o);
  config << v.serialize(true);
  config.close();

  return true;
}

//
// 初期設定関連の処理
//

// 各種設定
#include "Config.h"

// 標準ライブラリ
#include <fstream>

// Ovrvision Pro
#include "ovrvision_pro.h"

// コンストラクタ
Config::Config()
  : camera_left{ -1 /* ※2 */ }
  , camera_left_image{ "normal_left.jpg" }
  , camera_left_movie{ "" }
  , camera_right{ -1 /* ※2 */ }
  , camera_right_image{ "normal_right.jpg" }
  , camera_right_movie{ "" }
  , camera_texture_samples{ 1271 }
  , camera_texture_repeat{ false }
  , camera_tracking{ true }
  , capture_width{ 0.0 }
  , capture_height{ 0.0 }
  , capture_fps{ 0.0 }
  , capture_codec{ '\0', '\0', '\0', '\0', '\0' }
  , ovrvision_property{ OVR::OV_CAM5MP_FHD /* ※3 */ }
  , display_mode{ MONO /* ※1 */}
  , display_secondary{ 1 }
  , display_fullscreen{ false }
  , display_width{ 960 }
  , display_height{ 540 }
  , display_aspect{ 1.0f }
  , display_center{ 0.5f }
  , display_distance{ 1.5f }
  , display_offset{ 0.0f }
  , display_near{ 0.1f }
  , display_far{ 5.0f }
  , display_zoom{ 1.0f }
  , display_focal{ 1.0f }
  , display_parallax{ 0.32f }
  , vertex_shader{ "fixed.vert" }
  , fragment_shader{ "normal.frag" }
  , role{ STANDALONE /* ※4 */}
  , port{ 0 }
  , address{ "" }
  , remote_delay{ 0, 0 }
  , remote_stabilize{ true }
  , remote_texture_reshape{ false }
  , remote_texture_width{ 640 }
  , remote_texture_height{ 480 }
  , remote_texture_quality{ 50 }
  , remote_texture_samples{ 1372 }
  , remote_fov{ 1.0, 1.0 }
  , local_share_size{ 50 }
  , remote_share_size{ 50 }
  , position{ 0.0f, 0.0f, 0.0f, 1.0f }
  , orientation{ 0.0f, 0.0f, 0.0f, 1.0f }
  , max_level{ 10 }
{
}

// デストラクタ
Config::~Config()
{
}

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
bool Config::load(const std::string& file)
{
  // 設定内容の読み込み
  picojson::value v;
  std::ifstream config(file);
  if (!config) return false;
  config >> v;
  config.close();

  // 設定内容のパース
  const auto& o(v.get<picojson::object>());
  if (o.empty()) return false;

  // 左目のキャプチャデバイスのデバイス番号もしくはムービーファイル
  const auto& v_left_camera(o.find("left_camera"));
  if (v_left_camera != o.end())
  {
    if (v_left_camera->second.is<std::string>())
      camera_left_movie = v_left_camera->second.get<std::string>();
    else if (v_left_camera->second.is<double>())
      camera_left = static_cast<int>(v_left_camera->second.get<double>());
  }

  // 左目のキャプチャデバイス不使用時に表示する静止画像
  const auto& v_left_image(o.find("left_image"));
  if (v_left_image != o.end() && v_left_image->second.is<std::string>())
    camera_left_image = v_left_image->second.get<std::string>();

  // 右目のキャプチャデバイスのデバイス番号もしくはムービーファイル
  const auto& v_right_camera(o.find("right_camera"));
  if (v_right_camera != o.end())
  {
    if (v_right_camera->second.is<std::string>())
      camera_right_movie = v_right_camera->second.get<std::string>();
    else if (v_right_camera->second.is<double>())
      camera_right = static_cast<int>(v_right_camera->second.get<double>());
  }

  // 右目のキャプチャデバイス不使用時に表示する静止画像
  const auto& v_right_image(o.find("right_image"));
  if (v_right_image != o.end() && v_right_image->second.is<std::string>())
    camera_right_image = v_right_image->second.get<std::string>();

  // スクリーンのサンプル数
  const auto& v_screen_samples(o.find("screen_samples"));
  if (v_screen_samples != o.end() && v_screen_samples->second.is<double>())
    camera_texture_samples = static_cast<int>(v_screen_samples->second.get<double>());

  // 正距円筒図法の場合はテクスチャを繰り返す
  const auto& v_texture_repeat(o.find("texture_repeat"));
  if (v_texture_repeat != o.end() && v_texture_repeat->second.is<bool>())
    camera_texture_repeat = v_texture_repeat->second.get<bool>();

  // ヘッドトラッキング
  const auto& v_tracking(o.find("tracking"));
  if (v_tracking != o.end() && v_tracking->second.is<bool>())
    camera_tracking = v_tracking->second.get<bool>();

  // カメラの横の画素数
  const auto& v_capture_width(o.find("capture_width"));
  if (v_capture_width != o.end() && v_capture_width->second.is<double>())
    capture_width = v_capture_width->second.get<double>();

  // カメラの縦の画素数
  const auto& v_capture_height(o.find("capture_height"));
  if (v_capture_height != o.end() && v_capture_height->second.is<double>())
    capture_height = v_capture_height->second.get<double>();

  // カメラのフレームレート
  const auto& v_capture_fps(o.find("capture_fps"));
  if (v_capture_fps != o.end() && v_capture_fps->second.is<double>())
    capture_fps = v_capture_fps->second.get<double>();

  // カメラのコーデック
  const auto& v_capture_codec(o.find("capture_codec"));
  if (v_capture_codec != o.end() && v_capture_codec->second.is<std::string>())
  {
    const std::string& codec(v_capture_codec->second.get<std::string>());
    if (codec.length() == 4)
    {
      capture_codec[0] = toupper(codec[0]);
      capture_codec[1] = toupper(codec[1]);
      capture_codec[2] = toupper(codec[2]);
      capture_codec[3] = toupper(codec[3]);
      capture_codec[4] = '\0';
    }
    else
    {
      std::fill(capture_codec.begin(), capture_codec.end(), '\0');
    }
  }

  // 魚眼レンズの横の画角
  const auto& v_fisheye_fov_x(o.find("fisheye_fov_x"));
  if (v_fisheye_fov_x != o.end() && v_fisheye_fov_x->second.is<double>())
    circle[0] = static_cast<GLfloat>(v_fisheye_fov_x->second.get<double>());

  // 魚眼レンズの縦の画角
  const auto& v_fisheye_fov_y(o.find("fisheye_fov_y"));
  if (v_fisheye_fov_y != o.end() && v_fisheye_fov_y->second.is<double>())
    circle[1] = static_cast<GLfloat>(v_fisheye_fov_y->second.get<double>());

  // 魚眼レンズの横の中心位置
  const auto& v_fisheye_center_x(o.find("fisheye_center_x"));
  if (v_fisheye_center_x != o.end() && v_fisheye_center_x->second.is<double>())
    circle[2] = static_cast<GLfloat>(v_fisheye_center_x->second.get<double>());

  // 魚眼レンズの縦の中心位置
  const auto& v_fisheye_center_y(o.find("fisheye_center_y"));
  if (v_fisheye_center_y != o.end() && v_fisheye_center_y->second.is<double>())
    circle[3] = static_cast<GLfloat>(v_fisheye_center_y->second.get<double>());

  // Ovrvision Pro のモード
  const auto& v_ovrvision_property(o.find("ovrvision_property"));
  if (v_ovrvision_property != o.end() && v_ovrvision_property->second.is<double>())
    ovrvision_property = static_cast<int>(v_ovrvision_property->second.get<double>());

  // 立体視の方式
  const auto& v_stereo(o.find("stereo"));
  if (v_stereo != o.end() && v_stereo->second.is<double>())
    display_mode = static_cast<int>(v_stereo->second.get<double>());

  // セカンダリディスプレイの使用
  const auto& v_use_secondary(o.find("use_secondary"));
  if (v_use_secondary != o.end() && v_use_secondary->second.is<double>())
    display_secondary = static_cast<int>(v_use_secondary->second.get<double>());

  // フルスクリーン表示
  const auto& v_fullscreen(o.find("fullscreen"));
  if (v_fullscreen != o.end() && v_fullscreen->second.is<bool>())
    display_fullscreen = v_fullscreen->second.get<bool>();

  // ディスプレイの横の画素数
  const auto& v_display_width(o.find("display_width"));
  if (v_display_width != o.end() && v_display_width->second.is<double>())
    display_width = static_cast<int>(v_display_width->second.get<double>());

  // ディスプレイの縦の画素数
  const auto& v_display_height(o.find("display_height"));
  if (v_display_height != o.end() && v_display_height->second.is<double>())
    display_height = static_cast<int>(v_display_height->second.get<double>());

  // ディスプレイの縦横比
  const auto& v_display_aspect(o.find("display_aspect"));
  if (v_display_aspect != o.end() && v_display_aspect->second.is<double>())
    display_aspect = static_cast<GLfloat>(v_display_aspect->second.get<double>());

  // ディスプレイの中心の高さ
  const auto& v_display_center(o.find("display_center"));
  if (v_display_center != o.end() && v_display_center->second.is<double>())
    display_center = static_cast<GLfloat>(v_display_center->second.get<double>());

  // 視点からディスプレイまでの距離
  const auto& v_display_distance(o.find("display_distance"));
  if (v_display_distance != o.end() && v_display_distance->second.is<double>())
    display_distance = static_cast<GLfloat>(v_display_distance->second.get<double>());

  // 左右のディスプレイの間隔
  const auto& v_display_offset(o.find("display_offset"));
  if (v_display_offset != o.end() && v_display_offset->second.is<double>())
    display_offset = static_cast<GLfloat>(v_display_offset->second.get<double>());

  // 視点から前方面までの距離 (焦点距離)
  const auto& v_depth_near(o.find("depth_near"));
  if (v_depth_near != o.end() && v_depth_near->second.is<double>())
    display_near = static_cast<GLfloat>(v_depth_near->second.get<double>());

  // 視点から後方面までの距離
  const auto& v_depth_far(o.find("depth_far"));
  if (v_depth_far != o.end() && v_depth_far->second.is<double>())
    display_far = static_cast<GLfloat>(v_depth_far->second.get<double>());

  // シーンに対するズーム率
  const auto& v_zoom(o.find("zoom"));
  if (v_zoom != o.end() && v_zoom->second.is<double>())
    display_zoom = static_cast<GLfloat>(v_zoom->second.get<double>());

  // 背景に対する焦点距離
  const auto& v_focal(o.find("focal"));
  if (v_focal != o.end() && v_focal->second.is<double>())
    display_focal = static_cast<GLfloat>(v_focal->second.get<double>());

  // 視差
  const auto& v_display_parallax(o.find("display_parallax"));
  if (v_display_parallax != o.end() && v_display_parallax->second.is<double>())
    display_parallax = static_cast<GLfloat>(v_display_parallax->second.get<double>());

  // バーテックスシェーダのソースファイル名
  const auto& v_vertex_shader(o.find("vertex_shader"));
  if (v_vertex_shader != o.end() && v_vertex_shader->second.is<std::string>())
    vertex_shader = v_vertex_shader->second.get<std::string>();

  // フラグメントシェーダのソースファイル名
  const auto& v_fragment_shader(o.find("fragment_shader"));
  if (v_fragment_shader != o.end() && v_fragment_shader->second.is<std::string>())
    fragment_shader = v_fragment_shader->second.get<std::string>();

  // ホストの役割
  const auto& v_role(o.find("role"));
  if (v_role != o.end())
    role = static_cast<int>(v_role->second.get<double>());

  // リモート表示に使うポート
  const auto& v_port(o.find("port"));
  if (v_port != o.end())
    port = static_cast<int>(v_port->second.get<double>());

  // リモート表示の送信先の IP アドレス
  const auto& v_host(o.find("host"));
  if (v_host != o.end())
    address = v_host->second.get<std::string>();

  // カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  const auto& v_tracking_delay(o.find("tracking_delay"));
  if (v_tracking_delay != o.end())
    remote_delay[0] = remote_delay[1] = static_cast<int>(v_tracking_delay->second.get<double>());

  // 左カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  const auto& v_tracking_delay_left(o.find("tracking_delay_left"));
  if (v_tracking_delay_left != o.end())
    remote_delay[0] = static_cast<int>(v_tracking_delay_left->second.get<double>());

  // 右カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  const auto& v_tracking_delay_right(o.find("tracking_delay_right"));
  if (v_tracking_delay_right != o.end())
    remote_delay[1] = static_cast<int>(v_tracking_delay_right->second.get<double>());

  // 安定化処理
  const auto& v_stabilize(o.find("stabilize"));
  if (v_stabilize != o.end() && v_stabilize->second.is<bool>())
    remote_stabilize = v_stabilize->second.get<bool>();

  // 安定化処理用のテクスチャの作成
  const auto& v_texture_reshape(o.find("texture_reshape"));
  if (v_texture_reshape != o.end() && v_texture_reshape->second.is<bool>())
    remote_texture_reshape = v_texture_reshape->second.get<bool>();

  // リモートカメラの横の画素数
  const auto& v_texture_width(o.find("texture_width"));
  if (v_texture_width != o.end() && v_texture_width->second.is<double>())
    remote_texture_width = static_cast<int>(v_texture_width->second.get<double>());

  // リモートカメラの縦の画素数
  const auto& v_texture_height(o.find("texture_height"));
  if (v_texture_height != o.end() && v_texture_height->second.is<double>())
    remote_texture_height = static_cast<int>(v_texture_height->second.get<double>());

  // 伝送画像の品質
  const auto& v_texture_quality(o.find("texture_quality"));
  if (v_texture_quality != o.end())
    remote_texture_quality = static_cast<int>(v_texture_quality->second.get<double>());

  // リモートカメラのテクスチャのサンプル数
  const auto& v_texture_samples(o.find("texture_samples"));
  if (v_texture_samples != o.end())
    remote_texture_samples = static_cast<int>(v_texture_samples->second.get<double>());

  // リモートカメラの横の画角
  const auto& v_remote_fov_x(o.find("remote_fov_x"));
  if (v_remote_fov_x != o.end() && v_remote_fov_x->second.is<double>())
    remote_fov[0] = static_cast<GLfloat>(v_remote_fov_x->second.get<double>());

  // リモートカメラの縦の画角
  const auto& v_remote_fov_y(o.find("remote_fov_y"));
  if (v_remote_fov_y != o.end() && v_remote_fov_y->second.is<double>())
    remote_fov[1] = static_cast<GLfloat>(v_remote_fov_y->second.get<double>());

  // ローカルの姿勢変換行列の最大数
  const auto& v_local_share_size(o.find("local_share_size"));
  if (v_local_share_size != o.end())
    local_share_size = static_cast<int>(v_local_share_size->second.get<double>());

  // リモートの姿勢変換行列の最大数
  const auto& v_remote_share_size(o.find("remote_share_size"));
  if (v_remote_share_size != o.end())
    remote_share_size = static_cast<int>(v_remote_share_size->second.get<double>());

  // 初期位置
  const auto& v_position(o.find("position"));
  if (v_position != o.end() && v_position->second.is<picojson::array>())
  {
    const picojson::array& p(v_position->second.get<picojson::array>());
    for (int i = 0; i < 4; ++i) position[i] = static_cast<GLfloat>(p[i].get<double>());
  }

  // 初期姿勢
  const auto& v_orientation(o.find("orientation"));
  if (v_orientation != o.end() && v_orientation->second.is<picojson::array>())
  {
    const picojson::array& a(v_orientation->second.get<picojson::array>());
    for (int i = 0; i < 4; ++i) orientation[i] = static_cast<GLfloat>(a[i].get<double>());
  }

  // シーングラフの最大の深さ
  const auto& v_max_level(o.find("max_level"));
  if (v_max_level != o.end())
    max_level = static_cast<int>(v_max_level->second.get<double>());

  // ワールド座標に固定するシーングラフ
  const auto& v_scene(o.find("scene"));
  if (v_scene != o.end())
    scene = v_scene->second;

  return true;
}

//
// 設定ファイルの書き込み
//
bool Config::save(const std::string &file) const
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

  // カメラの横の画素数
  o.insert(std::make_pair("capture_width", picojson::value(capture_width)));

  // カメラの縦の画素数
  o.insert(std::make_pair("capture_height", picojson::value(capture_height)));

  // カメラのフレームレート
  o.insert(std::make_pair("capture_fps", picojson::value(capture_fps)));

  // カメラのコーデック
  o.insert(std::make_pair("capture_codec", picojson::value(std::string(capture_codec.data(), 4))));

  // 魚眼レンズの横の画角
  o.insert(std::make_pair("fisheye_fov_x", picojson::value(static_cast<double>(circle[0]))));

  // 魚眼レンズの縦の画角
  o.insert(std::make_pair("fisheye_fov_y", picojson::value(static_cast<double>(circle[1]))));

  // 魚眼レンズの横の中心位置
  o.insert(std::make_pair("fisheye_center_x", picojson::value(static_cast<double>(circle[2]))));

  // 魚眼レンズの縦の中心位置
  o.insert(std::make_pair("fisheye_center_y", picojson::value(static_cast<double>(circle[3]))));

  // Ovrvision Pro のモード
  o.insert(std::make_pair("ovrvision_property", picojson::value(static_cast<double>(ovrvision_property))));

  // 立体視の方式
  o.insert(std::make_pair("stereo", picojson::value(static_cast<double>(display_mode))));

  // セカンダリディスプレイの使用
  o.insert(std::make_pair("use_secondary", picojson::value(static_cast<double>(display_secondary))));

  // フルスクリーン表示
  o.insert(std::make_pair("fullscreen", picojson::value(display_fullscreen)));

  // ディスプレイの横の画素数
  o.insert(std::make_pair("display_width", picojson::value(static_cast<double>(display_width))));

  // ディスプレイの縦の画素数
  o.insert(std::make_pair("display_height", picojson::value(static_cast<double>(display_height))));

  // ディスプレイの縦横比
  o.insert(std::make_pair("display_aspect", picojson::value(static_cast<double>(display_aspect))));

  // ディスプレイの中心の高さ
  o.insert(std::make_pair("display_center", picojson::value(static_cast<double>(display_center))));

  // 視点からディスプレイまでの距離
  o.insert(std::make_pair("display_distance", picojson::value(static_cast<double>(display_distance))));

  // 左右のディスプレイの間隔
  o.insert(std::make_pair("display_offset", picojson::value(static_cast<double>(display_offset))));

  // 視点から前方面までの距離 (焦点距離)
  o.insert(std::make_pair("depth_near", picojson::value(static_cast<double>(display_near))));

  // 視点から後方面までの距離
  o.insert(std::make_pair("depth_far", picojson::value(static_cast<double>(display_far))));

  // シーンに対するズーム率
  o.insert(std::make_pair("zoom", picojson::value(static_cast<double>(display_zoom))));

  // 背景にに対する焦点距離
  o.insert(std::make_pair("focal", picojson::value(static_cast<double>(display_focal))));

  // 視差
  o.insert(std::make_pair("display_parallax", picojson::value(static_cast<double>(display_parallax))));

  // バーテックスシェーダのソースファイル名
  o.insert(std::make_pair("vertex_shader", picojson::value(vertex_shader)));

  // フラグメントシェーダのソースファイル名
  o.insert(std::make_pair("fragment_shader", picojson::value(fragment_shader)));

  // ホストの役割
  o.insert(std::make_pair("role", picojson::value(static_cast<double>(role))));

  // リモート表示に使うポート
  o.insert(std::make_pair("port", picojson::value(static_cast<double>(port))));

  // リモート表示の送信先の IP アドレス
  o.insert(std::make_pair("host", picojson::value(address)));

  // 左カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  o.insert(std::make_pair("tracking_delay_left", picojson::value(static_cast<double>(remote_delay[0]))));

  // 右カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  o.insert(std::make_pair("tracking_delay_right", picojson::value(static_cast<double>(remote_delay[1]))));

  // 安定化処理
  o.insert(std::make_pair("stabilize", picojson::value(remote_stabilize)));

  // 安定化処理用のテクスチャの作成
  o.insert(std::make_pair("texture_reshape", picojson::value(remote_texture_reshape)));

  // リモートカメラの横の画素数
  o.insert(std::make_pair("texture_width", picojson::value(static_cast<double>(remote_texture_width))));

  // リモートカメラの縦の画素数
  o.insert(std::make_pair("texture_height", picojson::value(static_cast<double>(remote_texture_height))));

  // 伝送画像の品質
  o.insert(std::make_pair("texture_quality", picojson::value(static_cast<double>(remote_texture_quality))));

  // リモートカメラのテクスチャのサンプル数
  o.insert(std::make_pair("texture_samples", picojson::value(static_cast<double>(remote_texture_samples))));

  // リモートカメラの横の画角
  o.insert(std::make_pair("remote_fov_x", picojson::value(static_cast<double>(remote_fov[0]))));

  // リモートカメラの縦の画角
  o.insert(std::make_pair("remote_fov_y", picojson::value(static_cast<double>(remote_fov[1]))));

  // ローカルの姿勢変換行列の最大数
  o.insert(std::make_pair("local_share_size", picojson::value(static_cast<double>(local_share_size))));

  // リモートの姿勢変換行列の最大数
  o.insert(std::make_pair("remote_share_size", picojson::value(static_cast<double>(remote_share_size))));

  // 位置
  picojson::array p;
  for (int i = 0; i < 4; ++i) p.push_back(picojson::value(position[i]));
  o.insert(std::make_pair("position", picojson::value(p)));

  // 姿勢
  picojson::array a;
  for (int i = 0; i < 4; ++i) a.push_back(picojson::value(orientation.get()[i]));
  o.insert(std::make_pair("orientation", picojson::value(a)));

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

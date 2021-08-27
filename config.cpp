//
// 初期設定関連の処理
//

// 各種設定
#include "config.h"

// Ovrvision Pro
#include "ovrvision_pro.h"

// 標準ライブラリ
#include <fstream>

// 初期設定
config defaults;

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
config::config()
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
  , input_mode { IMAGE }                      // 入力モード (※2)
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
config::~config()
{
}

//
// JSON の読み取り
//
bool config::read(picojson::value &v)
{
  // 設定内容のパース
  const auto &o(v.get<picojson::object>());
  if (o.empty()) return false;

  // 立体視の方式
  const auto &v_stereo(o.find("stereo"));
  if (v_stereo != o.end() && v_stereo->second.is<double>())
    display_mode = static_cast<int>(v_stereo->second.get<double>());

  // クアッドバッファステレオ表示
  const auto &v_quadbuffer(o.find("quadbuffer"));
  if (v_quadbuffer != o.end() && v_quadbuffer->second.is<bool>())
    display_quadbuffer = v_quadbuffer->second.get<bool>();

  // フルスクリーン表示
  const auto &v_fullscreen(o.find("fullscreen"));
  if (v_fullscreen != o.end() && v_fullscreen->second.is<bool>())
    display_fullscreen = v_fullscreen->second.get<bool>();

  // フルスクリーン表示するディスプレイの番号
  const auto &v_use_secondary(o.find("use_secondary"));
  if (v_use_secondary != o.end() && v_use_secondary->second.is<double>())
    display_secondary = static_cast<int>(v_use_secondary->second.get<double>());

  // ディスプレイの横の画素数
  const auto &v_display_width(o.find("display_width"));
  if (v_display_width != o.end() && v_display_width->second.is<double>())
    display_size[0] = static_cast<int>(v_display_width->second.get<double>());

  // ディスプレイの縦の画素数
  const auto &v_display_height(o.find("display_height"));
  if (v_display_height != o.end() && v_display_height->second.is<double>())
    display_size[1] = static_cast<int>(v_display_height->second.get<double>());

  // ディスプレイの縦横比
  const auto &v_display_aspect(o.find("display_aspect"));
  if (v_display_aspect != o.end() && v_display_aspect->second.is<double>())
    display_aspect = static_cast<GLfloat>(v_display_aspect->second.get<double>());

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

  // ニュー力ソース
  const auto &v_input_mode(o.find("input_mode"));
  if (v_input_mode != o.end() && v_input_mode->second.is<double>())
    input_mode = static_cast<int>(v_input_mode->second.get<double>());

  // 左目のキャプチャデバイスのデバイス番号
  const auto &v_left_camera(o.find("left_camera"));
  if (v_left_camera != o.end() && v_left_camera->second.is<double>())
    camera_id[camL] = static_cast<int>(v_left_camera->second.get<double>());

  // 左目のムービーファイル
  const auto& v_left_movie(o.find("left_movie"));
  if (v_left_movie != o.end() && v_left_movie->second.is<std::string>())
    camera_movie[camL] = v_left_movie->second.get<std::string>();

  // 左目のキャプチャデバイス不使用時に表示する静止画像
  const auto &v_left_image(o.find("left_image"));
  if (v_left_image != o.end() && v_left_image->second.is<std::string>())
    camera_image[camL] = v_left_image->second.get<std::string>();

  // 右目のキャプチャデバイスのデバイス番号
  const auto& v_right_camera(o.find("right_camera"));
  if (v_right_camera != o.end() && v_right_camera->second.is<double>())
    camera_id[camR] = static_cast<int>(v_right_camera->second.get<double>());

  // 右目のムービーファイル
  const auto& v_right_movie(o.find("right_movie"));
  if (v_right_movie != o.end() && v_right_movie->second.is<std::string>())
    camera_movie[camR] = v_right_movie->second.get<std::string>();

  // 右目のキャプチャデバイス不使用時に表示する静止画像
  const auto &v_right_image(o.find("right_image"));
  if (v_right_image != o.end() && v_right_image->second.is<std::string>())
    camera_image[camR] = v_right_image->second.get<std::string>();

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
    camera_size[0] = static_cast<int>(v_capture_width->second.get<double>());

  // カメラの縦の画素数
  const auto &v_capture_height(o.find("capture_height"));
  if (v_capture_height != o.end() && v_capture_height->second.is<double>())
    camera_size[1] = static_cast<int>(v_capture_height->second.get<double>());

  // カメラのフレームレート
  const auto &v_capture_fps(o.find("capture_fps"));
  if (v_capture_fps != o.end() && v_capture_fps->second.is<double>())
    camera_fps = v_capture_fps->second.get<double>();

  // カメラのコーデック
  const auto &v_capture_codec(o.find("capture_codec"));
  if (v_capture_codec != o.end() && v_capture_codec->second.is<std::string>())
  {
    const std::string &codec(v_capture_codec->second.get<std::string>());
    if (codec.length() == 4)
    {
      camera_fourcc[0] = toupper(codec[0]);
      camera_fourcc[1] = toupper(codec[1]);
      camera_fourcc[2] = toupper(codec[2]);
      camera_fourcc[3] = toupper(codec[3]);
    }
    else
    {
      camera_fourcc[0] = '\0';
    }
  }

  // 魚眼レンズの横の中心位置
  const auto &v_fisheye_center_x(o.find("fisheye_center_x"));
  if (v_fisheye_center_x != o.end() && v_fisheye_center_x->second.is<double>())
    camera_center_x = static_cast<GLfloat>(v_fisheye_center_x->second.get<double>());

  // 魚眼レンズの縦の中心位置
  const auto &v_fisheye_center_y(o.find("fisheye_center_y"));
  if (v_fisheye_center_y != o.end() && v_fisheye_center_y->second.is<double>())
    camera_center_y = static_cast<GLfloat>(v_fisheye_center_y->second.get<double>());

  // 魚眼レンズの横の画角
  const auto &v_fisheye_fov_x(o.find("fisheye_fov_x"));
  if (v_fisheye_fov_x != o.end() && v_fisheye_fov_x->second.is<double>())
    camera_fov_x = static_cast<GLfloat>(v_fisheye_fov_x->second.get<double>());

  // 魚眼レンズの縦の画角
  const auto &v_fisheye_fov_y(o.find("fisheye_fov_y"));
  if (v_fisheye_fov_y != o.end() && v_fisheye_fov_y->second.is<double>())
    camera_fov_y = static_cast<GLfloat>(v_fisheye_fov_y->second.get<double>());

  // Ovrvision Pro のモード
  const auto &v_ovrvision_property(o.find("ovrvision_property"));
  if (v_ovrvision_property != o.end() && v_ovrvision_property->second.is<double>())
    ovrvision_property = static_cast<int>(v_ovrvision_property->second.get<double>());

  // ゲームコントローラの使用
  const auto &v_use_controller(o.find("leap_motion"));
  if (v_use_controller != o.end() && v_use_controller->second.is<bool>())
    use_controller = v_use_controller->second.get<bool>();

  // Leap Motion の使用
  const auto &v_use_leap_motion(o.find("leap_motion"));
  if (v_use_leap_motion != o.end() && v_use_leap_motion->second.is<bool>())
    use_leap_motion = v_use_leap_motion->second.get<bool>();

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
    remote_delay[0] = remote_delay[1] = static_cast<int>(v_tracking_delay->second.get<double>());

  // 左カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  const auto &v_tracking_delay_left(o.find("tracking_delay_left"));
  if (v_tracking_delay_left != o.end())
    remote_delay[0] = static_cast<int>(v_tracking_delay_left->second.get<double>());

  // 右カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  const auto &v_tracking_delay_right(o.find("tracking_delay_right"));
  if (v_tracking_delay_right != o.end())
    remote_delay[1] = static_cast<int>(v_tracking_delay_right->second.get<double>());

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
// 設定ファイルの読み込み
//
bool config::load(const std::string &file)
{
  // 読み込んだ設定ファイル名を覚えておく
  config_file = file;

  // 設定ファイルを開く
  std::ifstream config(file);
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
bool config::save(const std::string &file) const
{
  // 設定値を保存する
  std::ofstream config(file);
  if (!config) return false;

  // オブジェクト
  picojson::object o;

  // 立体視の方式
  o.insert(std::make_pair("stereo", picojson::value(static_cast<double>(display_mode))));

  // クアッドバッファステレオ表示
  o.insert(std::make_pair("quadbuffer", picojson::value(display_quadbuffer)));

  // フルスクリーン表示
  o.insert(std::make_pair("fullscreen", picojson::value(display_fullscreen)));

  // フルスクリーン表示するディスプレイの番号
  o.insert(std::make_pair("use_secondary", picojson::value(static_cast<double>(display_secondary))));

  // ディスプレイの横の画素数
  o.insert(std::make_pair("display_width", picojson::value(static_cast<double>(display_size[0]))));

  // ディスプレイの縦の画素数
  o.insert(std::make_pair("display_height", picojson::value(static_cast<double>(display_size[1]))));

  // ディスプレイの縦横比
  o.insert(std::make_pair("display_aspect", picojson::value(static_cast<double>(display_aspect))));

  // ディスプレイの中心の高さ
  o.insert(std::make_pair("display_center", picojson::value(static_cast<double>(display_center))));

  // 視点からディスプレイまでの距離
  o.insert(std::make_pair("display_distance", picojson::value(static_cast<double>(display_distance))));

  // 視点から前方面までの距離 (焦点距離)
  o.insert(std::make_pair("depth_near", picojson::value(static_cast<double>(display_near))));

  // 視点から後方面までの距離
  o.insert(std::make_pair("depth_far", picojson::value(static_cast<double>(display_far))));

  // 左目のキャプチャデバイスのデバイス番号
  o.insert(std::make_pair("left_camera", picojson::value(static_cast<double>(camera_id[camL]))));

  // 左目のムービーファイル
  o.insert(std::make_pair("left_movie", picojson::value(camera_movie[camL])));

  // 左目のキャプチャデバイス不使用時に表示する静止画像
  o.insert(std::make_pair("left_image", picojson::value(camera_image[camL])));

  // 右目のキャプチャデバイスのデバイス番号
  o.insert(std::make_pair("right_camera", picojson::value(static_cast<double>(camera_id[camR]))));

  // 右目のムービーファイル
  o.insert(std::make_pair("right_camera", picojson::value(camera_movie[camL])));

  // 右目のキャプチャデバイス不使用時に表示する静止画像
  o.insert(std::make_pair("right_image", picojson::value(camera_image[camR])));

  // スクリーンのサンプル数
  o.insert(std::make_pair("screen_samples", picojson::value(static_cast<double>(camera_texture_samples))));

  // 正距円筒図法の場合はテクスチャを繰り返す
  o.insert(std::make_pair("texture_repeat", picojson::value(camera_texture_repeat)));

  // ヘッドトラッキング
  o.insert(std::make_pair("tracking", picojson::value(camera_tracking)));

  // 安定化処理
  o.insert(std::make_pair("stabilize", picojson::value(remote_stabilize)));

  // カメラの横の画素数
  o.insert(std::make_pair("capture_width", picojson::value(static_cast<double>(camera_size[0]))));

  // カメラの縦の画素数
  o.insert(std::make_pair("capture_height", picojson::value(static_cast<double>(camera_size[1]))));

  // カメラのフレームレート
  o.insert(std::make_pair("capture_fps", picojson::value(camera_fps)));

  // カメラのコーデック
  o.insert(std::make_pair("capture_codec", picojson::value(std::string(camera_fourcc, 4))));

  // 魚眼レンズの横の中心位置
  o.insert(std::make_pair("fisheye_center_x", picojson::value(static_cast<double>(camera_center_x))));

  // 魚眼レンズの縦の中心位置
  o.insert(std::make_pair("fisheye_center_y", picojson::value(static_cast<double>(camera_center_y))));

  // 魚眼レンズの横の画角
  o.insert(std::make_pair("fisheye_fov_x", picojson::value(static_cast<double>(camera_fov_x))));

  // 魚眼レンズの縦の画角
  o.insert(std::make_pair("fisheye_fov_y", picojson::value(static_cast<double>(camera_fov_y))));

  // Ovrvision Pro のモード
  o.insert(std::make_pair("ovrvision_property", picojson::value(static_cast<double>(ovrvision_property))));

  // コントローラの使用
  o.insert(std::make_pair("controller", picojson::value(use_controller)));

  // Leap Motion の使用
  o.insert(std::make_pair("leap_motion", picojson::value(use_leap_motion)));

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
  o.insert(std::make_pair("tracking_delay_left", picojson::value(static_cast<double>(remote_delay[0]))));

  // 右カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  o.insert(std::make_pair("tracking_delay_right", picojson::value(static_cast<double>(remote_delay[1]))));

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

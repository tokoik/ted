//
// 初期設定関連の処理
//

// 各種設定
#include "Config.h"

// Ovrvision Pro
#include "ovrvision_pro.h"

// 標準ライブラリ
#include <fstream>

//
// コンストラクタ
//
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
  , circle{ 1.0f, 1.0f, 0.0f, 0.0f }
  , ovrvision_property{ OVR::OV_CAM5MP_FHD /* ※3 */ }
  , display_mode{ MONO /* ※1 */ }
  , display_secondary{ 1 }
  , display_fullscreen{ false }
  , display_width{ 960 }
  , display_height{ 540 }
  , display_aspect{ 1.0f }
  , display_center{ 0.5f }
  , display_distance{ 1.5f }
  , display_near{ 0.1f }
  , display_far{ 5.0f }
  , display_offset{ 0.0f }
  , display_zoom{ 1.0f }
  , display_focal{ 1.0f }
  , parallax{ 0.32f }
  , parallax_offset{ ggIdentityQuaternion(), ggIdentityQuaternion() }
  , vertex_shader{ "fixed.vert" }
  , fragment_shader{ "normal.frag" }
  , role{ STANDALONE /* ※4 */ }
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

//
// デストラクタ
//
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
// 値の取得
//
template <typename T>
static bool getValue(T& scalar, const picojson::object& object, const char* name)
{
    const auto& value{ object.find(name) };
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
    const auto& value{ object.find(name) };
    if (value == object.end() || !value->second.is<picojson::array>()) return false;

    // 配列を取り出す
    const auto& array{ value->second.get<picojson::array>() };

    // 配列の要素数
    const auto n{ std::min(static_cast<decltype(array.size())>(U), array.size()) };

    // 配列のすべての要素について
    for (decltype(array.size()) i = 0; i < n; ++i)
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
    for (decltype(array.size()) i = 0; i < U; ++i)
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
    const auto& value{ object.find(name) };
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
static bool getText(std::array<std::string, U>& source, const picojson::object& object,
    const char* name)
{
    const auto& v_shader{ object.find(name) };
    if (v_shader == object.end() || !v_shader->second.is<picojson::array>()) return false;

    // 配列を取り出す
    const auto& array{ v_shader->second.get<picojson::array>() };

    // 配列の要素数
    const auto n{ std::min(static_cast<decltype(array.size())>(U), array.size()) };

    // 配列のすべての要素について
    for (decltype(array.size()) i = 0; i < n; ++i)
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
static void setText(const std::array<std::string, U>& source, picojson::object& object,
    const char* name)
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

//
// 設定ファイルの読み込み
//
bool Config::load(const std::string &file)
{
  // 設定ファイルを開く
  std::ifstream config{ file };
  if (!config) return false;

  // 設定ファイルを読み込む
  picojson::value v;
  config >> v;
  config.close();

  // 読み込んだ内容が JSON でなければ戻る
  if (!v.is<picojson::object>()) return false;

  // 設定内容のパース
  const auto& o{ v.get<picojson::object>() };
  if (o.empty()) return false;

  // 左目のキャプチャデバイスのデバイス番号
  getValue(camera_left, o, "left_camera");

  // 左目のムービーファイル
  getString(camera_left_movie, o, "left_movie");

  // 左目のキャプチャデバイス不使用時に表示する静止画像
  getString(camera_left_image, o, "left_image");

  // 右目のキャプチャデバイスのデバイス番号
  getValue(camera_right, o, "right_camera");

  // 右目のムービーファイル
  getString(camera_right_movie, o, "right_movie");

  // 右目のキャプチャデバイス不使用時に表示する静止画像
  getString(camera_right_image, o, "right_image");

  // スクリーンのサンプル数
  getValue(camera_texture_samples, o, "screen_samples");

  // 正距円筒図法の場合はテクスチャを繰り返す
  getValue(camera_texture_repeat, o, "texture_repeat");

  // ヘッドトラッキング
  getValue(camera_tracking, o, "tracking");

  // カメラの横の画素数
  getValue(capture_width, o, "capture_width");

  // カメラの縦の画素数
  getValue(capture_height, o, "capture_height");

  // カメラのフレームレート
  getValue(capture_fps, o, "capture_fps");

  // カメラのコーデック
  const auto& v_capture_codec(o.find("capture_codec"));
  if (v_capture_codec != o.end() && v_capture_codec->second.is<std::string>())
  {
    // コーデックの文字列
    const std::string& codec(v_capture_codec->second.get<std::string>());
    
    // コーデックの文字列の長さと格納先の要素数 - 1 の小さいほう
    const auto n{ std::min(codec.length(), capture_codec.size() - 1) };
    
    // 格納先の先頭からコーデックの文字を格納する
    std::size_t i{ 0 };
    for (; i < n; ++i) capture_codec[i] = toupper(codec[i]);
    
    // 格納先の残りの要素を 0 で埋める
    std::fill(capture_codec.begin() + i, capture_codec.end(), '\0');
  }

  // 魚眼レンズの横の画角
  getValue(circle[0], o, "fisheye_fov_x");

  // 魚眼レンズの縦の画角
  getValue(circle[1], o, "fisheye_fov_y");

  // 魚眼レンズの横の中心位置
  getValue(circle[2], o, "fisheye_center_x");

  // 魚眼レンズの縦の中心位置
  getValue(circle[3], o, "fisheye_center_y");

  // Ovrvision Pro のモード
  getValue(ovrvision_property, o, "ovrvision_property");

  // 立体視の方式
  getValue(display_mode, o, "stereo");

  // フルスクリーン表示
  getValue(display_fullscreen, o, "fullscreen");

  // フルスクリーン表示するディスプレイの番号
  getValue(display_secondary, o, "use_secondary");

  // ディスプレイの横の画素数
  getValue(display_width, o, "display_width");

  // ディスプレイの縦の画素数
  getValue(display_height, o, "display_height");

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

  // 左右のディスプレイの間隔
  getValue(display_offset, o, "display_offset");

  // シーンに対するズーム率
  getValue(display_zoom, o, "zoom");

  // 背景に対する焦点距離
  getValue(display_focal, o, "focal");

  // 視差
  getValue(parallax, o, "parallax");

  // カメラ方向の補正値
  const auto& v_parallax_offset{ o.find("parallax_offset") };
  if (v_parallax_offset != o.end() && v_parallax_offset->second.is<picojson::array>())
  {
    const picojson::array& a{ v_parallax_offset->second.get<picojson::array>() };
    for (int cam = 0; cam < camCount; ++cam)
    {
      GgVector q;
      for (int i = 0; i < 4; ++i) q[i] = static_cast<GLfloat>(a[cam * 4 + i].get<double>());
      parallax_offset[cam] = GgQuaternion(q);
    }
  }

  // バーテックスシェーダのソースファイル名
  getString(vertex_shader, o, "vertex_shader");

  // フラグメントシェーダのソースファイル名
  getString(fragment_shader, o, "fragment_shader");

  // ホストの役割
  getValue(role, o, "role");

  // リモート表示に使うポート
  getValue(port, o, "port");

  // リモート表示の送信先の IP アドレス
  getString(address, o, "host");

  // カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  if (getValue(remote_delay[0], o, "tracking_delay")) remote_delay[1] = remote_delay[0];

  // 左カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  getValue(remote_delay[0], o, "tracking_delay_left");

  // 右カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  getValue(remote_delay[1], o, "tracking_delay_right");

  // 安定化処理
  getValue(remote_stabilize, o, "stabilize");

  // 安定化処理用のテクスチャの作成
  getValue(remote_texture_reshape, o, "texture_reshape");

  // リモートカメラの横の画素数
  getValue(remote_texture_width, o, "texture_width");

  // リモートカメラの縦の画素数
  getValue(remote_texture_height, o, "texture_height");

  // 伝送画像の品質
  getValue(remote_texture_quality, o, "texture_quality");

  // リモートカメラのテクスチャのサンプル数
  getValue(remote_texture_samples, o, "texture_samples");

  // リモートカメラの横の画角
  getValue(remote_fov[0], o, "remote_fov_x");

  // リモートカメラの縦の画角
  getValue(remote_fov[1], o, "remote_fov_y");

  // ローカルの姿勢変換行列の最大数
  getValue(local_share_size, o, "local_share_size");

  // リモートの姿勢変換行列の最大数
  getValue(remote_share_size, o, "remote_share_size");

  // 初期位置
  getVector(position, o, "position");

  // 初期姿勢
  getVector(orientation, o, "orientation");

  // シーングラフの最大の深さ
  getValue(max_level, o, "max_level");

  // ワールド座標に固定するシーングラフ
  const auto& v_scene{ o.find("scene") };
  if (v_scene != o.end()) scene = v_scene->second;

  return true;
}

//
// 設定ファイルの書き込み
//
bool Config::save(const std::string& file) const
{
  // 設定値を保存する
  std::ofstream config(file);
  if (!config) return false;

  // オブジェクト
  picojson::object o;

  // 左目のキャプチャデバイスのデバイス番号
  setValue(camera_left, o, "left_camera");

  // 左目のムービーファイル
  setString(camera_left_movie, o, "left_movie");

  // 左目のキャプチャデバイス不使用時に表示する静止画像
  setString(camera_left_image, o, "left_image");

  // 右目のキャプチャデバイスのデバイス番号
  setValue(camera_right, o, "right_camera");

  // 右目のムービーファイル
  setString(camera_right_movie, o, "right_movie");

  // 右目のキャプチャデバイス不使用時に表示する静止画像
  setString(camera_right_image, o, "right_image");

  // スクリーンのサンプル数
  setValue(camera_texture_samples, o, "screen_samples");

  // 正距円筒図法の場合はテクスチャを繰り返す
  setValue(camera_texture_repeat, o, "texture_repeat");

  // ヘッドトラッキング
  setValue(camera_tracking, o, "tracking");

  // カメラの横の画素数
  setValue(capture_width, o, "capture_width");

  // カメラの縦の画素数
  setValue(capture_height, o, "capture_height");

  // カメラのフレームレート
  setValue(capture_fps, o, "capture_fps");

  // カメラのコーデック
  o.insert(std::make_pair("capture_codec", picojson::value(std::string(capture_codec.data(), 4))));

  // 魚眼レンズの横の画角
  setValue(circle[0], o, "fisheye_fov_x");

  // 魚眼レンズの縦の画角
  setValue(circle[1], o, "fisheye_fov_y");

  // 魚眼レンズの横の中心位置
  setValue(circle[2], o, "fisheye_center_x");

  // 魚眼レンズの縦の中心位置
  setValue(circle[3], o, "fisheye_center_y");

  // Ovrvision Pro のモード
  setValue(ovrvision_property, o, "ovrvision_property");

  // 立体視の方式
  setValue(display_mode, o, "stereo");

  // フルスクリーン表示
  setValue(display_fullscreen, o, "fullscreen");

  // フルスクリーン表示するディスプレイの番号
  setValue(display_secondary, o, "use_secondary");

  // ディスプレイの横の画素数
  setValue(display_width, o, "display_width");

  // ディスプレイの縦の画素数
  setValue(display_height, o, "display_height");

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

  // 左右のディスプレイの間隔
  setValue(display_offset, o, "display_offset");

  // シーンに対するズーム率
  setValue(display_zoom, o, "zoom");

  // 背景にに対する焦点距離
  setValue(display_focal, o, "focal");

  // 視差
  setValue(parallax, o, "parallax");

  // カメラ方向の補正値
  picojson::array q;
  for (int cam = 0; cam < camCount; ++cam)
    for (int i = 0; i < 4; ++i) q.push_back(picojson::value(static_cast<double>(parallax_offset[cam][i])));
  o.insert(std::make_pair("parallax_offset", picojson::value(q)));

  // バーテックスシェーダのソースファイル名
  setString(vertex_shader, o, "vertex_shader");

  // フラグメントシェーダのソースファイル名
  setString(fragment_shader, o, "fragment_shader");

  // ホストの役割
  setValue(role, o, "role");

  // リモート表示に使うポート
  setValue(port, o, "port");

  // リモート表示の送信先の IP アドレス
  setString(address, o, "host");

  // 左カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  setValue(remote_delay[0], o, "tracking_delay_left");

  // 右カメラのフレームに対してトラッキング情報を遅らせるフレームの数
  setValue(remote_delay[1], o, "tracking_delay_right");

  // 安定化処理
  setValue(remote_stabilize, o, "stabilize");

  // 安定化処理用のテクスチャの作成
  setValue(remote_texture_reshape, o, "texture_reshape");

  // リモートカメラの横の画素数
  setValue(remote_texture_width, o, "texture_width");

  // リモートカメラの縦の画素数
  setValue(remote_texture_height, o, "texture_height");

  // 伝送画像の品質
  setValue(remote_texture_quality, o, "texture_quality");

  // リモートカメラのテクスチャのサンプル数
  setValue(remote_texture_samples, o, "texture_samples");

  // リモートカメラの横の画角
  setValue(remote_fov[0], o, "remote_fov_x");

  // リモートカメラの縦の画角
  setValue(remote_fov[1], o, "remote_fov_y");

  // ローカルの姿勢変換行列の最大数
  setValue(local_share_size, o, "local_share_size");

  // リモートの姿勢変換行列の最大数
  setValue(remote_share_size, o, "remote_share_size");

  // 位置
  setVector(position, o, "position");

  // 姿勢
  setVector(orientation, o, "orientation");

  // シーングラフの最大の深さ
  o.insert(std::make_pair("max_level", picojson::value(static_cast<double>(max_level))));

  // ワールド座標に固定するシーングラフ
  o.insert(std::make_pair("scene", scene));

  // 設定内容をシリアライズして保存
  picojson::value v{ o };
  config << v.serialize(true);
  config.close();

  return true;
}

//
// 展開シェーダのパラメータリスト
//
#include "ShaderList.h"

#if 0
//
// 値の取得
//
template <typename T>
static bool getValue(T& scalar, const picojson::object& object, const char* name)
{
  const auto&& value(object.find(name));
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
// ４要素以下のベクトルの取得
//
template <typename T, std::size_t U>
static bool getVector(std::array<T, U>& vector, const picojson::object& object, const char* name)
{
  const auto&& value(object.find(name));
  if (value == object.end() || !value->second.is<picojson::array>()) return false;

  // 配列を取り出す
  const auto& array(value->second.get<picojson::array>());

  // 配列の要素数の上限は４
  const auto n(std::min(static_cast<decltype(array.size())>(U), array.size()));

  // 配列のすべての要素について
  for (decltype(array.size()) i = 0; i < n; ++i)
  {
    // 要素が数値なら保存する
    if (array[i].is<double>()) vector[i] = static_cast<T>(array[i].get<double>());
  }

  return true;
}

//
// ４要素以下のベクトルの設定
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
  const auto&& value(object.find(name));
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
static bool getSource(std::array<std::string, U>& source, const picojson::object& object,
  const char* name)
{
  const auto& v_shader(object.find(name));
  if (v_shader == object.end() || !v_shader->second.is<picojson::array>()) return false;

  // 配列を取り出す
  const auto& array(v_shader->second.get<picojson::array>());

  // 配列の要素数の上限は４
  const auto n(std::min(static_cast<decltype(array.size())>(U), array.size()));

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
static void setSource(const std::array<std::string, U>& source, picojson::object& object,
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
// 調整パラメータの読み込み
//
void ShaderData::load(octatech::OmniImage::Config& config, const picojson::object& object)
{
  // 説明
  getString(description, object, "description");

  // 平面展開用シェーダのソースファイル名
  getSource(expand, object, "expand_shader");

  // 展開図用シェーダのソースファイル名
  getSource(develop, object, "develop_shader");

  // 立体図用シェーダのソースファイル名
  getSource(solid, object, "solid_shader");

  // 展開する画像ファイルのファイルパス
  getString(config.filepath, object, "filepath");

  // 画素数
  getVector(config.size, object, "size");

  // 横の画素数
  getValue(config.size[0], object, "width");

  // 縦の画素数
  getValue(config.size[1], object, "height");

  // フレームレート
  getValue(config.rate, object, "fps");

  // コーデック
  const auto& v_capture_codec(object.find("codec"));
  if (v_capture_codec != object.end() && v_capture_codec->second.is<std::string>())
  {
    const std::string& codec(v_capture_codec->second.get<std::string>());
    if (codec.length() == 4)
    {
      config.fourcc[0] = toupper(codec[0]);
      config.fourcc[1] = toupper(codec[1]);
      config.fourcc[2] = toupper(codec[2]);
      config.fourcc[3] = toupper(codec[3]);
    }
    else
    {
      config.fourcc[0] = '\0';
      config.fourcc[1] = '\0';
      config.fourcc[2] = '\0';
      config.fourcc[3] = '\0';
    }
  }

  // レンズの縦横の画角と中心位置
  getVector(config.circle, object, "circle");

  // レンズの横の画角
  getValue(config.circle[0], object, "fov_x");

  // レンズの縦の画角
  getValue(config.circle[1], object, "fov_y");

  // レンズの横の中心位置
  getValue(config.circle[2], object, "center_x");

  // レンズの縦の中心位置
  getValue(config.circle[3], object, "center_y");

  // 図形に投影するテクスチャの投影中心 (＝カメラの位置, m)
  getVector(config.origin, object, "origin");

  // 図形の拡大率 (＝部屋の大きさ, m)
  getVector(config.scale, object, "scale");

  // テクスチャのチルト (deg), パン (deg), ロール (deg), 焦点距離 (mm)
  getVector(config.param, object, "attitude");

  // テクスチャの境界色
  getVector(config.border, object, "border");
}

//
// 調整パラメータの読み込み
//
bool ShaderData::load(octatech::OmniImage& image, std::ifstream& setting)
{
  // ファイルが開けていなかったらエラー
  if (!setting) return false;

  // JSON を読み込む
  picojson::value value;
  setting >> value;
  setting.close();

  // 設定内容のパース
  const auto& object(value.get<picojson::object>());

  // 設定ファイルが読めていなかったら戻る
  if (object.empty()) return false;

  // 調整パラメータの読み込み
  load(image.config, object);

  return true;
}

//
// 調整パラメータデータのファイルへの書き込み
//
bool ShaderData::save(const octatech::OmniImage& image, std::ofstream& setting) const
{
  // ファイルが開けていなかったらエラー
  if (!setting) return false;

  // 空のオブジェクト
  picojson::object object;

  // 説明
  setString(description, object, "description");

  // 平面展開用シェーダのソースファイル名
  setSource(expand, object, "expand_shader");

  // 展開図用シェーダのソースファイル名
  setSource(develop, object, "develop_shader");

  // 立体図用シェーダのソースファイル名
  setSource(solid, object, "solid_shader");

  // データの格納先
  const octatech::OmniImage::Config& config{ image.config };

  // 展開する画像ファイルのファイルパス
  setString(config.filepath, object, "filepath");

  // 画素数
  setVector(config.size, object, "size");

  // フレームレート
  setValue(config.rate, object, "fps");

  // コーデック
  setString(std::string(config.fourcc.data(), 4), object, "codec");

  // レンズパラメータ
  setVector(config.circle, object, "circle");

  // 図形に投影するテクスチャの投影中心 (＝カメラの位置, m)
  setVector(config.origin, object, "origin");

  // 図形の拡大率 (＝部屋の大きさ, m)
  setVector(config.scale, object, "scale");

  // テクスチャのチルト (deg), パン (deg), ロール (deg), 焦点距離 (mm)
  setVector(config.param, object, "attitude");

  // テクスチャの境界色
  setVector(config.border, object, "border");

  // JSON をシリアライズして書き込み
  setting << picojson::value(object).serialize(true);
  setting.close();

  return true;
}

//
// 設定ファイルの読み込み
//
bool ShaderList::load(const pathString& file)
{
  // リストファイルの読み込み
  std::ifstream config(file);
  if (!config) return false;

  // JSON の読み込み
  picojson::value v;
  config >> v;
  config.close();

  // 設定内容のパース
  const auto& setting(v.get<picojson::object>());
  if (setting.empty()) return false;

  // メッシュの分割数
  getValue(samples, setting, "samples");

  // 境界色
  getVector(border, setting, "border");

  // 背景色
  getVector(background, setting, "background");

  // 前方面
  getValue(zNear, setting, "near");

  // 後方面
  getValue(zFar, setting, "far");

  // デフォルトの光源
  GgSimpleShader::Light light{
      { 0.2f, 0.2f, 0.2f, 0.4f }, // 環境光成分
      { 1.0f, 1.0f, 1.0f, 0.0f }, // 拡散反射光成分
      { 1.0f, 1.0f, 1.0f, 0.0f }, // 鏡面光成分
      { 0.0f, 0.0f, 1.0f, 1.0f }  // 位置
  };

  // 光源データの取り出し
  const auto& v_light(setting.find("light"));
  if (v_light != setting.end() && v_light->second.is<picojson::object>())
  {
    // 光源データの各成分を保存する
    const auto& l(v_light->second.get<picojson::object>());
    getVector(light.ambient, l, "ambient");
    getVector(light.diffuse, l, "diffuse");
    getVector(light.specular, l, "specular");
    getVector(light.position, l, "position");
  }

  // 光源の uniform buffer object を作成する
  lightBuffer.reset(new GgSimpleShader::LightBuffer(light));

  // 合成表示する図形ファイル
  std::string obj("box.obj");
  getString(obj, setting, "shape");
  shape.reset(new GgSimpleObj(obj));

  // 入力デバイスリストを取り出す
  const auto& v_devices(setting.find("device_list"));
  if (v_devices != setting.end() && v_devices->second.is<picojson::array>())
  {
    // 全ての入力デバイスについて
    for (const auto& data : v_devices->second.get<picojson::array>())
    {
      if (data.is<double>())
      {
        // 数値だったらカメラデバイスの番号
        const int id(static_cast<int>(data.get<double>()));
        if (id >= 0) deviceList.emplace_back(id, std::to_string(id));
      }
      else if (data.is<std::string>())
      {
        // 文字列だったらムービーファイル名
        const std::string& name(data.get<std::string>());
        if (!name.empty()) deviceList.emplace_back(-1, name);
      }
    }
  }

  // 設定データを取り出す
  const auto& v_config(setting.find("config_list"));
  if (v_config != setting.end() && v_config->second.is<picojson::array>())
  {
    // 配列を取り出す
    const auto& array(v_config->second.get<picojson::array>());

    // 設定リストを空にする
    configList.clear();

    // 配列のすべての要素について
    for (const auto& element : array)
    {
      // リストを一つ追加
      configList.emplace_back();

      // 最後のコンテナを参照する
      ShaderData& last(configList.back());

      // オブジェクトを取り出す
      const picojson::object& object(element.get<picojson::object>());

      // オブジェクトを最後のコンテナに読み込む
      last.load(last.config, object);
    }
  }

  return true;
}
#endif

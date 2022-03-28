#pragma once

#if 0
// 魚眼・全天球カメラ画像展開ライブラリ
#include "OmniImage.h"

// Window 関連の処理
#include "gg.h"

// JSON
#include "picojson.h"

// 標準ライブラリ
#include <fstream>
#include <map>

//
// 展開用シェーダのパラメータ
//
struct ShaderData
{
  // 説明
  std::string description;

  // 平面展開用シェーダのソースファイル名
  std::array<std::string, 2> expand;

  // 展開図用シェーダのソースファイル名
  std::array<std::string, 2> develop;

  // 立体用シェーダのソースファイル名
  std::array<std::string, 2> solid;

  // カメラの設定
  octatech::OmniImage::Config config;

  // 調整パラメータの読み込み
  void load(octatech::OmniImage::Config& config, const picojson::object& object);

  // 調整パラメータの読み込み
  bool load(octatech::OmniImage& image, std::ifstream& setting);

  // 調整パラメータのファイルからの読み込み
  bool load(octatech::OmniImage& image, const char* file)
  {
    // JSON ファイルの読み込み
    std::ifstream setting(file);

    // JSON の解析
    return load(image, setting);
  }

#if defined(_MSC_VER)
  // 調整パラメータのファイルからの読み込み
  bool load(octatech::OmniImage& image, const wchar_t* file)
  {
    // JSON ファイルの読み込み
    std::ifstream setting{ file };

    // JSON の解析
    return load(image, setting);
  }
#endif

  // 調整パラメータのファイルからの読み込み
  bool load(octatech::OmniImage& image, const std::string& file)
  {
    // JSON ファイルの読み込み
    std::ifstream setting{ file };

    // JSON の解析
    return load(image, setting);
  }

#if defined(_MSC_VER)
  // 調整パラメータのファイルからの読み込み
  bool load(octatech::OmniImage& image, const std::wstring& file)
  {
    // JSON ファイルの読み込み
    std::ifstream setting{ file };

    // JSON の解析
    return load(image, setting);
  }
#endif

  // 調整パラメータの書き込み
  bool save(const octatech::OmniImage& image, std::ofstream& setting) const;

  // 調整パラメータのファイルへの書き込み
  bool save(const octatech::OmniImage& image, const char* file) const
  {
    // 設定値を保存する
    std::ofstream setting{ file };

    return save(image, setting);
  }

#if defined(_MSC_VER)
  // 調整パラメータのファイルへの書き込み
  bool save(const octatech::OmniImage& image, const wchar_t* file) const
  {
    // 設定値を保存する
    std::ofstream setting{ file };

    return save(image, setting);
  }
#endif

  // 調整パラメータのファイルへの書き込み
  bool save(const octatech::OmniImage& image, const std::string& file) const
  {
    // 設定値を保存する
    std::ofstream setting{ file };

    return save(image, setting);
  }

#if defined(_MSC_VER)
  // 調整パラメータのファイルへの書き込み
  bool save(const octatech::OmniImage& image, const std::wstring& file) const
  {
    // 設定値を保存する
    std::ofstream setting{ file };

    return save(image, setting);
  }
#endif
};

//
// 入力デバイス
//
struct DeviceData
{
  int id;                   // デバイス番号
  std::string name;         // デバイス名／入力ファイル名

  // コンストラクタ
  DeviceData(int id, std::string name)
    : id(id)
    , name(name)
  {}
};

//
// 平面展開用シェーダの設定ファイル
//
class ShaderList
{
  // 入力デバイスリスト
  std::vector<DeviceData> deviceList;

  // パラメータリスト
  std::vector<ShaderData> configList;

  // 平面展開用シェーダのソースファイルとプログラム名のリスト
  std::map<std::string, octatech::ExpandShader> expandList;

  // 展開図用シェーダのソースファイルとプログラム名のリスト
  std::map<std::string, octatech::CylinderShader> developList;

  // 立体用シェーダのソースファイルとプログラム名のリスト
  std::map<std::string, octatech::SolidShader> solidList;

  // 展開用シェーダリストにソースファイル名が登録されていれば
  // 登録されているシェーダのポインタを返し無ければ作って返す
  template <typename T>
  const T* getShader(std::map<std::string, T>& list, const std::array<std::string, 2>& file)
  {
    // file[0] バーテックスシェーダ, file[1] フラグメントシェーダ
    return &list.try_emplace(file[0] + ":" + file[1], file[0], file[1]).first->second;
  }

public:

  // 展開に使うメッシュの解像度
  int samples;

  // 境界色
  std::array<GLfloat, 4> border;

  // 背景色
  std::array<GLfloat, 4> background;

  // 前方面と後方面
  GLfloat zNear, zFar;

  // 合成表示する際の光源
  std::unique_ptr<GgSimpleShader::LightBuffer> lightBuffer;

  // 合成表示する図形ファイル
  std::unique_ptr<GgSimpleObj> shape;

  // コンストラクタ
  ShaderList()
    : samples(5202)
#ifdef _DEBUG
    , border{ 1.0f, 0.0f, 0.0f, 0.0f }
#else
    , border{ 0.0f, 0.0f, 0.0f, 0.0f }
#endif
    , background{ 0.0f, 0.0f, 0.0f, 0.0f }
    , zNear(0.1f), zFar(50.0f)
  {}

  // ファイル名を指定するコンストラクタ
  ShaderList(const pathString& file)
    : ShaderList()
  {
    load(file);
  }

  // 設定ファイルの読み込み
  bool load(const pathString& file);

  // デバイスの取得
  const std::vector<DeviceData>& getDevice() const
  {
    return deviceList;
  }

  // パラメータの取得
  std::vector<ShaderData>& getConfig()
  {
    return configList;
  }

  // 平面展開用のシェーダのポインタを得る
  const octatech::ExpandShader* getExpandShader(const std::array<std::string, 2>& file)
  {
    return getShader(expandList, file);
  }

  // 展開図用のシェーダのポインタを得る
  const octatech::DevelopShader* getDevelopShader(const std::array<std::string, 2>& file)
  {
    return getShader(developList, file);
  }

  // 立体図用のシェーダのポインタを得る
  const octatech::SolidShader* getSolidShader(const std::array<std::string, 2>& file)
  {
    return getShader(solidList, file);
  }
};

#endif

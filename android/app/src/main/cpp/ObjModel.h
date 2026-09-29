#pragma once

///
/// Quest 3 版で手のモデルを描画するための Wavefront OBJ 形状
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
/// @details
/// PC 版と同じ handr.obj / handl.obj / finger.obj などを APK の assets から読み込み、
/// 材質 (mtl) の拡散反射色 Kd ごとに描き分ける。
/// PC 版の GgSimpleObj は OpenGL 4.3 に依存するため、OpenGL ES 3.0 向けに最小限の機能で実装する。
///

#include <android/asset_manager.h>
#include <GLES3/gl3.h>

#include <array>
#include <string>
#include <vector>

#include "QuestMath.h"

///
/// 手のモデルを描くシェーダ
///
class ObjShader
{
  GLuint program{ 0 };
  GLint mvpLoc{ -1 }, mvLoc{ -1 }, colorLoc{ -1 }, gammaLoc{ -1 };

public:

  ObjShader() = default;
  ~ObjShader();

  ObjShader(const ObjShader&) = delete;
  ObjShader& operator=(const ObjShader&) = delete;

  /// シェーダを作成する
  bool create();

  /// シェーダを破棄する
  void destroy();

  /// シェーダの使用を開始する
  void use() const;

  ///
  /// 材質の色に掛けるガンマ値を設定する
  ///
  /// @param gamma sRGB のスワップチェーンに描くなら 2.2 (線形化する)、そうでなければ 1.0
  ///
  void setGamma(float gamma) const;

  /// 変換行列を設定する
  void setMatrix(const qm::Mat4& projection, const qm::Mat4& modelview) const;

  /// 材質の色を設定する
  void setColor(const std::array<float, 3>& color) const;
};

///
/// OBJ 形状
///
class ObjModel
{
  /// 材質ごとの描画範囲
  struct Group
  {
    GLint first{ 0 };
    GLsizei count{ 0 };
    std::array<float, 3> color{ 0.8f, 0.8f, 0.8f };
  };

  GLuint vao{ 0 }, vbo{ 0 };
  std::vector<Group> groups;

public:

  ObjModel() = default;
  ~ObjModel();

  ObjModel(const ObjModel&) = delete;
  ObjModel& operator=(const ObjModel&) = delete;

  ///
  /// assets から OBJ ファイルを読み込む
  ///
  /// @param assets アセットマネージャ
  /// @param name OBJ ファイル名
  /// @return 読み込めたら true
  ///
  bool load(AAssetManager* assets, const std::string& name);

  /// 描画資源を破棄する
  void destroy();

  /// 読み込み済みなら true
  bool valid() const { return vao != 0; }

  ///
  /// 描画する
  ///
  /// @param shader 使用中のシェーダ
  /// @param projection 投影変換行列
  /// @param modelview モデルビュー変換行列
  ///
  void draw(const ObjShader& shader, const qm::Mat4& projection, const qm::Mat4& modelview) const;
};

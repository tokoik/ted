#pragma once

// Windows API 関連の設定
#if defined(_WIN32)
#  define NOMINMAX
#  define GLFW_EXPOSE_NATIVE_WIN32
#  define GLFW_EXPOSE_NATIVE_WGL
#  include <GLFW/glfw3native.h>
#  define OVR_OS_WIN32
#endif

// Oculus Rift SDK ライブラリ (LibOVR) の組み込み
#include <OVR_CAPI_GL.h>
#include <Extras/OVR_Math.h>

// ウィンドウ関連の処理
class Window;

//
// Oculus Rift
//
class Oculus
{
  // Oculus Rift のセッション
  ovrSession session;

  // Oculus Rift の情報
  ovrHmdDesc hmdDesc;

  // Oculus Rift 表示用の FBO
  GLuint oculusFbo[ovrEye_Count];

#if OVR_PRODUCT_VERSION >= 1
  // Oculus Rift にレンダリングするフレームの番号
  long long frameIndex;

  // Oculus Rift への描画情報
  ovrLayerEyeFov layerData;

  // Oculus Rift 表示用の FBO のデプステクスチャ
  GLuint oculusDepth[ovrEye_Count];

  // ミラー表示用の FBO のカラーテクスチャ
  ovrMirrorTexture mirrorTexture;

  // ミラー表示用の FBO のカラーテクスチャのサイズ
  GLsizei mirrorWidth, mirrorHeight;

#else
  // Oculus Rift のレンダリング情報
  ovrEyeRenderDesc eyeRenderDesc[ovrEye_Count];

  // Oculus Rift の視点情報
  ovrPosef eyePose[ovrEye_Count];

  // Oculus Rift に転送する描画データ
  ovrLayer_Union layerData;

  // ミラー表示用の FBO のカラーテクスチャ
  ovrGLTexture *mirrorTexture;
#endif

  // ミラー表示用の FBO
  GLuint mirrorFbo;

  // コンストラクタ
  Oculus();

  // デストラクタ
  ~Oculus() = default;

public:

  // コピー・ムーブ禁止
  Oculus(const Oculus&) = delete;
  Oculus& operator=(const Oculus&) = delete;
  Oculus(Oculus&&) = delete;
  Oculus& operator=(Oculus&&) = delete;

  ///
  /// @brief Oculus Rift を初期化する.
  ///
  ///   @param zoom シーンのズーム率.
  ///   @param aspect Oculus Rift のアスペクト比の格納先のポインタ.
  ///   @param mp 透視投影変換行列の配列.
  ///   @param screen 背景に対するスクリーンのサイズの配列.
  ///   @return Oculus Rift の初期化に成功したらコンテキストのポインタ.
  ///
  static Oculus* initialize(GLfloat zoom, GLfloat* aspect, GgMatrix* mp, GgVector* screen);

  ///
  /// @brief Oculus Rift を停止する.
  ///
  void terminate();

  ///
  /// @brief Oculus Rift の透視投影変換行列を求める.
  ///
  ///   @param zoom シーンのズーム率.
  ///   @param mp 透視投影変換行列の配列.
  ///
void getPerspective(GLfloat zoom, GgMatrix* mp) const;

  ///
  /// @brief Oculus Rift のセッションが有効かどうか判定する.
  ///
  ///   @return Oculus Rift のセッションが有効なら true.
  ///
  operator bool()
  {
    return this != nullptr && session != nullptr;
  }

  ///
  /// @brief Oculus Rift に表示する図形の描画を開始する.
  ///
  ///   @param mv それぞれの目の位置に平行移動する GgMatrix 型の変換行列.
  ///   @return 描画が可能なら VISIBLE, 不可能なら INVISIBLE, 終了要求があれば WANTQUIT.
  ///
  enum OculusStatus { VISIBLE = 0, INVISIBLE, WANTQUIT } start(GgMatrix* mv);

  ///
  /// @brief 図形を描画する Oculus Rift の目を選択する.
  ///
  ///   @param eye 図形を描画する Oculus Rift の目.
  ///   @param po GgVecotr 型の変数で eye に指定した目の位置の同次座標が格納される.
  ///   @param qo GgQuaternion 型の変数で eye に指定した目の方向が四元数で格納される.
  ///
  void select(int eye, GgVector& mo, GgQuaternion& qo);

  ///
  /// @brief Oculus Rift に表示する図形の描画を完了する.
  ///
  ///   @param eye 図形の描画を完了する Oculus Rift の目.
  ///
  void commit(int eye);

  ///
  /// @brief 描画したフレームを Oculus Rift に転送する.
  ///
  ///   @return Oculus Rift への転送が成功したら true.
  ///
  bool submit();

  ///
  /// @brief Oculus Rift に描画したフレームをミラー表示する.
  ///
  ///   @param width ミラー表示を行うフレームバッファ上の領域の幅.
  ///   @param height ミラー表示を行うフレームバッファ上の領域の高さ.
  ///
  void submitMirror(GLsizei width, GLsizei height);
};

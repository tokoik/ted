#pragma once

//
// ウィンドウ関連の処理
//

// 各種設定
#include "Config.h"

// カメラ関連の処理
#include "Camera.h"

// Windows API 関連の設定
#if defined(_WIN32)
#  define NOMINMAX
#  define GLFW_EXPOSE_NATIVE_WIN32
#  define GLFW_EXPOSE_NATIVE_WGL
#  include <GLFW/glfw3native.h>
#  define OVR_OS_WIN32
#  define NOTIFY(msg) MessageBox(NULL, TEXT(msg), TEXT("TED"), MB_ICONERROR | MB_OK)
#else
#  define NOTIFY(msg) std::cerr << msg << '\n'
#endif

// Oculus Rift SDK ライブラリ (LibOVR) の組み込み
#include <OVR_CAPI_GL.h>
#include <Extras/OVR_Math.h>

// Dear ImGui を使う
#include "imgui.h"

//
// ウィンドウ関連の処理を担当するクラス
//
class Window
{
  // 設定
  const Config& defaults;

  // 設定のコピー
  Config config;

  // ウィンドウの識別子
  GLFWwindow* const window;

  // このウィンドウで制御するカメラ
  Camera* camera;

  // ビューポートの幅と高さ
  int width, height;

  // ビューポートのアスペクト比
  GLfloat aspect;

  // メッシュの縦横の格子点数
  std::array<GLsizei, 2> samples;

  // メッシュの縦横の格子間隔
  std::array<GLfloat, 2> gap;

  // 最後にタイプしたキー
  int key;

  // ジョイスティックの番号
  int joy;

  // スティックの中立位置
  std::array<float, 4> origin;

  // ドラッグ開始位置
  double cx, cy;

  //
  // モデル変換
  //

  // トラックボール処理
  GgTrackball trackball;

  // モデル変換行列
  GgMatrix mm;

  // ヘッドトラッキングの変換行列
  std::array<GgMatrix, camCount> mo;

  //
  // ビュー変換
  //

  // カメラ方向の補正ステップ
  static std::array<GgQuaternion, camCount> qrStep;

  // ビュー変換
  std::array<GgMatrix, camCount> mv;

  //
  // 投影変換
  //

  // ズーム率の変化量
  int zoomChange;

  // 投影変換
  std::array<GgMatrix, camCount> mp;

  //
  // 背景画像
  //

  // スクリーンの幅と高さ
  std::array<GgVector, camCount> screen;

  // 焦点距離の変化量
  int focalChange;

  // 背景テクスチャの半径と中心の変化量
  std::array<int, 4> circleChange;

  //
  // Oculus Rift
  //

  // Oculus Rift のセッション
  const ovrSession session;

  // Oculus Rift の情報
  ovrHmdDesc hmdDesc;

  // Oculus Rift 表示用の FBO
  GLuint oculusFbo[ovrEye_Count];

#if OVR_PRODUCT_VERSION > 0
  // Oculus Rift にレンダリングするフレームの番号
  long long frameIndex;

  // Oculus Rift への描画情報
  ovrLayerEyeFov layerData;

  // Oculus Rift 表示用の FBO のデプステクスチャ
  std::array<GLuint, ovrEye_Count> oculusDepth;

  // ミラー表示用の FBO のカラーテクスチャ
  ovrMirrorTexture mirrorTexture;
#else
  // Oculus Rift のレンダリング情報
  std::array<ovrEyeRenderDesc, ovrEye_Count> eyeRenderDesc;

  // Oculus Rift の視点情報
  std::array<ovrPosef, ovrEye_Count> eyePose;

  // Oculus Rift に転送する描画データ
  ovrLayer_Union layerData;

  // ミラー表示用の FBO のカラーテクスチャ
  ovrGLTexture* mirrorTexture;
#endif

  // ミラー表示用の FBO
  GLuint mirrorFbo;

  //
  // 透視投影変換行列を求める
  //
  //   ・ウィンドウのサイズ変更時やカメラパラメータの変更時に呼び出す
  //
  void updateProjectionMatrix();

public:

  //
  // コンストラクタ
  //
  Window(Config& config, GLFWwindow* share = nullptr);

  //
  // コピーコンストラクタを封じる
  //
  Window(const Window& w) = delete;

  //
  // 代入を封じる
  //
  Window& operator=(const Window& w) = delete;

  //
  // デストラクタ
  //
  virtual ~Window();

  //
  // ウィンドウの識別子の取得
  //
  GLFWwindow* get() const
  {
    return window;
  }

  //
  // ウィンドウを閉じるよう指示する
  //
  void setClose(int close = GLFW_TRUE) const
  {
    glfwSetWindowShouldClose(window, close);
  }

  //
  // ウィンドウを閉じるべきかを判定する
  //
  int shouldClose() const
  {
    // ウィンドウを閉じるべきなら真を返す
    return glfwWindowShouldClose(window);
  }

  //
  // イベントを取得してループを継続するなら真を返す
  //
  operator bool();

  //
  // 描画開始
  //
  //   ・図形の描画開始前に呼び出す
  //
  bool start();

  //
  // カラーバッファを入れ替えてイベントを取り出す
  //
  //   ・図形の描画終了後に呼び出す
  //   ・ダブルバッファリングのバッファの入れ替えを行う
  //   ・キーボード操作等のイベントを取り出す
  //
  void swapBuffers();

  //
  // ウィンドウのサイズ変更時の処理
  //
  //   ・ウィンドウのサイズ変更時にコールバック関数として呼び出される
  //   ・ウィンドウの作成時には明示的に呼び出す
  //
  static void resize(GLFWwindow* window, int width, int height);

  //
  // マウスボタンを操作したときの処理
  //
  //   ・マウスボタンを押したときにコールバック関数として呼び出される
  //
  static void mouse(GLFWwindow* window, int button, int action, int mods);

  //
  // マウスホイール操作時の処理
  //
  //   ・マウスホイールを操作した時にコールバック関数として呼び出される
  //
  static void wheel(GLFWwindow* window, double x, double y);

  //
  // キーボードをタイプした時の処理
  //
  //   ．キーボードをタイプした時にコールバック関数として呼び出される
  //
  static void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);

  //
  // 設定値の保存
  // 
  bool save(const std::string& file)
  {
    config.orientation = trackball.getQuaternion();
    return config.save(file);
  }

  //
  // 設定値の初期化
  //
  void reset();

  //
  // このウィンドウで制御するカメラを設定する
  //
  void setControlCamera(Camera* cam)
  {
    camera = cam;
  }

  //
  // 図形を見せる目を選択する
  //
  void select(int eye);

  //
  // 図形の描画を完了する
  //
  void commit(int eye);

  //
  // モデル変換行列を得る
  //
  const GgMatrix& getMm() const
  {
    return mm;
  }

  //
  // ビュー変換行列を得る
  //
  const GgMatrix& getMv(int eye) const
  {
    return mv[eye];
  }

  //
  // Oculus Rift のヘッドラッキングによる回転の補正値の四元数を得る
  //
  const GgQuaternion& getQa(int eye) const
  {
    return config.parallax_offset[eye];
  }

  //
  // Oculus Rift のヘッドラッキングによる回転の変換行列を得る
  //
  const GgMatrix& getMo(int eye) const
  {
    return mo[eye];
  }

  //
  // プロジェクション変換行列を得る
  //
  const GgMatrix& getMp(int eye) const
  {
    return mp[eye];
  }

  //
  // メッシュの縦横の格子点数を取り出す
  //
  const GLsizei* getSamples() const
  {
    return samples.data();
  }

  //
  // メッシュの縦横の格子間隔を取り出す
  //
  const GLfloat* getGap() const
  {
    return gap.data();
  }

  //
  // スクリーンの幅と高さを取り出す
  //
  const GgVector& getScreen(int eye) const
  {
    return screen[eye];
  }

  //
  // 焦点距離を取り出す
  //
  GLfloat getFocal() const
  {
    return config.display_focal;
  }

  //
  // スクリーンの間隔を取り出す
  //
  GLfloat getOffset(int eye = 0) const
  {
    return static_cast<GLfloat>(1 - (eye & 1) * 2) * config.display_offset;
  }

  //
  // 背景テクスチャの半径と中心を取り出す
  //
  const GgVector& getCircle() const
  {
    return config.circle;
  }

  //
  // ウィンドウのアスペクト比を取り出す
  //
  GLfloat getAspect() const
  {
    return aspect;
  }

  // シーン表示
  bool showScene;

  // ミラー表示
  bool showMirror;

  // メニュー表示
  bool showMenu;
};

#pragma once

//
// ウィンドウ関連の処理
//

// 各種設定
#include "config.h"

// カメラ関連の処理
#include "Camera.h"

// Windows API 関連の設定
#if defined(_WIN32)
#  define NOMINMAX
#  define GLFW_EXPOSE_NATIVE_WIN32
#  define GLFW_EXPOSE_NATIVE_WGL
#  include <GLFW/glfw3native.h>
#  undef APIENTRY
#  define OVR_OS_WIN32
#  define NOTIFY(msg) MessageBox(NULL, TEXT(msg), TEXT("TED"), MB_ICONERROR | MB_OK)
#else
#  define NOTIFY(msg) std::cerr << msg << '\n'
#endif

// Oculus Rift SDK ライブラリ (LibOVR) の組み込み
#include <OVR_CAPI_GL.h>
#include <Extras/OVR_Math.h>

//
// ウィンドウ関連の処理を担当するクラス
//
class Window
{
  // ウィンドウの識別子
  GLFWwindow *const window;

  // このウィンドウで制御するカメラ
  Camera *camera;

  // ビューポートの幅と高さ
  int width, height;

  // ビューポートのアスペクト比
  GLfloat aspect;

  // メッシュの縦横の格子点数
  GLsizei samples[2];

  // メッシュの縦横の格子間隔
  GLfloat gap[2];

  // 最後にタイプしたキー
  int key;

  // ジョイスティックの番号
  int joy;

  // スティックの中立位置
  float origin[4];

  // ドラッグ開始位置
  double cx, cy;

  //
  // モデル変換
  //

  // 物体の位置
  GLfloat ox, oy, oz;

  // 物体の初期位置
  static GLfloat startPosition[3];

  // トラックボール処理
  GgTrackball trackball;

  // 物体の初期姿勢
  static GLfloat startOrientation[4];

  // モデル変換行列
  GgMatrix mm;

  //
  // ビュー変換
  //

  // ヘッドトラッキングによる回転
  GgQuaternion qo[camCount];

  // ヘッドトラッキングによる位置
  GLfloat po[camCount][4];

  // ビュー変換
  GgMatrix mv[camCount];

  //
  // 投影変換
  //

  // 物体に対するズーム率
  GLfloat zoom;

  // ズーム率の変化量
  int zoomChange;

  // 視差
  GLfloat parallax;

  // 視差の初期値
  static GLfloat initialParallax;

  // 投影変換
  GgMatrix mp[camCount];

  //
  // 背景画像
  //

  // スクリーンの幅と高さ
  GLfloat screen[camCount][4];

  // 焦点距離
  GLfloat focal;

  // 焦点距離の変化量
  int focalChange;

  // 背景テクスチャの半径と中心
  GLfloat circle[4];

  // 背景テクスチャの半径と中心の変化量
  int circleChange[4];

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
  GLuint oculusDepth[ovrEye_Count];

  // ミラー表示用の FBO のカラーテクスチャ
  ovrMirrorTexture mirrorTexture;
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

  //
  // 透視投影変換行列を求める
  //
  //   ・ウィンドウのサイズ変更時やカメラパラメータの変更時に呼び出す
  //
  void updateProjectionMatrix();

  //
  // コピーコンストラクタ (コピー禁止)
  //
  Window(const Window &w);

  //
  // 代入 (代入禁止)
  //
  Window &operator=(const Window &w);

public:

  //
  // コンストラクタ
  //
  Window(int width = 640, int height = 480, const char *title = "GLFW Window"
    , GLFWmonitor *monitor = nullptr, GLFWwindow *share = nullptr
  );

  //
  // デストラクタ
  //
  virtual ~Window();

  //
  // ウィンドウの識別子の取得
  //
  const GLFWwindow *get() const
  {
    return window;
  }

  //
  // ウィンドウを閉じるべきかを判定する
  //
  //   ・描画ループの継続条件として使う
  //
  bool shouldClose() const
  {
    // ウィンドウを閉じるか ESC キーがタイプされていれば真
    return glfwWindowShouldClose(window) || glfwGetKey(window, GLFW_KEY_ESCAPE);
  }

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
  static void resize(GLFWwindow *window, int width, int height);

  //
  // マウスボタンを操作したときの処理
  //
  //   ・マウスボタンを押したときにコールバック関数として呼び出される
  //
  static void mouse(GLFWwindow *window, int button, int action, int mods);

  //
  // マウスホイール操作時の処理
  //
  //   ・マウスホイールを操作した時にコールバック関数として呼び出される
  //
  static void wheel(GLFWwindow *window, double x, double y);

  //
  // キーボードをタイプした時の処理
  //
  //   ．キーボードをタイプした時にコールバック関数として呼び出される
  //
  static void keyboard(GLFWwindow *window, int key, int scancode, int action, int mods);

  //
  // 設定値の初期化
  //
  void reset();

  //
  // このウィンドウで制御するカメラを設定する
  //
  void setControlCamera(Camera *cam)
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
  const GgMatrix &getMm() const
  {
    return mm;
  }

  //
  // ビュー変換行列を得る
  //
  const GgMatrix &getMv(int eye) const
  {
    return mv[eye];
  }

  //
  // Oculus Rift のヘッドラッキングによる移動を得る
  //
  const GLfloat *getPo(int eye) const
  {
    return po[eye];
  }

  //
  // Oculus Rift のヘッドラッキングによる回転の四元数を得る
  //
  const GgQuaternion &getQo(int eye) const
  {
    return qo[eye];
  }

  //
  // Oculus Rift のヘッドラッキングによる回転の変換行列を得る
  //
  GgMatrix getMo(int eye) const;

  //
  // プロジェクション変換行列を得る
  //
  const GgMatrix &getMp(int eye) const
  {
    return mp[eye];
  }

  //
  // メッシュの縦横の格子点数を取り出す
  //
  const GLsizei *getSamples() const
  {
    return samples;
  }

  //
  // メッシュの縦横の格子間隔を取り出す
  //
  const GLfloat *getGap() const
  {
    return gap;
  }

  //
  // スクリーンの幅と高さを取り出す
  //
  const GLfloat *getScreen(int eye) const
  {
    return screen[eye];
  }

  //
  // 焦点距離を取り出す
  //
  GLfloat getFocal() const
  {
    return focal;
  }

  //
  // 背景テクスチャの半径と中心を取り出す
  //
  const GLfloat *getCircle() const
  {
    return circle;
  }

  //
  // ウィンドウのアスペクト比を取り出す
  //
  GLfloat getAspect() const
  {
    return aspect;
  }

  // 物体の表示
  bool showScene;

  // ミラー表示
  bool showMirror;
};

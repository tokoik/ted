#pragma once

//
// ウィンドウ関連の処理
//

// カメラ関連の処理
#include "Camera.h"

// メッセージボックス
#if defined(_WIN32)
#  define NOTIFY(msg) MessageBox(NULL, TEXT(msg), TEXT("TED"), MB_ICONERROR | MB_OK)
#else
#  define NOTIFY(msg) std::cerr << msg << '\n'
#endif

// Oculus Rift 関連の処理
class Oculus;

//
// ウィンドウ関連の処理を担当するクラス
//
class Window
{
  // ウィンドウの識別子
  GLFWwindow* const window;

  // ウィンドウのサイズ
  std::array<GLsizei, 2> size;

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

  // このウィンドウで制御するカメラ
  Camera* camera;

  //
  // ヘッドトラッキング
  //

  // ヘッドトラッキングによる回転
  std::array<GgQuaternion, camCount> qo;

  // ヘッドトラッキングによる位置
  std::array<GgVector, camCount> po;

  // ヘッドトラッキングの変換行列
  std::array<GgMatrix, camCount> mo;

  //
  // 座標変換
  //

  // モデル変換行列
  GgMatrix mm;

  // ビュー変換行列
  std::array<GgMatrix, camCount> mv;

  // 投影変換行列
  std::array<GgMatrix, camCount> mp;

  // ズーム率
  GLfloat zoom;

  //
  // 背景画像
  //

  // 背景テクスチャの半径と中心
  GgVector circle;

  // スクリーンの幅と高さ
  std::array<GgVector, camCount> screen;

  // 焦点距離
  GLfloat focal;

  // 視差
  GLfloat parallax;

  // スクリーンの間隔
  GLfloat offset;

  // 背景の描画に使う矩形から参照する
  friend class Rect;

public:

  //
  // コンストラクタ
  //
  Window(int width = 640, int height = 480,
    const char* title = "GLFW Window",
    GLFWmonitor* monitor = nullptr,
    GLFWwindow* share = nullptr
  );

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
  // 設定値の初期化
  //
  void reset();

  //
  // ビューポートの初期化
  // 
  void resetViewport()
  {
    resize(window, size[0], size[1]);
  }

  //
  // 透視投影変換行列を更新する
  //
  void update();

  //
  // カメラの画角と中心位置を更新する
  //
  void updateCircle();

  //
  // このウィンドウで制御するカメラを設定する
  //
  void setControlCamera(Camera* cam)
  {
    camera = cam;
  }

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
  // Oculus Rift のヘッドラッキングによる移動を得る
  //
  const GgVector& getPo(int eye) const
  {
    return po[eye];
  }

  //
  // Oculus Rift のヘッドラッキングによる回転の四元数を得る
  //
  const GgQuaternion& getQo(int eye) const
  {
    return qo[eye];
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
  const auto& getSamples() const
  {
    return samples;
  }

  //
  // メッシュの縦横の格子間隔を取り出す
  //
  const auto& getGap() const
  {
    return gap;
  }

  //
  // スクリーンの幅と高さを取り出す
  //
  const auto& getScreen(int eye) const
  {
    return screen[eye];
  }

  //
  // 焦点距離を取り出す
  //
  auto getFocal() const
  {
    return focal;
  }

  //
  // スクリーンの間隔を取り出す
  //
  auto getOffset(int eye = 0) const
  {
    return static_cast<GLfloat>(1 - (eye & 1) * 2) * offset;
  }

  //
  // 背景テクスチャの半径と中心を取り出す
  //
  const auto& getCircle() const
  {
    return circle;
  }

  //
  // ウィンドウのサイズを取り出す
  //
  const auto& getSize() const
  {
    return size;
  }

  //
  // ウィンドウのアスペクト比を取り出す
  //
  auto getAspect() const
  {
    return aspect;
  }

  // シーン表示
  bool showScene;

  // ミラー表示
  bool showMirror;

  // メニュー表示
  bool showMenu;

  // Oculus Rift のコンテキスト
  friend class Oculus;
  static Oculus* oculus;

  // Oculus Rift を起動する
  bool startOculus();

  // Oculus Rift を停止する
  void stopOculus();

  // 描画する目を選択する
  void select(int eye);

  // 描画を開始する
  bool start();

  // 描画を完了する
  static void commit(int eye);
};

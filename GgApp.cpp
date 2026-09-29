///
/// アプリケーションのクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///
#include "GgApp.h"

// 各種設定
#include "Config.h"

// 姿勢
#include "Attitude.h"

// シーングラフ
#include "Scene.h"

// 標準ライブラリ
#include <iostream>
#include <cmath>
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <algorithm>

#if defined(_WIN32)
#  include <windows.h>

//
// UTF-8 のメッセージを Windows のダイアログに表示する
//
int showNotification(const char* message)
{
  // アプリケーション内部の UTF-8 を Win32 の UTF-16 へ変換し、
  // 日本語をシステムの ANSI コードページに依存せず表示する。
  const int length{ MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
    message, -1, nullptr, 0) };
  if (length <= 0)
    return MessageBoxW(nullptr, L"Invalid UTF-8 notification.", L"TED", MB_ICONERROR | MB_OK);

  std::wstring wide(static_cast<std::size_t>(length), L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
    message, -1, wide.data(), length);
  return MessageBoxW(nullptr, wide.c_str(), L"TED", MB_ICONERROR | MB_OK);
}
#endif

//
// ハンドトラッキングの使用状態を変更する
//
bool GgApp::setHandTrackingMode(int mode)
{
  // 同じモードなら何もしない
  if (mode == defaults.hand_tracking) return true;

  // 以前のモードのクリーンアップ
  if (defaults.hand_tracking == HAND_TRACKING_LEAP_MOTION)
  {
    // Leap Motion の取得を停止する
    Scene::stopLeapMotion();
  }
  else if (defaults.hand_tracking == HAND_TRACKING_OPENXR)
  {
    // OpenXR が格納した手の姿勢を消去する
    Scene::setLocalHandAttitudes(0, nullptr);
    Scene::setLocalHandAttitudes(1, nullptr);
  }

  // 新しいモードの初期化
  if (mode == HAND_TRACKING_LEAP_MOTION)
  {
    // Leap Motion が使えなければハンドトラッキングを使わない
    if (!Scene::startLeapMotion())
    {
      defaults.hand_tracking = HAND_TRACKING_NONE;
      return false;
    }
  }
  else if (mode == HAND_TRACKING_OPENXR)
  {
#if defined(GG_USE_OPENXR)
    // HMD のハンドトラッキングが使えなければハンドトラッキングを使わない
    if (!OpenXR::getInstance().hasHandTracking())
    {
      defaults.hand_tracking = HAND_TRACKING_NONE;
      return false;
    }
#else
    // OpenXR を使わないときは OpenXR のハンドトラッキングは使えない
    defaults.hand_tracking = HAND_TRACKING_NONE;
    return false;
#endif
  }

  defaults.hand_tracking = mode;
  return true;
}

//
// HMD を使用中か調べる
//
bool GgApp::Window::isHMD()
{
#if defined(GG_USE_OPENXR)
  return OpenXR::getInstance().isInitialized();
#else
  return false;
#endif
}

//
// GgApp::Window コンストラクタ
//
GgApp::Window::Window(int width, int height, const char* title, GLFWmonitor* monitor, GLFWwindow* share)
{
  // 最初のインスタンスで一度だけ実行
  static bool firstTime{ true };
  if (firstTime)
  {
    // SRGB モードでレンダリングできるようにする
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    // OpenGL のバージョンを指定する
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Core Profile を選択する
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(IMGUI_VERSION)
    // ImGui のバージョンをチェックする
    IMGUI_CHECKVERSION();

    // ImGui のコンテキストを作成する
    ImGui::CreateContext();

    // プログラム終了時には ImGui のコンテキストを破棄する
    atexit([] { ImGui::DestroyContext(); });
#endif

    // 実行済みの印をつける
    firstTime = false;
  }

  // Quad Buffer は作成後に追加できないため、起動時設定に応じてステレオバッファを要求する。
  // ドライバが実際に提供したかどうかは、ウィンドウ作成後に GLFW_STEREO 属性で検証する。
  glfwWindowHint(GLFW_STEREO, defaults.display_quadbuffer ? GLFW_TRUE : GLFW_FALSE);

  // GLFW のウィンドウを開く
  window = glfwCreateWindow(width, height, title, monitor, share);

  // ステレオバッファを要求してウィンドウが開けなければ、要求を取り下げて開き直す
  if (!window && defaults.display_quadbuffer)
  {
    glfwWindowHint(GLFW_STEREO, GLFW_FALSE);
    defaults.display_quadbuffer = false;
    window = glfwCreateWindow(width, height, title, monitor, share);
  }

  // ウィンドウが開かれなかったら戻る
  if (!window) return;

  //
  // 初期設定
  //

  // 設定を初期化する
  reset();

  //
  // 表示用のウィンドウの設定
  //

  // 現在のウィンドウを処理対象にする
  glfwMakeContextCurrent(window);

  // ゲームグラフィックス特論の都合にもとづく初期化を行う
  ggInit();

  // フルスクリーンモードでもマウスカーソルを表示する
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  // このインスタンスの this ポインタを記録しておく
  glfwSetWindowUserPointer(window, this);

  // マウスボタンを操作したときの処理を登録する
  glfwSetMouseButtonCallback(window, mouse);

  // マウスホイール操作時に呼び出す処理を登録する
  glfwSetScrollCallback(window, wheel);

  // キーボードを操作した時の処理を登録する
  glfwSetKeyCallback(window, keyboard);

  // ウィンドウのサイズ変更時に呼び出す処理を登録する
  glfwSetFramebufferSizeCallback(window, resize);

  //
  // ジョイスティックの設定
  //

  // ジョイステックの有無を調べて番号を決める
  constexpr int maxJoystick{ 4 };
  for (int i = 0; i < maxJoystick; ++i)
  {
    if (glfwJoystickPresent(i))
    {
      // 存在するジョイスティック番号
      joy = i;

      // スティックの中立位置を求める
      int axesCount;
      const auto* const axes(glfwGetJoystickAxes(joy, &axesCount));

      if (axesCount > 3)
      {
        // 起動直後のスティックの位置を基準にする
        origin[0] = axes[0];
        origin[1] = axes[1];
        origin[2] = axes[2];
        origin[3] = axes[3];
      }

      break;
    }
  }

  //
  // OpenGL の設定
  //

  // 背景色
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

#if defined(IMGUI_VERSION)
  // Setup Platform/Renderer bindings
  // (上で登録したコールバック関数は ImGui のコールバック関数から呼び出される)
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 430");

  //
  // ユーザインタフェースの準備
  //
  ImGuiIO& io = ImGui::GetIO();

  // メニューフォントを読み込む
  const ImFont* const font
  {
    io.Fonts->AddFontFromFileTTF(
      defaults.menu_font.c_str(),
      defaults.menu_font_size,
      NULL,
      io.Fonts->GetGlyphRangesJapanese()
    )
  };
  IM_ASSERT(font != NULL);
#endif

  // sRGB カラースペースに切り替える
  glEnable(GL_FRAMEBUFFER_SRGB);

  // スワップ間隔を待つ
  glfwSwapInterval(1);

  // 実際のフレームバッファのサイズを取得する
  int fboWidth, fboHeight;
  glfwGetFramebufferSize(window, &fboWidth, &fboHeight);

  // 投影変換行列・ビューポートを初期化する
  resize(window, fboWidth, fboHeight);
}

//
// ムーブコンストラクタ
//
GgApp::Window::Window(Window&& w) noexcept
  : window{ w.window }
  , size{ w.size }
  , fboSize{ w.fboSize }
  , width{ w.width }
  , height{ w.height }
  , aspect{ w.aspect }
  , samples{ w.samples }
  , gap{ w.gap }
  , key{ w.key }
  , joy{ w.joy }
  , origin{ w.origin }
  , cx{ w.cx }
  , cy{ w.cy }
  , camera{ w.camera }
  , qo{ w.qo }
  , po{ w.po }
  , mo{ w.mo }
  , mm{ w.mm }
  , mv{ w.mv }
  , mp{ w.mp }
  , zoom{ w.zoom }
  , circle{ w.circle }
  , screen{ w.screen }
  , focal{ w.focal }
  , parallax{ w.parallax }
  , offset{ w.offset }
  , showScene{ w.showScene }
  , showMirror{ w.showMirror }
  , showMenu{ w.showMenu }
{
  // ムーブ元はウィンドウを持たない
  w.window = nullptr;

  // コールバック関数から参照するインスタンスをムーブ先に切り替える
  if (window) glfwSetWindowUserPointer(window, this);
}

//
// ムーブ代入演算子
//
GgApp::Window& GgApp::Window::operator=(Window&& w) noexcept
{
  if (this != &w)
  {
    // 自分のウィンドウを破棄する
    if (window) glfwDestroyWindow(window);

    // ムーブ元のウィンドウと設定を引き継ぐ
    window = w.window;
    size = w.size;
    fboSize = w.fboSize;
    width = w.width;
    height = w.height;
    aspect = w.aspect;
    samples = w.samples;
    gap = w.gap;
    key = w.key;
    joy = w.joy;
    origin = w.origin;
    cx = w.cx;
    cy = w.cy;
    camera = w.camera;
    qo = w.qo;
    po = w.po;
    mo = w.mo;
    mm = w.mm;
    mv = w.mv;
    mp = w.mp;
    zoom = w.zoom;
    circle = w.circle;
    screen = w.screen;
    focal = w.focal;
    parallax = w.parallax;
    offset = w.offset;
    showScene = w.showScene;
    showMirror = w.showMirror;
    showMenu = w.showMenu;

    // ムーブ元はウィンドウを持たない
    w.window = nullptr;

    // コールバック関数から参照するインスタンスをムーブ先に切り替える
    if (window) glfwSetWindowUserPointer(window, this);
  }
  return *this;
}

//
// デストラクタ
//
GgApp::Window::~Window()
{
  // ウィンドウが開かれていなかったら戻る
  if (!window) return;

  // HMD を使っていたら停止する
  stopHMD();

#if defined(IMGUI_VERSION)
  // Shutdown Platform/Renderer bindings
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
#endif

  // 表示用のウィンドウを破棄する
  glfwDestroyWindow(window);
  window = nullptr;
}

//
// イベントを取得してループを継続するなら真を返す
//
GgApp::Window::operator bool()
{
  // イベントを取り出す
  glfwPollEvents();

  // ウィンドウを閉じるべきなら false
  if (glfwWindowShouldClose(window)) return false;

#if defined(LEAP_INTERPORATE_FRAME)
  // Leap Motion の変換行列を更新する
  Scene::update();
#endif

  // 速度を距離に比例させる
  const auto speedFactor((fabs(attitude.position[2]) + 0.2f));

  //
  // ジョイスティックによる操作
  //

  // ジョイスティックが有効なら
  if (joy >= 0 && defaults.use_controller)
  {
    // ボタン
    int btnsCount;
    const auto* const btns{ glfwGetJoystickButtons(joy, &btnsCount) };

    // スティック
    int axesCount;
    const auto* const axes{ glfwGetJoystickAxes(joy, &axesCount) };

    // スティックの速度係数
    const auto axesSpeedFactor(axesSpeedScale * speedFactor);

    // ボタンの数が足りなければボタンは使わない
    constexpr int requiredButtons{ 14 };
    const bool buttonsAvailable{ btns && btnsCount >= requiredButtons };

    // L ボタンと R ボタンの状態
    const auto lrButton(buttonsAvailable ? btns[4] | btns[5] : 0);

    if (axes && axesCount > 3)
    {
      // 物体を左右に移動する
      attitude.position[0] += (axes[0] - origin[0]) * axesSpeedFactor;

      // L ボタンか R ボタンを同時に押していれば
      if (lrButton)
      {
        // 物体を前後に移動する
        attitude.position[2] += (axes[1] - origin[1]) * axesSpeedFactor;
      }
      else
      {
        // 物体を上下に移動する
        attitude.position[1] += (axes[1] - origin[1]) * axesSpeedFactor;
      }

      // 物体を左右に回転する
      attitude.orientation.rotate(ggRotateQuaternion(0.0f, 1.0f, 0.0f, (axes[2] - origin[2]) * axesSpeedFactor));

      // 物体を上下に回転する
      attitude.orientation.rotate(ggRotateQuaternion(1.0f, 0.0f, 0.0f, (axes[3] - origin[3]) * axesSpeedFactor));
    }

    if (buttonsAvailable)
    {
      // B, X ボタンの状態
      const auto parallaxButton(btns[1] - btns[2]);

      // B, X ボタンに変化があれば
      if (parallaxButton)
      {
        // L ボタンか R ボタンを同時に押していれば
        if (lrButton)
        {
          // 背景を左右に回転する
          if (parallaxButton > 0)
          {
            attitude.eyeOrientation[camL] *= Attitude::eyeOrientationStep[0];
            attitude.eyeOrientation[camR] *= Attitude::eyeOrientationStep[0].conjugate();
          }
          else
          {
            attitude.eyeOrientation[camL] *= Attitude::eyeOrientationStep[0].conjugate();
            attitude.eyeOrientation[camR] *= Attitude::eyeOrientationStep[0];
          }
        }
        else
        {
          // スクリーンの間隔を調整する
          offset = offsetDefault + offsetStep * (attitude.offset += parallaxButton);
        }
      }

      // Y, A ボタンの状態
      const auto zoomButton(btns[3] - btns[0]);

      // Y, A ボタンに変化があれば
      if (zoomButton)
      {
        // L ボタンか R ボタンを同時に押していれば
        if (lrButton)
        {
          // 背景を上下に回転する
          if (zoomButton > 0)
          {
            attitude.eyeOrientation[camL] *= Attitude::eyeOrientationStep[1];
            attitude.eyeOrientation[camR] *= Attitude::eyeOrientationStep[1].conjugate();
          }
          else
          {
            attitude.eyeOrientation[camL] *= Attitude::eyeOrientationStep[1].conjugate();
            attitude.eyeOrientation[camR] *= Attitude::eyeOrientationStep[1];
          }
        }
        else
        {
          // 焦点距離を調整する
          focal = 1.0f / (1.0f - backFocalStep * (attitude.backAdjust[0] += zoomButton));
        }
      }

      // 十字キーの左右ボタンの状態
      const auto textureXButton(btns[11] - btns[13]);

      // 十字キーの左右ボタンに変化があれば
      if (textureXButton)
      {
        // L ボタンか R ボタンを同時に押していれば
        if (lrButton)
        {
          // 背景に対する横方向の画角を調整する
          circle[0] = defaults.camera_fov_x + fovStep * (attitude.circleAdjust[0] += textureXButton);
        }
        else
        {
          // 背景の横位置を調整する
          circle[2] = defaults.camera_center_x + shiftStep * (attitude.circleAdjust[2] += textureXButton);
        }
      }

      // 十字キーの上下ボタンの状態
      const auto textureYButton(btns[10] - btns[12]);

      // 十字キーの上下ボタンに変化があれば
      if (textureYButton)
      {
        // L ボタンか R ボタンを同時に押していれば
        if (lrButton)
        {
          // 背景に対する縦方向の画角を調整する
          circle[1] = defaults.camera_fov_y + fovStep * (attitude.circleAdjust[1] += textureYButton);
        }
        else
        {
          // 背景の縦位置を調整する
          circle[3] = defaults.camera_center_y + shiftStep * (attitude.circleAdjust[3] += textureYButton);
        }
      }

      // 設定をリセットする
      if (btns[7]) reset();
    }
  }

  //
  // マウスによる操作
  //

  // マウスの位置
  double x, y;

#if defined(IMGUI_VERSION)
  // メニューを表示するとき
  if (showMenu)
  {
    // ImGui の新規フレームを作成する (ImGui::NewFrame() は Menu::show() で呼び出す)
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    // ImGui がマウスを使うときは Window クラスのマウス位置を更新しない
    if (ImGui::GetIO().WantCaptureMouse) return true;
  }

  // マウスの現在位置を調べる
  const ImGuiIO& io{ ImGui::GetIO() };
  x = io.MousePos.x;
  y = io.MousePos.y;
#else
  // マウスの位置を調べる
  glfwGetCursorPos(window, &x, &y);
#endif

  // 左ボタンドラッグ
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1))
  {
    // 物体の位置を移動する
    const auto speed(speedScale * speedFactor);
    attitude.position[0] += static_cast<GLfloat>(x - cx) * speed;
    attitude.position[1] += static_cast<GLfloat>(cy - y) * speed;
    cx = x;
    cy = y;
  }

  // 右ボタンドラッグ
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2))
  {
    // 物体を回転する
    attitude.orientation.motion(static_cast<float>(x), static_cast<float>(y));
  }

  return true;
}

//
// カラーバッファを入れ替える
//
void GgApp::Window::swapBuffers()
{
  // エラーチェック
  ggError();

#if defined(IMGUI_VERSION)
  // メニューを表示するとき
  if (showMenu)
  {
    // OpenXR の描画でガンマ補正が無効になっていてもデスクトップ表示と同じ色で描く
    glEnable(GL_FRAMEBUFFER_SRGB);

    // Dear ImGui のメニューを描画する (ImGui::Render() は Menu::show() で呼び出す)
    ImDrawData* const imDrawData{ ImGui::GetDrawData() };
    if (imDrawData) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
  }
#endif

  // カラーバッファを入れ替える
  glfwSwapBuffers(window);
}

//
// ウィンドウのサイズ変更時の処理
//
void GgApp::Window::resize(GLFWwindow* window, int width, int height)
{
  // このインスタンスの this ポインタを得る
  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

  if (instance)
  {
    // ウィンドウのサイズを保存する
    glfwGetWindowSize(window, &instance->size[0], &instance->size[1]);

    // フレームバッファのサイズを保存する
    instance->fboSize[0] = width;
    instance->fboSize[1] = height;

    // 最小化されたときはビューポートを更新しない
    if (width <= 0 || height <= 0) return;

    // HMD 使用時以外
    if (!isHMD())
    {
      if (defaults.display_mode == OVERLAY)
      {
        // ビューポートの横半分を画面の横サイズにする
        width /= 2;
        instance->aspect = static_cast<GLfloat>(width) / static_cast<GLfloat>(height);
      }
      else
      {
        // リサイズ後のディスプレイのアスペクト比を求める
        instance->aspect = defaults.display_aspect
          ? defaults.display_aspect
          : static_cast<GLfloat>(width) / static_cast<GLfloat>(height);

        // リサイズ後のビューポートを設定する
        switch (defaults.display_mode)
        {
        case SIDE_BY_SIDE:
          width /= 2;
          break;

        case TOP_AND_BOTTOM:
          height /= 2;
          break;

        default:
          break;
        }
      }
    }

    // ビューポートの大きさを保存しておく
    instance->width = width;
    instance->height = height;

    // 背景描画用のメッシュの縦横の格子点数を求める
    instance->samples[0] = static_cast<GLsizei>(sqrt(instance->aspect * defaults.camera_texture_samples));
    instance->samples[1] = defaults.camera_texture_samples / instance->samples[0];

    // 背景描画用のメッシュの縦横の格子間隔を求める
    instance->gap[0] = 2.0f / static_cast<GLfloat>(instance->samples[0] - 1);
    instance->gap[1] = 2.0f / static_cast<GLfloat>(instance->samples[1] - 1);

    // 透視投影変換行列を求める
    instance->update();

    // トラックボール処理の範囲を設定する
    attitude.orientation.region(static_cast<float>(width) / angleScale, static_cast<float>(height) / angleScale);
  }
}

//
// マウスボタンを操作したときの処理
//
void GgApp::Window::mouse(GLFWwindow* window, int button, int action, int mods)
{
#if defined(IMGUI_VERSION)
  // ImGui がマウスを使うときは何もしない
  if (ImGui::GetIO().WantCaptureMouse) return;
#endif

  // このインスタンスの this ポインタを得る
  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

  if (instance)
  {
    // マウスの現在位置を得る
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    switch (button)
    {
    case GLFW_MOUSE_BUTTON_1:
      if (action)
      {
        // 左ドラッグ開始位置を記録する
        instance->cx = x;
        instance->cy = y;
      }
      break;

    case GLFW_MOUSE_BUTTON_2:
      if (action)
      {
        // 右ドラッグによるトラックボール処理を開始する
        attitude.orientation.begin(static_cast<float>(x), static_cast<float>(y));
      }
      else
      {
        // 右ドラッグによるトラックボール処理を終了する
        attitude.orientation.end(static_cast<float>(x), static_cast<float>(y));
      }
      break;

    default:
      break;
    }
  }
}

//
// マウスホイール操作時の処理
//
void GgApp::Window::wheel(GLFWwindow* window, double x, double y)
{
#if defined(IMGUI_VERSION)
  // ImGui がマウスを使うときは何もしない
  if (ImGui::GetIO().WantCaptureMouse) return;
#endif

  // このインスタンスの this ポインタを得る
  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

  if (instance)
  {
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT))
    {
      // シフトキーを押していればズーム率を調整する
      attitude.foreAdjust[0] += static_cast<int>(y);
      instance->update();
    }
    else
    {
      // 物体を前後に移動する (遠いほど速く移動する)
      const GLfloat advSpeed((fabs(attitude.position[2]) * 5.0f + 1.0f) * wheelYStep * static_cast<GLfloat>(y));
      attitude.position[2] += advSpeed;
    }
  }
}

//
// キーボードをタイプした時の処理
//
void GgApp::Window::keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
#if defined(IMGUI_VERSION)
  // ImGui がキーボードを使うときは何もしない
  if (ImGui::GetIO().WantCaptureKeyboard) return;
#endif

  // このインスタンスの this ポインタを得る
  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

  if (instance)
  {
    if (action != GLFW_RELEASE)
    {
      // 修飾キーの状態
      const auto shiftKey{ glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) };
      const auto ctrlKey{ glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) };
      const auto altKey{ glfwGetKey(window, GLFW_KEY_LEFT_ALT) || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) };

      // 最後にタイプしたキーを記録する
      instance->key = key;

      switch (key)
      {
      case GLFW_KEY_R:
        // 設定をリセットする
        instance->reset();
        break;

      case GLFW_KEY_M:
        // メニューの表示・非表示を切り替える
        instance->showMenu = !instance->showMenu;
        break;

      case GLFW_KEY_S:
        // シーンの表示・非表示を切り替える
        instance->showScene = !instance->showScene;
        break;

      case GLFW_KEY_Z:
        // ズーム率を調整する
        if (shiftKey)
          --attitude.foreAdjust[0];
        else
          ++attitude.foreAdjust[0];
        instance->update();
        break;

      case GLFW_KEY_P:
        // HMD では視差は調整しない
        if (isHMD()) break;

        // 視差を調整する
        if (shiftKey)
          --attitude.parallax;
        else
          ++attitude.parallax;
        instance->update();
        break;

      case GLFW_KEY_E:
        // カメラの露出を調整する
        if (!instance->camera) break;
        if (shiftKey)
          instance->camera->decreaseExposure();
        else
          instance->camera->increaseExposure();
        break;

      case GLFW_KEY_G:
        // カメラの利得を調整する
        if (!instance->camera) break;
        if (shiftKey)
          instance->camera->decreaseGain();
        else
          instance->camera->increaseGain();
        break;

      case GLFW_KEY_UP:
        if (altKey)
        {
          // 背景を上に回転する (Ctrl なら右だけ, Shift なら左だけ)
          if (!ctrlKey) attitude.eyeOrientation[camL] *= Attitude::eyeOrientationStep[1];
          if (!shiftKey) attitude.eyeOrientation[camR] *= Attitude::eyeOrientationStep[1];
        }
        else if (ctrlKey)
        {
          // 背景に対する縦方向の画角を広げる
          ++attitude.circleAdjust[1];
        }
        else if (shiftKey)
        {
          // 背景を上に移動する
          ++attitude.circleAdjust[3];
        }
        else
        {
          // 焦点距離を延ばす
          ++attitude.backAdjust[0];
        }
        instance->updateCircle();
        break;

      case GLFW_KEY_DOWN:
        if (altKey)
        {
          // 背景を下に回転する (Ctrl なら右だけ, Shift なら左だけ)
          if (!ctrlKey) attitude.eyeOrientation[camL] *= Attitude::eyeOrientationStep[1].conjugate();
          if (!shiftKey) attitude.eyeOrientation[camR] *= Attitude::eyeOrientationStep[1].conjugate();
        }
        else if (ctrlKey)
        {
          // 背景に対する縦方向の画角を狭める
          --attitude.circleAdjust[1];
        }
        else if (shiftKey)
        {
          // 背景を下に移動する
          --attitude.circleAdjust[3];
        }
        else
        {
          // 焦点距離を縮める
          --attitude.backAdjust[0];
        }
        instance->updateCircle();
        break;

      case GLFW_KEY_RIGHT:
        if (altKey)
        {
          // 背景を右に回転する (Ctrl なら右だけ, Shift なら左だけ)
          if (!ctrlKey) attitude.eyeOrientation[camL] *= Attitude::eyeOrientationStep[0].conjugate();
          if (!shiftKey) attitude.eyeOrientation[camR] *= Attitude::eyeOrientationStep[0].conjugate();
        }
        else if (ctrlKey)
        {
          // 背景に対する横方向の画角を広げる
          ++attitude.circleAdjust[0];
        }
        else if (shiftKey)
        {
          // 背景を右に移動する
          ++attitude.circleAdjust[2];
        }
        else if (defaults.display_mode != MONOCULAR)
        {
          // スクリーンの間隔を広げる
          ++attitude.offset;
        }
        instance->updateCircle();
        break;

      case GLFW_KEY_LEFT:
        if (altKey)
        {
          // 背景を左に回転する (Ctrl なら右だけ, Shift なら左だけ)
          if (!ctrlKey) attitude.eyeOrientation[camL] *= Attitude::eyeOrientationStep[0];
          if (!shiftKey) attitude.eyeOrientation[camR] *= Attitude::eyeOrientationStep[0];
        }
        else if (ctrlKey)
        {
          // 背景に対する横方向の画角を狭める
          --attitude.circleAdjust[0];
        }
        else if (shiftKey)
        {
          // 背景を左に移動する
          --attitude.circleAdjust[2];
        }
        else if (defaults.display_mode != MONOCULAR)
        {
          // スクリーンの間隔を狭める
          --attitude.offset;
        }
        instance->updateCircle();
        break;

      default:
        break;
      }
    }
  }
}

//
// 表示モードの変更
//
bool GgApp::Window::setDisplayMode(int mode)
{
  if (mode < MONOCULAR || mode > OPENXR) return false;

  // 起動時は設定値だけが先に反映されているため、同じモードでも必要な資源を検証する。
  // 実行中の再選択では、既に確保済みの資源をそのまま利用する。
  if (mode == defaults.display_mode)
  {
    if (mode == QUADBUFFER) return isQuadBufferAvailable();
    if (mode == OPENXR) return isHMD() || startHMD();
    return true;
  }

  if (mode == OPENXR)
  {
    // OpenXR の全資源を確保できた場合だけ、設定値を HMD 表示へ進める。
    if (!startHMD()) return false;
  }
  else
  {
    // Quad Buffer はウィンドウ作成後に追加できないため、実際の属性で利用可否を判定する。
    if (mode == QUADBUFFER && !isQuadBufferAvailable()) return false;

    // HMD 以外へ移るときは、OpenXR 資源を先に解放して通常描画へ戻す。
    if (defaults.display_mode == OPENXR) stopHMD();
  }

  // 資源の準備が完了してから実行時設定とビューポートを同時に更新する。
  defaults.display_mode = mode;
  resetViewport();
  return true;
}

//
// 前方面と後方面の変更
//
bool GgApp::Window::setClipPlanes(float nearPlane, float farPlane)
{
  if (nearPlane <= 0.0f || farPlane <= nearPlane) return false;

  defaults.display_near = nearPlane;
  defaults.display_far = farPlane;
  update();
  return true;
}

//
// 設定値の初期化
//
void GgApp::Window::reset()
{
  attitude.position = attitude.initialPosition;
  attitude.orientation.reset(attitude.initialOrientation.getQuaternion());
  for (int cam = 0; cam < camCount; ++cam) attitude.eyeOrientation[cam] = attitude.initialEyeOrientation[cam];
  attitude.foreAdjust = attitude.initialForeAdjust;
  attitude.backAdjust = attitude.initialBackAdjust;
  attitude.parallax = attitude.initialParallax;
  attitude.offset = attitude.initialOffset;
  update();
  attitude.circleAdjust = attitude.initialCircleAdjust;
  updateCircle();

#if defined(GG_USE_OPENXR)
  // 次に取得する頭部位置を、シーン座標の新しい原点として採用する
  OpenXR::getInstance().resetOrigin();
#endif
}

//
// 透視投影変換行列を更新する
//
void GgApp::Window::update()
{
  // ズーム率・焦点距離・スクリーンの間隔を求める
  zoom = 1.0f / (1.0f - zoomStep * attitude.foreAdjust[0]);
  focal = 1.0f / (1.0f - backFocalStep * attitude.backAdjust[0]);
  offset = offsetDefault + offsetStep * attitude.offset;

  // HMD 使用時の透視投影変換行列は updateHMD() で求める
  if (isHMD()) return;

  // 視差
  parallax = (defaults.display_mode == MONOCULAR ? 0.0f : defaultParallax) + parallaxStep * attitude.parallax;

  // 前方面におけるスクリーンの高さと幅の半分
  const GLfloat screenHeight{ defaults.display_center / defaults.display_distance };
  const GLfloat screenWidth{ screenHeight * aspect };

  // 視差による視錐台の水平方向のずれ
  const GLfloat shift{ defaults.display_mode != MONOCULAR
    ? parallax * defaults.display_near / defaults.display_distance : 0.0f };

  // ズーム率を反映した前方面の大きさの係数
  const GLfloat zf{ defaults.display_near / zoom };

  // 左目の視錐台
  const GLfloat fovL[]
  {
    -screenWidth + shift,
    screenWidth + shift,
    -screenHeight,
    screenHeight
  };

  // 左目の透視投影変換行列を求める
  mp[0].loadFrustum(fovL[0] * zf, fovL[1] * zf, fovL[2] * zf, fovL[3] * zf,
    defaults.display_near, defaults.display_far);

  // 左目のスクリーンの幅と高さ・中心位置を求める
  screen[0][0] = (fovL[1] - fovL[0]) * 0.5f;
  screen[0][1] = (fovL[3] - fovL[2]) * 0.5f;
  screen[0][2] = (fovL[1] + fovL[0]) * 0.5f;
  screen[0][3] = (fovL[3] + fovL[2]) * 0.5f;

  // 立体視のとき
  if (defaults.display_mode != MONOCULAR)
  {
    // 右目の視錐台
    const GLfloat fovR[] =
    {
      -screenWidth - shift,
      screenWidth - shift,
      -screenHeight,
      screenHeight,
    };

    // 右目の透視投影変換行列を求める
    mp[1].loadFrustum(fovR[0] * zf, fovR[1] * zf, fovR[2] * zf, fovR[3] * zf,
      defaults.display_near, defaults.display_far);

    // 右目のスクリーンの幅と高さ・中心位置を求める
    screen[1][0] = (fovR[1] - fovR[0]) * 0.5f;
    screen[1][1] = (fovR[3] - fovR[2]) * 0.5f;
    screen[1][2] = (fovR[1] + fovR[0]) * 0.5f;
    screen[1][3] = (fovR[3] + fovR[2]) * 0.5f;
  }
}

//
// カメラの画角と中心位置を更新する
//
void GgApp::Window::updateCircle()
{
  circle[0] = defaults.camera_fov_x + fovStep * attitude.circleAdjust[0];
  circle[1] = defaults.camera_fov_y + fovStep * attitude.circleAdjust[1];
  circle[2] = defaults.camera_center_x + shiftStep * attitude.circleAdjust[2];
  circle[3] = defaults.camera_center_y + shiftStep * attitude.circleAdjust[3];
}

//
// HMD を起動する
//
bool GgApp::Window::startHMD()
{
#if defined(GG_USE_OPENXR)
  try
  {
    // OpenXR のセッションを作成する (セッションは begin() の中で READY になってから開始する)
    return OpenXR::initialize(*this, XR_REFERENCE_SPACE_TYPE_STAGE, "TED").isInitialized();
  }
  catch (const std::exception& e)
  {
    // HMD が接続されていないときなどは通常表示に戻す
    std::cerr << "OpenXR start failed: " << e.what() << '\n';
    return false;
  }
#else
  return false;
#endif
}

//
// HMD を停止する
//
void GgApp::Window::stopHMD()
{
#if defined(GG_USE_OPENXR)
  // OpenXR を使っていなければ何もしない
  auto& openxr{ OpenXR::getInstance() };
  if (!openxr.isInitialized()) return;

  // OpenXR のセッションを破棄する
  openxr.terminate();

  // OpenXR の手の姿勢が残らないようにする
  if (defaults.hand_tracking == HAND_TRACKING_OPENXR)
  {
    Scene::setLocalHandAttitudes(0, nullptr);
    Scene::setLocalHandAttitudes(1, nullptr);
  }

  // terminate() で無効にされた sRGB カラースペースに戻す
  glEnable(GL_FRAMEBUFFER_SRGB);
#endif
}

//
// HMD の視点の姿勢と視野角から、左右の目の変換行列とスクリーンを更新する
//
void GgApp::Window::updateHMD()
{
#if defined(GG_USE_OPENXR)
  const auto& openxr{ OpenXR::getInstance() };

  // 視点の姿勢が得られていなければ何もしない
  if (!openxr.isPoseValid()) return;

  // シーン座標の原点 (起動時または回復時の頭部中心位置)
  const auto& origin{ openxr.getOriginPosition() };

  // 近クリップ面の位置 (ズーム対応)
  const GLfloat zf{ defaults.display_near / zoom };

  // 左右の目について
  const int eyes{ std::min(static_cast<int>(openxr.getViewCount()), static_cast<int>(camCount)) };
  for (int eye = 0; eye < eyes; ++eye)
  {
    // libOVR と同じく、OpenXR の姿勢を逆回転のビュー行列として扱う
    const auto& p{ openxr.getPose(eye).position };
    const auto& o{ openxr.getPose(eye).orientation };
    po[eye] = GgVector{ p.x, p.y, p.z, 1.0f };
    qo[eye] = GgQuaternion{ o.x, o.y, o.z, -o.w };
    mo[eye] = qo[eye].getMatrix();

    // 視野角 (FOV) 情報を取得する
    const auto& fov{ openxr.getFov(eye) };

    // 背景画像もシーンと同じ目別 FOV で投影する
    const GLfloat left{ tanf(fov.angleLeft) * focal };
    const GLfloat right{ tanf(fov.angleRight) * focal };
    const GLfloat down{ tanf(fov.angleDown) * focal };
    const GLfloat up{ tanf(fov.angleUp) * focal };
    screen[eye][0] = (right - left) * 0.5f;
    screen[eye][1] = (up - down) * 0.5f;
    screen[eye][2] = (right + left) * 0.5f;
    screen[eye][3] = (up + down) * 0.5f;

    // 視差補正シフト量
    const GLfloat shift{ static_cast<GLfloat>((1 - eye * 2) * attitude.parallax) * parallaxStep };

    // 視野角 (角度のタンジェント) から視錐台の透視投影変換行列を求める
    mp[eye].loadFrustum(
      (shift + tanf(fov.angleLeft)) * zf, (shift + tanf(fov.angleRight)) * zf,
      tanf(fov.angleDown) * zf, tanf(fov.angleUp) * zf,
      defaults.display_near, defaults.display_far
    );

    // 頭部原点からの相対的な各眼位置を、ワールド座標から眼座標へ移すビュー平行移動 T^-1 にする
    mv[eye].loadTranslate(-(p.x - origin[0]), -(p.y - origin[1]), -(p.z - origin[2]));
  }
#endif
}

//
// 描画を開始する (デスクトップ表示)
//
bool GgApp::Window::start()
{
  // 設定からローカルモデルの座標変換行列を作成し、シーングラフの基準行列として設定する
  mm = ggTranslate(attitude.position) * attitude.orientation.getMatrix();
  Scene::setup(mm);

  return true;
}

//
// 描画する目を選択する (デスクトップ表示)
//
void GgApp::Window::select(int eye)
{
  switch (defaults.display_mode)
  {
  case MONOCULAR:
    // 分割表示から戻った場合も、ビューポートをウィンドウ全体へ戻す
    glViewport(0, 0, width, height);
    glClear(GL_DEPTH_BUFFER_BIT);
    break;

  case TOP_AND_BOTTOM:
    // 左目は上半分、右目は下半分に描く
    if (eye == camL)
    {
      glViewport(0, height, width, height);
      glClear(GL_DEPTH_BUFFER_BIT);
    }
    else
    {
      glViewport(0, 0, width, height);
    }
    break;

  case SIDE_BY_SIDE:
  case OVERLAY:
    // 左目は左半分、右目は右半分に描く
    if (eye == camL)
    {
      glViewport(0, 0, width, height);
      glClear(GL_DEPTH_BUFFER_BIT);
    }
    else
    {
      glViewport(width, 0, width, height);
    }
    break;

  case QUADBUFFER:
    // 左右のバックバッファに描く
    glDrawBuffer(eye == camL ? GL_BACK_LEFT : GL_BACK_RIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);
    break;

  default:
    break;
  }

  // 視差に応じて目の位置を左右にずらす
  mv[eye] = ggTranslate(static_cast<GLfloat>(1 - eye * 2) * parallax, 0.0f, 0.0f);
}

//
// 描画を完了する (デスクトップ表示)
//
void GgApp::Window::commit(int eye)
{
  // デスクトップ表示では追加のバッファコミットは不要
}

#if defined(GG_USE_OPENXR)

namespace
{
  //
  // OpenXR の関数の戻り値を文字列にする
  //
  std::string xrMessage(XrInstance instance, XrResult result, const std::string& message)
  {
    char buffer[XR_MAX_RESULT_STRING_SIZE]{ '\0' };
    if (instance == XR_NULL_HANDLE
      || XR_FAILED(xrResultToString(instance, result, buffer))
      || buffer[0] == '\0')
    {
      std::snprintf(buffer, sizeof buffer, "XrResult(%d)", static_cast<int>(result));
    }
    return message + ": " + buffer;
  }

  //
  // OpenXR の関数の戻り値を検査して, エラーなら例外を投げる
  //
  void xrCheck(XrInstance instance, XrResult result, const std::string& message)
  {
    if (XR_SUCCEEDED(result)) return;
    throw std::runtime_error(xrMessage(instance, result, message));
  }

  //
  // OpenXR の関数の戻り値を検査して, エラーなら標準エラー出力に報告する
  //
  bool xrWarn(XrInstance instance, XrResult result, const std::string& message)
  {
    if (XR_SUCCEEDED(result)) return true;
    std::cerr << "OpenXR: " << xrMessage(instance, result, message) << '\n';
    return false;
  }

  //
  // 固定長の文字列に安全にコピーする
  //
  void xrCopyString(char* destination, size_t size, const char* source)
  {
    if (size == 0) return;
    const auto length{ std::min(std::strlen(source), size - 1) };
    std::memcpy(destination, source, length);
    destination[length] = '\0';
  }
}

//
// コンストラクタ
//
GgApp::OpenXR::OpenXR()
{
}

//
// デストラクタ
//
GgApp::OpenXR::~OpenXR()
{
  // このオブジェクトは関数内 static なので, 破棄されるのは main() が
  // 終了した後, すなわち OpenGL のコンテキストが失われた後である.
  // したがってここでは OpenGL の資源には触れず, OpenXR のハンドルだけを
  // 解放する (OpenGL の資源は terminate() で解放しておくこと).
  destroyXr();
}

//
// アクションシステムを初期化する
//
void GgApp::OpenXR::initActions()
{
  // アクションセットの作成
  XrActionSetCreateInfo actionSetInfo{ XR_TYPE_ACTION_SET_CREATE_INFO };
  xrCopyString(actionSetInfo.actionSetName, sizeof actionSetInfo.actionSetName, "gameplay");
  xrCopyString(actionSetInfo.localizedActionSetName, sizeof actionSetInfo.localizedActionSetName, "Gameplay");
  actionSetInfo.priority = 0;
  xrCheck(instance, xrCreateActionSet(instance, &actionSetInfo, &actionSet),
    "Can't create the OpenXR action set");

  // サブアクションパスの取得
  xrCheck(instance, xrStringToPath(instance, "/user/hand/left", &handSubactionPath[Hand::Left]),
    "Can't convert the path of the left hand");
  xrCheck(instance, xrStringToPath(instance, "/user/hand/right", &handSubactionPath[Hand::Right]),
    "Can't convert the path of the right hand");

  // アクション作成ヘルパー
  auto createAction = [this](const char* name, const char* localizedName, XrActionType type, XrAction& action)
  {
    XrActionCreateInfo createInfo{ XR_TYPE_ACTION_CREATE_INFO };
    xrCopyString(createInfo.actionName, sizeof createInfo.actionName, name);
    xrCopyString(createInfo.localizedActionName, sizeof createInfo.localizedActionName, localizedName);
    createInfo.actionType = type;
    createInfo.countSubactionPaths = Hand::Count;
    createInfo.subactionPaths = handSubactionPath;
    xrCheck(instance, xrCreateAction(actionSet, &createInfo, &action),
      std::string("Can't create the OpenXR action \"") + name + "\"");
  };

  createAction("aim_pose", "Aim Pose", XR_ACTION_TYPE_POSE_INPUT, aimPoseAction);
  createAction("grip_pose", "Grip Pose", XR_ACTION_TYPE_POSE_INPUT, gripPoseAction);
  createAction("trigger", "Trigger", XR_ACTION_TYPE_FLOAT_INPUT, triggerAction);
  createAction("grip", "Grip", XR_ACTION_TYPE_FLOAT_INPUT, gripAction);
  createAction("thumbstick", "Thumbstick", XR_ACTION_TYPE_VECTOR2F_INPUT, thumbstickAction);
  createAction("thumbstick_click", "Thumbstick Click", XR_ACTION_TYPE_BOOLEAN_INPUT, thumbstickClickAction);
  createAction("primary_button", "Primary Button", XR_ACTION_TYPE_BOOLEAN_INPUT, primaryButtonAction);
  createAction("secondary_button", "Secondary Button", XR_ACTION_TYPE_BOOLEAN_INPUT, secondaryButtonAction);
  createAction("menu_button", "Menu Button", XR_ACTION_TYPE_BOOLEAN_INPUT, menuButtonAction);
  createAction("haptic", "Haptic Vibration", XR_ACTION_TYPE_VIBRATION_OUTPUT, hapticAction);

  // バインディング設定ヘルパー (対応していない対話プロファイルは読み飛ばす)
  auto suggestBindings = [this](const char* profileStr, const std::vector<std::pair<XrAction, const char*>>& bindings)
  {
    XrPath profilePath{ XR_NULL_PATH };
    if (XR_FAILED(xrStringToPath(instance, profileStr, &profilePath))) return;

    std::vector<XrActionSuggestedBinding> suggestedBindings;
    suggestedBindings.reserve(bindings.size());
    for (const auto& [action, pathStr] : bindings)
    {
      XrPath path{ XR_NULL_PATH };
      if (XR_FAILED(xrStringToPath(instance, pathStr, &path))) continue;
      suggestedBindings.push_back(XrActionSuggestedBinding{ action, path });
    }
    if (suggestedBindings.empty()) return;

    XrInteractionProfileSuggestedBinding profileSuggestedBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
    profileSuggestedBindings.interactionProfile = profilePath;
    profileSuggestedBindings.suggestedBindings = suggestedBindings.data();
    profileSuggestedBindings.countSuggestedBindings = static_cast<uint32_t>(suggestedBindings.size());
    xrWarn(instance, xrSuggestInteractionProfileBindings(instance, &profileSuggestedBindings),
      std::string("Can't suggest the bindings for ") + profileStr);
  };

  // Simple Controller のバインディング (すべてのランタイムが対応する最小限のもの)
  suggestBindings("/interaction_profiles/khr/simple_controller", {
    { aimPoseAction, "/user/hand/left/input/aim/pose" },
    { aimPoseAction, "/user/hand/right/input/aim/pose" },
    { gripPoseAction, "/user/hand/left/input/grip/pose" },
    { gripPoseAction, "/user/hand/right/input/grip/pose" },
    { triggerAction, "/user/hand/left/input/select/click" },
    { triggerAction, "/user/hand/right/input/select/click" },
    { menuButtonAction, "/user/hand/left/input/menu/click" },
    { menuButtonAction, "/user/hand/right/input/menu/click" },
    { hapticAction, "/user/hand/left/output/haptic" },
    { hapticAction, "/user/hand/right/output/haptic" }
  });

  // Meta (Oculus) Touch コントローラーのバインディング
  suggestBindings("/interaction_profiles/oculus/touch_controller", {
    { aimPoseAction, "/user/hand/left/input/aim/pose" },
    { aimPoseAction, "/user/hand/right/input/aim/pose" },
    { gripPoseAction, "/user/hand/left/input/grip/pose" },
    { gripPoseAction, "/user/hand/right/input/grip/pose" },
    { triggerAction, "/user/hand/left/input/trigger/value" },
    { triggerAction, "/user/hand/right/input/trigger/value" },
    { gripAction, "/user/hand/left/input/squeeze/value" },
    { gripAction, "/user/hand/right/input/squeeze/value" },
    { thumbstickAction, "/user/hand/left/input/thumbstick" },
    { thumbstickAction, "/user/hand/right/input/thumbstick" },
    { thumbstickClickAction, "/user/hand/left/input/thumbstick/click" },
    { thumbstickClickAction, "/user/hand/right/input/thumbstick/click" },
    { primaryButtonAction, "/user/hand/left/input/x/click" },
    { primaryButtonAction, "/user/hand/right/input/a/click" },
    { secondaryButtonAction, "/user/hand/left/input/y/click" },
    { secondaryButtonAction, "/user/hand/right/input/b/click" },
    { menuButtonAction, "/user/hand/left/input/menu/click" },
    { hapticAction, "/user/hand/left/output/haptic" },
    { hapticAction, "/user/hand/right/output/haptic" }
  });

  // HTC Vive コントローラーのバインディング
  suggestBindings("/interaction_profiles/htc/vive_controller", {
    { aimPoseAction, "/user/hand/left/input/aim/pose" },
    { aimPoseAction, "/user/hand/right/input/aim/pose" },
    { gripPoseAction, "/user/hand/left/input/grip/pose" },
    { gripPoseAction, "/user/hand/right/input/grip/pose" },
    { triggerAction, "/user/hand/left/input/trigger/value" },
    { triggerAction, "/user/hand/right/input/trigger/value" },
    { gripAction, "/user/hand/left/input/squeeze/click" },
    { gripAction, "/user/hand/right/input/squeeze/click" },
    { thumbstickAction, "/user/hand/left/input/trackpad" },
    { thumbstickAction, "/user/hand/right/input/trackpad" },
    { thumbstickClickAction, "/user/hand/left/input/trackpad/click" },
    { thumbstickClickAction, "/user/hand/right/input/trackpad/click" },
    { menuButtonAction, "/user/hand/left/input/menu/click" },
    { menuButtonAction, "/user/hand/right/input/menu/click" },
    { hapticAction, "/user/hand/left/output/haptic" },
    { hapticAction, "/user/hand/right/output/haptic" }
  });

  // Valve Index コントローラーのバインディング
  suggestBindings("/interaction_profiles/valve/index_controller", {
    { aimPoseAction, "/user/hand/left/input/aim/pose" },
    { aimPoseAction, "/user/hand/right/input/aim/pose" },
    { gripPoseAction, "/user/hand/left/input/grip/pose" },
    { gripPoseAction, "/user/hand/right/input/grip/pose" },
    { triggerAction, "/user/hand/left/input/trigger/value" },
    { triggerAction, "/user/hand/right/input/trigger/value" },
    { gripAction, "/user/hand/left/input/squeeze/value" },
    { gripAction, "/user/hand/right/input/squeeze/value" },
    { thumbstickAction, "/user/hand/left/input/thumbstick" },
    { thumbstickAction, "/user/hand/right/input/thumbstick" },
    { thumbstickClickAction, "/user/hand/left/input/thumbstick/click" },
    { thumbstickClickAction, "/user/hand/right/input/thumbstick/click" },
    { primaryButtonAction, "/user/hand/left/input/a/click" },
    { primaryButtonAction, "/user/hand/right/input/a/click" },
    { secondaryButtonAction, "/user/hand/left/input/b/click" },
    { secondaryButtonAction, "/user/hand/right/input/b/click" },
    { hapticAction, "/user/hand/left/output/haptic" },
    { hapticAction, "/user/hand/right/output/haptic" }
  });

  // Microsoft Mixed Reality モーションコントローラーのバインディング
  suggestBindings("/interaction_profiles/microsoft/motion_controller", {
    { aimPoseAction, "/user/hand/left/input/aim/pose" },
    { aimPoseAction, "/user/hand/right/input/aim/pose" },
    { gripPoseAction, "/user/hand/left/input/grip/pose" },
    { gripPoseAction, "/user/hand/right/input/grip/pose" },
    { triggerAction, "/user/hand/left/input/trigger/value" },
    { triggerAction, "/user/hand/right/input/trigger/value" },
    { gripAction, "/user/hand/left/input/squeeze/click" },
    { gripAction, "/user/hand/right/input/squeeze/click" },
    { thumbstickAction, "/user/hand/left/input/thumbstick" },
    { thumbstickAction, "/user/hand/right/input/thumbstick" },
    { thumbstickClickAction, "/user/hand/left/input/thumbstick/click" },
    { thumbstickClickAction, "/user/hand/right/input/thumbstick/click" },
    { menuButtonAction, "/user/hand/left/input/menu/click" },
    { menuButtonAction, "/user/hand/right/input/menu/click" },
    { hapticAction, "/user/hand/left/output/haptic" },
    { hapticAction, "/user/hand/right/output/haptic" }
  });

  // セッションにアクションセットをアタッチする (これ以降はバインディングを追加できない)
  XrSessionActionSetsAttachInfo attachInfo{ XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
  attachInfo.countActionSets = 1;
  attachInfo.actionSets = &actionSet;
  xrCheck(instance, xrAttachSessionActionSets(session, &attachInfo),
    "Can't attach the OpenXR action set to the session");

  // アクションスペースの作成
  for (int i = 0; i < Hand::Count; ++i)
  {
    XrActionSpaceCreateInfo spaceInfo{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
    spaceInfo.poseInActionSpace.orientation.w = 1.0f;
    spaceInfo.subactionPath = handSubactionPath[i];

    spaceInfo.action = aimPoseAction;
    xrCheck(instance, xrCreateActionSpace(session, &spaceInfo, &aimSpace[i]),
      "Can't create the OpenXR action space for the aim pose");

    spaceInfo.action = gripPoseAction;
    xrCheck(instance, xrCreateActionSpace(session, &spaceInfo, &gripSpace[i]),
      "Can't create the OpenXR action space for the grip pose");
  }
}

//
// アクション状態を更新する
//
void GgApp::OpenXR::pollActions()
{
  // 入力を受け付けていなければコントローラーの状態を無効にする
  if (!isSessionRunning || actionSet == XR_NULL_HANDLE || sessionState != XR_SESSION_STATE_FOCUSED)
  {
    for (auto& state : controllerStates) state = ControllerState{};
    return;
  }

  XrActiveActionSet activeActionSet{ actionSet, XR_NULL_PATH };
  XrActionsSyncInfo syncInfo{ XR_TYPE_ACTIONS_SYNC_INFO };
  syncInfo.countActiveActionSets = 1;
  syncInfo.activeActionSets = &activeActionSet;
  if (!xrWarn(instance, xrSyncActions(session, &syncInfo),
    "Can't synchronize the OpenXR actions")) return;

  // 空間の姿勢を取り出すヘルパー (位置と向きの両方が有効なときだけ更新する)
  auto locate = [this](XrSpace space, XrPosef& pose)
  {
    if (space == XR_NULL_HANDLE) return false;

    XrSpaceLocation location{ XR_TYPE_SPACE_LOCATION };
    if (XR_FAILED(xrLocateSpace(space, appSpace, frameState.predictedDisplayTime, &location)))
      return false;

    constexpr XrSpaceLocationFlags valid
    {
      XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT
    };
    if ((location.locationFlags & valid) != valid) return false;

    pose = location.pose;
    return true;
  };

  for (int i = 0; i < Hand::Count; ++i)
  {
    auto& state = controllerStates[i];
    const XrPath subaction = handSubactionPath[i];

    XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
    getInfo.subactionPath = subaction;

    // グリップポーズの取得
    XrActionStatePose gripPoseState{ XR_TYPE_ACTION_STATE_POSE };
    getInfo.action = gripPoseAction;
    const bool gripActive
    {
      XR_SUCCEEDED(xrGetActionStatePose(session, &getInfo, &gripPoseState))
        && gripPoseState.isActive != XR_FALSE
    };

    // エイムポーズの取得
    XrActionStatePose aimPoseState{ XR_TYPE_ACTION_STATE_POSE };
    getInfo.action = aimPoseAction;
    const bool aimActive
    {
      XR_SUCCEEDED(xrGetActionStatePose(session, &getInfo, &aimPoseState))
        && aimPoseState.isActive != XR_FALSE
    };

    // どちらかの姿勢が有効ならコントローラーが接続されている
    state.isTracked = gripActive || aimActive;
    if (gripActive) locate(gripSpace[i], state.gripPose);
    if (aimActive) locate(aimSpace[i], state.aimPose);

    // 連続値のアクションを取り出すヘルパー
    auto getFloat = [this, &getInfo](XrAction action, float& value)
    {
      getInfo.action = action;
      XrActionStateFloat floatState{ XR_TYPE_ACTION_STATE_FLOAT };
      value = XR_SUCCEEDED(xrGetActionStateFloat(session, &getInfo, &floatState))
        && floatState.isActive != XR_FALSE ? floatState.currentState : 0.0f;
    };

    // 論理値のアクションを取り出すヘルパー
    auto getBoolean = [this, &getInfo](XrAction action, bool& value)
    {
      getInfo.action = action;
      XrActionStateBoolean booleanState{ XR_TYPE_ACTION_STATE_BOOLEAN };
      value = XR_SUCCEEDED(xrGetActionStateBoolean(session, &getInfo, &booleanState))
        && booleanState.isActive != XR_FALSE && booleanState.currentState != XR_FALSE;
    };

    // トリガーとグリップ
    getFloat(triggerAction, state.trigger);
    getFloat(gripAction, state.grip);

    // スティック
    getInfo.action = thumbstickAction;
    XrActionStateVector2f thumbstickState{ XR_TYPE_ACTION_STATE_VECTOR2F };
    if (XR_SUCCEEDED(xrGetActionStateVector2f(session, &getInfo, &thumbstickState))
      && thumbstickState.isActive != XR_FALSE)
      state.thumbstick = { thumbstickState.currentState.x, thumbstickState.currentState.y };
    else
      state.thumbstick = { 0.0f, 0.0f };

    // ボタン
    getBoolean(thumbstickClickAction, state.thumbstickClick);
    getBoolean(primaryButtonAction, state.primaryButton);
    getBoolean(secondaryButtonAction, state.secondaryButton);
    getBoolean(menuButtonAction, state.menuButton);
  }
}

//
// OpenXR のイベントを処理する
//
void GgApp::OpenXR::pollEvents()
{
  XrEventDataBuffer eventData{ XR_TYPE_EVENT_DATA_BUFFER };

  while (xrPollEvent(instance, &eventData) == XR_SUCCESS)
  {
    switch (eventData.type)
    {
    case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:

      // OpenXR のインスタンスが失われるのでアプリケーションを終了する
      isSessionRunning = false;
      if (window) window->setClose(GLFW_TRUE);
      break;

    case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
    {
      const auto* stateChanged{ reinterpret_cast<const XrEventDataSessionStateChanged*>(&eventData) };
      sessionState = stateChanged->state;

      switch (sessionState)
      {
      case XR_SESSION_STATE_READY:
      {
        // セッションを開始する
        XrSessionBeginInfo beginInfo{ XR_TYPE_SESSION_BEGIN_INFO };
        beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        if (xrWarn(instance, xrBeginSession(session, &beginInfo),
          "Can't begin the OpenXR session")) isSessionRunning = true;
        break;
      }

      case XR_SESSION_STATE_STOPPING:

        // セッションを終了する
        isSessionRunning = false;
        frameBegun = false;
        xrWarn(instance, xrEndSession(session), "Can't end the OpenXR session");
        break;

      case XR_SESSION_STATE_EXITING:
      case XR_SESSION_STATE_LOSS_PENDING:

        // アプリケーションを終了する
        isSessionRunning = false;
        if (window) window->setClose(GLFW_TRUE);
        break;

      default:
        break;
      }
      break;
    }

    default:
      break;
    }

    // 次のイベントを取り出す準備をする
    eventData = XrEventDataBuffer{ XR_TYPE_EVENT_DATA_BUFFER };
  }
}

//
// スワップチェーンを作成する
//
void GgApp::OpenXR::createSwapchains()
{
  // ビュー構成を取得する
  uint32_t viewCount{ 0 };
  xrCheck(instance, xrEnumerateViewConfigurationViews(instance, systemId,
    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr),
    "Can't count the OpenXR view configuration views");
  views.assign(viewCount, XrViewConfigurationView{ XR_TYPE_VIEW_CONFIGURATION_VIEW });
  xrCheck(instance, xrEnumerateViewConfigurationViews(instance, systemId,
    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewCount, &viewCount, views.data()),
    "Can't enumerate the OpenXR view configuration views");
  if (viewCount == 0) throw std::runtime_error("The OpenXR system has no view");

  viewStates.assign(viewCount, XrView{ XR_TYPE_VIEW });
  currentImageIndex.assign(viewCount, 0);
  imageAcquired.assign(viewCount, false);

  // 環境の合成方法を取得する (最初のものが最も推奨される)
  uint32_t blendModeCount{ 0 };
  if (XR_SUCCEEDED(xrEnumerateEnvironmentBlendModes(instance, systemId,
    XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &blendModeCount, nullptr))
    && blendModeCount > 0)
  {
    std::vector<XrEnvironmentBlendMode> blendModes(blendModeCount);
    if (XR_SUCCEEDED(xrEnumerateEnvironmentBlendModes(instance, systemId,
      XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, blendModeCount,
      &blendModeCount, blendModes.data())))
    {
      blendMode = blendModes[0];
    }
  }

  // 利用可能なスワップチェーンのカラーフォーマットを取得する
  uint32_t formatCount{ 0 };
  xrCheck(instance, xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr),
    "Can't count the OpenXR swapchain formats");
  std::vector<int64_t> formats(formatCount);
  xrCheck(instance, xrEnumerateSwapchainFormats(session, formatCount, &formatCount, formats.data()),
    "Can't enumerate the OpenXR swapchain formats");
  if (formats.empty()) throw std::runtime_error("The OpenXR runtime has no swapchain format");

  // 使用したいカラーフォーマットの候補 (前にあるものを優先する)
  static const int64_t preferred[]{ GL_SRGB8_ALPHA8, GL_SRGB8, GL_RGBA8, GL_RGB10_A2 };

  // 利用可能なカラーフォーマットの中から使用するものを選ぶ
  int64_t format{ formats[0] };
  for (const auto candidate : preferred)
  {
    if (std::find(formats.begin(), formats.end(), candidate) != formats.end())
    {
      format = candidate;
      break;
    }
  }

  // sRGB のフォーマットならリニア色空間で描画してガンマ補正をランタイムに任せる
  swapchainIsSrgb = format == GL_SRGB8_ALPHA8 || format == GL_SRGB8;

  // ビューの数だけ FBO とデプスバッファを作成する
  openxrFbo.assign(viewCount, 0);
  openxrDepth.assign(viewCount, 0);
  glGenFramebuffers(static_cast<GLsizei>(viewCount), openxrFbo.data());
  glGenRenderbuffers(static_cast<GLsizei>(viewCount), openxrDepth.data());

  for (uint32_t i = 0; i < viewCount; ++i)
  {
    // スワップチェーンを作成する
    XrSwapchainCreateInfo swapchainCreateInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
    swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT
      | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
    swapchainCreateInfo.format = format;
    swapchainCreateInfo.sampleCount = 1;
    swapchainCreateInfo.width = views[i].recommendedImageRectWidth;
    swapchainCreateInfo.height = views[i].recommendedImageRectHeight;
    swapchainCreateInfo.faceCount = 1;
    swapchainCreateInfo.arraySize = 1;
    swapchainCreateInfo.mipCount = 1;

    XrSwapchain swapchain{ XR_NULL_HANDLE };
    xrCheck(instance, xrCreateSwapchain(session, &swapchainCreateInfo, &swapchain),
      "Can't create the OpenXR swapchain");
    swapchains.push_back(swapchain);

    // スワップチェーンのイメージを取得する
    uint32_t imageCount{ 0 };
    xrCheck(instance, xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr),
      "Can't count the OpenXR swapchain images");
    std::vector<XrSwapchainImageOpenGLKHR> images(imageCount,
      XrSwapchainImageOpenGLKHR{ XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });
    xrCheck(instance, xrEnumerateSwapchainImages(swapchain, imageCount, &imageCount,
      reinterpret_cast<XrSwapchainImageBaseHeader*>(images.data())),
      "Can't enumerate the OpenXR swapchain images");
    swapchainImages.push_back(std::move(images));

    // 隠面消去処理に使うデプスバッファを作成する
    glBindRenderbuffer(GL_RENDERBUFFER, openxrDepth[i]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
      static_cast<GLsizei>(swapchainCreateInfo.width),
      static_cast<GLsizei>(swapchainCreateInfo.height));
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // FBO にデプスバッファを取り付けておく (カラーバッファは select() で取り付ける)
    glBindFramebuffer(GL_FRAMEBUFFER, openxrFbo[i]);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
      GL_RENDERBUFFER, openxrDepth[i]);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//
// OpenXR の static object を取得する
//
GgApp::OpenXR& GgApp::OpenXR::getInstance()
{
  // 最初に呼び出されたときに作成されるシングルトン
  static OpenXR openxr;
  return openxr;
}

//
// OpenXR のセッションを作成する
//
GgApp::OpenXR& GgApp::OpenXR::initialize(const Window& window,
  XrReferenceSpaceType spaceType, const char* appName)
{
  auto& openxr = getInstance();

  // 初期化済みならそのまま返す
  if (openxr.initialized) return openxr;

  // 初期化に失敗していた場合に備えて後始末をしておく
  openxr.terminate();

  openxr.window = &window;
  openxr.referenceSpaceType = spaceType;

  // 最初に取得した頭部中心位置をシーン座標の原点にする
  openxr.xrOriginValid = false;

  try
  {
    // 利用可能な拡張機能を調べる
    uint32_t extensionCount{ 0 };
    xrCheck(XR_NULL_HANDLE,
      xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr),
      "Can't count the OpenXR instance extensions");
    std::vector<XrExtensionProperties> extensionProperties(extensionCount,
      XrExtensionProperties{ XR_TYPE_EXTENSION_PROPERTIES });
    xrCheck(XR_NULL_HANDLE,
      xrEnumerateInstanceExtensionProperties(nullptr, extensionCount,
        &extensionCount, extensionProperties.data()),
      "Can't enumerate the OpenXR instance extensions");

    // OpenGL との連携に必要な拡張機能が使えなければあきらめる
    const auto found{ std::any_of(extensionProperties.begin(), extensionProperties.end(),
      [](const XrExtensionProperties& p)
      {
        return std::strcmp(p.extensionName, XR_KHR_OPENGL_ENABLE_EXTENSION_NAME) == 0;
      }) };
    if (!found)
    {
      throw std::runtime_error(
        "The OpenXR runtime does not support " XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
    }

    // ハンドトラッキング拡張が利用可能か確認
    openxr.xrHandTrackingSupported = std::any_of(extensionProperties.begin(), extensionProperties.end(),
      [](const XrExtensionProperties& p)
      {
        return std::strcmp(p.extensionName, XR_EXT_HAND_TRACKING_EXTENSION_NAME) == 0;
      });

    // XrInstance の作成
    XrInstanceCreateInfo createInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
    xrCopyString(createInfo.applicationInfo.applicationName,
      sizeof createInfo.applicationInfo.applicationName, appName ? appName : "TED");
    createInfo.applicationInfo.applicationVersion = 1;
    xrCopyString(createInfo.applicationInfo.engineName,
      sizeof createInfo.applicationInfo.engineName, "TED");
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    std::vector<const char*> enabledExtensions{ XR_KHR_OPENGL_ENABLE_EXTENSION_NAME };
    if (openxr.xrHandTrackingSupported)
    {
      enabledExtensions.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
    }
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.enabledExtensionNames = enabledExtensions.data();

    xrCheck(XR_NULL_HANDLE, xrCreateInstance(&createInfo, &openxr.instance),
      "Can't create the OpenXR instance (is an OpenXR runtime installed and active?)");

    if (openxr.xrHandTrackingSupported)
    {
      xrGetInstanceProcAddr(openxr.instance, "xrCreateHandTrackerEXT",
        reinterpret_cast<PFN_xrVoidFunction*>(&openxr.xrCreateHandTracker));
      xrGetInstanceProcAddr(openxr.instance, "xrDestroyHandTrackerEXT",
        reinterpret_cast<PFN_xrVoidFunction*>(&openxr.xrDestroyHandTracker));
      xrGetInstanceProcAddr(openxr.instance, "xrLocateHandJointsEXT",
        reinterpret_cast<PFN_xrVoidFunction*>(&openxr.xrLocateHandJoints));
    }

    // システムの取得
    XrSystemGetInfo systemInfo{ XR_TYPE_SYSTEM_GET_INFO };
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    xrCheck(openxr.instance, xrGetSystem(openxr.instance, &systemInfo, &openxr.systemId),
      "Can't get the OpenXR system (is the head mounted display connected?)");

    // システムの名前の取得
    XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES };
    if (XR_SUCCEEDED(xrGetSystemProperties(openxr.instance, openxr.systemId, &systemProperties)))
    {
      openxr.systemName = systemProperties.systemName;
    }

    // OpenGL との連携に必要な拡張機能の関数の取得
    PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR{ nullptr };
    xrCheck(openxr.instance, xrGetInstanceProcAddr(openxr.instance,
      "xrGetOpenGLGraphicsRequirementsKHR",
      reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLGraphicsRequirementsKHR)),
      "Can't get the address of xrGetOpenGLGraphicsRequirementsKHR");
    if (!pfnGetOpenGLGraphicsRequirementsKHR)
      throw std::runtime_error("Can't get the address of xrGetOpenGLGraphicsRequirementsKHR");

    // OpenGL の要件の取得 (セッションの作成前に必ず呼ばなければならない)
    XrGraphicsRequirementsOpenGLKHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
    xrCheck(openxr.instance, pfnGetOpenGLGraphicsRequirementsKHR(openxr.instance,
      openxr.systemId, &graphicsRequirements),
      "Can't get the OpenGL graphics requirements");

    // OpenGL のバージョンが要件を満たしているかどうか調べる
    GLint major{ 0 }, minor{ 0 };
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    if (XR_MAKE_VERSION(major, minor, 0) < graphicsRequirements.minApiVersionSupported)
    {
      char message[128];
      std::snprintf(message, sizeof message,
        "The OpenXR runtime requires OpenGL %d.%d or later, but %d.%d is current",
        static_cast<int>(XR_VERSION_MAJOR(graphicsRequirements.minApiVersionSupported)),
        static_cast<int>(XR_VERSION_MINOR(graphicsRequirements.minApiVersionSupported)),
        major, minor);
      throw std::runtime_error(message);
    }

    // OpenGL のコンテキストをセッションに結びつける
#if defined(XR_USE_PLATFORM_WIN32)
    XrGraphicsBindingOpenGLWin32KHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
    graphicsBinding.hDC = wglGetCurrentDC();
    graphicsBinding.hGLRC = glfwGetWGLContext(window.get());
#elif defined(XR_USE_PLATFORM_XLIB)
    XrGraphicsBindingOpenGLXlibKHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR };
    graphicsBinding.xDisplay = glfwGetX11Display();
    graphicsBinding.visualid = 0;
    graphicsBinding.glxFBConfig = nullptr;
    graphicsBinding.glxDrawable = glfwGetGLXWindow(window.get());
    graphicsBinding.glxContext = glfwGetGLXContext(window.get());
#else
#  error "GG_USE_OPENXR is not supported on this platform"
#endif

    // セッションの作成
    XrSessionCreateInfo sessionCreateInfo{ XR_TYPE_SESSION_CREATE_INFO };
    sessionCreateInfo.next = &graphicsBinding;
    sessionCreateInfo.systemId = openxr.systemId;
    xrCheck(openxr.instance, xrCreateSession(openxr.instance, &sessionCreateInfo, &openxr.session),
      "Can't create the OpenXR session");

    // 参照空間の作成 (要求されたものが使えなければ LOCAL にフォールバックする)
    XrReferenceSpaceCreateInfo spaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    spaceCreateInfo.referenceSpaceType = openxr.referenceSpaceType;
    spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
    if (XR_FAILED(xrCreateReferenceSpace(openxr.session, &spaceCreateInfo, &openxr.appSpace)))
    {
      openxr.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
      spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
      xrCheck(openxr.instance,
        xrCreateReferenceSpace(openxr.session, &spaceCreateInfo, &openxr.appSpace),
        "Can't create the OpenXR reference space");
    }

    // アクションシステムの初期化
    openxr.initActions();

    // ハンドトラッカーを作成する
    if (openxr.xrHandTrackingSupported && openxr.xrCreateHandTracker)
    {
      for (int hand = 0; hand < 2; ++hand)
      {
        XrHandTrackerCreateInfoEXT htCreateInfo{ XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
        htCreateInfo.hand = (hand == 0) ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT;
        htCreateInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
        if (XR_FAILED(openxr.xrCreateHandTracker(openxr.session, &htCreateInfo, &openxr.xrHandTracker[hand])))
        {
          openxr.xrHandTracker[hand] = XR_NULL_HANDLE;
        }
      }
    }

    // 拡張があってもハンドトラッカーを作成できなければハンドトラッキングは使えない
    openxr.xrHandTrackingSupported = openxr.xrHandTracker[Hand::Left] != XR_NULL_HANDLE
      || openxr.xrHandTracker[Hand::Right] != XR_NULL_HANDLE;

    // スワップチェーンの作成
    openxr.createSwapchains();
  }
  catch (...)
  {
    // 途中まで確保した資源を解放してから例外を投げ直す
    openxr.terminate();
    throw;
  }

  // OpenXR は xrWaitFrame() でフレームの表示速度を制御するので
  // ウィンドウ側の垂直同期の待ち合わせは行わない
  glfwSwapInterval(0);

  openxr.initialized = true;

  return openxr;
}

//
// OpenXR のハンドルを破棄する (OpenGL の資源には触れない)
//
void GgApp::OpenXR::destroyXr()
{
  for (int i = 0; i < Hand::Count; ++i)
  {
    if (aimSpace[i] != XR_NULL_HANDLE) { xrDestroySpace(aimSpace[i]); aimSpace[i] = XR_NULL_HANDLE; }
    if (gripSpace[i] != XR_NULL_HANDLE) { xrDestroySpace(gripSpace[i]); gripSpace[i] = XR_NULL_HANDLE; }
  }

  // アクションはアクションセットと一緒に破棄される
  if (actionSet != XR_NULL_HANDLE) xrDestroyActionSet(actionSet);
  actionSet = XR_NULL_HANDLE;
  aimPoseAction = gripPoseAction = XR_NULL_HANDLE;
  triggerAction = gripAction = XR_NULL_HANDLE;
  thumbstickAction = thumbstickClickAction = XR_NULL_HANDLE;
  primaryButtonAction = secondaryButtonAction = menuButtonAction = XR_NULL_HANDLE;
  hapticAction = XR_NULL_HANDLE;

  for (auto swapchain : swapchains) xrDestroySwapchain(swapchain);
  swapchains.clear();
  swapchainImages.clear();

  if (appSpace != XR_NULL_HANDLE) { xrDestroySpace(appSpace); appSpace = XR_NULL_HANDLE; }
  if (session != XR_NULL_HANDLE) { xrDestroySession(session); session = XR_NULL_HANDLE; }
  if (instance != XR_NULL_HANDLE) { xrDestroyInstance(instance); instance = XR_NULL_HANDLE; }

  systemId = XR_NULL_SYSTEM_ID;
  sessionState = XR_SESSION_STATE_UNKNOWN;
  isSessionRunning = false;
  frameBegun = false;
  viewPoseValid = false;
  initialized = false;
}

//
// OpenXR のセッションを破棄する
//
void GgApp::OpenXR::terminate()
{
  // 取得中のスワップチェーンイメージがあれば解放する (xrEndFrame() より前に行う)
  for (size_t i = 0; i < imageAcquired.size(); ++i)
  {
    if (!imageAcquired[i]) continue;
    XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
    xrReleaseSwapchainImage(swapchains[i], &releaseInfo);
    imageAcquired[i] = false;
  }

  // 描画中のフレームがあれば完了しておく
  if (frameBegun) endFrame();

  // OpenGL の資源を解放する
  if (!openxrFbo.empty())
  {
    glDeleteFramebuffers(static_cast<GLsizei>(openxrFbo.size()), openxrFbo.data());
    openxrFbo.clear();
  }
  if (!openxrDepth.empty())
  {
    glDeleteRenderbuffers(static_cast<GLsizei>(openxrDepth.size()), openxrDepth.data());
    openxrDepth.clear();
  }

  // ハンドトラッカーを破棄する
  if (xrDestroyHandTracker)
  {
    for (auto& ht : xrHandTracker)
    {
      if (ht != XR_NULL_HANDLE)
      {
        xrDestroyHandTracker(ht);
        ht = XR_NULL_HANDLE;
      }
    }
  }

  // OpenXR のハンドルを破棄する
  destroyXr();

  views.clear();
  viewStates.clear();
  currentImageIndex.clear();
  imageAcquired.clear();
  systemName.clear();

  for (auto& state : controllerStates) state = ControllerState{};

  // ウィンドウ側の設定を元に戻す
  if (window)
  {
    glDisable(GL_FRAMEBUFFER_SRGB);
    glfwSwapInterval(1);
    window = nullptr;
  }
}

//
// OpenXR による描画開始
//
bool GgApp::OpenXR::begin()
{
  // 初期化されていなければ何もしない
  if (instance == XR_NULL_HANDLE) return false;

  // OpenXR のイベントを処理する
  pollEvents();

  // セッションが実行中でなければ描画しない
  if (!isSessionRunning) return false;

  // 合成器がこのフレームの描画を始めるべき時刻まで待つ
  XrFrameWaitInfo waitInfo{ XR_TYPE_FRAME_WAIT_INFO };
  frameState = XrFrameState{ XR_TYPE_FRAME_STATE };
  if (!xrWarn(instance, xrWaitFrame(session, &waitInfo, &frameState),
    "Can't wait for the OpenXR frame")) return false;

  // フレームの描画を開始する
  XrFrameBeginInfo beginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
  if (!xrWarn(instance, xrBeginFrame(session, &beginInfo),
    "Can't begin the OpenXR frame")) return false;

  // ここから先は必ず xrEndFrame() を呼ばなければならない
  frameBegun = true;
  viewPoseValid = false;

  // コントローラーの状態を更新する
  pollActions();

  // 描画すべきフレームなら視点の姿勢を取得する
  if (frameState.shouldRender != XR_FALSE)
  {
    XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
    viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    viewLocateInfo.displayTime = frameState.predictedDisplayTime;
    viewLocateInfo.space = appSpace;

    XrViewState viewState{ XR_TYPE_VIEW_STATE };
    uint32_t viewCount{ 0 };
    if (XR_SUCCEEDED(xrLocateViews(session, &viewLocateInfo, &viewState,
      static_cast<uint32_t>(viewStates.size()), &viewCount, viewStates.data())))
    {
      // 位置と向きの両方が有効なときだけ描画する
      constexpr XrViewStateFlags valid
      {
        XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT
      };
      viewPoseValid = (viewState.viewStateFlags & valid) == valid
        && viewCount == static_cast<uint32_t>(viewStates.size());
    }
  }

  // 描画するなら true を返す
  if (viewPoseValid)
  {
    // 起動時または回復時の頭部中心を、シーン座標の原点として保存する
    if (!xrOriginValid && viewStates.size() >= 2)
    {
      const auto& leftEye{ viewStates[0].pose.position };
      const auto& rightEye{ viewStates[1].pose.position };
      xrOriginPosition = GgVector
      {
        (leftEye.x + rightEye.x) * 0.5f,
        (leftEye.y + rightEye.y) * 0.5f,
        (leftEye.z + rightEye.z) * 0.5f,
        1.0f
      };
      xrOriginValid = true;
    }

    return true;
  }

  // 描画しないフレームでもここで xrEndFrame() を呼んで辻褄を合わせる
  endFrame();

  return false;
}

//
// 描画対象の目を指定してフレームバッファとビューポートを設定する
//
void GgApp::OpenXR::select(int eye)
{
  // 描画すべきフレームでなければ何もしない
  if (!frameBegun || !viewPoseValid) return;

  // 視点の番号が範囲を外れていたら何もしない
  assert(eye >= 0 && eye < static_cast<int>(swapchains.size()));
  if (eye < 0 || eye >= static_cast<int>(swapchains.size())) return;

  // 取得済みなら描画先を結合し直すだけにする
  if (imageAcquired[eye])
  {
    glBindFramebuffer(GL_FRAMEBUFFER, openxrFbo[eye]);
    glViewport(0, 0,
      static_cast<GLsizei>(views[eye].recommendedImageRectWidth),
      static_cast<GLsizei>(views[eye].recommendedImageRectHeight));
    if (swapchainIsSrgb) glEnable(GL_FRAMEBUFFER_SRGB);
    return;
  }

  // 描画可能なスワップチェーンイメージを取得する
  XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
  if (!xrWarn(instance, xrAcquireSwapchainImage(swapchains[eye], &acquireInfo,
    &currentImageIndex[eye]), "Can't acquire the OpenXR swapchain image")) return;

  // そのスワップチェーンイメージが描画可能になるのを待つ
  XrSwapchainImageWaitInfo waitInfo{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
  waitInfo.timeout = XR_INFINITE_DURATION;
  if (!xrWarn(instance, xrWaitSwapchainImage(swapchains[eye], &waitInfo),
    "Can't wait for the OpenXR swapchain image"))
  {
    XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
    xrReleaseSwapchainImage(swapchains[eye], &releaseInfo);
    return;
  }

  imageAcquired[eye] = true;

  // 描画先をこのスワップチェーンイメージに切り替える
  const GLuint texture{ swapchainImages[eye][currentImageIndex[eye]].image };
  glBindFramebuffer(GL_FRAMEBUFFER, openxrFbo[eye]);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

  // フレームバッファオブジェクトが完成しているか確かめる (最初の一度だけ報告する)
  static bool reported{ false };
  if (!reported && glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    reported = true;
    std::cerr << "OpenXR: The framebuffer object for the swapchain image is not complete\n";
  }

  glViewport(0, 0,
    static_cast<GLsizei>(views[eye].recommendedImageRectWidth),
    static_cast<GLsizei>(views[eye].recommendedImageRectHeight));

  // sRGB のスワップチェーンならリニア色空間で描画する
  if (swapchainIsSrgb) glEnable(GL_FRAMEBUFFER_SRGB);
}

//
// 描画対象の目を指定する (旧 LibOVR 仕様互換)
//
void GgApp::OpenXR::select(int eye, GLfloat* screen, GLfloat* position, GLfloat* orientation)
{
  select(eye);

  assert(eye >= 0 && eye < static_cast<int>(viewStates.size()));
  const auto& pose = viewStates[eye].pose;
  const auto& fov = viewStates[eye].fov;

  screen[0] = tanf(fov.angleLeft);
  screen[1] = tanf(fov.angleRight);
  screen[2] = tanf(fov.angleDown);
  screen[3] = tanf(fov.angleUp);

  position[0] = pose.position.x;
  position[1] = pose.position.y;
  position[2] = pose.position.z;

  orientation[0] = pose.orientation.x;
  orientation[1] = pose.orientation.y;
  orientation[2] = pose.orientation.z;
  orientation[3] = pose.orientation.w;
}

//
// 指定した目の描画を完了する
//
void GgApp::OpenXR::commit(int eye)
{
  if (!frameBegun) return;
  assert(eye >= 0 && eye < static_cast<int>(swapchains.size()));
  if (eye < 0 || eye >= static_cast<int>(swapchains.size())) return;
  if (!imageAcquired[eye]) return;

  // ガンマ補正を元に戻して描画先をウィンドウに戻す
  if (swapchainIsSrgb) glDisable(GL_FRAMEBUFFER_SRGB);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // スワップチェーンイメージはミラー表示に使うので, ここでは解放しない
  // (解放は submit() の中でミラー表示を行った後に実施する)
}

//
// ミラー表示を行う
//
void GgApp::OpenXR::blitMirror() const
{
  // ミラー表示を行わないなら何もしない
  if (mirrorView < 0 || !window) return;
  const auto eye{ static_cast<size_t>(mirrorView) };
  if (eye >= swapchains.size() || !imageAcquired[eye]) return;

  // 転送元の大きさ
  const auto srcWidth{ static_cast<GLint>(views[eye].recommendedImageRectWidth) };
  const auto srcHeight{ static_cast<GLint>(views[eye].recommendedImageRectHeight) };
  if (srcWidth <= 0 || srcHeight <= 0) return;

  // 転送先 (ウィンドウ) の大きさ
  const auto& fboSize{ window->getFboSize() };
  if (fboSize[0] <= 0 || fboSize[1] <= 0) return;

  // 縦横比を保ったままウィンドウに収まる転送先の矩形を求める
  const auto scale{ std::min(
    static_cast<float>(fboSize[0]) / static_cast<float>(srcWidth),
    static_cast<float>(fboSize[1]) / static_cast<float>(srcHeight)) };
  const auto dstWidth{ static_cast<GLint>(static_cast<float>(srcWidth) * scale) };
  const auto dstHeight{ static_cast<GLint>(static_cast<float>(srcHeight) * scale) };
  const auto dstLeft{ (static_cast<GLint>(fboSize[0]) - dstWidth) / 2 };
  const auto dstBottom{ (static_cast<GLint>(fboSize[1]) - dstHeight) / 2 };

  // sRGB の再変換を避けるためにガンマ補正を無効にする
  if (swapchainIsSrgb) glDisable(GL_FRAMEBUFFER_SRGB);

  // 上下左右の余白を黒で塗りつぶす (消去色は元に戻す)
  GLfloat clearColor[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glDisable(GL_SCISSOR_TEST);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);

  // スワップチェーンイメージをウィンドウに転送する
  glBindFramebuffer(GL_READ_FRAMEBUFFER, openxrFbo[eye]);
  glBlitFramebuffer(0, 0, srcWidth, srcHeight,
    dstLeft, dstBottom, dstLeft + dstWidth, dstBottom + dstHeight,
    GL_COLOR_BUFFER_BIT, GL_LINEAR);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

//
// 描画中のフレームを合成器に転送する
//
void GgApp::OpenXR::endFrame()
{
  if (!frameBegun) return;

  // 合成する層
  std::vector<XrCompositionLayerProjectionView> projectionViews;
  XrCompositionLayerProjection layer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
  const XrCompositionLayerBaseHeader* layers[1]{ nullptr };

  // 描画したのなら層を用意する
  if (viewPoseValid && frameState.shouldRender != XR_FALSE)
  {
    projectionViews.resize(swapchains.size());
    for (size_t i = 0; i < swapchains.size(); ++i)
    {
      auto& projectionView{ projectionViews[i] };
      projectionView = XrCompositionLayerProjectionView{ XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
      projectionView.pose = viewStates[i].pose;
      projectionView.fov = viewStates[i].fov;
      projectionView.subImage.swapchain = swapchains[i];
      projectionView.subImage.imageRect.offset = { 0, 0 };
      projectionView.subImage.imageRect.extent = {
        static_cast<int32_t>(views[i].recommendedImageRectWidth),
        static_cast<int32_t>(views[i].recommendedImageRectHeight)
      };
      projectionView.subImage.imageArrayIndex = 0;
    }

    // 環境の合成方法に応じて層の属性を設定する
    layer.layerFlags = blendMode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE ? 0
      : XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT
      | XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
    layer.space = appSpace;
    layer.viewCount = static_cast<uint32_t>(projectionViews.size());
    layer.views = projectionViews.data();
    layers[0] = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer);
  }

  // フレームを合成器に転送する
  XrFrameEndInfo endInfo{ XR_TYPE_FRAME_END_INFO };
  endInfo.displayTime = frameState.predictedDisplayTime;
  endInfo.environmentBlendMode = blendMode;
  endInfo.layerCount = layers[0] ? 1u : 0u;
  endInfo.layers = layers;
  xrWarn(instance, xrEndFrame(session, &endInfo), "Can't end the OpenXR frame");

  frameBegun = false;
}

//
// フレームを転送して HMD に表示する
//
bool GgApp::OpenXR::submit(bool mirror)
{
  // 描画中のフレームがなければ何もしない
  if (!frameBegun) return false;

  // ミラー表示の有無を設定する
  if (!mirror) mirrorView = -1;
  else if (mirrorView < 0) mirrorView = 0;

  // スワップチェーンイメージを解放する前にミラー表示を行う
  blitMirror();

  // ウィンドウのビューポートを復帰して Dear ImGui などの描画に備える
  if (window) window->restoreViewport();

  // 取得したスワップチェーンイメージを解放する
  for (size_t i = 0; i < imageAcquired.size(); ++i)
  {
    if (!imageAcquired[i]) continue;
    XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
    xrWarn(instance, xrReleaseSwapchainImage(swapchains[i], &releaseInfo),
      "Can't release the OpenXR swapchain image");
    imageAcquired[i] = false;
  }

  // 合成器にフレームを転送する
  endFrame();

  return true;
}

//
// ミラー表示を行うビューの番号を設定する
//
void GgApp::OpenXR::setMirror(int eye)
{
  mirrorView = eye;
}

//
// ミラー表示を行うビューの番号を取得する
//
int GgApp::OpenXR::getMirror() const
{
  return mirrorView;
}

//
// セッションが実行中かどうか調べる
//
bool GgApp::OpenXR::isRunning() const
{
  return isSessionRunning;
}

//
// アプリケーションが入力を受け付けているかどうか調べる
//
bool GgApp::OpenXR::isFocused() const
{
  return sessionState == XR_SESSION_STATE_FOCUSED;
}

//
// OpenXR のシステム (HMD) の名前を取得する
//
const std::string& GgApp::OpenXR::getSystemName() const
{
  return systemName;
}

//
// 指定した目の透視投影変換行列を取得する
//
GgMatrix GgApp::OpenXR::getProjectionMatrix(int eye, GLfloat zNear, GLfloat zFar) const
{
  assert(eye >= 0 && eye < static_cast<int>(viewStates.size()));
  const auto& fov{ viewStates[eye].fov };
  const GLfloat l{ tanf(fov.angleLeft) * zNear };
  const GLfloat r{ tanf(fov.angleRight) * zNear };
  const GLfloat b{ tanf(fov.angleDown) * zNear };
  const GLfloat t{ tanf(fov.angleUp) * zNear };
  return ggFrustum(l, r, b, t, zNear, zFar);
}

//
// 指定した目のビュー変換行列を取得する
//
GgMatrix GgApp::OpenXR::getViewMatrix(int eye) const
{
  assert(eye >= 0 && eye < static_cast<int>(viewStates.size()));
  const auto& pose{ viewStates[eye].pose };
  const GgQuaternion q{ pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w };
  return q.getConjugateMatrix() * ggTranslate(-pose.position.x, -pose.position.y, -pose.position.z);
}

//
// 指定した目の姿勢行列を取得する
//
GgMatrix GgApp::OpenXR::getPoseMatrix(int eye) const
{
  assert(eye >= 0 && eye < static_cast<int>(viewStates.size()));
  const auto& pose{ viewStates[eye].pose };
  const GgQuaternion q{ pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w };
  return ggTranslate(pose.position.x, pose.position.y, pose.position.z) * q.getMatrix();
}

//
// 指定した目の視点位置を取得する
//
GgVector GgApp::OpenXR::getPosition(int eye) const
{
  assert(eye >= 0 && eye < static_cast<int>(viewStates.size()));
  const auto& pos{ viewStates[eye].pose.position };
  return GgVector{ pos.x, pos.y, pos.z, 1.0f };
}

//
// 指定した目の視線方向の回転四元数を取得する
//
GgQuaternion GgApp::OpenXR::getOrientation(int eye) const
{
  assert(eye >= 0 && eye < static_cast<int>(viewStates.size()));
  const auto& ori{ viewStates[eye].pose.orientation };
  return GgQuaternion{ ori.x, ori.y, ori.z, ori.w };
}

//
// 指定した目の視野角情報 (XrFovf) を取得する
//
const XrFovf& GgApp::OpenXR::getFov(int eye) const
{
  assert(eye >= 0 && eye < static_cast<int>(viewStates.size()));
  return viewStates[eye].fov;
}

//
// 指定した目の姿勢情報 (XrPosef) を取得する
//
const XrPosef& GgApp::OpenXR::getPose(int eye) const
{
  assert(eye >= 0 && eye < static_cast<int>(viewStates.size()));
  return viewStates[eye].pose;
}

//
// 視点の姿勢が有効かどうか調べる
//
bool GgApp::OpenXR::isPoseValid() const
{
  return viewPoseValid;
}

//
// レンダリング推奨解像度の横幅を取得する
//
GLsizei GgApp::OpenXR::getWidth(int eye) const
{
  assert(eye >= 0 && eye < static_cast<int>(views.size()));
  return static_cast<GLsizei>(views[eye].recommendedImageRectWidth);
}

//
// レンダリング推奨解像度の高さを取得する
//
GLsizei GgApp::OpenXR::getHeight(int eye) const
{
  assert(eye >= 0 && eye < static_cast<int>(views.size()));
  return static_cast<GLsizei>(views[eye].recommendedImageRectHeight);
}

//
// アスペクト比を取得する
//
GLfloat GgApp::OpenXR::getAspect(int eye) const
{
  assert(eye >= 0 && eye < static_cast<int>(views.size()));
  return static_cast<GLfloat>(views[eye].recommendedImageRectWidth)
    / static_cast<GLfloat>(views[eye].recommendedImageRectHeight);
}

//
// ビューの総数を取得する
//
uint32_t GgApp::OpenXR::getViewCount() const
{
  return static_cast<uint32_t>(views.size());
}

//
// 現在の参照空間タイプを取得する
//
XrReferenceSpaceType GgApp::OpenXR::getReferenceSpaceType() const
{
  return referenceSpaceType;
}

//
// コントローラーがトラッキングされているか取得する
//
bool GgApp::OpenXR::isTracked(int hand) const
{
  assert(hand >= 0 && hand < Hand::Count);
  return controllerStates[hand].isTracked;
}

//
// コントローラーのグリップ変換行列を取得する
//
GgMatrix GgApp::OpenXR::getGripMatrix(int hand) const
{
  assert(hand >= 0 && hand < Hand::Count);
  const auto& pose = controllerStates[hand].gripPose;
  const GgQuaternion q{ pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w };
  return ggTranslate(pose.position.x, pose.position.y, pose.position.z) * q.getMatrix();
}

//
// コントローラーのエイム変換行列を取得する
//
GgMatrix GgApp::OpenXR::getAimMatrix(int hand) const
{
  assert(hand >= 0 && hand < Hand::Count);
  const auto& pose = controllerStates[hand].aimPose;
  const GgQuaternion q{ pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w };
  return ggTranslate(pose.position.x, pose.position.y, pose.position.z) * q.getMatrix();
}

//
// コントローラーのグリップ位置を取得する
//
GgVector GgApp::OpenXR::getGripPosition(int hand) const
{
  assert(hand >= 0 && hand < Hand::Count);
  const auto& pos = controllerStates[hand].gripPose.position;
  return GgVector{ pos.x, pos.y, pos.z, 1.0f };
}

//
// コントローラーのグリップ回転四元数を取得する
//
GgQuaternion GgApp::OpenXR::getGripOrientation(int hand) const
{
  assert(hand >= 0 && hand < Hand::Count);
  const auto& ori = controllerStates[hand].gripPose.orientation;
  return GgQuaternion{ ori.x, ori.y, ori.z, ori.w };
}

//
// コントローラーのエイム位置を取得する
//
GgVector GgApp::OpenXR::getAimPosition(int hand) const
{
  assert(hand >= 0 && hand < Hand::Count);
  const auto& pos = controllerStates[hand].aimPose.position;
  return GgVector{ pos.x, pos.y, pos.z, 1.0f };
}

//
// コントローラーのエイム回転四元数を取得する
//
GgQuaternion GgApp::OpenXR::getAimOrientation(int hand) const
{
  assert(hand >= 0 && hand < Hand::Count);
  const auto& ori = controllerStates[hand].aimPose.orientation;
  return GgQuaternion{ ori.x, ori.y, ori.z, ori.w };
}

//
// トリガーの押し込み量を取得する
//
float GgApp::OpenXR::getTrigger(int hand) const
{
  assert(hand >= 0 && hand < Hand::Count);
  return controllerStates[hand].trigger;
}

//
// グリップの押し込み量を取得する
//
float GgApp::OpenXR::getGrip(int hand) const
{
  assert(hand >= 0 && hand < Hand::Count);
  return controllerStates[hand].grip;
}

//
// アナログスティックの入力値を取得する
//
std::array<float, 2> GgApp::OpenXR::getThumbstick(int hand) const
{
  assert(hand >= 0 && hand < Hand::Count);
  return controllerStates[hand].thumbstick;
}

//
// アナログスティックのクリック状態を取得する
//
bool GgApp::OpenXR::getThumbstickClick(int hand) const
{
  assert(hand >= 0 && hand < Hand::Count);
  return controllerStates[hand].thumbstickClick;
}

//
// プライマリボタンの押下状態を取得する
//
bool GgApp::OpenXR::getPrimaryButton(int hand) const
{
  assert(hand >= 0 && hand < Hand::Count);
  return controllerStates[hand].primaryButton;
}

//
// セカンダリボタンの押下状態を取得する
//
bool GgApp::OpenXR::getSecondaryButton(int hand) const
{
  assert(hand >= 0 && hand < Hand::Count);
  return controllerStates[hand].secondaryButton;
}

//
// メニューボタンの押下状態を取得する
//
bool GgApp::OpenXR::getMenuButton(int hand) const
{
  assert(hand >= 0 && hand < Hand::Count);
  return controllerStates[hand].menuButton;
}

//
// コントローラーに振動を出力する
//
void GgApp::OpenXR::applyHapticVibration(int hand, float durationSeconds, float frequency, float amplitude)
{
  assert(hand >= 0 && hand < Hand::Count);
  if (session == XR_NULL_HANDLE || hapticAction == XR_NULL_HANDLE) return;

  XrHapticVibration vibration{ XR_TYPE_HAPTIC_VIBRATION };
  vibration.duration = durationSeconds > 0.0f
    ? static_cast<XrDuration>(static_cast<double>(durationSeconds) * 1.0e9)
    : XR_MIN_HAPTIC_DURATION;
  vibration.frequency = frequency;
  vibration.amplitude = std::min(std::max(amplitude, 0.0f), 1.0f);

  XrHapticActionInfo actionInfo{ XR_TYPE_HAPTIC_ACTION_INFO };
  actionInfo.action = hapticAction;
  actionInfo.subactionPath = handSubactionPath[hand];

  xrWarn(instance, xrApplyHapticFeedback(session, &actionInfo,
    reinterpret_cast<const XrHapticBaseHeader*>(&vibration)),
    "Can't apply the haptic feedback");
}

//
// Quest 等の OpenXR ランタイムから手の関節姿勢を取得し、Leap Motion 互換の 22 姿勢へ変換する
//
void GgApp::OpenXR::updateOpenXRHands(XrTime time)
{
  // ハンドトラッキングが使えないか、シーン座標の原点が決まっていなければ何もしない
  if (!xrHandTrackingSupported || !xrLocateHandJoints || appSpace == XR_NULL_HANDLE || !xrOriginValid) return;

  // Leap 側の各指 4 骨は、OpenXR では各骨の末端に当たる 4 関節へ対応させる。
  constexpr std::array<XrHandJointEXT, 22> jointMap
  {
    XR_HAND_JOINT_PALM_EXT, XR_HAND_JOINT_WRIST_EXT,
    XR_HAND_JOINT_THUMB_METACARPAL_EXT, XR_HAND_JOINT_THUMB_PROXIMAL_EXT,
    XR_HAND_JOINT_THUMB_DISTAL_EXT, XR_HAND_JOINT_THUMB_TIP_EXT,
    XR_HAND_JOINT_INDEX_PROXIMAL_EXT, XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT,
    XR_HAND_JOINT_INDEX_DISTAL_EXT, XR_HAND_JOINT_INDEX_TIP_EXT,
    XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT, XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT,
    XR_HAND_JOINT_MIDDLE_DISTAL_EXT, XR_HAND_JOINT_MIDDLE_TIP_EXT,
    XR_HAND_JOINT_RING_PROXIMAL_EXT, XR_HAND_JOINT_RING_INTERMEDIATE_EXT,
    XR_HAND_JOINT_RING_DISTAL_EXT, XR_HAND_JOINT_RING_TIP_EXT,
    XR_HAND_JOINT_LITTLE_PROXIMAL_EXT, XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT,
    XR_HAND_JOINT_LITTLE_DISTAL_EXT, XR_HAND_JOINT_LITTLE_TIP_EXT
  };

  // 各指 4 骨の始点。終点は jointMap の同じ要素を使う。
  constexpr std::array<XrHandJointEXT, 20> boneStartMap
  {
    XR_HAND_JOINT_WRIST_EXT, XR_HAND_JOINT_THUMB_METACARPAL_EXT,
    XR_HAND_JOINT_THUMB_PROXIMAL_EXT, XR_HAND_JOINT_THUMB_DISTAL_EXT,
    XR_HAND_JOINT_INDEX_METACARPAL_EXT, XR_HAND_JOINT_INDEX_PROXIMAL_EXT,
    XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT, XR_HAND_JOINT_INDEX_DISTAL_EXT,
    XR_HAND_JOINT_MIDDLE_METACARPAL_EXT, XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT,
    XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT, XR_HAND_JOINT_MIDDLE_DISTAL_EXT,
    XR_HAND_JOINT_RING_METACARPAL_EXT, XR_HAND_JOINT_RING_PROXIMAL_EXT,
    XR_HAND_JOINT_RING_INTERMEDIATE_EXT, XR_HAND_JOINT_RING_DISTAL_EXT,
    XR_HAND_JOINT_LITTLE_METACARPAL_EXT, XR_HAND_JOINT_LITTLE_PROXIMAL_EXT,
    XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT, XR_HAND_JOINT_LITTLE_DISTAL_EXT
  };

  // hand.json の親 controller 0 は左右眼の中点 (頭部中心) を使う。
  // 左眼を親にすると右眼との眼間距離が回転半径へ混入し、頭部回転時に
  // 視界固定の手が微小に並進して見えるため、関節も同じ頭部中心ローカルへ戻す。
  const GgMatrix worldToHandParent{ getHeadPoseMatrix().invert() };

  for (int hand = 0; hand < Hand::Count; ++hand)
  {
    if (xrHandTracker[hand] == XR_NULL_HANDLE) continue;

    // 全関節の位置と向きを取得する
    std::array<XrHandJointLocationEXT, XR_HAND_JOINT_COUNT_EXT> locations{};
    XrHandJointLocationsEXT joints{ XR_TYPE_HAND_JOINT_LOCATIONS_EXT };
    joints.jointCount = static_cast<uint32_t>(locations.size());
    joints.jointLocations = locations.data();
    XrHandJointsLocateInfoEXT locateInfo{ XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT };
    locateInfo.baseSpace = appSpace;
    locateInfo.time = time;
    if (XR_FAILED(xrLocateHandJoints(xrHandTracker[hand], &locateInfo, &joints)) || !joints.isActive)
    {
      // シーングラフの左右スロットは Leap Motion のモデル対応を基準にしているため、
      // OpenXR の XrHandEXT 順序だけを反転して格納する。
      Scene::setLocalHandAttitudes(1 - hand, nullptr);
      continue;
    }

    // すべての関節の位置が有効か調べる
    std::array<GgMatrix, jointMap.size()> matrices;
    bool valid{ true };
    constexpr XrSpaceLocationFlags requiredFlags{ XR_SPACE_LOCATION_POSITION_VALID_BIT };
    for (const auto& joint : locations)
    {
      if ((joint.locationFlags & requiredFlags) != requiredFlags)
      {
        valid = false;
        break;
      }
    }

    if (valid)
    {
      const auto& wrist{ locations[XR_HAND_JOINT_WRIST_EXT].pose.position };
      const auto& middle{ locations[XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT].pose.position };
      const auto& index{ locations[XR_HAND_JOINT_INDEX_METACARPAL_EXT].pose.position };
      const auto& little{ locations[XR_HAND_JOINT_LITTLE_METACARPAL_EXT].pose.position };

      // 左右で同じ解剖学的意味の横軸になるよう、右手ではランドマークの差分順を反転する。
      const GLfloat side{ hand == 0 ? 1.0f : -1.0f };
      GLfloat palmX[]{ side * (index.x - little.x), side * (index.y - little.y),
        side * (index.z - little.z) };
      GLfloat palmY[]{ middle.x - wrist.x, middle.y - wrist.y, middle.z - wrist.z };
      const auto lengthSquared = [](const GLfloat* v)
      {
        return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
      };
      constexpr GLfloat minimumAxisLengthSquared{ 1.0e-8f };
      if (lengthSquared(palmX) < minimumAxisLengthSquared
        || lengthSquared(palmY) < minimumAxisLengthSquared)
      {
        continue;
      }
      ggNormalize3(palmX);
      ggNormalize3(palmY);
      GLfloat palmZ[3];
      ggCross(palmZ, palmX, palmY);
      if (lengthSquared(palmZ) < minimumAxisLengthSquared) continue;
      ggNormalize3(palmZ);
      ggCross(palmY, palmZ, palmX);

      // 関節の位置と軸から、頭部中心を親とする座標系の変換行列を作る
      const auto makeMatrix = [this, &worldToHandParent](const XrVector3f& p, const GLfloat* x,
        const GLfloat* y, const GLfloat* z)
      {
        const GLfloat m[]
        {
          x[0], x[1], x[2], 0.0f,
          y[0], y[1], y[2], 0.0f,
          z[0], z[1], z[2], 0.0f,
          p.x - xrOriginPosition[0], p.y - xrOriginPosition[1],
          p.z - xrOriginPosition[2], 1.0f
        };
        return worldToHandParent * GgMatrix(m);
      };

      // 手のひらは、手首から中指および人差し指から小指への実測方向で作る。
      matrices[0] = makeMatrix(locations[XR_HAND_JOINT_PALM_EXT].pose.position,
        palmX, palmY, palmZ);

      // OpenXR には肘関節がないため、手首から手のひらへ向かう実測方向を
      // Leap の腕方向 (肘から手首) に相当する長手軸として使う。
      // 手のひら行列の固定回転を流用すると手首が手のひらへ完全に追従し、
      // finger.obj の長手軸も逆向きになるため、独立した正規直交基底を作る。
      const auto& palm{ locations[XR_HAND_JOINT_PALM_EXT].pose.position };
      GLfloat wristZ[]{ palm.x - wrist.x, palm.y - wrist.y, palm.z - wrist.z };
      if (lengthSquared(wristZ) < minimumAxisLengthSquared)
      {
        valid = false;
      }
      else
      {
        ggNormalize3(wristZ);
        const GLfloat wristDot{
          palmZ[0] * wristZ[0] + palmZ[1] * wristZ[1] + palmZ[2] * wristZ[2] };
        GLfloat wristY[]{ palmZ[0] - wristDot * wristZ[0],
          palmZ[1] - wristDot * wristZ[1], palmZ[2] - wristDot * wristZ[2] };
        if (lengthSquared(wristY) < minimumAxisLengthSquared)
        {
          valid = false;
        }
        else
        {
          ggNormalize3(wristY);
          GLfloat wristX[3];
          ggCross(wristX, wristY, wristZ);
          ggNormalize3(wristX);
          ggCross(wristY, wristZ, wristX);
          matrices[1] = makeMatrix(wrist, wristX, wristY, wristZ);
        }
      }

      // finger.obj の先端方向に合わせ、各ボーンの始点から終点への方向をローカル Z 軸にする。
      // Leap Motion と同じく、ボーンの姿勢はボーンの終点 (next_joint) に置く。
      for (size_t bone = 0; valid && bone < boneStartMap.size(); ++bone)
      {
        const auto& start{ locations[static_cast<size_t>(boneStartMap[bone])].pose.position };
        const auto& end{
          locations[static_cast<size_t>(jointMap[bone + 2])].pose.position };
        GLfloat z[]{ end.x - start.x, end.y - start.y, end.z - start.z };
        if (lengthSquared(z) < minimumAxisLengthSquared)
        {
          valid = false;
          break;
        }
        ggNormalize3(z);

        // 手のひら法線を基準にロールを決め、ボーン方向と直交する正規直交基底にする。
        const GLfloat dot{ palmZ[0] * z[0] + palmZ[1] * z[1] + palmZ[2] * z[2] };
        GLfloat y[]{ palmZ[0] - dot * z[0], palmZ[1] - dot * z[1],
          palmZ[2] - dot * z[2] };
        if (lengthSquared(y) < minimumAxisLengthSquared)
        {
          valid = false;
          break;
        }
        ggNormalize3(y);
        GLfloat x[3];
        ggCross(x, y, z);
        ggNormalize3(x);
        ggCross(y, z, x);
        matrices[bone + 2] = makeMatrix(end, x, y, z);
      }
    }

    if (valid)
    {
      Scene::setLocalHandAttitudes(1 - hand, matrices.data());
    }
    // 一時的な無効値や縮退した関節では、ちらつきを避けるため直前の姿勢を維持する。
  }
}

//
// 頭部中心 (左右の目の中点) の位置を取得する
//
GgVector GgApp::OpenXR::getHeadPosition() const
{
  // 左右の目の姿勢が得られていなければ原点を返す
  if (viewStates.size() < 2 || !viewPoseValid) return GgVector{ 0.0f, 0.0f, 0.0f, 1.0f };

  // シーン座標の原点を基準にした左右の目の中点
  const auto& leftEye{ viewStates[0].pose.position };
  const auto& rightEye{ viewStates[1].pose.position };
  return GgVector{
    (leftEye.x + rightEye.x) * 0.5f - xrOriginPosition[0],
    (leftEye.y + rightEye.y) * 0.5f - xrOriginPosition[1],
    (leftEye.z + rightEye.z) * 0.5f - xrOriginPosition[2],
    1.0f
  };
}

//
// 頭部の向きを取得する
//
GgQuaternion GgApp::OpenXR::getHeadOrientation() const
{
  // 視点の姿勢が得られていなければ回転しない
  if (viewStates.empty() || !viewPoseValid) return ggIdentityQuaternion();

  // 左右の目は同じ向きなので左目の向きを使う
  const auto& q{ viewStates[0].pose.orientation };
  return GgQuaternion{ q.x, q.y, q.z, q.w };
}

//
// 頭部中心姿勢の変換行列を取得する
//
GgMatrix GgApp::OpenXR::getHeadPoseMatrix() const
{
  // 頭部中心の位置に平行移動して頭部の向きに回転する
  return ggTranslate(getHeadPosition()) * getHeadOrientation().getMatrix();
}

#endif

#if defined(_WIN32)
#  if !defined(_INC_WINDOWS) && !defined(_WINDOWS_)
#    include <windows.h>
#  endif
#else
#  include <pwd.h>
#  include <unistd.h>
#endif
//
// ユーザ名を得る
//
std::string GgApp::getUsername()
{
  // 環境変数からユーザ名を得る
  const char* user{
#if defined(_WIN32)
    std::getenv("USERNAME")
#else
    std::getenv("USER")
#endif
  };

  // 環境変数からユーザ名が得られたらそれを返す
  if (user) return user;

#if defined(_WIN32)
  // Win32 API を使ってユーザ名を得る
  char username[256];
  DWORD size{ sizeof(username) };
  if (GetUserNameA(username, &size)) return std::string(username);
#else
  struct passwd* pw{ getpwuid(getuid()) };
  if (pw) return std::string(pw->pw_name);
#endif

  // ユーザ名が得られなかった
  return "unknown";
}

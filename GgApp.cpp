//
// ウィンドウ関連およびアプリケーションのメイン処理
//
#include "GgApp.h"
#pragma comment(lib, "opengl32.lib")

// 各種設定
#include "Config.h"

// 姿勢
#include "Attitude.h"

// シーングラフ
#include "Scene.h"

// 標準ライブラリ
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cassert>
#include <stdexcept>
#include <algorithm>

// Dear ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#if defined(GG_USE_OPENXR)
//
// OpenXR のエラーチェックマクロ
//
#define XR_CHECK(func) \
  do { \
    XrResult result = (func); \
    if (XR_FAILED(result)) { \
      char buf[256]; \
      sprintf_s(buf, "OpenXR error at %s:%d (Result: %d)", __FILE__, __LINE__, result); \
      NOTIFY(buf); \
      return false; \
    } \
  } while (0)

#define XR_CHECK_VOID(func) \
  do { \
    XrResult result = (func); \
    if (XR_FAILED(result)) { \
      char buf[256]; \
      sprintf_s(buf, "OpenXR error at %s:%d (Result: %d)", __FILE__, __LINE__, result); \
      NOTIFY(buf); \
      return; \
    } \
  } while (0)
#endif



//
// GgApp::Window コンストラクタ
//
GgApp::Window::Window(int width, int height, const char* title, GLFWmonitor* monitor, GLFWwindow* share)
{
  // 最初のインスタンスで一度だけ実行
  static bool firstTime{ true };
  if (firstTime)
  {
    // クワッドバッファステレオモードを有効にする
    if (defaults.display_quadbuffer) glfwWindowHint(GLFW_STEREO, GLFW_TRUE);

    // SRGB モードでレンダリングできるようにする
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    // OpenGL のバージョンを指定する
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Core Profile を選択する
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef IMGUI_VERSION
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

  // GLFW のウィンドウを開く
  const_cast<GLFWwindow*>(window) = glfwCreateWindow(width, height, title, monitor, share);

  // ウィンドウが開かれなかったら戻る
  if (!window) return;

  //
  // 初期設定
  //

  // ヘッドトラッキングの変換行列を初期化する
  for (auto& m : mo) m = ggIdentity();

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

#ifdef IMGUI_VERSION
  // Setup Platform/Renderer bindings
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

  // 投影変換行列・ビューポートを初期化する
  resize(window, width, height);
}

//
// デストラクタ
//
GgApp::Window::~Window()
{
  // ウィンドウが開かれていなかったら戻る
  if (!window) return;

#if defined(GG_USE_OPENXR)
  cleanupOpenXR();
#endif

#ifdef IMGUI_VERSION
  // Shutdown Platform/Renderer bindings
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
#endif

  // 表示用のウィンドウを破棄する
  glfwDestroyWindow(window);
}

//
// イベントを取得してループを継続するなら真を返す
//
GgApp::Window::operator bool()
{
  // イベントを取り出す
  glfwPollEvents();

#if defined(GG_USE_OPENXR)
  pollEvents();
#endif

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

    // L ボタンと R ボタンの状態
    const auto lrButton(btns[4] | btns[5]);

    if (axesCount > 3)
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

  //
  // マウスによる操作
  //

  // マウスの位置
  double x, y;

#ifdef IMGUI_VERSION
  if (showMenu)
  {
    // ImGui の新規フレームを作成する
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
// ウィンドウのサイズ変更時の処理
//
void GgApp::Window::resize(GLFWwindow* window, int width, int height)
{
  // このインスタンスの this ポインタを得る
  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

  if (instance)
  {
    // ウィンドウのサイズを保存する
    instance->size[0] = width;
    instance->size[1] = height;

#if defined(GG_USE_OPENXR)
    const bool isHmd{ instance->xrSession != XR_NULL_HANDLE };
#else
    constexpr bool isHmd{ false };
#endif

    // HMD 使用時以外
    if (!isHmd)
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
#ifdef IMGUI_VERSION
  if (ImGui::GetIO().WantCaptureMouse) return;
#endif

  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

  if (instance)
  {
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    switch (button)
    {
    case GLFW_MOUSE_BUTTON_1:
      if (action)
      {
        instance->cx = x;
        instance->cy = y;
      }
      break;

    case GLFW_MOUSE_BUTTON_2:
      if (action)
      {
        attitude.orientation.begin(static_cast<float>(x), static_cast<float>(y));
      }
      else
      {
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
#ifdef IMGUI_VERSION
  if (ImGui::GetIO().WantCaptureMouse) return;
#endif

  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

  if (instance)
  {
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT))
    {
      attitude.foreAdjust[0] += static_cast<int>(y);
      instance->update();
    }
    else
    {
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
#ifdef IMGUI_VERSION
  if (ImGui::GetIO().WantCaptureKeyboard) return;
#endif

  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

  if (instance)
  {
    if (action != GLFW_RELEASE)
    {
      const auto shiftKey{ glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) };
      const auto ctrlKey{ glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) };
      const auto altKey{ glfwGetKey(window, GLFW_KEY_LEFT_ALT) || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) };

      instance->key = key;

#if defined(GG_USE_OPENXR)
      const bool isHmd{ instance->xrSession != XR_NULL_HANDLE };
#endif

      switch (key)
      {
      case GLFW_KEY_R:
        instance->reset();
        break;

      case GLFW_KEY_M:
        instance->showMenu = !instance->showMenu;
        break;

      case GLFW_KEY_Z:
        if (shiftKey)
          --attitude.foreAdjust[0];
        else
          ++attitude.foreAdjust[0];
        instance->update();
        break;

      case GLFW_KEY_P:
#if defined(GG_USE_OPENXR)
        if (isHmd) break;
#endif
        if (shiftKey)
          --attitude.parallax;
        else
          ++attitude.parallax;
        instance->update();
        break;

      case GLFW_KEY_E:
        if (!instance->camera) break;
        if (shiftKey)
          instance->camera->decreaseExposure();
        else
          instance->camera->increaseExposure();
        break;

      case GLFW_KEY_G:
        if (!instance->camera) break;
        if (shiftKey)
          instance->camera->decreaseGain();
        else
          instance->camera->increaseGain();
        break;

      case GLFW_KEY_UP:
        if (altKey)
        {
          if (!ctrlKey) attitude.eyeOrientation[camL] *= Attitude::eyeOrientationStep[1];
          if (!shiftKey) attitude.eyeOrientation[camR] *= Attitude::eyeOrientationStep[1];
        }
        else if (ctrlKey)
        {
          ++attitude.circleAdjust[1];
        }
        else if (shiftKey)
        {
          ++attitude.circleAdjust[3];
        }
        else
        {
          ++attitude.backAdjust[0];
        }
        instance->updateCircle();
        break;

      case GLFW_KEY_DOWN:
        if (altKey)
        {
          if (!ctrlKey) attitude.eyeOrientation[camL] *= Attitude::eyeOrientationStep[1].conjugate();
          if (!shiftKey) attitude.eyeOrientation[camR] *= Attitude::eyeOrientationStep[1].conjugate();
        }
        else if (ctrlKey)
        {
          --attitude.circleAdjust[1];
        }
        else if (shiftKey)
        {
          --attitude.circleAdjust[3];
        }
        else
        {
          --attitude.backAdjust[0];
        }
        instance->updateCircle();
        break;

      case GLFW_KEY_RIGHT:
        if (altKey)
        {
          if (!ctrlKey) attitude.eyeOrientation[camL] *= Attitude::eyeOrientationStep[0].conjugate();
          if (!shiftKey) attitude.eyeOrientation[camR] *= Attitude::eyeOrientationStep[0].conjugate();
        }
        else if (ctrlKey)
        {
          ++attitude.circleAdjust[0];
        }
        else if (shiftKey)
        {
          ++attitude.circleAdjust[2];
        }
        else if (defaults.display_mode != MONOCULAR)
        {
          ++attitude.offset;
        }
        instance->updateCircle();
        break;

      case GLFW_KEY_LEFT:
        if (altKey)
        {
          if (!ctrlKey) attitude.eyeOrientation[camL] *= Attitude::eyeOrientationStep[0];
          if (!shiftKey) attitude.eyeOrientation[camR] *= Attitude::eyeOrientationStep[0];
        }
        else if (ctrlKey)
        {
          --attitude.circleAdjust[0];
        }
        else if (shiftKey)
        {
          --attitude.circleAdjust[2];
        }
        else if (defaults.display_mode != MONOCULAR)
        {
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
  xrOriginValid = false;
#endif
}


#if defined(GG_USE_OPENXR)
//
// OpenXR (VR) の初期化処理
//
bool GgApp::Window::initOpenXR()
{
  xrOriginValid = false;

  // OpenGLによるレンダリングを有効にするための拡張機能（XR_KHR_opengl_enable）を指定
  std::vector<const char*> extensions;
  extensions.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);

  // インスタンス作成情報の初期設定
  XrInstanceCreateInfo instanceCreateInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
  strcpy_s(instanceCreateInfo.applicationInfo.applicationName, "TED-OpenXR");
  instanceCreateInfo.applicationInfo.applicationVersion = 1;
  strcpy_s(instanceCreateInfo.applicationInfo.engineName, "None");
  instanceCreateInfo.applicationInfo.engineVersion = 1;
  instanceCreateInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION; // APIのバージョンを指定
  instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  instanceCreateInfo.enabledExtensionNames = extensions.data();

  // OpenXRインスタンスを作成
  XR_CHECK(xrCreateInstance(&instanceCreateInfo, &xrInstance));

  // VRシステム（HMD）の情報を取得
  XrSystemGetInfo systemGetInfo{ XR_TYPE_SYSTEM_GET_INFO };
  systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY; // HMDタイプを指定
  XR_CHECK(xrGetSystem(xrInstance, &systemGetInfo, &xrSystemId));

  // OpenGLグラフィックス要件を取得するためのランタイム関数ポインタを取得
  PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
  XR_CHECK(xrGetInstanceProcAddr(xrInstance, "xrGetOpenGLGraphicsRequirementsKHR",
    reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLGraphicsRequirementsKHR)));

  // OpenGLのバージョンや動作要件をVRシステム側から取得
  XrGraphicsRequirementsOpenGLKHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
  XR_CHECK(pfnGetOpenGLGraphicsRequirementsKHR(xrInstance, xrSystemId, &graphicsRequirements));

  // 現在のGLFW OpenGLコンテキストをOpenXRセッションへ関連付ける。
  // GetDCのHDCは借用資源なので、xrCreateSessionが返った直後にReleaseDCする。
  XrGraphicsBindingOpenGLWin32KHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
  const HWND windowHandle{ glfwGetWin32Window(window) };
  graphicsBinding.hDC = GetDC(windowHandle);              // セッション作成時だけ借用するHDC
  if (!graphicsBinding.hDC) return false;
  graphicsBinding.hGLRC = wglGetCurrentContext();        // レンダリングコンテキストのハンドル

  // OpenXRセッションを作成（取得したグラフィックスコンテキストを紐付ける）
  XrSessionCreateInfo sessionCreateInfo{ XR_TYPE_SESSION_CREATE_INFO };
  sessionCreateInfo.next = &graphicsBinding;
  sessionCreateInfo.systemId = xrSystemId;
  const XrResult sessionResult{ xrCreateSession(xrInstance, &sessionCreateInfo, &xrSession) };
  ReleaseDC(windowHandle, graphicsBinding.hDC);
  if (XR_FAILED(sessionResult))
  {
    char buf[256];
    sprintf_s(buf, "OpenXR session creation failed (Result: %d)", sessionResult);
    NOTIFY(buf);
    return false;
  }

  // トラッキング空間（Stage空間：床面基準のプレイエリア空間）を作成
  XrReferenceSpaceCreateInfo playSpaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
  playSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  playSpaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
  XrResult spaceResult = xrCreateReferenceSpace(xrSession, &playSpaceCreateInfo, &xrPlaySpace);

  // Stage空間の作成に失敗した場合は、フォールバックとしてLocal空間（頭部初期位置基準）を作成
  if (XR_FAILED(spaceResult))
  {
    playSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    XR_CHECK(xrCreateReferenceSpace(xrSession, &playSpaceCreateInfo, &xrPlaySpace));
  }

  // 視点空間（View空間：カメラ基準の空間）を作成
  XrReferenceSpaceCreateInfo viewSpaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
  viewSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  viewSpaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
  XR_CHECK(xrCreateReferenceSpace(xrSession, &viewSpaceCreateInfo, &xrViewSpace));

  // ステレオ表示（左右の目）に必要な設定情報の数を取得
  uint32_t viewCount{ 0 };
  XR_CHECK(xrEnumerateViewConfigurationViews(xrInstance, xrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
    0, &viewCount, nullptr));

  // 左右それぞれの表示領域の詳細設定を取得
  std::vector<XrViewConfigurationView> configViews(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
  XR_CHECK(xrEnumerateViewConfigurationViews(xrInstance, xrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
    viewCount, &viewCount, configViews.data()));
  if (viewCount < xrSwapchains.size()) return false;

  // OpenXRランタイムごとに対応形式が異なるため固定値を渡さず、列挙結果から
  // sRGB、通常RGBA、浮動小数RGBAの優先順でOpenGLが描画可能な形式を選ぶ。
  uint32_t formatCount{ 0 };
  XR_CHECK(xrEnumerateSwapchainFormats(xrSession, 0, &formatCount, nullptr));
  if (formatCount == 0) return false;
  std::vector<int64_t> formats(formatCount);
  XR_CHECK(xrEnumerateSwapchainFormats(xrSession, formatCount, &formatCount, formats.data()));
  const std::array<int64_t, 3> preferredFormats{ GL_SRGB8_ALPHA8, GL_RGBA8, GL_RGBA16F };
  int64_t colorFormat{ 0 };
  for (const auto preferred : preferredFormats)
  {
    if (std::find(formats.begin(), formats.end(), preferred) != formats.end())
    {
      colorFormat = preferred;
      break;
    }
  }
  if (colorFormat == 0)
  {
    NOTIFY("OpenXR runtime has no supported OpenGL color swapchain format.");
    return false;
  }

  // 左右それぞれのスワップチェーン（描画バッファ）をセットアップ
  for (uint32_t eye = 0; eye < 2; ++eye)
  {
    // 左右の目に対応するスワップチェーンを取得
    auto& swapchain{ xrSwapchains[eye] };

    // VRシステム推奨の画像解像度を設定
    swapchain.width = configViews[eye].recommendedImageRectWidth;
    swapchain.height = configViews[eye].recommendedImageRectHeight;

    // スワップチェーンの作成情報を設定
    XrSwapchainCreateInfo swapchainCreateInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };

    // レンダリング先（カラーアタッチメント）および転送元として使用可能に指定
    swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT;
    swapchainCreateInfo.format = colorFormat;
    swapchainCreateInfo.sampleCount = 1;
    swapchainCreateInfo.width = swapchain.width;
    swapchainCreateInfo.height = swapchain.height;
    swapchainCreateInfo.faceCount = 1;
    swapchainCreateInfo.arraySize = 1;
    swapchainCreateInfo.mipCount = 1;

    // スワップチェーンを作成する
    XR_CHECK(xrCreateSwapchain(xrSession, &swapchainCreateInfo, &swapchain.handle));

    // スワップチェーン内のイメージ（テクスチャ）数を取得
    uint32_t imageCount{ 0 };
    XR_CHECK(xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr));
    swapchain.images.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });

    // スワップチェーンから実際のOpenGLテクスチャイメージの配列を取得
    XR_CHECK(xrEnumerateSwapchainImages(swapchain.handle, imageCount, &imageCount,
      reinterpret_cast<XrSwapchainImageBaseHeader*>(swapchain.images.data())));

    // スワップチェーンイメージごとにFBO（フレームバッファオブジェクト）を割り当てる
    swapchain.fbos.resize(imageCount);
    glGenFramebuffers(imageCount, swapchain.fbos.data());

    // 各イメージをFBOにアタッチして、OpenGLから描画可能にする
    for (uint32_t i = 0; i < imageCount; ++i)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, swapchain.fbos[i]);
      glBindTexture(GL_TEXTURE_2D, swapchain.images[i].image);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

      // FBOのカラーアタッチメントとしてスワップチェーンのイメージ（テクスチャ）をバインド
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, swapchain.images[i].image, 0);
    }

    // VR描画用の深度テクスチャを作成し、各設定を行う
    glGenTextures(1, &xrDepthTextures[eye]);
    glBindTexture(GL_TEXTURE_2D, xrDepthTextures[eye]);

    // 深度情報用に32bit浮動小数点形式（GL_DEPTH_COMPONENT32F）を使用
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, swapchain.width, swapchain.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  // PCモニター上でVR映像を確認するためのミラーバッファ（FBOとテクスチャ）を作成
  glGenFramebuffers(1, &xrMirrorFbo);
  glGenTextures(1, &xrMirrorTexture);
  glBindTexture(GL_TEXTURE_2D, xrMirrorTexture);

  // PC表示用のウインドウサイズに合わせてカラーバッファを生成
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[0], size[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindFramebuffer(GL_FRAMEBUFFER, xrMirrorFbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, xrMirrorTexture, 0);

  // FBOのバインドを解除してデフォルトフレームバッファに戻す
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // sRGB色空間への自動補正（ガンマ補正など）を有効化
  glEnable(GL_FRAMEBUFFER_SRGB);

  // HMDと画面描画のズレを防ぐため、PCモニタの垂直同期を一時的に無効化(0)にする（描画はHMDの同期に任せるため）
  glfwSwapInterval(0);

  return true;
}

//
// OpenXR のクリーンアップ処理
//
void GgApp::Window::cleanupOpenXR()
{
  // セッション関連のリソース解放
  if (xrSession != XR_NULL_HANDLE)
  {
    // セッションがまだ実行中なら終了処理を行う
    if (xrSessionRunning)
    {
      xrEndSession(xrSession);
      xrSessionRunning = false;
    }

    // ミラー表示用のFBOとテクスチャを削除
    if (xrMirrorFbo)
    {
      glDeleteFramebuffers(1, &xrMirrorFbo);
      xrMirrorFbo = 0;
    }
    if (xrMirrorTexture)
    {
      glDeleteTextures(1, &xrMirrorTexture);
      xrMirrorTexture = 0;
    }

    // 左右それぞれのスワップチェーンと深度バッファを削除
    for (uint32_t eye = 0; eye < 2; ++eye)
    {
      // 左右の目に対応するスワップチェーンを取得
      auto& swapchain{ xrSwapchains[eye] };

      // スワップチェーンに紐付くFBOリストを削除
      if (!swapchain.fbos.empty())
      {
        glDeleteFramebuffers(static_cast<GLsizei>(swapchain.fbos.size()), swapchain.fbos.data());
        swapchain.fbos.clear();
      }

      // スワップチェーンハンドルを破棄
      if (swapchain.handle != XR_NULL_HANDLE)
      {
        // 通常描画の途中で終了した場合も、ランタイム所有画像を返してからswapchainを破棄する
        if (swapchain.imageAcquired)
        {
          XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
          xrReleaseSwapchainImage(swapchain.handle, &releaseInfo);
          swapchain.imageAcquired = false;
        }
        xrDestroySwapchain(swapchain.handle);
        swapchain.handle = XR_NULL_HANDLE;
      }

      // 深度テクスチャを削除
      if (xrDepthTextures[eye])
      {
        glDeleteTextures(1, &xrDepthTextures[eye]);
        xrDepthTextures[eye] = 0;
      }

      // スワップチェーンのイメージリストをクリア
      swapchain.images.clear();
    }

    // 作成した参照空間（PlaySpace, ViewSpace）を破棄
    if (xrPlaySpace != XR_NULL_HANDLE)
    {
      xrDestroySpace(xrPlaySpace);
      xrPlaySpace = XR_NULL_HANDLE;
    }

    // 作成した参照空間（PlaySpace, ViewSpace）を破棄
    if (xrViewSpace != XR_NULL_HANDLE)
    {
      xrDestroySpace(xrViewSpace);
      xrViewSpace = XR_NULL_HANDLE;
    }

    // セッションオブジェクト自体を破棄
    xrDestroySession(xrSession);
    xrSession = XR_NULL_HANDLE;
  }

  // OpenXR インスタンスオブジェクトを破棄
  if (xrInstance != XR_NULL_HANDLE)
  {
    xrDestroyInstance(xrInstance);
    xrInstance = XR_NULL_HANDLE;
  }

  // PCモニタ側の垂直同期(V-Sync)設定を標準(1)に戻す
  glfwSwapInterval(1);
}

//
// OpenXR イベントのポーリングとセッション状態管理
//
void GgApp::Window::pollEvents()
{
  // OpenXRインスタンスが作成されていなければ何もしない
  if (xrInstance == XR_NULL_HANDLE) return;

  // イベントデータを格納するバッファを準備
  XrEventDataBuffer event{ XR_TYPE_EVENT_DATA_BUFFER };

  // 利用可能なイベントがある限りループして取得する
  while (xrPollEvent(xrInstance, &event) == XR_SUCCESS)
  {
    // イベントの種類に応じて処理を分岐
    switch (event.type)
    {
    // OpenXRインスタンスの破棄要求イベント（強制終了など）
    case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
      setClose(GLFW_TRUE); // アプリケーションウィンドウの終了フラグをセット
      break;

    // セッション（描画やトラッキングの状態）の変化イベント
    case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
      {
        // セッション状態変化イベントのデータを取得
        auto* sessionStateChangedEvent{ reinterpret_cast<XrEventDataSessionStateChanged*>(&event) };

        // このイベントが現在のセッションに関するものであれば処理する
        if (sessionStateChangedEvent->session == xrSession)
        {
          // セッション状態を取得
          XrSessionState state{ sessionStateChangedEvent->state };

          // セッションが描画可能な状態（READY）になったらセッションを開始する
          if (state == XR_SESSION_STATE_READY)
          {
            // セッション開始情報を設定してセッションを開始する
            XrSessionBeginInfo beginInfo{ XR_TYPE_SESSION_BEGIN_INFO };
            beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            if (XR_SUCCEEDED(xrBeginSession(xrSession, &beginInfo)))
            {
              xrSessionRunning = true; // セッション実行中フラグを立てる
            }
          }
          // セッションが停止要求（STOPPING）を受け取ったらセッションを終了する
          else if (state == XR_SESSION_STATE_STOPPING)
          {
            if (XR_SUCCEEDED(xrEndSession(xrSession)))
            {
              xrSessionRunning = false; // セッション実行中フラグを降ろす
            }
          }
          // セッション終了（EXITING）または消失（LOSS_PENDING）時にはアプリケーションも閉じる
          else if (state == XR_SESSION_STATE_EXITING || state == XR_SESSION_STATE_LOSS_PENDING)
          {
            setClose(GLFW_TRUE);
          }
        }
      }
      break;

    default:
      break;
    }

    // 次のイベントのためにバッファのタイプ情報を再セット
    event = { XR_TYPE_EVENT_DATA_BUFFER };
  }
}
#endif

//
// HMD を起動する
//
bool GgApp::Window::startHMD()
{
#if defined(GG_USE_OPENXR)
  return initOpenXR();
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
  cleanupOpenXR();
#endif
}

//
// 描画する目を選択する
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
    glDrawBuffer(eye == camL ? GL_BACK_LEFT : GL_BACK_RIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);
    break;

  case OPENXR:
#if defined(GG_USE_OPENXR)
    if (xrSession != XR_NULL_HANDLE && xrSessionRunning)
    {
      // ランタイムから画像を借り、使用可能になるまで待ってから対応FBOへ描画する。
      // imageAcquiredはcommitで同じ画像を一度だけ返すための所有状態である。
      XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
      uint32_t activeIndex{ 0 };
      auto& swapchain{ xrSwapchains[eye] };
      XR_CHECK_VOID(xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &activeIndex));
      xrSwapchains[eye].activeImageIndex = activeIndex; // 現在のアクティブイメージインデックスを記録

      // 書き込み可能になるまで（GPUでイメージが解放されるまで）待機
      XrSwapchainImageWaitInfo waitInfo{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, nullptr, XR_INFINITE_DURATION };
      const XrResult waitResult{ xrWaitSwapchainImage(swapchain.handle, &waitInfo) };
      if (XR_FAILED(waitResult))
      {
        // 待機失敗時もacquire済み画像を保持したままにせず、空のままランタイムへ返す
        XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
        xrReleaseSwapchainImage(swapchain.handle, &releaseInfo);
        char buf[256];
        sprintf_s(buf, "OpenXR swapchain wait failed (Result: %d)", waitResult);
        NOTIFY(buf);
        return;
      }
      swapchain.imageAcquired = true;

      // 描画先として、スワップチェーンイメージに紐付いたFBOをバインド
      glBindFramebuffer(GL_FRAMEBUFFER, xrSwapchains[eye].fbos[activeIndex]);
      // 深度バッファテクスチャをアタッチ
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, xrDepthTextures[eye], 0);

      // カラーバッファと深度バッファをクリア
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      // ビューポートをVRシステム推奨サイズに設定
      glViewport(0, 0, xrSwapchains[eye].width, xrSwapchains[eye].height);

      return;
    }
#endif
    break;
  default:
    break;
  }

  mv[eye] = ggTranslate(static_cast<GLfloat>(1 - eye * 2) * parallax, 0.0f, 0.0f);
}

//
// 描画を開始する
//   OpenXRのフレーム同期制御を行い、HMDの姿勢情報から左右それぞれの視点の射影行列を決定します。
//
bool GgApp::Window::start()
{
#if defined(GG_USE_OPENXR)
  // セッションが有効でなければ、標準の描画（非OpenXR）として true を返す
  if (xrSession == XR_NULL_HANDLE || !xrSessionRunning) return true;

  // 設定からローカルモデルの座標変換行列を作成し、シーングラフの基準行列として設定
  mm = ggTranslate(attitude.position) * attitude.orientation.getMatrix();
  Scene::setup(mm);

  // VRランタイムに対し、フレームのレンダリング待機を要求（GPU/CPU同期およびフレームレート制御）
  XrFrameWaitInfo waitInfo{ XR_TYPE_FRAME_WAIT_INFO };
  XrResult waitResult = xrWaitFrame(xrSession, &waitInfo, &xrFrameState);
  if (XR_FAILED(waitResult)) return false;

  // フレームのレンダリング開始を宣言
  XrFrameBeginInfo beginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
  XrResult beginResult = xrBeginFrame(xrSession, &beginInfo);
  if (XR_FAILED(beginResult)) return false;

  // アクティブなフレームであることを示すフラグをセット
  xrFrameActive = true;

  // ランタイムがこのフレームの描画を必要としているか判定
  if (xrFrameState.shouldRender)
  {
    // HMDの位置と姿勢（トラッキングデータ）を取得するためのリクエスト情報を設定
    XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
    viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO; // 左右ステレオ
    viewLocateInfo.displayTime = xrFrameState.predictedDisplayTime;                  // 予測表示時刻を指定
    viewLocateInfo.space = xrPlaySpace;                                             // 基準トラッキング空間

    uint32_t viewCount{ 2 };
    XrViewState viewState{ XR_TYPE_VIEW_STATE };
    std::vector<XrView> locateViews(2, { XR_TYPE_VIEW });
    // 左右それぞれの目の位置と向き、および視野角(FOV)を取得
    XrResult locateResult = xrLocateViews(xrSession, &viewLocateInfo, &viewState, viewCount, &viewCount, locateViews.data());

    // 取得したトラッキングデータが有効であれば、各目の射影行列を更新
    if (XR_SUCCEEDED(locateResult) && (viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) &&
      (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT))
    {
      for (int eye = 0; eye < 2; ++eye)
      {
        xrViews[eye] = locateViews[eye];

        // libOVR と同じく、OpenXR の姿勢を逆回転のビュー行列として扱う
        const auto& p = xrViews[eye].pose.position;
        const auto& o = xrViews[eye].pose.orientation;
        po[eye] = GgVector(p.x, p.y, p.z, 1.0f);
        qo[eye] = GgQuaternion(o.x, o.y, o.z, -o.w);
        mo[eye] = qo[eye].getMatrix();

        // 視野角(FOV)情報を取得
        const auto& fov = xrViews[eye].fov;
        // 近クリップ面の位置（ズーム対応）
        const GLfloat zf = defaults.display_near / zoom;

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
        const float p_shift = static_cast<float>((1 - eye * 2) * attitude.parallax) * parallaxStep;

        // 視野角（角度のタンジェント）からFrustum（視錐台）射影行列を構築
        mp[eye].loadFrustum(
          (p_shift + tanf(fov.angleLeft)) * zf, (p_shift + tanf(fov.angleRight)) * zf,
          tanf(fov.angleDown) * zf, tanf(fov.angleUp) * zf,
          defaults.display_near, defaults.display_far
        );
      }

      // 起動時または回復時の頭部中心を、シーン座標の原点として保存する
      const auto& leftEye{ xrViews[camL].pose.position };
      const auto& rightEye{ xrViews[camR].pose.position };
      if (!xrOriginValid)
      {
        xrOriginPosition = GgVector
        {
          (leftEye.x + rightEye.x) * 0.5f,
          (leftEye.y + rightEye.y) * 0.5f,
          (leftEye.z + rightEye.z) * 0.5f,
          1.0f
        };
        xrOriginValid = true;
      }

      // 頭部原点からの相対的な各眼位置を、シーン用のビュー変換へ反映する
      for (int eye = 0; eye < eyeCount; ++eye)
      {
        const auto& eyePosition{ xrViews[eye].pose.position };
        const GLfloat tx{ eyePosition.x - xrOriginPosition[0] };
        const GLfloat ty{ eyePosition.y - xrOriginPosition[1] };
        const GLfloat tz{ eyePosition.z - xrOriginPosition[2] };

        // ワールド座標から眼座標へ移すビュー平行移動 T^-1
        mv[eye].loadTranslate(-tx, -ty, -tz);

        // controller 0/1にはビュー変換 R^-1*T^-1 の逆変換 T*Rを格納する。
        // これにより眼へ追従するノードでは頭部の回転と平行移動が相殺され、
        // 左右眼の相対位置だけを保ったまま視界上の位置が固定される。
        const GgMatrix eyePose{ ggTranslate(tx, ty, tz) * mo[eye].transpose() };
        Scene::setLocalAttitude(eye, eyePose);
      }
    }
    return true; // 描画処理へ進む
  }

  // HMDが非表示中などshouldRender=falseでも、BeginFrameと対になるEndFrameは必要なので
  // レイヤーを持たない空フレームを送り、通常の左右描画は行わない。
  XrFrameEndInfo endInfo{ XR_TYPE_FRAME_END_INFO };
  endInfo.displayTime = xrFrameState.predictedDisplayTime;
  endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  endInfo.layerCount = 0;
  endInfo.layers = nullptr;
  xrFrameActive = false;
  XR_CHECK(xrEndFrame(xrSession, &endInfo));

  return false; // 描画は行わない
#else
  return true;
#endif
}

//
// 透視投影変換行列を更新する
//
void GgApp::Window::update()
{
  zoom = 1.0f / (1.0f - zoomStep * attitude.foreAdjust[0]);
  focal = 1.0f / (1.0f - backFocalStep * attitude.backAdjust[0]);
  offset = offsetDefault + offsetStep * attitude.offset;

#if defined(GG_USE_OPENXR)
  if (xrSession != XR_NULL_HANDLE)
  {
    return;
  }
#endif

  parallax = (defaults.display_mode == MONOCULAR ? 0.0f : defaultParallax) + parallaxStep * attitude.parallax;
  const GLfloat screenHeight{ defaults.display_center / defaults.display_distance };
  const GLfloat screenWidth{ screenHeight * aspect };
  const GLfloat shift{ defaults.display_mode != MONOCULAR
    ? parallax * defaults.display_near / defaults.display_distance : 0.0f };
  const GLfloat zf{ defaults.display_near / zoom };

  const GLfloat fovL[]
  {
    -screenWidth + shift,
    screenWidth + shift,
    -screenHeight,
    screenHeight
  };

  mp[0].loadFrustum(fovL[0] * zf, fovL[1] * zf, fovL[2] * zf, fovL[3] * zf,
    defaults.display_near, defaults.display_far);

  screen[0][0] = (fovL[1] - fovL[0]) * 0.5f;
  screen[0][1] = (fovL[3] - fovL[2]) * 0.5f;
  screen[0][2] = (fovL[1] + fovL[0]) * 0.5f;
  screen[0][3] = (fovL[3] + fovL[2]) * 0.5f;

  if (defaults.display_mode != MONOCULAR)
  {
    const GLfloat fovR[] =
    {
      -screenWidth - shift,
      screenWidth - shift,
      -screenHeight,
      screenHeight,
    };

    mp[1].loadFrustum(fovR[0] * zf, fovR[1] * zf, fovR[2] * zf, fovR[3] * zf,
      defaults.display_near, defaults.display_far);

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
// 描画を完了する
//
void GgApp::Window::commit(int eye)
{
#if defined(GG_USE_OPENXR)
  if (xrSession != XR_NULL_HANDLE && xrSessionRunning && xrFrameActive
    && xrSwapchains[eye].imageAcquired)
  {
    // PCモニターでのミラー表示が有効な場合
    if (showMirror)
    {
      GLsizei wWidth = size[0];
      GLsizei wHeight = size[1];

      // 読み込み元FBOとして現在レンダリングしたスワップチェーンイメージのFBOを設定
      glBindFramebuffer(GL_READ_FRAMEBUFFER, xrSwapchains[eye].fbos[xrSwapchains[eye].activeImageIndex]);

      // 書き込み先FBOとしてミラー用のFBOを設定
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, xrMirrorFbo);

      // 左右の目に対応する画面上の描画領域（ピクセル座標）を決定
      GLint dx0 = (eye == 0) ? 0 : wWidth / 2;
      GLint dx1 = (eye == 0) ? wWidth / 2 : wWidth;

      // スワップチェーンFBOをミラーFBOの対応領域へ拡大・縮小コピーする
      glBlitFramebuffer(0, 0, xrSwapchains[eye].width, xrSwapchains[eye].height,
        dx0, 0, dx1, wHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }

    // デフォルトのフレームバッファにバインドを戻す
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // スワップチェーン画像をランタイムへ返す前に、描画コマンドを GPU へ送る
    glFlush();

    // GLコマンドを投入した画像の所有権をランタイムへ返し、HMD合成処理で使用可能にする
    XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };

    // イメージのコントロール権をVRシステムに返却（これによりVRシステム側で表示されるようになる）
    XR_CHECK_VOID(xrReleaseSwapchainImage(xrSwapchains[eye].handle, &releaseInfo));
    xrSwapchains[eye].imageAcquired = false;
  }
#endif
}

//
// カラーバッファを入れ替えてイベントを取り出す
//
void GgApp::Window::swapBuffers()
{
  ggError();

#if defined(GG_USE_OPENXR)
  if (xrSession != XR_NULL_HANDLE && xrSessionRunning && xrFrameActive)
  {
    // プロジェクション（投影）レイヤーを設定（HMDに表示する基本的な3Dシーンの構成要素）
    XrCompositionLayerProjection projectionLayer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };

    // レンズの色収差補正フラグを設定
    projectionLayer.layerFlags = XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
    projectionLayer.space = xrPlaySpace; // トラッキング基準空間を指定

    // 左右の目それぞれの投影ビュー情報を格納するベクタ
    std::vector<XrCompositionLayerProjectionView> projectionViews(2, { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW });
    for (int eye = 0; eye < 2; ++eye)
    {
      projectionViews[eye].pose = xrViews[eye].pose; // start() で取得した視点姿勢を設定
      projectionViews[eye].fov = xrViews[eye].fov;   // start() で取得した視野角を設定
      projectionViews[eye].subImage.swapchain = xrSwapchains[eye].handle; // 描画したスワップチェーン
      projectionViews[eye].subImage.imageRect.offset = { 0, 0 };
      projectionViews[eye].subImage.imageRect.extent = {
        static_cast<int32_t>(xrSwapchains[eye].width),
        static_cast<int32_t>(xrSwapchains[eye].height)
      };
      projectionViews[eye].subImage.imageArrayIndex = 0;
    }

    projectionLayer.viewCount = 2;
    projectionLayer.views = projectionViews.data();

    // 合成処理に送るレイヤーのリストを構築
    const XrCompositionLayerBaseHeader* const layers[] = {
      reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer)
    };

    // startで予測した同じ表示時刻と左右姿勢をレイヤーへまとめ、1回のEndFrameで提出する
    XrFrameEndInfo endInfo{ XR_TYPE_FRAME_END_INFO };
    endInfo.displayTime = xrFrameState.predictedDisplayTime;          // 予測表示時刻
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE; // 不透明モード
    endInfo.layerCount = 1;                                          // レイヤー数は1個
    endInfo.layers = layers;                                         // レイヤーの配列

    // VRシステムに対し、現在のフレームのすべての描画レイヤーを送信して表示を要求
    xrFrameActive = false;
    XR_CHECK_VOID(xrEndFrame(xrSession, &endInfo));

    // ミラー表示が有効なら、ミラーFBO（左右の目が合成されたもの）からデフォルトフレームバッファへコピー
    if (showMirror)
    {
      GLsizei wWidth = size[0];
      GLsizei wHeight = size[1];

      glBindFramebuffer(GL_READ_FRAMEBUFFER, xrMirrorFbo);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // デフォルトフレームバッファを指定

      // ミラーバッファのカラーデータを画面にコピー
      glBlitFramebuffer(0, 0, wWidth, wHeight, 0, 0, wWidth, wHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

#if defined(IMGUI_VERSION)
      // PC側モニター上に Dear ImGui のメニューを描画
      if (showMenu)
      {
        ImDrawData* const imDrawData{ ImGui::GetDrawData() };
        if (imDrawData) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
      }
#endif
      // GLFWのダブルバッファリング：バックバッファをフロントバッファに入れ替えてPC画面に表示
      glfwSwapBuffers(window);
    }
    glFlush();
    return;
  }
#endif

#if defined(IMGUI_VERSION)
  // 非OpenXR（通常モード）での ImGui メニューの描画
  if (showMenu)
  {
    ImDrawData* const imDrawData{ ImGui::GetDrawData() };
    if (imDrawData) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
  }
#endif

// GLFWのダブルバッファリングによる通常画面の表示更新
  glfwSwapBuffers(window);
}

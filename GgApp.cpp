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
  : window{ nullptr }
  , camera{ nullptr }
  , showScene{ true }
  , showMirror{ true }
  , showMenu{ true }
  , key{ GLFW_KEY_UNKNOWN }
  , joy{ -1 }
  , zoom{ 1.0f }
  , focal{ 1.0f }
  , parallax{ defaultParallax }
  , offset{ 0.0f }
{
  // 最初のインスタンスで一度だけ実行
  static bool firstTime{ true };
  if (firstTime)
  {
    // クワッドバッファステレオモードを有効にする
    if (defaults.display_quadbuffer)
    {
      glfwWindowHint(GLFW_STEREO, GLFW_TRUE);
    }

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

    bool isHmd = false;
#if defined(GG_USE_OPENXR)
    isHmd = (instance->xrSession != XR_NULL_HANDLE);
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

      bool isHmd = false;
#if defined(GG_USE_OPENXR)
      isHmd = (instance->xrSession != XR_NULL_HANDLE);
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
        if (isHmd) break;
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
}


#if defined(GG_USE_OPENXR)
//
// OpenXR の初期化
//
bool GgApp::Window::initOpenXR()
{
  std::vector<const char*> extensions;
  extensions.push_back(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);

  XrInstanceCreateInfo instanceCreateInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
  strcpy_s(instanceCreateInfo.applicationInfo.applicationName, "TED-OpenXR");
  instanceCreateInfo.applicationInfo.applicationVersion = 1;
  strcpy_s(instanceCreateInfo.applicationInfo.engineName, "None");
  instanceCreateInfo.applicationInfo.engineVersion = 1;
  instanceCreateInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
  instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  instanceCreateInfo.enabledExtensionNames = extensions.data();

  XR_CHECK(xrCreateInstance(&instanceCreateInfo, &xrInstance));

  XrSystemGetInfo systemGetInfo{ XR_TYPE_SYSTEM_GET_INFO };
  systemGetInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  XR_CHECK(xrGetSystem(xrInstance, &systemGetInfo, &xrSystemId));

  PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
  XR_CHECK(xrGetInstanceProcAddr(xrInstance, "xrGetOpenGLGraphicsRequirementsKHR",
    reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLGraphicsRequirementsKHR)));

  XrGraphicsRequirementsOpenGLKHR graphicsRequirements{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
  XR_CHECK(pfnGetOpenGLGraphicsRequirementsKHR(xrInstance, xrSystemId, &graphicsRequirements));

  XrGraphicsBindingOpenGLWin32KHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
  graphicsBinding.hDC = GetDC(glfwGetWin32Window(window));
  graphicsBinding.hGLRC = wglGetCurrentContext();

  XrSessionCreateInfo sessionCreateInfo{ XR_TYPE_SESSION_CREATE_INFO };
  sessionCreateInfo.next = &graphicsBinding;
  sessionCreateInfo.systemId = xrSystemId;
  XR_CHECK(xrCreateSession(xrInstance, &sessionCreateInfo, &xrSession));

  XrReferenceSpaceCreateInfo playSpaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
  playSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  playSpaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
  XrResult spaceResult = xrCreateReferenceSpace(xrSession, &playSpaceCreateInfo, &xrPlaySpace);
  if (XR_FAILED(spaceResult))
  {
    playSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    XR_CHECK(xrCreateReferenceSpace(xrSession, &playSpaceCreateInfo, &xrPlaySpace));
  }

  XrReferenceSpaceCreateInfo viewSpaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
  viewSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  viewSpaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
  XR_CHECK(xrCreateReferenceSpace(xrSession, &viewSpaceCreateInfo, &xrViewSpace));

  uint32_t viewCount = 0;
  XR_CHECK(xrEnumerateViewConfigurationViews(xrInstance, xrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
    0, &viewCount, nullptr));
  std::vector<XrViewConfigurationView> configViews(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
  XR_CHECK(xrEnumerateViewConfigurationViews(xrInstance, xrSystemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
    viewCount, &viewCount, configViews.data()));

  for (uint32_t eye = 0; eye < 2; ++eye)
  {
    auto& swapchain = xrSwapchains[eye];
    swapchain.width = configViews[eye].recommendedImageRectWidth;
    swapchain.height = configViews[eye].recommendedImageRectHeight;

    XrSwapchainCreateInfo swapchainCreateInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
    swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT;
    swapchainCreateInfo.format = GL_SRGB8_ALPHA8;
    swapchainCreateInfo.sampleCount = 1;
    swapchainCreateInfo.width = swapchain.width;
    swapchainCreateInfo.height = swapchain.height;
    swapchainCreateInfo.faceCount = 1;
    swapchainCreateInfo.arraySize = 1;
    swapchainCreateInfo.mipCount = 1;

    XR_CHECK(xrCreateSwapchain(xrSession, &swapchainCreateInfo, &swapchain.handle));

    uint32_t imageCount = 0;
    XR_CHECK(xrEnumerateSwapchainImages(swapchain.handle, 0, &imageCount, nullptr));
    swapchain.images.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });
    XR_CHECK(xrEnumerateSwapchainImages(swapchain.handle, imageCount, &imageCount,
      reinterpret_cast<XrSwapchainImageBaseHeader*>(swapchain.images.data())));

    swapchain.fbos.resize(imageCount);
    glGenFramebuffers(imageCount, swapchain.fbos.data());

    for (uint32_t i = 0; i < imageCount; ++i)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, swapchain.fbos[i]);
      glBindTexture(GL_TEXTURE_2D, swapchain.images[i].image);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, swapchain.images[i].image, 0);
    }

    glGenTextures(1, &xrDepthTextures[eye]);
    glBindTexture(GL_TEXTURE_2D, xrDepthTextures[eye]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, swapchain.width, swapchain.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

  glGenFramebuffers(1, &xrMirrorFbo);
  glGenTextures(1, &xrMirrorTexture);
  glBindTexture(GL_TEXTURE_2D, xrMirrorTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[0], size[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindFramebuffer(GL_FRAMEBUFFER, xrMirrorFbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, xrMirrorTexture, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glEnable(GL_FRAMEBUFFER_SRGB);
  glfwSwapInterval(0);

  return true;
}

//
// OpenXR のクリーンアップ
//
void GgApp::Window::cleanupOpenXR()
{
  if (xrSession != XR_NULL_HANDLE)
  {
    if (xrSessionRunning)
    {
      xrEndSession(xrSession);
      xrSessionRunning = false;
    }

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

    for (uint32_t eye = 0; eye < 2; ++eye)
    {
      auto& swapchain = xrSwapchains[eye];
      if (!swapchain.fbos.empty())
      {
        glDeleteFramebuffers(static_cast<GLsizei>(swapchain.fbos.size()), swapchain.fbos.data());
        swapchain.fbos.clear();
      }
      if (swapchain.handle != XR_NULL_HANDLE)
      {
        xrDestroySwapchain(swapchain.handle);
        swapchain.handle = XR_NULL_HANDLE;
      }
      if (xrDepthTextures[eye])
      {
        glDeleteTextures(1, &xrDepthTextures[eye]);
        xrDepthTextures[eye] = 0;
      }
      swapchain.images.clear();
    }

    if (xrPlaySpace != XR_NULL_HANDLE)
    {
      xrDestroySpace(xrPlaySpace);
      xrPlaySpace = XR_NULL_HANDLE;
    }
    if (xrViewSpace != XR_NULL_HANDLE)
    {
      xrDestroySpace(xrViewSpace);
      xrViewSpace = XR_NULL_HANDLE;
    }

    xrDestroySession(xrSession);
    xrSession = XR_NULL_HANDLE;
  }

  if (xrInstance != XR_NULL_HANDLE)
  {
    xrDestroyInstance(xrInstance);
    xrInstance = XR_NULL_HANDLE;
  }

  glDisable(GL_FRAMEBUFFER_SRGB);
  glfwSwapInterval(1);
}

//
// OpenXR イベントのポーリングとセッション状態管理
//
void GgApp::Window::pollEvents()
{
  if (xrInstance == XR_NULL_HANDLE) return;

  XrEventDataBuffer event{ XR_TYPE_EVENT_DATA_BUFFER };
  while (xrPollEvent(xrInstance, &event) == XR_SUCCESS)
  {
    switch (event.type)
    {
    case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
      setClose(GLFW_TRUE);
      break;

    case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
      {
        auto* sessionStateChangedEvent = reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
        if (sessionStateChangedEvent->session == xrSession)
        {
          XrSessionState state = sessionStateChangedEvent->state;
          if (state == XR_SESSION_STATE_READY)
          {
            XrSessionBeginInfo beginInfo{ XR_TYPE_SESSION_BEGIN_INFO };
            beginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
            if (XR_SUCCEEDED(xrBeginSession(xrSession, &beginInfo)))
            {
              xrSessionRunning = true;
            }
          }
          else if (state == XR_SESSION_STATE_STOPPING)
          {
            if (XR_SUCCEEDED(xrEndSession(xrSession)))
            {
              xrSessionRunning = false;
            }
          }
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

    event = { XR_TYPE_EVENT_DATA_BUFFER };
  }
}
#endif

//
// Oculus Rift を起動する
//
bool GgApp::Window::startOculus()
{
#if defined(GG_USE_OPENXR)
  return initOpenXR();
#else
  return false;
#endif
}

//
// Oculus Rift を停止する
//
void GgApp::Window::stopOculus()
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

  case OCULUS:
#if defined(GG_USE_OPENXR)
    if (xrSession != XR_NULL_HANDLE && xrSessionRunning)
    {
      XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
      uint32_t activeIndex = 0;
      xrAcquireSwapchainImage(xrSwapchains[eye].handle, &acquireInfo, &activeIndex);
      xrSwapchains[eye].activeImageIndex = activeIndex;

      XrSwapchainImageWaitInfo waitInfo{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO, nullptr, XR_INFINITE_DURATION };
      xrWaitSwapchainImage(xrSwapchains[eye].handle, &waitInfo);

      glBindFramebuffer(GL_FRAMEBUFFER, xrSwapchains[eye].fbos[activeIndex]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, xrDepthTextures[eye], 0);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glViewport(0, 0, xrSwapchains[eye].width, xrSwapchains[eye].height);

      Scene::setLocalAttitude(eye, mo[eye].transpose());
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
//
bool GgApp::Window::start()
{
#if defined(GG_USE_OPENXR)
  if (xrSession == XR_NULL_HANDLE || !xrSessionRunning) return true;

  mm = ggTranslate(attitude.position) * attitude.orientation.getMatrix();
  Scene::setup(mm);

  XrFrameWaitInfo waitInfo{ XR_TYPE_FRAME_WAIT_INFO };
  XrResult waitResult = xrWaitFrame(xrSession, &waitInfo, &xrFrameState);
  if (XR_FAILED(waitResult)) return false;

  XrFrameBeginInfo beginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
  XrResult beginResult = xrBeginFrame(xrSession, &beginInfo);
  if (XR_FAILED(beginResult)) return false;

  xrFrameActive = true;

  if (xrFrameState.shouldRender)
  {
    XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
    viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    viewLocateInfo.displayTime = xrFrameState.predictedDisplayTime;
    viewLocateInfo.space = xrPlaySpace;

    uint32_t viewCount = 2;
    XrViewState viewState{ XR_TYPE_VIEW_STATE };
    std::vector<XrView> locateViews(2, { XR_TYPE_VIEW });
    XrResult locateResult = xrLocateViews(xrSession, &viewLocateInfo, &viewState, viewCount, &viewCount, locateViews.data());

    if (XR_SUCCEEDED(locateResult) && (viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) &&
      (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT))
    {
      for (int eye = 0; eye < 2; ++eye)
      {
        xrViews[eye] = locateViews[eye];

        const auto& p = xrViews[eye].pose.position;
        const auto& o = xrViews[eye].pose.orientation;
        po[eye] = GgVector(p.x, p.y, p.z, 1.0f);
        qo[eye] = GgQuaternion(o.x, o.y, o.z, o.w);
        mo[eye] = qo[eye].getMatrix();

        const auto& fov = xrViews[eye].fov;
        const GLfloat zf = defaults.display_near / zoom;
        const float p_shift = static_cast<float>((1 - eye * 2) * attitude.parallax) * parallaxStep;

        mp[eye].loadFrustum(
          (p_shift + tanf(fov.angleLeft)) * zf, (p_shift + tanf(fov.angleRight)) * zf,
          tanf(fov.angleDown) * zf, tanf(fov.angleUp) * zf,
          defaults.display_near, defaults.display_far
        );
      }
    }
    return true;
  }

  XrFrameEndInfo endInfo{ XR_TYPE_FRAME_END_INFO };
  endInfo.displayTime = xrFrameState.predictedDisplayTime;
  endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  endInfo.layerCount = 0;
  endInfo.layers = nullptr;
  xrEndFrame(xrSession, &endInfo);
  xrFrameActive = false;

  return false;
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

// 描画を完了する
void GgApp::Window::commit(int eye)
{
#if defined(GG_USE_OPENXR)
  if (xrSession != XR_NULL_HANDLE && xrSessionRunning && xrFrameActive)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
    xrReleaseSwapchainImage(xrSwapchains[eye].handle, &releaseInfo);
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
    XrCompositionLayerProjection projectionLayer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    projectionLayer.layerFlags = XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
    projectionLayer.space = xrPlaySpace;

    std::vector<XrCompositionLayerProjectionView> projectionViews(2, { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW });
    for (int eye = 0; eye < 2; ++eye)
    {
      projectionViews[eye].pose = xrViews[eye].pose;
      projectionViews[eye].fov = xrViews[eye].fov;
      projectionViews[eye].subImage.swapchain = xrSwapchains[eye].handle;
      projectionViews[eye].subImage.imageRect.offset = { 0, 0 };
      projectionViews[eye].subImage.imageRect.extent = {
        static_cast<int32_t>(xrSwapchains[eye].width),
        static_cast<int32_t>(xrSwapchains[eye].height)
      };
      projectionViews[eye].subImage.imageArrayIndex = 0;
    }

    projectionLayer.viewCount = 2;
    projectionLayer.views = projectionViews.data();

    const XrCompositionLayerBaseHeader* const layers[] = {
      reinterpret_cast<const XrCompositionLayerBaseHeader*>(&projectionLayer)
    };

    XrFrameEndInfo endInfo{ XR_TYPE_FRAME_END_INFO };
    endInfo.displayTime = xrFrameState.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    endInfo.layerCount = 1;
    endInfo.layers = layers;

    xrEndFrame(xrSession, &endInfo);
    xrFrameActive = false;

    if (showMirror)
    {
      GLsizei wWidth = size[0];
      GLsizei wHeight = size[1];

      for (int eye = 0; eye < 2; ++eye)
      {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, xrSwapchains[eye].fbos[xrSwapchains[eye].activeImageIndex]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, xrMirrorFbo);

        GLint dx0 = (eye == 0) ? 0 : wWidth / 2;
        GLint dx1 = (eye == 0) ? wWidth / 2 : wWidth;

        glBlitFramebuffer(0, 0, xrSwapchains[eye].width, xrSwapchains[eye].height,
          dx0, 0, dx1, wHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
      }

      glBindFramebuffer(GL_READ_FRAMEBUFFER, xrMirrorFbo);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
      glBlitFramebuffer(0, 0, wWidth, wHeight, 0, 0, wWidth, wHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

#if defined(IMGUI_VERSION)
      if (showMenu)
      {
        ImDrawData* const imDrawData{ ImGui::GetDrawData() };
        if (imDrawData) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
      }
#endif
      glfwSwapBuffers(window);
    }
    glFlush();
    return;
  }
#endif

#if defined(IMGUI_VERSION)
  if (showMenu)
  {
    ImDrawData* const imDrawData{ ImGui::GetDrawData() };
    if (imDrawData) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
  }
#endif
  glfwSwapBuffers(window);
}

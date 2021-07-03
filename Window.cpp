//
// ウィンドウ関連の処理
//
#include "Window.h"

// Oculus Rift 関連の処理
#include "Oculus.h"

// 姿勢
#include "Attitude.h"

// シーングラフ
#include "Scene.h"

// 標準ライブラリ
#include <fstream>

// Dear ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// ジョイスティック番号の最大値
constexpr int maxJoystick{ 4 };

// Oculus Rift のセッション
Oculus* Window::oculus{ nullptr };

//
// コンストラクタ
//
Window::Window(int width, int height, const char *title, GLFWmonitor *monitor, GLFWwindow *share)
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
  const_cast<GLFWwindow *>(window) = glfwCreateWindow(width, height, title, monitor, share);

  // ウィンドウが開かれなかったら戻る
  if (!window) return;

  //
  // 初期設定
  //

  // ヘッドトラッキングの変換行列を初期化する
  for (auto &m : mo) m = ggIdentity();

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
  for (int i = 0; i < maxJoystick; ++i)
  {
    if (glfwJoystickPresent(i))
    {
      // 存在するジョイスティック番号
      joy = i;

      // スティックの中立位置を求める
      int axesCount;
      const auto *const axes(glfwGetJoystickAxes(joy, &axesCount));

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
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  //ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can also read multiple fonts and use ImGui::PushFont()/PopFont() to select them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
  // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
  // - Read 'docs/FONTS.txt' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
  //io.Fonts->AddFontDefault();
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
  const ImFont* const font{ io.Fonts->AddFontFromFileTTF("Mplus1-Regular.ttf", 22.0f, NULL, io.Fonts->GetGlyphRangesJapanese()) };
  IM_ASSERT(font != NULL);
#endif

  // 投影変換行列・ビューポートを初期化する
  resize(window, width, height);
}

//
// デストラクタ
//
Window::~Window()
{
  // ウィンドウが開かれていなかったら戻る
  if (!window) return;

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
Window::operator bool()
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
    const auto *const btns(glfwGetJoystickButtons(joy, &btnsCount));

    // スティック
    int axesCount;
    const auto *const axes(glfwGetJoystickAxes(joy, &axesCount));

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
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[0];
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[0].conjugate();
        }
        else
        {
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[0].conjugate();
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[0];
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
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[1];
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[1].conjugate();
        }
        else
        {
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[1].conjugate();
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[1];
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
  const ImGuiIO &io(ImGui::GetIO());
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

<<<<<<< HEAD
  //
  // キーボードによる操作
  //

  // シフトキーの状態
  const auto shiftKey(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT));

  // コントロールキーの状態
  const auto ctrlKey(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL));

  // オルタネートキーの状態
  const auto altKey(glfwGetKey(window, GLFW_KEY_LEFT_ALT) || glfwGetKey(window, GLFW_KEY_RIGHT_ALT));

  // 右矢印キー操作
  if (glfwGetKey(window, GLFW_KEY_RIGHT))
  {
    if (altKey)
    {
      // 背景を右に回転する
      if (!ctrlKey) attitude.eyeOrientation[camL] *= qrStep[0].conjugate();
      if (!shiftKey) attitude.eyeOrientation[camR] *= qrStep[0].conjugate();
    }
    else if (ctrlKey)
    {
      // 背景に対する横方向の画角を広げる
      circle[0] = defaults.camera_fov_x + fovStep * ++attitude.circleAdjust[0];
    }
    else if (shiftKey)
    {
      // 背景を右にずらす
      circle[2] = defaults.camera_center_x + shiftStep * ++attitude.circleAdjust[2];
    }
    else if (defaults.display_mode != MONOCULAR)
    {
      // スクリーンの間隔を拡大する
      offset = offsetDefault + offsetStep * ++attitude.offset;
    }
  }

  // 左矢印キー操作
  if (glfwGetKey(window, GLFW_KEY_LEFT))
  {
    if (altKey)
    {
      // 背景を左に回転する
      if (!ctrlKey) attitude.eyeOrientation[camL] *= qrStep[0];
      if (!shiftKey) attitude.eyeOrientation[camR] *= qrStep[0];
    }
    else if (ctrlKey)
    {
      // 背景に対する横方向の画角を狭める
      circle[0] = defaults.camera_fov_x + fovStep * --attitude.circleAdjust[0];
    }
    else if (shiftKey)
    {
      // 背景を左にずらす
      circle[2] = defaults.camera_center_x + shiftStep * --attitude.circleAdjust[2];
    }
    else if (defaults.display_mode != MONOCULAR)
    {
      // 視差を縮小する
      offset = offsetDefault + offsetStep * --attitude.offset;
    }
  }

  // 上矢印キー操作
  if (glfwGetKey(window, GLFW_KEY_UP))
  {
    if (altKey)
    {
      // 背景を上に回転する
      if (!ctrlKey) attitude.eyeOrientation[camL] *= qrStep[1];
      if (!shiftKey) attitude.eyeOrientation[camR] *= qrStep[1];
    }
    else if (ctrlKey)
    {
      // 背景に対する縦方向の画角を広げる
      circle[1] = defaults.camera_fov_y + fovStep * ++attitude.circleAdjust[1];
    }
    else if (shiftKey)
    {
      // 背景を上にずらす
      circle[3] = defaults.camera_center_y + shiftStep * ++attitude.circleAdjust[3];
    }
    else
    {
      // 焦点距離を延ばす
      focal = 1.0f / (1.0f - backFocalStep * ++attitude.backAdjust[0]);
    }
  }

  // 下矢印キー操作
  if (glfwGetKey(window, GLFW_KEY_DOWN))
  {
    if (altKey)
    {
      // 背景を下に回転する
      if (!ctrlKey) attitude.eyeOrientation[camL] *= qrStep[1].conjugate();
      if (!shiftKey) attitude.eyeOrientation[camR] *= qrStep[1].conjugate();
    }
    else if (ctrlKey)
    {
      // 背景に対する縦方向の画角を狭める
      circle[1] = defaults.camera_fov_y + fovStep * --attitude.circleAdjust[1];
    }
    else if (shiftKey)
    {
      // 背景を下にずらす
      circle[3] = defaults.camera_center_y + shiftStep * --attitude.circleAdjust[3];
    }
    else
    {
      // 焦点距離を縮める
      focal = 1.0f / (1.0f - backFocalStep * --attitude.backAdjust[0]);
    }
  }

  // 'P' キーの操作
  if (glfwGetKey(window, GLFW_KEY_P))
  {
    // 'P' キーの操作
    if (glfwGetKey(window, GLFW_KEY_P))
    {
      // 視差を調整する
      parallax = defaultParallax + parallaxStep * (shiftKey ? --attitude.parallax : ++attitude.parallax);
    }

    // 'Z' キーの操作
    else if (glfwGetKey(window, GLFW_KEY_Z))
    {
      // ズーム率を調整する
      zoom = 1.0f / (1.0f - zoomStep * (shiftKey ? --attitude.foreAdjust[0] : ++attitude.foreAdjust[0]));
    }

    // 透視投影変換行列を更新する
    update();
  }

  // カメラの制御
  if (camera)
  {
    // 'E' キーの操作
    if (glfwGetKey(window, GLFW_KEY_E))
    {
      if (shiftKey)
      {
        // 露出を下げる
        camera->decreaseExposure();
      }
      else
      {
        // 露出を上げる
        camera->increaseExposure();
      }
    }

    // 'G' キーの操作
    if (glfwGetKey(window, GLFW_KEY_G))
    {
      if (shiftKey)
      {
        // 利得を下げる
        camera->decreaseGain();
      }
      else
      {
        // 利得を上げる
        camera->increaseGain();
      }
    }
  }

=======
>>>>>>> 6a7aec8 (add attitude menu)
  return true;
}

//
// カラーバッファを入れ替えてイベントを取り出す
//
//   ・図形の描画終了後に呼び出す
//   ・ダブルバッファリングのバッファの入れ替えを行う
//
void Window::swapBuffers()
{
  // エラーチェック
  ggError();

  // Oculus Rift 使用時
  if (oculus)
  {
    // 描画したフレームを Oculus Rift に転送する
    oculus->submit();

    // 描画したフレームを Oculus Rift に転送する
    if (showMirror) oculus->submitMirror(width, height);

#ifdef IMGUI_VERSION
    if (showMenu)
    {
      // ユーザインタフェースを描画する
      ImDrawData *const imDrawData(ImGui::GetDrawData());
      if (imDrawData) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
    }
#endif

    // 残っている OpenGL コマンドを実行する
    glFlush();
  }
  else
  {
#ifdef IMGUI_VERSION
    if (showMenu)
    {
      // ユーザインタフェースを描画する
      ImDrawData *const imDrawData(ImGui::GetDrawData());
      if (imDrawData) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
    }
#endif

    // カラーバッファを入れ替える
    glfwSwapBuffers(window);
  }
}

//
// ウィンドウのサイズ変更時の処理
//
//   ・ウィンドウのサイズ変更時にコールバック関数として呼び出される
//   ・ウィンドウの作成時には明示的に呼び出す
//
void Window::resize(GLFWwindow *window, int width, int height)
{
  // このインスタンスの this ポインタを得る
  Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

  if (instance)
  {
    // ウィンドウのサイズを保存する
    instance->size[0] = width;
    instance->size[1] = height;

    // Oculus Rift 使用時以外
    if (!instance->oculus)
    {
      if (defaults.display_mode == INTERLACE)
      {
        // VR 室のディスプレイでは表示領域の横半分をビューポートにする
        width /= 2;

        // VR 室のディスプレイのアスペクト比は表示領域の横半分になる
        instance->aspect = static_cast<GLfloat>(width) / static_cast<GLfloat>(height);
      }
      else
      {
        // リサイズ後のディスプレイののアスペクト比を求める
        instance->aspect = defaults.display_aspect
          ? defaults.display_aspect
          : static_cast<GLfloat>(width) / static_cast<GLfloat>(height);

        // リサイズ後のビューポートを設定する
        switch (defaults.display_mode)
        {
        case SIDE_BY_SIDE:

          // ウィンドウの横半分をビューポートにする
          width /= 2;
          break;

        case TOP_AND_BOTTOM:

          // ウィンドウの縦半分をビューポートにする
          height /= 2;
          break;

        default:
          break;
        }
      }
    }

    // ビューポート (Oculus Rift の場合はミラー表示用のウィンドウ) の大きさを保存しておく
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
//   ・マウスボタンを押したときにコールバック関数として呼び出される
//
void Window::mouse(GLFWwindow *window, int button, int action, int mods)
{
#ifdef IMGUI_VERSION
  // ImGui がマウスを使うときは Window クラスのマウス位置を更新しない
  if (ImGui::GetIO().WantCaptureMouse) return;
#endif

  // このインスタンスの this ポインタを得る
  Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

  if (instance)
  {
    // マウスの現在位置を取り出す
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    switch (button)
    {
    case GLFW_MOUSE_BUTTON_1:

      // 左ボタンを押した時の処理
      if (action)
      {
        // ドラッグ開始位置を保存する
        instance->cx = x;
        instance->cy = y;
      }
      break;

    case GLFW_MOUSE_BUTTON_2:

      // 右ボタンを押した時の処理
      if (action)
      {
        // トラックボール処理開始
        attitude.orientation.begin(static_cast<float>(x), static_cast<float>(y));
      }
      else
      {
        // トラックボール処理終了
        attitude.orientation.end(static_cast<float>(x), static_cast<float>(y));
      }
      break;

    case GLFW_MOUSE_BUTTON_3:

      // 中ボタンを押した時の処理
      break;

    default:
      break;
    }
  }
}

//
// マウスホイール操作時の処理
//
//   ・マウスホイールを操作した時にコールバック関数として呼び出される
//
void Window::wheel(GLFWwindow *window, double x, double y)
{
#ifdef IMGUI_VERSION
  // ImGui がマウスを使うときは Window クラスのマウスホイールの回転量を更新しない
  if (ImGui::GetIO().WantCaptureMouse) return;
#endif

  // このインスタンスの this ポインタを得る
  Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

  if (instance)
  {
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT))
    {
      // ズーム率を調整する
      attitude.foreAdjust[0] += static_cast<int>(y);

      // 透視投影変換行列を更新する
      instance->update();
    }
    else
    {
      // 物体を前後に移動する
      const GLfloat advSpeed((fabs(attitude.position[2]) * 5.0f + 1.0f) * wheelYStep * static_cast<GLfloat>(y));
      attitude.position[2] += advSpeed;
    }
  }
}

//
// キーボードをタイプした時の処理
//
//   ．キーボードをタイプした時にコールバック関数として呼び出される
//
void Window::keyboard(GLFWwindow *window, int key, int scancode, int action, int mods)
{
#ifdef IMGUI_VERSION
  // ImGui がキーボードを使うときはキーボードの処理を行わない
  if (ImGui::GetIO().WantCaptureKeyboard) return;
#endif

  // このインスタンスの this ポインタを得る
  Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

  if (instance)
  {
    if (action != GLFW_RELEASE)
    {
      // シフトキーの状態
      const auto shiftKey(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)
        || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT));

      // コントロールキーの状態
      const auto ctrlKey(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)
        || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL));

      // オルタネートキーの状態
      const auto altKey(glfwGetKey(window, GLFW_KEY_LEFT_ALT)
        || glfwGetKey(window, GLFW_KEY_RIGHT_ALT));

      // 最後にタイプしたキーを覚えておく
      instance->key = key;

      // キーボード操作による処理
      switch (key)
      {
      case GLFW_KEY_R:

        // 設定をリセットする
        instance->reset();
        break;

      case GLFW_KEY_M:

        // メニュー表示を ON/OFF する
        instance->showMenu = !instance->showMenu;
        break;

      case GLFW_KEY_Z:

        // ズーム率を調整する
        if (shiftKey)
          --attitude.foreAdjust[0];
        else
          ++attitude.foreAdjust[0];

        // 透視投影変換行列を更新する
        instance->update();
        break;

      case GLFW_KEY_P:

        // HMD のときは視差を調整しない
        if (oculus) break;

        // 視差を調整する
        if (shiftKey)
          --attitude.parallax;
        else
          ++attitude.parallax;

        // 透視投影変換行列を更新する
        instance->update();
        break;

      case GLFW_KEY_E:

        // カメラが有効でなければ露出を調整しない
        if (!instance->camera) break;

        // 露出を調整する
        if (shiftKey)
          instance->camera->decreaseExposure();
        else
          instance->camera->increaseExposure();
        break;

      case GLFW_KEY_G:

        // カメラが有効でなければ露出を調整しない
        if (!instance->camera) break;

        // 利得を調整する
        if (shiftKey)
          instance->camera->decreaseGain();
        else
          instance->camera->increaseGain();
        break;

      case GLFW_KEY_UP:

        if (ctrlKey)
        {
          // 背景に対する縦方向の画角を広げる
          ++attitude.circleAdjust[1];
        }
        else if (shiftKey)
        {
          // 背景を上にずらす
          ++attitude.circleAdjust[3];
        }
        else if (altKey)
        {
          // 背景を上に回転する
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[1];
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[1].conjugate();
        }
        else
        {
          // 背景に対する焦点距離を延ばす
          ++attitude.backAdjust[0];
        }

        // カメラの画角と中心位置を更新する
        instance->updateCircle();
        break;

      case GLFW_KEY_DOWN:

        if (ctrlKey)
        {
          // 背景に対する縦方向の画角を狭める
          --attitude.circleAdjust[1];
        }
        else if (shiftKey)
        {
          // 背景を下にずらす
          --attitude.circleAdjust[3];
        }
        else if (altKey)
        {
          // 背景を下に回転する
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[1].conjugate();
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[1];
        }
        else
        {
          // 背景に対する焦点距離を縮める
          --attitude.backAdjust[0];
        }

        // カメラの画角と中心位置を更新する
        instance->updateCircle();
        break;

      case GLFW_KEY_RIGHT:

        if (ctrlKey)
        {
          // 背景に対する横方向の画角を広げる
          ++attitude.circleAdjust[0];
        }
        else if (shiftKey)
        {
          // 背景を右にずらす
          ++attitude.circleAdjust[2];
        }
        else if (altKey)
        {
          // 背景を右に回転する
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[0].conjugate();
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[0];
        }
        else if (defaults.display_mode != MONOCULAR)
        {
          // スクリーンの間隔を拡大する
          ++attitude.offset;
        }

        // カメラの画角と中心位置を更新する
        instance->updateCircle();
        break;

      case GLFW_KEY_LEFT:

        if (ctrlKey)
        {
          // 背景に対する横方向の画角を狭める
          --attitude.circleAdjust[0];
        }
        else if (shiftKey)
        {
          // 背景を左にずらす
          --attitude.circleAdjust[2];
        }
        else if (altKey)
        {
          // 背景を左に回転する
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[0];
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[0].conjugate();
        }
        else if (defaults.display_mode != MONOCULAR)
        {
          // 視差を縮小する
          --attitude.offset;
        }

        // カメラの画角と中心位置を更新する
        instance->updateCircle();
        break;

      case GLFW_KEY_ESCAPE:
        break;

      case GLFW_KEY_BACKSPACE:
      case GLFW_KEY_DELETE:
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
void Window::reset()
{
  // 物体の位置
  attitude.position = attitude.initialPosition;

  // 物体の回転
  attitude.orientation.reset(attitude.initialOrientation.getQuaternion());

  // カメラごとの姿勢の補正値
  for (int cam = 0; cam < camCount; ++cam) attitude.eyeOrientation[cam] = attitude.initialEyeOrientation[cam];

  // 前景に対する焦点距離と中心位置の補正値
  attitude.foreAdjust = attitude.initialForeAdjust;

  // 背景に対する焦点距離と中心位置の補正値
  attitude.backAdjust = attitude.initialBackAdjust;

  // 背景の視差
  attitude.parallax = attitude.initialParallax;

  // 背景のスクリーンの間隔
  attitude.offset = attitude.initialOffset;

  // 透視投影変換行列を更新する
  update();

  // 背景テクスチャの半径と中心位置の補正値
  attitude.circleAdjust = attitude.initialCircleAdjust;

  // カメラの画角と中心位置を更新する
  updateCircle();
}

//
// Oculus Rift を起動する
//
bool Window::startOculus()
{
  // Oculus Rift のコンテキストを作る
  oculus = Oculus::initialize(zoom, &aspect, mp, screen);

  // 作れなかったらエラー
  if (oculus == nullptr) return false;

  // Oculus Rift の起動に成功した
  return true;
}

//
// Oculus Rift を停止する
//
void Window::stopOculus()
{
  if (oculus) oculus->terminate();
}

//
// 描画する目を選択する
//
void Window::select(int eye)
{
  switch (defaults.display_mode)
  {
  case MONOCULAR:

    // ウィンドウ全体をビューポートにする
    glViewport(0, 0, width, height);

    // デプスバッファを消去する
    glClear(GL_DEPTH_BUFFER_BIT);

    break;

  case TOP_AND_BOTTOM:

    if (eye == camL)
    {
      // ディスプレイの上半分だけに描画する
      glViewport(0, height, width, height);

      // デプスバッファを消去する
      glClear(GL_DEPTH_BUFFER_BIT);
    }
    else
    {
      // ディスプレイの下半分だけに描画する
      glViewport(0, 0, width, height);
    }
    break;

  case INTERLACE:
  case SIDE_BY_SIDE:

    if (eye == camL)
    {
      // ディスプレイの左半分だけに描画する
      glViewport(0, 0, width, height);

      // デプスバッファを消去する
      glClear(GL_DEPTH_BUFFER_BIT);
    }
    else
    {
      // ディスプレイの右半分だけに描画する
      glViewport(width, 0, width, height);
    }
    break;

  case QUADBUFFER:

    // 左右のバッファに描画する
    glDrawBuffer(eye == camL ? GL_BACK_LEFT : GL_BACK_RIGHT);

    // デプスバッファを消去する
    glClear(GL_DEPTH_BUFFER_BIT);
    break;

  case OCULUS:

    if (oculus)
    {
      // 描画する Oculus Rift の目を選択する
      oculus->select(eye, po[eye], qo[eye]);

      // 四元数から変換行列を求める
      mo[eye] = qo[eye].getMatrix();

      // ヘッドトラッキングの変換行列を共有メモリに保存する
      Scene::setLocalAttitude(eye, mo[eye].transpose());

      // mv は変更しないので戻る
      return;
    }

  default:
    break;
  }

  // 目をずらす代わりに前景を動かす
  mv[eye] = ggTranslate(eye == camL ? parallax : -parallax, 0.0f, 0.0f);
}

//
// 描画を開始する
//
bool Window::start()
{
  // Oculus Rift を使っていなかったら関係ない
  if (!oculus) return true;

  // モデル変換行列を設定する
  mm = ggTranslate(attitude.position) * attitude.orientation.getMatrix();

  // モデル変換行列を共有メモリに保存する
  Scene::setup(mm);

  // Oculus Rift の描画を開始する
  auto status{ oculus->start(mv) };

  // Oculus Rift に表示可能なら true
  if (status == Oculus::VISIBLE) return true;

  // 終了ようきゅが出ていればウィンドウを閉じる
  if (status == Oculus::WANTQUIT) setClose();

  // Oculus Rift に表示しない
  return false;
}

//
// 透視投影変換行列を更新する
//
//   ・ウィンドウのサイズ変更時やカメラパラメータの変更時に呼び出す
//
void Window::update()
{
  // 前景のズーム率を更新する
  zoom = 1.0f / (1.0f - zoomStep * attitude.foreAdjust[0]);

  // 背景の焦点距離を更新する
  focal = 1.0f / (1.0f - backFocalStep * attitude.backAdjust[0]);

  // 背景の視差を更新する
  parallax = defaultParallax + parallaxStep * attitude.parallax;

  // 背景のスクリーンの間隔を更新する
  offset = offsetDefault + offsetStep * attitude.offset;

  // Oculus Rift 使用時は透視投影変換行列だけ更新する
  if (oculus)
  {
    oculus->getPerspective(zoom, mp);
    return;
  }

  // スクリーンの高さと幅
  const GLfloat screenHeight{ defaults.display_center / defaults.display_distance };
  const GLfloat screenWidth{ screenHeight * aspect };

  // 視差によるスクリーンのシフト量
  const GLfloat shift{ defaults.display_mode != MONOCULAR
    ? parallax * defaults.display_near / defaults.display_distance : 0.0f };

  // 前方面が defaults.display_near なのでスクリーンもそれに合わせる
  const GLfloat zf{ defaults.display_near / zoom };

  // 左目の視野
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

  // 左目のスクリーンのサイズと中心位置
  screen[0][0] = (fovL[1] - fovL[0]) * 0.5f;
  screen[0][1] = (fovL[3] - fovL[2]) * 0.5f;
  screen[0][2] = (fovL[1] + fovL[0]) * 0.5f;
  screen[0][3] = (fovL[3] + fovL[2]) * 0.5f;

  // Oculus Rift 以外の立体視表示の場合
  if (defaults.display_mode != MONOCULAR)
  {
    // 右目の視野
    const GLfloat fovR[] =
    {
      -screenWidth - shift,
      screenWidth - shift,
      -screenHeight,
      screenHeight,
    };

    // 右の透視投影変換行列を求める
    mp[1].loadFrustum(fovR[0] * zf, fovR[1] * zf, fovR[2] * zf, fovR[3] * zf,
      defaults.display_near, defaults.display_far);

    // 右目のスクリーンのサイズと中心位置
    screen[1][0] = (fovR[1] - fovR[0]) * 0.5f;
    screen[1][1] = (fovR[3] - fovR[2]) * 0.5f;
    screen[1][2] = (fovR[1] + fovR[0]) * 0.5f;
    screen[1][3] = (fovR[3] + fovR[2]) * 0.5f;
  }
}

//
// カメラの画角と中心位置を更新する
//
void Window::updateCircle()
{
  // 背景テクスチャの半径と中心位置
  circle[0] = defaults.camera_fov_x + fovStep * attitude.circleAdjust[0];
  circle[1] = defaults.camera_fov_y + fovStep * attitude.circleAdjust[1];
  circle[2] = defaults.camera_center_x + shiftStep * attitude.circleAdjust[2];
  circle[3] = defaults.camera_center_y + shiftStep * attitude.circleAdjust[3];
}

// 描画を完了する
void Window::commit(int eye)
{
  if (oculus) oculus->commit(eye);
}

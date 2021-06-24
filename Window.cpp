//
// ウィンドウ関連の処理
//
#include "Window.h"

<<<<<<< HEAD
// Oculus Rift SDK ライブラリ (LibOVR) の組み込み
#if defined(_WIN32)
// コンフィギュレーションを調べる
#  if defined(_DEBUG)
// デバッグビルドのライブラリをリンクする
#    pragma comment(lib, "libOVRd.lib")
#  else
// リリースビルドのライブラリをリンクする
#    pragma comment(lib, "libOVR.lib")
#  endif
#endif

// Dear ImGui を使うとき
#ifdef IMGUI_VERSION
#  include "imgui_impl_glfw.h"
#  include "imgui_impl_opengl3.h"
#endif
=======
// Oculus Rift 関連の処理
#include "Oculus.h"
>>>>>>> 3fa99b2 (Separate oculus classes)

// シーングラフ
#include "Scene.h"

// 標準ライブラリ
#include <fstream>

// Dear ImGui
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// ジョイスティック番号の最大値
constexpr int maxJoystick{ 4 };

//
// コンストラクタ
//
Window::Window(int width, int height, const char *title, GLFWmonitor *monitor, GLFWwindow *share)
  : window{ nullptr }
  , camera{ nullptr }
  , showScene{ true }
  , showMirror{ true }
  , startPosition{ 0.0f, 0.0f, 0.0f }
  , startOrientation{ 0.0f, 0.0f, 0.0f, 1.0f }
  , initialOffset{ defaultOffset }
  , initialParallax{ defaultParallax }
  , zoomChange{ 0 }
  , focalChange{ 0 }
  , circleChange{ 0, 0, 0, 0 }
  , key{ GLFW_KEY_UNKNOWN }
  , joy{ -1 }
{
  // 最初のインスタンスで一度だけ実行
  static bool firstTime{ true };
  if (firstTime)
  {
    // カメラ方向の調整ステップを求める
    qrStep[0].loadRotate(0.0f, 1.0f, 0.0f, 0.001f);
    qrStep[1].loadRotate(1.0f, 0.0f, 0.0f, 0.001f);

    // クワッドバッファステレオモードにする
    if (defaults.display_mode == QUADBUFFER)
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

  // カメラの補正値を初期化する
  for (auto &q : qa) q = ggIdentityQuaternion();

  // 初期設定を読み込む
  std::ifstream attitude("attitude.json");
  if (attitude)
  {
    // 設定内容の読み込み
    picojson::value v;
    attitude >> v;
    attitude.close();

    // 設定内容のパース
    picojson::object &o(v.get<picojson::object>());

    // 初期位置
    const auto &v_position(o.find("position"));
    if (v_position != o.end() && v_position->second.is<picojson::array>())
    {
      picojson::array &p(v_position->second.get<picojson::array>());
      for (int i = 0; i < 3; ++i) startPosition[i] = static_cast<GLfloat>(p[i].get<double>());
    }

    // 初期姿勢
    const auto &v_orientation(o.find("orientation"));
    if (v_orientation != o.end() && v_orientation->second.is<picojson::array>())
    {
      picojson::array &a(v_orientation->second.get<picojson::array>());
      for (int i = 0; i < 4; ++i) startOrientation[i] = static_cast<GLfloat>(a[i].get<double>());
    }

    // 初期ズーム率
    const auto &v_zoom(o.find("zoom"));
    if (v_zoom != o.end() && v_zoom->second.is<double>())
      zoomChange = static_cast<int>(v_zoom->second.get<double>());

    // 焦点距離の初期値
    const auto &v_focal(o.find("focal"));
    if (v_focal != o.end() && v_focal->second.is<double>())
      focalChange = static_cast<int>(v_focal->second.get<double>());

    // 背景テクスチャの半径と中心位置の初期値
    const auto &v_circle(o.find("circle"));
    if (v_circle != o.end() && v_circle->second.is<picojson::array>())
    {
      picojson::array &c(v_circle->second.get<picojson::array>());
      for (int i = 0; i < 4; ++i) circleChange[i] = static_cast<int>(c[i].get<double>());
    }

    // スクリーンの間隔の初期値
    const auto &v_offset(o.find("offset"));
    if (v_offset != o.end() && v_offset->second.is<double>())
      initialOffset = static_cast<GLfloat>(v_offset->second.get<double>());

    // 視差の初期値
    const auto &v_parallax(o.find("parallax"));
    if (v_parallax != o.end() && v_parallax->second.is<double>())
      initialParallax = static_cast<GLfloat>(v_parallax->second.get<double>());

    // カメラ方向の補正値
    const auto &v_parallax_offset(o.find("parallax_offset"));
    if (v_parallax_offset != o.end() && v_parallax_offset->second.is<picojson::array>())
    {
      picojson::array &a(v_parallax_offset->second.get<picojson::array>());
      for (int eye = 0; eye < camCount; ++eye)
      {
        GLfloat q[4];
        for (int i = 0; i < 4; ++i) q[i] = static_cast<GLfloat>(a[eye * 4 + i].get<double>());
        qa[eye] = GgQuaternion(q);
      }
    }
  }

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
  //ImGuiIO& io = ImGui::GetIO(); (void)io;
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  //ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
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
  //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
  //IM_ASSERT(font != NULL);
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

  // 設定値の保存先
  std::ofstream attitude("attitude.json");

  // 設定値を保存する
  if (attitude)
  {
    picojson::object o;

    // 位置
    picojson::array p(3);
    p[0] = picojson::value(ox);
    p[1] = picojson::value(oy);
    p[2] = picojson::value(oz);
    o.insert(std::make_pair("position", picojson::value(p)));

    // 姿勢
    picojson::array a;
    for (int i = 0; i < 4; ++i) a.push_back(picojson::value(trackball.getQuaternion().get()[i]));
    o.insert(std::make_pair("orientation", picojson::value(a)));

    // ズーム率
    o.insert(std::make_pair("zoom", picojson::value(static_cast<double>(zoomChange))));

    // 焦点距離
    o.insert(std::make_pair("focal", picojson::value(static_cast<double>(focalChange))));

    // 背景テクスチャの半径と中心位置
    picojson::array c;
    for (int i = 0; i < 4; ++i) c.push_back(picojson::value(static_cast<double>(circleChange[i])));
    o.insert(std::make_pair("circle", picojson::value(c)));

    // スクリーンの間隔
    o.insert(std::make_pair("offset", picojson::value(static_cast<double>(initialOffset))));

    // 視差
    o.insert(std::make_pair("parallax", picojson::value(static_cast<double>(initialParallax))));

    // カメラ方向の補正値
    picojson::array q;
    for (int eye = 0; eye < camCount; ++eye)
      for (int i = 0; i < 4; ++i) q.push_back(picojson::value(static_cast<double>(qa[eye].get()[i])));
    o.insert(std::make_pair("parallax_offset", picojson::value(q)));

    // 設定内容をシリアライズして保存
    picojson::value v(o);
    attitude << v.serialize(true);
    attitude.close();
  }

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
  const auto speedFactor((fabs(oz) + 0.2f));

  //
  // ジョイスティックによる操作
  //

  // ジョイスティックが有効なら
  if (joy >= 0)
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
      ox += (axes[0] - origin[0]) * axesSpeedFactor;

      // L ボタンか R ボタンを同時に押していれば
      if (lrButton)
      {
        // 物体を前後に移動する
        oz += (axes[1] - origin[1]) * axesSpeedFactor;
      }
      else
      {
        // 物体を上下に移動する
        oy += (axes[1] - origin[1]) * axesSpeedFactor;
      }

      // 物体を左右に回転する
      trackball.rotate(ggRotateQuaternion(0.0f, 1.0f, 0.0f, (axes[2] - origin[2]) * axesSpeedFactor));

      // 物体を上下に回転する
      trackball.rotate(ggRotateQuaternion(1.0f, 0.0f, 0.0f, (axes[3] - origin[3]) * axesSpeedFactor));
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
          qa[camL] *= qrStep[0];
          qa[camR] *= qrStep[0].conjugate();
        }
        else
        {
          qa[camL] *= qrStep[0].conjugate();
          qa[camR] *= qrStep[0];
        }
      }
      else
      {
        // スクリーンの間隔を調整する
        offset += parallaxButton * offsetStep;
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
          qa[camL] *= qrStep[1];
          qa[camR] *= qrStep[1].conjugate();
        }
        else
        {
          qa[camL] *= qrStep[1].conjugate();
          qa[camR] *= qrStep[1];
        }
      }
      else
      {
        // 焦点距離を調整する
        focal = focalStep / (focalStep - static_cast<GLfloat>(focalChange += zoomButton));
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
        circle[0] = defaults.fisheye_fov_x + static_cast<GLfloat>(circleChange[0] += textureXButton) * shiftStep;
      }
      else
      {
        // 背景の横位置を調整する
        circle[2] = defaults.fisheye_center_x + static_cast<GLfloat>(circleChange[2] += textureXButton) * shiftStep;
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
        circle[1] = defaults.fisheye_fov_y + static_cast<GLfloat>(circleChange[1] += textureYButton) * shiftStep;
      }
      else
      {
        // 背景の縦位置を調整する
        circle[3] = defaults.fisheye_center_y + static_cast<GLfloat>(circleChange[3] += textureYButton) * shiftStep;
      }
    }

    // 設定をリセットする
    if (btns[7])
    {
      reset();
      update();
    }
  }

  //
  // マウスによる操作
  //

  // マウスの位置
  double x, y;

#ifdef IMGUI_VERSION

  // ImGui の新規フレームを作成する
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();

  // ImGui がマウスを使うときは Window クラスのマウス位置を更新しない
  if (ImGui::GetIO().WantCaptureMouse) return true;

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
    ox += static_cast<GLfloat>(x - cx) * speed;
    oy += static_cast<GLfloat>(cy - y) * speed;
    cx = x;
    cy = y;
  }

  // 右ボタンドラッグ
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2))
  {
    // 物体を回転する
    trackball.motion(float(x), float(y));
  }

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
      if (!ctrlKey) qa[camL] *= qrStep[0].conjugate();
      if (!shiftKey) qa[camR] *= qrStep[0].conjugate();
    }
    else if (ctrlKey)
    {
      // 背景に対する横方向の画角を広げる
      circle[0] = defaults.fisheye_fov_x + static_cast<GLfloat>(++circleChange[0]) * shiftStep;
    }
    else if (shiftKey)
    {
      // 背景を右にずらす
      circle[2] = defaults.fisheye_center_x + static_cast<GLfloat>(++circleChange[2]) * shiftStep;
    }
    else if (defaults.display_mode != MONOCULAR)
    {
      // スクリーンの間隔を拡大する
      offset += offsetStep;
    }
  }

  // 左矢印キー操作
  if (glfwGetKey(window, GLFW_KEY_LEFT))
  {
    if (altKey)
    {
      // 背景を左に回転する
      if (!ctrlKey) qa[camL] *= qrStep[0];
      if (!shiftKey) qa[camR] *= qrStep[0];
    }
    else if (ctrlKey)
    {
      // 背景に対する横方向の画角を狭める
      circle[0] = defaults.fisheye_fov_x + static_cast<GLfloat>(--circleChange[0]) * shiftStep;
    }
    else if (shiftKey)
    {
      // 背景を左にずらす
      circle[2] = defaults.fisheye_center_x + static_cast<GLfloat>(--circleChange[2]) * shiftStep;
    }
    else if (defaults.display_mode != MONOCULAR)
    {
      // 視差を縮小する
      offset -= offsetStep;
    }
  }

  // 上矢印キー操作
  if (glfwGetKey(window, GLFW_KEY_UP))
  {
    if (altKey)
    {
      // 背景を上に回転する
      if (!ctrlKey) qa[camL] *= qrStep[1];
      if (!shiftKey) qa[camR] *= qrStep[1];
    }
    else if (ctrlKey)
    {
      // 背景に対する縦方向の画角を広げる
      circle[1] = defaults.fisheye_fov_y + static_cast<GLfloat>(++circleChange[1]) * shiftStep;
    }
    else if (shiftKey)
    {
      // 背景を上にずらす
      circle[3] = defaults.fisheye_center_y + static_cast<GLfloat>(++circleChange[3]) * shiftStep;
    }
    else
    {
      // 焦点距離を延ばす
      focal = focalStep / (focalStep - static_cast<GLfloat>(++focalChange));
    }
  }

  // 下矢印キー操作
  if (glfwGetKey(window, GLFW_KEY_DOWN))
  {
    if (altKey)
    {
      // 背景を下に回転する
      if (!ctrlKey) qa[camL] *= qrStep[1].conjugate();
      if (!shiftKey) qa[camR] *= qrStep[1].conjugate();
    }
    else if (ctrlKey)
    {
      // 背景に対する縦方向の画角を狭める
      circle[1] = defaults.fisheye_fov_y + static_cast<GLfloat>(--circleChange[1]) * shiftStep;
    }
    else if (shiftKey)
    {
      // 背景を下にずらす
      circle[3] = defaults.fisheye_center_y + static_cast<GLfloat>(--circleChange[3]) * shiftStep;
    }
    else
    {
      // 焦点距離を縮める
      focal = focalStep / (focalStep - static_cast<GLfloat>(--focalChange));
    }
  }

  // 'P' キーの操作
  if (glfwGetKey(window, GLFW_KEY_P))
  {
    // 視差を調整する
    parallax += shiftKey ? -parallaxStep : parallaxStep;

    // 透視投影変換行列を更新する
    updateProjectionMatrix();
  }

  // 'Z' キーの操作
  if (glfwGetKey(window, GLFW_KEY_Z))
  {
    // ズーム率を調整する
    if (shiftKey) ++zoomChange; else --zoomChange;
    zoom = (defaults.display_zoom != 0.0f ? 1.0f / defaults.display_zoom : 1.0f)
      + zoomChange * zoomStep;

    // 透視投影変換行列を更新する
    updateProjectionMatrix();
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
    oculus->submit(showMirror, width, height);

#ifdef IMGUI_VERSION
    // ユーザインタフェースを描画する
    ImDrawData *const imDrawData(ImGui::GetDrawData());
    if (imDrawData) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
#endif

    // 残っている OpenGL コマンドを実行する
    glFlush();
  }
  else
  {
#ifdef IMGUI_VERSION
    // ユーザインタフェースを描画する
    ImDrawData *const imDrawData(ImGui::GetDrawData());
    if (imDrawData) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
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
    // Oculus Rift 使用時以外
    if (!instance->oculus)
    {
      if (defaults.display_mode == LINE_BY_LINE)
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
    instance->trackball.region(static_cast<float>(width) / angleScale, static_cast<float>(height) / angleScale);
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
        instance->trackball.begin(float(x), float(y));
      }
      else
      {
        // トラックボール処理終了
        instance->trackball.end(float(x), float(y));
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
      instance->zoom = (defaults.display_zoom != 0.0f ? 1.0f / defaults.display_zoom : 1.0f)
        + static_cast<GLfloat>(instance->zoomChange += static_cast<int>(y)) * zoomStep;

      // 透視投影変換行列を更新する
      instance->update();
    }
    else
    {
      // 物体を前後に移動する
      const GLfloat advSpeed((fabs(instance->oz) * 5.0f + 1.0f) * wheelYStep * static_cast<GLfloat>(y));
      instance->oz += advSpeed;
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
    if (action == GLFW_PRESS)
    {
      // 最後にタイプしたキーを覚えておく
      instance->key = key;

      // キーボード操作による処理
      switch (key)
      {
      case GLFW_KEY_R:

        // 設定をリセットする
        instance->reset();
        instance->update();
        break;

      case GLFW_KEY_SPACE:

        // シーンの表示を ON/OFF する
        instance->showScene = !instance->showScene;
        break;

      case GLFW_KEY_M:

        // ミラー表示を ON/OFF する
        instance->showMirror = !instance->showMirror;
        break;

      case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
 
      case GLFW_KEY_BACKSPACE:
      case GLFW_KEY_DELETE:
        break;

      case GLFW_KEY_UP:
        break;

      case GLFW_KEY_DOWN:
        break;

      case GLFW_KEY_RIGHT:
        break;

      case GLFW_KEY_LEFT:
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
  ox = startPosition[0];
  oy = startPosition[1];
  oz = startPosition[2];

  // 物体の回転
  trackball.reset();
  trackball.rotate(ggQuaternion(startOrientation));

  // ズーム率
  zoom = (defaults.display_zoom != 0.0f ? 1.0f / defaults.display_zoom : 1.0f)
    + static_cast<GLfloat>(zoomChange) * zoomStep;

  // 焦点距離
  focal = focalStep / (focalStep - static_cast<GLfloat>(focalChange));

  // 背景テクスチャの半径と中心位置
  circle[0] = defaults.fisheye_fov_x + static_cast<GLfloat>(circleChange[0]) * shiftStep;
  circle[1] = defaults.fisheye_fov_y + static_cast<GLfloat>(circleChange[1]) * shiftStep;
  circle[2] = defaults.fisheye_center_x + static_cast<GLfloat>(circleChange[2]) * shiftStep;
  circle[3] = defaults.fisheye_center_y + static_cast<GLfloat>(circleChange[3]) * shiftStep;

  // スクリーンの間隔
  offset = initialOffset;

  // 視差
  parallax = defaults.display_mode != MONOCULAR ? initialParallax : 0.0f;
}

//
// Oculus Rift を起動する
//
bool Window::startOculus()
{
  return Oculus::initialize(*this);
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

  case LINE_BY_LINE:
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

  // 目をずらす代わりにシーンを動かす
  mv[eye] = ggTranslate(eye == camL ? parallax : -parallax, 0.0f, 0.0f);
}

//
// 描画を開始する
//
bool Window::start()
{
  if (!oculus) return true;

  // モデル変換行列を設定する
  mm = ggTranslate(ox, oy, -oz) * trackball.getMatrix();

  // モデル変換行列を共有メモリに保存する
  Scene::setup(mm);

  // Oculus Rift の描画を開始する
  return oculus->start(mv);
}

//
// 透視投影変換行列を更新する
//
//   ・ウィンドウのサイズ変更時やカメラパラメータの変更時に呼び出す
//
void Window::update()
{
  // Oculus Rift 使用時は更新しない
  if (oculus) return;

  // ズーム率
  const auto zf(zoom * defaults.display_near);

  // スクリーンの高さと幅
  const auto screenHeight(defaults.display_center / defaults.display_distance);
  const auto screenWidth(screenHeight * aspect);

  // 視差によるスクリーンのシフト量
  GLfloat shift(parallax * defaults.display_near / defaults.display_distance);

  // 左目の視野
  const GLfloat fovL[] =
  {
    -screenWidth + shift,
    screenWidth + shift,
    -screenHeight,
    screenHeight
  };

  // 左目の透視投影変換行列を求める
  mp[ovrEye_Left].loadFrustum(fovL[0] * zf, fovL[1] * zf, fovL[2] * zf, fovL[3] * zf,
    defaults.display_near, defaults.display_far);

  // 左目のスクリーンのサイズと中心位置
  screen[ovrEye_Left][0] = (fovL[1] - fovL[0]) * 0.5f;
  screen[ovrEye_Left][1] = (fovL[3] - fovL[2]) * 0.5f;
  screen[ovrEye_Left][2] = (fovL[1] + fovL[0]) * 0.5f;
  screen[ovrEye_Left][3] = (fovL[3] + fovL[2]) * 0.5f;

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
    mp[ovrEye_Right].loadFrustum(fovR[0] * zf, fovR[1] * zf, fovR[2] * zf, fovR[3] * zf,
      defaults.display_near, defaults.display_far);

    // 右目のスクリーンのサイズと中心位置
    screen[ovrEye_Right][0] = (fovR[1] - fovR[0]) * 0.5f;
    screen[ovrEye_Right][1] = (fovR[3] - fovR[2]) * 0.5f;
    screen[ovrEye_Right][2] = (fovR[1] + fovR[0]) * 0.5f;
    screen[ovrEye_Right][3] = (fovR[3] + fovR[2]) * 0.5f;
  }
}

// 描画を完了する
void Window::commit(int eye)
{
  if (oculus) oculus->commit(eye);
}

// カメラ方向の調整ステップ
GgQuaternion Window::qrStep[2];

// Oculus Rift のセッション
Oculus* Window::oculus{ nullptr };

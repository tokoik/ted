//
// ウィンドウ関連の処理
//
#include "Window.h"

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
#if defined(IMGUI_VERSION)
#  include "imgui_impl_glfw.h"
#  include "imgui_impl_opengl3.h"
#endif

// シーングラフ
#include "Scene.h"

// 標準ライブラリ
#include <fstream>

// ジョイスティック番号の最大値
constexpr int maxJoystick{ 4 };

#if OVR_PRODUCT_VERSION > 0
// GetDefaultAdapterLuid のため
#  if defined(_WIN32)
#    include <dxgi.h>
#    pragma comment(lib, "dxgi.lib")
#  endif

static ovrGraphicsLuid GetDefaultAdapterLuid()
{
  ovrGraphicsLuid luid = ovrGraphicsLuid();

#  if defined(_WIN32)
  IDXGIFactory* factory = nullptr;

  if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&factory))))
  {
    IDXGIAdapter* adapter = nullptr;

    if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
    {
      DXGI_ADAPTER_DESC desc;

      adapter->GetDesc(&desc);
      memcpy(&luid, &desc.AdapterLuid, sizeof(luid));
      adapter->Release();
    }

    factory->Release();
  }
#  endif

  return luid;
}

static int Compare(const ovrGraphicsLuid& lhs, const ovrGraphicsLuid& rhs)
{
  return memcmp(&lhs, &rhs, sizeof(ovrGraphicsLuid));
}
#endif

//
// コンストラクタ
//
Window::Window(Config& defaults, GLFWwindow* share)
  : defaults{ defaults }
  , config{ defaults }
  , window{ nullptr }
  , camera{ nullptr }
  , session{ nullptr }
  , showScene{ true }
  , showMirror{ true }
  , showMenu{ true }
  , oculusFbo{ 0, 0 }
  , mirrorFbo{ 0 }
  , mirrorTexture{ nullptr }
  , zoomChange{ 0 }
  , focalChange{ 0 }
  , circleChange{ 0, 0, 0, 0 }
  , key{ GLFW_KEY_UNKNOWN }
  , joy{ -1 }
#if OVR_PRODUCT_VERSION > 0
  , frameIndex{ 0LL }
  , oculusDepth{ 0, 0 }
#endif
{
  GLFWmonitor* monitor{ nullptr };

  // フルスクリーン表示
  if (config.display_fullscreen)
  {
    // 接続されているモニタの数を数える
    int mcount;
    GLFWmonitor** const monitors(glfwGetMonitors(&mcount));

    // モニタの存在チェック
    if (mcount == 0)
    {
      NOTIFY("GLFW の初期化をしていないか表示可能なディスプレイが見つかりません。");
      exit(EXIT_FAILURE);
    }

    // セカンダリモニタがあればそれを使う
    monitor = monitors[mcount > config.display_secondary ? config.display_secondary : 0];

    // モニタのモードを調べる
    const GLFWvidmode* mode(glfwGetVideoMode(monitor));

    // ウィンドウのサイズをディスプレイのサイズにする
    width = mode->width;
    height = mode->height;
  }
  else
  {
    // プライマリモニタをウィンドウモードで使う
    monitor = nullptr;

    // ウィンドウのサイズにデフォルト値を設定する
    width = config.display_width;
    height = config.display_height;
  }

  // 最初のインスタンスのときだけ true
  static bool firstTime(true);

  // 最初のインスタンスで一度だけ実行
  if (firstTime)
  {
    // カメラ方向の調整ステップを求める
    qrStep[0].loadRotate(0.0f, 1.0f, 0.0f, parallaxStep);
    qrStep[1].loadRotate(1.0f, 0.0f, 0.0f, parallaxStep);

    // Oculus Rift に表示するとき
    if (config.display_mode == OCULUS)
    {
      // Oculus Rift (LibOVR) を初期化する
      ovrInitParams initParams = { ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0 };
      if (OVR_FAILURE(ovr_Initialize(&initParams)))
      {
        // LibOVR の初期化に失敗した
        NOTIFY("LibOVR が初期化できません。");
        return;
      }

      // プログラム終了時には LibOVR を終了する
      atexit(ovr_Shutdown);

      // Oculus Rift のセッションを作成する
      ovrGraphicsLuid luid;
      if (OVR_FAILURE(ovr_Create(&const_cast<ovrSession>(session), &luid)))
      {
        // Oculus Rift のセッションの作成に失敗した
        NOTIFY("Oculus Rift のセッションが作成できません。");
        return;
      }

#if OVR_PRODUCT_VERSION > 0
      // デフォルトのグラフィックスアダプタが使われているか確かめる
      if (Compare(luid, GetDefaultAdapterLuid()))
      {
        // デフォルトのグラフィックスアダプタが使用されていない
        NOTIFY("OpenGL ではデフォルトのグラフィックスアダプタ以外使用できません。");
      }
#else
      // (LUID は OpenGL では使っていないらしい)
#endif

      // Oculus ではダブルバッファモードにしない
      glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
    }
    else if (config.display_mode == QUADBUFFER)
    {
      // クワッドバッファステレオモードにする
      glfwWindowHint(GLFW_STEREO, GLFW_TRUE);
    }

    // SRGB モードでレンダリングする
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

  // GLFW のウィンドウを開く
  const_cast<GLFWwindow*>(window) = glfwCreateWindow(width, height, windowTitle, monitor, share);

  // ウィンドウが開かれたかどうか確かめる
  if (!window)
  {
    // ウィンドウが開かれなかった
    NOTIFY("GLFW のウィンドウが作成できません。");
    return;
  }

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

  // Oculus Rift への表示ではスワップ間隔を待たない
  glfwSwapInterval(config.display_mode == OCULUS ? 0 : 1);

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

  // sRGB カラースペースを使う
  glEnable(GL_FRAMEBUFFER_SRGB);

  // 背景色
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  //
  // Oculus Rift の設定
  //

  // Oculus Rift に表示するとき
  if (session)
  {
    // Oculus Rift の情報を取り出す
    hmdDesc = ovr_GetHmdDesc(session);

#  if defined(DEBUG)
    // Oculus Rift の情報を表示する
    std::cerr
      << "\nProduct name: " << hmdDesc.ProductName
      << "\nResolution:   " << hmdDesc.Resolution.w << " x " << hmdDesc.Resolution.h
      << "\nDefault Fov:  (" << hmdDesc.DefaultEyeFov[ovrEye_Left].LeftTan
      << "," << hmdDesc.DefaultEyeFov[ovrEye_Left].DownTan
      << ") - (" << hmdDesc.DefaultEyeFov[ovrEye_Left].RightTan
      << "," << hmdDesc.DefaultEyeFov[ovrEye_Left].UpTan
      << ")\n              (" << hmdDesc.DefaultEyeFov[ovrEye_Right].LeftTan
      << "," << hmdDesc.DefaultEyeFov[ovrEye_Right].DownTan
      << ") - (" << hmdDesc.DefaultEyeFov[ovrEye_Right].RightTan
      << "," << hmdDesc.DefaultEyeFov[ovrEye_Right].UpTan
      << ")\nMaximum Fov:  (" << hmdDesc.MaxEyeFov[ovrEye_Left].LeftTan
      << "," << hmdDesc.MaxEyeFov[ovrEye_Left].DownTan
      << ") - (" << hmdDesc.MaxEyeFov[ovrEye_Left].RightTan
      << "," << hmdDesc.MaxEyeFov[ovrEye_Left].UpTan
      << ")\n              (" << hmdDesc.MaxEyeFov[ovrEye_Right].LeftTan
      << "," << hmdDesc.MaxEyeFov[ovrEye_Right].DownTan
      << ") - (" << hmdDesc.MaxEyeFov[ovrEye_Right].RightTan
      << "," << hmdDesc.MaxEyeFov[ovrEye_Right].UpTan
      << ")\n" << std::endl;
#  endif

    // Oculus Rift に転送する描画データを作成する
#if OVR_PRODUCT_VERSION > 0
    layerData.Header.Type = ovrLayerType_EyeFov;
#else
    layerData.Header.Type = ovrLayerType_EyeFovDepth;
#endif
    layerData.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // OpenGL なので左下が原点

    // Oculus Rift 表示用の FBO を作成する
    for (int eye = 0; eye < ovrEye_Count; ++eye)
    {
      // Oculus Rift の視野を取得する
      const auto& eyeFov(hmdDesc.DefaultEyeFov[eye]);
#if OVR_PRODUCT_VERSION > 0
      layerData.Fov[eye] = eyeFov;

      const auto& fov(eyeFov);
#else
      layerData.EyeFov.Fov[eye] = eyeFov;

      // Oculus Rift のレンズ補正等の設定値を取得する
      eyeRenderDesc[eye] = ovr_GetRenderDesc(session, ovrEyeType(eye), eyeFov);

      // Oculus Rift の片目の頭の位置からの変位を求める
      const auto& offset(eyeRenderDesc[eye].HmdToEyeViewOffset);
      mv[eye] = ggTranslate(-offset.x, -offset.y, -offset.z);

      const auto& fov(eyeRenderDesc[eye].Fov);
#endif

      // ズーム率
      const auto zf(config.display_zoom * config.display_near);

      // 片目の透視投影変換行列を求める
      mp[eye].loadFrustum(-fov.LeftTan * zf, fov.RightTan * zf, -fov.DownTan * zf, fov.UpTan * zf,
        config.display_near, config.display_far);

      // 片目のスクリーンのサイズと中心位置
      screen[eye][0] = (fov.RightTan + fov.LeftTan) * 0.5f;
      screen[eye][1] = (fov.UpTan + fov.DownTan) * 0.5f;
      screen[eye][2] = (fov.RightTan - fov.LeftTan) * 0.5f;
      screen[eye][3] = (fov.UpTan - fov.DownTan) * 0.5f;

      // Oculus Rift 表示用の FBO のサイズ
      const auto size(ovr_GetFovTextureSize(session, ovrEyeType(eye), eyeFov, 1.0f));
#if OVR_PRODUCT_VERSION > 0
      layerData.Viewport[eye].Pos = OVR::Vector2i(0, 0);
      layerData.Viewport[eye].Size = size;

      // Oculus Rift 表示用の FBO のカラーバッファとして使うテクスチャセットの特性
      const ovrTextureSwapChainDesc colorDesc =
      {
        ovrTexture_2D,                    // Type
        OVR_FORMAT_R8G8B8A8_UNORM_SRGB,   // Format
        1,                                // ArraySize
        size.w,                           // Width
        size.h,                           // Height
        1,                                // MipLevels
        1,                                // SampleCount
        ovrFalse,                         // StaticImage
        0, 0
      };

      // Oculus Rift 表示用の FBO のバッファとして使うテクスチャセットを作成する
      layerData.ColorTexture[eye] = nullptr;
      if (OVR_SUCCESS(ovr_CreateTextureSwapChainGL(session, &colorDesc, &layerData.ColorTexture[eye])))
      {
        // Oculus Rift 表示用の FBO のカラーバッファとして使うテクスチャセットを作成する
        int length(0);
        if (OVR_SUCCESS(ovr_GetTextureSwapChainLength(session, layerData.ColorTexture[eye], &length)))
        {
          for (int i = 0; i < length; ++i)
          {
            GLuint texId;
            ovr_GetTextureSwapChainBufferGL(session, layerData.ColorTexture[eye], i, &texId);
            glBindTexture(GL_TEXTURE_2D, texId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
          }
        }

        // Oculus Rift 表示用の FBO のデプスバッファとして使うテクスチャを作成する
        glGenTextures(1, &oculusDepth[eye]);
        glBindTexture(GL_TEXTURE_2D, oculusDepth[eye]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, size.w, size.h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      }
#else
      layerData.EyeFov.Viewport[eye].Pos = OVR::Vector2i(0, 0);
      layerData.EyeFov.Viewport[eye].Size = size;

      // Oculus Rift 表示用の FBO のカラーバッファとして使うテクスチャセットを作成する
      ovrSwapTextureSet* colorTexture;
      ovr_CreateSwapTextureSetGL(session, GL_SRGB8_ALPHA8, size.w, size.h, &colorTexture);
      layerData.EyeFov.ColorTexture[eye] = colorTexture;

      // Oculus Rift 表示用の FBO のデプスバッファとして使うテクスチャセットの作成
      ovrSwapTextureSet* depthTexture;
      ovr_CreateSwapTextureSetGL(session, GL_DEPTH_COMPONENT32F, size.w, size.h, &depthTexture);
      layerData.EyeFovDepth.DepthTexture[eye] = depthTexture;
#endif
    }

#if OVR_PRODUCT_VERSION > 0
    // Oculus Rift の画面のアスペクト比を求める
    aspect = static_cast<GLfloat>(layerData.Viewport[ovrEye_Left].Size.w)
      / static_cast<GLfloat>(layerData.Viewport[ovrEye_Left].Size.h);

    // ミラー表示用の FBO を作成する
    const ovrMirrorTextureDesc mirrorDesc
    {
      OVR_FORMAT_R8G8B8A8_UNORM_SRGB,   // Format
      width,                            // Width
      height,                           // Height
      0
    };

    if (OVR_SUCCESS(ovr_CreateMirrorTextureGL(session, &mirrorDesc, &mirrorTexture)))
    {
      // ミラー表示用の FBO のカラーバッファとして使うテクスチャを作成する
      GLuint texId;
      if (OVR_SUCCESS(ovr_GetMirrorTextureBufferGL(session, mirrorTexture, &texId)))
      {
        glGenFramebuffers(1, &mirrorFbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFbo);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
        glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      }
    }

    // 姿勢のトラッキングにおける床の高さを 0 に設定する
    ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);
#else
    // Oculus Rift の画面のアスペクト比を求める
    aspect = static_cast<GLfloat>(layerData.EyeFov.Viewport[ovrEye_Left].Size.w)
      / static_cast<GLfloat>(layerData.EyeFov.Viewport[ovrEye_Left].Size.h);

    // ミラー表示用の FBO を作成する
    if (OVR_SUCCESS(ovr_CreateMirrorTextureGL(session, GL_SRGB8_ALPHA8, width, height, reinterpret_cast<ovrTexture**>(&mirrorTexture))))
    {
      glGenFramebuffers(1, &mirrorFbo);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFbo);
      glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTexture->OGL.TexId, 0);
      glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

    // TimeWarp に使う変換行列の成分を取り出す
    auto& posTimewarpProjectionDesc(layerData.EyeFovDepth.ProjectionDesc);
    posTimewarpProjectionDesc.Projection22 = (mp[ovrEye_Left].get()[4 * 2 + 2] + mp[ovrEye_Left].get()[4 * 3 + 2]) * 0.5f;
    posTimewarpProjectionDesc.Projection23 = mp[ovrEye_Left].get()[4 * 2 + 3] * 0.5f;
    posTimewarpProjectionDesc.Projection32 = mp[ovrEye_Left].get()[4 * 3 + 2];
#endif

    // Oculus Rift のレンダリング用の FBO を作成する
    glGenFramebuffers(ovrEye_Count, oculusFbo);
  }

#if defined(IMGUI_VERSION)
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

  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 430");
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

  // Oculus Rift 使用時
  if (session)
  {
    // ミラー表示用の FBO を削除する
    if (mirrorFbo) glDeleteFramebuffers(1, &mirrorFbo);

    // ミラー表示に使ったテクスチャを開放する
#if OVR_PRODUCT_VERSION > 0
    if (mirrorTexture) ovr_DestroyMirrorTexture(session, mirrorTexture);
#else
    if (mirrorTexture)
    {
      glDeleteTextures(1, &mirrorTexture->OGL.TexId);
      ovr_DestroyMirrorTexture(session, reinterpret_cast<ovrTexture*>(mirrorTexture));
    }
#endif

    // Oculus Rift のレンダリング用の FBO を削除する
    glDeleteFramebuffers(ovrEye_Count, oculusFbo);

    // Oculus Rift 表示用の FBO を削除する
    for (int eye = 0; eye < ovrEye_Count; ++eye)
    {
      // レンダリングターゲットに使ったテクスチャを開放する
#if OVR_PRODUCT_VERSION > 0
      if (layerData.ColorTexture[eye])
      {
        ovr_DestroyTextureSwapChain(session, layerData.ColorTexture[eye]);
        layerData.ColorTexture[eye] = nullptr;
      }
#else
      auto* const colorTexture(layerData.EyeFov.ColorTexture[eye]);
      for (int i = 0; i < colorTexture->TextureCount; ++i)
      {
        const auto* const ctex(reinterpret_cast<ovrGLTexture*>(&colorTexture->Textures[i]));
        glDeleteTextures(1, &ctex->OGL.TexId);
      }
      ovr_DestroySwapTextureSet(session, colorTexture);
#endif

      // デプスバッファとして使ったテクスチャを開放する
#if OVR_PRODUCT_VERSION > 0
      glDeleteTextures(1, &oculusDepth[eye]);
      oculusDepth[eye] = 0;
#else
      auto* const depthTexture(layerData.EyeFovDepth.DepthTexture[eye]);
      for (int i = 0; i < depthTexture->TextureCount; ++i)
      {
        const auto* const dtex(reinterpret_cast<ovrGLTexture*>(&depthTexture->Textures[i]));
        glDeleteTextures(1, &dtex->OGL.TexId);
      }
      ovr_DestroySwapTextureSet(session, depthTexture);
#endif
    }

    // Oculus Rift のセッションを破棄する
    ovr_Destroy(session);
    const_cast<ovrSession>(session) = nullptr;
  }

#if defined(IMGUI_VERSION)
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
  const auto speedFactor((fabs(config.position[2]) + 0.2f));

  //
  // ジョイスティックによる操作
  //

  // ジョイスティックが有効なら
  if (joy >= 0)
  {
    // ボタン
    int btnsCount;
    const auto* const btns{ glfwGetJoystickButtons(joy, &btnsCount) };

    for (int i = 0; i < btnsCount; ++i)
    {
      if (btns[i] != 0) std::cout << "btns[" << i << "] = " << (int)btns[i] << "\n";
    }

    // スティック
    int axesCount;
    const auto* const axes{ glfwGetJoystickAxes(joy, &axesCount) };

    for (int i = 0; i < axesCount; ++i)
    {
      if (fabs(axes[i]) > 0.1f) std::cout << "axes[" << i << "] = " << axes[i] << "\n";
    }

    // スティックの速度係数
    const auto axesSpeedFactor{ axesSpeedScale * speedFactor };

    // L ボタンと R ボタンの状態
    const auto lrButton{ btns[4] | btns[5] };

    // 右アナログスティック
    if (axesCount > 3)
    {
      // 物体を左右に移動する
      config.position[0] += (axes[0] - origin[0]) * axesSpeedFactor;

      // L ボタンか R ボタンを同時に押していれば
      if (lrButton)
      {
        // 物体を前後に移動する
        config.position[2] += (axes[1] - origin[1]) * axesSpeedFactor;
      }
      else
      {
        // 物体を上下に移動する
        config.position[1] += (axes[1] - origin[1]) * axesSpeedFactor;
      }

      // 物体を左右に回転する
      trackball.rotate(ggRotateQuaternion(0.0f, 1.0f, 0.0f, (axes[2] - origin[2]) * axesSpeedFactor));

      // 物体を上下に回転する
      trackball.rotate(ggRotateQuaternion(1.0f, 0.0f, 0.0f, (axes[3] - origin[3]) * axesSpeedFactor));
    }

    // B, X ボタンの状態
    const auto parallaxButton{ btns[3] - btns[0] };

    // B, X ボタンに変化があれば
    if (parallaxButton)
    {
      // L ボタンか R ボタンを同時に押していれば
      if (lrButton)
      {
        // 背景を左右に回転する
        if (parallaxButton > 0)
        {
          config.parallax_offset[camL] *= qrStep[0];
          config.parallax_offset[camR] *= qrStep[0].conjugate();
        }
        else
        {
          config.parallax_offset[camL] *= qrStep[0].conjugate();
          config.parallax_offset[camR] *= qrStep[0];
        }
      }
      else
      {
        // スクリーンの間隔を調整する
        config.display_offset += parallaxButton * offsetStep;
      }
    }

    // Y, A ボタンの状態
    const auto zoomButton{ btns[2] - btns[3] };

    // Y, A ボタンに変化があれば
    if (zoomButton)
    {
      // L ボタンか R ボタンを同時に押していれば
      if (lrButton)
      {
        // 背景を上下に回転する
        if (zoomButton > 0)
        {
          config.parallax_offset[camL] *= qrStep[1];
          config.parallax_offset[camR] *= qrStep[1].conjugate();
        }
        else
        {
          config.parallax_offset[camL] *= qrStep[1].conjugate();
          config.parallax_offset[camR] *= qrStep[1];
        }
      }
      else
      {
        // 焦点距離を調整する
        const int change{ focalChange + zoomButton };
        if (change < focalMax)
        {
          focalChange = change;
          config.display_focal = defaults.display_focal * focalMax / (focalMax - change);
        }
      }
    }

    // 十字キーの左右ボタンの状態
    const auto textureXButton{ btns[13] - btns[15] };

    // 十字キーの左右ボタンに変化があれば
    if (textureXButton)
    {
      // L ボタンか R ボタンを同時に押していれば
      if (lrButton)
      {
        // 背景に対する横方向の画角を調整する
        config.circle[0] = defaults.circle[0] + static_cast<GLfloat>(circleChange[0] += textureXButton) * shiftStep;
      }
      else
      {
        // 背景の横位置を調整する
        config.circle[2] = defaults.circle[2] + static_cast<GLfloat>(circleChange[2] += textureXButton) * shiftStep;
      }
    }

    // 十字キーの上下ボタンの状態
    const auto textureYButton{ btns[12] - btns[14] };

    // 十字キーの上下ボタンに変化があれば
    if (textureYButton)
    {
      // L ボタンか R ボタンを同時に押していれば
      if (lrButton)
      {
        // 背景に対する縦方向の画角を調整する
        config.circle[1] = defaults.circle[1] + static_cast<GLfloat>(circleChange[1] += textureYButton) * shiftStep;
      }
      else
      {
        // 背景の縦位置を調整する
        config.circle[3] = defaults.circle[3] + static_cast<GLfloat>(circleChange[3] += textureYButton) * shiftStep;
      }
    }

    // 設定をリセットする
    if (btns[7])
    {
      reset();
      updateProjectionMatrix();
    }
  }

  //
  // マウスによる操作
  //

  // マウスの位置
  double x, y;

#if defined(IMGUI_VERSION)

  // ImGui の新規フレームを作成する
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();

  // マウスカーソルが ImGui のウィンドウ上にあったら Window クラスのマウス位置を更新しない
  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return true;

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
    const auto speed{ speedScale * speedFactor };
    config.position[0] += static_cast<GLfloat>(x - cx) * speed;
    config.position[1] += static_cast<GLfloat>(cy - y) * speed;
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

#if defined(IMGUI_VERSION)
    // ImGui がキーボードを使うときはキーボードの処理を行わない
  if (ImGui::GetIO().WantCaptureKeyboard) return true;
#endif

  // シフトキーの状態
  const auto shiftKey{ glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) };

  // コントロールキーの状態
  const auto ctrlKey{ glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) };

  // オルタネートキーの状態
  const auto altKey{ glfwGetKey(window, GLFW_KEY_LEFT_ALT) || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) };

  // 右矢印キー操作
  if (glfwGetKey(window, GLFW_KEY_RIGHT))
  {
    if (altKey)
    {
      // 背景を右に回転する
      if (!ctrlKey) config.parallax_offset[camL] *= qrStep[0].conjugate();
      if (!shiftKey) config.parallax_offset[camR] *= qrStep[0].conjugate();
    }
    else if (ctrlKey)
    {
      // 背景に対する横方向の画角を広げる
      config.circle[0] = defaults.circle[0] + static_cast<GLfloat>(++circleChange[0]) * shiftStep;
    }
    else if (shiftKey)
    {
      // 背景を右にずらす
      config.circle[2] = defaults.circle[2] + static_cast<GLfloat>(++circleChange[2]) * shiftStep;
    }
    else if (config.display_mode != MONO)
    {
      // スクリーンの間隔を拡大する
      config.display_offset += offsetStep;
    }
  }

  // 左矢印キー操作
  if (glfwGetKey(window, GLFW_KEY_LEFT))
  {
    if (altKey)
    {
      // 背景を左に回転する
      if (!ctrlKey) config.parallax_offset[camL] *= qrStep[0];
      if (!shiftKey) config.parallax_offset[camR] *= qrStep[0];
    }
    else if (ctrlKey)
    {
      // 背景に対する横方向の画角を狭める
      config.circle[0] = defaults.circle[0] + static_cast<GLfloat>(--circleChange[0]) * shiftStep;
    }
    else if (shiftKey)
    {
      // 背景を左にずらす
      config.circle[2] = defaults.circle[2] + static_cast<GLfloat>(--circleChange[2]) * shiftStep;
    }
    else if (config.display_mode != MONO)
    {
      // 視差を縮小する
      config.display_offset -= offsetStep;
    }
  }

  // 上矢印キー操作
  if (glfwGetKey(window, GLFW_KEY_UP))
  {
    if (altKey)
    {
      // 背景を上に回転する
      if (!ctrlKey) config.parallax_offset[camL] *= qrStep[1];
      if (!shiftKey) config.parallax_offset[camR] *= qrStep[1];
    }
    else if (ctrlKey)
    {
      // 背景に対する縦方向の画角を広げる
      config.circle[1] = defaults.circle[1] + static_cast<GLfloat>(++circleChange[1]) * shiftStep;
    }
    else if (shiftKey)
    {
      // 背景を上にずらす
      config.circle[3] = defaults.circle[3] + static_cast<GLfloat>(++circleChange[3]) * shiftStep;
    }
    else
    {
      // 焦点距離を延ばす
      const int change{ focalChange + 1 };
      if (change < focalMax)
      {
        focalChange = change;
        config.display_focal = defaults.display_focal * focalMax / (focalMax - change);
      }
    }
  }

  // 下矢印キー操作
  if (glfwGetKey(window, GLFW_KEY_DOWN))
  {
    if (altKey)
    {
      // 背景を下に回転する
      if (!ctrlKey) config.parallax_offset[camL] *= qrStep[1].conjugate();
      if (!shiftKey) config.parallax_offset[camR] *= qrStep[1].conjugate();
    }
    else if (ctrlKey)
    {
      // 背景に対する縦方向の画角を狭める
      config.circle[1] = defaults.circle[2] + static_cast<GLfloat>(--circleChange[1]) * shiftStep;
    }
    else if (shiftKey)
    {
      // 背景を下にずらす
      config.circle[3] = defaults.circle[3] + static_cast<GLfloat>(--circleChange[3]) * shiftStep;
    }
    else
    {
      // 焦点距離を縮める
      const int change{ focalChange - 1 };
      if (change < focalMax)
      {
        focalChange = change;
        config.display_focal = defaults.display_focal * focalMax / (focalMax - change);
      }
    }
  }

  // 'P' キーの操作
  if (glfwGetKey(window, GLFW_KEY_P))
  {
    // 視差を調整する
    config.parallax += shiftKey ? -parallaxStep : parallaxStep;

    // 透視投影変換行列を更新する
    updateProjectionMatrix();
  }

  // 'Z' キーの操作
  if (glfwGetKey(window, GLFW_KEY_Z))
  {
    // ズーム率を調整する
    const int change{ zoomChange + (shiftKey ? 1 : -1) };
    if (change < zoomMax)
    {
      zoomChange = change;
      config.display_zoom = defaults.display_zoom * zoomMax / (zoomMax - change);
    }

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
// 描画開始
//
//   ・図形の描画開始前に呼び出す
//
bool Window::start()
{
  // Oculus Rift 使用時
  if (session)
  {
#if OVR_PRODUCT_VERSION > 0
    // セッションの状態を取得する
    ovrSessionStatus sessionStatus;
    ovr_GetSessionStatus(session, &sessionStatus);

    // アプリケーションが終了を要求しているときはウィンドウのクローズフラグを立てる
    if (sessionStatus.ShouldQuit) glfwSetWindowShouldClose(window, GLFW_TRUE);

    // 現在の状態をトラッキングの原点にする
    if (sessionStatus.ShouldRecenter) ovr_RecenterTrackingOrigin(session);

    // HmdToEyeOffset などは実行時に変化するので毎フレーム ovr_GetRenderDesc() で ovrEyeRenderDesc を取得する
    const ovrEyeRenderDesc eyeRenderDesc[]
    {
      ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]),
      ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1])
    };

    // Oculus Rift の左右の目のトラッキングの位置からの変位を求める
    const ovrPosef hmdToEyePose[]
    {
      eyeRenderDesc[0].HmdToEyePose,
      eyeRenderDesc[1].HmdToEyePose
    };

    // 視点の姿勢情報を取得する
    ovr_GetEyePoses(session, frameIndex, ovrTrue, hmdToEyePose, layerData.RenderPose, &layerData.SensorSampleTime);

    // Oculus Rift の両目の頭の位置からの変位を求める
    mv[0] = ggTranslate(-hmdToEyePose[0].Position.x, -hmdToEyePose[0].Position.y, -hmdToEyePose[0].Position.z);
    mv[1] = ggTranslate(-hmdToEyePose[1].Position.x, -hmdToEyePose[1].Position.y, -hmdToEyePose[1].Position.z);

#else
    // フレームのタイミング計測開始
    const auto ftiming(ovr_GetPredictedDisplayTime(session, 0));

    // sensorSampleTime の取得は可能な限り ovr_GetTrackingState() の近くで行う
    layerData.EyeFov.SensorSampleTime = ovr_GetTimeInSeconds();

    // ヘッドトラッキングの状態を取得する
    const auto hmdState(ovr_GetTrackingState(session, ftiming, ovrTrue));

    // 瞳孔間隔
    const ovrVector3f hmdToEyeViewOffset[]
    {
      eyeRenderDesc[0].HmdToEyeViewOffset,
      eyeRenderDesc[1].HmdToEyeViewOffset
    };

    // 視点の姿勢情報を求める
    ovr_CalcEyePoses(hmdState.HeadPose.ThePose, hmdToEyeViewOffset, eyePose);
#endif
  }

  // モデル変換行列を設定する
  mm = ggTranslate(config.position[0], config.position[1], -config.position[2]) * trackball.getMatrix();

  // モデル変換行列を共有メモリに保存する
  Scene::setup(mm);

  return true;
}

//
// 図形を見せる目を選択する
//
// ・視点ごとに呼び出す
//
void Window::select(int eye)
{
  // Oculus Rift に表示する
  if (session)
  {
#if OVR_PRODUCT_VERSION > 0
    // Oculus Rift にレンダリングする FBO に切り替える
    if (layerData.ColorTexture[eye])
    {
      // FBO のカラーバッファに使う現在のテクスチャのインデックスを取得する
      int curIndex;
      ovr_GetTextureSwapChainCurrentIndex(session, layerData.ColorTexture[eye], &curIndex);

      // FBO のカラーバッファに使うテクスチャを取得する
      GLuint curTexId;
      ovr_GetTextureSwapChainBufferGL(session, layerData.ColorTexture[eye], curIndex, &curTexId);

      // FBO を設定する
      glBindFramebuffer(GL_FRAMEBUFFER, oculusFbo[eye]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, oculusDepth[eye], 0);

      // ビューポートを設定する
      const auto& vp{ layerData.Viewport[eye] };
      glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
    }

    // Oculus Rift の片目の位置と回転を取得する
    const auto& o{ layerData.RenderPose[eye].Orientation };
    const auto& p{ layerData.RenderPose[eye].Position };
#else
    // レンダーターゲットに描画する前にレンダーターゲットのインデックスをインクリメントする
    auto* const colorTexture(layerData.EyeFov.ColorTexture[eye]);
    colorTexture->CurrentIndex = (colorTexture->CurrentIndex + 1) % colorTexture->TextureCount;
    auto* const depthTexture(layerData.EyeFovDepth.DepthTexture[eye]);
    depthTexture->CurrentIndex = (depthTexture->CurrentIndex + 1) % depthTexture->TextureCount;

    // レンダーターゲットを切り替える
    glBindFramebuffer(GL_FRAMEBUFFER, oculusFbo[eye]);
    const auto& ctex(reinterpret_cast<ovrGLTexture*>(&colorTexture->Textures[colorTexture->CurrentIndex]));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctex->OGL.TexId, 0);
    const auto& dtex(reinterpret_cast<ovrGLTexture*>(&depthTexture->Textures[depthTexture->CurrentIndex]));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dtex->OGL.TexId, 0);

    // ビューポートを設定する
    const auto& vp(layerData.EyeFov.Viewport[eye]);
    glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);

    // Oculus Rift の片目の位置と回転を取得する
    const auto& p(eyePose[eye].Position);
    const auto& o(eyePose[eye].Orientation);
#endif

    // Oculus Rift の片目の位置を保存する
    po[eye][0] = p.x;
    po[eye][1] = p.y;
    po[eye][2] = p.z;
    po[eye][3] = 1.0f;

    // Oculus Rift の片目の回転を保存する
    qo[eye] = GgQuaternion(o.x, o.y, o.z, -o.w);

    // 四元数から変換行列を求める
    mo[eye] = qo[eye].getMatrix();

    // ヘッドトラッキングの変換行列を共有メモリに保存する
    Scene::setLocalAttitude(eye, mo[eye].transpose());

    // デプスバッファを消去する
    glClear(GL_DEPTH_BUFFER_BIT);

    // Oculus Rift に対する処理はここで終了
    return;
  }

  // Oculus Rift 以外に表示する
  switch (config.display_mode)
  {
  case MONO:

    // ウィンドウ全体をビューポートにする
    glViewport(0, 0, width, height);

    // デプスバッファを消去する
    glClear(GL_DEPTH_BUFFER_BIT);

    break;

  case TOPANDBOTTOM:

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

  case LINEBYLINE:
  case SIDEBYSIDE:

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

  default:
    break;
  }

  // 目をずらす代わりにシーンを動かす
  mv[eye] = ggTranslate(eye == camL ? config.parallax : -config.parallax, 0.0f, 0.0f);
}

//
// 図形の描画を完了する
//
void Window::commit(int eye)
{
#if OVR_PRODUCT_VERSION > 0
  if (session)
  {
    // GL_COLOR_ATTACHMENT0 に割り当てられたテクスチャが wglDXUnlockObjectsNV() によって
    // アンロックされるために次のフレームの処理において無効な GL_COLOR_ATTACHMENT0 が
    // FBO に結合されるのを避ける
    glBindFramebuffer(GL_FRAMEBUFFER, oculusFbo[eye]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

    // 保留中の変更を layerData.ColorTexture[eye] に反映しインデックスを更新する
    ovr_CommitTextureSwapChain(session, layerData.ColorTexture[eye]);
  }
#endif
}

//
// カラーバッファを入れ替えてイベントを取り出す
//
//   ・図形の描画終了後に呼び出す
//   ・ダブルバッファリングのバッファの入れ替えを行う
//   ・キーボード操作等のイベントを取り出す
//
void Window::swapBuffers()
{
  // エラーチェック
  ggError();

  // Oculus Rift 使用時
  if (session)
  {
#if OVR_PRODUCT_VERSION > 0
    // 描画データを Oculus Rift に転送する
    const auto* const layers(&layerData.Header);
    if (OVR_FAILURE(ovr_SubmitFrame(session, frameIndex++, nullptr, &layers, 1)))
#else
    // Oculus Rift 上の描画位置と拡大率を求める
    ovrViewScaleDesc viewScaleDesc;
    viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
    viewScaleDesc.HmdToEyeViewOffset[0] = eyeRenderDesc[0].HmdToEyeViewOffset;
    viewScaleDesc.HmdToEyeViewOffset[1] = eyeRenderDesc[1].HmdToEyeViewOffset;

    // 描画データを更新する
    layerData.EyeFov.RenderPose[0] = eyePose[0];
    layerData.EyeFov.RenderPose[1] = eyePose[1];

    // 描画データを Oculus Rift に転送する
    const auto* const layers(&layerData.Header);
    if (OVR_FAILURE(ovr_SubmitFrame(session, 0, &viewScaleDesc, &layers, 1)))
#endif
    {
      // 転送に失敗したら Oculus Rift の設定を最初からやり直す必要があるらしい
      NOTIFY("Oculus Rift へのデータの転送に失敗しました。");

      // けどめんどくさいのでウィンドウを閉じてしまう
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    // レンダリング結果をミラー表示用のフレームバッファにも転送する
    if (showMirror)
    {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFbo);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#if OVR_PRODUCT_VERSION > 0
      glBlitFramebuffer(0, height, width, 0, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
#else
      const auto w(mirrorTexture->OGL.Header.TextureSize.w);
      const auto h(mirrorTexture->OGL.Header.TextureSize.h);
      glBlitFramebuffer(0, h, w, 0, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
#endif
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

#if defined(IMGUI_VERSION)
    // ユーザインタフェースを描画する
    if (showMenu)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      ImDrawData* const imDrawData(ImGui::GetDrawData());
      if (imDrawData) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
    }
#endif

    // 残っている OpenGL コマンドを実行する
    glFlush();
  }
  else
  {
#if defined(IMGUI_VERSION)
    // ユーザインタフェースを描画する
    if (showMenu)
    {
      ImDrawData* const imDrawData(ImGui::GetDrawData());
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
void Window::resize(GLFWwindow* window, int width, int height)
{
  // このインスタンスの this ポインタを得る
  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

  if (instance)
  {
    // Oculus Rift 使用時以外
    if (!instance->session)
    {
      if (instance->config.display_mode == LINEBYLINE)
      {
        // VR 室のディスプレイでは表示領域の横半分をビューポートにする
        width /= 2;

        // VR 室のディスプレイのアスペクト比は表示領域の横半分になる
        instance->aspect = static_cast<GLfloat>(width) / static_cast<GLfloat>(height);
      }
      else
      {
        // リサイズ後のディスプレイののアスペクト比を求める
        instance->aspect = instance->config.display_aspect
          ? instance->config.display_aspect
          : static_cast<GLfloat>(width) / static_cast<GLfloat>(height);

        // リサイズ後のビューポートを設定する
        switch (instance->config.display_mode)
        {
        case SIDEBYSIDE:

          // ウィンドウの横半分をビューポートにする
          width /= 2;
          break;

        case TOPANDBOTTOM:

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
    instance->samples[0] = static_cast<GLsizei>(sqrt(instance->aspect * instance->config.camera_texture_samples));
    instance->samples[1] = instance->config.camera_texture_samples / instance->samples[0];

    // 背景描画用のメッシュの縦横の格子間隔を求める
    instance->gap[0] = 2.0f / static_cast<GLfloat>(instance->samples[0] - 1);
    instance->gap[1] = 2.0f / static_cast<GLfloat>(instance->samples[1] - 1);

    // 透視投影変換行列を求める
    instance->updateProjectionMatrix();

    // トラックボール処理の範囲を設定する
    instance->trackball.region(static_cast<float>(width) / angleScale, static_cast<float>(height) / angleScale);
  }
}

//
// マウスボタンを操作したときの処理
//
//   ・マウスボタンを押したときにコールバック関数として呼び出される
//
void Window::mouse(GLFWwindow* window, int button, int action, int mods)
{
#if defined(IMGUI_VERSION)
  // マウスカーソルが ImGui のウィンドウ上にあったら Window クラスのマウス位置を更新しない
  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;
#endif

  // このインスタンスの this ポインタを得る
  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

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
void Window::wheel(GLFWwindow* window, double x, double y)
{
#if defined(IMGUI_VERSION)
  // マウスカーソルが ImGui のウィンドウ上にあったら Window クラスのマウスホイールの回転量を更新しない
  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;
#endif

  // このインスタンスの this ポインタを得る
  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

  if (instance)
  {
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT))
    {
      // ズーム率を調整する
      const int change{ instance->zoomChange + static_cast<int>(y) };
      if (change < zoomMax)
      {
        instance->zoomChange = change;
        instance->config.display_zoom = instance->defaults.display_zoom * zoomMax / (zoomMax - change);
      }

      // 透視投影変換行列を更新する
      instance->updateProjectionMatrix();
    }
    else
    {
      // 物体を前後に移動する
      const GLfloat advSpeed((fabs(instance->config.position[2]) * 5.0f + 1.0f) * wheelYStep * static_cast<GLfloat>(y));
      instance->config.position[2] += advSpeed;
    }
  }
}

//
// キーボードをタイプした時の処理
//
//   ．キーボードをタイプした時にコールバック関数として呼び出される
//
void Window::keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
#if defined(IMGUI_VERSION)
  // ImGui がキーボードを使うときはキーボードの処理を行わない
  if (ImGui::GetIO().WantCaptureKeyboard) return;
#endif

  // このインスタンスの this ポインタを得る
  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

  if (instance)
  {
    if (action == GLFW_PRESS)
    {
      // 最後にタイプしたキーを覚えておく
      instance->key = key;

      // キーボード操作による処理
      switch (key)
      {
      case GLFW_KEY_HOME:
      case GLFW_KEY_R:

        // 設定をリセットする
        instance->reset();
        instance->updateProjectionMatrix();
        break;

      case GLFW_KEY_SPACE:

        // シーンの表示を ON/OFF する
        instance->showScene = !instance->showScene;
        break;

      case GLFW_KEY_M:

        // ミラー表示を ON/OFF する
        instance->showMirror = !instance->showMirror;
        break;

      case GLFW_KEY_N:

        // メニュー表示を ON/OFF する
        instance->showMenu = !instance->showMenu;
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
  // 背景テクスチャの半径と中心位置
  config.circle = defaults.circle;

  // スクリーンの間隔
  config.display_offset = defaults.display_offset;

  // ズーム率
  config.display_zoom = defaults.display_zoom;

  // 焦点距離
  config.display_focal = defaults.display_focal;

  // 視差
  config.parallax = defaults.display_mode != MONO ? defaults.parallax : 0.0f;

  // カメラ方向の補正値
  config.parallax_offset = defaults.parallax_offset;
  
  // 物体の位置
  config.position = defaults.position;

  // 物体の回転
  trackball.reset(defaults.orientation);
}

//
// 透視投影変換行列を求める
//
//   ・ウィンドウのサイズ変更時やカメラパラメータの変更時に呼び出す
//
void Window::updateProjectionMatrix()
{
  if (session)
  {
    // Oculus Rift 表示用の FBO を作成する
    for (int eye = 0; eye < ovrEye_Count; ++eye)
    {
      // Oculus Rift の視野を取得する
      const auto& eyeFov{ hmdDesc.DefaultEyeFov[eye] };
#if OVR_PRODUCT_VERSION > 0
      layerData.Fov[eye] = eyeFov;

      const auto& fov(eyeFov);
#else
      layerData.EyeFov.Fov[eye] = eyeFov;

      // Oculus Rift のレンズ補正等の設定値を取得する
      eyeRenderDesc[eye] = ovr_GetRenderDesc(session, ovrEyeType(eye), eyeFov);

      // Oculus Rift の片目の頭の位置からの変位を求める
      const auto& offset(eyeRenderDesc[eye].HmdToEyeViewOffset);
      mv[eye] = ggTranslate(-offset.x, -offset.y, -offset.z);

      const auto& fov(eyeRenderDesc[eye].Fov);
#endif

      // ズーム率
      const auto zf{ config.display_zoom * config.display_near };

      // 片目の透視投影変換行列を求める
      mp[eye].loadFrustum(-fov.LeftTan * zf, fov.RightTan * zf, -fov.DownTan * zf, fov.UpTan * zf,
        config.display_near, config.display_far);
    }
  }
  else
  {
    // ズーム率
    const auto zf{ config.display_zoom * config.display_near };

    // スクリーンの高さと幅
    const auto screenHeight{ config.display_center / config.display_distance };
    const auto screenWidth{ screenHeight * aspect };

    // 視差によるスクリーンのシフト量
    GLfloat shift{ config.parallax * config.display_near / config.display_distance };

    // 左目の視野
    const GLfloat fovL[]
    {
      -screenWidth + shift,
      screenWidth + shift,
      -screenHeight,
      screenHeight
    };

    // 左目の透視投影変換行列を求める
    mp[ovrEye_Left].loadFrustum(fovL[0] * zf, fovL[1] * zf, fovL[2] * zf, fovL[3] * zf,
      config.display_near, config.display_far);

    // 左目のスクリーンのサイズと中心位置
    screen[ovrEye_Left][0] = (fovL[1] - fovL[0]) * 0.5f;
    screen[ovrEye_Left][1] = (fovL[3] - fovL[2]) * 0.5f;
    screen[ovrEye_Left][2] = (fovL[1] + fovL[0]) * 0.5f;
    screen[ovrEye_Left][3] = (fovL[3] + fovL[2]) * 0.5f;

    // Oculus Rift 以外の立体視表示の場合
    if (config.display_mode != MONO)
    {
      // 右目の視野
      const GLfloat fovR[]
      {
        -screenWidth - shift,
        screenWidth - shift,
        -screenHeight,
        screenHeight,
      };

      // 右の透視投影変換行列を求める
      mp[ovrEye_Right].loadFrustum(fovR[0] * zf, fovR[1] * zf, fovR[2] * zf, fovR[3] * zf,
        config.display_near, config.display_far);

      // 右目のスクリーンのサイズと中心位置
      screen[ovrEye_Right][0] = (fovR[1] - fovR[0]) * 0.5f;
      screen[ovrEye_Right][1] = (fovR[3] - fovR[2]) * 0.5f;
      screen[ovrEye_Right][2] = (fovR[1] + fovR[0]) * 0.5f;
      screen[ovrEye_Right][3] = (fovR[3] + fovR[2]) * 0.5f;
    }
  }
}

// カメラ方向の調整ステップ
std::array<GgQuaternion, camCount> Window::qrStep;

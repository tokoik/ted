//
// ウィンドウ関連の処理
//
#include "Window.h"

// Oculus Rift 関連の処理
#include "Oculus.h"

// 姿勢
#include "Attitude.h"

// Oculus SDK ライブラリ (LibOVR) の組み込み
#if defined(_WIN32)
// コンフィギュレーションを調べる
#  if defined(_DEBUG)
// デバッグビルドのライブラリをリンクする
#    pragma comment(lib, "libOVRd.lib")
#  else
// リリースビルドのライブラリをリンクする
#    pragma comment(lib, "libOVR.lib")
#  endif
// LibOVR 1.0 以降なら
#  if OVR_PRODUCT_VERSION >= 1
// GetDefaultAdapterLuid を使うために dxgi.lib をリンクする
#    include <dxgi.h>
#    pragma comment(lib, "dxgi.lib")

//
// デフォルトのグラフィックアダプタの LUID を調べる
//
inline ovrGraphicsLuid GetDefaultAdapterLuid()
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

// 使用しているグラフィックスアダプタをデフォルトと比較する
inline int Compare(const ovrGraphicsLuid& lhs, const ovrGraphicsLuid& rhs)
{
  return memcmp(&lhs, &rhs, sizeof(ovrGraphicsLuid));
}
#  endif
#endif

//
// コンストラクタ
//
Oculus::Oculus()
  : oculusFbo{ 0 }
  , mirrorFbo{ 0 }
  , mirrorTexture{ nullptr }
#if OVR_PRODUCT_VERSION >= 1
  , frameIndex{ 0LL }
  , oculusDepth{ 0 }
  , mirrorWidth{ 1280 }
  , mirrorHeight{ 640 }
#endif
{
}

//
// Oculus Rift を初期化する.
//
//   window Oculus Rift に関連付けるウィンドウ.
//   zoom シーンのズーム率.
//   aspect Oculus Rift のアスペクト比の格納先のポインタ.
//   mp 透視投影変換行列.
//   screen 背景に対するスクリーンのサイズ.
//   戻り値 Oculus Rift の初期化に成功したらコンテキストのポインタ.
//
Oculus* Oculus::initialize(GLfloat zoom, GLfloat* aspect, GgMatrix* mp, GgVector* screen)
{
  // Oculus Rift のコンテキスト
  static Oculus oculus;

  // Oculus Rift のセッション
  auto& session{ oculus.session };

  // 既に Oculus Rift のセッションが作成されていれば何もしないで戻る
  if (session) return &oculus;

  // 一度だけ実行する
  static bool firstTime{ true };
  if (firstTime)
  {
    // Oculus Rift (LibOVR) を初期化パラメータ
    ovrInitParams initParams{ ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0 };

    // Oculus Rift の初期化に失敗したら戻る
    if (OVR_FAILURE(ovr_Initialize(&initParams))) return nullptr;

    // プログラム終了時には LibOVR を終了する
    atexit(ovr_Shutdown);

    // 実行済みの印をつける
    firstTime = false;
  }

  // Oculus Rift のセッションを作成する
  ovrGraphicsLuid luid;
  if (OVR_FAILURE(ovr_Create(&session, &luid))) return nullptr;

#if OVR_PRODUCT_VERSION >= 1
  // デフォルトのグラフィックスアダプタが使われているか確かめる
  if (Compare(luid, GetDefaultAdapterLuid())) return nullptr;
#else
  // (LUID は OpenGL では使っていないらしい)
#endif

  // Oculus Rift の情報を取り出す
  auto& hmdDesc{ oculus.hmdDesc };
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
  auto& layerData{ oculus.layerData };
#if OVR_PRODUCT_VERSION >= 1
  layerData.Header.Type = ovrLayerType_EyeFov;
#else
  layerData.Header.Type = ovrLayerType_EyeFovDepth;
#endif

  // OpenGL なので左下が原点
  layerData.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;

  // Oculus Rift のレンダリング用の FBO を作成する
  glGenFramebuffers(ovrEye_Count, oculus.oculusFbo);

#  if OVR_PRODUCT_VERSION >= 1
  // FBO のデプスバッファとして使うテクスチャを作成する
  glGenTextures(ovrEye_Count, oculus.oculusDepth);
#  endif

  // 前方面が defaults.display_near なのでスクリーンもそれに合わせる
  const GLfloat zf{ defaults.display_near / zoom };

  // Oculus Rift 表示用の FBO を作成する
  for (int eye = 0; eye < ovrEye_Count; ++eye)
  {
    // Oculus Rift の視野を取得する
    const auto &eyeFov(hmdDesc.DefaultEyeFov[eye]);
#if OVR_PRODUCT_VERSION >= 1
    layerData.Fov[eye] = eyeFov;

    // Oculus Rift の実際の視野
    const auto& fov(eyeFov);
#else
    layerData.EyeFov.Fov[eye] = eyeFov;

    // Oculus Rift のレンズ補正等の設定値を取得する
    auto& eyeRenderDesc{ oculus.eyeRenderDesc };
    eyeRenderDesc[eye] = ovr_GetRenderDesc(session, ovrEyeType(eye), eyeFov);

    // Oculus Rift の片目の頭の位置からの変位を求める
    const auto &offset(eyeRenderDesc[eye].HmdToEyeViewOffset);
    mv[eye] = ggTranslate(-offset.x, -offset.y, -offset.z);

    // Oculus Rift の実際の視野
    const auto &fov(eyeRenderDesc.Fov);
#endif

    // 視差の調整値
    const float p{ static_cast<float>((1 - eye * 2) * attitude.parallax) * parallaxStep };

    // 片目の透視投影変換行列を求める
    mp[eye].loadFrustum(
      (p - fov.LeftTan) * zf, (p + fov.RightTan) * zf,
      -fov.DownTan * zf, fov.UpTan * zf,
      defaults.display_near, defaults.display_far
    );

    // 片目のスクリーンのサイズと中心位置
    screen[eye][0] = (fov.RightTan + fov.LeftTan) * 0.5f;
    screen[eye][1] = (fov.UpTan + fov.DownTan) * 0.5f;
    screen[eye][2] = (fov.RightTan - fov.LeftTan) * 0.5f;
    screen[eye][3] = (fov.UpTan - fov.DownTan) * 0.5f;

    // Oculus Rift 表示用の FBO のサイズ
    const auto size(ovr_GetFovTextureSize(session, ovrEyeType(eye), eyeFov, 1.0f));
#if OVR_PRODUCT_VERSION >= 1
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
    if (OVR_FAILURE(ovr_CreateTextureSwapChainGL(session,
      &colorDesc, &layerData.ColorTexture[eye]))) return false;

    // Oculus Rift 表示用の FBO のカラーバッファとして使うテクスチャセットを作成する
    int length(0);
    if (OVR_FAILURE(ovr_GetTextureSwapChainLength(session,
      layerData.ColorTexture[eye], &length))) return false;

    // テクスチャチェインの個々の要素について
    for (int i = 0; i < length; ++i)
    {
      // テクスチャのパラメータを設定する
      GLuint texId;
      ovr_GetTextureSwapChainBufferGL(session, layerData.ColorTexture[eye], i, &texId);
      glBindTexture(GL_TEXTURE_2D, texId);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // FBO のデプスバッファに使うテクスチャのパラメータを設定する
    glBindTexture(GL_TEXTURE_2D, oculus.oculusDepth[eye]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, size.w, size.h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    layerData.EyeFov.Viewport[eye].Pos = OVR::Vector2i(0, 0);
    layerData.EyeFov.Viewport[eye].Size = size;

    // Oculus Rift 表示用の FBO のカラーバッファとして使うテクスチャセットを作成する
    ovrSwapTextureSet *colorTexture;
    ovr_CreateSwapTextureSetGL(session, GL_SRGB8_ALPHA8, size.w, size.h, &colorTexture);
    layerData.EyeFov.ColorTexture[eye] = colorTexture;

    // Oculus Rift 表示用の FBO のデプスバッファとして使うテクスチャセットの作成
    ovrSwapTextureSet *depthTexture;
    ovr_CreateSwapTextureSetGL(session, GL_DEPTH_COMPONENT32F, size.w, size.h, &depthTexture);
    layerData.EyeFovDepth.DepthTexture[eye] = depthTexture;
#endif
  }

#if OVR_PRODUCT_VERSION >= 1
  // Oculus Rift の画面のアスペクト比を求める
  * aspect = static_cast<GLfloat>(layerData.Viewport[ovrEye_Left].Size.w)
    / static_cast<GLfloat>(layerData.Viewport[ovrEye_Left].Size.h);

  // ミラー表示用の FBO カラーバッファとして使うテクスチャの特性
  const ovrMirrorTextureDesc mirrorDesc =
  {
    OVR_FORMAT_R8G8B8A8_UNORM_SRGB,                   // Format
    oculus.mirrorWidth = defaults.display_size[0],    // Width
    oculus.mirrorHeight = defaults.display_size[1],   // Height
    0
  };

  // ミラー表示用の FBO のカラーバッファとして使うテクスチャを作成する
  if (OVR_FAILURE(ovr_CreateMirrorTextureGL(session,
    &mirrorDesc, &oculus.mirrorTexture))) return nullptr;

  // ミラー表示用の FBO のカラーバッファに使うテクスチャを得る
  GLuint texId;
  if (OVR_FAILURE(ovr_GetMirrorTextureBufferGL(session,
    oculus.mirrorTexture, &texId))) return nullptr;

  // ミラー表示用の FBO を作成する
  glGenFramebuffers(1, &oculus.mirrorFbo);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, oculus.mirrorFbo);
  glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
  glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

  // 姿勢のトラッキングにおける床の高さを 0 に設定する
  ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);
#else
  // Oculus Rift の画面のアスペクト比を求める
  *aspect = static_cast<GLfloat>(layerData.EyeFov.Viewport[ovrEye_Left].Size.w)
    / static_cast<GLfloat>(layerData.EyeFov.Viewport[ovrEye_Left].Size.h);

  // ミラー表示用の FBO のカラーバッファに使うテクスチャを作成する
  if (OVR_SUCCESS(ovr_CreateMirrorTextureGL(session, GL_SRGB8_ALPHA8,
    window.width, window.height, reinterpret_cast<ovrTexture **>(&oculus.mirrorTexture))))
  {
    // ミラー表示用の FBO を作成する
    glGenFramebuffers(1, &oculus.mirrorFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, oculus.mirrorFbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, oculus.mirrorTexture->OGL.TexId, 0);
    glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  }

  // TimeWarp に使う変換行列の成分を取り出す
  auto &posTimewarpProjectionDesc(layerData.EyeFovDepth.ProjectionDesc);
  posTimewarpProjectionDesc.Projection22 = (mp[ovrEye_Left].get()[4 * 2 + 2] + mp[ovrEye_Left].get()[4 * 3 + 2]) * 0.5f;
  posTimewarpProjectionDesc.Projection23 = mp[ovrEye_Left].get()[4 * 2 + 3] * 0.5f;
  posTimewarpProjectionDesc.Projection32 = mp[ovrEye_Left].get()[4 * 3 + 2];
#endif

  // ミラー表示はフロントバッファに描く
  glDrawBuffer(GL_FRONT);

  // Oculus Rift への表示ではスワップ間隔を待たない
  glfwSwapInterval(0);

  // Oculus Rift の初期化に成功した
  return &oculus;
}

//
// Oculus Rift を停止する.
//
void Oculus::terminate()
{
  // Oculus Rift のセッションが作成されていれば
  if (session != nullptr)
  {
    // ミラー表示用の FBO を削除する
    if (mirrorFbo)
    {
      glDeleteFramebuffers(1, &mirrorFbo);
      mirrorFbo = 0;
    }

    // ミラー表示に使ったテクスチャを開放する
    if (mirrorTexture)
    {
#if OVR_PRODUCT_VERSION >= 1
      ovr_DestroyMirrorTexture(session, mirrorTexture);
#else
      glDeleteTextures(1, &mirrorTexture->OGL.TexId);
      ovr_DestroyMirrorTexture(session, reinterpret_cast<ovrTexture *>(mirrorTexture));
#endif
      mirrorTexture = nullptr;
    }

    // Oculus Rift 表示用の FBO を削除する
    for (int eye = 0; eye < ovrEye_Count; ++eye)
    {
      // Oculus Rift のレンダリング用の FBO を削除する
      if (oculusFbo[eye] != 0)
      {
        glDeleteFramebuffers(eye, oculusFbo + eye);
        oculusFbo[eye] = 0;
      }

#if OVR_PRODUCT_VERSION >= 1
      // レンダリングターゲットに使ったテクスチャを開放する
      if (layerData.ColorTexture[eye])
      {
        ovr_DestroyTextureSwapChain(session, layerData.ColorTexture[eye]);
        layerData.ColorTexture[eye] = nullptr;
      }

      // デプスバッファとして使ったテクスチャを開放する
      if (oculusDepth[eye] != 0)
      {
        glDeleteTextures(eye, oculusDepth + eye);
        oculusDepth[eye] = 0;
      }
#else
      // レンダリングターゲットに使ったテクスチャを開放する
      auto *const colorTexture(layerData.EyeFov.ColorTexture[eye]);
      for (int i = 0; i < colorTexture->TextureCount; ++i)
      {
        const auto *const ctex(reinterpret_cast<ovrGLTexture *>(&colorTexture->Textures[i]));
        glDeleteTextures(1, &ctex->OGL.TexId);
      }
      ovr_DestroySwapTextureSet(session, colorTexture);

      // デプスバッファとして使ったテクスチャを開放する
      auto *const depthTexture(layerData.EyeFovDepth.DepthTexture[eye]);
      for (int i = 0; i < depthTexture->TextureCount; ++i)
      {
        const auto *const dtex(reinterpret_cast<ovrGLTexture *>(&depthTexture->Textures[i]));
        glDeleteTextures(1, &dtex->OGL.TexId);
      }
      ovr_DestroySwapTextureSet(session, depthTexture);
#endif
    }

    // Oculus Rift のセッションを破棄する
    ovr_Destroy(session);
    session = nullptr;
  }

  // 通常の表示はバックバッファに描く
  glDrawBuffer(GL_BACK);

  // 通常の表示はスワップ間隔を待つ
  glfwSwapInterval(1);
}

//
// Oculus Rift の透視投影変換行列を求める.
//
//   zoom シーンのズーム率.
//   mp 透視投影変換行列の配列.
//
void Oculus::getPerspective(GLfloat zoom, GgMatrix* mp) const
{
  // 前方面が defaults.display_near なのでスクリーンもそれに合わせる
  const GLfloat zf{ defaults.display_near / zoom };

  for (int eye = 0; eye < ovrEye_Count; ++eye)
  {
    // Oculus Rift の視野を取得する
    const auto &eyeFov(hmdDesc.DefaultEyeFov[eye]);

    // Oculus Rift の実際の視野
#if OVR_PRODUCT_VERSION >= 1
    const auto& fov(eyeFov);
#else
    // Oculus Rift のレンズ補正等の設定値を取得する
    auto& eyeRenderDesc{ oculus.eyeRenderDesc };
    eyeRenderDesc[eye] = ovr_GetRenderDesc(session, ovrEyeType(eye), eyeFov);

    // Oculus Rift の実際の視野
    const auto &fov(eyeRenderDesc.Fov);
#endif

    // 視差の調整値
    const float p{ static_cast<float>((1 - eye * 2) * attitude.parallax) * parallaxStep };

    // 片目の透視投影変換行列を求める
    mp[eye].loadFrustum(
      (p - fov.LeftTan) * zf, (p + fov.RightTan) * zf,
      -fov.DownTan * zf, fov.UpTan * zf,
      defaults.display_near, defaults.display_far
    );
  }
}

//
// Oculus Rift に表示する図形の描画を開始する.
//
//   mv それぞれの目の位置に平行移動する GgMatrix 型の変換行列.
//   戻り値 描画が可能なら VISIBLE, 不可能なら INVISIBLE, 終了要求があれば WANTQUIT.
//
enum Oculus::OculusStatus Oculus::start(GgMatrix *mv)
{
  // 既に Oculus Rift のセッションが作成されていないとおかしい
  assert(session != nullptr);

#if OVR_PRODUCT_VERSION >= 1
  // セッションの状態を取得する
  ovrSessionStatus sessionStatus;
  ovr_GetSessionStatus(session, &sessionStatus);

  // アプリケーションが終了を要求しているときはウィンドウのクローズフラグを立てる
  if (sessionStatus.ShouldQuit) return WANTQUIT;

  // 現在の状態をトラッキングの原点にする
  if (sessionStatus.ShouldRecenter) ovr_RecenterTrackingOrigin(session);

  // Oculus Rift に表示されていないときは戻る
  if (!sessionStatus.IsVisible) return INVISIBLE;

  // HmdToEyeOffset などは実行時に変化するので毎フレーム ovr_GetRenderDesc() で ovrEyeRenderDesc を取得する
  const ovrEyeRenderDesc eyeRenderDesc[] =
  {
    ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]),
    ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1])
  };

  // Oculus Rift の左右の目のトラッキングの位置からの変位を求める
  const ovrPosef hmdToEyePose[] =
  {
    eyeRenderDesc[0].HmdToEyePose,
    eyeRenderDesc[1].HmdToEyePose
  };

  // 視点の姿勢情報を取得する
  ovr_GetEyePoses(session, frameIndex, ovrTrue, hmdToEyePose, layerData.RenderPose, &layerData.SensorSampleTime);

  // Oculus Rift の両目の頭の位置からの変位を求める
  mv[0].loadTranslate(-hmdToEyePose[0].Position.x, -hmdToEyePose[0].Position.y, -hmdToEyePose[0].Position.z);
  mv[1].loadTranslate(-hmdToEyePose[1].Position.x, -hmdToEyePose[1].Position.y, -hmdToEyePose[1].Position.z);

#else
  // フレームのタイミング計測開始
  const auto ftiming(ovr_GetPredictedDisplayTime(session, 0));

  // sensorSampleTime の取得は可能な限り ovr_GetTrackingState() の近くで行う
  layerData.EyeFov.SensorSampleTime = ovr_GetTimeInSeconds();

  // ヘッドトラッキングの状態を取得する
  const auto hmdState(ovr_GetTrackingState(session, ftiming, ovrTrue));

  // 瞳孔間隔
  const ovrVector3f hmdToEyeViewOffset[] =
  {
    eyeRenderDesc[0].HmdToEyeViewOffset,
    eyeRenderDesc[1].HmdToEyeViewOffset
  };

  // 視点の姿勢情報を求める
  ovr_CalcEyePoses(hmdState.HeadPose.ThePose, hmdToEyeViewOffset, eyePose);
#endif

  return VISIBLE;
}

//
// 図形を描画する Oculus Rift の目を選択する.
//
//   eye 図形を描画する Oculus Rift の目.
//   po GgVecotr 型の変数で eye に指定した目の位置の同次座標が格納される.
//   qo GgQuaternion 型の変数で eye に指定した目の方向が四元数で格納される.
//
void Oculus::select(int eye, GgVector &po, GgQuaternion &qo)
{
  // 既に Oculus Rift のセッションが作成されていないとおかしい
  assert(session != nullptr);

#if OVR_PRODUCT_VERSION >= 1
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
    const auto &vp(layerData.Viewport[eye]);
    glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
  }

  // Oculus Rift の片目の位置と回転を取得する
  const auto &o(layerData.RenderPose[eye].Orientation);
  const auto &p(layerData.RenderPose[eye].Position);
#else
  // レンダーターゲットに描画する前にレンダーターゲットのインデックスをインクリメントする
  auto *const colorTexture(layerData.EyeFov.ColorTexture[eye]);
  colorTexture->CurrentIndex = (colorTexture->CurrentIndex + 1) % colorTexture->TextureCount;
  auto *const depthTexture(layerData.EyeFovDepth.DepthTexture[eye]);
  depthTexture->CurrentIndex = (depthTexture->CurrentIndex + 1) % depthTexture->TextureCount;

  // レンダーターゲットを切り替える
  glBindFramebuffer(GL_FRAMEBUFFER, oculusFbo[eye]);
  const auto &ctex(reinterpret_cast<ovrGLTexture *>(&colorTexture->Textures[colorTexture->CurrentIndex]));
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctex->OGL.TexId, 0);
  const auto &dtex(reinterpret_cast<ovrGLTexture *>(&depthTexture->Textures[depthTexture->CurrentIndex]));
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dtex->OGL.TexId, 0);

  // ビューポートを設定する
  const auto &vp(layerData.EyeFov.Viewport[eye]);
  glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);

  // Oculus Rift の片目の位置と回転を取得する
  const auto &p(eyePose[eye].Position);
  const auto &o(eyePose[eye].Orientation);
#endif

  // Oculus Rift の片目の位置を保存する
  po[0] = p.x;
  po[1] = p.y;
  po[2] = p.z;
  po[3] = 1.0f;

  // Oculus Rift の片目の回転を保存する
  qo = GgQuaternion(o.x, o.y, o.z, -o.w);

  // デプスバッファを消去する
  glClear(GL_DEPTH_BUFFER_BIT);
}

//
// Oculus Rift に表示する図形の描画を完了する.
//
//   eye 図形の描画を完了する Oculus Rift の目.
//
void Oculus::commit(int eye)
{
  // 既に Oculus Rift のセッションが作成されていないとおかしい
  assert(session != nullptr);

#if OVR_PRODUCT_VERSION >= 1
  // GL_COLOR_ATTACHMENT0 に割り当てられたテクスチャが wglDXUnlockObjectsNV() によって
  // アンロックされるために次のフレームの処理において無効な GL_COLOR_ATTACHMENT0 が
  // FBO に結合されるのを避ける
  glBindFramebuffer(GL_FRAMEBUFFER, oculusFbo[eye]);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

  // 保留中の変更を layerData.ColorTexture[eye] に反映しインデックスを更新する
  ovr_CommitTextureSwapChain(session, layerData.ColorTexture[eye]);
#endif
}

//
// 描画したフレームを Oculus Rift に転送する.
//
//   戻り値 Oculus Rift への転送が成功したら true.
//
bool Oculus::submit()
{
  // 既に Oculus Rift のセッションが作成されていないとおかしい
  assert(session != nullptr);

#if OVR_PRODUCT_VERSION >= 1
  // 描画データを Oculus Rift に転送する
  const auto *const layers(&layerData.Header);
  if (OVR_FAILURE(ovr_SubmitFrame(session, frameIndex++, nullptr, &layers, 1))) return false;
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
  const auto *const layers(&layerData.Header);
  if (OVR_FAILURE(ovr_SubmitFrame(session, 0, &viewScaleDesc, &layers, 1))) return false;
#endif

  // 通常のフレームバッファへの描画に戻す
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

  // Oculus Rift への転送に成功した
  return true;
}

//
// Oculus Rift に描画したフレームをミラー表示する.
//
//   width ミラー表示を行うフレームバッファ上の領域の幅.
//   height ミラー表示を行うフレームバッファ上の領域の高さ.
//
void Oculus::submitMirror(GLsizei width, GLsizei height)
{
  // 転送元の FBO の領域
#  if OVR_PRODUCT_VERSION >= 1
  const auto& sx1{ mirrorWidth };
  const auto& sy1{ mirrorHeight };
#  else
  const auto& sx1{ mirrorTexture->OGL.Header.TextureSize.w };
  const auto& sy1{ mirrorTexture->OGL.Header.TextureSize.h };
#  endif

  // ミラー表示を行うフレームバッファ上の領域
  GLint dx0{ 0 }, dx1{ width }, dy0{ 0 }, dy1{ height };

  // ミラー表示がウィンドウからはみ出ないようにする
  if ((width *= sy1) < (height *= sx1))
  {
    const GLint ty1{ width / sx1 };
    dy0 = (dy1 - ty1) / 2;
    dy1 = dy0 + ty1;
  }
  else
  {
    const GLint tx1{ height / sy1 };
    dx0 = (dx1 - tx1) / 2;
    dx1 = dx0 + tx1;
  }

  // レンダリング結果をミラー表示用のフレームバッファにも転送する
  glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFbo);
  glBlitFramebuffer(0, sy1, sx1, 0, dx0, dy0, dx1, dy1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

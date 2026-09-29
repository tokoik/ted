///
/// TED Quest 3 版 (実験用)
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
/// @details
/// Quest 3 を作業者 (WORKER) として動かす。
/// - パススルー映像 (XR_FB_passthrough) を背景に、ハンドトラッキング (XR_EXT_hand_tracking) による
///   自分の手のモデルを表示する。
/// - パススルーカメラ (Passthrough Camera API) の左右画像と頭部姿勢・手の関節姿勢を、
///   PC 版 TED と同じ形式で中継サーバまたは指示者 PC へ送る。
/// - 中継サーバまたは指示者 PC から受け取った指示者の手の関節姿勢で、
///   指示者の手のモデルを重畳表示する。
///

#include <jni.h>

#include <android/log.h>
#include <android_native_app_glue.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include <sys/stat.h>
#include <time.h>

#ifndef XR_USE_PLATFORM_ANDROID
#define XR_USE_PLATFORM_ANDROID
#endif
#ifndef XR_USE_GRAPHICS_API_OPENGL_ES
#define XR_USE_GRAPHICS_API_OPENGL_ES
#endif
#ifndef XR_USE_TIMESPEC
#define XR_USE_TIMESPEC
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#include "ObjModel.h"
#include "PassthroughCamera.h"
#include "QuestConfig.h"
#include "QuestMath.h"
#include "TedLink.h"
#include "TedProtocol.h"

#define LOG_TAG "TED"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace
{
  /// 設定ファイル名 (アプリ専用の外部ストレージに置く)
  constexpr const char* configFileName{ "ted_quest.json" };

  /// パススルーカメラの利用に必要な実行時パーミッション
  constexpr const char* cameraPermissions[]{ "android.permission.CAMERA", "horizonos.permission.HEADSET_CAMERA" };

  /// 片手の関節数と、送受信する変換行列テーブルの手の関節の開始位置
  constexpr int jointsPerHand{ TedLink::jointsPerHand };
  constexpr int firstJoint{ TedLink::camCount + 1 };

  /// 描画する手のモデル
  enum Model { HAND_R, HAND_L, FINGER, HAND_R_REMOTE, HAND_L_REMOTE, MODEL_COUNT };
  constexpr const char* modelFile[MODEL_COUNT]
  {
    "handr.obj", "handl.obj", "finger.obj", "handr_remote.obj", "handl_remote.obj"
  };

  /// 描画するクリップ面
  constexpr float zNear{ 0.05f }, zFar{ 10.0f };
}

struct Swapchain
{
  XrSwapchain handle{ XR_NULL_HANDLE };
  int width{ 0 };
  int height{ 0 };
  GLuint depthRenderbuffer{ 0 };
  std::vector<XrSwapchainImageOpenGLESKHR> images;
};

struct Engine
{
  struct android_app* app{ nullptr };
  JNIEnv* env{ nullptr };

  EGLDisplay display{ EGL_NO_DISPLAY };
  EGLContext context{ EGL_NO_CONTEXT };
  EGLSurface surface{ EGL_NO_SURFACE };
  EGLConfig config{ nullptr };

  bool windowInitialized{ false };
  bool sessionRunning{ false };

  XrInstance instance{ XR_NULL_HANDLE };
  XrSystemId systemId{ XR_NULL_SYSTEM_ID };
  XrSession session{ XR_NULL_HANDLE };
  XrSessionState sessionState{ XR_SESSION_STATE_UNKNOWN };
  XrSpace appSpace{ XR_NULL_HANDLE };
  XrSpace viewSpace{ XR_NULL_HANDLE };

  XrViewConfigurationType viewConfigType{ XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO };
  std::vector<XrViewConfigurationView> configViews;
  std::vector<Swapchain> swapchains;
  std::vector<XrView> views;
  bool srgbSwapchain{ false };

  GLuint framebuffer{ 0 };

  // 拡張機能の有無
  bool handTrackingSupported{ false };
  bool passthroughSupported{ false };
  bool convertTimeSupported{ false };

  // 拡張機能の関数
  PFN_xrInitializeLoaderKHR pfnInitializeLoaderKHR{ nullptr };
  PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR{ nullptr };
  PFN_xrCreateHandTrackerEXT pfnCreateHandTrackerEXT{ nullptr };
  PFN_xrDestroyHandTrackerEXT pfnDestroyHandTrackerEXT{ nullptr };
  PFN_xrLocateHandJointsEXT pfnLocateHandJointsEXT{ nullptr };
  PFN_xrCreatePassthroughFB pfnCreatePassthroughFB{ nullptr };
  PFN_xrDestroyPassthroughFB pfnDestroyPassthroughFB{ nullptr };
  PFN_xrCreatePassthroughLayerFB pfnCreatePassthroughLayerFB{ nullptr };
  PFN_xrDestroyPassthroughLayerFB pfnDestroyPassthroughLayerFB{ nullptr };
  PFN_xrConvertTimespecTimeToTimeKHR pfnConvertTimespecTimeToTimeKHR{ nullptr };

  // ハンドトラッカー (0: 左手, 1: 右手)
  XrHandTrackerEXT handTracker[2]{ XR_NULL_HANDLE, XR_NULL_HANDLE };

  // パススルー
  XrPassthroughFB passthrough{ XR_NULL_HANDLE };
  XrPassthroughLayerFB passthroughLayer{ XR_NULL_HANDLE };

  // 設定
  QuestConfig settings;

  // 通信
  TedLink link;

  // パススルーカメラ
  PassthroughCamera camera;
  bool permissionRequested{ false };
  std::chrono::steady_clock::time_point nextCameraRetry{};
  std::uint64_t lastCameraSequence{ 0 };

  // 動画の形式 (ted::IMAGE_JPEG なら JPEG を 1 枚ずつ送る)
  std::uint32_t videoFormat{ ted::IMAGE_JPEG };

  // 次にキーフレームを作ってよい時刻
  std::chrono::steady_clock::time_point nextKeyframe{};

  // 送信中の画像を撮影したときの頭部中心姿勢
  qm::Mat4 imagePose{ qm::identity() };
  bool imagePoseValid{ false };

  // 手のモデル
  ObjShader shader;
  ObjModel models[MODEL_COUNT];

  // 受信状態のログ出力用
  bool remoteActive{ false };
};

// ---------------------------------------------------------------------------
// 実行時パーミッション (JNI)
// ---------------------------------------------------------------------------

//
// パーミッションが許可されているか調べる
//
static bool hasPermission(Engine* engine, const char* permission)
{
  JNIEnv* const env{ engine->env };
  if (!env) return false;

  const jobject activity{ engine->app->activity->clazz };
  const jclass activityClass{ env->GetObjectClass(activity) };
  const jmethodID check{ env->GetMethodID(activityClass, "checkSelfPermission", "(Ljava/lang/String;)I") };
  const jstring name{ env->NewStringUTF(permission) };
  const jint result{ check ? env->CallIntMethod(activity, check, name) : -1 };
  if (env->ExceptionCheck()) env->ExceptionClear();
  env->DeleteLocalRef(name);
  env->DeleteLocalRef(activityClass);

  // PackageManager.PERMISSION_GRANTED == 0
  return result == 0;
}

//
// パーミッションを要求する (ヘッドセット内に許可ダイアログが出る)
//
static void requestPermissions(Engine* engine)
{
  JNIEnv* const env{ engine->env };
  if (!env) return;

  const jobject activity{ engine->app->activity->clazz };
  const jclass activityClass{ env->GetObjectClass(activity) };
  const jmethodID request{ env->GetMethodID(activityClass, "requestPermissions", "([Ljava/lang/String;I)V") };
  const jclass stringClass{ env->FindClass("java/lang/String") };
  const jsize count{ static_cast<jsize>(sizeof cameraPermissions / sizeof cameraPermissions[0]) };
  const jobjectArray array{ env->NewObjectArray(count, stringClass, nullptr) };
  for (jsize i = 0; i < count; ++i)
  {
    const jstring name{ env->NewStringUTF(cameraPermissions[i]) };
    env->SetObjectArrayElement(array, i, name);
    env->DeleteLocalRef(name);
  }
  if (request) env->CallVoidMethod(activity, request, array, 1);
  if (env->ExceptionCheck()) env->ExceptionClear();
  env->DeleteLocalRef(array);
  env->DeleteLocalRef(stringClass);
  env->DeleteLocalRef(activityClass);
}

// ---------------------------------------------------------------------------
// EGL
// ---------------------------------------------------------------------------

static bool initEGL(Engine* engine)
{
  engine->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (engine->display == EGL_NO_DISPLAY)
  {
    LOGE("eglGetDisplay failed");
    return false;
  }

  EGLint major{ 0 }, minor{ 0 };
  if (!eglInitialize(engine->display, &major, &minor))
  {
    LOGE("eglInitialize failed");
    return false;
  }
  LOGI("EGL Initialized: %d.%d", major, minor);

  const EGLint attribs[]
  {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 24,
    EGL_NONE
  };

  EGLint numConfigs{ 0 };
  if (!eglChooseConfig(engine->display, attribs, &engine->config, 1, &numConfigs) || numConfigs == 0)
  {
    LOGE("eglChooseConfig failed");
    return false;
  }

  const EGLint contextAttribs[]{ EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
  engine->context = eglCreateContext(engine->display, engine->config, EGL_NO_CONTEXT, contextAttribs);
  if (engine->context == EGL_NO_CONTEXT)
  {
    LOGE("eglCreateContext failed");
    return false;
  }

  engine->surface = eglCreateWindowSurface(engine->display, engine->config, engine->app->window, nullptr);
  if (engine->surface == EGL_NO_SURFACE)
  {
    LOGE("eglCreateWindowSurface failed");
    return false;
  }

  if (!eglMakeCurrent(engine->display, engine->surface, engine->surface, engine->context))
  {
    LOGE("eglMakeCurrent failed");
    return false;
  }

  LOGI("EGL surface and context bound successfully.");
  return true;
}

static void terminateEGL(Engine* engine)
{
  if (engine->display != EGL_NO_DISPLAY)
  {
    eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (engine->surface != EGL_NO_SURFACE)
    {
      eglDestroySurface(engine->display, engine->surface);
      engine->surface = EGL_NO_SURFACE;
    }
    if (engine->context != EGL_NO_CONTEXT)
    {
      eglDestroyContext(engine->display, engine->context);
      engine->context = EGL_NO_CONTEXT;
    }
    eglTerminate(engine->display);
    engine->display = EGL_NO_DISPLAY;
  }
  LOGI("EGL terminated.");
}

// ---------------------------------------------------------------------------
// 手のモデル
// ---------------------------------------------------------------------------

static void loadModels(Engine* engine)
{
  if (!engine->shader.create()) LOGE("hand shader creation failed");
  AAssetManager* const assets{ engine->app->activity->assetManager };
  for (int i = 0; i < MODEL_COUNT; ++i)
  {
    if (!engine->models[i].load(assets, modelFile[i])) LOGE("cannot load %s", modelFile[i]);
  }
}

static void destroyModels(Engine* engine)
{
  for (auto& model : engine->models) model.destroy();
  engine->shader.destroy();
}

// ---------------------------------------------------------------------------
// OpenXR
// ---------------------------------------------------------------------------

static void terminateOpenXR(Engine* engine);

template <typename T>
static void getProc(XrInstance instance, const char* name, T& function)
{
  if (XR_FAILED(xrGetInstanceProcAddr(instance, name, reinterpret_cast<PFN_xrVoidFunction*>(&function))))
    function = nullptr;
}

static bool initOpenXR(Engine* engine)
{
  // 1. Android 向けの OpenXR ローダを初期化する
  xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
    reinterpret_cast<PFN_xrVoidFunction*>(&engine->pfnInitializeLoaderKHR));
  if (engine->pfnInitializeLoaderKHR)
  {
    XrLoaderInitInfoAndroidKHR loaderInitInfo{ XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR };
    loaderInitInfo.applicationVM = engine->app->activity->vm;
    loaderInitInfo.applicationContext = engine->app->activity->clazz;
    const XrResult res{ engine->pfnInitializeLoaderKHR(
      reinterpret_cast<const XrLoaderInitInfoBaseHeaderKHR*>(&loaderInitInfo)) };
    if (XR_FAILED(res))
    {
      LOGE("xrInitializeLoaderKHR failed: %d", res);
      terminateOpenXR(engine);
      return false;
    }
  }
  else
  {
    LOGI("xrInitializeLoaderKHR symbol not present; proceeding with standard create info.");
  }

  // 2. インスタンス拡張機能を調べる
  uint32_t extCount{ 0 };
  xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCount, nullptr);
  std::vector<XrExtensionProperties> extensionProperties(extCount, { XR_TYPE_EXTENSION_PROPERTIES });
  xrEnumerateInstanceExtensionProperties(nullptr, extCount, &extCount, extensionProperties.data());
  const auto available = [&extensionProperties](const char* name)
  {
    for (const auto& ext : extensionProperties)
      if (std::strcmp(name, ext.extensionName) == 0) return true;
    return false;
  };

  std::vector<const char*> enabledExtensions;
  for (const char* required : { XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME, XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME })
  {
    if (!available(required))
    {
      LOGE("Required OpenXR extension NOT supported by runtime: %s", required);
      terminateOpenXR(engine);
      return false;
    }
    enabledExtensions.push_back(required);
  }

  // 使えれば使う拡張機能
  engine->handTrackingSupported = available(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
  engine->passthroughSupported = available(XR_FB_PASSTHROUGH_EXTENSION_NAME);
  engine->convertTimeSupported = available(XR_KHR_CONVERT_TIMESPEC_TIME_EXTENSION_NAME);
  if (engine->handTrackingSupported) enabledExtensions.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
  if (engine->passthroughSupported) enabledExtensions.push_back(XR_FB_PASSTHROUGH_EXTENSION_NAME);
  if (engine->convertTimeSupported) enabledExtensions.push_back(XR_KHR_CONVERT_TIMESPEC_TIME_EXTENSION_NAME);
  for (const char* name : enabledExtensions) LOGI("Enabled OpenXR extension: %s", name);

  // 3. インスタンスを作成する
  XrInstanceCreateInfoAndroidKHR createInfoAndroid{ XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR };
  createInfoAndroid.applicationVM = engine->app->activity->vm;
  createInfoAndroid.applicationActivity = engine->app->activity->clazz;

  XrInstanceCreateInfo createInfo{ XR_TYPE_INSTANCE_CREATE_INFO };
  createInfo.next = &createInfoAndroid;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
  createInfo.enabledExtensionNames = enabledExtensions.data();
  std::strncpy(createInfo.applicationInfo.applicationName, "TED OpenXR Native", XR_MAX_APPLICATION_NAME_SIZE);
  createInfo.applicationInfo.applicationVersion = 1;
  std::strncpy(createInfo.applicationInfo.engineName, "TED Engine", XR_MAX_ENGINE_NAME_SIZE);
  createInfo.applicationInfo.engineVersion = 1;
  createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

  XrResult res{ xrCreateInstance(&createInfo, &engine->instance) };
  if (XR_FAILED(res))
  {
    LOGE("xrCreateInstance failed: %d", res);
    terminateOpenXR(engine);
    return false;
  }
  LOGI("OpenXR Instance created successfully.");

  // 4. 拡張機能の関数を取得する
  getProc(engine->instance, "xrGetOpenGLESGraphicsRequirementsKHR", engine->pfnGetOpenGLESGraphicsRequirementsKHR);
  if (engine->handTrackingSupported)
  {
    getProc(engine->instance, "xrCreateHandTrackerEXT", engine->pfnCreateHandTrackerEXT);
    getProc(engine->instance, "xrDestroyHandTrackerEXT", engine->pfnDestroyHandTrackerEXT);
    getProc(engine->instance, "xrLocateHandJointsEXT", engine->pfnLocateHandJointsEXT);
  }
  if (engine->passthroughSupported)
  {
    getProc(engine->instance, "xrCreatePassthroughFB", engine->pfnCreatePassthroughFB);
    getProc(engine->instance, "xrDestroyPassthroughFB", engine->pfnDestroyPassthroughFB);
    getProc(engine->instance, "xrCreatePassthroughLayerFB", engine->pfnCreatePassthroughLayerFB);
    getProc(engine->instance, "xrDestroyPassthroughLayerFB", engine->pfnDestroyPassthroughLayerFB);
  }
  if (engine->convertTimeSupported)
  {
    getProc(engine->instance, "xrConvertTimespecTimeToTimeKHR", engine->pfnConvertTimespecTimeToTimeKHR);
  }

  // 5. システムを取得する
  XrSystemGetInfo systemInfo{ XR_TYPE_SYSTEM_GET_INFO };
  systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  res = xrGetSystem(engine->instance, &systemInfo, &engine->systemId);
  if (XR_FAILED(res))
  {
    LOGE("xrGetSystem failed: %d", res);
    terminateOpenXR(engine);
    return false;
  }
  LOGI("OpenXR System ID obtained: %llu", static_cast<unsigned long long>(engine->systemId));

  // ハンドトラッキングにシステムが対応しているか調べる
  if (engine->handTrackingSupported)
  {
    XrSystemHandTrackingPropertiesEXT handProperties{ XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT };
    XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES };
    systemProperties.next = &handProperties;
    if (XR_FAILED(xrGetSystemProperties(engine->instance, engine->systemId, &systemProperties))
      || !handProperties.supportsHandTracking)
    {
      LOGW("Hand tracking is not supported by the system.");
      engine->handTrackingSupported = false;
    }
  }

  // 6. OpenGL ES の要件を確認する
  if (engine->pfnGetOpenGLESGraphicsRequirementsKHR)
  {
    XrGraphicsRequirementsOpenGLESKHR graphicsReq{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR };
    res = engine->pfnGetOpenGLESGraphicsRequirementsKHR(engine->instance, engine->systemId, &graphicsReq);
    if (XR_FAILED(res))
    {
      LOGE("xrGetOpenGLESGraphicsRequirementsKHR failed: %d", res);
      terminateOpenXR(engine);
      return false;
    }
  }

  // 7. セッションを作成する
  XrGraphicsBindingOpenGLESAndroidKHR graphicsBinding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR };
  graphicsBinding.display = engine->display;
  graphicsBinding.config = engine->config;
  graphicsBinding.context = engine->context;

  XrSessionCreateInfo sessionCreateInfo{ XR_TYPE_SESSION_CREATE_INFO };
  sessionCreateInfo.next = &graphicsBinding;
  sessionCreateInfo.systemId = engine->systemId;
  res = xrCreateSession(engine->instance, &sessionCreateInfo, &engine->session);
  if (XR_FAILED(res))
  {
    LOGE("xrCreateSession failed: %d", res);
    terminateOpenXR(engine);
    return false;
  }
  LOGI("OpenXR Session created successfully.");

  // 8. 基準空間 (STAGE, 使えなければ LOCAL) と頭部 (VIEW) の空間を作成する
  XrReferenceSpaceCreateInfo spaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
  spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  spaceCreateInfo.poseInReferenceSpace.orientation.w = 1.0f;
  res = xrCreateReferenceSpace(engine->session, &spaceCreateInfo, &engine->appSpace);
  if (XR_FAILED(res))
  {
    LOGW("STAGE space failed; falling back to LOCAL space...");
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    res = xrCreateReferenceSpace(engine->session, &spaceCreateInfo, &engine->appSpace);
    if (XR_FAILED(res))
    {
      LOGE("xrCreateReferenceSpace LOCAL failed: %d", res);
      terminateOpenXR(engine);
      return false;
    }
  }
  spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  res = xrCreateReferenceSpace(engine->session, &spaceCreateInfo, &engine->viewSpace);
  if (XR_FAILED(res))
  {
    LOGE("xrCreateReferenceSpace VIEW failed: %d", res);
    terminateOpenXR(engine);
    return false;
  }
  LOGI("OpenXR Reference Space created.");

  // ハンドトラッカーを作成する (PC 版と同じく 0 が左手, 1 が右手)
  if (engine->handTrackingSupported && engine->pfnCreateHandTrackerEXT)
  {
    for (int hand = 0; hand < 2; ++hand)
    {
      XrHandTrackerCreateInfoEXT createInfo{ XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
      createInfo.hand = hand == 0 ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT;
      createInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
      if (XR_FAILED(engine->pfnCreateHandTrackerEXT(engine->session, &createInfo, &engine->handTracker[hand])))
      {
        LOGW("xrCreateHandTrackerEXT failed for hand %d", hand);
        engine->handTracker[hand] = XR_NULL_HANDLE;
      }
    }
  }

  // パススルーを開始する (失敗してもパススルー無しで続ける)
  if (engine->settings.passthrough && engine->passthroughSupported
    && engine->pfnCreatePassthroughFB && engine->pfnCreatePassthroughLayerFB)
  {
    XrPassthroughCreateInfoFB passthroughInfo{ XR_TYPE_PASSTHROUGH_CREATE_INFO_FB };
    passthroughInfo.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
    res = engine->pfnCreatePassthroughFB(engine->session, &passthroughInfo, &engine->passthrough);
    if (XR_SUCCEEDED(res))
    {
      XrPassthroughLayerCreateInfoFB layerInfo{ XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB };
      layerInfo.passthrough = engine->passthrough;
      layerInfo.flags = XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB;
      layerInfo.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB;
      res = engine->pfnCreatePassthroughLayerFB(engine->session, &layerInfo, &engine->passthroughLayer);
      if (XR_FAILED(res))
      {
        LOGW("xrCreatePassthroughLayerFB failed: %d", res);
        engine->passthroughLayer = XR_NULL_HANDLE;
      }
    }
    else
    {
      LOGW("xrCreatePassthroughFB failed: %d", res);
      engine->passthrough = XR_NULL_HANDLE;
    }
  }
  LOGI("Passthrough %s", engine->passthroughLayer != XR_NULL_HANDLE ? "enabled" : "disabled");

  // 9. スワップチェーンを作成する (sRGB が使えれば sRGB にする)
  uint32_t formatCount{ 0 };
  xrEnumerateSwapchainFormats(engine->session, 0, &formatCount, nullptr);
  std::vector<int64_t> formats(formatCount);
  xrEnumerateSwapchainFormats(engine->session, formatCount, &formatCount, formats.data());
  int64_t colorFormat{ GL_RGBA8 };
  for (const auto format : formats)
  {
    if (format == GL_SRGB8_ALPHA8)
    {
      colorFormat = GL_SRGB8_ALPHA8;
      break;
    }
  }
  engine->srgbSwapchain = colorFormat == GL_SRGB8_ALPHA8;

  uint32_t viewCount{ 0 };
  xrEnumerateViewConfigurationViews(engine->instance, engine->systemId, engine->viewConfigType, 0, &viewCount, nullptr);
  engine->configViews.resize(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
  xrEnumerateViewConfigurationViews(engine->instance, engine->systemId, engine->viewConfigType, viewCount, &viewCount,
    engine->configViews.data());
  engine->views.resize(viewCount, { XR_TYPE_VIEW });

  engine->swapchains.resize(viewCount);
  for (uint32_t i = 0; i < viewCount; ++i)
  {
    XrSwapchainCreateInfo swapchainCreateInfo{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
    swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
    swapchainCreateInfo.format = colorFormat;
    swapchainCreateInfo.sampleCount = 1;
    swapchainCreateInfo.width = engine->configViews[i].recommendedImageRectWidth;
    swapchainCreateInfo.height = engine->configViews[i].recommendedImageRectHeight;
    swapchainCreateInfo.faceCount = 1;
    swapchainCreateInfo.arraySize = 1;
    swapchainCreateInfo.mipCount = 1;

    engine->swapchains[i].width = static_cast<int>(swapchainCreateInfo.width);
    engine->swapchains[i].height = static_cast<int>(swapchainCreateInfo.height);

    res = xrCreateSwapchain(engine->session, &swapchainCreateInfo, &engine->swapchains[i].handle);
    if (XR_FAILED(res))
    {
      LOGE("xrCreateSwapchain[%u] failed: %d", i, res);
      terminateOpenXR(engine);
      return false;
    }

    uint32_t imgCount{ 0 };
    xrEnumerateSwapchainImages(engine->swapchains[i].handle, 0, &imgCount, nullptr);
    engine->swapchains[i].images.resize(imgCount, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR });
    xrEnumerateSwapchainImages(engine->swapchains[i].handle, imgCount, &imgCount,
      reinterpret_cast<XrSwapchainImageBaseHeader*>(engine->swapchains[i].images.data()));

    // スワップチェーンと同じ大きさのデプスバッファ
    glGenRenderbuffers(1, &engine->swapchains[i].depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, engine->swapchains[i].depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, engine->swapchains[i].width,
      engine->swapchains[i].height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    LOGI("Swapchain[%u] created (%dx%d, %s), image count: %u", i, engine->swapchains[i].width,
      engine->swapchains[i].height, engine->srgbSwapchain ? "sRGB" : "RGBA8", imgCount);
  }

  // 10. フレームバッファオブジェクト
  glGenFramebuffers(1, &engine->framebuffer);

  return true;
}

static void terminateOpenXR(Engine* engine)
{
  if (engine->framebuffer)
  {
    glDeleteFramebuffers(1, &engine->framebuffer);
    engine->framebuffer = 0;
  }
  for (auto& sc : engine->swapchains)
  {
    if (sc.depthRenderbuffer != 0)
    {
      glDeleteRenderbuffers(1, &sc.depthRenderbuffer);
      sc.depthRenderbuffer = 0;
    }
    if (sc.handle != XR_NULL_HANDLE)
    {
      xrDestroySwapchain(sc.handle);
      sc.handle = XR_NULL_HANDLE;
    }
  }
  engine->swapchains.clear();
  if (engine->passthroughLayer != XR_NULL_HANDLE && engine->pfnDestroyPassthroughLayerFB)
    engine->pfnDestroyPassthroughLayerFB(engine->passthroughLayer);
  engine->passthroughLayer = XR_NULL_HANDLE;
  if (engine->passthrough != XR_NULL_HANDLE && engine->pfnDestroyPassthroughFB)
    engine->pfnDestroyPassthroughFB(engine->passthrough);
  engine->passthrough = XR_NULL_HANDLE;
  for (auto& tracker : engine->handTracker)
  {
    if (tracker != XR_NULL_HANDLE && engine->pfnDestroyHandTrackerEXT) engine->pfnDestroyHandTrackerEXT(tracker);
    tracker = XR_NULL_HANDLE;
  }
  if (engine->viewSpace != XR_NULL_HANDLE)
  {
    xrDestroySpace(engine->viewSpace);
    engine->viewSpace = XR_NULL_HANDLE;
  }
  if (engine->appSpace != XR_NULL_HANDLE)
  {
    xrDestroySpace(engine->appSpace);
    engine->appSpace = XR_NULL_HANDLE;
  }
  if (engine->session != XR_NULL_HANDLE)
  {
    xrDestroySession(engine->session);
    engine->session = XR_NULL_HANDLE;
  }
  if (engine->instance != XR_NULL_HANDLE)
  {
    xrDestroyInstance(engine->instance);
    engine->instance = XR_NULL_HANDLE;
  }
  engine->sessionRunning = false;
  engine->sessionState = XR_SESSION_STATE_UNKNOWN;
  LOGI("OpenXR resources destroyed.");
}

static void pollOpenXREvents(Engine* engine)
{
  XrEventDataBuffer eventData{ XR_TYPE_EVENT_DATA_BUFFER };
  while (xrPollEvent(engine->instance, &eventData) == XR_SUCCESS)
  {
    if (eventData.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
    {
      const auto* const stateEvent{ reinterpret_cast<XrEventDataSessionStateChanged*>(&eventData) };
      LOGI("Session state changed from %d to %d", engine->sessionState, stateEvent->state);
      engine->sessionState = stateEvent->state;

      if (engine->sessionState == XR_SESSION_STATE_READY)
      {
        XrSessionBeginInfo beginInfo{ XR_TYPE_SESSION_BEGIN_INFO };
        beginInfo.primaryViewConfigurationType = engine->viewConfigType;
        const XrResult res{ xrBeginSession(engine->session, &beginInfo) };
        if (XR_SUCCEEDED(res))
        {
          engine->sessionRunning = true;
          LOGI("xrBeginSession succeeded.");
        }
        else
        {
          LOGE("xrBeginSession failed: %d", res);
        }
      }
      else if (engine->sessionState == XR_SESSION_STATE_STOPPING)
      {
        engine->sessionRunning = false;
        xrEndSession(engine->session);
        LOGI("xrEndSession completed.");
      }
      else if (engine->sessionState == XR_SESSION_STATE_EXITING || engine->sessionState == XR_SESSION_STATE_LOSS_PENDING)
      {
        engine->sessionRunning = false;
        if (engine->app && engine->app->activity)
        {
          LOGI("Requesting ANativeActivity_finish due to session exit/loss.");
          ANativeActivity_finish(engine->app->activity);
        }
      }
    }
    eventData = { XR_TYPE_EVENT_DATA_BUFFER };
  }
}

// ---------------------------------------------------------------------------
// トラッキング
// ---------------------------------------------------------------------------

//
// 指定した時刻の頭部中心姿勢 (VIEW 空間) を求める
//
static bool locateHead(Engine* engine, XrTime time, qm::Mat4& pose)
{
  XrSpaceLocation location{ XR_TYPE_SPACE_LOCATION };
  if (XR_FAILED(xrLocateSpace(engine->viewSpace, engine->appSpace, time, &location))) return false;
  constexpr XrSpaceLocationFlags required{ XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_VALID_BIT };
  if ((location.locationFlags & required) != required) return false;
  pose = qm::fromPose(location.pose);
  return true;
}

//
// パススルーカメラの撮影時刻 (CLOCK_MONOTONIC のナノ秒) を OpenXR の時刻に変換する
//
static bool convertTime(Engine* engine, std::int64_t nanoseconds, XrTime& time)
{
  if (!engine->pfnConvertTimespecTimeToTimeKHR || nanoseconds <= 0) return false;
  timespec ts{};
  ts.tv_sec = static_cast<time_t>(nanoseconds / 1000000000LL);
  ts.tv_nsec = static_cast<long>(nanoseconds % 1000000000LL);
  return XR_SUCCEEDED(engine->pfnConvertTimespecTimeToTimeKHR(engine->instance, &ts, &time));
}

//
// 片手の関節姿勢を PC 版 (GgApp::OpenXR::updateOpenXRHands) と同じ方法で求める
//
// @param hand OpenXR の手 (0: 左手, 1: 右手)
// @param matrices 基準空間における 22 個の関節の変換行列の格納先
// @return 求められたら true
//
static bool locateHand(Engine* engine, int hand, XrTime time, std::array<qm::Mat4, jointsPerHand>& matrices)
{
  if (engine->handTracker[hand] == XR_NULL_HANDLE || !engine->pfnLocateHandJointsEXT) return false;

  constexpr std::array<XrHandJointEXT, jointsPerHand> jointMap
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

  constexpr std::array<XrHandJointEXT, jointsPerHand - 2> boneStartMap
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

  std::array<XrHandJointLocationEXT, XR_HAND_JOINT_COUNT_EXT> locations{};
  XrHandJointLocationsEXT joints{ XR_TYPE_HAND_JOINT_LOCATIONS_EXT };
  joints.jointCount = static_cast<uint32_t>(locations.size());
  joints.jointLocations = locations.data();
  XrHandJointsLocateInfoEXT locateInfo{ XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT };
  locateInfo.baseSpace = engine->appSpace;
  locateInfo.time = time;
  if (XR_FAILED(engine->pfnLocateHandJointsEXT(engine->handTracker[hand], &locateInfo, &joints)) || !joints.isActive)
    return false;

  for (const auto& joint : locations)
    if ((joint.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) == 0) return false;

  using Vec3 = std::array<float, 3>;
  const auto sub = [](const XrVector3f& a, const XrVector3f& b) { return Vec3{ a.x - b.x, a.y - b.y, a.z - b.z }; };
  const auto dot = [](const Vec3& a, const Vec3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; };
  const auto cross = [](const Vec3& a, const Vec3& b)
  {
    return Vec3{ a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };
  };
  constexpr float minimumAxisLengthSquared{ 1.0e-8f };
  const auto normalize = [&dot](Vec3& v)
  {
    const float lengthSquared{ dot(v, v) };
    if (lengthSquared < minimumAxisLengthSquared) return false;
    const float length{ std::sqrt(lengthSquared) };
    for (auto& x : v) x /= length;
    return true;
  };
  const auto makeMatrix = [](const XrVector3f& p, const Vec3& x, const Vec3& y, const Vec3& z)
  {
    return qm::Mat4{
      x[0], x[1], x[2], 0.0f,
      y[0], y[1], y[2], 0.0f,
      z[0], z[1], z[2], 0.0f,
      p.x, p.y, p.z, 1.0f };
  };

  const auto& wrist{ locations[XR_HAND_JOINT_WRIST_EXT].pose.position };
  const auto& middle{ locations[XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT].pose.position };
  const auto& index{ locations[XR_HAND_JOINT_INDEX_METACARPAL_EXT].pose.position };
  const auto& little{ locations[XR_HAND_JOINT_LITTLE_METACARPAL_EXT].pose.position };
  const auto& palm{ locations[XR_HAND_JOINT_PALM_EXT].pose.position };

  // 手のひらの座標軸 (左右で同じ向きになるよう, 右手は x 軸の向きを反転する)
  const float side{ hand == 0 ? 1.0f : -1.0f };
  Vec3 palmX{ sub(index, little) };
  for (auto& x : palmX) x *= side;
  Vec3 palmY{ sub(middle, wrist) };
  if (!normalize(palmX) || !normalize(palmY)) return false;
  Vec3 palmZ{ cross(palmX, palmY) };
  if (!normalize(palmZ)) return false;
  palmY = cross(palmZ, palmX);
  matrices[0] = makeMatrix(palm, palmX, palmY, palmZ);

  // 手首から手のひらへ向かう軸と手のひらの法線から手首の座標軸を作る
  Vec3 wristZ{ sub(palm, wrist) };
  if (!normalize(wristZ)) return false;
  const float wristDot{ dot(palmZ, wristZ) };
  Vec3 wristY{ palmZ[0] - wristDot * wristZ[0], palmZ[1] - wristDot * wristZ[1], palmZ[2] - wristDot * wristZ[2] };
  if (!normalize(wristY)) return false;
  matrices[1] = makeMatrix(wrist, cross(wristY, wristZ), wristY, wristZ);

  // 指の骨は始点から終点へ向かう軸を z 軸にし, Leap Motion と同じく骨の終点に置く
  for (std::size_t joint = 0; joint < boneStartMap.size(); ++joint)
  {
    const auto& startPos{ locations[boneStartMap[joint]].pose.position };
    const auto& endPos{ locations[jointMap[joint + 2]].pose.position };
    Vec3 boneZ{ sub(endPos, startPos) };
    if (!normalize(boneZ)) return false;
    const float d{ dot(palmZ, boneZ) };
    Vec3 boneY{ palmZ[0] - d * boneZ[0], palmZ[1] - d * boneZ[1], palmZ[2] - d * boneZ[2] };
    if (!normalize(boneY)) return false;
    matrices[joint + 2] = makeMatrix(endPos, cross(boneY, boneZ), boneY, boneZ);
  }

  return true;
}

// ---------------------------------------------------------------------------
// パススルーカメラ
// ---------------------------------------------------------------------------

//
// パーミッションを確認してパススルーカメラを開始 (切断時は再開) する
//
static void serviceCamera(Engine* engine)
{
  if (!engine->windowInitialized || !engine->sessionRunning) return;
  if (!engine->settings.send_images || engine->camera.isRunning()) return;

  const auto now{ std::chrono::steady_clock::now() };
  if (now < engine->nextCameraRetry) return;
  engine->nextCameraRetry = now + std::chrono::seconds(3);

  bool granted{ true };
  for (const char* permission : cameraPermissions) granted = granted && hasPermission(engine, permission);
  if (!granted)
  {
    if (!engine->permissionRequested)
    {
      LOGI("requesting passthrough camera permissions");
      requestPermissions(engine);
      engine->permissionRequested = true;
    }
    return;
  }
  engine->permissionRequested = false;

  const auto& s{ engine->settings };
  PassthroughCamera::VideoSettings video;
  video.format = engine->videoFormat;
  video.bitrate = s.bitrate;
  video.keyframeInterval = s.keyframe_interval;
  if (!engine->camera.open(s.camera_width, s.camera_height, s.transmit_quality, s.transmit_fps, video))
  {
    LOGW("passthrough camera is not available; retrying");
  }
}

// ---------------------------------------------------------------------------
// 描画
// ---------------------------------------------------------------------------

//
// 変換行列テーブルの手の関節にモデルを描く
//
// @param parent 手の関節の親の変換行列 (基準空間)
// @param table 変換行列テーブル
// @param remote 指示者の手なら true (色違いのモデルを使う)
//
static void drawHands(Engine* engine, const qm::Mat4& projection, const qm::Mat4& view, const qm::Mat4& parent,
  const qm::Mat4* table, bool remote)
{
  const qm::Mat4 mv{ view * parent };
  for (int hand = 0; hand < 2; ++hand)
  {
    for (int joint = 0; joint < jointsPerHand; ++joint)
    {
      const qm::Mat4& m{ table[firstJoint + joint * 2 + hand] };
      if (qm::isZero(m)) continue;

      // 手のひらは左右の手のモデル, それ以外の関節は指のモデル (PC 版の hand.json と同じ)
      Model model{ FINGER };
      if (joint == 0) model = hand == 0 ? (remote ? HAND_R_REMOTE : HAND_R) : (remote ? HAND_L_REMOTE : HAND_L);
      engine->models[model].draw(engine->shader, projection, mv * m);
    }
  }
}

static void renderFrame(Engine* engine)
{
  if (!engine->sessionRunning) return;

  XrFrameWaitInfo waitInfo{ XR_TYPE_FRAME_WAIT_INFO };
  XrFrameState frameState{ XR_TYPE_FRAME_STATE };
  XrResult res{ xrWaitFrame(engine->session, &waitInfo, &frameState) };
  if (XR_FAILED(res))
  {
    LOGE("xrWaitFrame failed: %d", res);
    return;
  }

  XrFrameBeginInfo beginInfo{ XR_TYPE_FRAME_BEGIN_INFO };
  res = xrBeginFrame(engine->session, &beginInfo);
  if (XR_FAILED(res))
  {
    LOGE("xrBeginFrame failed: %d", res);
    return;
  }

  const XrTime displayTime{ frameState.predictedDisplayTime };
  std::vector<XrCompositionLayerBaseHeader*> layers;

  // パススルー映像を最背面に置く
  XrCompositionLayerPassthroughFB passthroughLayer{ XR_TYPE_COMPOSITION_LAYER_PASSTHROUGH_FB };
  if (engine->passthroughLayer != XR_NULL_HANDLE)
  {
    passthroughLayer.flags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    passthroughLayer.space = XR_NULL_HANDLE;
    passthroughLayer.layerHandle = engine->passthroughLayer;
    layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&passthroughLayer));
  }

  XrCompositionLayerProjection projectionLayer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
  std::vector<XrCompositionLayerProjectionView> projectionViews;

  // 現在の頭部中心姿勢
  qm::Mat4 head{ qm::identity() };
  const bool headValid{ locateHead(engine, displayTime, head) };

  // 手の関節姿勢 (基準空間) を求める。PC 版と同じく OpenXR の左手 (0) をテーブルの 1 に、右手 (1) を 0 に置く
  std::array<qm::Mat4, jointsPerHand> handMatrices[2];
  bool handValid[2]{ false, false };
  for (int xrHand = 0; xrHand < 2; ++xrHand)
    handValid[1 - xrHand] = locateHand(engine, xrHand, displayTime, handMatrices[1 - xrHand]);

  // 基準の姿勢 (画像を撮影したときの頭部中心姿勢) から変換行列テーブルを作る
  const auto makeTable = [&handMatrices, &handValid](const qm::Mat4& reference)
  {
    const qm::Mat4 worldToReference{ qm::invertRigid(reference) };
    std::array<qm::Mat4, TedLink::tableSize> table;
    table[0] = table[1] = reference;
    table[2] = qm::identity();
    for (int hand = 0; hand < 2; ++hand)
      for (int joint = 0; joint < jointsPerHand; ++joint)
        table[firstJoint + joint * 2 + hand] = handValid[hand]
        ? worldToReference * handMatrices[hand][joint] : qm::zero();
    return table;
  };

  // カメラの撮影時刻の頭部中心姿勢を求める
  const auto poseAt = [engine, headValid, &head](std::int64_t timestamp, qm::Mat4& pose)
  {
    XrTime captureTime{ 0 };
    if (convertTime(engine, timestamp, captureTime) && locateHead(engine, captureTime, pose)) return true;
    pose = head;
    return headValid;
  };

  PassthroughCamera::Frame frame;
  bool newImage{ false };
  if (engine->settings.send_images && engine->camera.isVideo())
  {
    // 動画はアクセスユニットごとに、撮影時の姿勢のテーブルと一緒に登録順に送る
    std::vector<PassthroughCamera::VideoUnit> units;
    engine->camera.takeVideoUnits(units);
    for (auto& unit : units)
    {
      qm::Mat4 pose;
      if (!poseAt(unit.timestamp, pose)) continue;
      engine->imagePose = pose;
      engine->imagePoseValid = true;
      const auto table{ makeTable(pose) };
      engine->link.publishVideo(table.data(), unit.eye, engine->videoFormat, std::move(unit.payload));
    }

    // 受信側の要求や送信待ちの溢れでキーフレームを作る (要求が続いても 300ms に 1 回にする)
    const bool request{ engine->link.takeKeyframeRequest() };
    const bool overflow{ engine->link.takeOverflow() };
    const auto now{ std::chrono::steady_clock::now() };
    if ((request || overflow) && now >= engine->nextKeyframe)
    {
      engine->camera.requestKeyframe();
      engine->nextKeyframe = now + std::chrono::milliseconds(300);
    }
  }
  else if (engine->settings.send_images)
  {
    // JPEG は新しい画像の組があれば、撮影時の姿勢と同じフレームで送る
    newImage = engine->camera.getLatest(frame, engine->lastCameraSequence);
    if (newImage)
    {
      engine->lastCameraSequence = frame.sequence;
      qm::Mat4 pose;
      if (poseAt(frame.timestamp, pose))
      {
        engine->imagePose = pose;
        engine->imagePoseValid = true;
      }
    }
  }

  // 一定間隔で送る姿勢の基準 (画像を送っていれば最後の画像の撮影時, そうでなければ現在の頭部中心姿勢)
  const qm::Mat4 reference{ engine->imagePoseValid ? engine->imagePose : head };
  const auto table{ makeTable(reference) };
  if (headValid || engine->imagePoseValid)
    engine->link.publish(table.data(), newImage ? frame.jpeg[0] : nullptr, newImage ? frame.jpeg[1] : nullptr);

  // 受信した指示者の変換行列
  std::array<qm::Mat4, TedLink::maxTableSize> remote{};
  int remoteCount{ 0 };
  const bool remoteActive{ engine->link.getRemote(remote.data(), remoteCount)
    && remoteCount >= TedLink::tableSize };
  if (remoteActive != engine->remoteActive)
  {
    LOGI("instructor data %s", remoteActive ? "receiving" : "lost");
    engine->remoteActive = remoteActive;
  }

  // 指示者の手は、指示者の頭部に対する手の姿勢を Quest 3 の頭部に対する姿勢として重畳する
  const auto& offset{ engine->settings.remote_hand_position };
  const qm::Mat4 remoteParent{ head * qm::translate(offset[0], offset[1], offset[2]) * remote[2] };

  if (frameState.shouldRender && headValid)
  {
    XrViewLocateInfo locateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
    locateInfo.viewConfigurationType = engine->viewConfigType;
    locateInfo.displayTime = displayTime;
    locateInfo.space = engine->appSpace;

    XrViewState viewState{ XR_TYPE_VIEW_STATE };
    uint32_t viewCount{ static_cast<uint32_t>(engine->views.size()) };
    res = xrLocateViews(engine->session, &locateInfo, &viewState, viewCount, &viewCount, engine->views.data());
    constexpr XrViewStateFlags required{ XR_VIEW_STATE_ORIENTATION_VALID_BIT | XR_VIEW_STATE_POSITION_VALID_BIT };
    if (XR_FAILED(res) || (viewState.viewStateFlags & required) != required)
    {
      // トラッキングを失っているときは描画を省略する
      viewCount = 0;
    }

    projectionViews.resize(viewCount, { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW });
    for (uint32_t i = 0; i < viewCount; ++i)
    {
      auto& swapchain{ engine->swapchains[i] };
      uint32_t imgIndex{ 0 };
      XrSwapchainImageAcquireInfo acquireInfo{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
      xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &imgIndex);

      XrSwapchainImageWaitInfo waitImageInfo{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
      waitImageInfo.timeout = XR_INFINITE_DURATION;
      xrWaitSwapchainImage(swapchain.handle, &waitImageInfo);

      projectionViews[i].pose = engine->views[i].pose;
      projectionViews[i].fov = engine->views[i].fov;
      projectionViews[i].subImage.swapchain = swapchain.handle;
      projectionViews[i].subImage.imageRect.offset = { 0, 0 };
      projectionViews[i].subImage.imageRect.extent = { swapchain.width, swapchain.height };

      glBindFramebuffer(GL_FRAMEBUFFER, engine->framebuffer);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, swapchain.images[imgIndex].image, 0);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, swapchain.depthRenderbuffer);
      glViewport(0, 0, swapchain.width, swapchain.height);

      // パススルーを使うときは透明で消去して手のモデルだけを重ねる
      const float alpha{ engine->passthroughLayer != XR_NULL_HANDLE ? 0.0f : 1.0f };
      glClearColor(0.0f, 0.0f, 0.0f, alpha);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glEnable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
      glDisable(GL_BLEND);

      const qm::Mat4 projection{ qm::projection(engine->views[i].fov, zNear, zFar) };
      const qm::Mat4 view{ qm::invertRigid(qm::fromPose(engine->views[i].pose)) };

      engine->shader.use();
      engine->shader.setGamma(engine->srgbSwapchain ? 2.2f : 1.0f);

      // 自分の手 (送信するテーブルと同じ姿勢)
      if (engine->settings.show_local_hands) drawHands(engine, projection, view, reference, table.data(), false);

      // 指示者の手
      if (engine->settings.show_remote_hands && remoteActive)
        drawHands(engine, projection, view, remoteParent, remote.data(), true);

      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      XrSwapchainImageReleaseInfo releaseInfo{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
      xrReleaseSwapchainImage(swapchain.handle, &releaseInfo);
    }

    if (viewCount > 0)
    {
      projectionLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
      projectionLayer.space = engine->appSpace;
      projectionLayer.viewCount = viewCount;
      projectionLayer.views = projectionViews.data();
      layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&projectionLayer));
    }
  }

  XrFrameEndInfo endInfo{ XR_TYPE_FRAME_END_INFO };
  endInfo.displayTime = displayTime;
  endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  endInfo.layerCount = frameState.shouldRender ? static_cast<uint32_t>(layers.size()) : 0;
  endInfo.layers = layers.data();
  res = xrEndFrame(engine->session, &endInfo);
  if (XR_FAILED(res)) LOGE("xrEndFrame failed: %d", res);
}

// ---------------------------------------------------------------------------
// アプリケーション
// ---------------------------------------------------------------------------

static void handleAppCmd(struct android_app* app, int32_t cmd)
{
  Engine* const engine{ reinterpret_cast<Engine*>(app->userData) };
  switch (cmd)
  {
  case APP_CMD_INIT_WINDOW:
    LOGI("APP_CMD_INIT_WINDOW received");
    if (app->window != nullptr && !engine->windowInitialized)
    {
      if (initEGL(engine) && initOpenXR(engine))
      {
        loadModels(engine);
        engine->windowInitialized = true;
      }
    }
    break;
  case APP_CMD_TERM_WINDOW:
    LOGI("APP_CMD_TERM_WINDOW received");
    engine->camera.close();
    engine->nextCameraRetry = {};
    engine->windowInitialized = false;
    destroyModels(engine);
    terminateOpenXR(engine);
    terminateEGL(engine);
    break;
  case APP_CMD_GAINED_FOCUS:
  case APP_CMD_RESUME:
    engine->permissionRequested = false;
    engine->nextCameraRetry = {};
    break;
  case APP_CMD_DESTROY:
    LOGI("APP_CMD_DESTROY received");
    break;
  default:
    break;
  }
}

//
// 設定ファイルを読み込む (無ければ既定値で作る)
//
static void loadSettings(Engine* engine)
{
  const char* const directory{ engine->app->activity->externalDataPath };
  if (!directory)
  {
    LOGW("no external data path; using default settings");
    return;
  }

  mkdir(directory, 0770);
  const std::string path{ std::string(directory) + "/" + configFileName };
  if (engine->settings.load(path))
  {
    LOGI("settings loaded from %s", path.c_str());
  }
  else if (engine->settings.save(path))
  {
    LOGI("default settings written to %s (edit with adb pull / push)", path.c_str());
  }
  else
  {
    LOGW("cannot read or write %s; using default settings", path.c_str());
  }
}

void android_main(struct android_app* app)
{
  Engine engine{};
  engine.app = app;
  app->userData = &engine;
  app->onAppCmd = handleAppCmd;

  // パーミッションの確認と要求に使う JNI 環境
  app->activity->vm->AttachCurrentThread(&engine.env, nullptr);

  LOGI("TED OpenXR Native app started via android_main");

  // 設定を読み込んで通信を開始する
  loadSettings(&engine);
  engine.videoFormat = engine.settings.codec == "hevc" ? ted::IMAGE_HEVC
    : engine.settings.codec == "jpeg" ? ted::IMAGE_JPEG : ted::IMAGE_H264;
  if (!engine.link.start(engine.settings.host, static_cast<unsigned short>(engine.settings.port),
    engine.settings.send_interval))
  {
    LOGE("network is not available; check host and port in %s", configFileName);
  }

  while (app->destroyRequested == 0)
  {
    int events{ 0 };
    struct android_poll_source* source{ nullptr };

    // 描画中は待たずに、そうでなければカメラの再開などのため 100ms ごとに戻る
    const int timeout{ engine.windowInitialized && engine.sessionRunning ? 0 : 100 };
    while (ALooper_pollOnce(timeout, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0)
    {
      if (source != nullptr) source->process(app, source);
      if (app->destroyRequested != 0) break;
    }

    if (engine.instance != XR_NULL_HANDLE) pollOpenXREvents(&engine);

    serviceCamera(&engine);

    if (engine.windowInitialized && engine.sessionRunning) renderFrame(&engine);
  }

  engine.camera.close();
  engine.link.stop();
  destroyModels(&engine);
  terminateOpenXR(&engine);
  terminateEGL(&engine);
  app->activity->vm->DetachCurrentThread();
  LOGI("TED OpenXR Native app exited successfully.");
}

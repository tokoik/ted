#pragma once

/*

ゲームグラフィックス特論用補助プログラム GLFW3 版

Copyright (c) 2011-2025 Kohe Tokoi. All Rights Reserved.

Permission is hereby granted, free of charge,  to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction,  including without limitation the rights
to use, copy,  modify, merge,  publish, distribute,  sublicense,  and/or sell
copies or substantial portions of the Software.

The above  copyright notice  and this permission notice  shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS IS",  WITHOUT WARRANTY OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING  BUT  NOT LIMITED  TO THE WARRANTIES  OF MERCHANTABILITY,
FITNESS  FOR  A PARTICULAR PURPOSE  AND NONINFRINGEMENT.  IN  NO EVENT  SHALL
KOHE TOKOI  BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER LIABILITY,  WHETHER IN
AN ACTION  OF CONTRACT,  TORT  OR  OTHERWISE,  ARISING  FROM,  OUT OF  OR  IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

///
/// アプリケーションクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///

// Dear ImGui を使うなら
#if !defined(GG_USE_IMGUI) && !defined(GG_NO_IMGUI)
#  if defined(__has_include)
#    if __has_include(<imgui.h>) || __has_include("imgui.h")
#      define GG_USE_IMGUI
#    endif
#  endif
#endif

// 使用するマウスのボタン数
#if !defined(GG_BUTTON_COUNT)
#  define GG_BUTTON_COUNT 3
#endif

// 使用するユーザインタフェースの数
#if !defined(GG_INTERFACE_COUNT)
#  define GG_INTERFACE_COUNT 5
#endif

// 補助プログラム
#include "gg.h"
using namespace gg;

// 各種設定
#include "Config.h"

// カメラ関連の処理
#include "Camera.h"

// 標準ライブラリ
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <iostream>
#include <stdexcept>

// ImGui の組み込み
#if defined(GG_USE_IMGUI)
#  include "imgui.h"
#  include "imgui_impl_glfw.h"
#  include "imgui_impl_opengl3.h"
#endif

// メッセージボックス
#if defined(_WIN32)
int showNotification(const char* message);
#  define NOTIFY(msg) showNotification(msg)
#else
#  define NOTIFY(msg) std::cerr << msg << '\n'
#endif

// OpenXR ライブラリの組み込み
#define GG_USE_OPENXR
#if defined(GG_USE_OPENXR)
#  if defined(_WIN32)
#    define XR_USE_PLATFORM_WIN32
#    define XR_USE_GRAPHICS_API_OPENGL
#    define GLFW_EXPOSE_NATIVE_WIN32
#    define GLFW_EXPOSE_NATIVE_WGL
#    include <GLFW/glfw3native.h>
#    include <windows.h>
#    include <unknwn.h>
#    if defined(_MSC_VER)
#      if defined(_DEBUG)
#        pragma comment(lib, "openxr_loaderd.lib")
#      else
#        pragma comment(lib, "openxr_loader.lib")
#      endif
#    endif
#  else
#    if !defined(__gl_h_)
#      define __gl_h_
#    endif
#    define XR_USE_PLATFORM_XLIB
#    define XR_USE_GRAPHICS_API_OPENGL
#    define GLFW_EXPOSE_NATIVE_X11
#    define GLFW_EXPOSE_NATIVE_GLX
#    include <GLFW/glfw3native.h>
#  endif
#  include <openxr/openxr.h>
#  include <openxr/openxr_platform.h>
#  include <algorithm>
#  include <cstdio>
#  include <cstring>
#  include <utility>
#endif

///
/// アプリケーションクラス
///
class GgApp
{
  /// 入力切替時に古いバックエンドを自動破棄するため、現在のCamera派生インスタンスを共有所有する
  std::shared_ptr<Camera> camera{ nullptr };

  /// 静止画入力だけが初期テクスチャ作成時に渡すCPU画像。動画・カメラではnullptrになる。
  const GLubyte *image[camCount]{ nullptr, nullptr };

  /// 背景画像のサイズ
  GLsizei size[camCount][2]{ { 0, 0 }, { 0, 0 } };

  /// 背景画像のアスペクト比
  GLfloat aspect[camCount]{ 1.0f, 1.0f };

  /// 背景画像を保存するテクスチャ
  GLuint texture[camCount]{ 0, 0 };

  /// 左右に独立した入力がある場合true。falseなら右眼も左テクスチャを共有する。
  bool stereo{ false };

  /// 静止画像ファイルを使う
  bool useImage();

  /// 動画像ファイルを使う
  bool useMovie();

  /// Web カメラを使う
  bool useCamera();

  /// Ovrvision Pro を使う
  bool useOvervision();

  /// リモートの TED から取得する
  bool useRemote();

public:

  ///
  /// コンストラクタ
  GgApp() = default;

  ///
  /// コピー・代入の禁止
  ///
  GgApp(const GgApp&) = delete;
  GgApp& operator=(const GgApp&) = delete;
  GgApp(GgApp&&) = default;
  GgApp& operator=(GgApp&&) = default;

  ///
  /// デストラクタ
  ///
  virtual ~GgApp();

  ///
  /// 入力ソースを選択する
  ///
  bool selectInput();

  ///
  /// ハンドトラッキングの使用状態を変更する
  ///
  bool setHandTrackingMode(int mode);

  ///
  /// アプリケーション本体
  ///
  int main(int argc, const char* const* argv);

  ///
  /// ウィンドウ関連の処理を担当するクラス
  ///
  class Window
  {
    /// ウィンドウの識別子
    GLFWwindow* window{ nullptr };

    /// ウィンドウのサイズ
    std::array<GLsizei, 2> size{ 0, 0 };

    /// フレームバッファの横幅と高さ
    std::array<GLsizei, 2> fboSize{ 0, 0 };

#if defined(IMGUI_VERSION)
    /// メニューバーの高さ
    GLsizei menubarHeight{ 0 };
#endif

    /// ビューポートの横幅と高さ
    int width{ 0 }, height{ 0 };

    /// ビューポートの縦横比
    GLfloat aspect{ 1.0f };

    /// マウスの移動速度[X/Y/Z]
    std::array<GLfloat, 3> velocity{ 1.0f, 1.0f, 0.1f };

    /// マウスボタンの状態
    std::array<bool, GG_BUTTON_COUNT> status{};

    /// ユーザインタフェースのデータ構造
    struct HumanInterface
    {
      int lastKey{ 0 };
      std::array<std::array<int, 2>, 4> arrow{};
      std::array<GLfloat, 2> mouse{};
      std::array<GLfloat, 2> wheel{};
      std::array<std::array<gg::GgVector, 2>, GG_BUTTON_COUNT> translation{};
      std::array<gg::GgTrackball, GG_BUTTON_COUNT> rotation;

      HumanInterface()
      {
        resetTranslation();
      }

      void resetTranslation();
      void calcTranslation(int button, const std::array<GLfloat, 3>& velocity);
    };

    std::array<HumanInterface, GG_INTERFACE_COUNT> interfaceData;
    int interfaceNo{ 0 };

    void* userPointer{ nullptr };
    void (*resizeFunc)(const Window* window, int width, int height){ nullptr };
    void (*keyboardFunc)(const Window* window, int key, int scancode, int action, int mods){ nullptr };
    void (*mouseFunc)(const Window* window, int button, int action, int mods){ nullptr };
    void (*wheelFunc)(const Window* window, double x, double y){ nullptr };

    // コールバック関数
    static void resize(GLFWwindow* window, int width, int height);
    static void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse(GLFWwindow* window, int button, int action, int mods);
    static void wheel(GLFWwindow* window, double x, double y);

    //
    // TED 固有の背景および座標変換パラメータ
    //

    std::array<GLsizei, 2> samples{ 0, 0 };
    std::array<GLfloat, 2> gap{ 0.0f, 0.0f };
    int key{ GLFW_KEY_UNKNOWN };
    int joy{ -1 };
    std::array<float, 4> origin{ 0.0f, 0.0f, 0.0f, 0.0f };
    double cx{ 0.0 }, cy{ 0.0 };
    Camera* camera{ nullptr };

    std::array<GgQuaternion, camCount> qo{ ggIdentityQuaternion(), ggIdentityQuaternion() };
    std::array<GgVector, camCount> po{ GgVector{ 0.0f, 0.0f, 0.0f, 1.0f }, GgVector{ 0.0f, 0.0f, 0.0f, 1.0f } };
    std::array<GgMatrix, camCount> mo{ ggIdentity(), ggIdentity() };
    GgMatrix mm{ ggIdentity() };
    std::array<GgMatrix, camCount> mv{ ggIdentity(), ggIdentity() };
    std::array<GgMatrix, camCount> mp{ ggIdentity(), ggIdentity() };
    GLfloat zoom{ 1.0f };

    GgVector circle{ 0.0f, 0.0f, 0.0f, 1.0f };
    std::array<GgVector, camCount> screen{ GgVector{ 0.0f, 0.0f, 0.0f, 1.0f }, GgVector{ 0.0f, 0.0f, 0.0f, 1.0f } };
    GLfloat focal{ 1.0f };
    GLfloat parallax{ defaultParallax };
    GLfloat offset{ 0.0f };

    friend class Rect;

    bool showScene{ true };
    bool showMirror{ true };
    bool showMenu{ true };

  public:

    ///
    /// コンストラクタ
    ///
    Window(int width = 640, int height = 480,
      const char* title = "GLFW Window",
      GLFWmonitor* monitor = nullptr,
      GLFWwindow* share = nullptr);

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&& w) noexcept;
    Window& operator=(Window&& w) noexcept;
    virtual ~Window();

    GLFWwindow* get() const { return window; }
    void setClose(int close = GLFW_TRUE) const { glfwSetWindowShouldClose(window, close); }
    int shouldClose() const { return glfwWindowShouldClose(window); }
    explicit operator bool();
    void swapBuffers();

    void reset();
    void resetViewport() { resize(window, size[0], size[1]); }
    bool setDisplayMode(int mode);
    bool isQuadBufferAvailable() const { return window && glfwGetWindowAttrib(window, GLFW_STEREO) == GLFW_TRUE; }
    bool setClipPlanes(float nearPlane, float farPlane);

    bool isMirrorVisible() const { return showMirror; }
    void setMirrorVisible(bool visible) { showMirror = visible; }
    bool isSceneVisible() const { return showScene; }
    void setSceneVisible(bool visible) { showScene = visible; }
    bool isMenuVisible() const { return showMenu; }
    void setMenuVisible(bool visible) { showMenu = visible; }

    void update();
    void updateCircle();
    void setControlCamera(Camera* cam) { camera = cam; }

    const GgMatrix& getMm() const { return mm; }
    const GgMatrix& getMv(int eye) const { return mv[eye]; }
    const GgVector& getPo(int eye) const { return po[eye]; }
    const GgQuaternion& getQo(int eye) const { return qo[eye]; }
    const GgMatrix& getMo(int eye) const { return mo[eye]; }
    const GgMatrix& getMp(int eye) const { return mp[eye]; }
    const auto& getSamples() const { return samples; }
    const auto& getGap() const { return gap; }
    const auto& getScreen(int eye) const { return screen[eye]; }
    auto getFocal() const { return focal; }
    auto getOffset(int eye = 0) const { return static_cast<GLfloat>(1 - (eye & 1) * 2) * offset; }
    const auto& getCircle() const { return circle; }
    const auto& getSize() const { return size; }
    const auto& getFboSize() const { return fboSize; }
    auto getAspect() const { return aspect; }

    bool startHMD();
    void stopHMD();

    /// ビューポートを元のサイズに復帰する
    void restoreViewport() const
    {
      if (!glfwGetWindowAttrib(window, GLFW_ICONIFIED)) glViewport(0, 0, fboSize[0], fboSize[1]);
    }

    /// デスクトップ表示用描画制御
    bool start();
    void select(int eye);
    void commit(int eye);
  };

#if defined(GG_USE_OPENXR)
  ///
  /// OpenXR 関連の処理.
  ///
  /// @note
  /// OpenXR を操作するラッパークラス（シングルトン）.
  /// OpenGL のコンテキストを使うので, GgApp::Window を作成した後に
  /// initialize() を呼び, ウィンドウを破棄する前に terminate() を呼ぶこと.
  ///
  class OpenXR
  {
  public:

    ///
    /// コントローラーの手の識別子.
    ///
    enum Hand
    {
      Left = 0,   ///< 左手
      Right = 1,  ///< 右手
      Count = 2   ///< 手の総数
    };

  private:

    // OpenXR のインスタンスとシステム
    XrInstance instance{ XR_NULL_HANDLE };
    XrSystemId systemId{ XR_NULL_SYSTEM_ID };

    // OpenXR のシステムの名前
    std::string systemName;

    // OpenXR のセッション
    XrSession session{ XR_NULL_HANDLE };
    XrSessionState sessionState{ XR_SESSION_STATE_UNKNOWN };

    // OpenXR の参照空間
    XrSpace appSpace{ XR_NULL_HANDLE };
    XrReferenceSpaceType referenceSpaceType{ XR_REFERENCE_SPACE_TYPE_STAGE };

    // ビュー設定とステート
    std::vector<XrViewConfigurationView> views;
    std::vector<XrView> viewStates;

    // OpenXR のスワップチェーン
    std::vector<XrSwapchain> swapchains;
    std::vector<std::vector<XrSwapchainImageOpenGLKHR>> swapchainImages;

    // OpenXR へのレンダリングに使う FBO とデプスバッファ (ビューの数だけ確保する)
    std::vector<GLuint> openxrFbo;
    std::vector<GLuint> openxrDepth;

    // スワップチェーンのカラーフォーマットが sRGB なら true
    bool swapchainIsSrgb{ true };

    // 環境の合成方法
    XrEnvironmentBlendMode blendMode{ XR_ENVIRONMENT_BLEND_MODE_OPAQUE };

    // フレームの同期状態
    XrFrameState frameState{ XR_TYPE_FRAME_STATE };
    bool isSessionRunning{ false };

    // xrBeginFrame() を呼んで xrEndFrame() を呼んでいない状態なら true
    bool frameBegun{ false };

    // 視点の姿勢が取得できていれば true
    bool viewPoseValid{ false };

    // 初期化が完了していれば true
    bool initialized{ false };

    // 各フレームで取得したスワップチェーンイメージのインデックス
    std::vector<uint32_t> currentImageIndex;

    // スワップチェーンイメージを取得中のビューなら true
    std::vector<bool> imageAcquired;

    // ミラー表示を行うビューの番号, ミラー表示を行わないなら -1
    int mirrorView{ 0 };

    // OpenXR のミラー表示を行うウィンドウ
    const Window* window{ nullptr };

    // アクションセット
    XrActionSet actionSet{ XR_NULL_HANDLE };

    // アクション定義
    XrAction aimPoseAction{ XR_NULL_HANDLE };
    XrAction gripPoseAction{ XR_NULL_HANDLE };
    XrAction triggerAction{ XR_NULL_HANDLE };
    XrAction gripAction{ XR_NULL_HANDLE };
    XrAction thumbstickAction{ XR_NULL_HANDLE };
    XrAction thumbstickClickAction{ XR_NULL_HANDLE };
    XrAction primaryButtonAction{ XR_NULL_HANDLE };
    XrAction secondaryButtonAction{ XR_NULL_HANDLE };
    XrAction menuButtonAction{ XR_NULL_HANDLE };
    XrAction hapticAction{ XR_NULL_HANDLE };

    // コントローラーのスペース
    XrSpace aimSpace[Hand::Count]{ XR_NULL_HANDLE, XR_NULL_HANDLE };
    XrSpace gripSpace[Hand::Count]{ XR_NULL_HANDLE, XR_NULL_HANDLE };

    // コントローラーの状態キャッシュ
    struct ControllerState
    {
      bool isTracked{ false };
      XrPosef gripPose{ { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } };
      XrPosef aimPose{ { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } };
      float trigger{ 0.0f };
      float grip{ 0.0f };
      std::array<float, 2> thumbstick{ 0.0f, 0.0f };
      bool thumbstickClick{ false };
      bool primaryButton{ false };
      bool secondaryButton{ false };
      bool menuButton{ false };
    };
    ControllerState controllerStates[Hand::Count];

    // サポートするパスの文字列
    XrPath handSubactionPath[Hand::Count]{ XR_NULL_PATH, XR_NULL_PATH };

    // アクションシステムを初期化する
    void initActions();

    // アクション状態を更新する
    void pollActions();

    // OpenXR のイベントを処理する
    void pollEvents();

    // スワップチェーンを作成する
    void createSwapchains();

    // 描画中のフレームを合成器に転送する
    void endFrame();

    // OpenXR のハンドルを破棄する (OpenGL の資源には触れない)
    void destroyXr();

    // ミラー表示を行う
    void blitMirror() const;

    // ハンドトラッキング拡張 (XR_EXT_hand_tracking)
    bool xrHandTrackingSupported{ false };
    XrHandTrackerEXT xrHandTracker[Hand::Count]{ XR_NULL_HANDLE, XR_NULL_HANDLE };
    XrHandJointLocationEXT xrJointLocations[Hand::Count][XR_HAND_JOINT_COUNT_EXT]{};
    XrHandJointLocationsEXT xrLocations[Hand::Count]{};
    PFN_xrCreateHandTrackerEXT xrCreateHandTracker{ nullptr };
    PFN_xrDestroyHandTrackerEXT xrDestroyHandTracker{ nullptr };
    PFN_xrLocateHandJointsEXT xrLocateHandJoints{ nullptr };

    // 頭部中心姿勢
    GgVector xrOriginPosition{ 0.0f, 0.0f, 0.0f, 1.0f };
    bool xrOriginValid{ false };

    // コンストラクタ / デストラクタ
    OpenXR();
    virtual ~OpenXR();

  public:

    OpenXR(const OpenXR&) = delete;
    OpenXR& operator=(const OpenXR&) = delete;
    OpenXR(OpenXR&&) = delete;
    OpenXR& operator=(OpenXR&&) = delete;

    static OpenXR& initialize(const Window& window,
      XrReferenceSpaceType spaceType = XR_REFERENCE_SPACE_TYPE_STAGE,
      const char* appName = "TED");
    static OpenXR& getInstance();

    void terminate();
    bool begin();
    void select(int eye);
    void select(int eye, GLfloat* screen, GLfloat* position, GLfloat* orientation);
    void commit(int eye);
    bool submit(bool mirror = true);

    void setMirror(int eye);
    int getMirror() const;
    bool isRunning() const;
    bool isFocused() const;
    const std::string& getSystemName() const;

    gg::GgMatrix getProjectionMatrix(int eye, GLfloat zNear = 0.1f, GLfloat zFar = 100.0f) const;
    gg::GgMatrix getViewMatrix(int eye) const;
    gg::GgMatrix getPoseMatrix(int eye) const;
    gg::GgVector getPosition(int eye) const;
    gg::GgQuaternion getOrientation(int eye) const;
    const XrFovf& getFov(int eye) const;
    const XrPosef& getPose(int eye) const;
    bool isPoseValid() const;

    GLsizei getWidth(int eye = 0) const;
    GLsizei getHeight(int eye = 0) const;
    GLfloat getAspect(int eye = 0) const;
    uint32_t getViewCount() const;
    XrReferenceSpaceType getReferenceSpaceType() const;
    XrTime getPredictedDisplayTime() const { return frameState.predictedDisplayTime; }

    // コントローラー
    bool isTracked(int hand) const;
    gg::GgMatrix getGripMatrix(int hand) const;
    gg::GgMatrix getAimMatrix(int hand) const;
    gg::GgVector getGripPosition(int hand) const;
    gg::GgQuaternion getGripOrientation(int hand) const;
    gg::GgVector getAimPosition(int hand) const;
    gg::GgQuaternion getAimOrientation(int hand) const;
    float getTrigger(int hand) const;
    float getGrip(int hand) const;
    std::array<float, 2> getThumbstick(int hand) const;
    bool getThumbstickClick(int hand) const;
    bool getPrimaryButton(int hand) const;
    bool getSecondaryButton(int hand) const;
    bool getMenuButton(int hand = Hand::Left) const;
    void applyHapticVibration(int hand, float durationSeconds = 0.1f, float frequency = XR_FREQUENCY_UNSPECIFIED, float amplitude = 0.5f);

    // ハンドトラッキング (XR_EXT_hand_tracking)
    bool hasHandTracking() const { return xrHandTrackingSupported; }
    void updateOpenXRHands(XrTime time);

    // 頭部中心姿勢
    GgVector getHeadPosition() const;
    GgQuaternion getHeadOrientation() const;
    GgMatrix getHeadPoseMatrix() const;
    const GgVector& getOriginPosition() const { return xrOriginPosition; }
    void setOriginPosition(const GgVector& pos) { xrOriginPosition = pos; xrOriginValid = true; }
  };
#endif

  static std::string getUsername();
};
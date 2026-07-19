#pragma once

/*
ゲームグラフィックス特論用補助プログラム GLFW3 版
Copyright (c) 2011-2025 Kohe Tokoi. All Rights Reserved.
*/

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

// メッセージボックス
#if defined(_WIN32)
#  define NOTIFY(msg) MessageBoxA(NULL, msg, "TED", MB_ICONERROR | MB_OK)
#else
#  define NOTIFY(msg) std::cerr << msg << '\n'
#endif

// OpenXR SDK の組み込み
#define GG_USE_OPENXR
#if defined(GG_USE_OPENXR)
#  if defined(_WIN32)
#    define GLFW_EXPOSE_NATIVE_WIN32
#    define GLFW_EXPOSE_NATIVE_WGL
#    include <GLFW/glfw3native.h>
#    define XR_USE_PLATFORM_WIN32
#    define XR_USE_GRAPHICS_API_OPENGL
#    include <openxr/openxr.h>
#    include <openxr/openxr_platform.h>
#    // #pragma comment(lib, "openxr_loader.lib")
#  endif
#endif

///
/// ゲームグラフィックス特論宿題アプリケーションクラス.
///
class GgApp
{
  // 背景画像を取得するカメラ
  std::shared_ptr<Camera> camera{ nullptr };

  // 背景画像のデータ
  const GLubyte *image[camCount]{ nullptr, nullptr };

  // 背景画像のサイズ
  GLsizei size[camCount][2]{ { 0, 0 }, { 0, 0 } };

  // 背景画像のアスペクト比
  GLfloat aspect[camCount]{ 1.0f, 1.0f };

  // 背景画像を保存するテクスチャ
  GLuint texture[camCount]{ 0, 0 };

  // ステレオカメラなら true
  bool stereo{ false };

  //
  // 静止画像ファイルを使う
  //
  bool useImage();

  //
  // 動画像ファイルを使う
  //
  bool useMovie();

  //
  // Web カメラを使う
  //
  bool useCamera();

  //
  // Ovrvision Pro を使う
  //
  bool useOvervision();



  //
  // リモートの TED から取得する
  //
  bool useRemote();

public:

  //
  // コンストラクタ
  //
  GgApp() = default;

  //
  // デストラクタ
  //
  virtual ~GgApp();

  //
  // 入力ソースを選択する
  //
  bool selectInput();

  //
  // アプリケーション本体
  //
  int main(int argc, const char* const* argv);

  //
  // ウィンドウ関連の処理を担当するクラス
  //
  class Window
  {
    // ウィンドウの識別子
    GLFWwindow* const window{ nullptr };

    // ウィンドウのサイズ
    std::array<GLsizei, 2> size{ 0, 0 };

    // ビューポートの幅と高さ
    int width{ 0 }, height{ 0 };

    // ビューポートのアスペクト比
    GLfloat aspect{ 1.0f };

    // メッシュの縦横の格子点数
    std::array<GLsizei, 2> samples{ 0, 0 };

    // メッシュの縦横の格子間隔
    std::array<GLfloat, 2> gap{ 0.0f, 0.0f };

    // 最後にタイプしたキー
    int key{ GLFW_KEY_UNKNOWN };

    // ジョイスティックの番号
    int joy{ -1 };

    // スティックの中立位置
    std::array<float, 4> origin{ 0.0f, 0.0f, 0.0f, 0.0f };

    // ドラッグ開始位置
    double cx{ 0.0 }, cy{ 0.0 };

    // このウィンドウで制御するカメラ
    Camera* camera{ nullptr };

    //
    // ヘッドトラッキング
    //

    // ヘッドトラッキングによる回転
    std::array<GgQuaternion, camCount> qo
    {
      ggIdentityQuaternion(),
      ggIdentityQuaternion()
    };

    // ヘッドトラッキングによる位置
    std::array<GgVector, camCount> po
    {
      GgVector{ 0.0f, 0.0f, 0.0f, 1.0f },
      GgVector{ 0.0f, 0.0f, 0.0f, 1.0f }
    };

    // ヘッドトラッキングの変換行列
    std::array<GgMatrix, camCount> mo
    {
      ggIdentity(),
      ggIdentity()
    };

    //
    // 座標変換
    //

    // モデル変換行列
    GgMatrix mm{ ggIdentity() };

    // ビュー変換行列
    std::array<GgMatrix, camCount> mv
    {
      ggIdentity(),
      ggIdentity()
    };

    // 投影変換行列
    std::array<GgMatrix, camCount> mp
    {
      ggIdentity(),
      ggIdentity()
    };

    // ズーム率
    GLfloat zoom{ 1.0f };

    //
    // 背景画像
    //

    // 背景テクスチャの半径と中心
    GgVector circle{ 0.0f, 0.0f, 0.0f, 1.0f };

    // スクリーンの幅と高さ
    std::array<GgVector, camCount> screen
    {
      GgVector{ 0.0f, 0.0f, 0.0f, 1.0f },
      GgVector{ 0.0f, 0.0f, 0.0f, 1.0f }
    };

    // 焦点距離
    GLfloat focal{ 1.0f };

    // 視差
    GLfloat parallax{ defaultParallax };

    // スクリーンの間隔
    GLfloat offset{ 0.0f };

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
      return glfwWindowShouldClose(window);
    }

    //
    // イベントを取得してループを継続するなら真を返す
    //
    explicit operator bool();

    //
    // カラーバッファを入れ替えてイベントを取り出す
    //
    void swapBuffers();

    //
    // ウィンドウのサイズ変更時の処理
    //
    static void resize(GLFWwindow* window, int width, int height);

    //
    // マウスボタンを操作したときの処理
    //
    static void mouse(GLFWwindow* window, int button, int action, int mods);

    //
    // マウスホイール操作時の処理
    //
    static void wheel(GLFWwindow* window, double x, double y);

    //
    // キーボードをタイプした時の処理
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
    // ヘッドラッキングによる移動を得る
    //
    const GgVector& getPo(int eye) const
    {
      return po[eye];
    }

    //
    // ヘッドラッキングによる回転の四元数を得る
    //
    const GgQuaternion& getQo(int eye) const
    {
      return qo[eye];
    }

    //
    // ヘッドラッキングによる回転の変換行列を得る
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
    bool showScene{ true };

    // ミラー表示
    bool showMirror{ true };

    // メニュー表示
    bool showMenu{ true };

#if defined(GG_USE_OPENXR)
  private:
    // OpenXR 関連のメンバ
    XrInstance xrInstance{ XR_NULL_HANDLE };
    XrSystemId xrSystemId{ XR_NULL_SYSTEM_ID };
    XrSession xrSession{ XR_NULL_HANDLE };
    XrSpace xrPlaySpace{ XR_NULL_HANDLE };
    XrSpace xrViewSpace{ XR_NULL_HANDLE };

    // スワップチェーン関連
    struct OpenXRSwapchain
    {
      XrSwapchain handle{ XR_NULL_HANDLE };
      uint32_t width{ 0 };
      uint32_t height{ 0 };
      std::vector<XrSwapchainImageOpenGLKHR> images;
      std::vector<GLuint> fbos;
      uint32_t activeImageIndex{ 0 };
    };
    std::array<OpenXRSwapchain, 2> xrSwapchains;

    // トラッキング状態
    std::array<XrView, 2> xrViews;
    XrFrameState xrFrameState{ XR_TYPE_FRAME_STATE };
    bool xrSessionRunning{ false };
    bool xrFrameActive{ false };

    // シーン座標の原点にする、HMD 起動時または回復時の頭部中心位置
    GgVector xrOriginPosition{ 0.0f, 0.0f, 0.0f, 1.0f };
    bool xrOriginValid{ false };

    // デプステクスチャ（各目用）
    std::array<GLuint, 2> xrDepthTextures{ 0, 0 };

    // ミラー表示用
    GLuint xrMirrorFbo{ 0 };
    GLuint xrMirrorTexture{ 0 };

    // OpenXR の初期化・終了
    bool initOpenXR();
    void cleanupOpenXR();
    void pollEvents();
#endif

  public:
    // HMD を起動する
    bool startHMD();

    // HMD を停止する
    void stopHMD();

    // 描画する目を選択する
    void select(int eye);

    // 描画を開始する
    bool start();

    // 描画を完了する
    void commit(int eye);
  };
};

// 古いクラス名との互換性のためのエイリアス
using Window = GgApp::Window;

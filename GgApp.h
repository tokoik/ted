#pragma once

///
/// アプリケーションのクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 197, 2026
///

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
/// アプリケーションのクラス
///
/// @details
/// TED 全体の入力選択、左右画像テクスチャ、ウィンドウ、描画ループを統括する。
/// Camera 派生クラスを共通インターフェースで切り替え、通常表示と OpenXR 表示へ同じシーンを供給する。
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

  ///
  /// 静止画像ファイルを使う
  ///
  /// @return 成功した場合は true
  ///
  bool useImage();

  ///
  /// 動画像ファイルを使う
  ///
  /// @return 成功した場合は true
  ///
  bool useMovie();

  ///
  /// Web カメラを使う
  ///
  /// @return 成功した場合は true
  ///
  bool useCamera();

  ///
  /// Ovrvision Pro を使う
  ///
  /// @return 成功した場合は true
  ///
  bool useOvervision();

  ///
  /// リモートの TED から取得する
  ///
  /// @return 成功した場合は true
  ///
  bool useRemote();

public:

  ///
  /// コンストラクタ
  ///
  GgApp() = default;

  ///
  /// デストラクタ
  ///
  virtual ~GgApp();

  ///
  /// 入力ソースを選択する
  ///
  bool selectInput();

  ///
  /// アプリケーション本体
  ///
  /// @param argc コマンドライン引数の個数
  /// @param argv コマンドライン引数の文字列配列
  /// @return 正常終了なら 0
  ///
  int main(int argc, const char* const* argv);

  ///
  /// ウィンドウ関連の処理を担当するクラス
  ///
  class Window
  {
    /// ウィンドウの識別子
    GLFWwindow* const window{ nullptr };

    /// ウィンドウのサイズ
    std::array<GLsizei, 2> size{ 0, 0 };

    /// ビューポートの幅と高さ
    int width{ 0 }, height{ 0 };

    /// ビューポートのアスペクト比
    GLfloat aspect{ 1.0f };

    /// メッシュの縦横の格子点数
    std::array<GLsizei, 2> samples{ 0, 0 };

    /// メッシュの縦横の格子間隔
    std::array<GLfloat, 2> gap{ 0.0f, 0.0f };

    /// 最後にタイプしたキー
    int key{ GLFW_KEY_UNKNOWN };

    /// ジョイスティックの番号
    int joy{ -1 };

    /// スティックの中立位置
    std::array<float, 4> origin{ 0.0f, 0.0f, 0.0f, 0.0f };

    /// ドラッグを開始した x 座標値
    double cx{ 0.0 };

    /// ドラッグを開始した y 座標値
    double cy{ 0.0 };

    /// このウィンドウで制御するカメラ
    Camera* camera{ nullptr };

    //
    // ヘッドトラッキング
    //

    /// ヘッドトラッキングによる回転
    std::array<GgQuaternion, camCount> qo
    {
      ggIdentityQuaternion(),
      ggIdentityQuaternion()
    };

    /// ヘッドトラッキングによる位置
    std::array<GgVector, camCount> po
    {
      GgVector{ 0.0f, 0.0f, 0.0f, 1.0f },
      GgVector{ 0.0f, 0.0f, 0.0f, 1.0f }
    };

    /// ヘッドトラッキングの変換行列
    std::array<GgMatrix, camCount> mo
    {
      ggIdentity(),
      ggIdentity()
    };

    //
    // 座標変換
    //

    /// モデル変換行列
    GgMatrix mm{ ggIdentity() };

    /// ビュー変換行列
    std::array<GgMatrix, camCount> mv
    {
      ggIdentity(),
      ggIdentity()
    };

    /// 投影変換行列
    std::array<GgMatrix, camCount> mp
    {
      ggIdentity(),
      ggIdentity()
    };

    /// ズーム率
    GLfloat zoom{ 1.0f };

    //
    // 背景画像
    //

    // 背景テクスチャの半径と中心
    GgVector circle{ 0.0f, 0.0f, 0.0f, 1.0f };

    /// スクリーンの幅と高さ
    std::array<GgVector, camCount> screen
    {
      GgVector{ 0.0f, 0.0f, 0.0f, 1.0f },
      GgVector{ 0.0f, 0.0f, 0.0f, 1.0f }
    };

    /// 焦点距離
    GLfloat focal{ 1.0f };

    /// 視差
    GLfloat parallax{ defaultParallax };

    /// スクリーンの間隔
    GLfloat offset{ 0.0f };

    /// 背景の描画に使う矩形から参照する
    friend class Rect;

  public:

    ///
    /// コンストラクタ
    ///
    /// @param width ウィンドウの幅
    /// @param height ウィンドウの高さ
    /// @param title ウィンドウのタイトル
    /// @param monitor フルスクリーン表示するモニタの識別子
    /// @param share 共有するウィンドウの識別子
    /// @return ウィンドウの識別子
    ///
    Window(int width = 640, int height = 480,
      const char* title = "GLFW Window",
      GLFWmonitor* monitor = nullptr,
      GLFWwindow* share = nullptr
    );

    ///
    /// コピーコンストラクタを封じる
    ///
    /// @param w コピー元の Window オブジェクト
    ///
    Window(const Window& w) = delete;

    ///
    /// 代入を封じる
    ///
    /// @param w コピー元の Window オブジェクト
    ///
    Window& operator=(const Window& w) = delete;

    ///
    /// デストラクタ
    ///
    virtual ~Window();

    ///
    /// ウィンドウの識別子の取得
    ///
    /// @return ウィンドウの識別子
    ///
    GLFWwindow* get() const
    {
      return window;
    }

    ///
    /// ウィンドウを閉じるよう指示する
    ///
    /// @param close 閉じるかどうか
    ///
    void setClose(int close = GLFW_TRUE) const
    {
      glfwSetWindowShouldClose(window, close);
    }

    ///
    /// ウィンドウを閉じるべきかを判定する
    ///
    /// @return 閉じるべきなら非ゼロ値、閉じないならゼロ
    ///
    int shouldClose() const
    {
      return glfwWindowShouldClose(window);
    }

    ///
    /// イベントを取得してループを継続するなら真を返す
    ///
    explicit operator bool();

    ///
    /// カラーバッファを入れ替えてイベントを取り出す
    ///
    void swapBuffers();

    ///
    /// ウィンドウのサイズ変更時の処理
    ///
    /// @param window ウィンドウの識別子
    /// @param width ウィンドウの幅
    /// @param height ウィンドウの高さ
    ///
    static void resize(GLFWwindow* window, int width, int height);

    ///
    /// マウスボタンを操作したときの処理
    ///
    /// @param window ウィンドウの識別子
    /// @param button マウスボタンの識別子
    /// @param action マウスボタンの操作
    /// @param mods 修飾キーの状態
    /// 
    static void mouse(GLFWwindow* window, int button, int action, int mods);

    ///
    /// マウスホイール操作時の処理
    ///
    /// @param window ウィンドウの識別子
    /// @param x ホイールの水平方向の移動量
    /// @param y ホイールの垂直方向の移動量
    ///
    static void wheel(GLFWwindow* window, double x, double y);

    ///
    /// キーボードをタイプした時の処理
    ///
    /// @param window ウィンドウの識別子
    /// @param key キーの識別子
    /// @param scancode スキャンコード
    /// @param action キーの操作
    /// @param mods 修飾キーの状態
    ///
    static void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);

    ///
    /// 設定値の初期化
    ///
    void reset();

    ///
    /// ビューポートの初期化
    ///
    void resetViewport()
    {
      resize(window, size[0], size[1]);
    }

    ///
    /// 透視投影変換行列を更新する
    ///
    void update();

    ///
    /// カメラの画角と中心位置を更新する
    ///
    void updateCircle();

    ///
    /// このウィンドウで制御するカメラを設定する
    ///
    /// @param cam 設定するカメラのポインタ
    ///
    void setControlCamera(Camera* cam)
    {
      camera = cam;
    }

    ///
    /// モデル変換行列を得る
    ///
    /// @return モデル変換行列
    ///
    const GgMatrix& getMm() const
    {
      return mm;
    }

    ///
    /// ビュー変換行列を得る
    ///
    /// @param eye 目の識別子
    /// @return ビュー変換行列
    ///
    const GgMatrix& getMv(int eye) const
    {
      return mv[eye];
    }

    ///
    /// ヘッドラッキングによる移動を得る
    ///
    /// @param eye 目の識別子
    /// @return ヘッドラッキングによる移動
    ///
    const GgVector& getPo(int eye) const
    {
      return po[eye];
    }

    ///
    /// ヘッドラッキングによる回転の四元数を得る
    ///
    /// @param eye 目の識別子
    /// @return ヘッドラッキングによる回転の四元数
    ///
    const GgQuaternion& getQo(int eye) const
    {
      return qo[eye];
    }

    ///
    /// ヘッドラッキングによる回転の変換行列を得る
    ///
    /// @param eye 目の識別子
    /// @return ヘッドラッキングによる回転の変換行列
    ///
    const GgMatrix& getMo(int eye) const
    {
      return mo[eye];
    }

    ///
    /// プロジェクション変換行列を得る
    ///
    /// @param eye 目の識別子
    /// @return プロジェクション変換行列
    ///
    const GgMatrix& getMp(int eye) const
    {
      return mp[eye];
    }

    ///
    /// メッシュの縦横の格子点数を取り出す
    ///
    /// @return メッシュの縦横の格子点数
    ///
    const auto& getSamples() const
    {
      return samples;
    }

    ///
    /// メッシュの縦横の格子間隔を取り出す
    ///
    /// @return メッシュの縦横の格子間隔
    ///
    const auto& getGap() const
    {
      return gap;
    }

    ///
    /// スクリーンの幅と高さを取り出す
    ///
    /// @param eye 目の識別子
    /// @return スクリーンの幅と高さ
    ///
    const auto& getScreen(int eye) const
    {
      return screen[eye];
    }

    ///
    /// 焦点距離を取り出す
    ///
    /// @return 焦点距離
    ///
    auto getFocal() const
    {
      return focal;
    }

    ///
    /// スクリーンの間隔を取り出す
    ///
    /// @param eye 目の識別子
    /// @return スクリーンの間隔
    ///
    auto getOffset(int eye = 0) const
    {
      return static_cast<GLfloat>(1 - (eye & 1) * 2) * offset;
    }

    ///
    /// 背景テクスチャの半径と中心を取り出す
    ///
    /// @return 背景テクスチャの半径と中心
    ///
    const auto& getCircle() const
    {
      return circle;
    }

    ///
    /// ウィンドウのサイズを取り出す
    ///
    /// @return ウィンドウのサイズ
    ///
    const auto& getSize() const
    {
      return size;
    }

    ///
    /// ウィンドウのアスペクト比を取り出す
    ///
    /// @return ウィンドウのアスペクト比
    ///
    auto getAspect() const
    {
      return aspect;
    }

    /// シーン表示
    bool showScene{ true };

    /// ミラー表示
    bool showMirror{ true };

    /// メニュー表示
    bool showMenu{ true };

#if defined(GG_USE_OPENXR)
  private:

    // OpenXRの所有資源。instanceを親としてsession、space、swapchainの順に作成し、逆順に破棄する。
    XrInstance xrInstance{ XR_NULL_HANDLE };
    XrSystemId xrSystemId{ XR_NULL_SYSTEM_ID };
    XrSession xrSession{ XR_NULL_HANDLE };
    XrSpace xrPlaySpace{ XR_NULL_HANDLE };
    XrSpace xrViewSpace{ XR_NULL_HANDLE };

    // 左右各眼のランタイム所有画像と、それをOpenGLで描画するためのFBOを対応付ける
    struct OpenXRSwapchain
    {
      XrSwapchain handle{ XR_NULL_HANDLE };
      uint32_t width{ 0 };
      uint32_t height{ 0 };
      std::vector<XrSwapchainImageOpenGLKHR> images;
      std::vector<GLuint> fbos;
      uint32_t activeImageIndex{ 0 };

      // acquire済み画像だけをcommitでreleaseし、二重解放や未取得画像への描画を防ぐ
      bool imageAcquired{ false };
    };
    std::array<OpenXRSwapchain, 2> xrSwapchains;

    // startで取得した予測表示時刻の視点姿勢を、左右描画とswapBuffersまで保持する
    std::array<XrView, 2> xrViews;
    XrFrameState xrFrameState{ XR_TYPE_FRAME_STATE };
    bool xrSessionRunning{ false };

    // xrBeginFrame後からxrEndFrameまでだけtrueにし、通常のGLFW描画経路と区別する
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

    ///
    /// HMD を起動する
    ///
    bool startHMD();

    ///
    /// HMD を停止する
    ///
    void stopHMD();

    ///
    /// 描画する目を選択する
    ///
    /// @param eye 目の識別子
    ///
    void select(int eye);

    ///
    /// 描画を開始する
    ///
    /// @return 描画の開始に成功したかどうか
    ///
    bool start();

    ///
    /// 描画を完了する
    ///
    /// @param eye 目の識別子
    ///
    void commit(int eye);
  };
};

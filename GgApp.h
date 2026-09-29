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

// 作業者として映像と姿勢を送信するクラス
#include "Worker.h"

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

  /// 役割が作業者のとき、入力中のカメラの映像と姿勢を送信する
  std::unique_ptr<Worker> worker;

  ///
  /// 役割と入力に合わせて作業者の送信を開始・停止する
  ///
  /// @details
  /// 役割が作業者で、入力がリモート以外なら送信を (再) 開始し、それ以外なら停止する。
  /// 入力を切り替えたときは新しいカメラで送信し直す。
  ///
  void updateWorker();

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
  /// コピーコンストラクタを封じる
  ///
  GgApp(const GgApp&) = delete;

  ///
  /// 代入を封じる
  ///
  GgApp& operator=(const GgApp&) = delete;

  ///
  /// ムーブコンストラクタ
  ///
  GgApp(GgApp&&) = default;

  ///
  /// ムーブ代入演算子
  ///
  GgApp& operator=(GgApp&&) = default;

  ///
  /// デストラクタ
  ///
  virtual ~GgApp();

  ///
  /// 入力ソースを選択する
  ///
  /// @return 入力ソースを使用できるようになった場合は true
  ///
  bool selectInput();

  ///
  /// ハンドトラッキングの使用状態を変更する
  ///
  /// @param mode 使用するハンドトラッキングのモード
  /// @return 要求した状態へ変更できた場合は true
  ///
  bool setHandTrackingMode(int mode);

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
  /// @details
  /// デスクトップ表示 (単眼視・上下・左右・オーバーレイ・Quad Buffer) の描画制御と、
  /// マウス・キーボード・ジョイスティックによる操作を担当する。
  /// OpenXR (HMD) の資源は GgApp::OpenXR が管理し、このクラスは HMD の視点の姿勢と
  /// 視野角から背景とシーンの描画に使う変換行列を求める (updateHMD())。
  ///
  class Window
  {
    /// ウィンドウの識別子
    GLFWwindow* window{ nullptr };

    /// ウィンドウのサイズ
    std::array<GLsizei, 2> size{ 0, 0 };

    /// フレームバッファのサイズ
    std::array<GLsizei, 2> fboSize{ 0, 0 };

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

    /// 背景テクスチャの半径と中心
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

    //
    // 表示モード
    //

    /// シーン表示
    bool showScene{ true };

    /// ミラー表示
    bool showMirror{ true };

    /// メニュー表示
    bool showMenu{ true };

    ///
    /// HMD を使用中か調べる
    ///
    /// @return OpenXR のセッションを作成済みなら true
    ///
    static bool isHMD();

  public:

    ///
    /// コンストラクタ
    ///
    /// @param width ウィンドウの幅
    /// @param height ウィンドウの高さ
    /// @param title ウィンドウのタイトル
    /// @param monitor フルスクリーン表示するモニタの識別子
    /// @param share 共有するウィンドウの識別子
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
    /// ムーブコンストラクタ
    ///
    /// @param w ムーブ元の Window オブジェクト
    ///
    Window(Window&& w) noexcept;

    ///
    /// ムーブ代入演算子
    ///
    /// @param w ムーブ元の Window オブジェクト
    /// @return ムーブ先の Window オブジェクト
    ///
    Window& operator=(Window&& w) noexcept;

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
    /// カラーバッファを入れ替える
    ///
    void swapBuffers();

    ///
    /// ウィンドウのサイズ変更時の処理
    ///
    /// @param window ウィンドウの識別子
    /// @param width フレームバッファの幅
    /// @param height フレームバッファの高さ
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
      resize(window, fboSize[0], fboSize[1]);
    }

    ///
    /// ビューポートをウィンドウ全体に戻す
    ///
    /// @details
    /// OpenXR のミラー表示の後などに、Dear ImGui のメニューを描画できるようにする。
    ///
    void restoreViewport() const
    {
      if (!glfwGetWindowAttrib(window, GLFW_ICONIFIED)) glViewport(0, 0, fboSize[0], fboSize[1]);
    }

    ///
    /// 表示モードを変更し、必要な表示資源とビューポートを更新する
    ///
    /// @param mode 新しい表示モード
    /// @return 表示モードを変更できた場合は true
    ///
    bool setDisplayMode(int mode);

    ///
    /// Quad Buffer Stereo を実際に利用できるか調べる
    ///
    /// @return 作成済みウィンドウがステレオバッファを持つ場合は true
    ///
    bool isQuadBufferAvailable() const
    {
      return window && glfwGetWindowAttrib(window, GLFW_STEREO) == GLFW_TRUE;
    }

    ///
    /// 前方面と後方面を変更して透視投影変換行列を更新する
    ///
    /// @param nearPlane 前方面
    /// @param farPlane 後方面
    /// @return 有効な範囲を設定できた場合は true
    ///
    bool setClipPlanes(float nearPlane, float farPlane);

    ///
    /// ミラー表示の状態を取得する
    ///
    /// @return ミラー表示する場合は true
    ///
    bool isMirrorVisible() const { return showMirror; }

    ///
    /// ミラー表示の状態を変更する
    ///
    /// @param visible ミラー表示する場合は true
    ///
    void setMirrorVisible(bool visible) { showMirror = visible; }

    ///
    /// シーン表示の状態を取得する
    ///
    /// @return シーン表示する場合は true
    ///
    bool isSceneVisible() const { return showScene; }

    ///
    /// シーン表示の状態を変更する
    ///
    /// @param visible シーン表示する場合は true
    ///
    void setSceneVisible(bool visible) { showScene = visible; }

    ///
    /// メニュー表示の状態を取得する
    ///
    /// @return メニュー表示する場合は true
    ///
    bool isMenuVisible() const { return showMenu; }

    ///
    /// メニュー表示の状態を変更する
    ///
    /// @param visible メニュー表示する場合は true
    ///
    void setMenuVisible(bool visible) { showMenu = visible; }

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
    /// フレームバッファのサイズを取り出す
    ///
    /// @return フレームバッファのサイズ
    ///
    const auto& getFboSize() const
    {
      return fboSize;
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

    ///
    /// HMD を起動する
    ///
    /// @return OpenXR のセッションを作成できた場合は true
    ///
    bool startHMD();

    ///
    /// HMD を停止する
    ///
    void stopHMD();

    ///
    /// HMD の視点の姿勢と視野角から、左右の目の変換行列とスクリーンを更新する
    ///
    /// @details
    /// GgApp::OpenXR::begin() が true を返した後、左右の目を描画する前に呼ぶ。
    /// 頭部の回転は getMo()、頭部原点からの各眼の位置は getMv()、
    /// ズームと視差を反映した視錐台は getMp()、背景のスクリーンは Rect が参照する。
    ///
    void updateHMD();

    ///
    /// 描画を開始する (デスクトップ表示)
    ///
    /// @return 描画を行うなら true
    ///
    bool start();

    ///
    /// 描画する目を選択する (デスクトップ表示)
    ///
    /// @param eye 目の識別子
    ///
    void select(int eye);

    ///
    /// 描画を完了する (デスクトップ表示)
    ///
    /// @param eye 目の識別子
    ///
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

    //
    // アクションシステムを初期化する
    //
    void initActions();

    //
    // アクション状態を更新する
    //
    void pollActions();

    //
    // OpenXR のイベントを処理する
    //
    void pollEvents();

    //
    // スワップチェーンを作成する
    //
    void createSwapchains();

    //
    // 描画中のフレームを合成器に転送する
    //
    void endFrame();

    //
    // OpenXR のハンドルを破棄する (OpenGL の資源には触れない)
    //
    void destroyXr();

    //
    // ミラー表示を行う
    //
    void blitMirror() const;

    //
    // ハンドトラッキング拡張 (XR_EXT_hand_tracking)
    //

    // XR_EXT_hand_tracking は任意拡張なので、ランタイムが公開した場合だけ使用する
    bool xrHandTrackingSupported{ false };

    // 左右の手のハンドトラッカー (XrHandEXT の順: 0 が左手, 1 が右手)
    XrHandTrackerEXT xrHandTracker[Hand::Count]{ XR_NULL_HANDLE, XR_NULL_HANDLE };

    // ハンドトラッキング拡張の関数
    PFN_xrCreateHandTrackerEXT xrCreateHandTracker{ nullptr };
    PFN_xrDestroyHandTrackerEXT xrDestroyHandTracker{ nullptr };
    PFN_xrLocateHandJointsEXT xrLocateHandJoints{ nullptr };

    //
    // 頭部中心姿勢
    //

    // シーン座標の原点にする、HMD 起動時または回復時の頭部中心位置
    GgVector xrOriginPosition{ 0.0f, 0.0f, 0.0f, 1.0f };

    // xrOriginPosition が有効なら true (false なら次に取得した頭部中心位置を原点にする)
    bool xrOriginValid{ false };


    //
    // コンストラクタ
    //
    OpenXR();

    //
    // デストラクタ
    //
    virtual ~OpenXR();

  public:

    // シングルトンなのでコピー・ムーブ禁止
    OpenXR(const OpenXR&) = delete;
    OpenXR& operator=(const OpenXR&) = delete;
    OpenXR(OpenXR&&) = delete;
    OpenXR& operator=(OpenXR&&) = delete;

    ///
    /// OpenXR のセッションを作成する.
    ///
    /// @param window ミラー表示を行うウィンドウ.
    /// @param spaceType 使用する参照空間のタイプ（デフォルトは XR_REFERENCE_SPACE_TYPE_STAGE）.
    /// @param appName OpenXR のランタイムに通知するアプリケーション名.
    /// @return OpenXR の static object の参照.
    ///
    /// @note
    /// 初期化に失敗したときは確保した資源を解放したうえで
    /// std::runtime_error を投げる. 二度目以降の呼び出しは
    /// 初期化済みのオブジェクトをそのまま返す.
    ///
    static OpenXR& initialize(const Window& window,
      XrReferenceSpaceType spaceType = XR_REFERENCE_SPACE_TYPE_STAGE,
      const char* appName = "TED");

    ///
    /// OpenXR の static object を取得する.
    ///
    /// @return OpenXR の static object の参照.
    ///
    /// @note
    /// initialize() を呼ぶ前でも取得できる. その場合 isInitialized() は false を返す.
    ///
    static OpenXR& getInstance();

    ///
    /// OpenXR のセッションを破棄する.
    ///
    /// @note
    /// 何度呼んでも安全である. OpenGL の資源を解放するので,
    /// ウィンドウ (OpenGL のコンテキスト) が有効なうちに呼ぶこと.
    ///
    void terminate();

    ///
    /// OpenXR による描画開始.
    ///
    /// @return このフレームで描画を行うべきなら true.
    ///
    /// @note
    /// イベントの処理, フレームの同期 (xrWaitFrame / xrBeginFrame),
    /// 視点の姿勢の取得 (xrLocateViews), コントローラの状態の更新を行う.
    /// 描画が不要なフレームでは内部で xrEndFrame() まで済ませて false を
    /// 返すので, false のときは select() / commit() / submit() を
    /// 呼んではならない (呼んでも安全に無視される).
    ///
    bool begin();

    ///
    /// 描画対象の目を指定してフレームバッファとビューポートを設定する.
    ///
    /// @param eye 表示する目 (0: 左目, 1: 右目).
    ///
    void select(int eye);

    ///
    /// 描画対象の目を指定する (旧 LibOVR 仕様からの移行用オーバーロード).
    ///
    /// @param eye 表示する目 (0: 左目, 1: 右目).
    /// @param screen HMD の視野の視錐台のタンジェント (tanLeft, tanRight, tanDown, tanUp).
    /// @param position HMD の位置 (x, y, z).
    /// @param orientation HMD の方向の四元数 (x, y, z, w).
    ///
    void select(int eye, GLfloat* screen, GLfloat* position, GLfloat* orientation);

    ///
    /// 指定した目の描画を完了する.
    ///
    /// @param eye 完了した目のインデックス (0: 左目, 1: 右目).
    ///
    /// @note
    /// 描画先をウィンドウに戻す. スワップチェーンイメージの解放は
    /// ミラー表示を行った後の submit() の中で行う.
    ///
    void commit(int eye);

    ///
    /// フレームを転送して HMD に表示する.
    ///
    /// @param mirror true ならウィンドウへのミラー表示を行う, デフォルトは true.
    /// @return フレームの転送に成功したら true.
    ///
    /// @note
    /// ミラー表示を行ってからスワップチェーンイメージを解放し,
    /// 合成レイヤを組み立てて xrEndFrame() を呼ぶ.
    /// ミラー表示するビューの番号は setMirror() で変更できる.
    ///
    bool submit(bool mirror = true);

    ///
    /// ミラー表示を行うビューの番号を設定する.
    ///
    /// @param eye ミラー表示を行うビューの番号, -1 ならミラー表示を行わない.
    ///
    void setMirror(int eye);

    ///
    /// ミラー表示を行うビューの番号を取得する.
    ///
    /// @return ミラー表示を行うビューの番号, ミラー表示を行わないなら -1.
    ///
    int getMirror() const;

    ///
    /// セッションが実行中かどうか調べる.
    ///
    /// @return セッションが実行中なら true.
    ///
    bool isRunning() const;

    ///
    /// アプリケーションが入力を受け付けているかどうか調べる.
    ///
    /// @return XR_SESSION_STATE_FOCUSED なら true.
    ///
    bool isFocused() const;

    ///
    /// OpenXR のシステム (HMD) の名前を取得する.
    ///
    /// @return システムの名前の文字列.
    ///
    const std::string& getSystemName() const;

    ///
    /// 指定した目の透視投影変換行列を取得する.
    ///
    /// @param eye 表示する目 (0: 左目, 1: 右目).
    /// @param zNear 前方面の位置 (デフォルトは 0.1f).
    /// @param zFar 後方面の位置 (デフォルトは 100.0f).
    /// @return 透視投影変換行列 (GgMatrix).
    ///
    gg::GgMatrix getProjectionMatrix(int eye, GLfloat zNear = 0.1f, GLfloat zFar = 100.0f) const;

    ///
    /// 指定した目のビュー変換行列（ワールド座標系から視点座標系への変換）を取得する.
    ///
    /// @param eye 表示する目 (0: 左目, 1: 右目).
    /// @return ビュー変換行列 (GgMatrix).
    ///
    gg::GgMatrix getViewMatrix(int eye) const;

    ///
    /// 指定した目の姿勢行列（視点のモデル変換行列: 位置と回転）を取得する.
    ///
    /// @param eye 表示する目 (0: 左目, 1: 右目).
    /// @return 姿勢行列 (GgMatrix).
    ///
    gg::GgMatrix getPoseMatrix(int eye) const;

    ///
    /// 指定した目の視点位置を取得する.
    ///
    /// @param eye 表示する目 (0: 左目, 1: 右目).
    /// @return 視点位置 (GgVector).
    ///
    gg::GgVector getPosition(int eye) const;

    ///
    /// 指定した目の視線方向の回転四元数を取得する.
    ///
    /// @param eye 表示する目 (0: 左目, 1: 右目).
    /// @return 回転四元数 (GgQuaternion).
    ///
    gg::GgQuaternion getOrientation(int eye) const;

    ///
    /// 指定した目の視野角情報 (XrFovf) を取得する.
    ///
    /// @param eye 表示する目 (0: 左目, 1: 右目).
    /// @return 視野角構造体の参照.
    ///
    const XrFovf& getFov(int eye) const;

    ///
    /// 指定した目の姿勢情報 (XrPosef) を取得する.
    ///
    /// @param eye 表示する目 (0: 左目, 1: 右目).
    /// @return 姿勢構造体の参照.
    ///
    const XrPosef& getPose(int eye) const;

    ///
    /// 視点の姿勢が有効かどうか調べる.
    ///
    /// @return 直前の begin() で視点の位置と向きが取得できていれば true.
    ///
    bool isPoseValid() const;

    ///
    /// レンダリング推奨解像度の横幅を取得する.
    ///
    /// @param eye 表示する目 (デフォルトは 0: 左目).
    /// @return レンダリング画像の幅 (ピクセル).
    ///
    GLsizei getWidth(int eye = 0) const;

    ///
    /// レンダリング推奨解像度の高さを取得する.
    ///
    /// @param eye 表示する目 (デフォルトは 0: 左目).
    /// @return レンダリング画像の高さ (ピクセル).
    ///
    GLsizei getHeight(int eye = 0) const;

    ///
    /// アスペクト比 (幅 / 高さ) を取得する.
    ///
    /// @param eye 表示する目 (デフォルトは 0: 左目).
    /// @return アスペクト比.
    ///
    GLfloat getAspect(int eye = 0) const;

    ///
    /// ビューの総数を取得する (通常は 2).
    ///
    /// @return ビューの総数.
    ///
    uint32_t getViewCount() const;

    ///
    /// 現在の参照空間タイプを取得する.
    ///
    /// @return 参照空間タイプ (XR_REFERENCE_SPACE_TYPE_STAGE または XR_REFERENCE_SPACE_TYPE_LOCAL).
    ///
    XrReferenceSpaceType getReferenceSpaceType() const;

    ///
    /// 直前の begin() で取得したフレームの予測表示時刻を取得する.
    ///
    /// @return 予測表示時刻 (XrTime).
    ///
    XrTime getPredictedDisplayTime() const { return frameState.predictedDisplayTime; }

    ///
    /// コントローラーがトラッキングされているか取得する.
    ///
    /// @param hand 対象の手 (0: 左手 Hand::Left, 1: 右手 Hand::Right).
    /// @return トラッキングされていれば true.
    ///
    bool isTracked(int hand) const;

    ///
    /// コントローラーのグリップ位置・姿勢を表すモデル変換行列を取得する.
    ///
    /// @param hand 対象の手 (0: 左手 Hand::Left, 1: 右手 Hand::Right).
    /// @return グリップの変換行列 (GgMatrix).
    ///
    gg::GgMatrix getGripMatrix(int hand) const;

    ///
    /// コントローラーのポインティング（エイム）方向を表すモデル変換行列を取得する.
    ///
    /// @param hand 対象の手 (0: 左手 Hand::Left, 1: 右手 Hand::Right).
    /// @return エイムの変換行列 (GgMatrix).
    ///
    gg::GgMatrix getAimMatrix(int hand) const;

    ///
    /// コントローラーのグリップ位置を取得する.
    ///
    /// @param hand 対象の手 (0: 左手 Hand::Left, 1: 右手 Hand::Right).
    /// @return グリップ位置 (GgVector).
    ///
    gg::GgVector getGripPosition(int hand) const;

    ///
    /// コントローラーのグリップ回転四元数を取得する.
    ///
    /// @param hand 対象の手 (0: 左手 Hand::Left, 1: 右手 Hand::Right).
    /// @return グリップ回転四元数 (GgQuaternion).
    ///
    gg::GgQuaternion getGripOrientation(int hand) const;

    ///
    /// コントローラーのエイム位置を取得する.
    ///
    /// @param hand 対象の手 (0: 左手 Hand::Left, 1: 右手 Hand::Right).
    /// @return エイム位置 (GgVector).
    ///
    gg::GgVector getAimPosition(int hand) const;

    ///
    /// コントローラーのエイム回転四元数を取得する.
    ///
    /// @param hand 対象の手 (0: 左手 Hand::Left, 1: 右手 Hand::Right).
    /// @return エイム回転四元数 (GgQuaternion).
    ///
    gg::GgQuaternion getAimOrientation(int hand) const;

    ///
    /// トリガーの押し込み量を取得する.
    ///
    /// @param hand 対象の手 (0: 左手 Hand::Left, 1: 右手 Hand::Right).
    /// @return トリガー値 (0.0f ～ 1.0f).
    ///
    float getTrigger(int hand) const;

    ///
    /// グリップ（スクイーズ）の押し込み量を取得する.
    ///
    /// @param hand 対象の手 (0: 左手 Hand::Left, 1: 右手 Hand::Right).
    /// @return グリップ値 (0.0f ～ 1.0f).
    ///
    float getGrip(int hand) const;

    ///
    /// アナログスティック / トラックパッドの入力値を取得する.
    ///
    /// @param hand 対象の手 (0: 左手 Hand::Left, 1: 右手 Hand::Right).
    /// @return 2次元入力値 (x, y 各 -1.0f ～ 1.0f).
    ///
    std::array<float, 2> getThumbstick(int hand) const;

    ///
    /// アナログスティック / トラックパッドのクリック状態を取得する.
    ///
    /// @param hand 対象の手 (0: 左手 Hand::Left, 1: 右手 Hand::Right).
    /// @return 押されていれば true.
    ///
    bool getThumbstickClick(int hand) const;

    ///
    /// プライマリボタン (X / A ボタン) の押下状態を取得する.
    ///
    /// @param hand 対象の手 (0: 左手 Hand::Left, 1: 右手 Hand::Right).
    /// @return 押されていれば true.
    ///
    bool getPrimaryButton(int hand) const;

    ///
    /// セカンダリボタン (Y / B ボタン) の押下状態を取得する.
    ///
    /// @param hand 対象の手 (0: 左手 Hand::Left, 1: 右手 Hand::Right).
    /// @return 押されていれば true.
    ///
    bool getSecondaryButton(int hand) const;

    ///
    /// メニューボタンの押下状態を取得する.
    ///
    /// @param hand 対象の手 (デフォルトは 0: 左手 Hand::Left).
    /// @return 押されていれば true.
    ///
    /// @note
    /// Meta Touch と Valve Index の対話プロファイルには右手のメニューボタンが
    /// 存在しない (システムに予約されている) ため, これらでは右手を指定しても
    /// 常に false になる.
    ///
    bool getMenuButton(int hand = Hand::Left) const;

    ///
    /// コントローラーに振動（ハプティクスフィードバック）を出力する.
    ///
    /// @param hand 対象の手 (0: 左手 Hand::Left, 1: 右手 Hand::Right).
    /// @param durationSeconds 振動の持続時間（秒）.
    /// @param frequency 振動数（Hz、XR_FREQUENCY_UNSPECIFIED でランタイムデフォルト）.
    /// @param amplitude 振幅強度（0.0f ～ 1.0f）.
    ///
    void applyHapticVibration(int hand, float durationSeconds = 0.1f, float frequency = XR_FREQUENCY_UNSPECIFIED, float amplitude = 0.5f);

    ///
    /// OpenXR のセッションを作成済みかどうか調べる.
    ///
    /// @return initialize() に成功して terminate() を呼んでいなければ true.
    ///
    /// @note
    /// セッションが実行中 (isRunning()) になるのは, begin() の中で
    /// XR_SESSION_STATE_READY のイベントを受け取った後である.
    ///
    bool isInitialized() const { return initialized; }

    ///
    /// ハンドトラッキング (XR_EXT_hand_tracking) が使えるかどうか調べる.
    ///
    /// @return ハンドトラッカーを作成できていれば true.
    ///
    bool hasHandTracking() const { return xrHandTrackingSupported; }

    ///
    /// OpenXR のランタイムから手の関節姿勢を取得し, Leap Motion 互換の 22 姿勢に変換して
    /// シーングラフに格納する.
    ///
    /// @param time 姿勢を求める時刻 (視点と同じ予測表示時刻を使う).
    ///
    /// @note
    /// 関節姿勢は頭部中心姿勢 (getHeadPoseMatrix()) を親とする座標系で表す.
    ///
    void updateOpenXRHands(XrTime time);

    ///
    /// 頭部中心 (左右の目の中点) の位置を取得する.
    ///
    /// @return シーン座標の原点 (getOriginPosition()) を基準にした頭部中心の位置.
    ///
    GgVector getHeadPosition() const;

    ///
    /// 頭部の向きを取得する.
    ///
    /// @return 頭部の向きの四元数 (左目の向きを使う).
    ///
    GgQuaternion getHeadOrientation() const;

    ///
    /// 頭部中心姿勢の変換行列を取得する.
    ///
    /// @return 頭部中心の位置と向きによるモデル変換行列.
    ///
    GgMatrix getHeadPoseMatrix() const;

    ///
    /// シーン座標の原点にしている頭部中心位置を取得する.
    ///
    /// @return OpenXR の参照空間におけるシーン座標の原点.
    ///
    const GgVector& getOriginPosition() const { return xrOriginPosition; }

    ///
    /// シーン座標の原点を設定する.
    ///
    /// @param pos OpenXR の参照空間におけるシーン座標の原点.
    ///
    void setOriginPosition(const GgVector& pos) { xrOriginPosition = pos; xrOriginValid = true; }

    ///
    /// シーン座標の原点を取り直す.
    ///
    /// @note
    /// 次の begin() で取得した頭部中心位置を, シーン座標の新しい原点にする.
    ///
    void resetOrigin() { xrOriginValid = false; }
  };
#endif

  ///
  /// ユーザ名を得る.
  ///
  /// @return ユーザ名の文字列.
  ///
  static std::string getUsername();
};

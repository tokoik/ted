///
/// TelExistence Display System
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///

// ウィンドウ関連の処理
#include "GgApp.h"

// ネットワーク関連の処理
#include "Network.h"

// カメラ関連の処理
#include "CamMf.h"
#include "CamOv.h"
#include "CamImage.h"
#include "CamRemote.h"

// シーングラフ
#include "Scene.h"

// 姿勢
#include "Attitude.h"

// 矩形
#include "Rect.h"

// メニュー
#include "Menu.h"

// 標準ライブラリ
#include <iostream>

static Rect* rectPointer{ nullptr };


//
// デストラクタ
//
GgApp::~GgApp()
{
}

//
// 静止画像ファイルを使う
//
bool GgApp::useImage()
{
  // 左の画像が指定されていなければ戻る
  if (defaults.camera_image[camL].empty())
  {
    NOTIFY(u8"左の画像ファイルが指定されていません。");
    return false;
  }

  // 左カメラに画像ファイルを使う
  std::unique_ptr<CamImage> cam{ new CamImage };

  // 左の画像が使用できなければ戻る
  if (!cam->open(defaults.camera_image[camL], camL))
  {
    NOTIFY(u8"左の画像ファイルが使用できません。");
    return false;
  }

  const GLubyte* imgL = cam->getImage(camL);
  const GLubyte* imgR = nullptr;
  bool isStereo = false;

  if (isPackedCameraLayout(defaults.camera_layout))
  {
    // 左画像内から分割済みの右画像を使用する
    imgR = cam->getImage(camR);
    isStereo = cam->opened(camR);
  }
  // 左右別入力なら右画像を個別に開く
  else if (defaults.camera_layout == CAMERA_LAYOUT_SEPARATE
    && !defaults.camera_image[camR].empty()
    && defaults.camera_image[camR] != defaults.camera_image[camL])
  {
    // 右の画像が使用できなければ警告する
    if (!cam->open(defaults.camera_image[camR], camR))
    {
      NOTIFY(u8"右の画像ファイルが使用できません。");
    }
    else
    {
      // 右の画像を保存しておく
      imgR = cam->getImage(camR);

      // ステレオ入力
      isStereo = true;
    }
  }

  // 成功時のみ適用
  camera = std::move(cam);
  image[camL] = imgL;
  stereo = isStereo;
  if (isStereo)
  {
    image[camR] = imgR;
  }
  else
  {
    image[camR] = nullptr;
  }
  return true;
}

//
// 動画像ファイルを使用する
//
bool GgApp::useMovie()
{
  // 左の動画像ファイルが指定されていなければ戻る
  if (defaults.camera_movie[camL].empty())
  {
    NOTIFY(u8"左の動画像ファイルが指定されていません。");
    return false;
  }

  // 左カメラに Media Foundation のキャプチャデバイスを使う
  std::unique_ptr<CamMf> cam{ new CamMf };

  // 左の動画像ファイルが開けなければ戻る
  if (!cam->open(defaults.camera_movie[camL], camL))
  {
    NOTIFY(u8"左の動画像ファイルが使用できません。");
    return false;
  }

  bool isStereo = false;

  if (isPackedCameraLayout(defaults.camera_layout))
  {
    // 左動画の各フレームをキャプチャスレッドで左右へ分割する
    isStereo = true;
  }
  // 左右別入力なら右動画を個別に開く
  else if (defaults.camera_layout == CAMERA_LAYOUT_SEPARATE
    && !defaults.camera_movie[camR].empty()
    && defaults.camera_movie[camR] != defaults.camera_movie[camL])
  {
    // 右の動画像ファイルが使用できなければ警告する
    if (!cam->open(defaults.camera_movie[camR], camR))
    {
      NOTIFY(u8"右の動画像ファイルが使用できません。");
    }
    else
    {
      // ステレオ入力
      isStereo = true;
    }
  }

  // 成功時のみ適用
  camera = std::move(cam);
  stereo = isStereo;
  return true;
}

//
// Web カメラを使用する
//
bool GgApp::useCamera()
{
  // 左カメラが指定されていなければ戻る
  if (defaults.camera_id[camL] < 0)
  {
    NOTIFY(u8"左のカメラが指定されていません。");
    return false;
  }

  // 左カメラに Media Foundation のキャプチャデバイスを使う
  std::unique_ptr<CamMf> cam{ new CamMf };

  // 左カメラのデバイスが開けなければ戻る
  if (!cam->open(defaults.camera_id[camL], camL))
  {
    NOTIFY(u8"左のカメラが使用できません。");
    return false;
  }

  bool isStereo = false;

  if (isPackedCameraLayout(defaults.camera_layout))
  {
    // 左カメラの各フレームをキャプチャスレッドで左右へ分割する
    isStereo = true;
  }
  // 左右別入力なら右カメラを個別に開く
  else if (defaults.camera_layout == CAMERA_LAYOUT_SEPARATE
    && defaults.camera_id[camR] >= 0
    && defaults.camera_id[camR] != defaults.camera_id[camL])
  {
    // 右カメラのデバイスが使用できなければ警告する
    if (!cam->open(defaults.camera_id[camR], camR))
    {
      NOTIFY(u8"右のカメラが使用できません。");
    }
    else
    {
      // ステレオ入力
      isStereo = true;
    }
  }

  // 成功時のみ適用
  camera = std::move(cam);
  stereo = isStereo;
  return true;
}

//
// Ovrvision Pro を使う
//
bool GgApp::useOvervision()
{
  // Ovrvision Pro を使う
  std::unique_ptr<CamOv> cam{ new CamOv };

  // Ovrvision Pro が開けなければ戻る
  if (!cam->open(static_cast<OVR::Camprop>(defaults.ovrvision_property)))
  {
    // Ovrvision Pro が使えなかった
    NOTIFY(u8"Ovrvision Pro が使えません。");
    return false;
  }

  // 成功時のみ適用
  camera = std::move(cam);
  stereo = true;

  return true;
}

//
// リモートの TED から取得する
//
bool GgApp::useRemote()
{
  // リモートカメラからキャプチャするためのダミーカメラを使う
  std::unique_ptr<CamRemote> cam{ new CamRemote };

  // 指導者側を起動する
  if (cam->open(defaults.port, defaults.address.c_str()) < 0)
  {
    NOTIFY(u8"作業者側のデータを受け取れません。");
    return false;
  }

  // 成功時のみ適用
  camera = std::move(cam);
  stereo = true;

  return true;
}

//
// 入力ソースを選択する
//
bool GgApp::selectInput()
{
  bool success = false;

  switch (defaults.input_mode)
  {
  case InputMode::IMAGE:
    success = useImage();
    break;
  case InputMode::MOVIE:
    success = useMovie();
    break;
  case InputMode::CAMERA:
    success = useCamera();
    break;
  case InputMode::OVRVISION:
    success = useOvervision();
    break;
  case InputMode::REMOTE:
    success = useRemote();
    break;
  default:
    break;
  }

  if (!success) return false;

  // 入力バックエンドを切り替えた後、左右サイズとステレオ状態を正規化し、
  // 描画側が入力方式を意識せず同じ2枚のOpenGLテクスチャを参照できるよう再構築する。
  if (defaults.input_mode != InputMode::IMAGE)
  {
    std::fill(image, image + camCount, nullptr);
  }

  // 入替設定は入力方式に依存させず、物理入力から論理的な左右眼への対応として適用する。
  // 画像データを複製せず、テクスチャの確保・更新先だけを交換する。
  const std::array<const GLubyte*, camCount> sourceImage{ image[camL], image[camR] };
  for (int eye = 0; eye < camCount; ++eye)
  {
    const int source{
      stereo && defaults.camera_swap_eyes ? camR - eye : eye };
    size[eye][0] = camera->getWidth(source);
    size[eye][1] = camera->getHeight(source);
    if (defaults.input_mode == InputMode::IMAGE)
    {
      image[eye] = sourceImage[source];
    }
  }

  // 単眼入力では右眼用にも左画像と同じ大きさを用意する
  if (!stereo)
  {
    size[camR][0] = size[camL][0];
    size[camR][1] = size[camL][1];
  }

  // それまで使っていたテクスチャを削除する
  glDeleteTextures(camCount, texture);

  // 背景画像を保存するテクスチャを作成する
  glGenTextures(camCount, texture);

  // テクスチャの境界の処理
  const GLenum border{ static_cast<GLenum>(defaults.camera_texture_repeat ? GL_REPEAT : GL_CLAMP_TO_BORDER) };

  // テクスチャを準備する
  for (int cam = 0; cam < camCount; ++cam)
  {
    // テクスチャメモリを確保する
    glBindTexture(GL_TEXTURE_2D, texture[cam]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[cam][0], size[cam][1], 0,
      GL_BGR, GL_UNSIGNED_BYTE, image[cam]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, border);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // テクスチャのアスペクト比を求める
    aspect[cam] = static_cast<GLfloat>(size[cam][0]) / static_cast<GLfloat>(size[cam][1]);
  }

  if (rectPointer)
  {
    rectPointer->setTexture(0, texture[0]);
    rectPointer->setTexture(1, texture[stereo ? 1 : 0]);
  }

  return true;
}

//
// メインプログラム
//
int GgApp::main(int argc, const char *const *const argv)
{
  // 引数を設定ファイル名に使う（指定されていなければ defaultConfig にする）
  const char *config_file{ argc > 1 ? argv[1] : defaultConfig };

  // 設定ファイルを読み込む (見つからなかったら作る)
  if (!defaults.load(config_file)) defaults.save(config_file);

  // 姿勢ファイルを読み込む（見つからなかったら作る）
  if (!attitude.load(defaultAttitude)) attitude.save(defaultAttitude);

  // GLFW を初期化する
  if (glfwInit() == GLFW_FALSE)
  {
    // GLFW の初期化に失敗した
    NOTIFY(u8"GLFW の初期化に失敗しました。");
    return EXIT_FAILURE;
  }

  // プログラム終了時には GLFW を終了する
  atexit(glfwTerminate);

  // ディスプレイの情報
  GLFWmonitor *monitor;
  int windowWidth, windowHeight;

  // フルスクリーン表示
  if (defaults.display_fullscreen)
  {
    // 接続されているモニタの数を数える
    int monitorCount;
    GLFWmonitor **const monitors(glfwGetMonitors(&monitorCount));

    // モニタの存在チェック
    if (monitorCount == 0)
    {
      NOTIFY(u8"表示可能なディスプレイが見つかりません。");
      return EXIT_FAILURE;
    }

    // セカンダリモニタがあればそれを使う
    monitor = monitors[monitorCount > defaults.display_secondary ? defaults.display_secondary : 0];

    // モニタのモードを調べる
    const GLFWvidmode *mode{ glfwGetVideoMode(monitor) };

    // ウィンドウのサイズをディスプレイのサイズにする
    windowWidth = mode->width;
    windowHeight = mode->height;
  }
  else
  {
    // プライマリモニタをウィンドウモードで使う
    monitor = nullptr;

    // ウィンドウのサイズにデフォルト値を設定する
    windowWidth = defaults.display_size[0] ? defaults.display_size[0] : defaultWindowWidth;
    windowHeight = defaults.display_size[1] ? defaults.display_size[1] : defaultWindowHeight;
  }

  // ウィンドウを開く
  Window window(windowWidth, windowHeight, windowTitle, monitor);

  // ウィンドウオブジェクトが生成されなければ終了する
  if (!window.get())
  {
    // ウインドウの作成を失敗させたと思われる設定を戻す
    defaults.display_secondary = 0;
    defaults.display_fullscreen = false;
    defaults.display_quadbuffer = false;

    // 設定ファイルを保存する
    defaults.save(config_file);

    // ウィンドウが開けなかったので終了する
    NOTIFY(u8"表示用のウィンドウを作成できませんでした。");
    return EXIT_FAILURE;
  }

  // 共有メモリを確保する
  if (!Scene::initialize(defaults.local_share_size, defaults.remote_share_size))
  {
    // 共有メモリの確保を失敗させたと思われる設定を戻す
    defaults.local_share_size = localShareSize;
    defaults.remote_share_size = remoteShareSize;

    // 設定ファイルを保存する
    defaults.save(config_file);

    // 共有メモリの確保に失敗したので終了する
    NOTIFY(u8"共有メモリが確保できませんでした。");
    return EXIT_FAILURE;
  }

  // 背景の描画に用いる矩形を作成する
  auto rect{ std::make_unique<Rect>(window, defaults.vertex_shader, defaults.fragment_shader) };
  if (!rect->get())
  {
    // シェーダが読み込めなかった
    NOTIFY(u8"背景描画用のシェーダファイルの読み込みに失敗しました。");
    return EXIT_FAILURE;
  }

  // 矩形のポインタを保存しておく
  rectPointer = rect.get();

  // 前景の描画に用いるシェーダプログラムを読み込む
  GgSimpleShader simple{ "simple.vert", "simple.frag" };
  if (!simple.get())
  {
    // シェーダが読み込めなかった
    NOTIFY(u8"図形描画用のシェーダファイルの読み込みに失敗しました。");
    return EXIT_FAILURE;
  }

  // winsock2 を初期化する
  WSAData wsaData;
  if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
  {
    NOTIFY(u8"Windows Sockets 2 の初期化に失敗しました。");
    return EXIT_FAILURE;
  }

  // winsock2 の終了処理を登録する
  atexit(reinterpret_cast<void(*)()>(WSACleanup));

  // ハンドトラッキングの初期起動
  if (defaults.hand_tracking == HAND_TRACKING_LEAP_MOTION)
  {
    if (!Scene::startLeapMotion())
    {
      defaults.hand_tracking = HAND_TRACKING_NONE;
    }
  }

  // シーングラフ
  auto scene{ std::make_unique<Scene>(defaults.scene) };

  // シーンが空の場合は初期表示フラグをオフにする
  if (scene->isEmpty()) window.setSceneVisible(false);

  // シーンにシェーダを設定する
  scene->setShader(simple);

  // 光源
  const GgSimpleShader::LightBuffer light{ lightData };

  // 入力を選択する
  if (!selectInput()) return EXIT_FAILURE;

  // ウィンドウにそのカメラを結び付ける
  window.setControlCamera(camera.get());

  // 左目用の背景画像を貼り付ける矩形にテクスチャを設定する
  rect->setTexture(0, texture[0]);

  // 右目用の背景画像を貼り付ける矩形にテクスチャを設定する
  rect->setTexture(1, texture[stereo ? 1 : 0]);

  // 通常のフレームバッファに描く
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // カリングする
  glCullFace(GL_BACK);

  // アルファブレンディングする
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // 設定値を実際の表示資源へ適用し、利用できないモードだけ単眼視へ戻す。
  // 起動時と設定再読み込み時で同じ経路を使うことで、UI の選択と描画状態を一致させる。
  const auto applyDisplayMode{ [&window](int requested)
    {
      if (window.setDisplayMode(requested)) return requested;

      if (requested == OPENXR)
        NOTIFY(u8"OpenXRの起動に失敗しました。単眼視に戻します。");
      else if (requested == QUADBUFFER)
        NOTIFY(u8"Quad Buffer Stereoが利用できません。単眼視に戻します。");

      // MONOCULAR は追加資源を必要としないため、失敗時の確実な実行モードとして使用する。
      window.setDisplayMode(MONOCULAR);
      return static_cast<int>(MONOCULAR);
    } };

  // 設定ファイルで選ばれた起動時モードについても、HMD とステレオバッファを実際に検証する。
  applyDisplayMode(defaults.display_mode);

  // メニュー
  Menu menu{ *this, window, attitude, defaults };

  // ウィンドウが開いている間くり返し描画する
  while (window)
  {
    // メニューを表示する
    if (window.isMenuVisible()) menu.show();

    // 設定ファイルが読み込まれたら描画オブジェクトを再構築する
    if (menu.isConfigReloadPending())
    {
      const auto& candidate{ menu.getPendingConfig() };
      bool status{ false };
      int displayMode{ defaults.display_mode };
      try
      {
        // 先に新しいオブジェクトを完成させ、成功した場合だけ現在のものと入れ替える
        auto newRect{ std::make_unique<Rect>(window,
          candidate.vertex_shader, candidate.fragment_shader) };
        if (newRect->get())
        {
          newRect->setTexture(0, texture[0]);
          newRect->setTexture(1, texture[stereo ? 1 : 0]);

          auto newScene{ std::make_unique<Scene>(candidate.scene, 0,
            std::filesystem::path{}, candidate) };
          if (newScene->isValid())
          {
            newScene->setShader(simple);
            rect = std::move(newRect);
            rectPointer = rect.get();
            scene = std::move(newScene);
            if (scene->isEmpty()) window.setSceneVisible(false);

            // 読み込んだ表示モードを検証し、利用不可なら単眼視を候補設定へ反映する。
            displayMode = applyDisplayMode(candidate.display_mode);

            status = true;
          }
        }
      }
      catch (const std::exception& error)
      {
#if defined(_DEBUG)
        std::cerr << "Failed to apply configuration: " << error.what() << std::endl;
#endif
      }
      catch (...)
      {
#if defined(_DEBUG)
        std::cerr << "Failed to apply configuration: unknown exception" << std::endl;
#endif
      }

      menu.finishConfigReload(status, displayMode);
    }

    // 有効なカメラの数
    int cam_count{ stereo ? camCount : 1 };

    // 有効なカメラについて
    for (int eye = 0; eye < cam_count; ++eye)
    {
      const int source{
        stereo && defaults.camera_swap_eyes ? camR - eye : eye };
      // camera->transmit 内でアトミック変数 captured が true ならテクスチャへ転送する
      camera->transmit(source, texture[eye], size[eye]);
    }

    // 描画開始
    if (window.start())
    {
      // 有効な目について
      for (int eye = 0; eye < eyeCount; ++eye)
      {
        // 図形を見せる目を選択する
        window.select(eye);

        // 背景の描画設定
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        // ローカルのヘッドトラッキングの変換行列
        const GgMatrix &mo(defaults.head_tracking
          ? window.getMo(eye) : attitude.eyeOrientation[eye].getMatrix());

        // リモートのヘッドトラッキングの変換行列
        const GgMatrix &&mr(mo * Scene::getRemoteAttitude(eye));

        // 背景を描く
        rect->draw(eye, defaults.remote_stabilize ? mr : mo, window.getSamples());

        // 図形と照準の描画設定
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);

        // 描画用のシェーダプログラムの使用開始
        simple.use(light);

        // 図形を描画する
        if (window.isSceneVisible())
        {
          // OpenXR は各眼 pose の逆変換 R^-1 * T^-1 をビュー行列に使う
          const GgMatrix sceneView{ defaults.display_mode == OPENXR
            ? (defaults.head_tracking ? window.getMo(eye) * window.getMv(eye) : ggIdentity())
            : window.getMv(eye) * window.getMo(eye) };
          scene->draw(window.getMp(eye), sceneView);
        }

        // 片目の処理を完了する
        window.commit(eye);

        // 単眼視なら終了
        if (defaults.display_mode == MONOCULAR) break;
      }
    }

    // バッファを入れ替える
    window.swapBuffers();
  }

  // 背景画像用のテクスチャを削除する
  glDeleteTextures(camCount, texture);

  return EXIT_SUCCESS;
}

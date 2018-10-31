//
// TelExistence Display System
//

// ウィンドウ関連の処理
#include "Window.h"

// ネットワーク関連の処理
#include "Network.h"

// 共有メモリ
#include "SharedMemory.h"

// カメラ関連の処理
#include "CamCv.h"
#include "CamOv.h"
#include "CamImage.h"
#include "CamRemote.h"

// シーングラフ
#include "Scene.h"

// 矩形
#include "Rect.h"

// ウィンドウモード時のウィンドウサイズの初期値
const int defaultWindowWidth(960);
const int defaultWindowHeight(540);

// ウィンドウのタイトル
const char windowTitle[] = "TED";

//
// メインプログラム
//
int main(int argc, const char *const *const argv)
{
  // 引数を設定ファイル名に使う（指定されていなければ config.json にする）
  const char *config_file(argc > 1 ? argv[1] : "config.json");

  // 設定ファイルを読み込む (見つからなかったら作る)
  if (!defaults.load(config_file)) defaults.save(config_file);

  // 共有メモリを確保する
  if (!SharedMemory::initialize(defaults.local_share_size, defaults.remote_share_size, camCount + 1))
  {
    // 共有メモリの確保に失敗した
    NOTIFY("変換行列を保持する共有メモリが確保できませんでした。");
    return EXIT_FAILURE;
  }

  // GLFW を初期化する
  if (glfwInit() == GL_FALSE)
  {
    // GLFW の初期化に失敗した
    NOTIFY("GLFW の初期化に失敗しました。");
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
    int mcount;
    GLFWmonitor **const monitors(glfwGetMonitors(&mcount));

    // モニタの存在チェック
    if (mcount == 0)
    {
      NOTIFY("GLFW の初期化をしていないか表示可能なディスプレイが見つかりません。");
      return EXIT_FAILURE;
    }

    // セカンダリモニタがあればそれを使う
    monitor = monitors[mcount > defaults.display_secondary ? defaults.display_secondary : 0];

    // モニタのモードを調べる
    const GLFWvidmode *mode(glfwGetVideoMode(monitor));

    // ウィンドウのサイズをディスプレイのサイズにする
    windowWidth = mode->width;
    windowHeight = mode->height;
  }
  else
  {
    // プライマリモニタをウィンドウモードで使う
    monitor = nullptr;

    // ウィンドウのサイズにデフォルト値を設定する
    windowWidth = defaults.display_width ? defaults.display_width : defaultWindowWidth;
    windowHeight = defaults.display_height ? defaults.display_height : defaultWindowHeight;
  }

  // ウィンドウを開く
  Window window(windowWidth, windowHeight, windowTitle, monitor);

  // ウィンドウオブジェクトが生成されなければ終了する
  if (!window.get()) return EXIT_FAILURE;

  // 背景画像のデータ
  const GLubyte *image[camCount] = { nullptr };

  // 背景画像のサイズ
  GLsizei size[camCount][2];

  // 背景画像を取得するカメラ
  std::shared_ptr<Camera> camera(nullptr);

  // ネットワークを使用する場合
  if (defaults.role != STANDALONE)
  {
    // winsock2 を初期化する
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
    {
      NOTIFY("Windows Sockets 2 の初期化に失敗しました。");
      return EXIT_FAILURE;
    }

    // winsock2 の終了処理を登録する
    atexit(reinterpret_cast<void(*)()>(WSACleanup));

    // 操縦者として動作する場合
    if (defaults.role == OPERATOR)
    {
      // リモートカメラからキャプチャするためのダミーカメラを使う
      CamRemote *const cam(new CamRemote(defaults.remote_texture_reshape));

      // 生成したカメラを記録しておく
      camera.reset(cam);

      // 操縦者側を起動する
      if (cam->open(defaults.port, defaults.address.c_str()) < 0)
      {
        NOTIFY("作業者側のデータを受け取れません。");
        return EXIT_FAILURE;
      }

      // 画像サイズを設定する
      size[camL][0] = cam->getWidth(camL);
      size[camL][1] = cam->getHeight(camL);
      size[camR][0] = cam->getWidth(camR);
      size[camR][1] = cam->getHeight(camR);
    }
  }

  // ダミーカメラが設定されていなければ
  if (!camera)
  {
    // 左カメラを使わないとき
    if (defaults.camera_left < 0)
    {
      // 右カメラだけを使うなら
      if (defaults.camera_right >= 0)
      {
        // Ovrvision Pro を使う
        CamOv *const cam(new CamOv);

        // 生成したカメラを記録しておく
        camera.reset(cam);

        // Ovrvision Pro を開く
        if (cam->open(static_cast<OVR::Camprop>(defaults.ovrvision_property)))
        {
          // Ovrvision Pro の画像サイズを得る
          size[camL][0] = cam->getWidth(camL);
          size[camL][1] = cam->getHeight(camL);
          size[camR][0] = cam->getWidth(camR);
          size[camR][1] = cam->getHeight(camR);
        }
        else
        {
          // Ovrvision Pro が使えなかった
          NOTIFY("Ovrvision Pro が使えません。");
          return EXIT_FAILURE;
        }
      }
      else
      {
        // 左カメラに画像ファイルを使う
        CamImage *const cam(new CamImage);

        // 生成したカメラを記録しておく
        camera.reset(cam);

        // 左の画像が使用可能なら
        if (cam->open(defaults.camera_left_image, camL))
        {
          // 左の画像を使用する
          image[camL] = cam->getImage(camL);

          // 左の画像のサイズを得る
          size[camL][0] = cam->getWidth(camL);
          size[camL][1] = cam->getHeight(camL);
        }
        else
        {
          // 左の画像ファイルが読み込めなかった
          NOTIFY("左の画像ファイルが使用できません。");
          return EXIT_FAILURE;
        }

        // 右の画像サイズは左と同じにしておく
        size[camR][0] = size[camL][0];
        size[camR][1] = size[camL][1];

        // 立体視表示を行うとき
        if (defaults.display_mode != MONO)
        {
          // 右の画像が指定されていれば
          if (!defaults.camera_right_image.empty())
          {
            // 右の画像が使用可能なら
            if (cam->open(defaults.camera_right_image, camR))
            {
              // 右の画像を使用する
              image[camR] = cam->getImage(camR);

              // 右の画像のサイズを得る
              size[camR][0] = cam->getWidth(camR);
              size[camR][1] = cam->getHeight(camR);
            }
            else
            {
              // 右の画像ファイルが読み込めなかった
              NOTIFY("右の画像ファイルが使用できません。");
              return EXIT_FAILURE;
            }
          }
        }
      }
    }
    else
    {
      // 左カメラに OpenCV のキャプチャデバイスを使う
      CamCv *const cam(new CamCv);

      // 生成したカメラを記録しておく
      camera.reset(cam);

      // 左カメラが使用可能なら
      if (defaults.camera_left >= 0
        ? cam->open(defaults.camera_left, camL)
        : !defaults.camera_left_movie.empty()
        ? cam->open(defaults.camera_left_movie, camL)
        : false)
      {
        // 左カメラの画像サイズを得る
        size[camL][0] = cam->getWidth(camL);
        size[camL][1] = cam->getHeight(camL);
      }
      else
      {
        // 左カメラが使えなかった
        NOTIFY("左のカメラが使えません。");
        return EXIT_FAILURE;
      }

      // 立体視表示を行うとき右カメラを使用するなら
      if (defaults.display_mode != MONO && defaults.camera_right >= 0)
      {
        if (defaults.camera_right != defaults.camera_left
          ? cam->open(defaults.camera_right, camR)
          : !defaults.camera_right_movie.empty()
          ? cam->open(defaults.camera_right_movie, camR)
          : false)
        {
          // 右カメラの画像サイズを得る
          size[camR][0] = cam->getWidth(camR);
          size[camR][1] = cam->getHeight(camR);
        }
        else
        {
          // 右カメラが使えなかった
          NOTIFY("右のカメラが使えません。");
          return EXIT_FAILURE;
        }
      }
      else
      {
        // 右の画像サイズは左と同じにしておく
        size[camR][0] = size[camL][0];
        size[camR][1] = size[camL][1];
      }
    }

    // 作業者として動作する場合
    if (defaults.role == WORKER && defaults.port > 0 && !defaults.address.empty())
    {
      // 通信スレッドを起動する
      camera->startWorker(defaults.port, defaults.address.c_str());
    }
  }

  // ウィンドウにそのカメラを結び付ける
  window.setControlCamera(camera.get());

  // テクスチャのアスペクト比
  const GLfloat texture_aspect[] =
  {
    (GLfloat(size[camL][0]) / GLfloat(size[camL][1])),
    (GLfloat(size[camR][0]) / GLfloat(size[camR][1]))
  };

  // テクスチャの境界の処理
  const GLenum border(defaults.camera_texture_repeat ? GL_REPEAT : GL_CLAMP_TO_BORDER);

  // 背景画像を保存するテクスチャ
  GLuint texture[camCount];

  // 左のテクスチャを作成する
  glGenTextures(1, texture + camL);

  // 左の背景画像を保存するテクスチャを準備する
  glBindTexture(GL_TEXTURE_2D, texture[camL]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[camL][0], size[camL][1], 0,
    GL_BGR, GL_UNSIGNED_BYTE, image[camL]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, border);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, border);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  // 右のカメラに入力するなら
  if (defaults.camera_right >= 0 || !defaults.camera_right_movie.empty() || defaults.role == OPERATOR)
  {
    // 右のテクスチャを作成する
    glGenTextures(1, texture + camR);

    // 右の背景画像を保存するテクスチャを準備する
    glBindTexture(GL_TEXTURE_2D, texture[camR]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[camR][0], size[camR][1], 0,
      GL_BGR, GL_UNSIGNED_BYTE, image[camR]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, border);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
  }
  else
  {
    // 右のテクスチャは左のテクスチャと同じにする
    texture[camR] = texture[camL];
  }

  // 通常のフレームバッファに戻す
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // 図形描画用のシェーダプログラムを読み込む
  GgSimpleShader simple("simple.vert", "simple.frag");
  if (!simple.get())
  {
    // シェーダが読み込めなかった
    NOTIFY("図形描画用のシェーダファイルの読み込みに失敗しました。");
    return EXIT_FAILURE;
  }

  // Leap Motion の listener と controller を作る
  LeapListener listener;

  // Leap Motion の listener が controller からイベントを受け取るようにする
  Leap::Controller controller;
  controller.addListener(listener);
  controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);
  controller.setPolicy(Leap::Controller::POLICY_ALLOW_PAUSE_RESUME);

  // シーンの変換行列を初期化する
  Scene::initialize();

  // シーングラフ
  Scene scene(defaults.scene, simple);

  // 背景描画用の矩形を作成する
  Rect rect(defaults.vertex_shader, defaults.fragment_shader);

  // シェーダプログラム名を調べる
  if (!rect.get())
  {
    // シェーダが読み込めなかった
    NOTIFY("背景描画用のシェーダファイルの読み込みに失敗しました。");
    return EXIT_FAILURE;
  }

  // 光源
  const GgSimpleShader::LightBuffer light(lightData);

  // 材質
  const GgSimpleShader::MaterialBuffer material(materialData);

  // カリング
  glCullFace(GL_BACK);

  // アルファブレンディング
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // 描画回数
  const int drawCount(defaults.display_mode == MONO ? 1 : camCount);

  // ウィンドウが開いている間くり返し描画する
  while (!window.shouldClose())
  {
    // ローカルとリモートの変換行列を共有メモリから取り出す
    Scene::setup();

    // 左カメラをロックして画像を転送する
    camera->transmit(camL, texture[camL], size[camL]);

    // 右のテクスチャが有効になっていれば
    if (texture[camR] != texture[camL])
    {
      // 右カメラをロックして画像を転送する
      camera->transmit(camR, texture[camR], size[camR]);
    }

    // 描画開始
    if (window.start())
    {
      for (int eye = 0; eye < drawCount; ++eye)
      {
        // 図形を見せる目を選択する
        window.select(eye);

        // 背景の描画設定
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        // スクリーンの格子間隔
        rect.setGap(window.getGap());

        // スクリーンのサイズと中心位置
        rect.setScreen(window.getScreen(eye));

        // 焦点距離
        rect.setFocal(window.getFocal());

        // 背景テクスチャの半径と中心位置
        rect.setCircle(window.getCircle());

        // ローカルのヘッドトラッキングの変換行列
        const GgMatrix mo(window.getMo(eye));
        const GgMatrix ml(defaults.camera_tracking ? mo : window.getQr(eye).getMatrix());

        // リモートのヘッドトラッキングの変換行列
        const GgMatrix mr(ml * Scene::getRemoteAttitude(eye));

        // 背景を描く
        rect.draw(texture[eye], camera->isOperator() && defaults.remote_stabilize ? mr : ml, window.getSamples());

        // 図形と照準の描画設定
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);

        // 描画用のシェーダプログラムの使用開始
        simple.use();
        simple.selectLight(light);
        simple.selectMaterial(material);

        // 図形を描画する
        printf("%d\n", eye);
        if (window.showScene) scene.draw(window.getMp(eye), window.getMv(eye) * mo);

        // 片目の処理を完了する
        window.commit(eye);
      }
    }

    // バッファを入れ替える
    window.swapBuffers();
  }

  // 背景画像用のテクスチャを削除する
  glDeleteBuffers(1, &texture[camL]);
  if (texture[camR] != texture[camL]) glDeleteBuffers(1, &texture[camR]);
}

//
// TelExistence Display System
//

// ウィンドウ関連の処理
#include "Window.h"

// ネットワーク関連の処理
#include "Network.h"

// カメラ関連の処理
#include "CamCv.h"
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

//
// メインプログラム
//
int main(int argc, const char *const *const argv)
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
      NOTIFY("表示可能なディスプレイが見つかりません。");
      return EXIT_FAILURE;
    }

    // セカンダリモニタがあればそれを使う
    monitor = monitors[mcount > defaults.display_secondary ? defaults.display_secondary : 0];

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
    windowWidth = defaults.display_width ? defaults.display_width : defaultWindowWidth;
    windowHeight = defaults.display_height ? defaults.display_height : defaultWindowHeight;
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
    NOTIFY("表示用のウィンドウを作成できませんでした。");
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
    NOTIFY("共有メモリが確保できませんでした。");
    return EXIT_FAILURE;
  }

  // 背景の描画に用いる矩形を作成する
  Rect rect{ window, defaults.vertex_shader, defaults.fragment_shader };
  if (!rect.get())
  {
    // シェーダが読み込めなかった
    NOTIFY("背景描画用のシェーダファイルの読み込みに失敗しました。");
    return EXIT_FAILURE;
  }

  // 前景の描画に用いるシェーダプログラムを読み込む
  GgSimpleShader simple{ "simple.vert", "simple.frag" };
  if (!simple.get())
  {
    // シェーダが読み込めなかった
    NOTIFY("図形描画用のシェーダファイルの読み込みに失敗しました。");
    return EXIT_FAILURE;
  }

  // シーングラフ
  Scene scene{ defaults.scene };

  // シーンにシェーダを設定する
  scene.setShader(simple);

  // 光源
  const GgSimpleShader::LightBuffer light{ lightData };

  // 背景画像のデータ
  const GLubyte *image[camCount]{ nullptr };

  // 背景画像のサイズ
  GLsizei size[camCount][2];

  // 背景画像を取得するカメラ
  std::shared_ptr<Camera> camera;

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
    // 左カメラから動画を入力するとき
    if (defaults.camera_left >= 0 || !defaults.camera_left_movie.empty())
    {
      // 左カメラに OpenCV のキャプチャデバイスを使う
      CamCv *const cam{ new CamCv };

      // 生成したカメラを記録しておく
      camera.reset(cam);

      // ムービーファイルが指定されていれば
      if (!defaults.camera_left_movie.empty())
      {
        // ムービーファイルを開く
        if (!cam->open(defaults.camera_left_movie, camL))
        {
          NOTIFY("左の動画ファイルが使用できません。");
          return EXIT_FAILURE;
        }
      }
      else
      {
        // カメラデバイスを開く
        if (!cam->open(defaults.camera_left, camL))
        {
          NOTIFY("左のカメラが使用できません。");
          return EXIT_FAILURE;
        }
      }

      // 左カメラのサイズを得る
      size[camL][0] = cam->getWidth(camL);
      size[camL][1] = cam->getHeight(camL);

      // 右カメラのサイズは左と同じにしておく
      size[camR][0] = size[camL][0];
      size[camR][1] = size[camL][1];

      // 立体視表示を行うとき
      if (defaults.display_mode != MONOCULAR)
      {
        // 右カメラに左と異なるムービーファイルが指定されていれば
        if (!defaults.camera_right_movie.empty() && defaults.camera_right_movie != defaults.camera_left_movie)
        {
          // ムービーファイルを開く
          if (!cam->open(defaults.camera_right_movie, camR))
          {
            NOTIFY("右の動画ファイルが使用できません。");
            return EXIT_FAILURE;
          }

          // 右カメラのサイズを得る
          size[camR][0] = cam->getWidth(camR);
          size[camR][1] = cam->getHeight(camR);
        }
        else if (defaults.camera_right >= 0 && defaults.camera_right != defaults.camera_left)
        {
          // カメラデバイスを開く
          if (!cam->open(defaults.camera_right, camR))
          {
            NOTIFY("右のカメラが使用できません。");
            return EXIT_FAILURE;
          }

          // 右カメラのサイズを得る
          size[camR][0] = cam->getWidth(camR);
          size[camR][1] = cam->getHeight(camR);
        }
      }
    }
    else
    {
      // 右カメラだけを使うなら
      if (defaults.camera_right >= 0)
      {
        // Ovrvision Pro を使う
        CamOv *const cam{ new CamOv };

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
        CamImage *const cam{ new CamImage };

        // 生成したカメラを記録しておく
        camera.reset(cam);

        // 左の画像が使用可能なら
        if (!cam->open(defaults.camera_left_image, camL))
        {
          // 左の画像ファイルが読み込めなかった
          NOTIFY("左の画像ファイルが使用できません。");
          return EXIT_FAILURE;
        }

        // 左の画像を使用する
        image[camL] = cam->getImage(camL);

        // 左カメラのサイズを得る
        size[camL][0] = camera->getWidth(camL);
        size[camL][1] = camera->getHeight(camL);

        // 右カメラのサイズは左と同じにしておく
        size[camR][0] = size[camL][0];
        size[camR][1] = size[camL][1];

        // 立体視表示を行うとき
        if (defaults.display_mode != MONOCULAR)
        {
          // 右の画像が指定されていれば
          if (!defaults.camera_right_image.empty())
          {
            // 右の画像が使用可能なら
            if (!cam->open(defaults.camera_right_image, camR))
            {
              // 右の画像ファイルが読み込めなかった
              NOTIFY("右の画像ファイルが使用できません。");
              return EXIT_FAILURE;
            }

            // 右の画像を使用する
            image[camR] = cam->getImage(camR);

            // 右の画像のサイズを得る
            size[camR][0] = cam->getWidth(camR);
            size[camR][1] = cam->getHeight(camR);
          }
        }
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
  const GLfloat texture_aspect[]
  {
    (GLfloat(size[camL][0]) / GLfloat(size[camL][1])),
    (GLfloat(size[camR][0]) / GLfloat(size[camR][1]))
  };

  // テクスチャの境界の処理
  const GLenum border(defaults.camera_texture_repeat ? GL_REPEAT : GL_CLAMP_TO_BORDER);

  // 背景画像を保存するテクスチャを作成する
  GLuint texture[camCount];
  glGenTextures(camCount, texture);

  // 背景画像を保存するテクスチャを準備する
  for (int cam = 0; cam < camCount; ++cam)
  {
    glBindTexture(GL_TEXTURE_2D, texture[cam]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[cam][0], size[cam][1], 0,
      GL_BGR, GL_UNSIGNED_BYTE, image[cam]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, border);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
  }

  // 通常のフレームバッファに描く
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // カリングする
  glCullFace(GL_BACK);

  // アルファブレンディングする
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // 描画回数は単眼視なら 1、それ以外ならカメラの数
  int drawCount{ defaults.display_mode == MONOCULAR ? 1 : camCount };

  // デフォルトが OCULUS なら Oculus Rift を起動する
  if (defaults.display_mode == OCULUS) window.startOculus();

  // メニュー
  Menu menu{ window, scene, attitude };

  // ウィンドウが開いている間くり返し描画する
  while (window)
  {
    // メニューを表示する
    if (window.showMenu) menu.show();

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
      // すべての目について
      for (int eye = 0; eye < drawCount; ++eye)
      {
        // 図形を見せる目を選択する
        window.select(eye);

        // 背景の描画設定
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        // ローカルのヘッドトラッキングの変換行列
        const GgMatrix &&mo(defaults.camera_tracking
          ? window.getMo(eye) * attitude.eyeOrientation[eye].getMatrix()
          : attitude.eyeOrientation[eye].getMatrix());

        // リモートのヘッドトラッキングの変換行列
        const GgMatrix &&mr(mo * Scene::getRemoteAttitude(eye));

        // 背景を描く
        rect.draw(eye, texture, defaults.remote_stabilize
          ? mr : mo, window.getSamples());

        // 図形と照準の描画設定
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);

        // 描画用のシェーダプログラムの使用開始
        simple.use(light);

        // 図形を描画する
        if (window.showScene) scene.draw(window.getMp(eye), window.getMv(eye) * window.getMo(eye));

        // 片目の処理を完了する
        window.commit(eye);
      }
    }

    // バッファを入れ替える
    window.swapBuffers();
  }

  // 背景画像用のテクスチャを削除する
  glDeleteBuffers(camCount, texture);
}

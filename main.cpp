//
// 産総研 3D ビューア
//

// 標準ライブラリ
#include <iostream>
#include <typeinfo>
#include <memory>

// ネットワーク関連の処理
#include "Network.h"

// ウィンドウ関連の処理
#include "Window.h"

// カメラ関連の処理
#include "Camera.h"
#include "CamCv.h"
#include "CamOv.h"
#include "CamImage.h"
#include "CamRemote.h"

// 矩形
#include "Rect.h"

// シーングラフ
#include "Scene.h"

// Leap Motion
#include "LeapListener.h"

// ウィンドウモード時のウィンドウサイズの初期値
const int defaultWindowWidth(960);
const int defaultWindowHeight(540);

// ウィンドウのタイトル
const char windowTitle[] = "TED";

// メッセージボックスのタイトル
const LPCWSTR messageTitle = L"TED";

// ファイルマッピングオブジェクト名
const LPCWSTR localMutexName = L"TED_LOCAL_MUTEX";
const LPCWSTR localShareName = L"TED_LOCAL_SHARE";
const LPCWSTR remoteMutexName = L"TED_REMOTE_MUTEX";
const LPCWSTR remoteShareName = L"TED_REMOTE_SHARE";

//
// メインプログラム
//
int main(int argc, const char *const *const argv)
{
  // 引数を設定ファイル名に使う（指定されていなければ config.json にする）
  const char *config_file(argc > 1 ? argv[1] : "config.json");

  // 設定ファイルを読み込む (見つからなかったら作る)
  if (!defaults.load(config_file)) defaults.save(config_file);

  // ローカルの変換行列を保持する共有メモリを確保する
  std::unique_ptr<SharedMemory> localAttitude(new SharedMemory(localMutexName, localShareName, defaults.local_share_size));

  // ローカルの変換行列を保持する共有メモリが確保できたかチェックする
  if (!localAttitude->get())
  {
    // 共有メモリが確保できなかった
    NOTIFY("ローカルの変換行列を保持する共有メモリが確保できませんでした。");
    return EXIT_FAILURE;
  }

  // リモートの変換行列を保持する共有メモリを確保する
  std::unique_ptr<SharedMemory> remoteAttitude(new SharedMemory(remoteMutexName, remoteShareName, defaults.remote_share_size));

  // リモートの変換行列を保持する共有メモリが確保できたかチェックする
  if (!remoteAttitude->get())
  {
    // 共有メモリが確保できなかった
    NOTIFY("リモートの変換行列を保持する共有メモリが確保できませんでした。");
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
  if (defaults.display_mode != MONO && defaults.display_mode != OCULUS && !debug)
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
    windowWidth = defaultWindowWidth;
    windowHeight = defaultWindowHeight;
  }

  // ウィンドウを開く
  Window window(windowWidth, windowHeight, windowTitle, monitor);

  // ウィンドウオブジェクトが生成されなければ終了する
  if (!window.get()) return EXIT_FAILURE;

  // 背景画像を保存するテクスチャ
  GLuint texture[eyeCount];

  // 背景画像のデータ
  const GLubyte *image[eyeCount] = { nullptr };

  // 背景画像のサイズ
  GLsizei size[eyeCount][2];

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

      // カメラで参照する変換行列の票を選択する
      camera->selectTable(localAttitude.get(), remoteAttitude.get());

      // 操縦者側を起動する
      if (cam->open(defaults.port, defaults.address.c_str(), texture) < 0)
      {
        NOTIFY("作業者側からデータが送られてきません。");
        return EXIT_FAILURE;
      }

      // 左右のテクスチャを作成する
      glGenTextures(2, texture);

      // 画像サイズを設定する
      size[eyeL][0] = cam->getWidth(eyeL);
      size[eyeL][1] = cam->getHeight(eyeL);
      size[eyeR][0] = cam->getWidth(eyeR);
      size[eyeR][1] = cam->getHeight(eyeR);
    }
  }

  // ダミーカメラが設定されていなければ
  if (!camera)
  {
    // 左のテクスチャを作成する
    glGenTextures(1, texture + eyeL);

    // 右のカメラに入力するなら
    if (defaults.camera_right >= 0 || !defaults.camera_right_movie.empty())
    {
      // 右のテクスチャを作成する
      glGenTextures(1, texture + eyeR);
    }
    else
    {
      // 右のテクスチャは左のテクスチャと同じにする
      texture[camR] = texture[camL];
    }

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
          size[eyeL][0] = cam->getWidth(eyeL);
          size[eyeL][1] = cam->getHeight(eyeL);
          size[eyeR][0] = cam->getWidth(eyeR);
          size[eyeR][1] = cam->getHeight(eyeR);
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
        if (cam->open(defaults.camera_left_image, eyeL))
        {
          // 左の画像を使用する
          image[eyeL] = cam->getImage(eyeL);

          // 左の画像のサイズを得る
          size[eyeL][0] = cam->getWidth(eyeL);
          size[eyeL][1] = cam->getHeight(eyeL);
        }
        else
        {
          // 左の画像ファイルが読み込めなかった
          NOTIFY("左の画像ファイルが使用できません。");
          return EXIT_FAILURE;
        }

        // 右の画像サイズは左と同じにしておく
        size[eyeR][0] = size[eyeL][0];
        size[eyeR][1] = size[eyeL][1];

        // 立体視表示を行うとき
        if (defaults.display_mode != MONO)
        {
          // 右の画像が指定されていれば
          if (!defaults.camera_right_image.empty())
          {
            // 右の画像が使用可能なら
            if (cam->open(defaults.camera_right_image, eyeR))
            {
              // 右の画像を使用する
              image[eyeR] = cam->getImage(eyeR);

              // 右の画像のサイズを得る
              size[eyeR][0] = cam->getWidth(eyeR);
              size[eyeR][1] = cam->getHeight(eyeR);
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
        ? cam->open(defaults.camera_left, eyeL)
        : !defaults.camera_left_movie.empty()
        ? cam->open(defaults.camera_left_movie, eyeL)
        : false)
      {
        // 左カメラの画像サイズを得る
        size[eyeL][0] = cam->getWidth(eyeL);
        size[eyeL][1] = cam->getHeight(eyeL);
      }
      else
      {
        // 左カメラが使えなかった
        NOTIFY("左のカメラが使えません。");
        return EXIT_FAILURE;
      }

      // 立体視表示を行うとき
      if (defaults.display_mode != MONO)
      {
        // 右カメラを使用するなら
        if (defaults.camera_right >= 0)
        {
          if (defaults.camera_right != defaults.camera_left
            ? cam->open(defaults.camera_right, eyeR)
            : !defaults.camera_right_movie.empty()
            ? cam->open(defaults.camera_right_movie, eyeR)
            : false)
          {
            // 右カメラの画像サイズを得る
            size[eyeR][0] = cam->getWidth(eyeR);
            size[eyeR][1] = cam->getHeight(eyeR);
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
          size[eyeR][0] = size[eyeL][0];
          size[eyeR][1] = size[eyeL][1];
        }
      }
    }

    // カメラで参照する変換行列の票を選択する
    camera->selectTable(localAttitude.get(), remoteAttitude.get());

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
    (GLfloat(size[eyeL][0]) / GLfloat(size[eyeL][1])),
    (GLfloat(size[eyeR][0]) / GLfloat(size[eyeR][1]))
  };

  // テクスチャの境界の処理
  const GLenum border(defaults.camera_texture_repeat ? GL_REPEAT : GL_CLAMP_TO_BORDER);

  // 左の背景画像を保存するテクスチャを準備する
  glBindTexture(GL_TEXTURE_2D, texture[eyeL]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[eyeL][0], size[eyeL][1], 0,
    GL_BGR, GL_UNSIGNED_BYTE, image[eyeL]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, border);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, border);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  // 右のテクスチャが左のテクスチャと異なるなら
  if (texture[eyeR] != texture[eyeL])
  {
    // 右の背景画像を保存するテクスチャを準備する
    glBindTexture(GL_TEXTURE_2D, texture[eyeR]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[eyeR][0], size[eyeR][1], 0,
      GL_BGR, GL_UNSIGNED_BYTE, image[eyeR]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, border);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
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

  // ローカルの姿勢の姿勢のインデックスを得る
  int localAttitudeIndex[eyeCount];
  for (int eye = 0; eye < eyeCount; ++eye) localAttitudeIndex[eye] = localAttitude->push(ggIdentity());

  // Leap Motion の listener と controller を作る
  LeapListener listener(localAttitude.get());

  // Leap Motion の listener が controller からイベントを受け取るようにする
  Leap::Controller controller;
  controller.addListener(listener);
  controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);
  controller.setPolicy(Leap::Controller::POLICY_ALLOW_PAUSE_RESUME);

  // シーンの変換行列を Leap Motion で制御できるようにする
  Scene::selectController(&listener);

  // シーンで参照する変換行列の表を選択する
  Scene::selectTable(localAttitude.get(), remoteAttitude.get());

  // シーングラフ
  Scene scene(defaults.scene, simple);
  Scene target(defaults.target, simple);
  Scene remote(defaults.remote, simple);

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
  const GgSimpleLightBuffer light(lightData);

  // 材質
  const GgSimpleMaterialBuffer material(materialData);

  // カリング
  glCullFace(GL_BACK);

  // アルファブレンディング
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // 描画回数
  const int drawCount(defaults.display_mode == MONO ? 1 : 2);

  // ウィンドウが開いている間くり返し描画する
  while (!window.shouldClose())
  {
    // 左カメラをロックして画像を転送する
    camera->transmit(eyeL, texture[eyeL], size[eyeL]);

    // 右のテクスチャが有効になっていれば
    if (texture[eyeR] != texture[eyeL])
    {
      // 右カメラをロックして画像を転送する
      camera->transmit(eyeR, texture[eyeR], size[eyeR]);
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
        const GgQuaternion qo(window.getQo(eye));
        const GgMatrix mo(qo.getMatrix());

        // ローカルのヘッドトラッキング情報を共有メモリに保存する
        localAttitude->set(localAttitudeIndex[eye], mo);

        // リモートのヘッドトラッキングの変換行列
        GgMatrix mr;

        // 背景を描く
        rect.draw(texture[eye], defaults.camera_tracking
          ? (camera->isOperator() ? mr = mo * camera->getRemoteAttitude(eye).transpose().get() : mo)
          : ggIdentity(), window.getSamples());

        // 図形と照準の描画設定
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);

        // 描画用のシェーダプログラムの使用開始
        simple.use();
        simple.selectLight(light);
        simple.selectMaterial(material);

        // 図形を描画する
        if (window.showScene) scene.draw(window.getMp(eye), mo * window.getMv(eye), window.getMm());

        // ローカルの照準を描画する
        if (window.showTarget) target.draw(window.getMp(eye), window.getMv(eye));

        // ネットワークが有効の時
        if (window.showTarget && camera->useNetwork())
        {
          // リモートの回転変換行列を使ってリモートの照準を描画する
          remote.draw(window.getMp(eye), mr * window.getMv(eye));
        }

        // 片目の処理を完了する
        window.commit(eye);
      }
    }

    // バッファを入れ替える
    window.swapBuffers();
  }

  // 背景画像用のテクスチャを削除する
  glDeleteBuffers(1, &texture[eyeL]);
  if (texture[eyeR] != texture[eyeL]) glDeleteBuffers(1, &texture[eyeR]);
}

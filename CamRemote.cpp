//
// リモートカメラからキャプチャ
//
#include "CamRemote.h"

// シーングラフ
#include "Scene.h"

// 共有メモリ
#include "SharedMemory.h"

// コンストラクタ
CamRemote::CamRemote(double width, double height, bool reshape)
  : reshape{ reshape }
{
  if (reshape)
  {
    // 背景画像の大きさ
    size[camL] = size[camR] = cv::Size(static_cast<int>(width), static_cast<int>(height));

    // 背景画像の変形に使うフレームバッファオブジェクト
    glGenFramebuffers(1, &fb);

    // リモートから取得したフレームのサンプリングに使うテクスチャ
    glGenTextures(camCount, resample);

    // 魚眼画像に変形するシェーダ
    shader = ggLoadShader("mesh.vert", "mesh.frag");
    gapLoc = glGetUniformLocation(shader, "gap");
    screenLoc = glGetUniformLocation(shader, "screen");
    rotationLoc = glGetUniformLocation(shader, "rotation");
    imageLoc = glGetUniformLocation(shader, "image");
  }
}

// デストラクタ
CamRemote::~CamRemote()
{
  // スレッドを停止する
  stop();

  if (reshape)
  {
    // 背景画像のタイリングに使うフレームバッファオブジェクト
    glDeleteFramebuffers(1, &fb);

    // リモートから取得したフレームのサンプリングに使うテクスチャ
    glDeleteTextures(camCount, resample);

    // 魚眼画像に変形するシェーダ
    glDeleteProgram(shader);
  }
}

// 操縦者側の起動
int CamRemote::open(unsigned short port, const char* address, const GLfloat* fov, int samples)
{
  // すでに確保されている作業用メモリを破棄する
  delete[] sendbuf, recvbuf;
  sendbuf = recvbuf = nullptr;

  // 操縦者として初期化する
  const int ret(network.initialize(1, port, address));
  if (ret != 0) return ret;

  // 作業用のメモリを確保する（これは Camera のデストラクタで delete する）
  sendbuf = new uchar[maxFrameSize];
  recvbuf = new uchar[maxFrameSize];

  // ヘッダのフォーマット
  unsigned int* const head(reinterpret_cast<unsigned int*>(recvbuf));

  // 1 フレーム受け取って
  for (int i = 0;;)
  {
    const int ret(network.recvData(recvbuf, maxFrameSize));
#if defined(DEBUG)
    std::cerr << "CamRemote open:" << ret << ',' << head[camL] << ',' << head[camR] << '\n';
#endif
    if (ret > 0 && head[camL] > 0) break;
    if (++i > receiveRetry) return ret;
  }

  // 受信した変換行列の格納場所
  GgMatrix* const body(reinterpret_cast<GgMatrix*>(head + headLength));

  // 変換行列を共有メモリに格納する
  Scene::storeRemoteAttitude(body, head[camCount]);

  // 左フレームの保存先 (変換行列の最後)
  uchar* const data(reinterpret_cast<uchar*>(body + head[camCount]));

  // 符号化されたデータの一時保存先
  std::vector<GLubyte> encoded;

  // 左フレームデータを vector に変換して
  encoded.assign(data, data + head[camL]);

  // 左フレームをデコードする
  remote[camL] = cv::imdecode(cv::Mat(encoded), 1);

  // リモートから取得したフレームのサイズ
  cv::Size rsize[camCount];

  // 左フレームのサイズを求める
  rsize[camL] = remote[camL].size();

  // 右フレームが存在すれば
  if (head[camR] > 0)
  {
    // 右フレームデータを vector に変換して
    encoded.assign(data + head[camL], data + head[camL] + head[camR]);

    // 右フレームをデコードする
    remote[camR] = cv::imdecode(cv::Mat(encoded), 1);

    // 右フレームのサイズを求める
    rsize[camR] = remote[camR].size();
  }
  else
  {
    // 右フレームは左と同じにする
    remote[camR] = remote[camL];

    // 右フレームのサイズは左フレームと同じにする
    rsize[camR] = rsize[camL];
  }

  if (reshape)
  {
    // 背景画像の変形に使うメッシュの縦横の格子点数を求め
    const GLfloat aspect(static_cast<GLfloat>(size[camL].width) / static_cast<GLfloat>(size[camL].height));
    slices = static_cast<GLsizei>(sqrt(aspect * static_cast<GLfloat>(samples)));
    stacks = samples / slices;

    // 背景画像の変形に使うメッシュの縦横の格子間隔を求める
    gap[0] = 2.0f / static_cast<GLfloat>(slices - 1);
    gap[1] = 2.0f / static_cast<GLfloat>(stacks - 1);

    // 背景画像を取得するリモートカメラの画角
    screen[0] = tan(fov[0]);
    screen[1] = tan(fov[1]);

    for (int cam = 0; cam < camCount; ++cam)
    {
      // リモートから取得したフレームのサンプリングに使うテクスチャを準備する
      glBindTexture(GL_TEXTURE_2D, resample[cam]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, rsize[cam].width, rsize[cam].height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    }
  }
  else
  {
    // ドーム画像に変形しないときは背景テクスチャのサイズにする
    size[camL] = rsize[camL];
    size[camR] = rsize[camR];
  }

  // 通信スレッドを開始する
  run[camL] = true;
  sendThread = std::thread([this]() { send(); });
  recvThread = std::thread([this]() { recv(); });

  return 0;
}

// 左カメラをロックして画像をテクスチャに転送する
bool CamRemote::transmit(int cam, GLuint texture, const GLsizei* size)
{
  // 画像のサイズ
  const GLsizei fsize[] = { remote[cam].cols, remote[cam].rows };

  if (!reshape) return Camera::transmit(cam, texture, fsize);

  if (Camera::transmit(cam, resample[cam], fsize))
  {
    // テクスチャ変形用のシェーダ
    glUseProgram(shader);
    glUniform2fv(gapLoc, 1, gap);
    glUniform2fv(screenLoc, 1, screen);
    glUniform1i(imageLoc, 0);
    glViewport(0, 0, size[0], size[1]);

    // 背景画像の変形に使うフレームバッファオブジェクトに切り替える
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

    // リモートから取得したフレームのサンプリングに使うテクスチャを指定する
    glBindTexture(GL_TEXTURE_2D, resample[cam]);

    // リモートのヘッドトラッキング情報を設定してレンダリング
    glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, Scene::getRemoteAttitude(cam).get());
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, slices * 2, stacks - 1);

    // レンダリング先を通常のフレームバッファに戻す
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // フレームバッファオブジェクトのカラーバッファに使ったテクスチャを指定する
    glBindTexture(GL_TEXTURE_2D, texture);

    return true;
  }

  return false;
}

// リモートの映像と姿勢を受信する
void CamRemote::recv()
{
  // スレッドが実行可の間
  while (run[camL])
  {
    // 姿勢データと画像データを受信する
    const int ret{ network.recvData(recvbuf, maxFrameSize) };

#if defined(DEBUG)
    std::cerr << "CamRemote recv:" << ret << '\n';
#endif

    // サイズが 0 なら終了する
    if (ret == 0) return;

    // エラーがなければデータを読み込む
    if (ret > 0 && network.checkRemote())
    {
      // ヘッダのフォーマット
      const auto head{ reinterpret_cast<unsigned int*>(recvbuf) };

      // 受信した変換行列の格納場所
      const auto body{ reinterpret_cast<GgMatrix*>(head + headLength) };

      // 変換行列を共有メモリに格納する
      Scene::storeRemoteAttitude(body, head[camCount]);

      // 左フレームの保存先 (変換行列の最後)
      const auto data{ reinterpret_cast<uchar*>(body + head[camCount]) };

      // リモートから取得したフレームのサイズ
      cv::Size rsize[camCount];

      // 符号化されたデータの一時保存先
      std::vector<GLubyte> encoded;

      // 左バッファが空のとき左フレームが送られてきていれば
      if (!captured[camL] && head[camL] > 0)
      {
        // 左フレームデータを vector に変換して
        encoded.assign(data, data + head[camL]);

        // 左フレームをデコードして保存し
        remote[camL] = cv::imdecode(cv::Mat(encoded), 1);

        // 左フレームのサイズを求めておいて
        rsize[camL] = remote[camL].size();

        // 左画像をロックし
        captureMutex[camL].lock();

        // 左画像を更新したら
        image[camL] = remote[camL];

        // 左フレームの取得の完了を記録して
        captured[camL] = true;

        // 左画像のロックを解除する
        captureMutex[camL].unlock();
      }

      // 右バッファが空のとき右フレームが送られてきていれば
      if (!captured[camR] && head[camR] > 0)
      {
        // 右フレームデータを vector に変換して
        encoded.assign(data + head[camL], data + head[camL] + head[camR]);

        // 右フレームをデコードして保存し
        remote[camR] = cv::imdecode(cv::Mat(encoded), 1);

        // 右フレームのサイズを求めておいて
        rsize[camR] = remote[camR].size();

        // 右画像をロックし
        captureMutex[camR].lock();

        // 右画像を更新したら
        image[camR] = remote[camR];

        // 右フレームの取得の完了を記録して
        captured[camR] = true;

        // 右画像のロックを解除する
        captureMutex[camR].unlock();
      }

      // 右フレームが保存されていなければ
      if (remote[camR].empty())
      {
        // 右フレームのサイズは左フレームと同じにして
        rsize[camR] = rsize[camL];

        // 右画像をロックし
        captureMutex[camR].lock();

        // 右画像は左フレームと同じにして
        image[camR] = remote[camL];

        // 右フレームの取得の完了を記録して
        captured[camR] = true;

        // 右画像のロックを解除する
        captureMutex[camR].unlock();
      }
    }

    // 他のスレッドがリソースにアクセスするために少し待つ
    std::this_thread::sleep_for(std::chrono::milliseconds(minDelay));
  }
}

// ローカルの姿勢を送信する
void CamRemote::send()
{
  // 直前のフレームの送信時刻
  auto last{ glfwGetTime() };

  // カメラスレッドが実行可の間
  while (run[camL])
  {
    // ヘッダのフォーマット
    const auto head{ reinterpret_cast<unsigned int*>(sendbuf) };

    // 左右のフレームのサイズは 0 にする
    head[camL] = head[camR] = 0;

    // 変換行列の数を保存する
    head[camCount] = Scene::getLocalAttitudeSize();

    // 送信する変換行列の格納場所
    const auto body{ reinterpret_cast<GgMatrix*>(head + headLength) };

    // 変換行列を共有メモリから取り出す
    Scene::loadLocalAttitude(body, head[camCount]);

    // 左フレームの保存先 (変換行列の最後)
    const auto data{ reinterpret_cast<uchar*>(body + head[camCount]) };

    // フレームを送信する
    network.sendData(sendbuf, static_cast<unsigned int>(data - sendbuf));

    // 現在時刻
    const auto now{ glfwGetTime() };

    // 次のフレームの送信時刻までの残り時間
    const auto remain{ static_cast<long long>(last + capture_interval - now) };

#if defined(DEBUG)
    std::cerr << "send remain = " << remain << '\n';
#endif

    // 直前のフレームの送信時刻を更新する
    last = now;

    // 残り時間分遅延させる
    const auto delay{ remain > minDelay ? remain : minDelay };

    // 次のフレームの送信時刻まで待つ
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
  }

  // ループを抜けるときに EOF を送信する
  network.sendEof();
}

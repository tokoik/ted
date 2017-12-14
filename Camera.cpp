//
// カメラ関連の処理
//
#include "Camera.h"

#if defined(_WIN32)
#  define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#  if defined(_DEBUG)
#    define CV_EXT_STR "d.lib"
#  else
#    define CV_EXT_STR ".lib"
#  endif
#  pragma comment(lib, "opencv_world" CV_VERSION_STR CV_EXT_STR)
#endif

// コンストラクタ
Camera::Camera()
{
  // 作業用のメモリ領域
  recvbuf = sendbuf = nullptr;

  // キャプチャされる画像のフォーマット
  format = GL_BGR;

  // 圧縮設定
  param.push_back(CV_IMWRITE_JPEG_QUALITY);
  param.push_back(defaults.remote_texture_quality);

  for (int cam = 0; cam < camCount; ++cam)
  {
    // 画像がまだ取得されていないことを記録しておく
    buffer[cam] = nullptr;

    // スレッドが停止状態であることを記録しておく
    run[cam] = false;
  }
}

// デストラクタ
Camera::~Camera()
{
  // 作業用のメモリを開放する
  delete[] recvbuf, sendbuf;
  recvbuf = sendbuf = nullptr;
}

// スレッドを停止する
void Camera::stop()
{
  // キャプチャスレッドを止める
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (run[cam])
    {
      // キャプチャスレッドのループを止める
      captureMutex[cam].lock();
      run[cam] = false;
      captureMutex[cam].unlock();

      // キャプチャスレッドが終了するのを待つ
      if (captureThread[cam].joinable()) captureThread[cam].join();
    }
  }

  // ネットワークスレッドを止める
  if (useNetwork())
  {
    // 受信スレッドが終了するのを待つ
    if (recvThread.joinable()) recvThread.join();

    // 送信スレッドが終了するのを待つ
    if (sendThread.joinable()) sendThread.join();
  }
}

// カメラをロックして画像をテクスチャに転送する
bool Camera::transmit(int cam, GLuint texture, const GLsizei *size)
{
  // カメラのロックを試みる
  if (captureMutex[cam].try_lock())
  {
    // 新しいデータが到着していたら
    if (buffer[cam])
    {
      // データをテクスチャに転送する
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size[0], size[1], format, GL_UNSIGNED_BYTE, buffer[cam]);

      // データの転送完了を記録する
      buffer[cam] = nullptr;
    }

    // 左カメラのロックを解除する
    captureMutex[cam].unlock();

    // テクスチャを転送した
    return true;
  }

  // テクスチャを転送していない
  return false;
}

// リモートの姿勢を受信する
void Camera::recv()
{
  // キャプチャ間隔
  const double capture_interval(defaults.capture_fps > 0.0 ? 1.0 / defaults.capture_fps : 30.0);

  // 直前のフレームの受信時刻
  double last(glfwGetTime());

  // スレッドが実行可の間
  while (run[camL])
  {
    // 姿勢データを受信する
    const int ret(network.recvData(recvbuf, maxFrameSize));

    // サイズが 0 なら終了する
    if (ret == 0) return;

    // エラーがなければデータを読み込む
    if (ret > 0 && network.checkRemote())
    {
      // ヘッダのフォーマット
      unsigned int *const head(reinterpret_cast<unsigned int *>(recvbuf));

      // 変換行列の保存先
      GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + camCount + 1));

      // 変換行列を復帰する
      remoteMatrix->store(body, 0, head[camCount]);
    }

    // 現在時刻
    const double now(glfwGetTime());

    // 次のフレームの送信時刻までの残り時間
    const double delay(last + capture_interval - now);

#if DEBUG
    std::cerr << "recv delay = " << delay << '\n';
#endif
    // 残り時間があれば
    if (delay > 0.0)
    {
      // 次のフレームの送信時刻まで待つ
      std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(delay)));
    }

    // 直前のフレームの送信時刻を更新する
    last = now;
  }
}

// ローカルの映像と姿勢を送信する
void Camera::send()
{
  // キャプチャ間隔
  const double capture_interval(defaults.capture_fps > 0.0 ? 1.0 / defaults.capture_fps : 30.0);

  // 直前のフレームの送信時刻
  double last(glfwGetTime());

  // カメラスレッドが実行可の間
  while (run[camL])
  {
    // ヘッダのフォーマット
    unsigned int *const head(reinterpret_cast<unsigned int *>(sendbuf));

    // 左右フレームのサイズを 0 にしておく
    head[camL] = head[camR] = 0;

    // 変換行列の数を保存する
    head[camCount] = localMatrix->getUsed();

    // 変換行列の保存先
    GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + camCount + 1));

    // 変換行列を保存する
    localMatrix->load(body);

    // 左フレームの保存先 (変換行列の最後)
    uchar *data(reinterpret_cast<uchar *>(body + head[camCount]));

    // 左に新しいフレームが到着していれば
    if (!encoded[camL].empty())
    {
      // 左キャプチャデバイスをロックする
      captureMutex[camL].lock();

      // 左フレームのサイズを保存する
      head[camL] = static_cast<unsigned int>(encoded[camL].size());

      // 左フレームのデータをコピーする
      memcpy(data, encoded[camL].data(), head[camL]);

      // 左フレームのデータを空にする
      encoded[camL].clear();

      // 左フレームの転送が完了すればロックを解除する
      captureMutex[camL].unlock();

      // 右フレームの保存先 (左フレームの最後)
      data += head[camL];

      // 右キャプチャデバイスが動作していれば
      if (run[camR])
      {
        // 右キャプチャデバイスをロックする
        captureMutex[camR].lock();

        // 右フレームのサイズを保存する
        head[camR] = static_cast<unsigned int>(encoded[camR].size());

        // 右フレームのデータを左フレームのデータの後ろにコピーする
        memcpy(data, encoded[camR].data(), head[camR]);

        // 右フレームのデータを空にする
        encoded[camR].clear();

        // フレームの転送が完了すればロックを解除する
        captureMutex[camR].unlock();

        // 右フレームの最後
        data += head[camR];
      }

      // フレームを送信する
      network.sendData(sendbuf, static_cast<unsigned int>(data - sendbuf));
    }

    // 現在時刻
    const double now(glfwGetTime());

    // 次のフレームの送信時刻までの残り時間
    const double delay(last + capture_interval - now);

#if DEBUG
    std::cerr << "send delay = " << delay << '\n';
#endif
    // 残り時間があれば
    if (delay > 0.0)
    {
      // 次のフレームの送信時刻まで待つ
      std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(delay)));
    }

    // 直前のフレームの送信時刻を更新する
    last = now;
  }

  // ループを抜けるときに EOF を送信する
  network.sendEof();
}

// 作業者通信スレッド起動
int Camera::startWorker(unsigned short port, const char *address)
{
  // すでに確保されている作業用メモリを破棄する
  delete[] recvbuf, sendbuf;
  recvbuf = sendbuf = nullptr;

  // 作業者として初期化する
  const int ret(network.initialize(2, port, address));
  if (ret > 0) return ret;

  // 作業用のメモリを確保する（これは Camera のデストラクタで delete する）
  recvbuf = new uchar[maxFrameSize];
  sendbuf = new uchar[maxFrameSize];

  // 通信スレッドを開始する
  run[camL] = true;
  recvThread = std::thread([this]() { recv(); });
  sendThread = std::thread([this]() { send(); });

  return 0;
}

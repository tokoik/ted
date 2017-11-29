//
// カメラ関連の処理
//
#include "Camera.h"

#ifdef _WIN32
#  define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#  ifdef _DEBUG
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
  sendbuf = recvbuf = nullptr;

  // キャプチャされる画像のフォーマット
  format = GL_BGR;

  // 圧縮設定
  param.push_back(CV_IMWRITE_JPEG_QUALITY);
  param.push_back(defaults.remote_texture_quality);

  for (int cam = 0; cam < camCount; ++cam)
  {
    // リモートの姿勢に初期値を設定する
    attitude[cam].loadIdentity();

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
  delete[] sendbuf, recvbuf;
  sendbuf = recvbuf = nullptr;
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
    // 送信スレッドが終了するのを待つ
    if (sendThread.joinable()) sendThread.join();

    // 受信スレッドが終了するのを待つ
    if (recvThread.joinable()) recvThread.join();
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

// データ送信
void Camera::send()
{
  // カメラスレッドが実行可の間
  while (run[camL])
  {
    // キャプチャデバイスをロックする
    captureMutex[camL].lock();

    // ヘッダのフォーマット
    unsigned int *const head(reinterpret_cast<unsigned int *>(sendbuf));

    // 左フレームのサイズを保存する
    head[camL] = static_cast<unsigned int>(encoded[camL].size());

    // 右フレームのサイズを 0 にしておく
    head[camR] = 0;

    // 変換行列の数を保存する
    head[camCount] = localMatrix->getUsed();

    // 変換行列の保存先
    GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + camCount + 1));

    // 変換行列を保存する
    localMatrix->load(body);

    // 左フレームの保存先 (変換行列の最後)
    uchar *data(reinterpret_cast<uchar *>(body + head[camCount]));

    // 左フレームのデータをコピーする
    memcpy(data, encoded[camL].data(), head[camL]);

    // フレームの転送が完了すればロックを解除する
    captureMutex[camL].unlock();

    // 左フレームに画像があれば
    if (head[camL] > 0)
    {
      // 右フレームの保存先 (左フレームの最後)
      data += head[camL];

      if (run[camR])
      {
        // キャプチャデバイスをロックする
        captureMutex[camR].lock();

        // 右フレームのサイズを保存する
        head[camR] = static_cast<unsigned int>(encoded[camR].size());

        // 右フレームのデータを左フレームのデータの後ろにコピーする
        memcpy(data, encoded[camR].data(), head[camR]);

        // フレームの転送が完了すればロックを解除する
        captureMutex[camR].unlock();

        // 右フレームの最後
        data += head[camR];
      }
    }

    // フレームを送信する
    network.sendFrame(sendbuf, static_cast<unsigned int>(data - sendbuf));

    // 他のスレッドがリソースにアクセスするために少し待つ
    std::this_thread::sleep_for(std::chrono::milliseconds(10L));
  }
}

// データ受信
void Camera::recv()
{
  // スレッドが実行可の間
  while (run[camL])
  {
    // 姿勢データを受信する
    if (network.recvFrame(recvbuf, maxFrameSize) != SOCKET_ERROR)
    {
      // ヘッダのフォーマット
      unsigned int *const head(reinterpret_cast<unsigned int *>(recvbuf));

      // 変換行列の保存先
      GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + camCount + 1));

      // 変換行列を復帰する
      remoteMatrix->store(body, 0, head[camCount]);

      // カメラの姿勢を保存する
      queueRemoteAttitude(camL, body[camL]);
      queueRemoteAttitude(camR, body[camR]);
    }

    // 他のスレッドがリソースにアクセスするために少し待つ
    std::this_thread::sleep_for(std::chrono::milliseconds(10L));
  }
}

// リモートの Oculus Rift のトラッキング情報を遅延させる
void Camera::queueRemoteAttitude(int eye, const GgMatrix &new_attitude)
{
  // キューをロックする
  if (fifoMutex[eye].try_lock())
  {
    // 新しいトラッキングデータを追加する
    fifo[eye].push(new_attitude);

    // キューの長さが遅延させるフレーム数より長ければキューを進める
    if (fifo[eye].size() > defaults.tracking_delay[eye] + 1) fifo[eye].pop();

    // キューをアンロックする
    fifoMutex[eye].unlock();
  }
}

// リモートの Oculus Rift のヘッドラッキングによる移動を得る
const GgMatrix &Camera::getRemoteAttitude(int eye)
{
  // キューをロックできてキューが空でなかったら
  if (fifoMutex[eye].try_lock() && !fifo[eye].empty())
  {
    // キューの先頭を取り出す
    attitude[eye] = fifo[eye].front();

    // キューをアンロックする
    fifoMutex[eye].unlock();
  }

  return attitude[eye];
}

// 作業者通信スレッド起動
void Camera::startWorker(unsigned short port, const char *address)
{
  // 作業者として初期化して
  if (network.initialize(2, port, address)) return;

  // 作業用のメモリを確保する
  delete[] sendbuf, recvbuf;
  sendbuf = new uchar[maxFrameSize];
  recvbuf = new uchar[maxFrameSize];

  // 送信スレッドを開始する
  sendThread = std::thread([this]() { send(); });

  // 受信スレッドを開始する
  recvThread = std::thread([this]() { recv(); });
}

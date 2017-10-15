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
  frame = nullptr;

  // キャプチャされる画像のフォーマット
  format = GL_BGR;

  // 圧縮設定
  param.push_back(CV_IMWRITE_JPEG_QUALITY);
  param.push_back(defaults.remote_texture_quality);

  for (int cam = 0; cam < camCount; ++cam)
  {
    // ローカルの姿勢に初期値を設定する
    attitude[cam].position[0] = 0.0f;
    attitude[cam].position[1] = 0.0f;
    attitude[cam].position[2] = 0.0f;
    attitude[cam].position[3] = 0.0f;
    attitude[cam].orientation = ggIdentityQuaternion();

    // リモートの姿勢に初期値を設定する
    fifo[cam].emplace(attitude[cam]);

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
  delete frame;
  frame = nullptr;
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

    // 左フレームのサイズを保存する
    frame->length[camL] = static_cast<unsigned int>(encoded[camL].size());

    // 左フレームのデータをコピーする
    memcpy(frame->data, encoded[camL].data(), frame->length[camL]);

    // フレームの転送が完了すればロックを解除する
    captureMutex[camL].unlock();

    // 右フレームのサイズを 0 にしておく
    frame->length[camR] = 0;

    // 左フレームに画像があれば
    if (frame->length[camL] > 0)
    {
      if (run[camR])
      {
        // キャプチャデバイスをロックする
        captureMutex[camR].lock();

        // 右フレームのサイズを保存する
        frame->length[camR] = static_cast<unsigned int>(encoded[camR].size());

        // 右フレームのデータを左フレームのデータの後ろにコピーする
        memcpy(frame->data + frame->length[camL], encoded[camR].data(), frame->length[camR]);

        // フレームの転送が完了すればロックを解除する
        captureMutex[camR].unlock();
      }

      // フレームを送信する
      network.sendFrame(frame, static_cast<unsigned int>(sizeof(Frame)
        - workingMemorySize * sizeof(uchar) + frame->length[camL] + frame->length[camR]));

      // 他のスレッドがリソースにアクセスするために少し待つ
      std::this_thread::sleep_for(std::chrono::milliseconds(10L));
    }
  }
}

// データ受信
void Camera::recv()
{
  // スレッドが実行可の間
  while (run[camL])
  {
    Attitude attitude[camCount];

    // 姿勢データを受信する
    if (network.recvFrame(&attitude, sizeof attitude) != SOCKET_ERROR)
    {
      // 姿勢データを保存する
      queueRemoteAttitude(camL, attitude[camL]);
      queueRemoteAttitude(camR, attitude[camR]);
    }

    // 他のスレッドがリソースにアクセスするために少し待つ
    std::this_thread::sleep_for(std::chrono::milliseconds(10L));
  }
}

// 作業者通信スレッド起動
void Camera::startWorker(unsigned short port, const char *address)
{
  // 作業者として初期化して
  if (network.initialize(2, port, address)) return;

  // 作業用のメモリを確保する
  delete frame;
  frame = new Frame;

  // 送信スレッドを開始する
  sendThread = std::thread([this]() { this->send(); });

  // 受信スレッドを開始する
  recvThread = std::thread([this]() { this->recv(); });
}

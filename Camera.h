#pragma once

//
// カメラ関連の処理
//

// 各種設定
#include "Config.h"

// ネットワーク関連の処理
#include "Network.h"

// OpenCV
#include <opencv2/highgui/highgui.hpp>

// 標準ライブラリ
#include <thread>
#include <mutex>

//
// カメラ関連の処理を担当するクラス
//
class Camera
{
protected:

  // カメラスレッド
  std::thread captureThread[camCount];

  // ミューテックス
  std::mutex captureMutex[camCount];

  // 実行状態
  bool run[camCount];

  // スレッドを停止する
  void stop();

  // キャプチャした画像
  const GLubyte* buffer[camCount];

  // キャプチャした画像のサイズ
  GLsizei size[camCount][2];

  // 期待するキャプチャ間隔
  double capture_interval;

  // 実際のキャプチャ間隔
  double interval[camCount];

  // キャプチャする画像のフォーマット
  GLenum format;

  // 露出と利得
  int exposure, gain;

public:

  // コンストラクタ
  Camera();

  // コピーコンストラクタを封じる
  Camera(const Camera &c) = delete;

  // 代入を封じる
  Camera &operator=(const Camera &w) = delete;

  // デストラクタ
  virtual ~Camera();

  // 画像の幅を得る
  int getWidth(int cam) const
  {
    return size[cam][0];
  }

  // 画像の高さを得る
  int getHeight(int cam) const
  {
    return size[cam][1];
  }

  // フレームレートからキャプチャ間隔を設定する
  void setInterval(double fps)
  {
    capture_interval = fps > 0.0 ? 1.0 / fps : minDelay * 0.001;
  }

  // 圧縮設定
  void setQuality(int quality);

  // Ovrvision Pro の露出を上げる
  virtual void increaseExposure() {};

  // Ovrvision Pro の露出を下げる
  virtual void decreaseExposure() {};

  // Ovrvision Pro の利得を上げる
  virtual void increaseGain() {};

  // Ovrvision Pro の利得を下げる
  virtual void decreaseGain() {};

  // カメラをロックして画像をテクスチャに転送する
  virtual bool transmit(int cam, GLuint texture, const GLsizei* size);

  //
  // 通信関連
  //

private:

  // リモートの姿勢を受信する
  void recv();

  // ローカルの映像と姿勢を送信する
  void send();

protected:

  // 受信スレッド
  std::thread recvThread;

  // 送信スレッド
  std::thread sendThread;

  // エンコードのパラメータ
  std::vector<int> param;

  // エンコードした画像
  std::vector<uchar> encoded[camCount];

  // 映像受信用のメモリ
  uchar* recvbuf;

  // 映像送信用のメモリ
  uchar* sendbuf;

  // 通信データ
  Network network;

public:

  // 作業者通信スレッド起動
  int startWorker(unsigned short port, const char* address);

  // ネットワークを使っているかどうか
  bool useNetwork() const
  {
    return network.running();
  }

  // 作業者かどうか
  bool isWorker() const
  {
    return network.isWorker();
  }

  // 作業者かどうか
  bool isOperator() const
  {
    return network.isOperator();
  }
};

#pragma once

//
// カメラ関連の処理
//

// 各種設定
#include "config.h"

// ネットワーク関連の処理
#include "Network.h"

// 共有メモリ
#include "SharedMemory.h"

// OpenCV
#include <opencv2/highgui/highgui.hpp>

// キャプチャを非同期で行う
#include <thread>
#include <mutex>

// カメラの数と識別子
const int camCount(2), camL(0), camR(1);

// 作業用メモリのサイズ
const int maxFrameSize(1024 * 1024);

//
// カメラ関連の処理を担当するクラス
//
class Camera
{
  // コピーコンストラクタを封じる
  Camera(const Camera &c);

  // 代入を封じる
  Camera &operator=(const Camera &w);

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
  const GLubyte *buffer[camCount];

  // キャプチャした画像のサイズ
  GLsizei size[camCount][2];

  // キャプチャする画像のフォーマット
  GLenum format;

  // 露出と利得
  int exposure, gain;

public:

  // コンストラクタ
  Camera();

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

  // Ovrvision Pro の露出を上げる
  virtual void increaseExposure() {};

  // Ovrvision Pro の露出を下げる
  virtual void decreaseExposure() {};

  // Ovrvision Pro の利得を上げる
  virtual void increaseGain() {};

  // Ovrvision Pro の利得を下げる
  virtual void decreaseGain() {};

  // カメラをロックして画像をテクスチャに転送する
  virtual bool transmit(int cam, GLuint texture, const GLsizei *size);

  //
  // 通信関連
  //

private:

  // データ送信
  void send();

  // データ受信
  void recv();

protected:

  // 送信スレッド
  std::thread sendThread;

  // 受信スレッド
  std::thread recvThread;

  // エンコードのパラメータ
  std::vector<int> param;

  // エンコードした画像
  std::vector<uchar> encoded[camCount];

  // データ送信用のメモリ
  uchar *sendbuf;
  
  // データ受信用のメモリ
  uchar *recvbuf;

  // 通信データ
  Network network;

  // 変換行列のテーブル
  SharedMemory *localMatrix, *remoteMatrix;

public:

  // モデル変換行列のテーブルを選択する
  void selectTable(SharedMemory *local, SharedMemory *remote)
  {
    localMatrix = local;
    remoteMatrix = remote;
  }

  // 作業者通信スレッド起動
  void startWorker(unsigned short port, const char *address);

  // ネットワークを使っているかどうか
  bool useNetwork() const
  {
    return network.running();
  }

  // 作業者かどうか
  bool isWorker() const
  {
    return useNetwork() && network.isSender();
  }

  // 操縦者かかどうか
  bool isOperator() const
  {
    return useNetwork() && !network.isSender();
  }
};

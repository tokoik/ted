#pragma once

//
// カメラ関連の処理
//

// 各種設定
#include "config.h"

// ネットワーク関連の処理
#include "Network.h"

// OpenCV
#include <opencv2/highgui/highgui.hpp>

// キャプチャを非同期で行う
#include <thread>
#include <mutex>

// キュー
#include <queue>

// カメラの数と識別子
const int camCount(2), camL(0), camR(1);

// 作業用メモリのサイズ
const int workingMemorySize(1024 * 1024);

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

  // カメラの姿勢
  struct Attitude
  {
    // コンストラクタ
    Attitude()
    {
    }

    // コンストラクタ
    Attitude(GLfloat p0, GLfloat p1, GLfloat p2, GLfloat p3, const GgQuaternion &q)
    {
      position[0] = p0;
      position[1] = p1;
      position[2] = p2;
      position[3] = p3;
      orientation = q;
    }

    // コンストラクタ
    Attitude(const GLfloat *p, const GgQuaternion &q)
      : Attitude(p[0], p[1], p[2], p[3], q) {}

    // 視点の位置
    GLfloat position[4];

    // 視線の向き
    GgQuaternion orientation;
  };

  // ローカルカメラの姿勢
  Attitude attitude[camCount];

  // ローカルカメラのトラッキング情報を保存する際に使うミューテックス
  std::mutex attitudeMutex[camCount];

  // リモートカメラの姿勢のタイミングをフレームに合わせて遅らせるためのキュー
  std::queue<Attitude> fifo[camCount];

  // エンコードのパラメータ
  std::vector<int> param;

  // エンコードした画像
  std::vector<uchar> encoded[camCount];

  // フレームのレイアウト
  struct Frame
  {
    // そのフレーム撮影時のカメラの姿勢
    Attitude attitude[camCount];

    // フレームデータの長さ
    unsigned int length[camCount];

    // 作業用のメモリ
    uchar data[workingMemorySize];
  };

  // 作業用のメモリ
  Frame *frame;

  // 通信データ
  Network network;

public:

  // ローカルの Oculus Rift のトラッキング情報を保存する
  void storeLocalAttitude(int eye, const Attitude &new_attitude)
  {
    if (attitudeMutex[eye].try_lock())
    {
      attitude[eye] = new_attitude;
      attitudeMutex[eye].unlock();
    }
  }

  // ローカルの Oculus Rift のトラッキング情報を保存する
  void storeLocalAttitude(int eye, const GLfloat *position, const GgQuaternion &orientation)
  {
    storeLocalAttitude(eye, Attitude(position, orientation));
  }

  // ローカルの Oculus Rift のヘッドラッキングによる移動を得る
  const Attitude &getLocalAttitude(int eye)
  {
    std::lock_guard<std::mutex> lock(attitudeMutex[eye]);
    return attitude[eye];
  }

  // リモートの Oculus Rift のトラッキング情報を遅延させる
  void queueRemoteAttitude(int eye, const Attitude &new_attitude)
  {
    // 新しいトラッキングデータを追加する
    fifo[eye].push(new_attitude);

    // キューの長さが遅延させるフレーム数より長ければキューを進める
    if (fifo[eye].size() > defaults.tracking_delay[eye] + 1) fifo[eye].pop();
  }

  // リモートの Oculus Rift のトラッキング情報を遅延させる
  void queueRemoteAttitude(int eye, const GLfloat *position, const GgQuaternion &orientation)
  {
    queueRemoteAttitude(eye, Attitude(position, orientation));
  }

  // リモートの Oculus Rift のヘッドラッキングによる移動を得る
  const Attitude &getRemoteAttitude(int eye)
  {
    static Attitude initialAttitude(0.0f, 0.0f, 0.0f, 1.0f, ggIdentityQuaternion());
    if (fifo[eye].empty()) return initialAttitude;
    return fifo[eye].front();
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

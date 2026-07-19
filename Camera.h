#pragma once

//
// カメラ関連の処理
//

// 各種設定
#include "Config.h"

// ネットワーク関連の処理
#include "Network.h"

// OpenCV
#include <opencv2/opencv.hpp>

// 標準ライブラリ
#include <thread>
#include <mutex>
#include <atomic>

// 通信フレームは「左右JPEGサイズ、行列数、行列列、左右JPEG」の順に格納する。
// headLength は先頭に置く unsigned int の個数で、camCount 番目が行列数になる。
constexpr int headLength{ camCount + 1 };

// UDPで送受信する1フレームの上限。符号化後の画像が収まらない場合は画像を省略し、
// 固定長バッファを越えて書き込まない。
constexpr int maxFrameSize{ 1024 * 1024 };

//
// 入力方式に共通する画像保持、OpenGLへの転送、姿勢・画像のネットワーク同期を担当する。
// 派生クラスは image[] を更新し、captured/unsent で描画側・送信側へ更新を通知する。
//
class Camera
{
protected:

  // 信頼できない受信値でポインタを作る前に各領域が length 内へ収まるか検証し、
  // 成功時だけ各領域の読み取り専用ポインタを返す。
  static bool unpackFrame(const uchar* buffer, int length, const unsigned int*& head,
    const GgMatrix*& body, const uchar*& imageData);

  // カメラスレッド
  std::thread captureThread[camCount];

  // ミューテックス
  std::mutex captureMutex[camCount];

  // キャプチャ・通信スレッド間で停止要求を共有する実行状態
  std::atomic<bool> run[camCount]{ false, false };

  // スレッドを停止する
  void stop();

  // キャプチャする画像のフォーマット
  GLenum format{ GL_BGR };

  // キャプチャデバイスから取得した画像
  cv::Mat image[camCount];

  // 各入力のキャプチャ間隔（秒）
  double interval[camCount]{ 1.0 / 30.0, 1.0 / 30.0 };

  // 全カメラに適用するキャプチャ間隔（秒）
  double capture_interval{ 0.0 };

  // キャプチャ完了なら true
  std::atomic<bool> captured[camCount]{ false, false };

  // レイテンシ優先なら true、全フレームキャプチャなら false
  std::atomic<bool> prioritizeLatency[camCount]{ true, true };

  // 未送信なら true
  std::atomic<bool> unsent[camCount]{ false, false };

  // ネットワークへ送るフレーム間隔（秒）。sleep_for の直前にミリ秒へ変換する。
  double send_interval{ minDelay * 0.001 };

  // 露出と利得
  int exposure{ 0 }, gain{ 0 };

public:

  // コンストラクタ
  Camera(int quality = 95);

  // コピーコンストラクタを封じる
  Camera(const Camera& c) = delete;

  // 代入を封じる
  Camera& operator=(const Camera& w) = delete;

  // デストラクタ
  virtual ~Camera();

  // 画像の幅を得る
  int getWidth(int cam) const
  {
    return image[cam].cols;
  }

  // 画像の高さを得る
  int getHeight(int cam) const
  {
    return image[cam].rows;
  }

  // フレームレートからキャプチャ間隔を設定する
  void setInterval(double fps)
  {
    capture_interval = fps > 0.0 ? 1.0 / fps : minDelay * 0.001;
  }

  // レイテンシ優先モードを設定する
  void setPrioritizeLatency(int cam, bool mode)
  {
    prioritizeLatency[cam] = mode;
  }

  // レイテンシ優先モードを取得する
  bool getPrioritizeLatency(int cam) const
  {
    return prioritizeLatency[cam];
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

  // 映像受信用のメモリ
  uchar* recvbuf{ nullptr };

  // 映像送信用のメモリ
  uchar* sendbuf{ nullptr };

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

  // 指示者かかどうか
  bool isInstructor() const
  {
    return network.isInstructor();
  }
};

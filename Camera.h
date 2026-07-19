#pragma once

///
/// カメラ関連の基底クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 197, 2026
///

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

/// 通信フレームのヘッダの長さ
///
/// @details
/// 通信フレームは「左フレームサイズ、右フレームサイズ、変換行列数」の順に格納する。
///
constexpr int headLength{ camCount + 1 };

/// UDP で送受信する1フレームの上限
///
/// @details
/// 符号化後の画像が収まらない場合は画像を省略して固定長バッファを越えて書き込まない。
///
constexpr int maxFrameSize{ 1024 * 1024 };

///
/// カメラの基底クラス
///
/// @details
/// 入力方式に共通する画像保持、OpenGLへの転送、姿勢・画像のネットワーク同期を担当する。
/// 派生クラスは image[] を更新し、captured/unsent で描画側・送信側へ更新を通知する。
///
class Camera
{
protected:

  ///
  /// 受信したフレームの解析
  ///
  /// @param buffer 受信したフレームの先頭アドレス
  /// @param length 受信したフレームの長さ
  /// @param head 受信したフレームのヘッダの先頭アドレス
  /// @param body 受信したフレームの変換行列の先頭アドレス
  /// @param imageData 受信したフレームの画像データの先頭アドレス
  /// @return 成功した場合は true
  ///
  /// @details
  /// 信頼できない受信値でポインタを作る前に各領域が length 内へ収まるか検証し、
  /// 成功時だけ各領域の読み取り専用ポインタを返す。
  ///
  static bool unpackFrame(const uchar* buffer, int length, const unsigned int*& head,
    const GgMatrix*& body, const uchar*& imageData);

  /// キャプチャスレッド
  std::thread captureThread[camCount];

  /// キャプチャスレッドのミューテックス
  std::mutex captureMutex[camCount];

  /// キャプチャ・通信スレッド間で停止要求を共有する実行状態
  std::atomic<bool> run[camCount]{ false, false };

  /// キャプチャスレッドを停止する
  void stop();

  /// キャプチャする画像のフォーマット
  GLenum format{ GL_BGR };

  /// キャプチャデバイスから取得した画像
  cv::Mat image[camCount];

  /// 各入力のキャプチャ間隔（秒）
  double interval[camCount]{ 1.0 / 30.0, 1.0 / 30.0 };

  /// 全カメラに適用するキャプチャ間隔（秒）
  double capture_interval{ 0.0 };

  /// キャプチャ完了なら true
  std::atomic<bool> captured[camCount]{ false, false };

  /// レイテンシ優先なら true、全フレームキャプチャなら false
  std::atomic<bool> prioritizeLatency[camCount]{ true, true };

  /// 未送信なら true
  std::atomic<bool> unsent[camCount]{ false, false };

  /// ネットワークへ送るフレーム間隔（秒）。sleep_for の直前にミリ秒へ変換する。
  double send_interval{ minDelay * 0.001 };

  /// 露出
  int exposure{ 0 };

  /// 利得  
  int gain{ 0 };

public:

  ///
  /// コンストラクタ
  ///
  /// @param quality JPEG 圧縮率 (0-100)
  ///
  Camera(int quality = 95);

  ///
  /// コピーコンストラクタを封じる
  ///
  /// @param c コピー元の Camera オブジェクト
  ///
  Camera(const Camera& c) = delete;

  ///
  /// 代入を封じる
  ///
  /// @param w コピー元の Camera オブジェクト
  /// @return コピー元の Camera オブジェクトへの参照
  ///
  Camera& operator=(const Camera& w) = delete;

  ///
  /// デストラクタ
  ///
  virtual ~Camera();

  ///
  /// 画像の幅を得る
  ///
  /// @param cam カメラ番号
  /// @return 画像の幅
  ///
  int getWidth(int cam) const
  {
    return image[cam].cols;
  }

  ///
  /// 画像の高さを得る
  ///
  /// @param cam カメラ番号
  /// @return 画像の高さ
  ///
  int getHeight(int cam) const
  {
    return image[cam].rows;
  }

  ///
  /// フレームレートからキャプチャ間隔を設定する
  ///
  /// @param fps フレームレート
  ///
  void setInterval(double fps)
  {
    capture_interval = fps > 0.0 ? 1.0 / fps : minDelay * 0.001;
  }

  ///
  /// レイテンシ優先モードを設定する
  ///
  /// @param cam カメラ番号
  /// @param mode モード
  ///
  void setPrioritizeLatency(int cam, bool mode)
  {
    prioritizeLatency[cam] = mode;
  }

  ///
  /// レイテンシ優先モードを取得する
  ///
  /// @param cam カメラ番号
  /// @return モード
  ///
  bool getPrioritizeLatency(int cam) const
  {
    return prioritizeLatency[cam];
  }

  ///
  /// 圧縮設定
  ///
  /// @param quality JPEG 圧縮率 (0-100)
  ///
  void setQuality(int quality);

  ///
  /// カメラの露出を上げる
  ///
  virtual void increaseExposure() {};

  ///
  /// カメラの露出を下げる
  ///
  virtual void decreaseExposure() {};

  ///
  /// カメラの利得を上げる
  ///
  virtual void increaseGain() {};

  ///
  /// カメラの利得を下げる
  ///
  virtual void decreaseGain() {};

  ///
  /// カメラをロックして画像をテクスチャに転送する
  ///
  /// @param cam カメラ番号
  /// @param texture 転送先のテクスチャ
  /// @param size 転送する画像の大きさ
  /// @return 成功した場合は true
  ///
  virtual bool transmit(int cam, GLuint texture, const GLsizei* size);

  //
  // 通信関連
  //

private:

  ///
  /// リモートの姿勢を受信する
  ///
  void recv();

  ///
  /// ローカルの映像と姿勢を送信する
  ///
  void send();

protected:

  /// 受信スレッド
  std::thread recvThread;

  /// 送信スレッド
  std::thread sendThread;

  /// エンコードのパラメータ
  std::vector<int> param;

  /// 映像受信用のメモリ
  uchar* recvbuf{ nullptr };

  /// 映像送信用のメモリ
  uchar* sendbuf{ nullptr };

  /// 通信データ
  Network network;

public:

  ///
  /// 作業者通信スレッド起動
  ///
  /// @param port ポート番号
  /// @param address 接続先のアドレス
  /// @return 成功した場合は 0、失敗した場合はエラーコード
  ///
  int startWorker(unsigned short port, const char* address);

  ///
  /// ネットワークを使っているかどうか
  ///
  /// @return ネットワークを使っている場合は true
  ///
  bool useNetwork() const
  {
    return network.running();
  }

  ///
  /// 作業者かどうか
  ///
  /// @return 作業者の場合は true
  ///
  bool isWorker() const
  {
    return network.isWorker();
  }

  ///
  /// 指導者かどうか
  ///
  /// @return 指導者の場合は true
  ///
  bool isInstructor() const
  {
    return network.isInstructor();
  }
};

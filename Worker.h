#pragma once

///
/// 作業者 (WORKER) として映像と姿勢を送信するクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
/// @details
/// 旧 Camera::startWorker() / send() / recv() (ce7b96e 以前の main.cpp から起動) を、
/// 現在の Camera 設計 (NVI・単一バッファ) に合わせて独立したクラスとして復元したもの。
/// 入力中のカメラの最新フレームを JPEG に符号化し、ローカルの姿勢 (共有メモリ localAttitude) と
/// ともに指示者 (OPERATOR) または中継サーバへ送る。指示者から受け取った姿勢は
/// 共有メモリ remoteAttitude に格納する。
///

// カメラ
#include "Camera.h"

// ネットワーク
#include "Network.h"

// OpenCV
#include "opencv_link.h"

// 標準ライブラリ
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

class Worker
{
  /// 送信する映像を取得するカメラ (送信中に入力が切り替わっても破棄されないよう共有所有する)
  std::shared_ptr<Camera> camera;

  /// UDP 通信
  Network network;

  /// 送信スレッドと受信スレッド
  std::thread sendThread, recvThread;

  /// スレッドの実行中なら true
  std::atomic<bool> running{ false };

  /// 送信する画像の数 (1: 単眼, 2: ステレオ)
  int eyeCount{ 1 };

  /// 論理的な左右眼を物理入力と入れ替えるなら true
  bool swapEyes{ false };

  /// 伝送解像度 (0 x 0 なら取得画像のまま)
  cv::Size transmitSize;

  /// 画像を送信する最小間隔
  std::chrono::steady_clock::duration imageInterval{};

  /// JPEG の圧縮設定
  std::vector<int> param;

  /// 送受信バッファ
  std::vector<unsigned char> sendbuf, recvbuf;

  /// 符号化の作業領域 (フレームごとに確保し直さないよう再利用する)
  std::vector<std::uint8_t> raw;
  cv::Mat converted, resized;
  std::vector<uchar> encoded;

  /// 送信スレッド
  void sendLoop();

  /// 受信スレッド
  void recvLoop();

  ///
  /// カメラの最新フレームを JPEG に符号化して送信バッファへ追記する
  ///
  /// @param source 物理的なカメラ番号
  /// @param data 追記先 (追記した分だけ進める)
  /// @return 追記したバイト数 (収まらなかったり取得できなかったりしたら 0)
  ///
  unsigned int appendImage(int source, unsigned char*& data);

public:

  Worker() = default;
  ~Worker();

  Worker(const Worker&) = delete;
  Worker& operator=(const Worker&) = delete;

  ///
  /// 作業者として送受信を開始する
  ///
  /// @param camera 送信する映像を取得するカメラ
  /// @param stereo 左右に独立した入力があれば true
  /// @param config 役割・通信先・伝送設定
  /// @return 成功したら 0, 失敗したらエラーコード
  ///
  int start(const std::shared_ptr<Camera>& camera, bool stereo, const Config& config);

  ///
  /// 送受信を停止する
  ///
  void stop();

  ///
  /// 送受信中かどうか
  ///
  bool isRunning() const
  {
    return running;
  }
};

#pragma once

///
/// リモートのカメラからキャプチャするクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///

// カメラ関連の処理
#include "Camera.h"

// ネットワーク
#include "Network.h"

// OpenCV
#include <opencv2/opencv.hpp>

// OpenGL
#include "gg.h"

// 設定
#include "Config.h"

#include <vector>
#include <atomic>
#include <thread>

///
/// 別のTEDから UDP で受け取ったフレームをカメラ入力として扱うクラス
///
/// @details
/// 受信映像を一度 FBO へ再投影し、遠隔カメラの画角を背景メッシュへ合わせる。
///
class CamRemote
  : public Camera
{
  /// ネットワーク処理
  Network network;

  /// 送受信バッファ
  unsigned char* sendbuf{ nullptr };
  unsigned char* recvbuf{ nullptr };

  /// 受信・送信スレッド
  std::thread sendThread;
  std::thread recvThread;

  /// 背景画像の変形に使うフレームバッファオブジェクト
  GLuint fb{ 0 };

  /// 背景画像の大きさ
  cv::Size size[camCount];

  /// 背景画像の変形に使うメッシュの幅
  GLsizei slices{ 0 };

  /// 背景画像の変形に使うメッシュの高さ
  GLsizei stacks{ 0 };

  /// 背景画像の変形に使うメッシュの格子間隔
  GLfloat gap[2]{ 0.0f, 0.0f };

  /// 背景画像を取得するリモートのカメラのスクリーンの大きさ
  GLfloat screen[2]{ 0.0f, 0.0f };

  /// リモートから取得したフレーム
  cv::Mat remote[camCount];

  /// 右眼用のフレームバッファ (CPU参照用)
  std::vector<std::uint8_t> imageR;

  /// 右眼用の解像度
  int widthR{ 0 };
  int heightR{ 0 };

  /// 右眼用のキャプチャ完了フラグ
  std::atomic<bool> capturedR{ false };

  /// 右眼の画像を一度でも受け取ったら true (受け取るまでは左の画像を右にも使う)
  std::atomic<bool> stereoSource{ false };

  /// リモートから取得したフレームのサンプリングに使うテクスチャ
  GLuint resample[camCount]{ 0 };

  /// 再サンプリング用テクスチャへ確保した受信画像の大きさ
  cv::Size resampleSize[camCount];

  /// 背景画像のタイリングに使うシェーダ
  GLuint shader{ 0 };

  /// 背景画像のタイリングに使うシェーダの uniform 変数の場所
  GLint gapLoc{ -1 }, screenLoc{ -1 }, rotationLoc{ -1 }, imageLoc{ -1 };

  ///
  /// 検証済みフレームから姿勢とフレームを取り出し描画スレッド用の画像に反映する
  ///
  void recv();

  ///
  /// 相手側が視点を同期できるよう、画像とは逆方向にローカル姿勢だけを定期送信する
  ///
  void send();

protected:

  ///
  /// キャプチャ開始処理を行う
  ///
  virtual bool onStart() override;

  ///
  /// キャプチャ停止処理を行う
  ///
  virtual void onStop() override;

  ///
  /// キャプチャデバイスを閉じる処理を行う
  ///
  virtual void onClose() override;

public:

  ///
  /// コンストラクタ
  ///
  CamRemote();

  ///
  /// デストラクタ
  ///
  virtual ~CamRemote();

  ///
  /// 平面展開後の画像の幅を得る
  ///
  virtual int getWidth(int cam = 0) const override { return (cam == 0) ? size[camL].width : size[camR].width; }

  ///
  /// 平面展開後の画像の高さを得る
  ///
  virtual int getHeight(int cam = 0) const override { return (cam == 0) ? size[camL].height : size[camR].height; }

  ///
  /// チャンネル数を得る
  ///
  virtual int getChannels(int cam = 0) const override { return 3; }

  ///
  /// カメラから入力する
  ///
  /// @param port ポート番号
  /// @param address 接続先のアドレス
  ///
  int open(unsigned short port, const char* address);

  ///
  /// カメラをロックして画像をテクスチャに転送する
  ///
  /// @param eye 視点番号
  /// @param texture 転送先のテクスチャ
  /// @param size 転送する画像のサイズ
  ///
  virtual bool transmit(int eye, unsigned int texture, const int* size) override;

  ///
  /// 指定した視点のフレームデータをロックして処理関数を実行する
  ///
  template <typename F>
  bool lockFrame(int eye, F&& func)
  {
    std::unique_lock<std::mutex> lock{ mtx, std::try_to_lock };
    if (!lock.owns_lock()) return false;

    if (eye == 0 && captured && !image.empty())
    {
      const auto length{ static_cast<size_t>(width) * height * channels };
      func(image.data(), std::min(image.size(), length), width, height, channels);
      return true;
    }
    else if (eye == 1 && capturedR && !imageR.empty())
    {
      const auto length{ static_cast<size_t>(widthR) * heightR * channels };
      func(imageR.data(), std::min(imageR.size(), length), widthR, heightR, channels);
      return true;
    }
    return false;
  }

  ///
  /// 単一視点用のフレームデータロック
  ///
  template <typename F>
  bool lockFrame(F&& func)
  {
    return lockFrame(0, std::forward<F>(func));
  }
};
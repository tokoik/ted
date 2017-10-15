#pragma once

//
// OpenCV を使ったキャプチャ
//

// カメラ関連の処理
#include "Camera.h"

// OpenCV を使ってキャプチャするクラス
class CamCv
  : public Camera
{
  // OpenCV のキャプチャデバイス
  cv::VideoCapture camera[camCount];

  // OpenCV のキャプチャデバイスから取得した画像
  cv::Mat image[camCount];

  // 現在のフレームの時刻
  double frameTime[camCount];

  // キャプチャデバイスを開始する
  bool start(int cam);

  // フレームをキャプチャする
  void capture(int cam);

public:

  // コンストラクタ
  CamCv();

  // デストラクタ
  virtual ~CamCv();

  // カメラから入力する
  bool open(int device, int cam)
  {
    // カメラを開いてキャプチャを開始する
    return camera[cam].open(device) && start(cam);
  }

  // ファイル／ネットワークからキャプチャを開始する
  bool open(const std::string &file, int cam)
  {
    // ファイル／ネットワークを開く
    return camera[cam].open(file) && start(cam);
  }

  // カメラが使用可能か判定する
  bool opened(int cam)
  {
    return camera[cam].isOpened();
  }

  // 露出を上げる
  virtual void increaseExposure();

  // 露出を下げる
  virtual void decreaseExposure();

  // 利得を上げる
  virtual void increaseGain();

  // 利得を下げる
  virtual void decreaseGain();
};

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

  // キャプチャデバイスを準備する
  void setup(int cam, const std::array<char, 4>& codec, const std::array<int, 2>& size, double fps);

  // キャプチャデバイスを開始する
  bool start(int cam);

  // 最初のフレームの時刻
  double startTime[camCount];

  // フレームをキャプチャする
  void capture(int cam);

public:

  // コンストラクタ
  CamCv();

  // デストラクタ
  virtual ~CamCv();

  // カメラから入力する
  bool open(int device, int cam);

  // ファイル／ネットワークからキャプチャを開始する
  bool open(const std::string& file, int cam);

  // カメラが使用可能か判定する
  bool opened(int cam);

  // 露出を上げる
  virtual void increaseExposure();

  // 露出を下げる
  virtual void decreaseExposure();

  // 利得を上げる
  virtual void increaseGain();

  // 利得を下げる
  virtual void decreaseGain();
};

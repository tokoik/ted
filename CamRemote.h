#pragma once

//
// リモートカメラからキャプチャ
//

// カメラ関連の処理
#include "Camera.h"

// リモートカメラからキャプチャするときのダミーカメラ
class CamRemote
  : public Camera
{
  // ドーム画像に変形するか否か
  const bool reshape;

  // 背景画像の変形に使うフレームバッファオブジェクト
  GLuint fb{ 0 };

  // 背景画像の大きさ
  cv::Size size[camCount];

  // 背景画像の変形に使うメッシュの解像度
  GLsizei slices{ 0 }, stacks{ 0 };

  // 背景画像の変形に使うメッシュの格子間隔
  GLfloat gap[2]{ 0.0f, 0.0f };

  // 背景画像を取得するリモートのカメラのスクリーンの大きさ
  GLfloat screen[2]{ 0.0f, 0.0f };

  // リモートから取得したフレーム
  cv::Mat remote[camCount];

  // リモートから取得したフレームのサンプリングに使うテクスチャ
  GLuint resample[camCount]{ 0 };

  // 背景画像のタイリングに使うシェーダ
  GLuint shader{ 0 };

  // 背景画像のタイリングに使うシェーダの uniform 変数の場所
  GLint gapLoc{ -1 }, screenLoc{ -1 }, rotationLoc{ -1 }, imageLoc{ -1 };

  // リモートの映像と姿勢を受信する
  void recv();

  // ローカルの姿勢を送信する
  void send();

public:

  // コンストラクタ
  CamRemote(bool reshape);

  // デストラクタ
  virtual ~CamRemote();

  // カメラから入力する
  int open(unsigned short port, const char* address);

  // カメラをロックして画像をテクスチャに転送する
  virtual bool transmit(int cam, GLuint texture, const GLsizei* size);
};

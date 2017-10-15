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
  GLuint fb;

  // 背景画像の変形に使うメッシュの解像度
  GLsizei slices, stacks;;

  // 背景画像の変形に使うメッシュの格子間隔
  GLfloat gap[2];

  // 背景画像を取得するリモートのカメラのスクリーンの大きさ
  GLfloat screen[2];

  // リモートから取得したフレーム
  cv::Mat remote[camCount];

  // リモートから取得したフレームのサンプリングに使うテクスチャ
  GLuint resample[camCount];

  // 背景画像のタイリングに使うシェーダ
  GLuint shader;

  // 背景画像のタイリングに使うシェーダの uniform 変数の場所
  GLint gapLoc, screenLoc, rotationLoc, imageLoc;

  // リモートのフレームと姿勢を受信する
  void recv();

  // ローカルの姿勢を送信する
  void send();

public:

  // コンストラクタ
  CamRemote(bool reshape);

  // デストラクタ
  virtual ~CamRemote();

  // カメラから入力する
  int open(unsigned short port, const char *address, const GLuint *image);

  // カメラをロックして画像をテクスチャに転送する
  virtual bool transmit(int cam, GLuint texture, const GLsizei *size);
};

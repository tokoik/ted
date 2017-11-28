#pragma once

//
// 静止画
//

// カメラ関連の処理
#include "Camera.h"

// 静止画を使うクラス
class CamImage
  : public Camera
{
  // ファイルから読み込んだ画像
  cv::Mat image[camCount];

public:

  // コンストラクタ
  CamImage();

  // デストラクタ
  virtual ~CamImage();

  // ファイルから入力する
  bool open(const std::string &file, int cam);

  // カメラが使用可能か判定する
  bool opened(int cam);

  // このカメラでは画像の転送を行わない
  bool CamImage::transmit(int cam, GLuint texture, const GLsizei *size);

  // 読み込んだ画像のデータを得る
  const GLubyte *getImage(int cam);
};

//
// 静止画
//
#include "CamImage.h"

// コンストラクタ
CamImage::CamImage()
{
}

// デストラクタ
CamImage::~CamImage()
{
  // 送信スレッドを止める
  stop();
}

// ファイルから入力する
bool CamImage::open(const std::string &file, int cam)
{
  // 画像をファイルから読み込む
  image[cam] = cv::imread(defaults.camera_left_image);

  // 読み込みに失敗したら戻る
  if (image[cam].empty()) return false;

  // 読み込んだ画像のサイズを求める
  size[cam][0] = image[cam].cols;
  size[cam][1] = image[cam].rows;

  return true;
}

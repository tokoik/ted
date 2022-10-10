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
bool CamImage::open(const std::string& file, int cam)
{
  // 画像をファイルから読み込む
  image[cam] = cv::imread(file);

  // 読み込みに成功したら true
  return !image[cam].empty();
}

// カメラが使用可能か判定する
bool CamImage::opened(int cam)
{
  return !image[cam].empty();
}

// このカメラでは画像の転送を行わない
bool CamImage::transmit(int cam, GLuint texture, const GLsizei* size)
{
  return true;
}

// 読み込んだ画像のデータを得る
const GLubyte* CamImage::getImage(int cam)
{
  return image[cam].data;
}

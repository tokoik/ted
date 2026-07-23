///
/// 静止画を使うクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///
#include "CamImage.h"

//
// コンストラクタ
//
CamImage::CamImage()
{
}

//
// デストラクタ
//
CamImage::~CamImage()
{
  // 送信スレッドを止める
  stop();
}

//
// ファイルから入力する
//
bool CamImage::open(const std::string& file, int cam)
{
  // 画像をファイルから読み込む
  cv::Mat source{ cv::imread(file) };
  if (source.empty()) return false;

  // 1枚に左右画像が格納されている場合は、左入力を読み込んだ時点で左右へ分割する
  if (cam == camL && isPackedCameraLayout(defaults.camera_layout))
  {
    if (defaults.camera_layout == CAMERA_LAYOUT_SIDE_BY_SIDE)
    {
      const int width{ source.cols / 2 };
      if (width <= 0) return false;
      image[camL] = source(cv::Rect(0, 0, width, source.rows)).clone();
      image[camR] = source(cv::Rect(width, 0, width, source.rows)).clone();
    }
    else
    {
      const int height{ source.rows / 2 };
      if (height <= 0) return false;
      image[camL] = source(cv::Rect(0, 0, source.cols, height)).clone();
      image[camR] = source(cv::Rect(0, height, source.cols, height)).clone();
    }
  }
  else
  {
    image[cam] = std::move(source);
  }

  // 読み込みに成功したら true
  return !image[cam].empty();
}

//
// カメラが使用可能か判定する
//
bool CamImage::opened(int cam)
{
  return !image[cam].empty();
}

//
// このカメラでは画像の転送を行わない
//
bool CamImage::transmit(int cam, GLuint texture, const GLsizei* size)
{
  return true;
}

//
// 読み込んだ画像のデータを得る
//
const GLubyte* CamImage::getImage(int cam)
{
  return image[cam].data;
}

///
/// 静止画像を使うクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///
#include "CamImage.h"
#include "Config.h"
#include "gg.h"
#include <fstream>

//
// 画像ファイルを読み込むヘルパー関数
//
static bool loadFile(const std::string& filename, cv::Mat& frame)
{
  std::ifstream file(Utf8ToTChar(filename),
    std::ifstream::in | std::ifstream::binary | std::ifstream::ate);
  if (file.is_open())
  {
    std::vector<char> buffer(static_cast<std::size_t>(file.tellg()));
    file.seekg(0, std::ifstream::beg);
    file.read(buffer.data(), buffer.size());
    file.close();
    if (file.good())
    {
      frame = cv::imdecode(buffer, cv::IMREAD_COLOR);
      return !frame.empty();
    }
  }
  return false;
}

//
// ファイルから入力する
//
bool CamImage::open(const std::string& file, int cam)
{
  cv::Mat cvFrame;
  if (!loadFile(file, cvFrame)) return false;

  std::lock_guard<std::mutex> lock{ mtx };

  // SBS / TAB 入力の場合は左右へ分割する
  if (cam == camL && isPackedCameraLayout(defaults.camera_layout))
  {
    cv::Mat left, right;
    if (defaults.camera_layout == CAMERA_LAYOUT_SIDE_BY_SIDE)
    {
      const int w{ cvFrame.cols / 2 };
      left = cvFrame(cv::Rect(0, 0, w, cvFrame.rows)).clone();
      right = cvFrame(cv::Rect(w, 0, w, cvFrame.rows)).clone();
    }
    else if (defaults.camera_layout == CAMERA_LAYOUT_TOP_AND_BOTTOM)
    {
      const int h{ cvFrame.rows / 2 };
      left = cvFrame(cv::Rect(0, 0, cvFrame.cols, h)).clone();
      right = cvFrame(cv::Rect(0, h, cvFrame.cols, h)).clone();
    }

    width = left.cols;
    height = left.rows;
    channels = left.channels();
    const std::size_t sizeL{ static_cast<std::size_t>(width) * height * channels };
    image.resize(sizeL);
    std::memcpy(image.data(), left.data, sizeL);

    widthR = right.cols;
    heightR = right.rows;
    const std::size_t sizeR{ static_cast<std::size_t>(widthR) * heightR * channels };
    imageR.resize(sizeR);
    std::memcpy(imageR.data(), right.data, sizeR);

    captured = true;
    capturedR = true;
    notifyFrame(camL);
    notifyFrame(camR);
    return true;
  }

  // 左右個別入力または単眼入力
  if (cam == camL)
  {
    width = cvFrame.cols;
    height = cvFrame.rows;
    channels = cvFrame.channels();
    const std::size_t size{ static_cast<std::size_t>(width) * height * channels };
    image.resize(size);
    std::memcpy(image.data(), cvFrame.data, size);
    captured = true;
    notifyFrame(camL);
  }
  else
  {
    widthR = cvFrame.cols;
    heightR = cvFrame.rows;
    channels = cvFrame.channels();
    const std::size_t size{ static_cast<std::size_t>(widthR) * heightR * channels };
    imageR.resize(size);
    std::memcpy(imageR.data(), cvFrame.data, size);
    capturedR = true;
    notifyFrame(camR);
  }

  return true;
}

//
// カメラが使用可能か判定する
//
bool CamImage::opened(int cam) const
{
  std::lock_guard<std::mutex> lock{ mtx };
  if (cam == camL) return !image.empty();
  if (cam == camR) return !imageR.empty();
  return false;
}

//
// 読み込んだ画像のデータを得る
//
const std::uint8_t* CamImage::getImage(int cam) const
{
  std::lock_guard<std::mutex> lock{ mtx };
  if (cam == camL && !image.empty()) return image.data();
  if (cam == camR && !imageR.empty()) return imageR.data();
  return nullptr;
}

//
// カメラフレームを OpenGL テクスチャへ転送する
//
bool CamImage::transmit(int eye, unsigned int texture, const int* size)
{
  return lockFrame(eye, [texture, size](const std::uint8_t* data, size_t length, int width, int height, int channels) {
    // 画像とテクスチャの大きさが違えば転送しない (バッファの範囲外を読まないようにする)
    if (width != size[0] || height != size[1]) return;

    // cv::imdecode() で読み込んだ画像は 3 チャンネルの BGR で, 行の境界は詰まっている
    glPixelStorei(GL_UNPACK_ALIGNMENT, channels == 4 ? 4 : 1);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size[0], size[1],
      channels == 4 ? GL_BGRA : GL_BGR, GL_UNSIGNED_BYTE, data);
  });
}

//
// OpenCV を使ったキャプチャ
//
#include "CamCv.h"

// コンストラクタ
CamCv::CamCv()
  : startTime{}
{
}

// デストラクタ
CamCv::~CamCv()
{
  // スレッドを停止する
  stop();

  // カメラを停止する
  for (int cam = 0; cam < camCount; ++cam)
    if (opened(cam)) camera[cam].release();
}

// カメラから入力する
bool CamCv::open(int cam, int device, const char* api, const char* codec, double width, double height, double fps)
{
  // カメラを開く
  if (!camera[cam].open(device, backend.at(api))) return false;

  // カメラを設定する
  setup(cam, codec, width, height, fps);

  // キャプチャを開始する
  return start(cam);
}

// ファイル／ネットワークからキャプチャを開始する
bool CamCv::open(const std::string& file, int cam)
{
  // ファイル／ネットワークを開く
  return camera[cam].open(file) && start(cam);
}

// カメラが使用可能か判定する
bool CamCv::opened(int cam)
{
  return camera[cam].isOpened();
}

// フレームをキャプチャする
void CamCv::capture(int cam)
{
  // スレッドが実行可の間
  while (run[cam])
  {
    // 経過時間が次のフレームの時刻に達していて
    if (glfwGetTime() - startTime[cam] >= camera[cam].get(cv::CAP_PROP_POS_MSEC) * 0.001)
    {
      // 次のフレームが存在すれば
      if (camera[cam].grab())
      {
        // キャプチャデバイスをロックして
        captureMutex[cam].lock();

        // 到着したフレームを切り出し
        camera[cam].retrieve(image[cam]);

        // キャプチャの完了を記録して
        unsent[cam] = captured[cam] = true;

        // ロックを解除したら
        captureMutex[cam].unlock();

        // 次のフレームに進む
        continue;
      }

      // フレームが取得できなかったらムービーファイルを巻き戻し
      if (camera[cam].set(cv::CAP_PROP_POS_FRAMES, 0.0))
      {
        // 次のフレームの時刻を求めて
        startTime[cam] = glfwGetTime() - camera[cam].get(cv::CAP_PROP_POS_MSEC) * 0.001 + interval[cam];

        // 次のフレームに進む
        continue;
      }
    }
  }
}

// キャプチャを準備する
void CamCv::setup(int cam, const char* codec, double width, double height, double fps)
{
  // カメラのコーデック・解像度・フレームレートを設定する
  if (codec[0] != '\0') camera[cam].set(cv::CAP_PROP_FOURCC,
//    cv::VideoWriter::fourcc(codec[0], codec[1], codec[2], codec[3]));
    cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
  if (width > 0.0) camera[cam].set(cv::CAP_PROP_FRAME_WIDTH, width);
  if (height > 0.0) camera[cam].set(cv::CAP_PROP_FRAME_HEIGHT, height);
  if (fps > 0.0) camera[cam].set(cv::CAP_PROP_FPS, fps);
}

// キャプチャを開始する
bool CamCv::start(int cam)
{
  // 左カメラから 1 フレームキャプチャする
  if (camera[cam].read(image[cam]))
  {
    // キャプチャした画像のフォーマットを調べる
    format = image[cam].channels() == 4 ? GL_BGRA : GL_BGR;

    // キャプチャした画像のフレームレートを取得する
    const double fps{ camera[cam].get(cv::CAP_PROP_FPS) };
    interval[cam] = fps > 0.0 ? 1.0 / fps : 1.0 / 30.0;

    // 次のフレームの時刻を求める
    startTime[cam] = glfwGetTime() - camera[cam].get(cv::CAP_PROP_POS_MSEC) * 0.001 + interval[cam];

#if defined(DEBUG)
    const int cc(static_cast<int>(camera[cam].get(cv::CAP_PROP_FOURCC)));
    std::cerr << "Camera:" << cam << ", width:" << getWidth(cam) << ", height:" << getHeight(cam)
      << ", fourcc: " << static_cast<char>(cc & 255)
      << static_cast<char>((cc >> 8) & 255)
      << static_cast<char>((cc >> 16) & 255)
      << static_cast<char>((cc >> 24) & 255) << "\n";
#endif

    // 左カメラの利得と露出を取得する
    gain = static_cast<GLsizei>(camera[cam].get(cv::CAP_PROP_GAIN));
    exposure = static_cast<GLsizei>(camera[cam].get(cv::CAP_PROP_EXPOSURE) * 10.0);

    // キャプチャスレッドを起動する
    run[cam] = true;
    captureThread[cam] = std::thread([this, cam]() { capture(cam); });

    return true;
  }

  return false;
}

// 露出を上げる
void CamCv::increaseExposure()
{
  const double e(static_cast<double>(++exposure) * 0.1);
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (camera[cam].isOpened()) camera[cam].set(cv::CAP_PROP_EXPOSURE, e);
  }
}

// 露出を下げる
void CamCv::decreaseExposure()
{
  const double e(static_cast<double>(--exposure) * 0.1);
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (camera[cam].isOpened()) camera[cam].set(cv::CAP_PROP_EXPOSURE, e);
  }
}

// 利得を上げる
void CamCv::increaseGain()
{
  const double g(++gain);
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (camera[cam].isOpened()) camera[cam].set(cv::CAP_PROP_GAIN, g);
  }
}

// 利得を下げる
void CamCv::decreaseGain()
{
  const double g(--gain);
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (camera[cam].isOpened()) camera[cam].set(cv::CAP_PROP_GAIN, g);
  }
}

// バックエンドのリスト
const std::map<std::string, int> CamCv::backend
{
  { "ANY", cv::CAP_ANY },
  { "DSHOW", cv::CAP_DSHOW },
  { "MSMF", cv::CAP_MSMF },
  { "GSTREAMER", cv::CAP_GSTREAMER },
  { "FFMPEG", cv::CAP_FFMPEG }
};

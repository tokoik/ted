//
// OpenCV を使ったキャプチャ
//
#include "CamCv.h"

// コンストラクタ
CamCv::CamCv()
{
}

// デストラクタ
CamCv::~CamCv()
{
  // スレッドを停止する
  stop();
}

// フレームをキャプチャする
void CamCv::capture(int cam)
{
  // あらかじめキャプチャデバイスをロックして
  captureMutex[cam].lock();

  // スレッドが実行可の間
  while (run[cam])
  {
    // バッファが空のとき経過時間が現在のフレームの時刻に達していて
    if (!buffer[cam] && glfwGetTime() >= frameTime[cam])
    {
      // 次のフレームが存在すれば
      if (camera[cam].grab())
      {
        // 到着したフレームを切り出して
        camera[cam].retrieve(image[cam], 3);

        // 画像を更新し
        buffer[cam] = image[cam].data;

        // 作業者として動作していたら
        if (isWorker())
        {
          // ヘッドトラッキング情報を保存して
          if (frame) frame->attitude[cam] = getLocalAttitude(cam);

          // フレームを圧縮して保存し
          cv::imencode(coder, image[cam], encoded[cam], param);
        }

        // 次のフレームに進む
        continue;
      }

      // フレームが取得できなかったらムービーファイルを巻き戻し
      if (camera[cam].set(CV_CAP_PROP_POS_FRAMES, 0.0))
      {
        // 経過時間をリセットして
        glfwSetTime(0.0);

        // フレームの時刻をリセットし
        frameTime[cam] = 0.0;

        // 次のフレームに進む
        continue;
      }
    }

    // フレームが切り出せなければロックを解除して
    captureMutex[cam].unlock();

    // 他のスレッドがリソースにアクセスするために少し待ってから
    std::this_thread::sleep_for(std::chrono::milliseconds(10L));

    // またキャプチャデバイスをロックする
    captureMutex[cam].lock();
  }

  // 終わるときはロックを解除する
  captureMutex[cam].unlock();
}

// キャプチャを開始する
bool CamCv::start(int cam)
{
  // カメラの解像度を設定する
  if (defaults.capture_width > 0.0) camera[cam].set(CV_CAP_PROP_FRAME_WIDTH, defaults.capture_width);
  if (defaults.capture_height > 0.0) camera[cam].set(CV_CAP_PROP_FRAME_HEIGHT, defaults.capture_height);
  if (defaults.capture_fps > 0.0) camera[cam].set(CV_CAP_PROP_FPS, defaults.capture_fps);

  // 左カメラから 1 フレームキャプチャする
  if (camera[cam].grab())
  {
    // 最初のフレームを取得した時刻を基準にする
    glfwSetTime(0.0);

    // 最初のフレームの時刻は 0 にする
    frameTime[cam] = 0.0;

    // キャプチャした画像のサイズを取得する
    size[cam][0] = static_cast<GLsizei>(camera[cam].get(CV_CAP_PROP_FRAME_WIDTH));
    size[cam][1] = static_cast<GLsizei>(camera[cam].get(CV_CAP_PROP_FRAME_HEIGHT));

    // キャプチャ用のメモリを確保する
    image[cam].create(size[cam][0], size[cam][1], CV_8UC3);
    buffer[cam] = image[cam].data;

    // 左カメラの利得と露出を取得する
    gain = static_cast<GLsizei>(camera[cam].get(CV_CAP_PROP_GAIN));
    exposure = static_cast<GLsizei>(camera[cam].get(CV_CAP_PROP_EXPOSURE) * 10.0);

    // キャプチャスレッドを起動する
    run[cam] = true;
    captureThread[cam] = std::thread([this, cam]() { this->capture(cam); });

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
    if (camera[cam].isOpened()) camera[cam].set(CV_CAP_PROP_EXPOSURE, e);
  }
}

// 露出を下げる
void CamCv::decreaseExposure()
{
  const double e(static_cast<double>(--exposure) * 0.1);
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (camera[cam].isOpened()) camera[cam].set(CV_CAP_PROP_EXPOSURE, e);
  }
}

// 利得を上げる
void CamCv::increaseGain()
{
  const double g(++gain);
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (camera[cam].isOpened()) camera[cam].set(CV_CAP_PROP_GAIN, g);
  }
}

// 利得を下げる
void CamCv::decreaseGain()
{
  const double g(--gain);
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (camera[cam].isOpened()) camera[cam].set(CV_CAP_PROP_GAIN, g);
  }
}

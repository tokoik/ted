///
/// 作業者 (WORKER) として映像と姿勢を送信するクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
#include "Worker.h"

// 共有メモリ上の姿勢 (localAttitude, remoteAttitude)
#include "Scene.h"

// 標準ライブラリ
#include <algorithm>
#include <cstring>

namespace
{
  /// 静止画像を送り直す間隔 (後から起動した指示者も画像を受け取れるようにする)
  constexpr auto stillImageInterval{ std::chrono::seconds(1) };

  /// 姿勢を送る間隔 (PC 版の他の送信処理と同じ minDelay)
  constexpr auto poseInterval{ std::chrono::milliseconds(minDelay) };
}

//
// デストラクタ
//
Worker::~Worker()
{
  stop();
}

//
// 作業者として送受信を開始する
//
int Worker::start(const std::shared_ptr<Camera>& camera, bool stereo, const Config& config)
{
  stop();
  if (!camera) return -1;

  // 作業者として初期化する (port + 1 で受信し port へ送信する)
  const int ret{ network.initialize(WORKER, static_cast<unsigned short>(config.port), config.address.c_str()) };
  if (ret != 0 || !network.running()) return ret != 0 ? ret : -1;

  this->camera = camera;
  eyeCount = stereo ? camCount : 1;
  swapEyes = stereo && config.camera_swap_eyes;
  transmitSize = cv::Size(config.transmit_size[0], config.transmit_size[1]);
  imageInterval = config.transmit_fps > 0.0
    ? std::chrono::duration_cast<std::chrono::steady_clock::duration>(
      std::chrono::duration<double>(1.0 / config.transmit_fps))
    : std::chrono::steady_clock::duration::zero();
  param = { cv::IMWRITE_JPEG_QUALITY, std::clamp(config.transmit_quality, 0, 100) };

  sendbuf.resize(maxFrameSize);
  recvbuf.resize(maxFrameSize);

  running = true;
  sendThread = std::thread([this]() { sendLoop(); });
  recvThread = std::thread([this]() { recvLoop(); });

  return 0;
}

//
// 送受信を停止する
//
void Worker::stop()
{
  // 受信スレッドは受信のタイムアウト (500ms) ごとに running を確認して抜ける
  if (running.exchange(false))
  {
    if (sendThread.joinable()) sendThread.join();
    if (recvThread.joinable()) recvThread.join();
  }
  network.finalize();
  camera.reset();
}

//
// カメラの最新フレームを JPEG に符号化して送信バッファへ追記する
//
unsigned int Worker::appendImage(int source, unsigned char*& data)
{
  // ロック中は複製だけを行い、キャプチャを長時間止めないよう変換と符号化はロックの外で行う
  int w{ 0 }, h{ 0 }, ch{ 0 };
  if (!camera->copyFrame(source, raw, w, h, ch)) return 0;
  const cv::Mat frame(h, w, CV_8UC(ch), raw.data());

  // JPEG は BGR で符号化する
  const cv::Mat* image{ &frame };
  if (ch == 4)
  {
    cv::cvtColor(frame, converted, cv::COLOR_BGRA2BGR);
    image = &converted;
  }

  // 伝送解像度が指定されていれば縮小・拡大する
  if (transmitSize.width > 0 && transmitSize.height > 0 && image->size() != transmitSize)
  {
    const bool shrink{ transmitSize.width < image->cols || transmitSize.height < image->rows };
    cv::resize(*image, resized, transmitSize, 0.0, 0.0, shrink ? cv::INTER_AREA : cv::INTER_LINEAR);
    image = &resized;
  }

  if (!cv::imencode(".jpg", *image, encoded, param)) return 0;

  // JPEG は内容によりサイズが変わるため、収まらない画像は送らない (姿勢だけ送る)
  const std::size_t used{ static_cast<std::size_t>(data - sendbuf.data()) };
  if (encoded.size() > sendbuf.size() - used) return 0;

  std::memcpy(data, encoded.data(), encoded.size());
  data += encoded.size();
  return static_cast<unsigned int>(encoded.size());
}

//
// 送信スレッド
//
void Worker::sendLoop()
{
  // 最後に送信したフレームの通し番号 (最初のフレームは必ず送る)
  std::uint64_t lastSerial{ ~std::uint64_t{ 0 } };

  // 次に画像を送ってよい時刻と、最後に画像を送った時刻
  auto nextImage{ std::chrono::steady_clock::now() };
  auto lastImage{ nextImage - stillImageInterval };

  while (running)
  {
    const auto start{ std::chrono::steady_clock::now() };

    // ヘッダのフォーマット: 左右のフレームのサイズ, 変換行列の数
    auto* const head{ reinterpret_cast<unsigned int*>(sendbuf.data()) };
    head[camL] = head[camR] = 0;
    head[camCount] = localAttitude->getSize();

    // 行列数は共有メモリの設定から決まるので、固定長の送信領域を越えないか確認する
    const std::size_t metadataBytes{ headLength * sizeof(unsigned int)
      + static_cast<std::size_t>(head[camCount]) * sizeof(GgMatrix) };
    if (metadataBytes > sendbuf.size())
    {
      std::this_thread::sleep_for(poseInterval);
      continue;
    }

    // 変換行列を共有メモリから取り出す
    auto* const body{ reinterpret_cast<GgMatrix*>(head + headLength) };
    localAttitude->load(body, head[camCount]);
    unsigned char* data{ reinterpret_cast<unsigned char*>(body + head[camCount]) };

    // 論理的な左眼に対応する物理入力
    const int sourceL{ swapEyes ? camR : camL };

    // 新しいフレームがあれば (静止画像なら一定間隔で) 画像を付ける
    const std::uint64_t serial{ camera->getFrameSerial(sourceL) };
    const bool fresh{ serial != lastSerial
      || (camera->isStillImage() && start - lastImage >= stillImageInterval) };
    if (fresh && start >= nextImage)
    {
      head[camL] = appendImage(sourceL, data);

      // 右画像は左画像を送れたときだけ付ける (右だけのフレームは受信側で扱えない)
      if (head[camL] > 0 && eyeCount > 1) head[camR] = appendImage(camR - sourceL, data);

      lastSerial = serial;
      lastImage = start;
      nextImage = start + imageInterval;
    }

    // フレームを送信する (画像が無くても姿勢だけは一定間隔で送る)
    network.sendData(sendbuf.data(), static_cast<int>(data - sendbuf.data()));

    // 次の送信時刻まで待つ
    std::this_thread::sleep_until(start + poseInterval);
  }
}

//
// 受信スレッド
//
void Worker::recvLoop()
{
  while (running)
  {
    // 姿勢データを受信する (タイムアウトは約 500ms)
    const int ret{ network.recvData(recvbuf.data(), static_cast<int>(recvbuf.size())) };

    // EOF (長さ 0) は相手の停止通知なので、相手の再起動に備えて受信を続ける
    if (ret <= 0 || !network.checkRemote()) continue;

    // 検証済みの行列だけを共有メモリへ保存する
    const unsigned int* head{ nullptr };
    const GgMatrix* body{ nullptr };
    const unsigned char* imageData{ nullptr };
    if (unpackFrame(recvbuf.data(), ret, head, body, imageData))
      remoteAttitude->store(body, head[camCount]);
  }
}

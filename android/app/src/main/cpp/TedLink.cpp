///
/// Quest 3 版と中継サーバ / 指示者 PC との通信の実装
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
#include "TedLink.h"

#include <android/log.h>

#include <algorithm>
#include <cstring>

#define LOG_TAG "TED"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

//
// デストラクタ
//
TedLink::~TedLink()
{
  stop();
}

//
// 通信を開始する
//
bool TedLink::start(const std::string& host, unsigned short port, int interval)
{
  stop();

  // 作業者 (WORKER) として port + 1 で受信し port へ送信する
  if (network.initialize(2, port, host.c_str()) != 0 || !network.running())
  {
    LOGW("TedLink: cannot open UDP sockets for %s:%u", host.c_str(), port);
    return false;
  }

  this->interval = std::max(interval, 1);

  {
    std::lock_guard<std::mutex> lock{ sendMutex };
    sendTable.assign(tableSize, qm::identity());

    // 手の関節はトラッキングするまで零行列 (非表示) にしておく
    std::fill(sendTable.begin() + camCount + 1, sendTable.end(), qm::zero());
    pendingImage[0].reset();
    pendingImage[1].reset();
    videoUnits.clear();
  }

  {
    std::lock_guard<std::mutex> lock{ recvMutex };
    recvTable.assign(maxTableSize, qm::identity());
    recvCount = 0;
    lastRecv = std::chrono::steady_clock::time_point{};
  }

  running = true;
  sendThread = std::thread([this]() { sendLoop(); });
  recvThread = std::thread([this]() { recvLoop(); });

  LOGI("TedLink: WORKER send to %s:%u, receive on port %u", host.c_str(), port, port + 1);
  return true;
}

//
// 通信を停止する
//
void TedLink::stop()
{
  if (running.exchange(false))
  {
    sendReady.notify_all();

    // 受信スレッドは受信のタイムアウト (500ms) ごとに running を確認して抜ける。
    // 相手へ EOF (長さ 0 のデータグラム) は送らない (受信側はいずれも EOF を無通信として扱う)。
    if (sendThread.joinable()) sendThread.join();
    if (recvThread.joinable()) recvThread.join();
  }
  network.finalize();
}

//
// 送信する変換行列のテーブルと画像を登録する
//
void TedLink::publish(const qm::Mat4* table, Image left, Image right)
{
  std::lock_guard<std::mutex> lock{ sendMutex };
  if (sendTable.size() != static_cast<std::size_t>(tableSize)) return;
  std::copy(table, table + tableSize, sendTable.begin());
  if (left)
  {
    // 新しい画像の組は、それより前の未送信の画像の組を置き換える
    pendingImage[0] = std::move(left);
    pendingImage[1] = std::move(right);
  }
}

//
// 受信した変換行列のテーブルを取り出す
//
bool TedLink::getRemote(qm::Mat4* table, int& count) const
{
  std::lock_guard<std::mutex> lock{ recvMutex };
  count = recvCount;
  if (recvCount <= 0) return false;
  std::copy(recvTable.begin(), recvTable.begin() + recvCount, table);
  return std::chrono::steady_clock::now() - lastRecv < std::chrono::seconds(1);
}

//
// 動画の 1 アクセスユニットを送信する
//
void TedLink::publishVideo(const qm::Mat4* table, int eye, std::uint32_t format, Image payload)
{
  if (!payload || eye < 0 || eye >= camCount) return;
  {
    std::lock_guard<std::mutex> lock{ sendMutex };

    // 送信が追いつかなければ捨てる (受信側は通し番号の欠落を検出してキーフレームを待つ)
    constexpr std::size_t maxUnits{ 60 };
    if (videoUnits.size() >= maxUnits)
    {
      videoUnits.clear();
      videoOverflowed = true;
    }
    videoUnits.push_back(VideoUnit{ std::vector<qm::Mat4>(table, table + tableSize), eye, format,
      std::move(payload) });
  }
  sendReady.notify_one();
}

//
// フレームを組み立てて送信する
//
void TedLink::sendFrame(std::vector<std::uint8_t>& buffer, const std::vector<qm::Mat4>& table,
  std::uint32_t format, const Image* image)
{
  // ヘッダと変換行列
  auto* const head{ reinterpret_cast<unsigned int*>(buffer.data()) };
  head[0] = head[1] = 0;
  head[camCount] = static_cast<unsigned int>(table.size()) | (format << ted::frameFormatShift);
  std::uint8_t* data{ buffer.data() + headLength * sizeof(unsigned int) };
  std::memcpy(data, table.data(), table.size() * sizeof(qm::Mat4));
  data += table.size() * sizeof(qm::Mat4);

  // 画像があれば左、右の順に続ける
  std::size_t total{ static_cast<std::size_t>(data - buffer.data()) };
  for (int eye = 0; eye < camCount; ++eye)
    if (image[eye]) total += image[eye]->size();

  if (total <= buffer.size())
  {
    for (int eye = 0; eye < camCount; ++eye)
    {
      if (!image[eye]) continue;
      std::memcpy(data, image[eye]->data(), image[eye]->size());
      data += image[eye]->size();
      head[eye] = static_cast<unsigned int>(image[eye]->size());
    }
  }
  else
  {
    // PC 版の受信バッファに収まらない画像は送らない (姿勢だけ送る)
    LOGW("TedLink: image data (%zu bytes) exceeds the frame limit; lower the quality or bitrate", total);
    if (format != ted::IMAGE_JPEG)
    {
      videoOverflowed = true;
      head[camCount] |= ted::frameKeyframeRequest;
    }
  }

  network.sendData(buffer.data(), static_cast<int>(data - buffer.data()));
}

//
// 送信スレッド
//
void TedLink::sendLoop()
{
  std::vector<std::uint8_t> buffer(maxFrameSize);
  auto next{ std::chrono::steady_clock::now() };

  while (running)
  {
    std::unique_lock<std::mutex> lock{ sendMutex };

    // 動画のアクセスユニットが届けばすぐに、そうでなければ次の姿勢の送信時刻まで待つ
    sendReady.wait_until(lock, next, [this]() { return !running || !videoUnits.empty(); });
    if (!running) break;

    if (!videoUnits.empty())
    {
      // 動画はアクセスユニットごとに、撮影時の姿勢と一緒に送る
      VideoUnit unit{ std::move(videoUnits.front()) };
      videoUnits.pop_front();
      lock.unlock();

      Image image[camCount];
      image[unit.eye] = std::move(unit.payload);
      sendFrame(buffer, unit.table, unit.format, image);
      continue;
    }

    // 一定間隔で姿勢 (と JPEG 画像があれば画像) を送る
    const std::vector<qm::Mat4> table{ sendTable };
    Image image[camCount]{ std::move(pendingImage[0]), std::move(pendingImage[1]) };
    lock.unlock();

    // 右画像だけのフレームは送らない
    if (!image[0]) image[1].reset();
    sendFrame(buffer, table, ted::IMAGE_JPEG, image);

    next += std::chrono::milliseconds(interval);
    const auto now{ std::chrono::steady_clock::now() };
    if (next < now) next = now;
  }
}

//
// 受信スレッド
//
void TedLink::recvLoop()
{
  std::vector<std::uint8_t> buffer(maxFrameSize);
  constexpr std::size_t headerBytes{ headLength * sizeof(unsigned int) };

  while (running)
  {
    const int ret{ network.recvData(buffer.data(), static_cast<int>(buffer.size())) };

    // 長さ 0 は相手の受信停止の通知なので、無通信として扱って受信を続ける
    if (ret <= 0 || !network.checkRemote()) continue;

    // PC 版の unpackFrame() と同じ検査で、フレーム内部の境界を確認する
    const std::size_t length{ static_cast<std::size_t>(ret) };
    if (length < headerBytes) continue;
    unsigned int head[headLength];
    std::memcpy(head, buffer.data(), headerBytes);
    const unsigned int matrices{ head[camCount] & ted::frameCountMask };
    const std::size_t matrixBytes{ static_cast<std::size_t>(matrices) * sizeof(qm::Mat4) };
    if (matrixBytes > length - headerBytes) continue;

    // 受信側 (指示者) がキーフレームを要求していれば記録する
    if (head[camCount] & ted::frameKeyframeRequest) keyframeRequested = true;

    // 指示者の変換行列を保存する (指示者から画像は送られてこないので読み捨てる)
    const int count{ std::min(static_cast<int>(matrices), maxTableSize) };
    std::lock_guard<std::mutex> lock{ recvMutex };
    std::memcpy(recvTable.data(), buffer.data() + headerBytes, count * sizeof(qm::Mat4));
    recvCount = count;
    lastRecv = std::chrono::steady_clock::now();
  }
}

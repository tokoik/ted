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
// 送信スレッド
//
void TedLink::sendLoop()
{
  std::vector<std::uint8_t> buffer(maxFrameSize);
  auto next{ std::chrono::steady_clock::now() };
  bool warned{ false };

  while (running)
  {
    // 送信するデータのスナップショットをロック中に取る
    std::vector<qm::Mat4> table;
    Image image[camCount];
    {
      std::lock_guard<std::mutex> lock{ sendMutex };
      table = sendTable;
      image[0] = std::move(pendingImage[0]);
      image[1] = std::move(pendingImage[1]);
    }

    // ヘッダと変換行列
    auto* const head{ reinterpret_cast<unsigned int*>(buffer.data()) };
    head[0] = head[1] = 0;
    head[camCount] = static_cast<unsigned int>(table.size());
    std::uint8_t* data{ buffer.data() + headLength * sizeof(unsigned int) };
    std::memcpy(data, table.data(), table.size() * sizeof(qm::Mat4));
    data += table.size() * sizeof(qm::Mat4);

    // 画像があれば左、右の順に続ける (右だけを送ることはできない)
    if (image[0])
    {
      const std::size_t used{ static_cast<std::size_t>(data - buffer.data()) };
      const std::size_t sizeL{ image[0]->size() };
      const std::size_t sizeR{ image[1] ? image[1]->size() : 0 };
      if (used + sizeL + sizeR <= buffer.size())
      {
        std::memcpy(data, image[0]->data(), sizeL);
        data += sizeL;
        head[0] = static_cast<unsigned int>(sizeL);
        if (sizeR > 0)
        {
          std::memcpy(data, image[1]->data(), sizeR);
          data += sizeR;
          head[1] = static_cast<unsigned int>(sizeR);
        }
        warned = false;
      }
      else if (!warned)
      {
        // PC 版の受信バッファに収まらない画像は送らない (姿勢だけ送る)
        LOGW("TedLink: encoded images (%zu + %zu bytes) exceed the frame limit; "
          "lower transmit_quality or camera size", sizeL, sizeR);
        warned = true;
      }
    }

    network.sendData(buffer.data(), static_cast<int>(data - buffer.data()));

    // 一定間隔で送信する
    next += std::chrono::milliseconds(interval);
    const auto now{ std::chrono::steady_clock::now() };
    if (next < now) next = now;
    std::this_thread::sleep_until(next);
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
    const std::size_t matrixBytes{ static_cast<std::size_t>(head[camCount]) * sizeof(qm::Mat4) };
    if (matrixBytes > length - headerBytes) continue;

    // 指示者の変換行列を保存する (指示者から画像は送られてこないので読み捨てる)
    const int count{ std::min(static_cast<int>(head[camCount]), maxTableSize) };
    std::lock_guard<std::mutex> lock{ recvMutex };
    std::memcpy(recvTable.data(), buffer.data() + headerBytes, count * sizeof(qm::Mat4));
    recvCount = count;
    lastRecv = std::chrono::steady_clock::now();
  }
}

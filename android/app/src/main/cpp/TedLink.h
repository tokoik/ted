#pragma once

///
/// Quest 3 版と中継サーバ / 指示者 PC との通信
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
/// @details
/// Quest 3 版は作業者 (WORKER, role 2) として振る舞う。
/// 通信には中継サーバと同じ server/Network.h/.cpp をそのまま使うので、
/// UDP の分割・再構成の方式は PC 版 TED と完全に一致する。
///
/// 1 フレームのレイアウトは PC 版の CamRemote と同じ
///   unsigned int head[3] = { 左 JPEG のバイト数, 右 JPEG のバイト数, 変換行列の数 }
///   GgMatrix body[head[2]]   (列優先の float[16])
///   左 JPEG, 右 JPEG
/// である。変換行列のテーブルは
///   [0], [1] : 左右眼の頭部姿勢 (送信する画像を撮影したときの頭部中心姿勢)
///   [2]      : モデル変換行列 (Quest 3 版では単位行列)
///   [3 + joint * 2 + hand] : 手の関節 (hand 0 が右手, 1 が左手, 頭部座標系)
/// で、PC 版の Scene の共有メモリと同じ並びにしている。
///

#include "Network.h"
#include "QuestMath.h"
#include "TedProtocol.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class TedLink
{
public:

  /// 左右のカメラ
  static constexpr int camCount{ 2 };

  /// ヘッダの要素数 (左右の画像サイズと変換行列の数)
  static constexpr int headLength{ camCount + 1 };

  /// 片手の関節数 (手のひら, 手首, 各指 4 本の骨)
  static constexpr int jointsPerHand{ 2 + 5 * 4 };

  /// 両手の関節数 (PC 版の jointCount と同じ)
  static constexpr int jointCount{ jointsPerHand * 2 };

  /// 送信する変換行列の数 (PC 版の jointCount + camCount + 1)
  static constexpr int tableSize{ jointCount + camCount + 1 };

  /// 受信する変換行列の上限 (PC 版の remote_share_size / local_share_size の既定値)
  static constexpr int maxTableSize{ 64 };

  /// 1 フレームの上限 (PC 版の maxFrameSize と同じ)
  static constexpr int maxFrameSize{ 1024 * 1024 };

  /// 符号化済み画像
  using Image = std::shared_ptr<const std::vector<std::uint8_t>>;

  TedLink() = default;
  ~TedLink();

  TedLink(const TedLink&) = delete;
  TedLink& operator=(const TedLink&) = delete;

  ///
  /// 通信を開始する
  ///
  /// @param host 通信相手 (中継サーバまたは指示者 PC) の IP アドレス
  /// @param port ポート番号 (port + 1 で受信し port へ送信する)
  /// @param interval 姿勢を送信する間隔 (ミリ秒)
  /// @return 開始できたら true
  ///
  bool start(const std::string& host, unsigned short port, int interval);

  ///
  /// 通信を停止する
  ///
  void stop();

  ///
  /// 送信する変換行列のテーブルと、あれば新しい画像を登録する
  ///
  /// @param table 変換行列のテーブル (tableSize 個)
  /// @param left 新しい左画像 (無ければ nullptr)
  /// @param right 新しい右画像 (無ければ nullptr)
  ///
  /// @details
  /// 画像とその撮影時の頭部姿勢が必ず同じフレームで送られるよう、一度に登録する。
  ///
  void publish(const qm::Mat4* table, Image left, Image right);

  ///
  /// 動画の 1 アクセスユニットを送信する
  ///
  /// @param table アクセスユニットを撮影したときの変換行列のテーブル (tableSize 個)
  /// @param eye 視点 (0: 左, 1: 右)
  /// @param format 画像の形式 (ted::IMAGE_H264 または ted::IMAGE_HEVC)
  /// @param payload VideoUnitHeader 付きのアクセスユニット
  ///
  /// @details
  /// 動画は途中のアクセスユニットが欠けると復号できなくなるので、JPEG と違って
  /// 新しいもので置き換えず、登録した順にすべて送る。送信が追いつかずに溢れたら捨てて、
  /// takeOverflow() で知らせる (キーフレームで受信側を復帰させる)。
  ///
  void publishVideo(const qm::Mat4* table, int eye, std::uint32_t format, Image payload);

  ///
  /// 受信側からキーフレームを要求されていれば true を返して要求を消す
  ///
  bool takeKeyframeRequest()
  {
    return keyframeRequested.exchange(false);
  }

  ///
  /// 送信待ちの動画が溢れて捨てていれば true を返して記録を消す
  ///
  bool takeOverflow()
  {
    return videoOverflowed.exchange(false);
  }

  ///
  /// 受信した変換行列のテーブルを取り出す
  ///
  /// @param table 格納先 (maxTableSize 個)
  /// @param count 受信した変換行列の数
  /// @return 最近 (1 秒以内) に受信していれば true
  ///
  bool getRemote(qm::Mat4* table, int& count) const;

private:

  /// 送信スレッド
  void sendLoop();

  /// 受信スレッド
  void recvLoop();

  /// UDP 通信
  Network network;

  /// 送受信スレッド
  std::thread sendThread, recvThread;

  /// スレッドの実行中なら true
  std::atomic<bool> running{ false };

  /// 送信姿勢の排他制御
  mutable std::mutex sendMutex;

  /// 送信する変換行列のテーブル
  std::vector<qm::Mat4> sendTable;

  /// 次のフレームで送る画像
  Image pendingImage[camCount];

  ///
  /// 送信待ちの動画のアクセスユニット
  ///
  struct VideoUnit
  {
    std::vector<qm::Mat4> table;
    int eye{ 0 };
    std::uint32_t format{ 0 };
    Image payload;
  };

  /// 送信待ちの動画のアクセスユニット
  std::deque<VideoUnit> videoUnits;

  /// 送信待ちの動画が届いたことを送信スレッドに知らせる
  std::condition_variable sendReady;

  /// 送信待ちの動画が溢れて捨てたら true
  std::atomic<bool> videoOverflowed{ false };

  /// 受信側からキーフレームを要求されたら true
  std::atomic<bool> keyframeRequested{ false };

  ///
  /// フレームを組み立てて送信する
  ///
  void sendFrame(std::vector<std::uint8_t>& buffer, const std::vector<qm::Mat4>& table, std::uint32_t format,
    const Image* image);

  /// 姿勢を送信する間隔 (ミリ秒)
  int interval{ 10 };

  /// 受信姿勢の排他制御
  mutable std::mutex recvMutex;

  /// 受信した変換行列のテーブル
  std::vector<qm::Mat4> recvTable;

  /// 受信した変換行列の数
  int recvCount{ 0 };

  /// 最後に受信した時刻
  std::chrono::steady_clock::time_point lastRecv;
};

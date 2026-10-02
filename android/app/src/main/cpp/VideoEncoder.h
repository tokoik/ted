#pragma once

///
/// パススルーカメラの映像を H.264 / HEVC に符号化するクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
/// @details
/// AMediaCodec のハードウェアエンコーダの入力 Surface をカメラの出力先にするので、
/// 画素を CPU で扱わずに符号化できる。低遅延の設定 (B フレームなし、リアルタイム優先) にし、
/// キーフレームには SPS / PPS (HEVC では VPS も) を必ず付けて、途中から受信しても復号できるようにする。
///

#include <media/NdkMediaCodec.h>
#include <android/native_window.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

class VideoEncoder
{
public:

  /// 符号化済みのアクセスユニット (先頭に ted::VideoUnitHeader が付く)
  using Payload = std::shared_ptr<const std::vector<std::uint8_t>>;

  ///
  /// 符号化したアクセスユニット
  ///
  struct Unit
  {
    /// VideoUnitHeader 付きのアクセスユニット
    Payload payload;

    /// 撮影時刻 (カメラのタイムスタンプ, ナノ秒)
    std::int64_t timestamp{ 0 };

    /// キーフレームなら true
    bool keyframe{ false };
  };

  /// 符号化したアクセスユニットを受け取る関数 (出力スレッドから呼ばれる)
  using Callback = std::function<void(Unit&&)>;

  VideoEncoder() = default;
  ~VideoEncoder();

  VideoEncoder(const VideoEncoder&) = delete;
  VideoEncoder& operator=(const VideoEncoder&) = delete;

  ///
  /// エンコーダを開始する
  ///
  /// @param format 画像の形式 (ted::IMAGE_H264 または ted::IMAGE_HEVC)
  /// @param width 画像の幅
  /// @param height 画像の高さ
  /// @param bitrate ビットレート (bps)
  /// @param fps 符号化するフレームレートの上限 (0 なら制限しない)
  /// @param keyframeInterval キーフレームの間隔 (秒)
  /// @param callback 符号化したアクセスユニットを受け取る関数
  /// @return 開始できたら true
  ///
  bool open(std::uint32_t format, int width, int height, int bitrate, double fps, int keyframeInterval,
    Callback callback);

  ///
  /// エンコーダを停止する (先にカメラの出力を止めておくこと)
  ///
  void close();

  ///
  /// カメラの出力先にする入力 Surface
  ///
  ANativeWindow* getInputSurface() const
  {
    return surface;
  }

  ///
  /// 次のフレームをキーフレームにする
  ///
  /// @return 要求に成功したら true
  ///
  bool requestKeyframe();

private:

  /// 出力スレッド
  void outputLoop();

  /// エンコーダ
  AMediaCodec* codec{ nullptr };

  /// 入力 Surface
  ANativeWindow* surface{ nullptr };

  /// 画像の形式
  std::uint32_t format{ 0 };

  /// 符号化したアクセスユニットを受け取る関数
  Callback callback;

  /// 出力スレッド
  std::thread output;
  std::atomic<bool> running{ false };

  /// 最後に出力された SPS / PPS (キーフレームに付いていなければ前に付ける)
  std::vector<std::uint8_t> config;

  /// 出力したアクセスユニットの通し番号
  std::uint32_t number{ 0 };
};

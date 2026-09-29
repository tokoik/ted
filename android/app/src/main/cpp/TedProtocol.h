#pragma once

///
/// Quest 3 版と PC 版の通信フレームの取り決め
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
/// @details
/// PC 版の Network.h にある定義と同じ値を保つこと。
///
/// 通信フレームのヘッダ head[2] の構成
/// - ビット 0～15: 変換行列の数
/// - ビット 16: キーフレーム要求 (動画を受信する側が送信側へ要求する)
/// - ビット 24～27: 画像の形式
///
/// 画像が動画 (H.264 / HEVC) のとき、左右の画像はそれぞれ 1 アクセスユニット (Annex B) で、
/// 先頭に VideoUnitHeader (8 バイト) が付く。左右のどちらか一方だけのフレームもある。
///

#include <cstdint>

namespace ted
{
  constexpr std::uint32_t frameCountMask{ 0xffffu };
  constexpr std::uint32_t frameKeyframeRequest{ 1u << 16 };
  constexpr std::uint32_t frameFormatShift{ 24 };
  constexpr std::uint32_t frameFormatMask{ 0xfu << frameFormatShift };

  /// 画像の形式
  enum ImageFormat : std::uint32_t
  {
    IMAGE_JPEG = 0,                 ///< 1 枚ずつ独立した JPEG
    IMAGE_H264 = 1,                 ///< H.264 (Annex B) のアクセスユニット
    IMAGE_HEVC = 2                  ///< HEVC (Annex B) のアクセスユニット
  };

  ///
  /// 動画のアクセスユニットの前に付ける情報
  ///
  struct VideoUnitHeader
  {
    /// 視点ごとの通し番号 (欠落の検出に使う)
    std::uint32_t number;

    /// フラグ (VIDEO_UNIT_KEYFRAME)
    std::uint32_t flags;
  };

  static_assert(sizeof(VideoUnitHeader) == 8, "VideoUnitHeader must be 8 bytes");

  /// キーフレーム (これだけで復号できるアクセスユニット)
  constexpr std::uint32_t VIDEO_UNIT_KEYFRAME{ 1u };
}

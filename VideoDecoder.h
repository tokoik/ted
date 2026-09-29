#pragma once

///
/// H.264 / HEVC のアクセスユニットを復号するクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
/// @details
/// Quest 3 版などがハードウェアエンコーダで符号化して送る動画を、
/// Media Foundation の同期型デコーダ MFT で BGR 画像に復号する。
/// 低遅延モード (MF_LOW_LATENCY) にして、1 アクセスユニットを入力するたびに 1 フレームを取り出す。
/// COM と Media Foundation は、このクラスを使うスレッドで初期化しておくこと。
///

// windows.h が古い winsock.h を巻き込み、Network.h の winsock2.h と衝突するのを防ぐ
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif

// windows.h の min/max マクロが std::min/std::max と衝突しないようにする
#ifndef NOMINMAX
#  define NOMINMAX
#endif

// OpenCV
#include "opencv_link.h"

// Media Foundation
#include <mfapi.h>
#include <mftransform.h>

#include <cstddef>
#include <cstdint>

class VideoDecoder
{
  /// デコーダ
  IMFTransform* decoder{ nullptr };

  /// 出力サンプルをデコーダが用意するなら true
  bool providesSamples{ false };

  /// 出力サンプルとバッファ (デコーダが用意しない場合に再利用する)
  IMFSample* outputSample{ nullptr };
  DWORD outputSize{ 0 };

  /// 出力画像の大きさ (符号化の単位に揃えた大きさ) と表示領域
  UINT32 codedWidth{ 0 }, codedHeight{ 0 };
  UINT32 displayWidth{ 0 }, displayHeight{ 0 };
  LONG stride{ 0 };

  /// 出力形式を NV12 に設定する
  HRESULT setOutputType();

  /// 出力サンプルを用意する
  HRESULT prepareOutputSample();

public:

  VideoDecoder() = default;
  ~VideoDecoder();

  VideoDecoder(const VideoDecoder&) = delete;
  VideoDecoder& operator=(const VideoDecoder&) = delete;

  ///
  /// デコーダを開く
  ///
  /// @param format 画像の形式 (IMAGE_H264 または IMAGE_HEVC)
  /// @return 成功したら true
  ///
  bool open(unsigned int format);

  ///
  /// デコーダを閉じる
  ///
  void close();

  ///
  /// 開いているかどうか
  ///
  bool isOpen() const
  {
    return decoder != nullptr;
  }

  ///
  /// 1 アクセスユニットを復号する
  ///
  /// @param data アクセスユニット (Annex B)
  /// @param size アクセスユニットのバイト数
  /// @param time 表示時刻 (100 ナノ秒単位, 単調増加していればよい)
  /// @param image 復号した BGR 画像の格納先
  /// @return 1: 画像を得た, 0: まだ画像が出ない, -1: 復号に失敗した
  ///
  int decode(const std::uint8_t* data, std::size_t size, LONGLONG time, cv::Mat& image);
};

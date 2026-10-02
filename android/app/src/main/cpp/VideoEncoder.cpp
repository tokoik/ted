///
/// パススルーカメラの映像を H.264 / HEVC に符号化するクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
#include "VideoEncoder.h"
#include "TedProtocol.h"

#include <android/log.h>
#include <media/NdkMediaFormat.h>

#include <cstring>

#define LOG_TAG "TED"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace
{
  /// MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface
  constexpr std::int32_t colorFormatSurface{ 0x7F000789 };

  /// MediaCodec.BUFFER_FLAG_KEY_FRAME (NDK の定数は API 34 から)
  constexpr std::uint32_t bufferFlagKeyFrame{ 1 };

  /// MediaCodecInfo.EncoderCapabilities.BITRATE_MODE_CBR
  constexpr std::int32_t bitrateModeCbr{ 2 };

  ///
  /// アクセスユニットの先頭が SPS (HEVC では VPS) で始まるか調べる
  ///
  bool startsWithParameterSets(const std::uint8_t* data, std::size_t size, std::uint32_t format)
  {
    std::size_t i{ 0 };
    while (i + 3 < size && data[i] == 0) ++i;
    if (i < 2 || i >= size || data[i] != 1 || i + 1 >= size) return false;
    const std::uint8_t header{ data[i + 1] };
    if (format == ted::IMAGE_HEVC) return ((header >> 1) & 0x3f) == 32;
    return (header & 0x1f) == 7;
  }
}

//
// デストラクタ
//
VideoEncoder::~VideoEncoder()
{
  close();
}

//
// エンコーダを開始する
//
bool VideoEncoder::open(std::uint32_t format, int width, int height, int bitrate, double fps,
  int keyframeInterval, Callback callback)
{
  close();

  const char* const mime{ format == ted::IMAGE_HEVC ? "video/hevc" : "video/avc" };
  this->format = format;
  this->callback = std::move(callback);
  number = 0;
  config.clear();

  codec = AMediaCodec_createEncoderByType(mime);
  if (!codec)
  {
    LOGE("no %s encoder", mime);
    return false;
  }

  // 低遅延のための設定は、使えない端末でも基本設定だけで動くよう、失敗したら外してやり直す
  for (int attempt = 0; attempt < 2; ++attempt)
  {
    AMediaFormat* const mediaFormat{ AMediaFormat_new() };
    AMediaFormat_setString(mediaFormat, AMEDIAFORMAT_KEY_MIME, mime);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_WIDTH, width);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_HEIGHT, height);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_BIT_RATE, bitrate);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_FRAME_RATE, fps > 0.0 ? static_cast<int>(fps) : 30);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, keyframeInterval);
    AMediaFormat_setInt32(mediaFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT, colorFormatSurface);
    if (attempt == 0)
    {
      // 一定ビットレート、リアルタイム優先、B フレームなし、キーフレームに SPS / PPS を付ける
      AMediaFormat_setInt32(mediaFormat, "bitrate-mode", bitrateModeCbr);
      AMediaFormat_setInt32(mediaFormat, "priority", 0);
      AMediaFormat_setInt32(mediaFormat, "latency", 1);
      AMediaFormat_setInt32(mediaFormat, "max-bframes", 0);
      AMediaFormat_setInt32(mediaFormat, "prepend-sps-pps-to-idr-frames", 1);
      if (fps > 0.0) AMediaFormat_setFloat(mediaFormat, "max-fps-to-encoder", static_cast<float>(fps));
    }

    const media_status_t status{ AMediaCodec_configure(codec, mediaFormat, nullptr, nullptr,
      AMEDIACODEC_CONFIGURE_FLAG_ENCODE) };
    AMediaFormat_delete(mediaFormat);
    if (status == AMEDIA_OK) break;

    LOGW("%s encoder configuration %d failed (%d)", mime, attempt, status);
    if (attempt == 1)
    {
      close();
      return false;
    }
  }

  if (AMediaCodec_createInputSurface(codec, &surface) != AMEDIA_OK || !surface)
  {
    LOGE("AMediaCodec_createInputSurface failed");
    close();
    return false;
  }

  if (AMediaCodec_start(codec) != AMEDIA_OK)
  {
    LOGE("AMediaCodec_start failed");
    close();
    return false;
  }

  running = true;
  output = std::thread([this]() { outputLoop(); });

  LOGI("%s encoder: %dx%d, %d bps, %.1f fps, keyframe every %d s", mime, width, height, bitrate, fps,
    keyframeInterval);
  return true;
}

//
// エンコーダを停止する
//
void VideoEncoder::close()
{
  running = false;
  if (output.joinable()) output.join();
  if (codec)
  {
    AMediaCodec_stop(codec);
    AMediaCodec_delete(codec);
    codec = nullptr;
  }
  if (surface)
  {
    ANativeWindow_release(surface);
    surface = nullptr;
  }
}

bool VideoEncoder::requestKeyframe()
{
  if (!codec) return false;
  AMediaFormat* const parameters{ AMediaFormat_new() };
  AMediaFormat_setInt32(parameters, "request-sync", 0);
  const media_status_t status{ AMediaCodec_setParameters(codec, parameters) };
  AMediaFormat_delete(parameters);
  if (status != AMEDIA_OK)
  {
    LOGW("AMediaCodec_setParameters(request-sync) failed: %d", status);
    return false;
  }
  return true;
}

//
// 出力スレッド
//
void VideoEncoder::outputLoop()
{
  while (running)
  {
    AMediaCodecBufferInfo info{};
    const ssize_t index{ AMediaCodec_dequeueOutputBuffer(codec, &info, 20000) };
    if (index < 0) continue;

    std::size_t capacity{ 0 };
    const std::uint8_t* const buffer{ AMediaCodec_getOutputBuffer(codec, static_cast<std::size_t>(index), &capacity) };
    if (buffer && info.size > 0 && static_cast<std::size_t>(info.offset) + info.size <= capacity)
    {
      const std::uint8_t* const data{ buffer + info.offset };
      const std::size_t size{ static_cast<std::size_t>(info.size) };

      if (info.flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG)
      {
        // SPS / PPS はキーフレームの前に付けるため保存しておく
        config.assign(data, data + size);
      }
      else
      {
        const bool keyframe{ (info.flags & bufferFlagKeyFrame) != 0 };
        const bool prepend{ keyframe && !config.empty() && !startsWithParameterSets(data, size, format) };

        auto payload{ std::make_shared<std::vector<std::uint8_t>>() };
        payload->resize(sizeof(ted::VideoUnitHeader) + (prepend ? config.size() : 0) + size);
        const ted::VideoUnitHeader header{ number++, keyframe ? ted::VIDEO_UNIT_KEYFRAME : 0u };
        std::uint8_t* p{ payload->data() };
        std::memcpy(p, &header, sizeof header);
        p += sizeof header;
        if (prepend)
        {
          std::memcpy(p, config.data(), config.size());
          p += config.size();
        }
        std::memcpy(p, data, size);

        if (callback) callback(Unit{ std::move(payload), info.presentationTimeUs * 1000, keyframe });
      }
    }

    AMediaCodec_releaseOutputBuffer(codec, static_cast<std::size_t>(index), false);
  }
}

///
/// H.264 / HEVC のアクセスユニットを復号するクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
#include "VideoDecoder.h"

// 画像の形式
#include "Network.h"

#include <mferror.h>

#include <algorithm>
#include <cstring>

#pragma comment(lib, "MFplat.lib")
#pragma comment(lib, "MFuuid.lib")

namespace
{
  template <class T>
  void safeRelease(T*& p)
  {
    if (p)
    {
      p->Release();
      p = nullptr;
    }
  }
}

//
// デストラクタ
//
VideoDecoder::~VideoDecoder()
{
  close();
}

//
// デコーダを開く
//
bool VideoDecoder::open(unsigned int format)
{
  close();

  GUID subtype;
  if (format == IMAGE_H264) subtype = MFVideoFormat_H264;
  else if (format == IMAGE_HEVC) subtype = MFVideoFormat_HEVC;
  else return false;

  // 同期型のデコーダを探す (非同期型のハードウェアデコーダはイベント駆動が必要なので使わない)
  MFT_REGISTER_TYPE_INFO inputInfo{ MFMediaType_Video, subtype };
  MFT_REGISTER_TYPE_INFO outputInfo{ MFMediaType_Video, MFVideoFormat_NV12 };
  IMFActivate** activate{ nullptr };
  UINT32 count{ 0 };
  HRESULT hr{ MFTEnumEx(MFT_CATEGORY_VIDEO_DECODER,
    MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_LOCALMFT | MFT_ENUM_FLAG_SORTANDFILTER,
    &inputInfo, &outputInfo, &activate, &count) };
  if (SUCCEEDED(hr) && count == 0) hr = MF_E_TOPO_CODEC_NOT_FOUND;
  if (SUCCEEDED(hr)) hr = activate[0]->ActivateObject(IID_PPV_ARGS(&decoder));
  for (UINT32 i = 0; i < count; ++i) activate[i]->Release();
  CoTaskMemFree(activate);
  if (FAILED(hr))
  {
    decoder = nullptr;
    return false;
  }

  // 1 アクセスユニットごとにすぐ画像を出すようにする
  IMFAttributes* attributes{ nullptr };
  if (SUCCEEDED(decoder->GetAttributes(&attributes)) && attributes)
  {
    attributes->SetUINT32(MF_LOW_LATENCY, TRUE);
    attributes->Release();
  }

  // 入力形式 (大きさは最初の SPS から決まるので指定しない)
  IMFMediaType* inputType{ nullptr };
  hr = MFCreateMediaType(&inputType);
  if (SUCCEEDED(hr)) hr = inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (SUCCEEDED(hr)) hr = inputType->SetGUID(MF_MT_SUBTYPE, subtype);
  if (SUCCEEDED(hr)) hr = inputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
  if (SUCCEEDED(hr)) hr = decoder->SetInputType(0, inputType, 0);
  safeRelease(inputType);

  if (SUCCEEDED(hr)) hr = setOutputType();
  if (SUCCEEDED(hr)) hr = decoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
  if (SUCCEEDED(hr)) hr = decoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);

  if (FAILED(hr))
  {
    close();
    return false;
  }

  return true;
}

//
// デコーダを閉じる
//
void VideoDecoder::close()
{
  safeRelease(outputSample);
  safeRelease(decoder);
  outputSize = 0;
  codedWidth = codedHeight = displayWidth = displayHeight = 0;
  stride = 0;
}

//
// 出力形式を NV12 に設定する
//
HRESULT VideoDecoder::setOutputType()
{
  HRESULT hr{ MF_E_INVALIDMEDIATYPE };
  for (DWORD i = 0;; ++i)
  {
    IMFMediaType* type{ nullptr };
    if (FAILED(decoder->GetOutputAvailableType(0, i, &type))) break;

    GUID subtype{};
    if (SUCCEEDED(type->GetGUID(MF_MT_SUBTYPE, &subtype)) && subtype == MFVideoFormat_NV12)
    {
      hr = decoder->SetOutputType(0, type, 0);
      if (SUCCEEDED(hr))
      {
        MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &codedWidth, &codedHeight);
        UINT32 defaultStride{ 0 };
        if (SUCCEEDED(type->GetUINT32(MF_MT_DEFAULT_STRIDE, &defaultStride)))
        {
          stride = static_cast<LONG>(static_cast<INT32>(defaultStride));
        }
        else
        {
          LONG tempStride{ 0 };
          if (SUCCEEDED(MFGetStrideForBitmapInfoHeader(MFVideoFormat_NV12.Data1, codedWidth, &tempStride)))
          {
            stride = tempStride;
          }
          else
          {
            stride = static_cast<LONG>(codedWidth);
          }
        }

        // 符号化の単位 (16 画素) に揃えた余白を表示領域で除く
        MFVideoArea area{};
        if (SUCCEEDED(type->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE,
          reinterpret_cast<UINT8*>(&area), sizeof area, nullptr)) && area.Area.cx > 0 && area.Area.cy > 0)
        {
          displayWidth = static_cast<UINT32>(area.Area.cx);
          displayHeight = static_cast<UINT32>(area.Area.cy);
        }
        else
        {
          displayWidth = codedWidth;
          displayHeight = codedHeight;
        }
      }
      type->Release();
      break;
    }
    type->Release();
  }
  if (FAILED(hr)) return hr;

  // 出力サンプルの要件は形式によって変わるので作り直す
  MFT_OUTPUT_STREAM_INFO info{};
  hr = decoder->GetOutputStreamInfo(0, &info);
  if (FAILED(hr)) return hr;
  providesSamples = (info.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES) != 0;
  outputSize = (std::max<DWORD>)(info.cbSize, static_cast<DWORD>(std::abs(stride)) * codedHeight * 3 / 2);
  safeRelease(outputSample);
  return S_OK;
}

//
// 出力サンプルを用意する
//
HRESULT VideoDecoder::prepareOutputSample()
{
  if (providesSamples) return S_OK;

  if (!outputSample)
  {
    IMFMediaBuffer* buffer{ nullptr };
    HRESULT hr{ MFCreateSample(&outputSample) };
    if (SUCCEEDED(hr)) hr = MFCreateMemoryBuffer((std::max<DWORD>)(outputSize, 1), &buffer);
    if (SUCCEEDED(hr)) hr = outputSample->AddBuffer(buffer);
    safeRelease(buffer);
    if (FAILED(hr))
    {
      safeRelease(outputSample);
      return hr;
    }
  }

  IMFMediaBuffer* buffer{ nullptr };
  if (SUCCEEDED(outputSample->GetBufferByIndex(0, &buffer)))
  {
    buffer->SetCurrentLength(0);
    buffer->Release();
  }
  return S_OK;
}

//
// 1 アクセスユニットを復号する
//
int VideoDecoder::decode(const std::uint8_t* data, std::size_t size, LONGLONG time, cv::Mat& image)
{
  if (!decoder || !data || size == 0) return -1;

  // アクセスユニットをサンプルに詰める
  IMFMediaBuffer* inputBuffer{ nullptr };
  IMFSample* inputSample{ nullptr };
  HRESULT hr{ MFCreateMemoryBuffer(static_cast<DWORD>(size), &inputBuffer) };
  if (SUCCEEDED(hr))
  {
    BYTE* dst{ nullptr };
    hr = inputBuffer->Lock(&dst, nullptr, nullptr);
    if (SUCCEEDED(hr))
    {
      std::memcpy(dst, data, size);
      inputBuffer->Unlock();
      hr = inputBuffer->SetCurrentLength(static_cast<DWORD>(size));
    }
  }
  if (SUCCEEDED(hr)) hr = MFCreateSample(&inputSample);
  if (SUCCEEDED(hr)) hr = inputSample->AddBuffer(inputBuffer);
  if (SUCCEEDED(hr)) hr = inputSample->SetSampleTime(time);
  safeRelease(inputBuffer);
  if (FAILED(hr))
  {
    safeRelease(inputSample);
    return -1;
  }

  int result{ 0 };

  // 出力を取り出せるだけ取り出す (最後に得た画像を返す)
  const auto drain = [this, &image, &result]()
  {
    for (;;)
    {
      if (FAILED(prepareOutputSample())) return false;

      MFT_OUTPUT_DATA_BUFFER output{ 0, providesSamples ? nullptr : outputSample, 0, nullptr };
      DWORD status{ 0 };
      const HRESULT hr{ decoder->ProcessOutput(0, 1, &output, &status) };
      if (output.pEvents) output.pEvents->Release();

      // MFT が出力サンプルを用意する場合、スコープ脱出時に確実に解放する
      struct SampleGuard
      {
        IMFSample* sample{ nullptr };
        bool active{ false };
        ~SampleGuard() { if (active && sample) sample->Release(); }
      } guard{ output.pSample, providesSamples };

      if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) return true;
      if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
      {
        // 最初の SPS で大きさが決まったときなどに出力形式を設定し直す
        if (FAILED(setOutputType())) return false;
        continue;
      }
      if (FAILED(hr) || !output.pSample) return false;

      // NV12 を BGR に変換する
      IMFMediaBuffer* buffer{ nullptr };
      bool converted{ false };
      if (SUCCEEDED(output.pSample->ConvertToContiguousBuffer(&buffer)))
      {
        IMF2DBuffer* buffer2d{ nullptr };
        BYTE* base{ nullptr };
        LONG pitch{ 0 };
        DWORD currentLength{ 0 };
        bool locked2d{ false };
        if (SUCCEEDED(buffer->QueryInterface(IID_PPV_ARGS(&buffer2d)))
          && SUCCEEDED(buffer2d->Lock2D(&base, &pitch)))
        {
          locked2d = true;
          buffer->GetCurrentLength(&currentLength);
        }
        else if (SUCCEEDED(buffer->Lock(&base, nullptr, &currentLength)))
        {
          pitch = stride != 0 ? stride : static_cast<LONG>(codedWidth);
        }

        const LONG absPitch{ std::abs(pitch) };
        if (base && absPitch > 0 && displayWidth > 0 && displayHeight > 0)
        {
          const std::size_t ySize{ static_cast<std::size_t>(absPitch) * codedHeight };
          const std::size_t totalNeeded{ ySize + static_cast<std::size_t>(absPitch) * (codedHeight / 2) };

          if (currentLength == 0 || currentLength >= totalNeeded)
          {
            const cv::Mat y(static_cast<int>(displayHeight), static_cast<int>(displayWidth), CV_8UC1,
              base, static_cast<std::size_t>(absPitch));
            const cv::Mat uv(static_cast<int>(displayHeight / 2), static_cast<int>(displayWidth / 2), CV_8UC2,
              base + ySize, static_cast<std::size_t>(absPitch));
            cv::cvtColorTwoPlane(y, uv, image, cv::COLOR_YUV2BGR_NV12);
            converted = true;
          }
        }

        if (locked2d) buffer2d->Unlock2D();
        else if (base) buffer->Unlock();
        safeRelease(buffer2d);
        buffer->Release();
      }

      if (!converted) return false;
      result = 1;
    }
  };

  hr = decoder->ProcessInput(0, inputSample, 0);
  if (hr == MF_E_NOTACCEPTING)
  {
    // 出力が溜まっていれば取り出してから入力し直す
    if (drain()) hr = decoder->ProcessInput(0, inputSample, 0);
  }
  safeRelease(inputSample);
  if (FAILED(hr)) return -1;

  if (!drain()) return -1;
  return result;
}

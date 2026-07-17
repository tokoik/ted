///
/// Microsoft Media Foundation を使ったビデオキャプチャクラスの実装 (ステレオ対応版)
///
/// @file
/// @author Kohe Tokoi
/// @date December 24, 2024
///
#include "CamMf.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <codecapi.h>
#include <algorithm>

// Microsoft Media Foundation
#pragma comment(lib, "MF.lib")
#pragma comment(lib, "MFplat.lib")
#pragma comment(lib, "MFuuid.lib")
#pragma comment(lib, "MFreadwrite.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")

// COM ライブラリの初期化と終了を行うオブジェクト
CamMf::ComInitializer CamMf::ComInitializer::instance;

//
// メモリの解放
//
template <class T> void SafeRelease(T** ppT)
{
  if (*ppT)
  {
    (*ppT)->Release();
    *ppT = nullptr;
  }
}

//
// GUID から人間が読める形式の名前を返すヘルパー関数
//
static std::string SubTypeToName(const GUID& subType)
{
  if (subType == MFVideoFormat_YUY2) return "YUY2";
  if (subType == MFVideoFormat_NV12) return "NV12";
  if (subType == MFVideoFormat_MJPG) return "MJPG";
  if (subType == MFVideoFormat_H264) return "H264";
  if (subType == MFVideoFormat_RGB32) return "RGB32";
  return "";
}

//
// COM ライブラリの初期化と終了を行うクラスのコンストラクタ
//
CamMf::ComInitializer::ComInitializer()
  : deviceList{}
  , ppSourceActivate{ nullptr }
  , cSourceActivate{ 0 }
  , coInitialized{ false }
  , mfStarted{ false }
{
}

//
// COM ライブラリの初期化と終了を行うクラスのデストラクタ
//
CamMf::ComInitializer::~ComInitializer()
{
  if (ppSourceActivate)
  {
    for (DWORD i = 0; i < cSourceActivate; ++i) SafeRelease(&ppSourceActivate[i]);
    CoTaskMemFree(ppSourceActivate);
    ppSourceActivate = nullptr;
    deviceList.clear();
  }

  if (mfStarted) MFShutdown();
  if (coInitialized) CoUninitialize();
}

//
// 初期化
//
const char* CamMf::ComInitializer::initialize()
{
  const char* message{ nullptr };
  IMFAttributes* pAttributes{ nullptr };

  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (FAILED(hr))
  {
    if (hr == RPC_E_CHANGED_MODE)
    {
      coInitialized = false;
    }
    else
    {
      message = "Failed to initialize COM library.";
      goto done;
    }
  }
  else
  {
    coInitialized = true;
  }

  if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_FULL)))
  {
    message = "Failed to start Media Foundation.";
    goto done;
  }
  mfStarted = true;

  if (FAILED(MFCreateAttributes(&pAttributes, 1)))
  {
    message = "Failed to create attribute store.";
    goto done;
  }

  if (FAILED(pAttributes->SetGUID(
    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID)))
  {
    message = "Failed to set attribute for video capture device.";
    goto done;
  }

  if (FAILED(MFEnumDeviceSources(pAttributes, &ppSourceActivate, &cSourceActivate)))
  {
    message = "Failed to enumerate media sources.";
    goto done;
  }

  for (DWORD i = 0; i < cSourceActivate; ++i)
  {
    WCHAR* szFriendlyName{ nullptr };
    UINT32 cFriendlyName{ 0 };

    if (SUCCEEDED(ppSourceActivate[i]->GetAllocatedString(
      MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyName, &cFriendlyName)))
    {
      std::stringstream ss;
      ss << TCharToUtf8(szFriendlyName) << "##" << i;
      deviceList.emplace_back(ss.str());
    }
    CoTaskMemFree(szFriendlyName);
  }

done:
  SafeRelease(&pAttributes);
  return message;
}

//
// COM ライブラリのシングルトンインスタンスを返す
//
const CamMf::ComInitializer& CamMf::ComInitializer::getInstance()
{
  if (!instance.ppSourceActivate)
  {
    auto message{ instance.initialize() };
    if (message) throw std::runtime_error(message);
  }
  return instance;
}

//
// 有効化
//
bool CamMf::ComInitializer::activate(int device, IMFMediaSource** pMediaSource)
{
  return device >= 0 && static_cast<UINT32>(device) < instance.cSourceActivate
    && SUCCEEDED(instance.ppSourceActivate[device]->ActivateObject(IID_PPV_ARGS(pMediaSource)))
    && pMediaSource;
}

//
// ビデオキャプチャデバイスの表示名のリストを返す
//
const std::vector<std::string>& CamMf::ComInitializer::getDeviceList()
{
  return getInstance().deviceList;
}

//
// デバイスを完全に開くことなくフォーマットを一時列挙する
//
bool CamMf::getDeviceFormats(int device, std::vector<VideoFormat>& formats)
{
  formats.clear();
  IMFMediaSource* pSource = nullptr;
  if (!ComInitializer::activate(device, &pSource)) return false;

  IMFAttributes* pAttributes = nullptr;
  if (FAILED(MFCreateAttributes(&pAttributes, 1)))
  {
    SafeRelease(&pSource);
    return false;
  }

  pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
  pAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, FALSE);
  pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, FALSE);
  pAttributes->SetUINT32(MF_LOW_LATENCY, TRUE);
  pAttributes->SetUINT32(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN, TRUE);

  IMFSourceReader* pReader = nullptr;
  if (FAILED(MFCreateSourceReaderFromMediaSource(pSource, pAttributes, &pReader)))
  {
    SafeRelease(&pAttributes);
    SafeRelease(&pSource);
    return false;
  }
  SafeRelease(&pAttributes);

  IMFMediaType* pMediaType = nullptr;
  for (DWORD dwMediaTypeIndex = 0;
    SUCCEEDED(pReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, dwMediaTypeIndex, &pMediaType));
    ++dwMediaTypeIndex)
  {
    struct MediaTypeReleaser { IMFMediaType*& p; ~MediaTypeReleaser() { SafeRelease(&p); } } releaser{ pMediaType };
    GUID majorType{};
    if (FAILED(pMediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType)) || majorType != MFMediaType_Video) continue;

    GUID subType{};
    if (FAILED(pMediaType->GetGUID(MF_MT_SUBTYPE, &subType))) continue;

    UINT32 width = 0, height = 0;
    if (FAILED(MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height))) continue;

    UINT32 numerator = 0, denominator = 0;
    if (FAILED(MFGetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, &numerator, &denominator))) continue;

    const auto& codecName = SubTypeToName(subType);
    if (codecName.empty() || denominator == 0 || numerator == 0) continue;

    formats.emplace_back(width, height, numerator, denominator, subType);
  }

  SafeRelease(&pReader);
  SafeRelease(&pSource);
  return !formats.empty();
}

//
// 使用可能な解像度、フレームレート、コーデックのリストを作成する
//
bool CamMf::enumerateFormats(int cam)
{
  if (!caps[cam].pSourceReader) return false;

  caps[cam].availableFormats.clear();
  caps[cam].formatList.clear();
  caps[cam].codecList.clear();
  caps[cam].resolutionList.clear();

  IMFMediaType* pMediaType{ nullptr };
  for (DWORD dwMediaTypeIndex = 0;
    SUCCEEDED(caps[cam].pSourceReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
      dwMediaTypeIndex, &pMediaType));
    ++dwMediaTypeIndex)
  {
    struct MediaTypeReleaser { IMFMediaType*& p; ~MediaTypeReleaser() { SafeRelease(&p); } } releaser{ pMediaType };

    GUID majorType{};
    if (FAILED(pMediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType))) continue;
    if (majorType != MFMediaType_Video) continue;

    GUID subType{};
    if (FAILED(pMediaType->GetGUID(MF_MT_SUBTYPE, &subType))) continue;

    UINT32 width{ 0 }, height{ 0 };
    if (FAILED(MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height))) continue;

    UINT32 numerator{ 0 }, denominator{ 0 };
    if (FAILED(MFGetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, &numerator, &denominator))) continue;

    const auto& codecName{ SubTypeToName(subType) };
    if (codecName.empty() || denominator == 0 || numerator == 0) continue;

    const double fps{ static_cast<double>(numerator) / static_cast<double>(denominator) };
    if (fps < 5.0) continue;

    std::stringstream ss;
    ss << width << " x " << height << " @ "
      << std::fixed << std::setprecision(2) << fps
      << " fps (" << codecName << ")##" << dwMediaTypeIndex;

    caps[cam].formatList.emplace_back(ss.str());
    caps[cam].availableFormats.emplace_back(width, height, numerator, denominator, subType);

    // 重複排除したコーデックリストの作成
    if (std::find(caps[cam].codecList.begin(), caps[cam].codecList.end(), codecName) == caps[cam].codecList.end())
    {
      caps[cam].codecList.push_back(codecName);
    }
  }

  return !caps[cam].formatList.empty();
}

//
// 指定されたサブタイプに対応するビデオデコーダを探す
//
HRESULT CamMf::findVideoDecoder(
  const GUID& subtype,
  IMFTransform** ppDecoder,
  BOOL bAllowAsync,
  BOOL bAllowHardware,
  BOOL bAllowTranscode
) const
{
  HRESULT hr{ S_OK };
  UINT32 count{ 0 };
  IMFActivate** ppActivate{ nullptr };

  MFT_REGISTER_TYPE_INFO inputInfo{ MFMediaType_Video, subtype };
  MFT_REGISTER_TYPE_INFO outputInfo{ MFMediaType_Video, MFVideoFormat_NV12 };

  UINT32 unFlags
  {
    MFT_ENUM_FLAG_SYNCMFT
    | MFT_ENUM_FLAG_LOCALMFT
    | MFT_ENUM_FLAG_SORTANDFILTER
  };

  if (bAllowAsync) unFlags |= MFT_ENUM_FLAG_ASYNCMFT;
  if (bAllowHardware) unFlags |= MFT_ENUM_FLAG_HARDWARE;
  if (bAllowTranscode) unFlags |= MFT_ENUM_FLAG_TRANSCODE_ONLY;

  hr = MFTEnumEx(MFT_CATEGORY_VIDEO_DECODER, unFlags, &inputInfo, &outputInfo, &ppActivate, &count);

  if (SUCCEEDED(hr) && count == 0) hr = MF_E_TOPO_CODEC_NOT_FOUND;
  if (SUCCEEDED(hr)) hr = ppActivate[0]->ActivateObject(IID_PPV_ARGS(ppDecoder));

  for (UINT32 i = 0; i < count; i++) ppActivate[i]->Release();
  CoTaskMemFree(ppActivate);

  return hr;
}

//
// MFT のセットアップと接続を行う
//
HRESULT CamMf::setUpPipeline(IMFTransform* pTransform, const VideoFormat& format, const GUID& subType) const
{
  IMFMediaType* pInputType{ nullptr };
  IMFMediaType* pOutputType{ nullptr };
  HRESULT hr{ S_OK };

  hr = MFCreateMediaType(&pInputType);
  if (SUCCEEDED(hr)) hr = pInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (SUCCEEDED(hr)) hr = pInputType->SetGUID(MF_MT_SUBTYPE, format.subType);
  if (SUCCEEDED(hr)) hr = MFSetAttributeSize(pInputType, MF_MT_FRAME_SIZE, format.width, format.height);
  if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(pInputType, MF_MT_FRAME_RATE, format.fpsNum, format.fpsDenom);

  if (SUCCEEDED(hr)) hr = pTransform->SetInputType(0, pInputType, 0);
  if (FAILED(hr)) goto done;

  hr = MFCreateMediaType(&pOutputType);
  if (SUCCEEDED(hr)) hr = pOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (SUCCEEDED(hr)) hr = pOutputType->SetGUID(MF_MT_SUBTYPE, subType);
  if (SUCCEEDED(hr)) hr = MFSetAttributeSize(pOutputType, MF_MT_FRAME_SIZE, format.width, format.height);
  if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(pOutputType, MF_MT_FRAME_RATE, format.fpsNum, format.fpsDenom);

  if (SUCCEEDED(hr)) hr = pTransform->SetOutputType(0, pOutputType, 0);
  if (FAILED(hr)) goto done;

  hr = pTransform->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, NULL);
  if (SUCCEEDED(hr)) hr = pTransform->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, NULL);
  if (SUCCEEDED(hr)) hr = pTransform->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, NULL);

done:
  SafeRelease(&pInputType);
  SafeRelease(&pOutputType);
  return hr;
}

//
// MFT を解放する
//
void CamMf::cleanUpTransform(IMFTransform** pTransform) const
{
  if (*pTransform)
  {
    (*pTransform)->ProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING, NULL);
    (*pTransform)->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, NULL);
    (*pTransform)->Release();
    *pTransform = nullptr;
  }
}

//
// デコーダの出力バッファを作成する
//
HRESULT CamMf::createDecoderBuffer(int cam)
{
  MFT_OUTPUT_STREAM_INFO streamInfo{ 0 };
  if (SUCCEEDED(caps[cam].pDecoder->GetOutputStreamInfo(0, &streamInfo))) {}

  const auto alignedWidth{ static_cast<UINT32>((caps[cam].width + 15) & ~15) };
  const auto alignedHeight{ static_cast<UINT32>((caps[cam].height + 15) & ~15) };
  const auto cbDecoderCalc{ static_cast<UINT32>((alignedWidth * alignedHeight * 3) / 2) };
  const auto cbDecoder{ static_cast<UINT32>((streamInfo.cbSize > cbDecoderCalc) ? streamInfo.cbSize : cbDecoderCalc) };
  const auto alignmentDecoder{ static_cast<DWORD>((streamInfo.cbAlignment > 0) ? (streamInfo.cbAlignment - 1) : 63) };

  SafeRelease(&caps[cam].pDecoderBuffer);
  return MFCreateAlignedMemoryBuffer(cbDecoder, alignmentDecoder, &caps[cam].pDecoderBuffer);
}

//
// カラーコンバータの出力バッファを作成する
//
HRESULT CamMf::createConverterBuffer(int cam)
{
  MFT_OUTPUT_STREAM_INFO convStreamInfo{ 0 };
  if (SUCCEEDED(caps[cam].pConverter->GetOutputStreamInfo(0, &convStreamInfo))) {}

  UINT32 cbConverterCalc{ 0 };
  MFCalculateImageSize(MFVideoFormat_RGB32, caps[cam].width, caps[cam].height, &cbConverterCalc);
  UINT32 cbConverter = (convStreamInfo.cbSize > cbConverterCalc) ? convStreamInfo.cbSize : cbConverterCalc;
  DWORD alignmentConverter = (convStreamInfo.cbAlignment > 0) ? (convStreamInfo.cbAlignment - 1) : 63;

  SafeRelease(&caps[cam].pConverterBuffer);
  return MFCreateAlignedMemoryBuffer(cbConverter, alignmentConverter, &caps[cam].pConverterBuffer);
}

//
// Source Reader の出力フォーマットを設定し、基底クラスの image を初期化する
//
bool CamMf::setFormat(int cam, int index)
{
  VideoFormat selectedFormat{ caps[cam].availableFormats[index] };

  enum class DecodeMethod { None = 0, Convert, Decode } decodeMethod
  {
    selectedFormat.subType == MFVideoFormat_MJPG ? DecodeMethod::Decode :
    selectedFormat.subType == MFVideoFormat_H264 ? DecodeMethod::Decode :
    selectedFormat.subType == MFVideoFormat_NV12 ? DecodeMethod::Convert :
    selectedFormat.subType == MFVideoFormat_YUY2 ? DecodeMethod::Convert :
    DecodeMethod::None
  };

  cleanUpTransform(&caps[cam].pDecoder);
  cleanUpTransform(&caps[cam].pConverter);

  HRESULT hr{ S_OK };
  IMFMediaType* pMediaType{ nullptr };
  hr = MFCreateMediaType(&pMediaType);
  if (FAILED(hr)) goto done;

  hr = pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (FAILED(hr)) goto done;

  hr = pMediaType->SetGUID(MF_MT_SUBTYPE, selectedFormat.subType);
  if (FAILED(hr)) goto done;

  hr = MFSetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, selectedFormat.width, selectedFormat.height);
  if (FAILED(hr)) goto done;

  hr = MFSetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, selectedFormat.fpsNum, selectedFormat.fpsDenom);
  if (FAILED(hr)) goto done;

  hr = caps[cam].pSourceReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, pMediaType);
  if (FAILED(hr)) goto done;

  caps[cam].width = selectedFormat.width;
  caps[cam].height = selectedFormat.height;

  // ted の Camera 基底クラスのメンバ image[cam] をアロケートする (BGR)
  image[cam] = cv::Mat::zeros(caps[cam].height, caps[cam].width, CV_8UC3);

  // interval を計算して設定
  interval[cam] = (selectedFormat.fpsDenom != 0)
    ? (static_cast<double>(selectedFormat.fpsDenom) / selectedFormat.fpsNum)
    : (1.0 / 30.0);

  if (decodeMethod != DecodeMethod::None)
  {
    if (decodeMethod == DecodeMethod::Decode)
    {
      hr = findVideoDecoder(selectedFormat.subType, &caps[cam].pDecoder);
      if (FAILED(hr)) goto done;

      ICodecAPI* pCodecAPI{ nullptr };
      if (SUCCEEDED(caps[cam].pDecoder->QueryInterface(IID_PPV_ARGS(&pCodecAPI))))
      {
        VARIANT var;
        VariantInit(&var);
        var.vt = VT_UI4;
        var.ulVal = 1;
        hr = pCodecAPI->SetValue(&CODECAPI_AVLowLatencyMode, &var);

        if (FAILED(hr))
        {
          var.vt = VT_BOOL;
          var.boolVal = VARIANT_TRUE;
          pCodecAPI->SetValue(&CODECAPI_AVLowLatencyMode, &var);
        }
        VariantClear(&var);
        SafeRelease(&pCodecAPI);
      }

      hr = setUpPipeline(caps[cam].pDecoder, selectedFormat, MFVideoFormat_NV12);
      if (FAILED(hr)) goto done;

      selectedFormat.subType = MFVideoFormat_NV12;
      hr = createDecoderBuffer(cam);
      if (FAILED(hr)) goto done;
    }

    hr = CoCreateInstance(CLSID_CColorConvertDMO, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&caps[cam].pConverter));
    if (FAILED(hr)) goto done;

    hr = setUpPipeline(caps[cam].pConverter, selectedFormat, MFVideoFormat_RGB32);
    if (FAILED(hr)) goto done;

    hr = createConverterBuffer(cam);
    if (FAILED(hr)) goto done;
  }

done:
  SafeRelease(&pMediaType);

  if (SUCCEEDED(hr)) return true;

  close(cam);
  return false;
}

//
// カメラを開く
//
bool CamMf::open(int device, int cam, bool setupFormat)
{
  if (!ComInitializer::activate(device, &caps[cam].pMediaSource)) return false;

  HRESULT hr{ S_OK };
  IMFAttributes* pAttributes{ nullptr };
  hr = MFCreateAttributes(&pAttributes, 1);
  if (FAILED(hr)) goto done;

  pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
  pAttributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, FALSE);
  pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, FALSE);
  pAttributes->SetUINT32(MF_LOW_LATENCY, TRUE);
  pAttributes->SetUINT32(MF_SOURCE_READER_DISCONNECT_MEDIASOURCE_ON_SHUTDOWN, TRUE);

  hr = MFCreateSourceReaderFromMediaSource(caps[cam].pMediaSource, pAttributes, &caps[cam].pSourceReader);
  if (FAILED(hr)) goto done;
  SafeRelease(&pAttributes);

  if (enumerateFormats(cam))
  {
    if (!setupFormat) return true;

    // デフォルトでフォーマットを自動決定する
    // config の希望する解像度とコーデックに合致するものを優先して選択する
    int selectedIdx = 0;
    double maxFps = 0.0;
    
    int reqWidth = 1280, reqHeight = 720;
    sscanf_s(defaults.camera_resolution[cam].c_str(), "%d x %d", &reqWidth, &reqHeight);
    std::string reqCodec = defaults.camera_codec[cam];

    for (int i = 0; i < caps[cam].availableFormats.size(); ++i)
    {
      const auto& fmt = caps[cam].availableFormats[i];
      if (SubTypeToName(fmt.subType) == reqCodec && fmt.width == reqWidth && fmt.height == reqHeight)
      {
        double fps = static_cast<double>(fmt.fpsNum) / fmt.fpsDenom;
        if (fps > maxFps)
        {
          maxFps = fps;
          selectedIdx = i;
        }
      }
    }

    if (setFormat(cam, selectedIdx))
    {
      // スレッドを起動
      run[cam] = true;
      captureThread[cam] = std::thread([this, cam]() { capture(cam); });
      return true;
    }
  }

done:
  SafeRelease(&caps[cam].pSourceReader);
  SafeRelease(&pAttributes);
  caps[cam].availableFormats.clear();
  caps[cam].formatList.clear();
  caps[cam].codecList.clear();
  caps[cam].resolutionList.clear();

  return false;
}

//
// カメラが使用可能か判定する
//
bool CamMf::opened(int cam) const
{
  return caps[cam].pSourceReader != nullptr;
}

//
// 指定されたカメラを閉じる
//
void CamMf::close(int cam)
{
  // キャプチャスレッドが実行中なら停止
  if (run[cam])
  {
    run[cam] = false;
    if (caps[cam].pSourceReader)
    {
      caps[cam].pSourceReader->Flush(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
    }
    if (captureThread[cam].joinable())
    {
      captureThread[cam].join();
    }
  }

  cleanUpTransform(&caps[cam].pDecoder);
  cleanUpTransform(&caps[cam].pConverter);
  SafeRelease(&caps[cam].pDecoderBuffer);
  SafeRelease(&caps[cam].pConverterBuffer);
  SafeRelease(&caps[cam].pSourceReader);
  SafeRelease(&caps[cam].pMediaSource);

  caps[cam].availableFormats.clear();
  caps[cam].formatList.clear();
  caps[cam].codecList.clear();
  caps[cam].resolutionList.clear();

  captured[cam] = false;
}

//
// 全てのカメラを閉じる
//
void CamMf::close()
{
  for (int cam = 0; cam < camCount; ++cam)
  {
    close(cam);
  }
}

//
// キャプチャスレッドを停止する
//
void CamMf::stop()
{
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (run[cam])
    {
      if (caps[cam].pSourceReader)
      {
        caps[cam].pSourceReader->Flush(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
      }
    }
  }
  Camera::stop();
}

//
// フォーマットを選択して設定する
//
bool CamMf::select(int cam, int index)
{
  if (!caps[cam].pSourceReader || index < 0 || index >= caps[cam].availableFormats.size()) return false;

  // スレッド停止
  bool wasRunning = run[cam];
  if (wasRunning)
  {
    run[cam] = false;
    caps[cam].pSourceReader->Flush(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
    if (captureThread[cam].joinable()) captureThread[cam].join();
  }

  bool success = setFormat(cam, index);

  if (success && wasRunning)
  {
    run[cam] = true;
    captureThread[cam] = std::thread([this, cam]() { capture(cam); });
  }

  return success;
}

//
// コーデックと解像度のコンボインデックスを指定してフォーマットを設定する
//
bool CamMf::selectFormat(int cam, int codecIdx, int resIdx)
{
  if (!caps[cam].pSourceReader) return false;
  if (codecIdx < 0 || codecIdx >= caps[cam].codecList.size()) return false;

  std::string targetCodec = caps[cam].codecList[codecIdx];

  // 選択された解像度文字列の取得
  std::string targetResStr = "";
  // 選択されたコーデックに対する解像度リストを一時的に構築する
  std::vector<std::string> tempResolutions;
  for (const auto& fmt : caps[cam].availableFormats)
  {
    if (SubTypeToName(fmt.subType) == targetCodec)
    {
      char buf[32];
      sprintf_s(buf, "%d x %d", fmt.width, fmt.height);
      std::string resStr(buf);
      if (std::find(tempResolutions.begin(), tempResolutions.end(), resStr) == tempResolutions.end())
      {
        tempResolutions.push_back(resStr);
      }
    }
  }

  if (resIdx < 0 || resIdx >= tempResolutions.size()) return false;
  targetResStr = tempResolutions[resIdx];

  int reqWidth = 0, reqHeight = 0;
  sscanf_s(targetResStr.c_str(), "%d x %d", &reqWidth, &reqHeight);

  // 一致するフォーマットの中で最高FPSのものを探す
  int foundIdx = -1;
  double maxFps = 0.0;
  for (int i = 0; i < caps[cam].availableFormats.size(); ++i)
  {
    const auto& fmt = caps[cam].availableFormats[i];
    if (SubTypeToName(fmt.subType) == targetCodec && fmt.width == reqWidth && fmt.height == reqHeight)
    {
      double fps = static_cast<double>(fmt.fpsNum) / fmt.fpsDenom;
      if (fps > maxFps)
      {
        maxFps = fps;
        foundIdx = i;
      }
    }
  }

  if (foundIdx != -1)
  {
    // config を更新
    defaults.camera_codec[cam] = targetCodec;
    defaults.camera_resolution[cam] = targetResStr;
    caps[cam].selectedCodecIndex = codecIdx;
    caps[cam].selectedResolutionIndex = resIdx;

    return select(cam, foundIdx);
  }

  return false;
}

//
// キャプチャ処理ループ
//
void CamMf::capture(int cam)
{
  while (run[cam])
  {
    DWORD dwStreamIndex{ 0 };
    DWORD dwStreamFlags{ 0 };
    LONGLONG llTimestamp{ 0 };
    IMFSample* pSample{ nullptr };

    if (FAILED(caps[cam].pSourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
      0, &dwStreamIndex, &dwStreamFlags, &llTimestamp, &pSample))) continue;

    if (!pSample) continue;

    IMFMediaBuffer* pBuffer{ nullptr };
    IMFSample* pDecodedSample{ nullptr };
    IMFSample* pConvertedSample{ nullptr };
    DWORD dwStatus{ 0 };

    if (caps[cam].pDecoder)
    {
      HRESULT hr{ caps[cam].pDecoder->ProcessInput(0, pSample, 0) };
      if (FAILED(hr)) goto done;

      MFT_OUTPUT_STREAM_INFO streamInfo{};
      bool mftProvidesSamples{ false };
      if (SUCCEEDED(caps[cam].pDecoder->GetOutputStreamInfo(0, &streamInfo)))
      {
        mftProvidesSamples = (streamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES) != 0;
      }

      IMFSample* pLatestDecodedSample{ nullptr };
      bool hasOutput{ false };

      while (true)
      {
        DWORD prevLength = 0;
        
        if (mftProvidesSamples)
        {
          pDecodedSample = nullptr;
        }
        else
        {
          hr = MFCreateSample(&pDecodedSample);
          if (FAILED(hr)) goto done;

          if (!caps[cam].pDecoderBuffer)
          {
            hr = createDecoderBuffer(cam);
            if (FAILED(hr))
            {
              SafeRelease(&pDecodedSample);
              goto done;
            }
          }

          caps[cam].pDecoderBuffer->GetCurrentLength(&prevLength);
          caps[cam].pDecoderBuffer->SetCurrentLength(0);

          hr = pDecodedSample->AddBuffer(caps[cam].pDecoderBuffer);
          if (FAILED(hr))
          {
            SafeRelease(&pDecodedSample);
            goto done;
          }
        }

        MFT_OUTPUT_DATA_BUFFER decodedBuffer{ 0, pDecodedSample, 0, nullptr };
        hr = caps[cam].pDecoder->ProcessOutput(0, 1, &decodedBuffer, &dwStatus);

        if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
        {
          IMFMediaType* pNewOutputType{ nullptr };
          GUID subtype{ 0 };
          DWORD dwTypeIndex = 0;
          bool foundNV12 = false;
          hr = S_OK;

          while (SUCCEEDED(caps[cam].pDecoder->GetOutputAvailableType(0, dwTypeIndex++, &pNewOutputType)))
          {
            GUID subType{};
            if (SUCCEEDED(pNewOutputType->GetGUID(MF_MT_SUBTYPE, &subType)) && subType == MFVideoFormat_NV12)
            {
              foundNV12 = true;
              subtype = subType;
              break;
            }
            SafeRelease(&pNewOutputType);
          }

          if (!foundNV12)
          {
            hr = caps[cam].pDecoder->GetOutputAvailableType(0, 0, &pNewOutputType);
            if (SUCCEEDED(hr))
            {
              hr = pNewOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
              subtype = MFVideoFormat_NV12;
            }
          }

          if (SUCCEEDED(hr) && pNewOutputType)
          {
            IMFMediaType* pInputType{ nullptr };
            if (SUCCEEDED(caps[cam].pDecoder->GetInputCurrentType(0, &pInputType)))
            {
              UINT w{ 0 }, h{ 0 };
              if (SUCCEEDED(MFGetAttributeSize(pInputType, MF_MT_FRAME_SIZE, &w, &h)))
              {
                MFSetAttributeSize(pNewOutputType, MF_MT_FRAME_SIZE, w, h);
              }

              UINT fpsNum{ 0 }, fpsDenom{ 0 };
              if (SUCCEEDED(MFGetAttributeRatio(pInputType, MF_MT_FRAME_RATE, &fpsNum, &fpsDenom)))
              {
                MFSetAttributeRatio(pNewOutputType, MF_MT_FRAME_RATE, fpsNum, fpsDenom);
              }

              UINT aspectNum{ 0 }, aspectDenom{ 0 };
              if (SUCCEEDED(MFGetAttributeRatio(pInputType, MF_MT_PIXEL_ASPECT_RATIO, &aspectNum, &aspectDenom)))
              {
                MFSetAttributeRatio(pNewOutputType, MF_MT_PIXEL_ASPECT_RATIO, aspectNum, aspectDenom);
              }

              PROPVARIANT var;
              PropVariantInit(&var);
              if (SUCCEEDED(pInputType->GetItem(MF_MT_INTERLACE_MODE, &var)))
              {
                if (var.vt == VT_UI4) pNewOutputType->SetUINT32(MF_MT_INTERLACE_MODE, var.ulVal);
                PropVariantClear(&var);
              }
              SafeRelease(&pInputType);
            }

            hr = caps[cam].pDecoder->SetOutputType(0, pNewOutputType, 0);
          }

          if (SUCCEEDED(hr) && pNewOutputType)
          {
            UINT32 newWidth{ 0 }, newHeight{ 0 };
            if (SUCCEEDED(MFGetAttributeSize(pNewOutputType, MF_MT_FRAME_SIZE, &newWidth, &newHeight)) && newWidth > 0 && newHeight > 0)
            {
              caps[cam].width = newWidth;
              caps[cam].height = newHeight;
            }

            UINT32 fpsNum{ 0 }, fpsDenom{ 0 };
            MFGetAttributeRatio(pNewOutputType, MF_MT_FRAME_RATE, &fpsNum, &fpsDenom);

            hr = createDecoderBuffer(cam);

            if (SUCCEEDED(hr) && caps[cam].pConverter)
            {
              VideoFormat newFormat{ static_cast<UINT32>(caps[cam].width), static_cast<UINT32>(caps[cam].height), fpsNum, fpsDenom, subtype };
              hr = setUpPipeline(caps[cam].pConverter, newFormat, MFVideoFormat_RGB32);
              if (SUCCEEDED(hr))
              {
                hr = createConverterBuffer(cam);
              }
            }

            if (SUCCEEDED(hr))
            {
              // BGR cv::Mat を再割り当て
              captureMutex[cam].lock();
              image[cam] = cv::Mat::zeros(caps[cam].height, caps[cam].width, CV_8UC3);
              captureMutex[cam].unlock();
            }
          }

          if (!mftProvidesSamples) SafeRelease(&pDecodedSample);
          else SafeRelease(&decodedBuffer.pSample);
          SafeRelease(&decodedBuffer.pEvents);
          SafeRelease(&pNewOutputType);

          if (FAILED(hr)) goto done;

          if (SUCCEEDED(caps[cam].pDecoder->GetOutputStreamInfo(0, &streamInfo)))
          {
            mftProvidesSamples = (streamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES) != 0;
          }

          if (!mftProvidesSamples && caps[cam].pDecoderBuffer)
          {
            caps[cam].pDecoderBuffer->SetCurrentLength(prevLength);
          }
          continue;
        }

        if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
        {
          if (!mftProvidesSamples)
          {
            if (caps[cam].pDecoderBuffer) caps[cam].pDecoderBuffer->SetCurrentLength(prevLength);
            SafeRelease(&pDecodedSample);
          }
          else
          {
            SafeRelease(&decodedBuffer.pSample);
          }
          break;
        }

        if (FAILED(hr))
        {
          if (!mftProvidesSamples)
          {
            if (caps[cam].pDecoderBuffer) caps[cam].pDecoderBuffer->SetCurrentLength(prevLength);
            SafeRelease(&pDecodedSample);
          }
          else
          {
            SafeRelease(&decodedBuffer.pSample);
          }
          SafeRelease(&decodedBuffer.pEvents);
          goto done;
        }

        SafeRelease(&pLatestDecodedSample);
        if (mftProvidesSamples) pLatestDecodedSample = decodedBuffer.pSample;
        else pLatestDecodedSample = pDecodedSample;

        pDecodedSample = nullptr;
        SafeRelease(&decodedBuffer.pEvents);
        hasOutput = true;
        
        // ted の低遅延を考慮
        break; 
      }

      if (hasOutput && pLatestDecodedSample)
      {
        pSample->Release();
        pSample = pLatestDecodedSample;
        pLatestDecodedSample = nullptr;
      }
      else
      {
        pSample->Release();
        pSample = nullptr;
        continue;
      }
    }

    if (caps[cam].pConverter)
    {
      HRESULT hr{ caps[cam].pConverter->ProcessInput(0, pSample, 0) };
      if (FAILED(hr)) goto done;

      hr = MFCreateSample(&pConvertedSample);
      if (FAILED(hr)) goto done;

      caps[cam].pConverterBuffer->SetCurrentLength(0);
      hr = pConvertedSample->AddBuffer(caps[cam].pConverterBuffer);
      if (FAILED(hr)) goto done;

      MFT_OUTPUT_DATA_BUFFER convertedBuffer{ 0, pConvertedSample, 0, nullptr };
      hr = caps[cam].pConverter->ProcessOutput(0, 1, &convertedBuffer, &dwStatus);
      if (FAILED(hr)) goto done;

      pSample->Release();
      pSample = pConvertedSample;
      pConvertedSample = nullptr;
    }

    if (FAILED(pSample->GetBufferByIndex(0, &pBuffer))) goto done;

    if (pBuffer)
    {
      BYTE* pData{ nullptr };
      DWORD cbDataLength{ 0 };

      if (SUCCEEDED(pBuffer->Lock(&pData, nullptr, &cbDataLength)) && pData)
      {
        captureMutex[cam].lock();

        // BGRA (4チャンネル) を BGR (3チャンネル) に変換して image[cam] にコピー
        cv::Mat temp(caps[cam].height, caps[cam].width, CV_8UC4, pData);
        cv::cvtColor(temp, image[cam], cv::COLOR_BGRA2BGR);

        captured[cam] = true;
        unsent[cam] = true;

        captureMutex[cam].unlock();
        pBuffer->Unlock();
      }
      pBuffer->Release();
      pBuffer = nullptr;
    }

  done:
    if (pDecodedSample) pDecodedSample->Release();
    if (pConvertedSample) pConvertedSample->Release();
    if (pSample) pSample->Release();
  }
}

CamMf::CamMf()
{
  for (int cam = 0; cam < camCount; ++cam)
  {
    caps[cam].pMediaSource = nullptr;
    caps[cam].pSourceReader = nullptr;
    caps[cam].pDecoder = nullptr;
    caps[cam].pDecoderBuffer = nullptr;
    caps[cam].pConverter = nullptr;
    caps[cam].pConverterBuffer = nullptr;
    caps[cam].selectedCodecIndex = -1;
    caps[cam].selectedResolutionIndex = -1;
  }
}

CamMf::~CamMf()
{
  close();
}

//
// ファイル／ネットワークからキャプチャを開始する
//
bool CamMf::open(const std::string& file, int cam)
{
  int wlen = MultiByteToWideChar(CP_UTF8, 0, file.c_str(), -1, nullptr, 0);
  if (wlen <= 0) return false;
  std::vector<wchar_t> wbuf(wlen);
  MultiByteToWideChar(CP_UTF8, 0, file.c_str(), -1, wbuf.data(), wlen);

  HRESULT hr = MFCreateSourceReaderFromURL(wbuf.data(), nullptr, &caps[cam].pSourceReader);
  if (FAILED(hr)) return false;

  if (enumerateFormats(cam))
  {
    if (setFormat(cam, 0))
    {
      run[cam] = true;
      captureThread[cam] = std::thread([this, cam]() { capture(cam); });
      return true;
    }
  }

  SafeRelease(&caps[cam].pSourceReader);
  return false;
}



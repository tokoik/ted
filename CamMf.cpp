///
/// Microsoft Media Foundation を使ったビデオキャプチャクラスの実装 (ステレオ対応版)
///
/// @file
/// @author Kohe Tokoi
/// @date December 24, 2024
///
#include "CamMf.h"

// バックエンド非依存のカメラ入力形式
#include "CameraCapabilities.h"

// 標準ライブラリ
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <limits>

// Microsoft Media Foundation
#include <codecapi.h>
#pragma comment(lib, "MF.lib")
#pragma comment(lib, "MFplat.lib")
#pragma comment(lib, "MFuuid.lib")
#pragma comment(lib, "MFreadwrite.lib")
#pragma comment(lib, "wmcodecdspuuid.lib")

// COM ライブラリの初期化と終了を行うオブジェクト
CamMf::ComInitializer CamMf::ComInitializer::instance;

//
// COM インターフェースを解放し、解放済みポインタの再利用を防ぐため nullptr に戻す
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
// COM ライブラリの初期化と終了を行うクラスのデストラクタ
//
CamMf::ComInitializer::~ComInitializer()
{
  // 列挙時に Media Foundation から受け取った Activate オブジェクトと配列をまとめて解放する
  if (ppSourceActivate)
  {
    for (DWORD i = 0; i < cSourceActivate; ++i) SafeRelease(&ppSourceActivate[i]);
    CoTaskMemFree(ppSourceActivate);
    ppSourceActivate = nullptr;
    deviceList.clear();
  }

  // 初期化に成功したサービスだけを、開始時とは逆の順序で終了する
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

  // Source Reader と MFT をキャプチャスレッドから利用できるよう、COM を MTA で初期化する
  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (FAILED(hr))
  {
    if (hr == RPC_E_CHANGED_MODE)
    {
      // 呼び出し元が別のアパートメント方式で COM を初期化済みでも、その COM 環境を借用して続行する
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

  // デバイス列挙や Source Reader の生成に先立って Media Foundation 全体を起動する
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

  // MFEnumDeviceSources の検索対象をビデオキャプチャデバイスだけに限定する
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

  // UI で同名デバイスを区別できるよう、表示名に列挙インデックスを ImGui の隠し ID として付加する
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
  // 初回アクセス時だけ列挙を行い、以後は同じ Activate オブジェクトと表示名一覧を共有する
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
  // UI のデバイス番号を検証してから、対応する物理カメラを Media Source として実体化する
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
  // 問い合わせ結果を呼び出しごとに作り直し、失敗時に古い形式が残らないようにする
  formats.clear();
  IMFMediaSource* pSource = nullptr;
  if (!ComInitializer::activate(device, &pSource)) return false;

  IMFAttributes* pAttributes = nullptr;
  if (FAILED(MFCreateAttributes(&pAttributes, 1)))
  {
    SafeRelease(&pSource);
    return false;
  }

  // 形式列挙では変換処理を実行しないが、実際の open() と同じ低遅延条件で Source Reader を作る
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

  // Source Reader が公開するネイティブ形式から、アプリが扱える映像形式だけを抽出する
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

    UINT32 width{ 0 }, height{ 0 };
    if (FAILED(MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height))) continue;

    UINT32 numerator{ 0 }, denominator{ 0 };
    if (FAILED(MFGetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, &numerator, &denominator))) continue;

    // 未対応コーデックや計算不能なフレームレートは UI の選択肢に含めない
    const auto& codecName = SubTypeToName(subType);
    if (codecName.empty() || denominator == 0 || numerator == 0) continue;

    formats.emplace_back(width, height, numerator, denominator, subType);
  }

  SafeRelease(&pReader);
  SafeRelease(&pSource);
  return !formats.empty();
}

//
// UIや設定処理へバックエンド非依存のカメラ情報を提供する
//
const std::vector<std::string>& CameraCapabilities::getDeviceList()
{
  return CamMf::getDeviceList();
}

bool CameraCapabilities::getCapabilities(int device,
  std::vector<CaptureCapability>& capabilities)
{
  // Media Foundation 固有の GUID と分数表現を、上位層が扱える文字列と実数値へ変換する
  std::vector<CamMf::VideoFormat> formats;
  if (!CamMf::getDeviceFormats(device, formats))
  {
    capabilities.clear();
    return false;
  }

  capabilities.clear();
  capabilities.reserve(formats.size());
  for (const auto& format : formats)
  {
    capabilities.push_back(CaptureCapability
      {
        SubTypeToName(format.subType),
        static_cast<int>(format.width),
        static_cast<int>(format.height),
        static_cast<double>(format.fpsNum) / static_cast<double>(format.fpsDenom)
      });
  }

  return !capabilities.empty();
}

//
// 使用可能な解像度、フレームレート、コーデックのリストを作成する
//
bool CamMf::enumerateFormats(int cam)
{
  if (!caps[cam].pSourceReader) return false;

  // 再オープン時に以前のカメラの選択肢が混在しないよう、関連する一覧をすべて再構築する
  caps[cam].availableFormats.clear();
  caps[cam].formatList.clear();
  caps[cam].codecList.clear();
  caps[cam].resolutionList.clear();

  // ネイティブ形式のインデックス順を保ったまま、内部選択用データと UI 表示文字列を対で保存する
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

    // 実用上キャプチャに向かない低フレームレート形式を候補から除外する
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

  // カメラの圧縮形式を受け取り、後段の色変換器が扱える NV12 を出力するデコーダを検索する
  MFT_REGISTER_TYPE_INFO inputInfo{ MFMediaType_Video, subtype };
  MFT_REGISTER_TYPE_INFO outputInfo{ MFMediaType_Video, MFVideoFormat_NV12 };

  // まず同期・ローカル MFT を対象とし、呼び出し側の許可に応じて非同期・ハードウェア実装も候補に加える
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

  // 優先順位付きの列挙結果の先頭を採用し、Activate 配列は成否にかかわらずここで解放する
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

  // 入力側には直前の処理段が出力する形式・寸法・レートをそのまま通知する
  hr = MFCreateMediaType(&pInputType);
  if (SUCCEEDED(hr)) hr = pInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (SUCCEEDED(hr)) hr = pInputType->SetGUID(MF_MT_SUBTYPE, format.subType);
  if (SUCCEEDED(hr)) hr = MFSetAttributeSize(pInputType, MF_MT_FRAME_SIZE, format.width, format.height);
  if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(pInputType, MF_MT_FRAME_RATE, format.fpsNum, format.fpsDenom);

  if (SUCCEEDED(hr)) hr = pTransform->SetInputType(0, pInputType, 0);
  if (FAILED(hr)) goto done;

  // 出力側は寸法とレートを維持し、ピクセル形式だけを次段が要求する形式へ変える
  hr = MFCreateMediaType(&pOutputType);
  if (SUCCEEDED(hr)) hr = pOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (SUCCEEDED(hr)) hr = pOutputType->SetGUID(MF_MT_SUBTYPE, subType);
  if (SUCCEEDED(hr)) hr = MFSetAttributeSize(pOutputType, MF_MT_FRAME_SIZE, format.width, format.height);
  if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(pOutputType, MF_MT_FRAME_RATE, format.fpsNum, format.fpsDenom);

  if (SUCCEEDED(hr)) hr = pTransform->SetOutputType(0, pOutputType, 0);
  if (FAILED(hr)) goto done;

  // 古い内部状態を捨ててから、新しい形式でストリーム処理を開始させる
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
    // MFT にストリーム終了を通知して内部キューを片付けてから COM オブジェクトを解放する
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

  // NV12 デコーダには16画素境界を前提とする実装があるため、表示寸法を切り上げて必要量を見積もる
  const auto alignedWidth{ static_cast<UINT32>((caps[cam].width + 15) & ~15) };
  const auto alignedHeight{ static_cast<UINT32>((caps[cam].height + 15) & ~15) };
  const auto cbDecoderCalc{ static_cast<UINT32>((alignedWidth * alignedHeight * 3) / 2) };
  // MFT の要求値と計算値の大きい方を使い、実装固有の余白不足による ProcessOutput 失敗を避ける
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

  // RGB32 一画面分の計算値と MFT の要求値を比較し、要求アラインメントで再利用バッファを確保する
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

  // 入力形式に応じて、圧縮解除と色変換のどこまでを明示的な MFT パイプラインで行うか決める
  enum class DecodeMethod { None = 0, Convert, Decode } decodeMethod
  {
    selectedFormat.subType == MFVideoFormat_MJPG ? DecodeMethod::Decode :
    selectedFormat.subType == MFVideoFormat_H264 ? DecodeMethod::Decode :
    selectedFormat.subType == MFVideoFormat_NV12 ? DecodeMethod::Convert :
    selectedFormat.subType == MFVideoFormat_YUY2 ? DecodeMethod::Convert :
    DecodeMethod::None
  };

  // 形式変更前の MFT とバッファを破棄し、新しい形式だけに対応するパイプラインを作り直す
  cleanUpTransform(&caps[cam].pDecoder);
  cleanUpTransform(&caps[cam].pConverter);

  HRESULT hr{ S_OK };
  IMFMediaType* pMediaType{ nullptr };
  // Source Reader には列挙時に得たネイティブ形式を指定し、不要な暗黙変換を避ける
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

  // 後段へ渡す共有画像を、最終出力である OpenCV の BGR 形式で確保する
  image[cam] = cv::Mat::zeros(caps[cam].height, caps[cam].width, CV_8UC3);

  // フレームレートの逆数を送出間隔として保存し、取得側のペーシングに利用する
  interval[cam] = (selectedFormat.fpsDenom != 0)
    ? (static_cast<double>(selectedFormat.fpsDenom) / selectedFormat.fpsNum)
    : (1.0 / 30.0);

  if (decodeMethod != DecodeMethod::None)
  {
    if (decodeMethod == DecodeMethod::Decode)
    {
      // MJPG/H.264 はまず NV12 へ伸張し、その NV12 を共通の色変換段へ渡す
      hr = findVideoDecoder(selectedFormat.subType, &caps[cam].pDecoder);
      if (FAILED(hr)) goto done;

      ICodecAPI* pCodecAPI{ nullptr };
      if (SUCCEEDED(caps[cam].pDecoder->QueryInterface(IID_PPV_ARGS(&pCodecAPI))))
      {
        // 対応デコーダでは低遅延モードを有効にする。実装差に合わせて UINT32 と BOOL の両表現を試す
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

    // NV12/YUY2 を OpenCV へコピーしやすい RGB32 に統一するため、カラーコンバータを後段に接続する
    hr = CoCreateInstance(CLSID_CColorConvertDMO, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&caps[cam].pConverter));
    if (FAILED(hr)) goto done;

    hr = setUpPipeline(caps[cam].pConverter, selectedFormat, MFVideoFormat_RGB32);
    if (FAILED(hr)) goto done;

    hr = createConverterBuffer(cam);
    if (FAILED(hr)) goto done;
  }

done:
  SafeRelease(&pMediaType);

  // 中途半端なパイプラインを残さないよう、設定失敗時は対象カメラをまとめて閉じる
  if (SUCCEEDED(hr)) return true;

  close(cam);
  return false;
}

//
// カメラを開く
//
bool CamMf::open(int device, int cam, bool setupFormat)
{
  // ライブカメラでは滞留フレームを捨て、常に最新映像を優先する
  prioritizeLatency[cam] = true;

  if (!ComInitializer::activate(device, &caps[cam].pMediaSource)) return false;

  HRESULT hr{ S_OK };
  IMFAttributes* pAttributes{ nullptr };
  hr = MFCreateAttributes(&pAttributes, 1);
  if (FAILED(hr)) goto done;

  // ハードウェア変換と低遅延を許可し、映像処理の自動挿入は避けて後段を明示的に構成する
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

    // 設定されたコーデック、解像度、FPS に最も近い形式を選ぶ。
    // FPS が 0 の場合は従来どおり利用可能な最高値を選ぶ。
    int selectedIdx = 0;
    const double requestedFps{ defaults.camera_fps[cam] };
    double bestValue{ requestedFps > 0.0 ? std::numeric_limits<double>::max() : -1.0 };
    bool matched{ false };

    int reqWidth = 1280, reqHeight = 720;
    sscanf_s(defaults.camera_resolution[cam].c_str(), "%d x %d", &reqWidth, &reqHeight);
    std::string reqCodec = defaults.camera_codec[cam];

    for (int i = 0; i < caps[cam].availableFormats.size(); ++i)
    {
      const auto& fmt = caps[cam].availableFormats[i];
      if (SubTypeToName(fmt.subType) == reqCodec && fmt.width == reqWidth && fmt.height == reqHeight)
      {
        double fps = static_cast<double>(fmt.fpsNum) / fmt.fpsDenom;
        const double value{ requestedFps > 0.0 ? std::abs(fps - requestedFps) : fps };
        if ((requestedFps > 0.0 && value < bestValue)
          || (requestedFps <= 0.0 && value > bestValue))
        {
          bestValue = value;
          selectedIdx = i;
          matched = true;
        }
      }
    }

    // パイプラインが完成してから取得スレッドを起動し、未初期化資源へのアクセスを防ぐ
    if (setFormat(cam, selectedIdx))
    {
      if (matched)
      {
        const auto& selected{ caps[cam].availableFormats[selectedIdx] };
        defaults.camera_fps[cam] = static_cast<double>(selected.fpsNum) / selected.fpsDenom;
      }

      // スレッドを起動
      run[cam] = true;
      captureThread[cam] = std::thread([this, cam]() { capture(cam); });
      return true;
    }
  }

done:
  // open の途中で失敗した場合は、再試行できる空の状態まで列挙結果と COM 資源を戻す
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
  // ReadSample の待機を Flush で解除し、スレッドが終了してから共有する COM 資源を解放する
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

  // スレッド停止後に変換パイプライン、再利用バッファ、Source Reader、Media Source を破棄する
  cleanUpTransform(&caps[cam].pDecoder);
  cleanUpTransform(&caps[cam].pConverter);
  SafeRelease(&caps[cam].pDecoderBuffer);
  SafeRelease(&caps[cam].pConverterBuffer);
  SafeRelease(&caps[cam].pSourceReader);
  SafeRelease(&caps[cam].pMediaSource);

  // 次の open で以前のデバイス情報やフレーム完了状態が参照されないよう、付随状態も初期化する
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
  // 同期 ReadSample 中の各スレッドを先に起こし、基底クラスの join が待ち続けないようにする
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

  // Source Reader と MFT を安全に組み替えるため、一時的に取得スレッドを完全停止する
  bool wasRunning = run[cam];
  if (wasRunning)
  {
    run[cam] = false;
    caps[cam].pSourceReader->Flush(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
    if (captureThread[cam].joinable()) captureThread[cam].join();
  }

  // 新形式の設定に成功した場合だけ、変更前に動作していた取得スレッドを再開する
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

  // UI の解像度インデックスを、選択コーデック内で重複を除いた解像度文字列へ変換する
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

  // 同じコーデック・解像度に複数のレートがある場合は、最も高い FPS のネイティブ形式を採用する
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
    // 実際に選択できる形式が確定してから、設定値と UI の選択位置を同期する
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
  // Source Reader からサンプルを同期取得し、必要な変換を経て共有 BGR 画像へ渡し続ける
  while (run[cam])
  {
    DWORD dwStreamIndex{ 0 };
    DWORD dwStreamFlags{ 0 };
    LONGLONG llTimestamp{ 0 };
    IMFSample* pSample{ nullptr };

    // Flush や一時的な読み取り失敗は終了条件にせず、run が有効な間は次のサンプルを待つ
    if (FAILED(caps[cam].pSourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
      0, &dwStreamIndex, &dwStreamFlags, &llTimestamp, &pSample))) continue;

    if (!pSample) continue;

    IMFMediaBuffer* pBuffer{ nullptr };
    IMFSample* pDecodedSample{ nullptr };
    IMFSample* pConvertedSample{ nullptr };
    DWORD dwStatus{ 0 };

    if (caps[cam].pDecoder)
    {
      // 圧縮サンプルをデコーダへ投入し、後段が扱う NV12 サンプルを取り出す
      HRESULT hr{ caps[cam].pDecoder->ProcessInput(0, pSample, 0) };
      if (FAILED(hr)) goto done;

      // MFT が出力 Sample を所有する型か、呼び出し側が Sample と Buffer を渡す型かを判別する
      MFT_OUTPUT_STREAM_INFO streamInfo{};
      bool mftProvidesSamples{ false };
      if (SUCCEEDED(caps[cam].pDecoder->GetOutputStreamInfo(0, &streamInfo)))
      {
        mftProvidesSamples = (streamInfo.dwFlags & MFT_OUTPUT_STREAM_PROVIDES_SAMPLES) != 0;
      }

      // 一入力から複数出力される場合に備え、低遅延モードでは最後に得た Sample だけを保持する
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
          // 呼び出し側バッファ方式の MFT には、繰り返し利用する整列済みバッファを Sample に付けて渡す
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
          // H.264 などがストリーム内で形式を確定・変更した場合、NV12 出力と後段資源を再交渉する
          IMFMediaType* pNewOutputType{ nullptr };
          GUID subtype{ 0 };
          DWORD dwTypeIndex = 0;
          bool foundNV12 = false;
          hr = S_OK;

          // デコーダが提示する候補から NV12 を優先し、カラーコンバータへの入力形式を統一する
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
            // NV12 が明示列挙されない実装でも、先頭候補を基に NV12 を要求して交渉を試みる
            hr = caps[cam].pDecoder->GetOutputAvailableType(0, 0, &pNewOutputType);
            if (SUCCEEDED(hr))
            {
              hr = pNewOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
              subtype = MFVideoFormat_NV12;
            }
          }

          if (SUCCEEDED(hr) && pNewOutputType)
          {
            // 出力候補に不足する寸法・レート・画素比などを現在の入力形式から引き継ぐ
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
            // 確定した出力形式に合わせてデコーダバッファ、色変換器、共有画像を一体で作り直す
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
              // 描画スレッドと競合しないようロック中に BGR 共有画像の寸法を更新する
              captureMutex[cam].lock();
              image[cam] = cv::Mat::zeros(caps[cam].height, caps[cam].width, CV_8UC3);
              captureMutex[cam].unlock();
            }
          }

          // 形式変更前に確保した Sample とイベントを破棄し、新しい条件で ProcessOutput をやり直す
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
          // 現入力から出力が得られなければ一時資源を戻し、次の Source Reader サンプルへ進む
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
          // その他のデコード失敗では所有方式に合わせて Sample を解放し、この入力の処理を打ち切る
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

        // 正常出力を得るたびに直前の候補を解放し、最新 Sample の所有権だけを保持する
        SafeRelease(&pLatestDecodedSample);
        if (mftProvidesSamples) pLatestDecodedSample = decodedBuffer.pSample;
        else pLatestDecodedSample = pDecodedSample;

        pDecodedSample = nullptr;
        SafeRelease(&decodedBuffer.pEvents);
        hasOutput = true;

        // レイテンシ優先なら
        if (prioritizeLatency[cam])
        {
          // 最新を取得し続けるためループを継続する
          continue;
        }
        else
        {
          // 全フレーム処理モードならループを抜ける
          break;
        }
      }

      if (hasOutput && pLatestDecodedSample)
      {
        // 元の圧縮 Sample を、デコーダから得た NV12 Sample に差し替えて後段へ渡す
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
      // NV12/YUY2 Sample を RGB32 に変換し、以降のコピー処理を入力コーデックから独立させる
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

    // 最終 Sample の先頭バッファをロックし、CPU から参照できる画素配列を取得する
    if (FAILED(pSample->GetBufferByIndex(0, &pBuffer))) goto done;

    if (pBuffer)
    {
      BYTE* pData{ nullptr };
      DWORD cbDataLength{ 0 };

      if (SUCCEEDED(pBuffer->Lock(&pData, nullptr, &cbDataLength)) && pData)
      {
        // 全フレーム処理モードなら
        if (!prioritizeLatency[cam])
        {
          // ファイル／ネットワーク入力では前フレームが消費されるまで待ち、フレーム欠落を防ぐ
          // キャプチャしている間は
          while (run[cam] && (captured[cam]
            || (cam == camL && isPackedCameraLayout(defaults.camera_layout) && captured[camR])))
          {
            // スレッドを一時停止して CPU リソースを解放する
            std::this_thread::yield();
          }
        }

        // BGRA (4チャンネル) を BGR (3チャンネル) に変換する
        cv::Mat temp(caps[cam].height, caps[cam].width, CV_8UC4, pData);
        cv::Mat converted;
        cv::cvtColor(temp, converted, cv::COLOR_BGRA2BGR);

        if (cam == camL && isPackedCameraLayout(defaults.camera_layout))
        {
          // 1フレームに格納された左右画像を、描画・送信が扱う左右別画像へ分割する
          std::scoped_lock lock(captureMutex[camL], captureMutex[camR]);
          if (defaults.camera_layout == CAMERA_LAYOUT_SIDE_BY_SIDE)
          {
            const int width{ converted.cols / 2 };
            converted(cv::Rect(0, 0, width, converted.rows)).copyTo(image[camL]);
            converted(cv::Rect(width, 0, width, converted.rows)).copyTo(image[camR]);
          }
          else
          {
            const int height{ converted.rows / 2 };
            converted(cv::Rect(0, 0, converted.cols, height)).copyTo(image[camL]);
            converted(cv::Rect(0, height, converted.cols, height)).copyTo(image[camR]);
          }
          captured[camL] = captured[camR] = true;
          unsent[camL] = unsent[camR] = true;
        }
        else
        {
          std::lock_guard<std::mutex> lock(captureMutex[cam]);
          image[cam] = std::move(converted);
          captured[cam] = true;
          unsent[cam] = true;
        }

        pBuffer->Unlock();
      }
      pBuffer->Release();
      pBuffer = nullptr;
    }

  done:
    // 途中のどの段で失敗しても、この反復で所有した Sample を解放して次の読み取りへ進む
    if (pDecodedSample) pDecodedSample->Release();
    if (pConvertedSample) pConvertedSample->Release();
    if (pSample) pSample->Release();
  }
}



CamMf::~CamMf()
{
  // オブジェクト破棄前に全取得スレッドを止め、参照中の Media Foundation 資源を解放する
  close();
}

//
// ファイル／ネットワークからキャプチャを開始する
//
bool CamMf::open(const std::string& file, int cam)
{
  // ファイル／ネットワーク入力ではフレーム順序を維持し、未消費フレームを破棄しない
  prioritizeLatency[cam] = false;

  // Media Foundation の URL API に渡すため、公開 API の UTF-8 パスを UTF-16 に変換する
  int wlen = MultiByteToWideChar(CP_UTF8, 0, file.c_str(), -1, nullptr, 0);
  if (wlen <= 0) return false;
  std::vector<wchar_t> wbuf(wlen);
  MultiByteToWideChar(CP_UTF8, 0, file.c_str(), -1, wbuf.data(), wlen);

  // URL から直接 Source Reader を作り、公開された先頭映像形式で共通キャプチャループを開始する
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

  // 列挙または形式設定に失敗した Reader を残さず、呼び出し側が再試行できる状態へ戻す
  SafeRelease(&caps[cam].pSourceReader);
  return false;
}



#pragma once

///
/// Microsoft Media Foundation を使ったビデオキャプチャクラスの定義 (ステレオ対応版)
///
/// @file
/// @author Kohe Tokoi
/// @date December 24, 2024
///

// カメラ関連の処理
#include "Camera.h"

// Microsoft Media Foundation
#include <MFapi.h>
#include <Mfidl.h>
#include <MFtransform.h>
#include <MFreadwrite.h>
#include <Mferror.h>
#include <wmcodecdsp.h>

#include <vector>
#include <string>

///
/// Microsoft Media Foundation を使ってビデオをキャプチャするクラス
///
class CamMf : public Camera
{
public:
  ///
  /// ビデオフォーマットの詳細を保持する構造体
  ///
  struct VideoFormat
  {
    UINT32 width;     ///< 幅
    UINT32 height;    ///< 高さ
    UINT32 fpsNum;    ///< フレームレートの分子 (Numerator)
    UINT32 fpsDenom;  ///< フレームレートの分母 (Denominator)
    GUID subType;     ///< ピクセルフォーマット/コーデックの GUID

    ///
    /// コンストラクタ
    ///
    VideoFormat(UINT32 width, UINT32 height,
      UINT32 fpsNum, UINT32 fpsDenom, GUID subType)
      : width{ width }
      , height{ height }
      , fpsNum{ fpsNum }
      , fpsDenom{ fpsDenom }
      , subType{ subType }
    {
    }
  };

private:
  ///
  /// COM ライブラリの初期化と終了を行うクラス
  ///
  class ComInitializer
  {
    /// COM ライブラリの初期化と終了を行うオブジェクト (シングルトン)
    static ComInitializer instance;

    /// メディアソースのリスト
    IMFActivate** ppSourceActivate;

    /// メディアソースの数
    UINT32 cSourceActivate;

    /// ビデオキャプチャデバイスの表示名のリスト
    std::vector<std::string> deviceList;

    /// COM ライブラリが初期化されていれば true
    bool coInitialized;

    /// Media Foundation が起動されていれば true
    bool mfStarted;

    ///
    /// COM ライブラリの初期化と終了を行うクラスのコンストラクタ
    ///
    ComInitializer();

    ///
    /// COM ライブラリの初期化と終了を行うクラスのデストラクタ
    ///
    ~ComInitializer();

    ///
    /// COM ライブラリを初期化して Media Foundation を開始する
    ///
    const char* initialize();

  public:

    // シングルトンなのでコピーは作らせない
    ComInitializer(const ComInitializer& com) = delete;
    ComInitializer(ComInitializer&& com) = delete;
    ComInitializer& operator=(const ComInitializer& com) = delete;
    ComInitializer& operator=(ComInitializer&&) = delete;

    ///
    /// COM ライブラリのシングルトンインスタンスを返す
    ///
    static const ComInitializer& getInstance();

    ///
    /// キャプチャデバイスを有効化してメディアソースを作成する
    ///
    static bool activate(int device, IMFMediaSource** pMediaSource);

    ///
    /// ビデオキャプチャデバイスの表示名のリストを返す
    ///
    static const std::vector<std::string>& getDeviceList();
  };

  /// 左右それぞれのカメラリソースを保持する構造体
  struct CameraResource
  {
    /// メディアソースのポインタ
    IMFMediaSource* pMediaSource = nullptr;

    /// メディアソースのリーダーへのポインタ
    IMFSourceReader* pSourceReader = nullptr;

    /// MFT デコーダへのポインタ (MJPG, H264 等デコード用)
    IMFTransform* pDecoder = nullptr;

    /// MFT デコーダの出力フレームを保持するバッファ
    IMFMediaBuffer* pDecoderBuffer = nullptr;

    /// MFT カラーコンバータへのポインタ (RGB32 変換用)
    IMFTransform* pConverter = nullptr;

    /// MFT カラーコンバータの出力フレームを保持するバッファ
    IMFMediaBuffer* pConverterBuffer = nullptr;

    /// 使用可能なビデオフォーマットのリスト
    std::vector<VideoFormat> availableFormats;

    /// 使用可能なビデオフォーマットの表示名のリスト
    std::vector<std::string> formatList;

    /// 重複を排除したコーデックのリスト
    std::vector<std::string> codecList;

    /// 重複を排除した解像度のリスト
    std::vector<std::string> resolutionList;

    /// 現在選択されているコーデック/解像度のインデックス (キャッシュ用)
    int selectedCodecIndex = -1;
    int selectedResolutionIndex = -1;

    /// カメラの解像度
    int width = 0;
    int height = 0;
  } caps[camCount];

  ///
  /// 使用可能な解像度、フレームレート、コーデックのリストを作成する
  ///
  bool enumerateFormats(int cam);

  ///
  /// 指定されたサブタイプに対応するビデオデコーダを探す
  ///
  HRESULT findVideoDecoder(
    const GUID& subtype,
    IMFTransform** ppDecoder,
    BOOL bAllowAsync = FALSE,
    BOOL bAllowHardware = FALSE,
    BOOL bAllowTranscode = FALSE
  ) const;

  ///
  /// MFT のセットアップと接続を行う
  ///
  HRESULT setUpPipeline(IMFTransform* pTransform,
    const VideoFormat& format, const GUID& subType) const;

  ///
  /// MFT を解放する
  ///
  void cleanUpTransform(IMFTransform** pTransform) const;

  ///
  /// デコーダの出力バッファを作成する
  ///
  HRESULT createDecoderBuffer(int cam);

  ///
  /// カラーコンバータの出力バッファを作成する
  ///
  HRESULT createConverterBuffer(int cam);

  ///
  /// Source Reader の出力フォーマットを設定し、基底クラスの image を初期化する
  ///
  bool setFormat(int cam, int index);

  ///
  /// フレームをキャプチャする（別スレッドでループ実行される）
  ///
  void capture(int cam);

public:
    
  ///
  /// コンストラクタ
  ///
  CamMf();

  ///
  /// デストラクタ
  ///
  virtual ~CamMf();

  ///
  /// Media Foundation のビデオデバイスの一覧を返す
  ///
  static const std::vector<std::string>& getDeviceList()
  {
    return ComInitializer::getDeviceList();
  }

  ///
  /// デバイス番号を指定して利用可能なビデオフォーマットを取得する (デバイスを開かない一時列挙用)
  ///
  static bool getDeviceFormats(int device, std::vector<VideoFormat>& formats);

  ///
  /// カメラを開く
  ///
  bool open(int device, int cam, bool setupFormat = true);

  ///
  /// ファイル／ネットワークからキャプチャを開始する
  ///
  bool open(const std::string& file, int cam);

  ///
  /// カメラが使用可能か判定する
  ///
  bool opened(int cam) const;

  ///
  /// 指定されたカメラを閉じる
  ///
  void close(int cam);

  ///
  /// 全てのカメラを閉じる
  ///
  void close();

  ///
  /// キャプチャスレッドを停止する
  ///
  virtual void stop();

  ///
  /// フォーマットを選択して設定する
  ///
  bool select(int cam, int index);

  ///
  /// コーデックと解像度のコンボインデックスを指定してフォーマットを設定する
  ///
  bool selectFormat(int cam, int codecIdx, int resIdx);

  ///
  /// 各種ゲッター
  ///
  const std::vector<std::string>& getFormatList(int cam) const { return caps[cam].formatList; }
  const std::vector<VideoFormat>& getAvailableFormats(int cam) const { return caps[cam].availableFormats; }
  const std::vector<std::string>& getCodecList(int cam) const { return caps[cam].codecList; }
  const std::vector<std::string>& getResolutionList(int cam) const { return caps[cam].resolutionList; }
  int getSelectedCodecIndex(int cam) const { return caps[cam].selectedCodecIndex; }
  int getSelectedResolutionIndex(int cam) const { return caps[cam].selectedResolutionIndex; }
};


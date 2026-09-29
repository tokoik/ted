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

// 各種設定
#include "Config.h"

// Microsoft Media Foundation
#include <MFapi.h>
#include <Mfidl.h>
#include <MFtransform.h>
#include <MFreadwrite.h>
#include <Mferror.h>
#include <wmcodecdsp.h>

#include <vector>
#include <string>
#include <algorithm>

///
/// Microsoft Media Foundation を使ってカメラまたは動画を取り込むクラス
///
/// @details
/// 圧縮入力はMFTデコーダ、非BGR入力はカラーコンバータへ通し、Camera基底が扱うBGRA画像へ統一する。
///
class CamMf : public Camera
{
public:

  ///
  /// ビデオフォーマットの詳細を保持する構造体
  ///
  struct VideoFormat
  {
    UINT32 width;     ///< フレームの幅
    UINT32 height;    ///< フレームの高さ
    UINT32 fpsNum;    ///< フレームレートの分子 (Numerator)
    UINT32 fpsDenom;  ///< フレームレートの分母 (Denominator)
    GUID subType;     ///< ピクセルフォーマット/コーデックの GUID

    ///
    /// コンストラクタ
    ///
    /// @param width フレームの幅
    /// @param height フレームの高さ
    /// @param fpsNum フレームレートの分子 (Numerator)
    /// @param fpsDenom フレームレートの分母 (Denominator)
    /// @param subType ピクセルフォーマット/コーデックの GUID
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
    /// COM/MFのプロセス内初期化とデバイス列挙結果を共有し、カメラごとの重複初期化を避ける
    static ComInitializer instance;

    /// メディアソースのリスト
    IMFActivate** ppSourceActivate{ nullptr };

    /// メディアソースの数
    UINT32 cSourceActivate{ 0 };

    /// ビデオキャプチャデバイスの表示名のリスト
    std::vector<std::string> deviceList;

    /// COM ライブラリが初期化されていれば true
    bool coInitialized{ false };

    /// Media Foundation が起動されていれば true
    bool mfStarted{ false };

    ///
    /// COM ライブラリの初期化と終了を行うクラスのコンストラクタ
    ///
    ComInitializer() = default;

    ///
    /// COM ライブラリの初期化と終了を行うクラスのデストラクタ
    ///
    ~ComInitializer();

    ///
    /// COM ライブラリを初期化して Media Foundation を開始する
    ///
    const char* initialize();

  public:

    ///
    /// コピーコンストラクタとムーブコンストラクタを封じる
    ///
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
    /// @param device デバイス番号
    /// @param pMediaSource メディアソースのポインタへのポインタ
    /// @return 成功した場合は true
    ///
    static bool activate(int device, IMFMediaSource** pMediaSource);

    ///
    /// ビデオキャプチャデバイスの表示名のリストを返す
    ///
    /// @return ビデオキャプチャデバイスの表示名のリスト
    ///
    static const std::vector<std::string>& getDeviceList();
  };

  ///
  /// 左右それぞれのカメラリソースを保持する構造体
  ///
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
  }
  caps[camCount];

  /// 右眼用のフレームバッファ
  std::vector<std::uint8_t> imageR;

  /// 右眼用の解像度
  int widthR{ 0 };
  int heightR{ 0 };

  /// 右眼用のキャプチャ完了フラグ
  std::atomic<bool> capturedR{ false };

  /// 左右それぞれのキャプチャスレッド
  std::thread captureThread[camCount];

  /// 各カメラの実行状態フラグ
  std::atomic<bool> run[camCount]{ false, false };

  /// 各カメラのレイテンシ優先フラグ
  std::atomic<bool> prioritizeLatencyCam[camCount]{ true, true };

  ///
  /// 使用可能な解像度、フレームレート、コーデックのリストを作成する
  ///
  /// @param cam カメラ番号
  /// @return 成功した場合は true
  ///
  bool enumerateFormats(int cam);

  ///
  /// 指定されたサブタイプに対応するビデオデコーダを探す
  ///
  /// @param subtype サブタイプの GUID
  /// @param ppDecoder デコーダのポインタへのポインタ
  /// @param bAllowAsync 非同期デコーダを許可するか
  /// @param bAllowHardware ハードウェアデコーダを許可するか
  /// @param bAllowTranscode トランスコードを許可するか
  /// @return 成功した場合は S_OK
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
  /// @param pTransform MFT のポインタ
  /// @param format ビデオフォーマットの詳細
  /// @param subType ピクセルフォーマット/コーデックの GUID
  /// @return 成功した場合は S_OK
  ///
  HRESULT setUpPipeline(IMFTransform* pTransform,
    const VideoFormat& format, const GUID& subType) const;

  ///
  /// MFT を解放する
  ///
  /// @param pTransform MFT のポインタへのポインタ
  ///
  void cleanUpTransform(IMFTransform** pTransform) const;

  ///
  /// デコーダの出力バッファを作成する
  ///
  /// @param cam カメラ番号
  /// @return 成功した場合は S_OK
  ///
  HRESULT createDecoderBuffer(int cam);

  ///
  /// カラーコンバータの出力バッファを作成する
  ///
  /// @param cam カメラ番号
  /// @return 成功した場合は S_OK
  ///
  HRESULT createConverterBuffer(int cam);

  ///
  /// Source Reader の出力フォーマットを設定し、基底クラスの image を初期化する
  ///
  /// @param cam カメラ番号
  /// @param index フォーマット番号
  /// @return 成功した場合は true
  ///
  bool setFormat(int cam, int index);

  ///
  /// フレームをキャプチャする（別スレッドでループ実行される）
  ///
  /// @param cam カメラ番号
  ///
  void capture(int cam);

protected:

  ///
  /// キャプチャ開始処理を行う（派生クラス固有の実装）
  ///
  /// @return 正常に開始できたら true
  ///
  virtual bool onStart() override;

  ///
  /// キャプチャ停止処理を行う（派生クラス固有の実装）
  ///
  virtual void onStop() override;

  ///
  /// キャプチャデバイスを閉じる処理を行う（派生クラス固有の実装）
  ///
  virtual void onClose() override;

public:

  ///
  /// コンストラクタ
  ///
  CamMf() = default;

  ///
  /// デストラクタ
  ///
  virtual ~CamMf();

  ///
  /// Media Foundation のビデオデバイスの一覧を返す
  ///
  /// @return ビデオデバイスの一覧
  ///
  static const std::vector<std::string>& getDeviceList()
  {
    return ComInitializer::getDeviceList();
  }

  ///
  /// デバイス番号を指定して利用可能なビデオフォーマットを取得する (デバイスを開かない一時列挙用)
  ///
  /// @param device デバイス番号
  /// @param formats ビデオフォーマットのリスト
  /// @return 成功した場合は true
  ///
  static bool getDeviceFormats(int device, std::vector<VideoFormat>& formats);

  ///
  /// カメラを開く
  ///
  /// @param device デバイス番号
  /// @param cam カメラ番号
  /// @param setupFormat フォーマットを設定するかどうか
  /// @return 成功した場合は true
  ///
  bool open(int device, int cam, bool setupFormat = true);

  ///
  /// ファイル／ネットワークからキャプチャを開始する
  ///
  /// @param file ファイルパスまたはネットワーク URL
  /// @param cam カメラ番号
  /// @return 成功した場合は true
  ///
  bool open(const std::string& file, int cam);

  ///
  /// カメラが使用可能か判定する
  ///
  /// @param cam カメラ番号
  /// @return 使用可能な場合は true
  ///
  bool opened(int cam) const;

  ///
  /// 分割後の画像の幅を得る
  ///
  virtual int getWidth(int eye = 0) const override
  {
    if (isPackedCameraLayout(defaults.camera_layout))
      return defaults.camera_layout == CAMERA_LAYOUT_SIDE_BY_SIDE
        ? static_cast<int>(caps[camL].width / 2) : static_cast<int>(caps[camL].width);
    return (eye == 0) ? caps[camL].width : caps[camR].width;
  }

  ///
  /// 分割後の画像の高さを得る
  ///
  virtual int getHeight(int eye = 0) const override
  {
    if (isPackedCameraLayout(defaults.camera_layout))
      return defaults.camera_layout == CAMERA_LAYOUT_TOP_AND_BOTTOM
        ? static_cast<int>(caps[camL].height / 2) : static_cast<int>(caps[camL].height);
    return (eye == 0) ? caps[camL].height : caps[camR].height;
  }

  ///
  /// チャンネル数を得る (BGRA = 4)
  ///
  virtual int getChannels(int eye = 0) const override
  {
    return 4;
  }

  ///
  /// 指定されたカメラを閉じる
  ///
  /// @param cam カメラ番号
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
  /// @param cam カメラ番号
  /// @param index フォーマットのインデックス
  /// @return 成功した場合は true
  ///
  bool select(int cam, int index);

  ///
  /// コーデックと解像度のコンボインデックスを指定してフォーマットを設定する
  ///
  /// @param cam カメラ番号
  /// @param codecIdx コーデックのインデックス
  /// @param resIdx 解像度のインデックス
  /// @return 成功した場合は true
  ///
  bool selectFormat(int cam, int codecIdx, int resIdx);

  ///
  /// フォーマットのリストを得る
  ///
  /// @param cam カメラ番号
  /// @return フォーマットのリスト
  ///
  const std::vector<std::string>& getFormatList(int cam) const { return caps[cam].formatList; }

  ///
  /// 利用可能なビデオフォーマットのリストを得る
  ///
  /// @param cam カメラ番号
  /// @return 利用可能なビデオフォーマットのリスト
  /// 
  const std::vector<VideoFormat>& getAvailableFormats(int cam) const { return caps[cam].availableFormats; }

  ///
  /// コーデックのリストを得る
  ///
  /// @param cam カメラ番号
  /// @return コーデックのリスト
  ///
  const std::vector<std::string>& getCodecList(int cam) const { return caps[cam].codecList; }

  ///
  /// 解像度のリストを得る
  ///
  /// @param cam カメラ番号
  /// @return 解像度のリスト
  ///
  const std::vector<std::string>& getResolutionList(int cam) const { return caps[cam].resolutionList; }

  /// 
  /// 選択されたコーデックのインデックスを取得する
  ///
  /// @param cam カメラ番号
  /// @return 選択されたコーデックのインデックス
  ///
  int getSelectedCodecIndex(int cam) const { return caps[cam].selectedCodecIndex; }

  ///
  /// 選択された解像度のインデックスを取得する
  /// 
  /// @param cam カメラ番号
  /// @return 選択された解像度のインデックス
  ///
  int getSelectedResolutionIndex(int cam) const { return caps[cam].selectedResolutionIndex; }

  ///
  /// 送信用に指定した視点の最新フレームを複製する
  ///
  bool copyFrame(int eye, std::vector<std::uint8_t>& data, int& w, int& h, int& ch) const override
  {
    std::lock_guard<std::mutex> lock{ mtx };
    return eye == 0
      ? copyBuffer(image, width, height, channels, data, w, h, ch)
      : copyBuffer(imageR, widthR, heightR, 4, data, w, h, ch);
  }

  ///
  /// カメラフレームを OpenGL テクスチャへ転送する
  ///
  /// @param eye 視点番号 (0: 左/単眼, 1: 右)
  /// @param texture 転送先のテクスチャ名 (GLuint)
  /// @param size テクスチャのサイズ配列 (幅, 高さ)
  /// @return 転送に成功したら true
  ///
  virtual bool transmit(int eye, unsigned int texture, const int* size) override;


  ///
  /// 指定した視点のフレームデータをロックして処理関数を実行する
  ///
  /// @tparam F 処理関数の型
  /// @param eye 視点番号 (0: 左/単眼, 1: 右)
  /// @param func フレームデータを処理する関数 (引数: const std::uint8_t* data, size_t length, int width, int height, int channels)
  /// @return フレームが取得できて処理関数が実行されたら true
  ///
  template <typename F>
  bool lockFrame(int eye, F&& func)
  {
    std::unique_lock<std::mutex> lock{ mtx, std::try_to_lock };
    if (!lock.owns_lock()) return false;

    // 転送したら転送待ちの印を下ろす (全フレーム処理モードのキャプチャスレッドはこれを待つ)
    if (eye == 0 && captured && !image.empty())
    {
      const auto length{ static_cast<size_t>(width) * height * channels };
      func(image.data(), std::min(image.size(), length), width, height, channels);
      captured = false;
      return true;
    }
    else if (eye == 1 && capturedR && !imageR.empty())
    {
      const auto length{ static_cast<size_t>(widthR) * heightR * 4 };
      func(imageR.data(), std::min(imageR.size(), length), widthR, heightR, 4);
      capturedR = false;
      return true;
    }
    return false;
  }

  ///
  /// 単一視点用のフレームデータロック
  ///
  template <typename F>
  bool lockFrame(F&& func)
  {
    return lockFrame(0, std::forward<F>(func));
  }
};

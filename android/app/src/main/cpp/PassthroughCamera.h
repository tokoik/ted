#pragma once

///
/// Quest 3 のパススルーカメラ (Passthrough Camera API) から画像を取得して JPEG に符号化する
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
/// @details
/// Horizon OS v74 以降の Quest 3 / 3S では、前面の左右 RGB カメラを
/// Android の Camera2 API (NDK の ACamera) で取得できる。
/// 実行時に android.permission.CAMERA と horizonos.permission.HEADSET_CAMERA が必要。
/// 取得した YUV420 画像は別スレッドで RGBA に変換し、AndroidBitmap_compress() で
/// JPEG に符号化する (PC 版は cv::imdecode() で復号する)。
///

#include "VideoEncoder.h"

#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>
#include <media/NdkImageReader.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class PassthroughCamera
{
public:

  /// 左右のカメラ
  static constexpr int camCount{ 2 };

  /// 符号化済み画像
  using Image = std::shared_ptr<const std::vector<std::uint8_t>>;

  ///
  /// 符号化済みの左右画像の組
  ///
  struct Frame
  {
    /// 左右の JPEG 画像 (右カメラが無ければ右は nullptr)
    Image jpeg[camCount];

    /// 左画像の撮影時刻 (ナノ秒, CLOCK_MONOTONIC に換算済み)
    std::int64_t timestamp{ 0 };

    /// 通し番号 (新しい組ほど大きい)
    std::uint64_t sequence{ 0 };
  };

  ///
  /// 動画のアクセスユニット
  ///
  struct VideoUnit
  {
    /// 視点 (0: 左, 1: 右)
    int eye{ 0 };

    /// VideoUnitHeader 付きのアクセスユニット
    Image payload;

    /// 撮影時刻 (ナノ秒, CLOCK_MONOTONIC に換算済み)
    std::int64_t timestamp{ 0 };

    /// キーフレームなら true
    bool keyframe{ false };
  };

  ///
  /// 動画の符号化の設定
  ///
  struct VideoSettings
  {
    /// 画像の形式 (ted::IMAGE_JPEG なら JPEG を 1 枚ずつ送る)
    std::uint32_t format{ 0 };

    /// 片眼あたりのビットレート (bps)
    int bitrate{ 6000000 };

    /// キーフレームの間隔 (秒)
    int keyframeInterval{ 2 };
  };

  PassthroughCamera() = default;
  ~PassthroughCamera();

  PassthroughCamera(const PassthroughCamera&) = delete;
  PassthroughCamera& operator=(const PassthroughCamera&) = delete;

  ///
  /// 左右のパススルーカメラを開いて取得を開始する
  ///
  /// @param width 画像の幅
  /// @param height 画像の高さ
  /// @param quality JPEG の品質 (0～100)
  /// @param fps 符号化するフレームレートの上限 (0 なら制限しない)
  /// @param video 動画として符号化する場合の設定
  /// @return 少なくとも左 (または唯一の) カメラが開けたら true
  ///
  bool open(int width, int height, int quality, double fps, const VideoSettings& video);

  ///
  /// 左右のパススルーカメラを開いて JPEG で取得を開始する
  ///
  bool open(int width, int height, int quality, double fps)
  {
    return open(width, height, quality, fps, VideoSettings{});
  }

  ///
  /// 動画として符号化しているか
  ///
  bool isVideo() const
  {
    return video.format != 0;
  }

  ///
  /// 符号化した動画のアクセスユニットをすべて取り出す
  ///
  /// @param units 格納先 (取り出した順に左右が混在する)
  /// @return 取り出したアクセスユニットがあれば true
  ///
  bool takeVideoUnits(std::vector<VideoUnit>& units);

  ///
  /// 動画の次のフレームをキーフレームにする
  ///
  /// @return キーフレームの要求に成功したら true
  ///
  bool requestKeyframe();

  ///
  /// 溢れて捨てたアクセスユニットがあれば true を返して記録を消す
  ///
  bool takeOverflow()
  {
    return unitsOverflowed.exchange(false);
  }

  ///
  /// カメラを閉じる
  ///
  void close();

  ///
  /// 取得中かどうか
  ///
  /// @return カメラが開いていて、切断やエラーが起きていなければ true
  ///
  bool isRunning() const;

  ///
  /// 前回より新しい画像の組があれば取り出す
  ///
  /// @param frame 格納先
  /// @param lastSequence 前回取り出した組の通し番号
  /// @return 新しい組があれば true
  ///
  bool getLatest(Frame& frame, std::uint64_t lastSequence) const;

private:

  ///
  /// 受け取った 1 枚分の YUV420 画像 (Y は全画素, U と V は縦横半分に間引いた画素)
  ///
  struct Yuv
  {
    std::vector<std::uint8_t> y, u, v;
    std::int64_t timestamp{ 0 };
  };

  ///
  /// 片眼のカメラ
  ///
  struct Eye
  {
    PassthroughCamera* owner{ nullptr };
    std::string id;
    ACameraDevice* device{ nullptr };
    AImageReader* reader{ nullptr };
    ANativeWindow* window{ nullptr };
    ACaptureSessionOutputContainer* container{ nullptr };
    ACaptureSessionOutput* output{ nullptr };
    ACameraOutputTarget* target{ nullptr };
    ACaptureRequest* request{ nullptr };
    ACameraCaptureSession* session{ nullptr };

    /// NDK に渡すコールバック (カメラを閉じるまで保持する)
    ACameraDevice_StateCallbacks deviceCallbacks{};
    ACameraCaptureSession_stateCallbacks sessionCallbacks{};
    AImageReader_ImageListener imageListener{};

    /// 動画のエンコーダ (動画として符号化する場合)
    std::unique_ptr<VideoEncoder> encoder;

    /// 最後に受け取った画像 (符号化スレッドが取り出したら fresh を false にする)
    Yuv yuv;
    bool fresh{ false };
  };

  /// カメラを開く
  bool openEye(Eye& eye, const std::string& id);

  /// カメラを閉じる
  void closeEye(Eye& eye);

  /// ImageReader のコールバック
  static void onImageAvailable(void* context, AImageReader* reader);

  /// カメラデバイスのコールバック
  static void onDisconnected(void* context, ACameraDevice* device);
  static void onError(void* context, ACameraDevice* device, int error);

  /// キャプチャセッションのコールバック (何もしない)
  static void onSessionState(void* context, ACameraCaptureSession* session);

  /// エンコーダが出力したアクセスユニットを保存する
  void pushVideoUnit(int eye, VideoEncoder::Unit&& unit);

  /// 符号化スレッド
  void encodeLoop();

  /// YUV を RGBA に変換して JPEG に符号化する
  Image encode(const Yuv& yuv, std::vector<std::uint8_t>& rgba) const;

  /// カメラマネージャ
  ACameraManager* manager{ nullptr };

  /// 左右のカメラ
  Eye eyes[camCount];

  /// 開いたカメラの数
  int eyeCount{ 0 };

  /// 画像の大きさ
  int width{ 0 }, height{ 0 };

  /// JPEG の品質
  int quality{ 50 };

  /// 符号化するフレームレートの上限 (0 なら制限しない)
  double fps{ 0.0 };

  /// 符号化の最小間隔 (秒)
  double minInterval{ 0.0 };

  /// 撮影時刻が CLOCK_BOOTTIME なら true
  bool bootTime{ false };

  /// 取得中なら true
  std::atomic<bool> running{ false };

  /// 切断やエラーが起きたら true
  std::atomic<bool> failed{ false };

  /// 受け取った YUV の排他制御
  std::mutex yuvMutex;
  std::condition_variable yuvReady;

  /// 符号化スレッド
  std::thread encoder;

  /// 符号化済み画像の排他制御
  mutable std::mutex frameMutex;

  /// 最新の符号化済み画像の組
  Frame latest;

  /// 動画の符号化の設定
  VideoSettings video;

  /// 符号化した動画のアクセスユニットの排他制御
  std::mutex unitMutex;

  /// 取り出されていない動画のアクセスユニット
  std::deque<VideoUnit> units;

  /// エンコーダの排他制御
  mutable std::mutex encoderMutex;

  /// 取り出されずに溢れたら true (キーフレームで復帰させる)
  std::atomic<bool> unitsOverflowed{ false };
};

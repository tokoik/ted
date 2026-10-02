///
/// Quest 3 のパススルーカメラから画像を取得して JPEG に符号化する処理の実装
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
#include "PassthroughCamera.h"
#include "TedProtocol.h"

#include <android/bitmap.h>
#include <android/data_space.h>
#include <android/log.h>
#include <camera/NdkCameraMetadata.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>

#define LOG_TAG "TED"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace
{
  /// Meta のベンダータグ com.meta.extra_metadata.position (左 0, 右 1)
  constexpr std::uint32_t metaCameraPositionTag{ 0x80004d01 };

  /// ベンダータグが読めない場合の Quest 3 の左右パススルーカメラの ID
  const char* const fallbackCameraId[PassthroughCamera::camCount]{ "50", "51" };

  ///
  /// 指定したクロックの現在時刻 (ナノ秒)
  ///
  std::int64_t now(clockid_t clock)
  {
    timespec ts{};
    clock_gettime(clock, &ts);
    return static_cast<std::int64_t>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
  }

  ///
  /// 整数のメタデータを 1 つ読む
  ///
  bool getInt(const ACameraMetadata* meta, std::uint32_t tag, int& value)
  {
    ACameraMetadata_const_entry entry{};
    if (ACameraMetadata_getConstEntry(meta, tag, &entry) != ACAMERA_OK || entry.count == 0) return false;
    switch (entry.type)
    {
    case ACAMERA_TYPE_BYTE: value = entry.data.u8[0]; return true;
    case ACAMERA_TYPE_INT32: value = entry.data.i32[0]; return true;
    case ACAMERA_TYPE_INT64: value = static_cast<int>(entry.data.i64[0]); return true;
    default: return false;
    }
  }

  ///
  /// カメラの内部パラメータから PC 版の remote_fov_x / remote_fov_y の推奨値をログに出す
  ///
  void logIntrinsics(const ACameraMetadata* meta, const char* id, int width, int height)
  {
    ACameraMetadata_const_entry intrinsics{};
    if (ACameraMetadata_getConstEntry(meta, ACAMERA_LENS_INTRINSIC_CALIBRATION, &intrinsics) != ACAMERA_OK
      || intrinsics.count < 4)
    {
      LOGW("camera %s: no intrinsic calibration", id);
      return;
    }

    // 内部パラメータはセンサーの有効画素領域を基準にしている
    ACameraMetadata_const_entry array{};
    if (ACameraMetadata_getConstEntry(meta, ACAMERA_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE, &array) != ACAMERA_OK
      || array.count < 4)
    {
      if (ACameraMetadata_getConstEntry(meta, ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &array) != ACAMERA_OK
        || array.count < 4) return;
    }

    const float fx{ intrinsics.data.f[0] }, fy{ intrinsics.data.f[1] };
    const float cx{ intrinsics.data.f[2] }, cy{ intrinsics.data.f[3] };
    const float sensorW{ static_cast<float>(array.data.i32[2]) };
    const float sensorH{ static_cast<float>(array.data.i32[3]) };
    if (fx <= 0.0f || fy <= 0.0f || sensorW <= 0.0f || sensorH <= 0.0f) return;

    // 出力画像はセンサー領域の中央を縦横比を保って切り出して拡大縮小したもの
    float scaleX{ width / sensorW }, scaleY{ height / sensorH };
    const float scaleMax{ std::max(scaleX, scaleY) };
    scaleX /= scaleMax;
    scaleY /= scaleMax;
    const float cropW{ sensorW * scaleX }, cropH{ sensorH * scaleY };
    const float cropX{ sensorW * (1.0f - scaleX) * 0.5f }, cropY{ sensorH * (1.0f - scaleY) * 0.5f };

    const float fxOut{ fx * width / cropW }, fyOut{ fy * height / cropH };
    const float cxOut{ (cx - cropX) * width / cropW }, cyOut{ (cy - cropY) * height / cropH };

    LOGI("camera %s: fx=%.2f fy=%.2f cx=%.2f cy=%.2f for %dx%d", id, fxOut, fyOut, cxOut, cyOut,
      width, height);
    LOGI("camera %s: PC config.json -> \"remote_fov_x\": %.4f, \"remote_fov_y\": %.4f "
      "(principal point offset %.1f, %.1f px)", id,
      std::atan(0.5f * width / fxOut), std::atan(0.5f * height / fyOut),
      cxOut - 0.5f * width, cyOut - 0.5f * height);

    ACameraMetadata_const_entry translation{}, rotation{};
    if (ACameraMetadata_getConstEntry(meta, ACAMERA_LENS_POSE_TRANSLATION, &translation) == ACAMERA_OK
      && translation.count >= 3
      && ACameraMetadata_getConstEntry(meta, ACAMERA_LENS_POSE_ROTATION, &rotation) == ACAMERA_OK
      && rotation.count >= 4)
    {
      LOGI("camera %s: lens pose t=(%.4f, %.4f, %.4f) q=(%.4f, %.4f, %.4f, %.4f)", id,
        translation.data.f[0], translation.data.f[1], translation.data.f[2],
        rotation.data.f[0], rotation.data.f[1], rotation.data.f[2], rotation.data.f[3]);
    }
  }

  ///
  /// YUV420 の出力として指定した大きさが使えるか確かめ、使えなければ近いものを選ぶ
  ///
  void selectSize(const ACameraMetadata* meta, int& width, int& height)
  {
    ACameraMetadata_const_entry entry{};
    if (ACameraMetadata_getConstEntry(meta, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry) != ACAMERA_OK)
      return;

    int bestW{ 0 }, bestH{ 0 };
    for (std::uint32_t i = 0; i + 3 < entry.count; i += 4)
    {
      const int format{ entry.data.i32[i] }, w{ entry.data.i32[i + 1] }, h{ entry.data.i32[i + 2] };
      const bool input{ entry.data.i32[i + 3] != 0 };
      if (format != AIMAGE_FORMAT_YUV_420_888 || input) continue;
      if (w == width && h == height) return;
      if (w * h > bestW * bestH) bestW = w, bestH = h;
    }

    if (bestW > 0)
    {
      LOGW("camera size %dx%d is not available; using %dx%d", width, height, bestW, bestH);
      width = bestW;
      height = bestH;
    }
  }

  ///
  /// JPEG の出力先
  ///
  bool writeJpeg(void* context, const void* data, size_t size)
  {
    auto* const out{ static_cast<std::vector<std::uint8_t>*>(context) };
    const auto* const bytes{ static_cast<const std::uint8_t*>(data) };
    out->insert(out->end(), bytes, bytes + size);
    return true;
  }
}

//
// デストラクタ
//
PassthroughCamera::~PassthroughCamera()
{
  close();
}

//
// 左右のパススルーカメラを開いて取得を開始する
//
bool PassthroughCamera::open(int width, int height, int quality, double fps, const VideoSettings& video)
{
  close();

  this->video = video;
  unitsOverflowed = false;

  this->width = width;
  this->height = height;
  this->quality = std::clamp(quality, 0, 100);
  this->fps = fps;
  minInterval = fps > 0.0 ? 1.0 / fps : 0.0;
  failed = false;

  manager = ACameraManager_create();
  if (!manager) return false;

  // パススルーカメラを探す
  std::string cameraId[camCount];
  ACameraIdList* list{ nullptr };
  if (ACameraManager_getCameraIdList(manager, &list) == ACAMERA_OK && list)
  {
    for (int i = 0; i < list->numCameras; ++i)
    {
      const char* const id{ list->cameraIds[i] };
      ACameraMetadata* meta{ nullptr };
      if (ACameraManager_getCameraCharacteristics(manager, id, &meta) != ACAMERA_OK) continue;

      int position{ -1 }, facing{ -1 };
      getInt(meta, metaCameraPositionTag, position);
      getInt(meta, ACAMERA_LENS_FACING, facing);
      LOGI("camera %s: facing=%d, meta position=%d", id, facing, position);

      if (position >= 0 && position < camCount && cameraId[position].empty()) cameraId[position] = id;
      ACameraMetadata_free(meta);
    }
    ACameraManager_deleteCameraIdList(list);
  }

  // ベンダータグが無ければ既知の ID を使う
  if (cameraId[0].empty() && cameraId[1].empty())
  {
    cameraId[0] = fallbackCameraId[0];
    cameraId[1] = fallbackCameraId[1];
  }

  // 右カメラしか見つからなければ単眼として左に回す
  if (cameraId[0].empty()) std::swap(cameraId[0], cameraId[1]);

  // 画像の大きさ、撮影時刻の基準、内部パラメータを調べる
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (cameraId[cam].empty()) continue;
    ACameraMetadata* meta{ nullptr };
    if (ACameraManager_getCameraCharacteristics(manager, cameraId[cam].c_str(), &meta) != ACAMERA_OK) continue;
    if (cam == 0)
    {
      selectSize(meta, this->width, this->height);
      int source{ ACAMERA_SENSOR_INFO_TIMESTAMP_SOURCE_UNKNOWN };
      getInt(meta, ACAMERA_SENSOR_INFO_TIMESTAMP_SOURCE, source);
      bootTime = source == ACAMERA_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    }
    logIntrinsics(meta, cameraId[cam].c_str(), this->width, this->height);
    ACameraMetadata_free(meta);
  }

  // 取得を開始する
  running = true;
  eyeCount = 0;
  if (!openEye(eyes[0], cameraId[0]))
  {
    close();
    return false;
  }
  eyeCount = 1;
  if (!cameraId[1].empty())
  {
    if (openEye(eyes[1], cameraId[1])) eyeCount = 2;
    else LOGW("right passthrough camera %s is not available; sending monocular images", cameraId[1].c_str());
  }

  // JPEG の場合は符号化スレッドを動かす (動画はハードウェアエンコーダが符号化する)
  if (!isVideo()) encoder = std::thread([this]() { encodeLoop(); });

  LOGI("passthrough camera: %d camera(s), %dx%d, %s", eyeCount, this->width, this->height,
    isVideo() ? (video.format == ted::IMAGE_HEVC ? "HEVC" : "H.264") : "JPEG");
  return true;
}

//
// カメラを開く
//
bool PassthroughCamera::openEye(Eye& eye, const std::string& id)
{
  eye.owner = this;
  eye.id = id;
  eye.fresh = false;

  eye.deviceCallbacks = ACameraDevice_StateCallbacks{ &eye, onDisconnected, onError };
  eye.sessionCallbacks = ACameraCaptureSession_stateCallbacks{ &eye, onSessionState, onSessionState,
    onSessionState };
  eye.imageListener = AImageReader_ImageListener{ &eye, onImageAvailable };

  const auto fail = [this, &eye](const char* what, int status)
  {
    LOGE("camera %s: %s failed (%d)", eye.id.c_str(), what, status);
    closeEye(eye);
    return false;
  };

  if (isVideo())
  {
    // カメラの出力先をハードウェアエンコーダの入力 Surface にする
    const int index{ static_cast<int>(&eye - eyes) };
    eye.encoder = std::make_unique<VideoEncoder>();
    const auto callback = [this, index](VideoEncoder::Unit&& unit) { pushVideoUnit(index, std::move(unit)); };
    const bool ok{ eye.encoder->open(video.format, width, height, video.bitrate, fps, video.keyframeInterval,
      callback) };
    if (!ok) return fail("VideoEncoder::open", -1);
    eye.window = eye.encoder->getInputSurface();
  }
  else
  {
    media_status_t media{ AImageReader_new(width, height, AIMAGE_FORMAT_YUV_420_888, 4, &eye.reader) };
    if (media != AMEDIA_OK) return fail("AImageReader_new", media);
    media = AImageReader_setImageListener(eye.reader, &eye.imageListener);
    if (media != AMEDIA_OK) return fail("AImageReader_setImageListener", media);
    media = AImageReader_getWindow(eye.reader, &eye.window);
    if (media != AMEDIA_OK) return fail("AImageReader_getWindow", media);
  }

  camera_status_t status{ ACameraManager_openCamera(manager, id.c_str(), &eye.deviceCallbacks, &eye.device) };
  if (status != ACAMERA_OK) return fail("ACameraManager_openCamera", status);

  status = ACaptureSessionOutputContainer_create(&eye.container);
  if (status != ACAMERA_OK) return fail("ACaptureSessionOutputContainer_create", status);
  status = ACaptureSessionOutput_create(eye.window, &eye.output);
  if (status != ACAMERA_OK) return fail("ACaptureSessionOutput_create", status);
  status = ACaptureSessionOutputContainer_add(eye.container, eye.output);
  if (status != ACAMERA_OK) return fail("ACaptureSessionOutputContainer_add", status);
  status = ACameraOutputTarget_create(eye.window, &eye.target);
  if (status != ACAMERA_OK) return fail("ACameraOutputTarget_create", status);
  status = ACameraDevice_createCaptureRequest(eye.device, TEMPLATE_PREVIEW, &eye.request);
  if (status != ACAMERA_OK) return fail("ACameraDevice_createCaptureRequest", status);
  status = ACaptureRequest_addTarget(eye.request, eye.target);
  if (status != ACAMERA_OK) return fail("ACaptureRequest_addTarget", status);
  status = ACameraDevice_createCaptureSession(eye.device, eye.container, &eye.sessionCallbacks, &eye.session);
  if (status != ACAMERA_OK) return fail("ACameraDevice_createCaptureSession", status);
  status = ACameraCaptureSession_setRepeatingRequest(eye.session, nullptr, 1, &eye.request, nullptr);
  if (status != ACAMERA_OK) return fail("ACameraCaptureSession_setRepeatingRequest", status);

  LOGI("camera %s opened", id.c_str());
  return true;
}

//
// カメラを閉じる
//
void PassthroughCamera::closeEye(Eye& eye)
{
  // 閉じている間に新しい画像のコールバックが来ないようにする
  if (eye.reader)
  {
    AImageReader_ImageListener none{ nullptr, nullptr };
    AImageReader_setImageListener(eye.reader, &none);
  }
  if (eye.session)
  {
    ACameraCaptureSession_stopRepeating(eye.session);
    ACameraCaptureSession_close(eye.session);
    eye.session = nullptr;
  }
  if (eye.device)
  {
    ACameraDevice_close(eye.device);
    eye.device = nullptr;
  }
  if (eye.request)
  {
    ACaptureRequest_free(eye.request);
    eye.request = nullptr;
  }
  if (eye.target)
  {
    ACameraOutputTarget_free(eye.target);
    eye.target = nullptr;
  }
  if (eye.container && eye.output) ACaptureSessionOutputContainer_remove(eye.container, eye.output);
  if (eye.output)
  {
    ACaptureSessionOutput_free(eye.output);
    eye.output = nullptr;
  }
  if (eye.container)
  {
    ACaptureSessionOutputContainer_free(eye.container);
    eye.container = nullptr;
  }
  if (eye.reader)
  {
    // ImageReader が所有するウィンドウも一緒に破棄される
    AImageReader_delete(eye.reader);
    eye.reader = nullptr;
    eye.window = nullptr;
  }
  {
    std::lock_guard<std::mutex> lock{ encoderMutex };
    if (eye.encoder)
    {
      // カメラを閉じてからエンコーダを止める (入力 Surface はエンコーダが所有する)
      eye.encoder->close();
      eye.encoder.reset();
      eye.window = nullptr;
    }
  }

  std::lock_guard<std::mutex> lock{ yuvMutex };
  eye.fresh = false;
}

//
// カメラを閉じる
//
void PassthroughCamera::close()
{
  running = false;
  yuvReady.notify_all();
  if (encoder.joinable()) encoder.join();

  for (auto& eye : eyes) closeEye(eye);
  eyeCount = 0;

  if (manager)
  {
    ACameraManager_delete(manager);
    manager = nullptr;
  }

  {
    std::lock_guard<std::mutex> lock{ frameMutex };
    latest = Frame{};
  }

  std::lock_guard<std::mutex> lock{ unitMutex };
  units.clear();
}

//
// エンコーダが出力したアクセスユニットを保存する (エンコーダの出力スレッドから呼ばれる)
//
void PassthroughCamera::pushVideoUnit(int eye, VideoEncoder::Unit&& unit)
{
  // 撮影時刻を CLOCK_MONOTONIC に揃える
  std::int64_t timestamp{ unit.timestamp };
  if (bootTime) timestamp -= now(CLOCK_BOOTTIME) - now(CLOCK_MONOTONIC);

  std::lock_guard<std::mutex> lock{ unitMutex };

  // 描画が止まっていて取り出されなければ捨てる (受信側は欠落を検出してキーフレームを待つ)
  constexpr std::size_t maxUnits{ 60 };
  if (units.size() >= maxUnits)
  {
    units.clear();
    unitsOverflowed = true;
  }
  units.push_back(VideoUnit{ eye, std::move(unit.payload), timestamp, unit.keyframe });
}

//
// 符号化した動画のアクセスユニットをすべて取り出す
//
bool PassthroughCamera::takeVideoUnits(std::vector<VideoUnit>& out)
{
  out.clear();
  std::lock_guard<std::mutex> lock{ unitMutex };
  out.assign(std::make_move_iterator(units.begin()), std::make_move_iterator(units.end()));
  units.clear();
  return !out.empty();
}

//
// 動画の次のフレームをキーフレームにする
//
bool PassthroughCamera::requestKeyframe()
{
  if (!isVideo()) return false;
  std::lock_guard<std::mutex> lock{ encoderMutex };
  int count{ 0 };
  int success{ 0 };
  for (auto& eye : eyes)
  {
    if (eye.encoder)
    {
      ++count;
      if (eye.encoder->requestKeyframe()) ++success;
    }
  }
  return count > 0 && count == success;
}

//
// 取得中かどうか
//
bool PassthroughCamera::isRunning() const
{
  return running && !failed;
}

//
// 前回より新しい画像の組があれば取り出す
//
bool PassthroughCamera::getLatest(Frame& frame, std::uint64_t lastSequence) const
{
  std::lock_guard<std::mutex> lock{ frameMutex };
  if (latest.sequence == lastSequence || !latest.jpeg[0]) return false;
  frame = latest;
  return true;
}

//
// ImageReader のコールバック
//
void PassthroughCamera::onImageAvailable(void* context, AImageReader* reader)
{
  auto* const eye{ static_cast<Eye*>(context) };
  auto* const self{ eye->owner };

  AImage* image{ nullptr };
  if (AImageReader_acquireLatestImage(reader, &image) != AMEDIA_OK || !image) return;

  std::int32_t w{ 0 }, h{ 0 }, planes{ 0 };
  std::int64_t timestamp{ 0 };
  AImage_getWidth(image, &w);
  AImage_getHeight(image, &h);
  AImage_getNumberOfPlanes(image, &planes);
  AImage_getTimestamp(image, &timestamp);

  if (self->running && w == self->width && h == self->height && planes >= 3)
  {
    std::uint8_t* data[3]{};
    int length[3]{};
    std::int32_t rowStride[3]{}, pixelStride[3]{};
    bool ok{ true };
    for (int p = 0; p < 3; ++p)
    {
      ok = ok && AImage_getPlaneData(image, p, &data[p], &length[p]) == AMEDIA_OK
        && AImage_getPlaneRowStride(image, p, &rowStride[p]) == AMEDIA_OK;
      if (p > 0) ok = ok && AImage_getPlanePixelStride(image, p, &pixelStride[p]) == AMEDIA_OK;
    }

    if (ok)
    {
      const int cw{ (w + 1) / 2 }, ch{ (h + 1) / 2 };
      std::lock_guard<std::mutex> lock{ self->yuvMutex };
      auto& yuv{ eye->yuv };
      yuv.y.resize(static_cast<std::size_t>(w) * h);
      yuv.u.resize(static_cast<std::size_t>(cw) * ch);
      yuv.v.resize(static_cast<std::size_t>(cw) * ch);

      // 行ごとの詰め物を取り除いて Y を複製する
      for (int row = 0; row < h; ++row)
        std::memcpy(yuv.y.data() + static_cast<std::size_t>(row) * w,
          data[0] + static_cast<std::size_t>(row) * rowStride[0], w);

      // U と V は画素間隔 (NV12/NV21 なら 2) を考慮して取り出す
      for (int row = 0; row < ch; ++row)
      {
        const std::uint8_t* const su{ data[1] + static_cast<std::size_t>(row) * rowStride[1] };
        const std::uint8_t* const sv{ data[2] + static_cast<std::size_t>(row) * rowStride[2] };
        std::uint8_t* const du{ yuv.u.data() + static_cast<std::size_t>(row) * cw };
        std::uint8_t* const dv{ yuv.v.data() + static_cast<std::size_t>(row) * cw };
        for (int col = 0; col < cw; ++col)
        {
          du[col] = su[col * pixelStride[1]];
          dv[col] = sv[col * pixelStride[2]];
        }
      }

      yuv.timestamp = timestamp;
      eye->fresh = true;
      self->yuvReady.notify_one();
    }
  }

  AImage_delete(image);
}

//
// カメラが切断された
//
void PassthroughCamera::onDisconnected(void* context, ACameraDevice*)
{
  auto* const eye{ static_cast<Eye*>(context) };
  LOGW("camera %s disconnected", eye->id.c_str());
  eye->owner->failed = true;
}

//
// カメラでエラーが起きた
//
void PassthroughCamera::onError(void* context, ACameraDevice*, int error)
{
  auto* const eye{ static_cast<Eye*>(context) };
  LOGE("camera %s error %d", eye->id.c_str(), error);
  eye->owner->failed = true;
}

//
// キャプチャセッションの状態変化 (使わない)
//
void PassthroughCamera::onSessionState(void*, ACameraCaptureSession*)
{
}

//
// 符号化スレッド
//
void PassthroughCamera::encodeLoop()
{
  Yuv yuv[camCount];
  std::vector<std::uint8_t> rgba;
  auto last{ std::chrono::steady_clock::now() - std::chrono::hours(1) };
  std::uint64_t sequence{ 0 };

  while (running)
  {
    // フレームレートの上限を超えないように待つ
    const auto next{ last + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
      std::chrono::duration<double>(minInterval)) };
    if (std::chrono::steady_clock::now() < next) std::this_thread::sleep_until(next);

    bool haveRight{ false };
    {
      // 左右とも新しい画像が届くまで待つ (右が遅れたら 100ms で左だけ使う)
      std::unique_lock<std::mutex> lock{ yuvMutex };
      const auto ready = [this]()
      {
        return !running || (eyes[0].fresh && (eyeCount < 2 || eyes[1].fresh));
      };
      yuvReady.wait_for(lock, std::chrono::milliseconds(100), ready);
      if (!running) break;
      if (!eyes[0].fresh) continue;

      std::swap(yuv[0], eyes[0].yuv);
      eyes[0].fresh = false;
      if (eyeCount > 1 && eyes[1].fresh)
      {
        std::swap(yuv[1], eyes[1].yuv);
        eyes[1].fresh = false;
        haveRight = true;
      }
    }
    last = std::chrono::steady_clock::now();

    Frame frame;
    frame.jpeg[0] = encode(yuv[0], rgba);
    if (!frame.jpeg[0]) continue;
    if (haveRight) frame.jpeg[1] = encode(yuv[1], rgba);

    // 撮影時刻を CLOCK_MONOTONIC に揃える
    frame.timestamp = yuv[0].timestamp;
    if (bootTime) frame.timestamp -= now(CLOCK_BOOTTIME) - now(CLOCK_MONOTONIC);
    frame.sequence = ++sequence;

    std::lock_guard<std::mutex> lock{ frameMutex };
    latest = std::move(frame);
  }
}

//
// YUV を RGBA に変換して JPEG に符号化する
//
PassthroughCamera::Image PassthroughCamera::encode(const Yuv& yuv, std::vector<std::uint8_t>& rgba) const
{
  const int w{ width }, h{ height }, cw{ (width + 1) / 2 };
  if (yuv.y.size() < static_cast<std::size_t>(w) * h) return nullptr;
  rgba.resize(static_cast<std::size_t>(w) * h * 4);

  // BT.601 フルレンジ (JFIF) の YCbCr から RGB への変換を固定小数点で行う
  const auto clamp = [](int value) { return static_cast<std::uint8_t>(std::clamp(value, 0, 255)); };
  for (int row = 0; row < h; ++row)
  {
    const std::uint8_t* const py{ yuv.y.data() + static_cast<std::size_t>(row) * w };
    const std::uint8_t* const pu{ yuv.u.data() + static_cast<std::size_t>(row / 2) * cw };
    const std::uint8_t* const pv{ yuv.v.data() + static_cast<std::size_t>(row / 2) * cw };
    std::uint8_t* out{ rgba.data() + static_cast<std::size_t>(row) * w * 4 };
    for (int col = 0; col < w; ++col)
    {
      const int y{ py[col] << 16 };
      const int u{ pu[col / 2] - 128 }, v{ pv[col / 2] - 128 };
      *out++ = clamp((y + 91881 * v + 32768) >> 16);
      *out++ = clamp((y - 22554 * u - 46802 * v + 32768) >> 16);
      *out++ = clamp((y + 116130 * u + 32768) >> 16);
      *out++ = 255;
    }
  }

  AndroidBitmapInfo info{};
  info.width = static_cast<std::uint32_t>(w);
  info.height = static_cast<std::uint32_t>(h);
  info.stride = static_cast<std::uint32_t>(w * 4);
  info.format = ANDROID_BITMAP_FORMAT_RGBA_8888;
  info.flags = ANDROID_BITMAP_FLAGS_ALPHA_OPAQUE;

  auto jpeg{ std::make_shared<std::vector<std::uint8_t>>() };
  jpeg->reserve(256 * 1024);
  if (AndroidBitmap_compress(&info, ADATASPACE_SRGB, rgba.data(), ANDROID_BITMAP_COMPRESS_FORMAT_JPEG,
    quality, jpeg.get(), writeJpeg) != ANDROID_BITMAP_RESULT_SUCCESS)
  {
    LOGE("AndroidBitmap_compress failed");
    return nullptr;
  }

  return jpeg;
}

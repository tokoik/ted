//
// Leap Motion 関連の処理
//
#include "LeapListener.h"

// Leap Motion のライブラリファイル
#pragma comment (lib, "LeapC.lib")

// 標準ライブラリ
#include <iostream>

// 冗長なデバッグメッセージ
#undef VERBOSE

// 長さのスケール
constexpr GLfloat scale{ 0.001f };

// タイムアウト
constexpr unsigned int timeout{ 1000 };

// リトライ回数
constexpr unsigned int retry{ 5 };

//
// The following code contains code modified from Leap Motion SDK sample code.
// Therefore, this code follows the development license of Leap Motion, Inc.

/******************************************************************************\
* Copyright (C) 2012-2017 Leap Motion, Inc. All rights reserved.               *
* Leap Motion proprietary and confidential. Not for distribution.              *
* Use subject to the terms of the Leap Motion SDK Agreement available at       *
* https://developer.leapmotion.com/sdk_agreement, or another agreement         *
* between Leap Motion and you, your company or other organization.             *
\******************************************************************************/


/* Leap Motion status */
static bool IsConnected(false);
static volatile bool _isRunning(false);
static LEAP_CONNECTION connectionHandle(nullptr);
static std::unique_ptr<LEAP_TRACKING_EVENT> lastFrame(nullptr);
static std::unique_ptr<LEAP_DEVICE_INFO> lastDevice(nullptr);
static int32_t lastDrawnFrameId(0);
static volatile int32_t newestFrameId(0);
#if defined(LEAP_INTERPORATE_FRAME)
static LEAP_CLOCK_REBASER clockSynchronizer;
#endif

/* Thread */
static std::thread pollingThread;
static std::mutex dataLock;

/**
 * Translates eLeapRS result codes into a human-readable string.
 */
static const char* ResultString(eLeapRS r)
{
  switch (r)
  {
  case eLeapRS_Success:                  return "eLeapRS_Success";
  case eLeapRS_UnknownError:             return "eLeapRS_UnknownError";
  case eLeapRS_InvalidArgument:          return "eLeapRS_InvalidArgument";
  case eLeapRS_InsufficientResources:    return "eLeapRS_InsufficientResources";
  case eLeapRS_InsufficientBuffer:       return "eLeapRS_InsufficientBuffer";
  case eLeapRS_Timeout:                  return "eLeapRS_Timeout";
  case eLeapRS_NotConnected:             return "eLeapRS_NotConnected";
  case eLeapRS_HandshakeIncomplete:      return "eLeapRS_HandshakeIncomplete";
  case eLeapRS_BufferSizeOverflow:       return "eLeapRS_BufferSizeOverflow";
  case eLeapRS_ProtocolError:            return "eLeapRS_ProtocolError";
  case eLeapRS_InvalidClientID:          return "eLeapRS_InvalidClientID";
  case eLeapRS_UnexpectedClosed:         return "eLeapRS_UnexpectedClosed";
  case eLeapRS_UnknownImageFrameRequest: return "eLeapRS_UnknownImageFrameRequest";
  case eLeapRS_UnknownTrackingFrameID:   return "eLeapRS_UnknownTrackingFrameID";
  case eLeapRS_RoutineIsNotSeer:         return "eLeapRS_RoutineIsNotSeer";
  case eLeapRS_TimestampTooEarly:        return "eLeapRS_TimestampTooEarly";
  case eLeapRS_ConcurrentPoll:           return "eLeapRS_ConcurrentPoll";
  case eLeapRS_NotAvailable:             return "eLeapRS_NotAvailable";
  case eLeapRS_NotStreaming:             return "eLeapRS_NotStreaming";
  case eLeapRS_CannotOpenDevice:         return "eLeapRS_CannotOpenDevice";
  default:                               return "unknown result type.";
  }
}

/**
 * Caches the newest frame by copying the tracking event struct returned by
 * LeapC.
 */
static void setFrame(const LEAP_TRACKING_EVENT* frame)
{
  std::lock_guard<std::mutex> lock(dataLock);
  if (!lastFrame.get()) lastFrame.reset(new LEAP_TRACKING_EVENT);
  *lastFrame = *frame;
}

/**
 * Returns a pointer to the cached tracking frame.
 */
static const LEAP_TRACKING_EVENT* getFrame()
{
  LEAP_TRACKING_EVENT* currentFrame;

  std::lock_guard<std::mutex> lock(dataLock);
  currentFrame = lastFrame.get();

  return currentFrame;
}

/**
 * Caches the last device found by copying the device info struct returned by
 * LeapC.
 */
static void setDevice(const LEAP_DEVICE_INFO* deviceProps)
{
  std::lock_guard<std::mutex> lock(dataLock);
  if (lastDevice)
  {
    delete[] lastDevice->serial;
  }
  else
  {
    lastDevice.reset(new LEAP_DEVICE_INFO);
  }
  *lastDevice = *deviceProps;
  lastDevice->serial = new char[deviceProps->serial_length];
  memcpy(lastDevice->serial, deviceProps->serial, deviceProps->serial_length);
}

/**
 * Returns a pointer to the cached device info.
 */
static LEAP_DEVICE_INFO* getDeviceProperties()
{
  LEAP_DEVICE_INFO* currentDevice;

  std::lock_guard<std::mutex> lock(dataLock);
  currentDevice = lastDevice.get();

  return currentDevice;
}

/** Called by serviceMessageLoop() when a connection event is returned by LeapPollConnection(). */
static void handleConnectionEvent(const LEAP_CONNECTION_EVENT* connection_event)
{
  IsConnected = true;
#if defined(DEBUG)
  std::cerr << "Leap: Connection established.\n";
#endif
}

/** Called by serviceMessageLoop() when a connection lost event is returned by LeapPollConnection(). */
static void handleConnectionLostEvent(const LEAP_CONNECTION_LOST_EVENT* connection_lost_event)
{
  IsConnected = false;
#if defined(DEBUG)
  std::cerr << "Leap: Connection lost.\n";
#endif
}

/**
 * Called by serviceMessageLoop() when a device event is returned by LeapPollConnection()
 * Demonstrates how to access device properties.
 */
static void handleDeviceEvent(const LEAP_DEVICE_EVENT* device_event)
{
  LEAP_DEVICE deviceHandle;

  //Open device using LEAP_DEVICE_REF from event struct.
  eLeapRS result(LeapOpenDevice(device_event->device, &deviceHandle));
  if (result != eLeapRS_Success)
  {
#if defined(DEBUG)
    std::cerr << "Leap: Could not open device " << ResultString(result) << ".\n";
#endif
    return;
  }

  //Create a struct to hold the device properties, we have to provide a buffer for the serial string
  LEAP_DEVICE_INFO deviceProperties{ sizeof(deviceProperties) };

  // Start with a length of 1 (pretending we don't know a priori what the length is).
  // Currently device serial numbers are all the same length, but that could change in the future
  deviceProperties.serial_length = 1;
  deviceProperties.serial = new char[deviceProperties.serial_length];

  //This will fail since the serial buffer is only 1 character long
  // But deviceProperties is updated to contain the required buffer length
  result = LeapGetDeviceInfo(deviceHandle, &deviceProperties);
  if (result == eLeapRS_InsufficientBuffer)
  {
    //try again with correct buffer size
    delete[] deviceProperties.serial;
    deviceProperties.serial = new char[deviceProperties.serial_length];
    result = LeapGetDeviceInfo(deviceHandle, &deviceProperties);
    if (result != eLeapRS_Success)
    {
#if defined(DEBUG)
      std::cerr << "Leap: Failed to get device info " << ResultString(result) << ".\n";
#endif
      delete[] deviceProperties.serial;
      deviceProperties.serial = nullptr;
      return;
    }
  }

  setDevice(&deviceProperties);

#if defined(DEBUG)
  std::cerr << "Leap: Found device " << deviceProperties.serial << ".\n";
#endif

  delete[] deviceProperties.serial;
  deviceProperties.serial = nullptr;

  LeapCloseDevice(deviceHandle);
}

/** Called by serviceMessageLoop() when a device lost event is returned by LeapPollConnection(). */
static void handleDeviceLostEvent(const LEAP_DEVICE_EVENT* device_event)
{
#if defined(DEBUG)
  std::cerr << "Leap: Device lost.\n";
#endif
}

/** Called by serviceMessageLoop() when a device failure event is returned by LeapPollConnection(). */
static void handleDeviceFailureEvent(const LEAP_DEVICE_FAILURE_EVENT* device_failure_event)
{
#if defined(DEBUG)
  std::cerr << "Leap: Device failure: ";
  switch (device_failure_event->status)
  {
  case eLeapDeviceStatus_Streaming:
    std::cerr << "The device is sending out frames.\n";
    break;
  case eLeapDeviceStatus_Paused:
    std::cerr << "Device streaming has been paused.\n";
    break;
  case eLeapDeviceStatus_Robust:
    std::cerr << "There are known sources of infrared interference. "
      "Device has transitioned to robust mode in order to compensate.\n";
    break;
  case eLeapDeviceStatus_Smudged:
    std::cerr << "The device's window is smudged, tracking may be degraded.\n";
    break;
  case eLeapDeviceStatus_LowResource:
    std::cerr << "The device has entered low-resource mode.\n";
    break;
  case eLeapDeviceStatus_UnknownFailure:
    std::cerr << "The device has failed, but the failure reason is not known.\n";
    break;
  case eLeapDeviceStatus_BadCalibration:
    std::cerr << "The device has a bad calibration record and cannot send frames.\n";
    break;
  case eLeapDeviceStatus_BadFirmware:
    std::cerr << " device reports corrupt firmware or cannot install a required firmware update.\n";
    break;
  case eLeapDeviceStatus_BadTransport:
    std::cerr << "The device USB connection is faulty.\n";
    break;
  case eLeapDeviceStatus_BadControl:
    std::cerr << "The device USB control interfaces failed to initialize.\n";
    break;
  default:
    std::cerr << "Unknown problem.\n";
    break;
  }
#endif
}

/** Called by serviceMessageLoop() when a tracking event is returned by LeapPollConnection(). */
static void handleTrackingEvent(const LEAP_TRACKING_EVENT* tracking_event)
{
  setFrame(tracking_event); //support polling tracking data from different thread
  newestFrameId = static_cast<int32_t>(tracking_event->tracking_frame_id);

#if defined(DEBUG) && defined(VERBOSE)
  std::cerr
    << "Leap: tracking frame "
    << tracking_event->info.frame_id
    << " with "
    << tracking_event->nHands
    << ".\n";

  for (uint32_t h = 0; h < tracking_event->nHands; ++h)
  {
    const LEAP_HAND* hand(&tracking_event->pHands[h]);
    std::cerr << "    Hand id "
      << hand->id
      << " is a "
      << (hand->type == eLeapHandType_Left ? "left" : "right")
      << " hand with position ("
      << hand->palm.position.x
      << ", "
      << hand->palm.position.y
      << ", "
      << hand->palm.position.z
      << ").\n";
  }
#endif
}

/** Called by serviceMessageLoop() when a log event is returned by LeapPollConnection(). */
static void handleLogEvent(const LEAP_LOG_EVENT* log_event)
{
#if defined(DEBUG)
  std::cerr << "Leap: Log: severity = ";
  switch (log_event->severity) {
  case eLeapLogSeverity_Critical:
    std::cerr << "Critical";
    break;
  case eLeapLogSeverity_Warning:
    std::cerr << "Warning";
    break;
  case eLeapLogSeverity_Information:
    std::cerr << "Info";
    break;
  default:
    break;
  }

  std::cerr
    << ", timestamp " << log_event->timestamp
    << ": " << log_event->message
    << ".\n";
#endif
}

/** Called by serviceMessageLoop() when a log event is returned by LeapPollConnection(). */
static void handleLogEvents(const LEAP_LOG_EVENTS* log_events)
{
#if defined(DEBUG)
  std::cerr << "Leap: Log:\n";
  for (int i = 0; i < static_cast<int>(log_events->nEvents); i++)
  {
    const LEAP_LOG_EVENT* log_event(&log_events->events[i]);

    std::cerr << "  severity = ";
    switch (log_event->severity) {
    case eLeapLogSeverity_Critical:
      std::cerr << "Critical";
      break;
    case eLeapLogSeverity_Warning:
      std::cerr << "Warning";
      break;
    case eLeapLogSeverity_Information:
      std::cerr << "Info";
      break;
    default:
      break;
    }

    std::cerr
      << ", timestamp " << log_event->timestamp
      << ": " << log_event->message
      << ".\n";
  }
#endif
}

/** Called by serviceMessageLoop() when a policy event is returned by LeapPollConnection(). */
static void handlePolicyEvent(const LEAP_POLICY_EVENT* policy_event)
{
#if defined(DEBUG)
  std::cerr << "Leap: policy " << policy_event->current_policy << ".\n";
#endif
}

/** Called by serviceMessageLoop() when a config change event is returned by LeapPollConnection(). */
static void handleConfigChangeEvent(const LEAP_CONFIG_CHANGE_EVENT* config_change_event)
{
#if defined(DEBUG)
  std::cerr
    << "Leap: config change: requset ID " << config_change_event->requestID
    << ", status" << config_change_event->status
    << ".\n";
#endif
}

/** Called by serviceMessageLoop() when a config response event is returned by LeapPollConnection(). */
static void handleConfigResponseEvent(const LEAP_CONFIG_RESPONSE_EVENT* config_response_event)
{
#if defined(DEBUG)
  std::cerr
    << "Leap: config response: requset ID " << config_response_event->requestID
    << ".\n";
#endif
}

/** Called by serviceMessageLoop() when a point mapping change event is returned by LeapPollConnection(). */
static void handleImageEvent(const LEAP_IMAGE_EVENT* image_event)
{
#if defined(DEBUG)
  std::cerr
    << "Leap: Image "
    << image_event->info.frame_id
    << ", Left: " << image_event->image[0].properties.width
    << " x "
    << image_event->image[0].properties.height
    << "(bpp="
    << image_event->image[0].properties.bpp * 8
    << "), Right: "
    << image_event->image[1].properties.width
    << " x "
    << image_event->image[1].properties.height
    << "(bpp="
    << image_event->image[1].properties.bpp * 8
    << ")\n";
#endif
}

/** Called by serviceMessageLoop() when a point mapping change event is returned by LeapPollConnection(). */
static void handlePointMappingChangeEvent(const LEAP_POINT_MAPPING_CHANGE_EVENT* point_mapping_change_event)
{
#if defined(DEBUG)
  std::cerr << "Leap: mapping change.\n";
#endif
}

/** Called by serviceMessageLoop() when a point mapping change event is returned by LeapPollConnection(). */
static void handleHeadPoseEvent(const LEAP_HEAD_POSE_EVENT* head_pose_event)
{
#if defined(DEBUG)
  std::cerr
    << "Leap: head pose: position ("
    << head_pose_event->head_position.x
    << ", "
    << head_pose_event->head_position.y
    << ", "
    << head_pose_event->head_position.z
    << "), orientation ("
    << head_pose_event->head_orientation.w
    << ", "
    << head_pose_event->head_orientation.x
    << ", "
    << head_pose_event->head_orientation.y
    << ", "
    << head_pose_event->head_orientation.z
    << ")\n";
#endif
}

/**
 * Services the LeapC message pump by calling LeapPollConnection().
 * The average polling time is determined by the framerate of the Leap Motion service.
 */
static void serviceMessageLoop()
{
  do
  {
    LEAP_CONNECTION_MESSAGE msg;

    const eLeapRS result(LeapPollConnection(connectionHandle, timeout, &msg));
    if (result != eLeapRS_Success)
    {
#if defined(DEBUG)
      std::cerr << "LeapC PollConnection call was " << ResultString(result) << ".\n";
#endif
      continue;
    }

    switch (msg.type)
    {
    case eLeapEventType_Connection:
      handleConnectionEvent(msg.connection_event);
      break;
    case eLeapEventType_ConnectionLost:
      handleConnectionLostEvent(msg.connection_lost_event);
      break;
    case eLeapEventType_Device:
      handleDeviceEvent(msg.device_event);
      break;
    case eLeapEventType_DeviceLost:
      handleDeviceLostEvent(msg.device_event);
      break;
    case eLeapEventType_DeviceFailure:
      handleDeviceFailureEvent(msg.device_failure_event);
      break;
    case eLeapEventType_Tracking:
      handleTrackingEvent(msg.tracking_event);
      break;
    case eLeapEventType_ImageComplete:
      // Ignore
      break;
    case eLeapEventType_ImageRequestError:
      // Ignore
      break;
    case eLeapEventType_LogEvent:
      handleLogEvent(msg.log_event);
      break;
    case eLeapEventType_Policy:
      handlePolicyEvent(msg.policy_event);
      break;
    case eLeapEventType_ConfigChange:
      handleConfigChangeEvent(msg.config_change_event);
      break;
    case eLeapEventType_ConfigResponse:
      handleConfigResponseEvent(msg.config_response_event);
      break;
    case eLeapEventType_Image:
      handleImageEvent(msg.image_event);
      break;
    case eLeapEventType_PointMappingChange:
      handlePointMappingChangeEvent(msg.point_mapping_change_event);
      break;
    case eLeapEventType_LogEvents:
      handleLogEvents(msg.log_events);
      break;
    case eLeapEventType_HeadPose:
      handleHeadPoseEvent(msg.head_pose_event);
      break;
    default:
      // discard unknown message types
#if defined(DEBUG)
      std::cerr << "Leap: Unhandled message type " << msg.type << ".\n";
#endif
      break;
    }
  } while (_isRunning);
}

/** Close the connection and let message thread function end. */
static void closeConnectionHandle(LEAP_CONNECTION* connectionHandle)
{
  LeapDestroyConnection(*connectionHandle);
  _isRunning = false;
}

// コンストラクタ
LeapListener::LeapListener()
{
}

// デストラクタ
LeapListener::~LeapListener()
{
  // Leap Motion から切断する
  destroyConnection();
}

/**
 * Creates the connection handle and opens a connection to the Leap Motion
 * service. On success, creates a thread to service the LeapC message pump.
 */
const LEAP_CONNECTION* LeapListener::openConnection()
{
  // serviceMessageLoop() が既に起動していたら何もしない
  if (_isRunning) return &connectionHandle;

  // connectionHandle が得られていなければ
  if (!connectionHandle)
  {
    // connectionHandle を作成する
    const eLeapRS resultCh{ LeapCreateConnection(NULL, &connectionHandle) };

    // connectionHandle が作成できなかったらエラー
    if (resultCh != eLeapRS_Success) return nullptr;
  }

  // connectionHandle に接続する
  const eLeapRS result(LeapOpenConnection(connectionHandle));

  // connectionHandle に接続できなかったらエラー
  if (result != eLeapRS_Success) return nullptr;

  // servceMessageLoop() でポーリングして接続完了を待つ
  unsigned int count{ 0 };
  do
  {
    serviceMessageLoop();
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  } while (!IsConnected && ++count < retry);

  // リトライ回数に達していたらあきらめる
  if (count == retry) return nullptr;

  // serviceMessageLoopLeap() のループが回るようにして
  _isRunning = true;

  // serviceMessage() を別スレッドでループさせる
  pollingThread = std::thread(serviceMessageLoop);

  // HMD 用に最適化する
  LeapSetPolicyFlags(connectionHandle, eLeapPolicyFlag_OptimizeHMD, 0);

#if defined(LEAP_INTERPORATE_FRAME)
  // クロックシンセサイザを生成する
  LeapCreateClockRebaser(&clockSynchronizer);
#endif

  // connectionHandle を返す
  return &connectionHandle;
}

void LeapListener::closeConnection()
{
  // serviceMessageLoop() が動いていなかったら何もしない
  if (!_isRunning) return;

  // serviceMessageLoop() を止めて
  _isRunning = false;

#if defined(LEAP_INTERPORATE_FRAME)
  // クロックシンセサイザを破棄する
  LeapDestroyClockRebaser(&clockSynchronizer);
#endif

  // Leap Motion との接続を閉じて
  LeapCloseConnection(connectionHandle);

  // 少し待ってから
  std::this_thread::sleep_for(std::chrono::milliseconds(250));

  // スレッドが停止するのを待つ
  if (pollingThread.joinable()) pollingThread.join();
}

void LeapListener::destroyConnection()
{
  closeConnection();
  LeapDestroyConnection(connectionHandle);
}

#if defined(LEAP_INTERPORATE_FRAME)
// Leap Motion と CPU の同期をとる
void LeapListener::synchronize()
{
  // アプリケーションの現在時刻を求める (マイクロ秒)
  const clock_t cpuTime(static_cast<clock_t>(0.000001 * clock() / CLOCKS_PER_SEC));

  // Leap Motion の時刻を同期する
  LeapUpdateRebase(clockSynchronizer, cpuTime, LeapGetNow());
}

static int64_t targetFrameTime = 0;
static uint64_t targetFrameSize = 0;
#endif

// 関節の変換行列のテーブルに値を取得する
void LeapListener::getHandPose(GgMatrix* matrix) const
{
#if defined(LEAP_INTERPORATE_FRAME)
  // アプリケーションの現在時刻を求める (マイクロ秒)
  const clock_t cpuTime(static_cast<clock_t>(0.000001 * clock() / CLOCKS_PER_SEC));

  //Translate application time to Leap time
  LeapRebaseClock(clockSynchronizer, cpuTime, &targetFrameTime);

  // 結果
  eLeapRS result;

  //Get the buffer size needed to hold the tracking data
  result = LeapGetFrameSize(connectionHandle, targetFrameTime, &targetFrameSize);
  if (result != eLeapRS_Success)
  {
#if defined(DEBUG)
    std::cerr << "LeapGetFrameSize() result was " << ResultString(result) << ".\n";
#endif
    return;
  }

  //Allocate enough memory
  const std::unique_ptr<LEAP_TRACKING_EVENT> frame(reinterpret_cast<LEAP_TRACKING_EVENT*>(new char[targetFrameSize]));

  //Get the frame
  result = LeapInterpolateFrame(connectionHandle, targetFrameTime, frame.get(), targetFrameSize);
  if (result != eLeapRS_Success)
  {
#if defined(DEBUG)
    std::cerr << "LeapInterpolateFrame() result was " << ResultString(result) << ".\n";
#endif
    return;
  }

#  if defined(DEBUG)
  std::cerr
    << "Frame "
    << static_cast<long long int>(frame->tracking_frame_id)
    << " with "
    << frame->nHands
    << " hands with delay of "
    << static_cast<long long int>(LeapGetNow()) - frame->info.timestamp
    << " microseconds.\n";
#  endif
#else
  // フレームが更新されていなかったら何もしない
  if (lastDrawnFrameId >= newestFrameId) return;
  lastDrawnFrameId = newestFrameId;

  // 保存されているフレームを取り出す
  const LEAP_TRACKING_EVENT* frame(getFrame());
  if (!frame) return;
#  if defined(DEBUG) && defined(VERBOSE)
  std::cerr << "Frame id: " << frame->info.frame_id
    << ", timestamp: " << frame->info.timestamp
    << ", hands: " << frame->nHands
    << "\n";
#  endif
#endif

  // 全ての腕について
  for (uint32_t h = 0; h < frame->nHands; ++h)
  {
    // h 番目の手のデータ
    const LEAP_HAND& hand(frame->pHands[h]);

    // 左手なら 0, 右手なら 1
    const int base(hand.type == eLeapHandType_Left ? 0 : 1);

#if defined(DEBUG) && defined(VERBOSE)
    std::cerr << "Hand id: " << hand.id << ", " << base << "\n";
    std::cerr << "Position = (" << hand.palm.position.x << ", " << hand.palm.position.y << ", " << hand.palm.position.z << ")\n";
#endif

    // 手のひらの位置
    const GLfloat handPos[]
    {
      hand.palm.position.x * scale,
      hand.palm.position.y * scale,
      hand.palm.position.z * scale
    };

    // 手のひらの法線
    const float* const& normal(hand.palm.normal.v);

    // 手のひらの方向
    const float* const& direction(hand.palm.direction.v);

    // 手のひらの法線と方向に直交するベクトル
    GLfloat tangent[3];
    ggCross(tangent, direction, normal);

    // 手のひらの変換行列を作成する
    const GLfloat mPalm[]
    {
      -tangent[0],   -tangent[2],   -tangent[1],   0.0f,
      -direction[0], -direction[2], -direction[1], 0.0f,
      -normal[0],    -normal[2],    -normal[1],    0.0f,
      -handPos[0],   -handPos[2],   -handPos[1],   1.0f
    };

    // 変換行列を共有メモリに格納する
    matrix[0 + base] = GgMatrix(mPalm);

    // 手首の位置
    const GLfloat wristPosition[]
    {
      hand.arm.next_joint.x * scale,
      hand.arm.next_joint.y * scale,
      hand.arm.next_joint.z * scale
    };

    // 腕の方向
    GLfloat armDirection[]
    {
      wristPosition[0] - hand.arm.prev_joint.x * scale,
      wristPosition[1] - hand.arm.prev_joint.y * scale,
      wristPosition[2] - hand.arm.prev_joint.z * scale
    };
    ggNormalize3(armDirection);

    // 腕の方向ベクトルと直交するベクトル
    GLfloat armTangent[3];
    ggCross(armTangent, armDirection, normal);
    ggNormalize3(armTangent);

    // 腕の法線ベクトル
    GLfloat  armNormal[3];
    ggCross(armNormal, armDirection, armTangent);

    // 変換行列を作成する
    const GLfloat mWrist[]
    {
      -armTangent[0],    -armTangent[2],    -armTangent[1],    0.0f,
      -armNormal[0],     -armNormal[2],     -armNormal[1],     0.0f,
      -armDirection[0],  -armDirection[2],  -armDirection[1],  0.0f,
      -wristPosition[0], -wristPosition[2], -wristPosition[1], 1.0f
    };

    // 手首の変換行列を共有メモリに格納する
    matrix[2 + base] = mWrist;

    // 全ての指について
    for (int d = 0; d < 5; ++d)
    {
      // 指
      const auto& digit(hand.digits[d]);

      // 全ての骨について
      for (int b = 0; b < 4; ++b)
      {
        // 骨
        const auto& bone(digit.bones[b]);

        // 骨の先端の位置
        const GLfloat bonePosition[]
        {
          bone.next_joint.x * scale,
          bone.next_joint.y * scale,
          bone.next_joint.z * scale
        };

        // 骨の方向
        GLfloat boneDirection[]
        {
          bone.next_joint.x - bone.prev_joint.x,
          bone.next_joint.y - bone.prev_joint.y,
          bone.next_joint.z - bone.prev_joint.z
        };
        ggNormalize3(boneDirection);

        // 骨の手のひらの法線方向
        GLfloat boneNormal[3];
        ggCross(boneNormal, boneDirection, tangent);
        ggNormalize3(boneNormal);

        // 骨の手のひらの接線方向
        GLfloat boneTangent[3];
        ggCross(boneTangent, boneNormal, boneDirection);

        // 変換行列を作成する
        const GLfloat mBone[]
        {
          -boneTangent[0],   -boneTangent[2],   -boneTangent[1],   0.0f,
          -boneNormal[0],    -boneNormal[2],    -boneNormal[1],    0.0f,
          -boneDirection[0], -boneDirection[2], -boneDirection[1], 0.0f,
          -bonePosition[0],  -bonePosition[2],  -bonePosition[1],  1.0f
        };

        // 変換行列を共有メモリに格納する
        matrix[4 + d * 8 + b * 2 + base] = GgMatrix(mBone);
      }
    }
  }
}

// 頭の姿勢の変換行列のテーブルに値を取得する
void LeapListener::getHeadPose(GgMatrix* matrixmatrix) const
{
}

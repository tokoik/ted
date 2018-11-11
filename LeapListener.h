#pragma once

//
// Leap Motion 関連の処理
//

// Leap Motion SDK
#include <LeapC.h>

// 補助プログラム
#include "gg.h"
using namespace gg;

// 標準ライブラリ
#include <thread>
#include <mutex>

// 関節の数
constexpr int jointCount((2 + 5 * 4) * 2);

//
// The following code contains code modified from Leap Motion SDK sample code.
// Therefore, this code follows the development license of Leap Motion, Inc.
//

/******************************************************************************\
* Copyright (C) 2012-2017 Leap Motion, Inc. All rights reserved.               *
* Leap Motion proprietary and confidential. Not for distribution.              *
* Use subject to the terms of the Leap Motion SDK Agreement available at       *
* https://developer.leapmotion.com/sdk_agreement, or another agreement         *
* between Leap Motion and you, your company or other organization.             *
\******************************************************************************/

class LeapListener
{
  // Leap Motion の状態
  static bool IsConnected;
  static volatile bool _isRunning;
  static LEAP_CONNECTION connectionHandle;
  static std::unique_ptr<LEAP_TRACKING_EVENT> lastFrame;
  static std::unique_ptr<LEAP_DEVICE_INFO> lastDevice;

  // スレッド
  static std::thread pollingThread;
  static std::mutex dataLock;

  /** Close the connection and let message thread function end. */
  static void closeConnectionHandle(LEAP_CONNECTION* connectionHandle);

  /** Called by serviceMessageLoop() when a connection event is returned by LeapPollConnection(). */
  static void handleConnectionEvent(const LEAP_CONNECTION_EVENT *connection_event);

  /** Called by serviceMessageLoop() when a connection lost event is returned by LeapPollConnection(). */
  static void handleConnectionLostEvent(const LEAP_CONNECTION_LOST_EVENT *connection_lost_event);

  /**
   * Called by serviceMessageLoop() when a device event is returned by LeapPollConnection()
   * Demonstrates how to access device properties.
   */
  static void handleDeviceEvent(const LEAP_DEVICE_EVENT *device_event);

  /** Called by serviceMessageLoop() when a device lost event is returned by LeapPollConnection(). */
  static void handleDeviceLostEvent(const LEAP_DEVICE_EVENT *device_event);

  /** Called by serviceMessageLoop() when a device failure event is returned by LeapPollConnection(). */
  static void handleDeviceFailureEvent(const LEAP_DEVICE_FAILURE_EVENT *device_failure_event);

  /** Called by serviceMessageLoop() when a tracking event is returned by LeapPollConnection(). */
  static void handleTrackingEvent(const LEAP_TRACKING_EVENT *tracking_event);

  /** Called by serviceMessageLoop() when a log event is returned by LeapPollConnection(). */
  static void handleLogEvent(const LEAP_LOG_EVENT *log_event);

  /** Called by serviceMessageLoop() when a log event is returned by LeapPollConnection(). */
  static void handleLogEvents(const LEAP_LOG_EVENTS *log_events);

  /** Called by serviceMessageLoop() when a policy event is returned by LeapPollConnection(). */
  static void handlePolicyEvent(const LEAP_POLICY_EVENT *policy_event);

  /** Called by serviceMessageLoop() when a config change event is returned by LeapPollConnection(). */
  static void handleConfigChangeEvent(const LEAP_CONFIG_CHANGE_EVENT *config_change_event);

  /** Called by serviceMessageLoop() when a config response event is returned by LeapPollConnection(). */
  static void handleConfigResponseEvent(const LEAP_CONFIG_RESPONSE_EVENT *config_response_event);

  /** Called by serviceMessageLoop() when a point mapping change event is returned by LeapPollConnection(). */
  static void handleImageEvent(const LEAP_IMAGE_EVENT *image_event);

  /** Called by serviceMessageLoop() when a point mapping change event is returned by LeapPollConnection(). */
  static void handlePointMappingChangeEvent(const LEAP_POINT_MAPPING_CHANGE_EVENT *point_mapping_change_event);

  /** Called by serviceMessageLoop() when a point mapping change event is returned by LeapPollConnection(). */
  static void handleHeadPoseEvent(const LEAP_HEAD_POSE_EVENT *head_pose_event);

  /**
   * Translates eLeapRS result codes into a human-readable string.
   */
  static const char *ResultString(eLeapRS r);

  /**
   * Services the LeapC message pump by calling LeapPollConnection().
   * The average polling time is determined by the framerate of the Leap Motion service.
   */
  static void serviceMessageLoop();

  /**
   * Caches the newest frame by copying the tracking event struct returned by
   * LeapC.
   */
  static void setFrame(const LEAP_TRACKING_EVENT *frame);

  /**
   * Returns a pointer to the cached tracking frame.
   */
  static const LEAP_TRACKING_EVENT *LeapListener::getFrame();

  /**
   * Caches the last device found by copying the device info struct returned by
   * LeapC.
   */
  static void setDevice(const LEAP_DEVICE_INFO *deviceProps);

  /**
   * Returns a pointer to the cached device info.
   */
  static LEAP_DEVICE_INFO *LeapListener::getDeviceProperties();

public:

  // コンストラクタ
  LeapListener();

  // デストラクタ
  virtual ~LeapListener();

  // Leap Motion と接続する
  const LEAP_CONNECTION *openConnection();

  // Leap Motion から切断する
  void closeConnection();

  // Leap Motion との接続を破棄する
  void destroyConnection();

  // 関節の変換行列のテーブルに値を取得する
  void getHandPose(GgMatrix *matrix) const;

  // 頭の姿勢の変換行列のテーブルに値を取得する
  void getHeadPose(GgMatrix *matrix) const;
};

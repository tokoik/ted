#pragma once

///
/// カメラが提供する入力形式の問い合わせインターフェース
///

#include <string>
#include <vector>

/// UIや設定処理から利用する、バックエンド非依存のカメラ入力形式
struct CaptureCapability
{
  std::string codec;
  int width{ 0 };
  int height{ 0 };
  double fps{ 0.0 };
};

namespace CameraCapabilities
{
  /// 利用可能なカメラデバイス名を返す
  const std::vector<std::string>& getDeviceList();

  /// 指定したデバイスが提供する入力形式を取得する
  bool getCapabilities(int device, std::vector<CaptureCapability>& capabilities);
}

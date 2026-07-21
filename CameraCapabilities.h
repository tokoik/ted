#pragma once

///
/// カメラが提供する入力形式の問い合わせインターフェース
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///

// 標準ライブラリ
#include <string>
#include <vector>

///
/// UIや設定処理から利用する、バックエンド非依存のカメラ入力形式
///
struct CaptureCapability
{
  std::string codec;  ///< コーデック名
  int width{ 0 };     ///< 幅
  int height{ 0 };    ///< 高さ
  double fps{ 0.0 };  ///< フレームレート
};

///
/// カメラの入力形式を問い合わせるためのインターフェース
///
namespace CameraCapabilities
{
  ///
  /// 利用可能なカメラデバイス名を返す
  ///
  /// @return デバイス名のリスト
  ///
  const std::vector<std::string>& getDeviceList();

  ///
  /// 指定したデバイスが提供する入力形式を取得する
  ///
  /// @param device デバイス番号
  /// @param capabilities 入力形式のリストを格納する変数
  /// @return 成功した場合は true、失敗した場合は false
  ///
  bool getCapabilities(int device, std::vector<CaptureCapability>& capabilities);
}

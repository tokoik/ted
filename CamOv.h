#pragma once

///
/// Ovrvision を使ってキャプチャするクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///

// カメラ関連の処理
#include "Camera.h"

// Windows API (ovrvision_pro.h / atlstr.h needs GetThreadLocale etc.)
#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#endif

// Ovrvision Pro
#include "ovrvision_pro.h"

#include <vector>
#include <atomic>

///
/// Ovrvision を使ってキャプチャするクラス
///
class CamOv
  : public Camera
{
  /// Ovrvision Pro
  static OVR::OvrvisionPro* ovrvision_pro;

  /// Ovrvision Pro の番号
  int device{ 0 };

  /// 接続されている Ovrvision Pro の台数
  static int count;

  /// 右眼用のフレームバッファ
  std::vector<std::uint8_t> imageR;

  /// 右眼用の解像度
  int widthR{ 0 };
  int heightR{ 0 };

  /// 右眼用のキャプチャ完了フラグ
  std::atomic<bool> capturedR{ false };

  /// 露出
  int exposure{ 0 };

  /// 利得
  int gain{ 0 };

  /// Ovrvision Pro から入力する
  void capture();

protected:

  ///
  /// キャプチャ開始処理を行う
  ///
  virtual bool onStart() override;

  ///
  /// キャプチャ停止処理を行う
  ///
  virtual void onStop() override;

  ///
  /// キャプチャデバイスを閉じる処理を行う
  ///
  virtual void onClose() override;

public:

  ///
  /// コンストラクタ
  ///
  CamOv();

  ///
  /// デストラクタ
  ///
  virtual ~CamOv();

  ///
  /// Ovrvision Pro を起動する
  ///
  /// @param ovrvision_property Ovrvision Pro のプロパティ
  /// @return 成功した場合は true
  ///
  bool open(OVR::Camprop ovrvision_property);

  ///
  /// 幅を得る
  ///
  virtual int getWidth(int eye = 0) const override
  {
    return (eye == 0) ? width : widthR;
  }

  ///
  /// 高さを得る
  ///
  virtual int getHeight(int eye = 0) const override
  {
    return (eye == 0) ? height : heightR;
  }

  ///
  /// チャンネル数を得る
  ///
  virtual int getChannels(int eye = 0) const override
  {
    return 4;
  }

  ///
  /// Ovrvision Pro の露出を上げる
  ///
  virtual void increaseExposure() override;

  ///
  /// Ovrvision Pro の露出を下げる
  ///
  virtual void decreaseExposure() override;

  ///
  /// Ovrvision Pro の利得を上げる
  ///
  virtual void increaseGain() override;

  ///
  /// Ovrvision Pro の利得を下げる
  ///
  virtual void decreaseGain() override;

  ///
  /// 送信用に指定した視点の最新フレームを複製する
  ///
  bool copyFrame(int eye, std::vector<std::uint8_t>& data, int& w, int& h, int& ch) const override
  {
    std::lock_guard<std::mutex> lock{ mtx };
    return eye == 0
      ? copyBuffer(image, width, height, 4, data, w, h, ch)
      : copyBuffer(imageR, widthR, heightR, 4, data, w, h, ch);
  }

  ///
  /// カメラフレームを OpenGL テクスチャへ転送する
  ///
  virtual bool transmit(int eye, unsigned int texture, const int* size) override;

  ///
  /// 指定した視点のフレームデータをロックして処理関数を実行する
  ///
  template <typename F>
  bool lockFrame(int eye, F&& func)
  {
    std::unique_lock<std::mutex> lock{ mtx, std::try_to_lock };
    if (!lock.owns_lock()) return false;

    // 転送したら転送待ちの印を下ろす
    if (eye == 0 && captured && !image.empty())
    {
      const auto length{ static_cast<size_t>(width) * height * 4 };
      func(image.data(), std::min(image.size(), length), width, height, 4);
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

#pragma once

///
/// 静止画像を使うクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

#include "Camera.h"
#include "opencv_link.h"

///
/// 静止画像を使うクラス
///
class CamImage : public Camera
{
  /// 右眼用の画像バッファ
  std::vector<std::uint8_t> imageR;

  /// 右眼用の幅と高さ
  int widthR{ 0 };
  int heightR{ 0 };

protected:

  ///
  /// キャプチャ開始処理（静止画ではスレッドを起動しない）
  ///
  /// @return 常に true
  ///
  bool onStart() override
  {
    return true;
  }

  ///
  /// キャプチャ停止処理
  ///
  void onStop() override
  {
  }

  ///
  /// キャプチャデバイスを閉じる処理
  ///
  void onClose() override
  {
    imageR.clear();
    widthR = 0;
    heightR = 0;
  }

public:

  ///
  /// コンストラクタ
  ///
  CamImage() = default;

  ///
  /// デストラクタ
  ///
  virtual ~CamImage()
  {
    close();
  }

  ///
  /// 静止画像入力であるかどうか調べる
  ///
  /// @return 静止画像なので true
  ///
  bool isStillImage() const override
  {
    return true;
  }

  ///
  /// ファイルから入力する
  ///
  /// @param file 画像ファイル名
  /// @param cam カメラ番号 (0: 左/単眼, 1: 右)
  /// @return 成功した場合は true
  ///
  bool open(const std::string& file, int cam = 0);

  ///
  /// カメラが使用可能か判定する
  ///
  /// @param cam カメラ番号
  /// @return 使用可能な場合は true
  ///
  bool opened(int cam = 0) const;

  ///
  /// 読み込んだ画像のデータを得る
  ///
  /// @param cam カメラ番号
  /// @return 画像のデータへの読み取り専用ポインタ
  ///
  const std::uint8_t* getImage(int cam = 0) const;

  ///
  /// 画像の幅を得る
  ///
  /// @param eye 視点番号
  /// @return 画像の幅
  ///
  int getWidth(int eye = 0) const override
  {
    return (eye == 0) ? width : widthR;
  }

  ///
  /// 画像の高さを得る
  ///
  /// @param eye 視点番号
  /// @return 画像の高さ
  ///
  int getHeight(int eye = 0) const override
  {
    return (eye == 0) ? height : heightR;
  }

  ///
  /// 画像のチャンネル数を得る
  ///
  /// @param eye 視点番号
  /// @return 画像のチャンネル数
  ///
  int getChannels(int eye = 0) const override
  {
    return channels;
  }

  ///
  /// 送信用に指定した視点の最新フレームを複製する
  ///
  bool copyFrame(int eye, std::vector<std::uint8_t>& data, int& w, int& h, int& ch) const override
  {
    std::lock_guard<std::mutex> lock{ mtx };
    return eye == 0
      ? copyBuffer(image, width, height, channels, data, w, h, ch)
      : copyBuffer(imageR, widthR, heightR, channels, data, w, h, ch);
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
    if (!lock.owns_lock() || !captured) return false;

    if (eye == 0 && !image.empty())
    {
      const auto length{ static_cast<size_t>(width) * height * channels };
      func(image.data(), std::min(image.size(), length), width, height, channels);
      return true;
    }
    else if (eye == 1 && !imageR.empty())
    {
      const auto length{ static_cast<size_t>(widthR) * heightR * channels };
      func(imageR.data(), std::min(imageR.size(), length), widthR, heightR, channels);
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

#pragma once

///
/// キャプチャデバイス関連の基底クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///

#include <vector>
#include <string>
#include <array>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <utility>

///
/// キャプチャデバイスが対応するビデオフォーマットの表示・選択情報
///
/// @details
/// バックエンド固有のメディア型を UI が解釈し直さなくて済むよう、
/// 解像度、フレームレート、コーデックを表示用文字列として保持する。
/// index はバックエンドが保持する実フォーマットの選択に使用する。
///
struct CaptureFormat
{
  std::string resolution; ///< 解像度の表示文字列（例: "1920 x 1080"）
  std::string fps;        ///< フレームレートの表示文字列（例: "30.00"）
  std::string codec;      ///< コーデックの表示文字列（例: "NV12"）
  int index{ 0 };         ///< バックエンドのフォーマットリストにおける選択番号

  ///
  /// コンストラクタ
  ///
  /// @param resolution 解像度の表示文字列（例: "1920 x 1080"）
  /// @param fps フレームレートの表示文字列（例: "30.00"）
  /// @param codec コーデックの表示文字列（例: "NV12"）
  /// @param index バックエンドのフォーマットリストにおける選択番号
  ///
  CaptureFormat(const std::string& resolution, const std::string& fps,
    const std::string& codec, int index)
    : resolution{ resolution }
    , fps{ fps }
    , codec{ codec }
    , index{ index }
  {
  }
};

///
/// キャプチャデバイス関連の基底クラス
///
class Camera
{
private:

  // コピー・代入は禁止
  Camera(const Camera&) = delete;
  Camera& operator=(const Camera&) = delete;

protected:

  /// 解像度（幅）
  int width{ 0 };

  /// 解像度（高さ）
  int height{ 0 };

  /// チャンネル数
  int channels{ 0 };

  /// キャプチャした画像のフレーム間隔 (ミリ秒)
  double interval{ 10.0 };

  /// キャプチャしたフレームを保持する単一バッファ (CPU メモリ上の生データ)
  std::vector<std::uint8_t> image;

  /// 排他制御用ミューテックス
  mutable std::mutex mtx;

  /// キャプチャスレッドが実行中なら true
  std::atomic<bool> running{ false };

  /// 新しいフレームが取得されたら true
  std::atomic<bool> captured{ false };

  /// レイテンシを優先するなら true
  std::atomic<bool> prioritizeLatency{ false };

  /// キャプチャを非同期に行うためのワーカースレッド
  std::thread thr;

  /// 視点ごとに新しいフレームを格納した回数
  ///
  /// @details
  /// テクスチャへの転送で下ろされる captured とは独立に、
  /// 作業者として映像を送信する Worker が新しいフレームを見分けるために使う。
  ///
  std::atomic<std::uint64_t> frameSerial[2]{};

  ///
  /// フレームのバッファを複製する（呼び出し側で mtx をロックしておく）
  ///
  static bool copyBuffer(const std::vector<std::uint8_t>& src, int srcW, int srcH, int srcCh,
    std::vector<std::uint8_t>& data, int& w, int& h, int& ch)
  {
    const auto length{ static_cast<std::size_t>(srcW) * srcH * srcCh };
    if (srcW <= 0 || srcH <= 0 || srcCh <= 0 || src.size() < length) return false;
    data.assign(src.begin(), src.begin() + length);
    w = srcW;
    h = srcH;
    ch = srcCh;
    return true;
  }

  ///
  /// 新しいフレームを格納したことを記録する（キャプチャスレッドが格納直後に呼ぶ）
  ///
  /// @param eye 視点番号 (0: 左/単眼, 1: 右)
  ///
  void notifyFrame(int eye)
  {
    frameSerial[eye == 0 ? 0 : 1].fetch_add(1, std::memory_order_release);
  }

  ///
  /// キャプチャ開始処理を行う（派生クラス固有の実装）
  ///
  /// @return 正常に開始できたら true
  ///
  virtual bool onStart() = 0;

  ///
  /// キャプチャ停止処理を行う（派生クラス固有の実装）
  ///
  /// @return なし
  ///
  virtual void onStop() = 0;

  ///
  /// キャプチャデバイスを閉じる処理を行う（派生クラス固有の実装）
  ///
  /// @return なし
  ///
  virtual void onClose() = 0;

public:

  ///
  /// コンストラクタ
  ///
  Camera() = default;

  ///
  /// デストラクタ
  ///
  virtual ~Camera() = default;

  ///
  /// キャプチャを開始する
  ///
  void start()
  {
    std::lock_guard<std::mutex> lock{ mtx };
    if (running) return;
    if (onStart())
    {
      running = true;
    }
  }

  ///
  /// キャプチャを停止する
  ///
  void stop()
  {
    if (!running) return;
    running = false;
    onStop();
    if (thr.joinable())
    {
      thr.join();
    }
  }

  ///
  /// キャプチャデバイスを閉じる
  ///
  void close()
  {
    stop();
    std::lock_guard<std::mutex> lock{ mtx };
    onClose();
    captured = false;
    width = 0;
    height = 0;
    channels = 0;
    image.clear();
  }

  ///
  /// キャプチャスレッドが実行中かどうか調べる
  ///
  /// @return キャプチャ中なら true
  ///
  bool isRunning() const
  {
    return running;
  }

  ///
  /// 新しいフレームが取得されているか調べる
  ///
  /// @return 新しいフレームが取得されていれば true
  ///
  bool isCaptured() const
  {
    return captured;
  }

  ///
  /// 新しいフレームを格納した回数を得る
  ///
  /// @param eye 視点番号 (0: 左/単眼, 1: 右)
  /// @return 通し番号（値が変われば新しいフレームがある）
  ///
  std::uint64_t getFrameSerial(int eye) const
  {
    return frameSerial[eye == 0 ? 0 : 1].load(std::memory_order_acquire);
  }

  ///
  /// 送信用に指定した視点の最新フレームを複製する
  ///
  /// @param eye 視点番号 (0: 左/単眼, 1: 右)
  /// @param data 複製先（容量は再利用する）
  /// @param w 画像の幅
  /// @param h 画像の高さ
  /// @param ch チャンネル数 (3: BGR, 4: BGRA)
  /// @return 複製できたら true
  ///
  /// @details
  /// captured を変更しないので、テクスチャへの転送 (transmit) と干渉しない。
  /// ロック中は複製だけを行い、縮小や符号化は呼び出し側がロックの外で行う。
  ///
  virtual bool copyFrame(int eye, std::vector<std::uint8_t>& data, int& w, int& h, int& ch) const
  {
    if (eye != 0) return false;
    std::lock_guard<std::mutex> lock{ mtx };
    return copyBuffer(image, width, height, channels, data, w, h, ch);
  }

  ///
  /// フレームデータをロックして処理関数を実行する
  ///
  /// @tparam F 処理関数の型
  /// @param func フレームデータを処理する関数 (引数: const std::uint8_t* data, size_t length, int width, int height, int channels)
  /// @return フレームが取得できて処理関数が実行されたら true
  ///
  template <typename F>
  bool lockFrame(F&& func)
  {
    return lockFrame(0, std::forward<F>(func));
  }

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
    if (eye != 0) return false;
    std::unique_lock<std::mutex> lock{ mtx, std::try_to_lock };
    if (lock.owns_lock() && captured && !image.empty())
    {
      const auto length{ static_cast<size_t>(width) * height * channels };
      func(image.data(), std::min(image.size(), length), width, height, channels);
      if (!isStillImage()) captured = false;
      return true;
    }
    return false;
  }

  ///
  /// 静止画像入力であるかどうかを調べる
  ///
  /// @return 静止画像なら true
  ///
  virtual bool isStillImage() const
  {
    return false;
  }

  ///
  /// キャプチャデバイスが対応するビデオフォーマットのリストを得る
  ///
  /// @return ビデオフォーマット情報のリスト
  ///
  virtual const std::vector<CaptureFormat>& getFormatList() const
  {
    static const std::vector<CaptureFormat> empty;
    return empty;
  }

  ///
  /// ビデオフォーマットを選択して設定する
  ///
  /// @param index 選択するフォーマットのインデックス
  /// @return 正常に設定できたら true
  ///
  virtual bool selectFormat(int index)
  {
    return false;
  }

  ///
  /// キャプチャしたフレームのサイズを得る
  ///
  /// @param eye 視点番号 (0: 左/単眼, 1: 右)
  /// @return キャプチャしたフレームのサイズ
  ///
  std::array<int, 2> getSize(int eye = 0) const
  {
    return std::array<int, 2>{ getWidth(eye), getHeight(eye) };
  }

  ///
  /// キャプチャしたフレームの横の画素数を得る
  ///
  /// @param eye 視点番号 (0: 左/単眼, 1: 右)
  /// @return キャプチャ中のフレームの横の画素数
  ///
  virtual int getWidth(int eye = 0) const
  {
    return (eye == 0) ? width : 0;
  }

  ///
  /// キャプチャしたフレームの縦の画素数を得る
  ///
  /// @param eye 視点番号 (0: 左/単眼, 1: 右)
  /// @return キャプチャ中のフレームの縦の画素数
  ///
  virtual int getHeight(int eye = 0) const
  {
    return (eye == 0) ? height : 0;
  }

  ///
  /// キャプチャしたフレームのチャネル数を調べる
  ///
  /// @param eye 視点番号 (0: 左/単眼, 1: 右)
  /// @return キャプチャしたフレームのチャネル数
  ///
  virtual int getChannels(int eye = 0) const
  {
    return (eye == 0) ? channels : 0;
  }

  ///
  /// キャプチャデバイスのフレームレートを得る
  ///
  /// @return キャプチャデバイスのフレームレート
  ///
  virtual double getFps() const
  {
    return interval > 0.0 ? 1000.0 / interval : 0.0;
  }

  ///
  /// レイテンシ優先モードを設定する
  ///
  /// @param mode レイテンシを優先する場合は true
  ///
  void setPrioritizeLatency(bool mode)
  {
    prioritizeLatency = mode;
  }

  ///
  /// レイテンシ優先モードかどうか調べる
  ///
  /// @return レイテンシを優先する場合は true
  ///
  bool getPrioritizeLatency() const
  {
    return prioritizeLatency;
  }

  ///
  /// 露出を上げる
  ///
  virtual void increaseExposure()
  {
  }

  ///
  /// 露出を下げる
  ///
  virtual void decreaseExposure()
  {
  }

  ///
  /// 利得を上げる
  ///
  virtual void increaseGain()
  {
  }

  ///
  /// 利得を下げる
  ///
  virtual void decreaseGain()
  {
  }

  ///
  /// カメラフレームを OpenGL テクスチャへ転送する
  ///
  /// @param eye 視点番号 (0: 左/単眼, 1: 右)
  /// @param texture 転送先のテクスチャ名 (GLuint)
  /// @param size テクスチャのサイズ配列 (幅, 高さ)
  /// @return 転送に成功したら true
  ///
  virtual bool transmit(int eye, unsigned int texture, const int* size);
};

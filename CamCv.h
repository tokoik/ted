#pragma once

///
/// OpenCV を使ったビデオキャプチャクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date December 27, 2022
///

#include <chrono>
#include <thread>
#include <iostream>

// OpenCV へのリンクとインクルード
#include "opencv_link.h"

// カメラ関連の処理
#include "Camera.h"

///
/// OpenCV を使ってビデオをキャプチャするクラス
///
class CamCv : public Camera
{
  /// OpenCV のキャプチャデバイス
  cv::VideoCapture camera;

  /// OpenCV のキャプチャデバイスから取得したフレーム
  cv::Mat cvFrame;

  /// 現在のフレームの時刻
  double elapsedTime{ 0.0 };

  /// ムービーファイルのインポイント
  double in{ 0.0 };

  /// ムービーファイルのアウトポイント
  double out{ 0.0 };

  /// ムービーファイルの総フレーム数
  double total{ 0.0 };

  /// 露出と利得
  int exposure{ 0 }, gain{ 0 };

  ///
  /// 現在時刻を秒単位で得る（glfwGetTime 互換の単調増加タイマー）
  ///
  /// @return 現在時刻（秒）
  ///
  static double getTime()
  {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
  }

  ///
  /// キャプチャデバイスを初期化する
  ///
  /// @param initial_width キャプチャデバイスを開く際に期待するフレームの横の画素数
  /// @param initial_height キャプチャデバイスを開く際に期待するフレームの縦の画素数
  /// @param initial_fps キャプチャデバイスを開く際に期待するフレームレート
  /// @param fourcc キャプチャデバイスを開く際に期待するコーデックの 4 文字
  /// @return キャプチャデバイスが使用可能なら true
  ///
  bool init(int initial_width, int initial_height, double initial_fps, const char* fourcc = "")
  {
    // カメラのコーデック・解像度・フレームレートを設定する
    if (fourcc[0] != '\0') camera.set(cv::CAP_PROP_FOURCC,
      cv::VideoWriter::fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]));
    if (initial_width > 0) camera.set(cv::CAP_PROP_FRAME_WIDTH, initial_width);
    if (initial_height > 0) camera.set(cv::CAP_PROP_FRAME_HEIGHT, initial_height);
    if (initial_fps > 0.0) camera.set(cv::CAP_PROP_FPS, initial_fps);

    // fps が 0 より大きければ逆数をカメラの遅延に使う
    const auto fps{ camera.get(cv::CAP_PROP_FPS) };
    if (fps > 0.0) interval = 1000.0 / fps;

    // ムービーファイルのインポイント・アウトポイントの初期値とフレーム数
    in = camera.get(cv::CAP_PROP_POS_FRAMES);
    out = total = camera.get(cv::CAP_PROP_FRAME_COUNT);

    // 経過時間
    elapsedTime = 0.0;

    // カメラから最初のフレームをキャプチャできなかったらカメラは使えない
    if (!camera.grab()) return false;

    // カメラの利得と露出を取得する
    gain = static_cast<int>(camera.get(cv::CAP_PROP_GAIN));
    exposure = static_cast<int>(camera.get(cv::CAP_PROP_EXPOSURE) * 10.0);

    // フレームを取り出してキャプチャ用のメモリを確保する
    camera.retrieve(cvFrame);

#if defined(DEBUG)
    char codec[5]{ 0, 0, 0, 0, 0 };
    getCodec(codec);
    std::cerr << "in:" << in << ", out:" << out
      << ", width:" << cvFrame.cols << ", height:" << cvFrame.rows
      << ", fourcc: " << codec << "\n";
#endif

    // 基底クラスのバッファとメンバを更新
    width = cvFrame.cols;
    height = cvFrame.rows;
    channels = cvFrame.channels();
    {
      const auto size{ static_cast<size_t>(cvFrame.total() * cvFrame.elemSize()) };
      image.resize(size);
      std::memcpy(image.data(), cvFrame.data, size);
    }

    // フレームがキャプチャされたことを記録する
    captured = true;

    // カメラが使える
    return true;
  }

  ///
  /// フレームをキャプチャする（別スレッドでループ実行される）
  ///
  void capture()
  {
    // 再生開始時刻
    auto startTime{ getTime() };

    // スレッドが実行可の間
    while (running)
    {
      // フレームを取り出せたら true
      auto status{ (total <= 0.0 || camera.get(cv::CAP_PROP_POS_FRAMES) < out) && camera.grab() };

      // ムービーファイルでないかムービーファイルの終端でなければ次のフレームを取り出して
      if (status && camera.retrieve(cvFrame))
      {
        // 単一バッファをロックしてから
        std::lock_guard<std::mutex> lock{ mtx };

        // 基底クラスのバッファとメンバを直接更新
        width = cvFrame.cols;
        height = cvFrame.rows;
        channels = cvFrame.channels();
        const auto size{ static_cast<size_t>(cvFrame.total() * cvFrame.elemSize()) };
        image.resize(size);
        std::memcpy(image.data(), cvFrame.data, size);

        // 新しいフレームがキャプチャされたことを通知する
        captured = true;
      }

      // 遅延時間
      auto deferred{ 0.0 };

      // ムービーファイルから入力しているとき
      if (total > 0.0)
      {
        // ムービーファイルの終端に到達していたら
        if (!status)
        {
          // インポイントまで巻き戻す
          camera.set(cv::CAP_PROP_POS_FRAMES, in);

          // 経過時間を戻す
          elapsedTime = 0.0;

          // 開始時間を更新する
          startTime = getTime();
        }
        else
        {
          // 現在のフレームのインポイントからの経過時間
          const auto pos{ camera.get(cv::CAP_PROP_POS_MSEC) - in };

          // 再生位置の次のフレームの時刻に対する経過時間
          const auto now{ (elapsedTime + getTime() - startTime) * 1000.0 + interval };

          // 遅延時間はフレームの経過時間と現在の経過時間の差
          deferred = pos - now;
        }
      }

      // 遅延時間あれば待つ
      if (deferred > 0.0)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(deferred)));
      }
    }

    // 再生停止までの経過時間を積算する
    elapsedTime += getTime() - startTime;
  }

protected:

  ///
  /// キャプチャ開始処理を行う
  ///
  /// @return 正常に開始できたら true
  ///
  bool onStart() override
  {
    if (!camera.isOpened()) return false;
    thr = std::thread(&CamCv::capture, this);
    return true;
  }

  ///
  /// キャプチャ停止処理を行う
  ///
  void onStop() override
  {
    // capture() の while (running) が抜ける
  }

  ///
  /// キャプチャデバイスを閉じる処理を行う
  ///
  void onClose() override
  {
    camera.release();
    in = 0.0;
    out = 0.0;
    total = 0.0;
    elapsedTime = 0.0;
  }

public:

  ///
  /// コンストラクタ
  ///
  CamCv() = default;

  ///
  /// デストラクタ
  ///
  virtual ~CamCv()
  {
    close();
  }

  ///
  /// キャプチャデバイスを開く
  ///
  /// @param device キャプチャデバイスの番号
  /// @param width キャプチャデバイスを開く際に期待するフレームの横の画素数, 0 ならお任せ
  /// @param height キャプチャデバイスを開く際に期待するフレームの縦の画素数, 0 ならお任せ
  /// @param fps キャプチャデバイスを開く際に期待するフレームレート, 0 ならお任せ
  /// @param fourcc キャプチャデバイスを開く際に期待するコーデックの 4 文字, "" ならお任せ
  /// @param pref OpenCV が使用する VideoCapture バックエンド
  /// @return キャプチャデバイスが使用可能なら true
  ///
  bool open(int device, int width = 0, int height = 0, double fps = 0.0, const char* fourcc = "", int pref = cv::CAP_ANY)
  {
    // カメラを開いて初期化する
    return camera.open(device, pref) && init(width, height, fps, fourcc);
  }

  ///
  /// ファイルまたはネットワークから入力する
  ///
  /// @param file 入力するファイルまたはネットワーク URL
  /// @param width 入力するファイルを開く際に期待するフレームの横の画素数, 0 ならお任せ
  /// @param height 入力するファイルを開く際に期待するフレームの縦の画素数, 0 ならお任せ
  /// @param fps 入力するファイルを開く際に期待するフレームレート, 0 ならお任せ
  /// @param fourcc 入力するファイルを開く際に期待するコーデックの 4 文字, "" ならお任せ
  /// @param pref OpenCV が使用する VideoCapture バックエンド
  /// @return 入力するファイルが使用可能なら true
  ///
  bool open(const std::string& file, int width = 0, int height = 0, double fps = 0.0, const char* fourcc = "", int pref = cv::CAP_ANY)
  {
    // ファイル／ネットワークを開いて初期化する
    return camera.open(file, pref) && init(width, height, fps, fourcc);
  }

  ///
  /// キャプチャしたフレームのフレームレートを得る
  ///
  /// @return キャプチャしたフレームのフレームレート
  ///
  virtual double getFps() const override
  {
    return camera.get(cv::CAP_PROP_FPS);
  }

  ///
  /// コーデックを調べる
  ///
  /// @return 使用しているコーデックを表す 4 バイト
  ///
  unsigned int getCodec() const
  {
    return static_cast<unsigned int>(camera.get(cv::CAP_PROP_FOURCC));
  }

  ///
  /// コーデックを調べる
  ///
  /// @param fourcc 使用しているコーデックを表す 4 文字の格納先
  ///
  void getCodec(char* fourcc) const
  {
    auto cc{ getCodec() };
    for (int i = 0; i < 4; ++i)
    {
      fourcc[i] = static_cast<char>(cc & 0x7f);
      if (!isalnum(fourcc[i])) fourcc[i] = '?';
      cc >>= 8;
    }
  }

  ///
  /// ファイルから入力しているとき現在のフレーム番号を得る
  ///
  /// @return 現在入力しているフレーム番号
  ///
  double getPosition() const
  {
    return camera.get(cv::CAP_PROP_POS_FRAMES);
  }

  ///
  /// ファイルから入力しているとき再生位置を指定する
  ///
  /// @param frame 再生位置
  ///
  void setPosition(double frame)
  {
    camera.set(cv::CAP_PROP_POS_FRAMES, frame);
  }

  ///
  /// 露出を設定する
  ///
  /// @param exposure 設定する露出
  ///
  void setExposure(double exposure)
  {
    if (camera.isOpened()) camera.set(cv::CAP_PROP_EXPOSURE, exposure);
  }

  ///
  /// 露出を一段階上げる
  ///
  void increaseExposure() override
  {
    setExposure(++exposure * 0.1);
  }

  ///
  /// 露出を一段階下げる
  ///
  void decreaseExposure() override
  {
    setExposure(--exposure * 0.1);
  }

  ///
  /// 利得を設定する
  ///
  /// @param gain 設定する利得
  ///
  void setGain(double gain)
  {
    if (camera.isOpened()) camera.set(cv::CAP_PROP_GAIN, gain);
  }

  ///
  /// 利得を一段階上げる
  ///
  void increaseGain() override
  {
    setGain(++gain);
  }

  ///
  /// 利得を一段階下げる
  ///
  void decreaseGain() override
  {
    setGain(--gain);
  }
};

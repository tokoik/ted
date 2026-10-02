///
/// リモートのカメラからキャプチャするクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///
#include "CamRemote.h"

// シーングラフ
#include "Scene.h"

// 共有メモリ
#include "SharedMemory.h"

// エラー通知 (NOTIFY)
#include "GgApp.h"

#include <cmath>
#include <chrono>
#include <iostream>
#include <cstring>

//
// 受信バッファ上の JPEG を複製せずにデコードする
//
static cv::Mat decode(const unsigned char* data, unsigned int length)
{
  const cv::Mat encoded(1, static_cast<int>(length), CV_8UC1, const_cast<unsigned char*>(data));
  return cv::imdecode(encoded, cv::IMREAD_COLOR);
}

//
// コンストラクタ
//
CamRemote::CamRemote()
{
  // 平面展開後の背景画像のサイズ
  size[camL] = size[camR] = cv::Size(
    std::max(defaults.remote_texture_width, 1),
    std::max(defaults.remote_texture_height, 1));

  // 背景画像の変形に使うフレームバッファオブジェクト
  glGenFramebuffers(1, &fb);

  // リモートから取得したフレームのサンプリングに使うテクスチャ
  glGenTextures(camCount, resample);

  // 魚眼画像を平面展開するシェーダ
  shader = ggLoadShader("mesh.vert", "mesh.frag");
  gapLoc = glGetUniformLocation(shader, "gap");
  screenLoc = glGetUniformLocation(shader, "screen");
  rotationLoc = glGetUniformLocation(shader, "rotation");
  imageLoc = glGetUniformLocation(shader, "image");
}

//
// デストラクタ
//
CamRemote::~CamRemote()
{
  close();

  // 背景画像のタイリングに使うフレームバッファオブジェクト
  glDeleteFramebuffers(1, &fb);

  // リモートから取得したフレームのサンプリングに使うテクスチャ
  glDeleteTextures(camCount, resample);

  // 魚眼画像に変形するシェーダ
  glDeleteProgram(shader);
}

//
// キャプチャ開始
//
bool CamRemote::onStart()
{
  sendThread = std::thread([this]() { send(); });
  recvThread = std::thread([this]() { recv(); });
  return true;
}

//
// キャプチャ停止
//
void CamRemote::onStop()
{
  network.sendEof();
  if (sendThread.joinable()) sendThread.join();
  if (recvThread.joinable()) recvThread.join();
}

//
// キャプチャデバイスを閉じる
//
void CamRemote::onClose()
{
  delete[] sendbuf;
  delete[] recvbuf;
  sendbuf = recvbuf = nullptr;
  network.finalize();
  imageR.clear();
  widthR = 0;
  heightR = 0;
  capturedR = false;
  stereoSource = false;
}

//
// 指導者側の起動
//
int CamRemote::open(unsigned short port, const char* address)
{
  // すでに確保されている作業用メモリを破棄する
  delete[] sendbuf;
  delete[] recvbuf;
  sendbuf = recvbuf = nullptr;

  // 指導者として初期化する (port で受信し port + 1 へ送信する)
  const int ret(network.initialize(INSTRUCTOR, port, address));
  if (ret != 0) return ret;

  // 作業用のメモリを確保する
  sendbuf = new unsigned char[maxFrameSize];
  recvbuf = new unsigned char[maxFrameSize];
  stereoSource = false;

  // 受信状態を初期化する
  firstImage = false;
  stereoSource = false;
  keyframeNeeded = false;

  // 通信スレッドを開始する。
  // 動画の場合はキーフレームが届くまで復号できないので、送信スレッドがキーフレームを要求しながら、
  // 受信スレッドが最初の左画像を復号するのを待つ。
  start();
  const auto deadline{ std::chrono::steady_clock::now()
    + std::chrono::milliseconds(receiveRetry * 500) };
  while (!firstImage && std::chrono::steady_clock::now() < deadline)
    std::this_thread::sleep_for(std::chrono::milliseconds(minDelay));

  // テクスチャ確保には画像寸法が必要なため、最初に受け取った画像の大きさを使う
  // (届いていない場合は設定または初期値を用い、後続フレームの到着時に動的リサイズする)
  cv::Size rsize[camCount];
  {
    std::lock_guard<std::mutex> lock{ mtx };
    if (firstImage && !remote[camL].empty())
    {
      rsize[camL] = remote[camL].size();
      rsize[camR] = remote[camR].empty() ? rsize[camL] : remote[camR].size();
    }
    else
    {
      const int w{ defaults.camera_width > 0 ? defaults.camera_width : (size[camL].width > 0 ? size[camL].width : 1280) };
      const int h{ defaults.camera_height > 0 ? defaults.camera_height : (size[camL].height > 0 ? size[camL].height : 720) };
      rsize[camL] = cv::Size(w, h);
      rsize[camR] = cv::Size(w, h);
    }
  }

  // 背景画像の変形に使うメッシュの縦横の格子点数を求める
  const GLfloat aspect(static_cast<GLfloat>(size[camL].width) / static_cast<GLfloat>(size[camL].height));
  const int samples{ std::max(defaults.remote_texture_samples, 4) };
  slices = std::max(static_cast<GLsizei>(sqrt(aspect * static_cast<GLfloat>(samples))), 2);
  stacks = std::max(samples / slices, 2);

  // 背景画像の変形に使うメッシュの縦横の格子間隔を求める
  gap[0] = 2.0f / static_cast<GLfloat>(slices - 1);
  gap[1] = 2.0f / static_cast<GLfloat>(stacks - 1);

  // 背景画像を取得するリモートカメラの画角
  screen[0] = tan(defaults.remote_fov_x);
  screen[1] = tan(defaults.remote_fov_y);

  for (int cam = 0; cam < camCount; ++cam)
  {
    resampleSize[cam] = rsize[cam];

    // リモートから取得したフレームのサンプリングに使うテクスチャを準備する
    glBindTexture(GL_TEXTURE_2D, resample[cam]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, rsize[cam].width, rsize[cam].height, 0,
      GL_BGR, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
  }

  return 0;
}

//
// カメラをロックして画像をテクスチャに転送する
//
bool CamRemote::transmit(int eye, unsigned int texture, const int* transmitSize)
{
  std::unique_lock<std::mutex> lock{ mtx, std::try_to_lock };
  if (lock.owns_lock())
  {
    const bool isCap{ (eye == 0) ? captured.load() : capturedR.load() };
    if (isCap)
    {
      const auto& img{ remote[eye] };
      if (!img.empty())
      {
        const GLsizei fsize[]{ img.cols, img.rows };

        glBindTexture(GL_TEXTURE_2D, resample[eye]);
        if (resampleSize[eye].width != fsize[0] || resampleSize[eye].height != fsize[1])
        {
          resampleSize[eye] = cv::Size(fsize[0], fsize[1]);
          glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, fsize[0], fsize[1], 0,
            GL_BGR, GL_UNSIGNED_BYTE, nullptr);
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fsize[0], fsize[1], GL_BGR, GL_UNSIGNED_BYTE, img.data);

        if (eye == 0) captured = false;
        else capturedR = false;
        lock.unlock();

        // テクスチャ変形用のシェーダ
        glUseProgram(shader);
        glUniform2fv(gapLoc, 1, gap);
        glUniform2fv(screenLoc, 1, screen);
        glUniform1i(imageLoc, 0);
        glViewport(0, 0, transmitSize[0], transmitSize[1]);

        // 背景画像の変形に使うフレームバッファオブジェクトに切り替える
        glBindFramebuffer(GL_FRAMEBUFFER, fb);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
          glBindFramebuffer(GL_FRAMEBUFFER, 0);
          return false;
        }

        // リモートから取得したフレームのサンプリングに使うテクスチャを指定する
        glBindTexture(GL_TEXTURE_2D, resample[eye]);

        // リモートのヘッドトラッキング情報を設定してレンダリング
        glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, Scene::getRemoteAttitude(eye).data());
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, slices * 2, stacks - 1);

        // レンダリング先を通常のフレームバッファに戻す
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
      }
    }
  }
  return false;
}

//
// 動画の 1 アクセスユニットを復号する
//
void CamRemote::decodeVideo(int eye, unsigned int format, const unsigned char* data, unsigned int size,
  cv::Mat& image)
{
  image.release();
  if (size <= sizeof(VideoUnitHeader)) return;

  VideoUnitHeader unit;
  std::memcpy(&unit, data, sizeof unit);
  const bool keyframe{ (unit.flags & VIDEO_UNIT_KEYFRAME) != 0 };
  auto& state{ video[eye] };

  // 形式が変わったらデコーダを開き直す
  if (!state.decoder.isOpen() || state.format != format)
  {
    state.format = format;
    state.waitKeyframe = true;
    state.hasExpected = false;
    if (!state.decoder.open(format))
    {
      static std::atomic<bool> notified{ false };
      if (!notified.exchange(true)) NOTIFY(u8"受信した動画を復号するデコーダが見つかりません。");
      return;
    }
  }

  // 通し番号が飛んでいたら (途中のアクセスユニットが欠けたら) 両眼ともキーフレームまで復号しない
  if (state.hasExpected && unit.number != state.expected)
  {
    for (int i = 0; i < camCount; ++i) decoderState[i].waitKeyframe = true;
    keyframeNeeded = true;
  }
  state.expected = unit.number + 1;
  state.hasExpected = true;

  if (state.waitKeyframe)
  {
    if (!keyframe)
    {
      keyframeNeeded = true;
      return;
    }
    state.waitKeyframe = false;
  }

  // 時刻はデコーダの内部で順序付けに使われるだけなので、フレームごとに進めればよい
  state.time += 333333;
  if (state.decoder.decode(data + sizeof unit, size - sizeof unit, state.time, image) < 0)
  {
    for (int i = 0; i < camCount; ++i) decoderState[i].waitKeyframe = true;
    keyframeNeeded = true;
    image.release();
  }
}

//
// リモートの映像と姿勢を受信する
//
void CamRemote::recv()
{
  // 動画のデコーダ (Media Foundation) はこのスレッドで使う
  const bool comInitialized{ SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)) };
  const bool mfStarted{ SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_LITE)) };

  // スレッドが実行可の間
  while (running)
  {
    // 姿勢データと画像データを受信する
    const int ret{ network.recvData(recvbuf, maxFrameSize) };

#if defined(DEBUG)
    std::cerr << "CamRemote recv:" << ret << '\n';
#endif

    // 長さ 0 は相手の停止通知 (EOF) だが、相手の再起動に備えて受信を続ける
    if (ret <= 0 || !network.checkRemote()) continue;

    // フレーム内部の境界が正しければデータを読み込む
    const unsigned int* head{ nullptr };
    const GgMatrix* body{ nullptr };
    const unsigned char* data{ nullptr };
    if (!unpackFrame(recvbuf, ret, head, body, data)) continue;

    // 変換行列を共有メモリに格納する
    remoteAttitude->store(body, getMatrixCount(head));

    const unsigned int format{ getImageFormat(head) };
    if (format != IMAGE_JPEG && (isKeyframeRequested(head) || head[camL] == 0))
    {
      for (int i = 0; i < camCount; ++i) decoderState[i].waitKeyframe = true;
      keyframeNeeded = true;
    }
    for (int eye = 0; eye < camCount; ++eye)
    {
      const unsigned int bytes{ head[eye] };
      const unsigned char* const payload{ data };
      data += bytes;
      if (bytes == 0) continue;

      cv::Mat decoded;
      if (format == IMAGE_JPEG)
      {
        // JPEG は前の画像がまだ使われていなければ復号を省く
        if (eye == camL ? captured.load() : capturedR.load()) continue;
        decoded = decode(payload, bytes);
      }
      else
      {
        // 動画は参照関係を保つため、表示に使わない画像も必ず復号する
        decodeVideo(eye, format, payload, bytes, decoded);
      }
      if (decoded.empty()) continue;

      std::lock_guard<std::mutex> lock{ mtx };
      remote[eye] = decoded;
      if (eye == camL)
      {
        width = decoded.cols;
        height = decoded.rows;
        channels = 3;
        captured = true;
        firstImage = true;
      }
      else
      {
        widthR = decoded.cols;
        heightR = decoded.rows;
        capturedR = true;
        stereoSource = true;
      }
    }

    // 右の画像を受け取っていなければ (単眼の送信側なら) 左の画像を右にも使う
    if (!stereoSource && captured && !capturedR)
    {
      std::lock_guard<std::mutex> lock{ mtx };
      remote[camR] = remote[camL];
      widthR = width;
      heightR = height;
      capturedR = true;
    }

    // recvData() はデータが届くまで (最長 500ms) 待つので、ここでは待たない。
    // 待つと送信側の頻度に受信が追いつかず、ソケットに古いフレームが溜まって遅延が増える。
  }

  for (auto& state : video) state.decoder.close();
  if (mfStarted) MFShutdown();
  if (comInitialized) CoUninitialize();
}

//
// ローカルの姿勢を送信する
//
void CamRemote::send()
{
  // 姿勢を送る間隔
  constexpr auto interval{ std::chrono::milliseconds(minDelay) };

  // カメラスレッドが実行可の間
  while (running)
  {
    const auto start{ std::chrono::steady_clock::now() };

    // ヘッダのフォーマット
    const auto head{ reinterpret_cast<unsigned int*>(sendbuf) };

    // 左右のフレームのサイズは 0 にする
    head[camL] = head[camR] = 0;

    // 変換行列の数を保存する
    const unsigned int count{ std::min(localAttitude->getSize(), frameCountMask) };
    head[camCount] = count;

    // 動画の欠落を検出していたら、送信側にキーフレームを要求する
    if (keyframeNeeded.exchange(false)) head[camCount] |= frameKeyframeRequest;

    // 送信する変換行列の格納場所
    const auto body{ reinterpret_cast<GgMatrix*>(head + headLength) };

    // 変換行列を共有メモリから取り出す
    localAttitude->load(body, count);

    // 左フレームの保存先 (変換行列の最後)
    const auto data{ reinterpret_cast<unsigned char*>(body + count) };

    // フレームを送信する
    network.sendData(sendbuf, static_cast<unsigned int>(data - sendbuf));

    // 送信にかかった時間を含めて一定間隔で送る
    std::this_thread::sleep_until(start + interval);
  }

  // ループを抜けるときに EOF を送信する
  network.sendEof();
}

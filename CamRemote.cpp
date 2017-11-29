//
// リモートカメラからキャプチャ
//
#include "CamRemote.h"

// リトライ回数
const int retry(3);

// コンストラクタ
CamRemote::CamRemote(bool reshape)
  : reshape(reshape)
{
  if (reshape)
  {
    // 背景画像のサイズ
    size[camR][0] = size[camL][0] = static_cast<GLsizei>(defaults.capture_width);
    size[camR][1] = size[camL][1] = static_cast<GLsizei>(defaults.capture_height);

    // 背景画像の変形に使うフレームバッファオブジェクト
    glGenFramebuffers(1, &fb);

    // リモートから取得したフレームのサンプリングに使うテクスチャ
    glGenTextures(camCount, resample);

    // 魚眼画像に変形するシェーダ
    shader = ggLoadShader("mesh.vert", "mesh.frag");
    gapLoc = glGetUniformLocation(shader, "gap");
    screenLoc = glGetUniformLocation(shader, "screen");
    rotationLoc = glGetUniformLocation(shader, "rotation");
    imageLoc = glGetUniformLocation(shader, "image");
  }
}

// デストラクタ
CamRemote::~CamRemote()
{
  // スレッドを停止する
  stop();

  if (reshape)
  {
    // 背景画像のタイリングに使うフレームバッファオブジェクト
    glDeleteFramebuffers(1, &fb);

    // リモートから取得したフレームのサンプリングに使うテクスチャ
    glDeleteTextures(camCount, resample);

    // 魚眼画像に変形するシェーダ
    glDeleteProgram(shader);
  }
}

// 操縦者側の起動
int CamRemote::open(unsigned short port, const char *address, const GLuint *texture)
{
  // 作業用のメモリを確保する（これは Camera のデストラクタで delete する）
  delete sendbuf, recvbuf;
  sendbuf = new uchar[maxFrameSize];
  recvbuf = new uchar[maxFrameSize];

  // 操縦者として初期化して
  network.initialize(1, port, address);

  // ヘッダのフォーマット
  unsigned int *const head(reinterpret_cast<unsigned int *>(recvbuf));

  // 1 フレーム受け取って
  for (int i = 0;;)
  {
    const int ret(network.recvFrame(recvbuf, maxFrameSize));
    if (ret > 0 && head[camL] > 0) break;
    if (++i > retry) return ret;
  }

  // 変換行列の保存先
  GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + camCount + 1));

  // 変換行列を復帰する
  remoteMatrix->store(body, 0, head[camCount]);

  // 左フレームの保存先 (変換行列の最後)
  uchar *const data(reinterpret_cast<uchar *>(body + head[camCount]));

  // 左フレームデータを vector に変換して
  encoded[camL].assign(data, data + head[camL]);

  // 左フレームをデコードする
  remote[camL] = cv::imdecode(cv::Mat(encoded[camL]), 1);

  // リモートから取得したフレームのサイズ
  GLsizei rsize[camCount][2];

  // 左フレームのサイズを求める
  rsize[camL][0] = remote[camL].cols;
  rsize[camL][1] = remote[camL].rows;

  // 右フレームが存在すれば
  if (head[camR] > 0)
  {
    // 右フレームデータを vector に変換して
    encoded[camR].assign(data + head[camL], data + head[camL] + head[camR]);

    // 右フレームをデコードする
    remote[camR] = cv::imdecode(cv::Mat(encoded[camR]), 1);

    // 右フレームのサイズを求める
    rsize[camR][0] = remote[camR].cols;
    rsize[camR][1] = remote[camR].rows;
  }
  else
  {
    // 右フレームは左と同じにする
    remote[camR] = remote[camL];

    // 右フレームのサイズは左フレームと同じにする
    rsize[camR][0] = rsize[camL][0];
    rsize[camR][1] = rsize[camL][1];
  }

  if (reshape)
  {
    // 背景画像の変形に使うメッシュの縦横の格子点数を求め
    const GLfloat aspect(static_cast<GLfloat>(size[camL][0]) / static_cast<GLfloat>(size[camL][1]));
    slices = static_cast<GLsizei>(sqrt(aspect * static_cast<GLfloat>(defaults.remote_texture_samples)));
    stacks = defaults.remote_texture_samples / slices;

    // 背景画像の変形に使うメッシュの縦横の格子間隔を求める
    gap[0] = 2.0f / static_cast<GLfloat>(slices - 1);
    gap[1] = 2.0f / static_cast<GLfloat>(stacks - 1);

    // 背景画像を取得するリモートカメラの画角
    screen[0] = tan(defaults.remote_fov_x);
    screen[1] = tan(defaults.remote_fov_y);

    for (int cam = 0; cam < camCount; ++cam)
    {
      // リモートから取得したフレームのサンプリングに使うテクスチャを準備する
      glBindTexture(GL_TEXTURE_2D, resample[cam]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, rsize[cam][0], rsize[cam][1], 0,
        GL_RGB, GL_UNSIGNED_BYTE, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    }
  }
  else
  {
    // ドーム画像に変形しないときは背景テクスチャのサイズにする
    size[camL][0] = rsize[camL][0];
    size[camL][1] = rsize[camL][1];
    size[camR][0] = rsize[camR][0];
    size[camR][1] = rsize[camR][1];
  }

  // 通信スレッドを開始する
  run[camL] = true;
  sendThread = std::thread([this]() { send(); });
  recvThread = std::thread([this]() { recv(); });

  return 0;
}

// 左カメラをロックして画像をテクスチャに転送する
bool CamRemote::transmit(int cam, GLuint texture, const GLsizei *size)
{
  // 画像のサイズ
  const GLsizei fsize[] = { remote[cam].cols, remote[cam].rows };

  if (!reshape) return Camera::transmit(cam, texture, fsize);

  if (Camera::transmit(cam, resample[cam], fsize))
  {
    // テクスチャ変形用のシェーダ
    glUseProgram(shader);
    glUniform2fv(gapLoc, 1, gap);
    glUniform2fv(screenLoc, 1, screen);
    glUniform1i(imageLoc, 0);
    glViewport(0, 0, size[0], size[1]);

    // 背景画像の変形に使うフレームバッファオブジェクトに切り替える
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

    // リモートから取得したフレームのサンプリングに使うテクスチャを指定する
    glBindTexture(GL_TEXTURE_2D, resample[cam]);

    // リモートのヘッドトラッキング情報を設定してレンダリング
    glUniformMatrix4fv(rotationLoc, 1, GL_FALSE, getRemoteAttitude(cam).transpose().get());
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, slices * 2, stacks - 1);

    // レンダリング先を通常のフレームバッファに戻す
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // フレームバッファオブジェクトのカラーバッファに使ったテクスチャを指定する
    glBindTexture(GL_TEXTURE_2D, texture);

    return true;
  }

  return false;
}

// ローカルの姿勢を送信する
void CamRemote::send()
{
  // カメラスレッドが実行可の間
  while (run[camL])
  {
    // ヘッダのフォーマット
    unsigned int *const head(reinterpret_cast<unsigned int *>(sendbuf));

    // 左右のフレームのサイズは 0 にする
    head[camL] = head[camR] = 0;

    // 変換行列の数を保存する
    head[camCount] = localMatrix->getUsed();

    // 変換行列の保存先
    GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + camCount + 1));

    // 変換行列を保存する
    localMatrix->load(body);

    // 左フレームの保存先 (変換行列の最後)
    uchar *const data(reinterpret_cast<uchar *>(body + head[camCount]));

    // フレームを送信する
    network.sendFrame(sendbuf, static_cast<unsigned int>(data - sendbuf));

    // 他のスレッドがリソースにアクセスするために少し待つ
    std::this_thread::sleep_for(std::chrono::milliseconds(10L));
  }
}

// リモートのフレームと姿勢を受信する
void CamRemote::recv()
{
  // スレッドが実行可の間
  while (run[camL])
  {
    // ヘッダのフォーマット
    unsigned int *const head(reinterpret_cast<unsigned int *>(recvbuf));

    // 1 フレーム受け取って
    for (int i = 0;;)
    {
      const int ret(network.recvFrame(recvbuf, maxFrameSize));
      if (ret > 0 && head[camL] > 0) break;
      if (++i > retry) return;
    }

    // 変換行列の保存先
    GgMatrix *const body(reinterpret_cast<GgMatrix *>(head + camCount + 1));

    // 変換行列を復帰する
    remoteMatrix->store(body, 0, head[camCount]);

    // カメラの姿勢を復帰する
    queueRemoteAttitude(camL, body[camL]);
    queueRemoteAttitude(camR, body[camR]);

    // 左フレームの保存先 (変換行列の最後)
    uchar *const data(reinterpret_cast<uchar *>(body + head[camCount]));

    // リモートから取得したフレームのサイズ
    GLsizei rsize[camCount][2];

    // 左フレームが送られてきていて左バッファが空のとき
    if (head[camL] > 0 && !buffer[camL])
    {
      // 左フレームデータを vector に変換して
      encoded[camL].assign(data, data + head[camL]);

      // 左フレームをロックして
      captureMutex[camL].lock();

      // 左フレームをデコードして
      remote[camL] = cv::imdecode(cv::Mat(encoded[camL]), 1);

      // 左画像を更新し
      buffer[camL] = remote[camL].data;

      // 左フレームのサイズを求める
      rsize[camL][0] = remote[camL].cols;
      rsize[camL][1] = remote[camL].rows;

      // 左フレームの転送が完了すればロックを解除する
      captureMutex[camL].unlock();
    }

    // 右フレームが送られてきていて右バッファが空のとき
    if (head[camR] > 0 && !buffer[camR])
    {
      // 左フレームが存在すれば
      if (head[camL] > 0)
      {
        // 右フレームデータを vector に変換して
        encoded[camR].assign(data + head[camL], data + head[camL] + head[camR]);

        // 右フレームをロックして
        captureMutex[camR].lock();

        // 右フレームをデコードして
        remote[camR] = cv::imdecode(cv::Mat(encoded[camR]), 1);

        // 右画像を更新し
        buffer[camR] = remote[camR].data;

        // 右フレームのサイズを求める
        rsize[camR][0] = remote[camR].cols;
        rsize[camR][1] = remote[camR].rows;

        // フレームの転送が完了すればロックを解除する
        captureMutex[camR].unlock();
      }
      else
      {
        // 右フレームをロックして
        captureMutex[camR].lock();

        // 右画像は左画像と同じにする
        buffer[camR] = remote[camL].data;

        // 右フレームのサイズは左フレームと同じにする
        rsize[camR][0] = rsize[camL][0];
        rsize[camR][1] = rsize[camL][1];

        // フレームの転送が完了すればロックを解除する
        captureMutex[camR].unlock();
      }
    }
  }
}

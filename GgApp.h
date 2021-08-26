#pragma once

// ウィンドウ関連の処理
#include "Window.h"

//
// ゲームグラフィックス特論宿題アプリケーションクラス
//
class GgApp
{
  // 背景画像を取得するカメラ
  std::shared_ptr<Camera> camera;

  // 背景画像のデータ
  const GLubyte *image[camCount];

  // 背景画像のサイズ
  GLsizei size[camCount][2];

  // 背景画像のアスペクト比
  GLfloat aspect[camCount];

  // 背景画像を保存するテクスチャ
  GLuint texture[camCount];

  // ステレオカメラなら true
  bool stereo;

  //
  // 静止画像ファイルを使う
  //
  bool useImage();

  //
  // 動画像ファイルを使う
  //
  bool useMovie();

  //
  // Web カメラを使う
  //
  bool useCamera();

  //
  // Ovrvision Pro を使う
  //
  bool useOvervision();

  //
  // RealSense を使う
  //
  bool useRealsense();

  //
  // リモートの TED から取得する
  //
  bool useRemote();

public:

  //
  // コンストラクタ
  //
  GgApp();

  //
  // デストラクタ
  virtual ~GgApp();

  //
  // 入力ソースを選択する
  //
  bool selectInput();

  //
  // 表示デバイスを選択する
  //
  bool selectDisplay();

  //
  // アプリケーション本体
  //
  int main(int argc, const char* const* argv);
};

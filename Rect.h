#pragma once

//
// 矩形
//

// ウィンドウ関連の処理
#include "Window.h"

// 標準ライブラリ
#include <string>

class Rect
{
  // 描画するウィンドウ
  const Window& window;

  // 描画に使うシェーダ
  const GLuint shader;

  // 格子間隔の uniform 変数の場所
  const GLint gapLoc;

  // スクリーンのサイズと中心位置の uniform 変数の場所
  const GLint screenLoc;

  // 焦点距離の uniform 変数の場所
  const GLint focalLoc;

  // 背景テクスチャの半径と中心位置の uniform 変数の場所
  const GLint circleLoc;

  // 視線の回転行列の uniform 変数の場所
  const GLint rotationLoc;

  // テクスチャのサンプラの uniform 変数の場所
  const GLint imageLoc;

  // 頂点配列オブジェクト
  const GLuint vao;

  // 貼り付けるテクスチャ
  GLuint texture[camCount];

public:

  // コンストラクタ
  Rect(const Window& window, const std::string& vert, const std::string& frag);

  // デストラクタ
  ~Rect();

  // テクスチャを設定する
  void setTexture(int eye, GLuint eyeTexture)
  {
    texture[eye] = eyeTexture;
  }

  // シェーダプログラム名を得る
  GLuint get() const;

  // 描画
  void draw(int eye, const GgMatrix& rotation, const GLsizei* samples) const;
};

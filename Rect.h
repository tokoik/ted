#pragma once

//
// 矩形
//

// ウィンドウ関連の処理
#include "GgApp.h"

// 標準ライブラリ
#include <string>

// カメラ画像を表示方式に応じた背景面へ投影する描画オブジェクト。
// Windowが計算した画角・中心・眼姿勢をuniformへ渡し、左右別のテクスチャを描画する。
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

  // eyeの画像を、samplesで指定した格子密度のインスタンス描画として背景面へ投影する
  void draw(int eye, const GgMatrix& rotation, const std::array<GLsizei, 2>& samples) const;
};

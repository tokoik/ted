#pragma once

//
// 矩形
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// 標準ライブラリ
#include <string>

class Rect
{
  // 描画に使うシェーダ
  const GLuint shader;

  // 格子間隔の uniform 変数の場所
  const GLuint gapLoc;

  // スクリーンのサイズと中心位置の uniform 変数の場所
  const GLuint screenLoc;

  // 焦点距離の uniform 変数の場所
  const GLuint focalLoc;

  // 背景テクスチャの半径と中心位置の uniform 変数の場所
  const GLuint circleLoc;

  // 視線の回転行列の uniform 変数の場所
  const GLuint rotationLoc;

  // テクスチャのサンプラの uniform 変数の場所
  const GLuint imageLoc;

  // 頂点配列オブジェクト
  const GLuint vao;

  // 格子間隔
  const GLfloat *gap;

  // スクリーンのサイズと中心位置
  const GLfloat *screen;

  // 焦点距離
  GLfloat focal;

  // 背景テクスチャの半径と中心位置
  const GLfloat *circle;

public:

  // コンストラクタ
  Rect(const std::string &vert, const std::string &frag);

  // デストラクタ
  ~Rect();

  // シェーダプログラム名を得る
  GLuint get() const;

  // 格子間隔を設定する
  void setGap(const GLfloat *gap);

  // スクリーンのサイズと中心位置を設定する
  void setScreen(const GLfloat *screen);

  // 焦点距離を設定する
  void setFocal(GLfloat focal);

  // 背景テクスチャの半径と中心位置を設定する
  void setCircle(const GLfloat *circle);

  // 描画
  void draw(GLint texture, const GgMatrix &rotation, const GLsizei *samples) const;
};

//
// 矩形
//
#include "Rect.h"

// コンストラクタ
Rect::Rect(const std::string& vert, const std::string& frag)
  : shader(ggLoadShader(vert.c_str(), frag.c_str()))
  , gapLoc(glGetUniformLocation(shader, "gap"))
  , screenLoc(glGetUniformLocation(shader, "screen"))
  , focalLoc(glGetUniformLocation(shader, "focal"))
  , circleLoc(glGetUniformLocation(shader, "circle"))
  , rotationLoc(glGetUniformLocation(shader, "rotation"))
  , imageLoc(glGetUniformLocation(shader, "image"))
  , vao([]() { GLuint vao; glGenVertexArrays(1, &vao); return vao; } ())
{
}

// デストラクタ
Rect::~Rect()
{
  // 頂点配列オブジェクトを削除する
  glDeleteVertexArrays(1, &vao);
}

// シェーダプログラム名を得る
GLuint Rect::get() const
{
  return shader;
}

// 格子間隔を設定する
void Rect::setGap(const GLfloat* gap)
{
  this->gap = gap;
}

// スクリーンのサイズと中心位置を設定する
void Rect::setScreen(const GgVector& screen)
{
  this->screen = screen.data();
}

// 焦点距離を設定する
void Rect::setFocal(GLfloat focal)
{
  this->focal = focal;
}

// 背景テクスチャの半径と中心位置を設定する
void Rect::setCircle(const GgVector& circle, GLfloat offset)
{
  this->circle[0] = circle[0];
  this->circle[1] = circle[1];
  this->circle[2] = circle[2] + offset;
  this->circle[3] = circle[3];
}

// 描画
void Rect::draw(GLint texture, const GgMatrix& rotation, const GLsizei* samples) const
{
  // シェーダプログラムを選択する
  glUseProgram(shader);

  // 格子間隔を設定する
  glUniform2fv(gapLoc, 1, gap);

  // スクリーンのサイズと中心位置を設定する
  glUniform4fv(screenLoc, 1, screen);

  // 焦点距離を設定する
  glUniform1f(focalLoc, focal);

  // 背景テクスチャの半径と中心位置を設定する
  glUniform4fv(circleLoc, 1, circle.data());

  // 視線の回転行列を設定する
  glUniformMatrix4fv(rotationLoc, 1, GL_TRUE, rotation.get());

  // 投影するテクスチャを指定する
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glUniform1i(imageLoc, 0);

  // 頂点配列オブジェクトを指定して描画する
  glBindVertexArray(vao);
  glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, samples[0] * 2, samples[1] - 1);
}

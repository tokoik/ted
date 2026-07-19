#pragma once

///
/// 背景描画に使うメッシュクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///

// 補助プログラム
#include "gg.h"
using namespace gg;

///
/// メッシュクラス
///
class Mesh
{
  /// 頂点配列オブジェクト
  GLuint vao;

public:

  ///
  /// コンストラクタ
  ///
  Mesh()
  {
    // 頂点配列オブジェクトを作成する
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
  }

  ///
  /// コピーコンストラクタを封じる
  ///
  /// @param mesh コピー元のメッシュ
  ///
  Mesh(const Mesh &mesh) = delete;

  ///
  /// 代入演算子を封じる
  ///
  /// @param mesh コピー元のメッシュ
  ///
  Mesh &operator=(const Mesh &mesh) = delete;

  ///
  /// デストラクタ
  ///
  virtual ~Mesh()
  {
    // 頂点配列オブジェクトを削除する
    glDeleteVertexArrays(1, &vao);
  }

  ///
  /// 描画
  ///
  /// @param slices メッシュの横の数
  /// @param stacks メッシュの縦の数
  ///
  virtual void draw(GLint slices, GLint stacks) const
  {
    // 頂点配列オブジェクトを指定する
    glBindVertexArray(vao);

    // 描画する
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, slices * 2, stacks - 1);
  }
};

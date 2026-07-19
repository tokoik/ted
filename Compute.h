#pragma once

///
/// コンピュートシェーダによる画像処理クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 197, 2026
///

// 補助プログラム
#include "gg.h"
using namespace gg;

// 標準ライブラリ
#include <vector>

///
/// コンピュートシェーダを使った画像処理クラス
///
class Compute
{
  /// 計算用のシェーダプログラム
  const GLuint program;

public:

  ///
  /// コンストラクタ
  ///
  /// @param comp コンピュートシェーダのソースコード
  ///
  Compute(const char *comp)
    : program(ggLoadComputeShader(comp))
  {
  }

  ///
  /// デストラクタ
  ///
  virtual ~Compute()
  {
    // シェーダプログラムを削除する
    glDeleteShader(program);
  }

  ///
  /// 計算用のシェーダプログラムを得る
  ///
  /// @return 計算用のシェーダプログラム
  ///
  GLuint get() const
  {
    return program;
  }

  ///
  /// 計算用のシェーダプログラムの使用を開始する
  ///
  void use() const
  {
    glUseProgram(program);
  }

  ///
  /// 計算を実行する
  ///
  /// @param width 計算対象の幅
  /// @param height 計算対象の高さ
  /// @param local_size_x ローカルワークグループの幅
  /// @param local_size_y ローカルワークグループの高さ
  ///
  void execute(GLuint width, GLuint height, GLuint local_size_x = 1, GLuint local_size_y = 1) const
  {
    glDispatchCompute((width + local_size_x - 1) / local_size_x, (height + local_size_y - 1) / local_size_y, 1);
  }
};

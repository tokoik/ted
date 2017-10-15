#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

//
// target.frag
//
//   ターゲット描画用のシェーダ
//

// フレームバッファに出力するデータ
layout (location = 0) out vec4 fc;                  // フラグメントの色

void main(void)
{
  // ターゲットの色
  fc = vec4(0.3, 0.9, 0.3, 0.4);
}

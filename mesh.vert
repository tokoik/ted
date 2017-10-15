#version 150 core

//
//   魚眼レンズ画像に変形
//

// スクリーンの格子間隔
uniform vec2 gap;

// スクリーンの高さと幅
uniform vec2 screen;

// スクリーンを回転する変換行列
uniform mat4 rotation;

// テクスチャ座標
out vec2 texcoord;

void main(void)
{
  // 頂点位置
  //   各頂点において gl_VertexID が 0, 1, 2, 3, ... のように割り当てられるから、
  //     x = gl_VertexID >> 1      = 0, 0, 1, 1, 2, 2, 3, 3, ...
  //     y = 1 - (gl_VertexID & 1) = 1, 0, 1, 0, 1, 0, 1, 0, ...
  //   のように GL_TRIANGLE_STRIP 向けの頂点座標値が得られる。
  //   y に gl_InstaceID を足せば glDrawArrayInstanced() のインスタンスごとに y が変化する。
  //   これに格子の間隔 gap をかけて 1 を引けば縦横 [-1, 1] の範囲の点群 position が得られる。
  int x = gl_VertexID >> 1;
  int y = gl_InstanceID + 1 - (gl_VertexID & 1);
  vec2 position = vec2(x, y) * gap - 1.0;

  // 頂点位置をそのままテクスチャ座標に使う
  texcoord = position * 0.5 + 0.5;

  // 視線ベクトル
  //   position にスクリーンの大きさ screen.st をかけて中心位置 screen.pq を足せば、
  //   スクリーン上の点の位置 p が得られるから、原点にある視点からこの点に向かう視線は、
  //   焦点距離を 1 スクリーンの大きさの二分の一を size としたとき (p * size, -1) となる。
  //   これを回転したあと正規化して、その方向の視線単位ベクトルを得る。
  vec4 vector = normalize(rotation * vec4(position * screen, -1.0, 0.0));

  // 頂点位置を魚眼画像空間に変換する
  gl_Position = vec4(acos(-vector.z) * normalize(vector.xy) * 0.31830987, 0.0, 1.0);
}

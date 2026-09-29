#pragma once

///
/// Quest 3 版で使う 4x4 行列の補助関数
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
/// @details
/// 行列は PC 版の gg::GgMatrix と同じ列優先 (column-major) の float[16] で表す。
/// ネットワークで送受信する姿勢行列は、このメモリ配置のままバイト列として扱う。
///

#include <array>
#include <cmath>

#include <openxr/openxr.h>

namespace qm
{
  /// 列優先の 4x4 行列 (gg::GgMatrix と同じ 64 バイトの配置)
  using Mat4 = std::array<float, 16>;

  static_assert(sizeof(Mat4) == 16 * sizeof(float), "Mat4 must be tightly packed");

  ///
  /// 単位行列
  ///
  inline Mat4 identity()
  {
    return Mat4{
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f };
  }

  ///
  /// 零行列 (トラッキングしていない手の関節を表す)
  ///
  inline Mat4 zero()
  {
    return Mat4{};
  }

  ///
  /// 零行列かどうか (同次座標の w 成分が 0 なら描画に使えない)
  ///
  inline bool isZero(const Mat4& m)
  {
    return m[15] == 0.0f;
  }

  ///
  /// 行列の積 a * b
  ///
  inline Mat4 multiply(const Mat4& a, const Mat4& b)
  {
    Mat4 c{};
    for (int col = 0; col < 4; ++col)
    {
      for (int row = 0; row < 4; ++row)
      {
        float s{ 0.0f };
        for (int k = 0; k < 4; ++k) s += a[k * 4 + row] * b[col * 4 + k];
        c[col * 4 + row] = s;
      }
    }
    return c;
  }

  inline Mat4 operator*(const Mat4& a, const Mat4& b)
  {
    return multiply(a, b);
  }

  ///
  /// 平行移動の変換行列
  ///
  inline Mat4 translate(float x, float y, float z)
  {
    Mat4 m{ identity() };
    m[12] = x;
    m[13] = y;
    m[14] = z;
    return m;
  }

  ///
  /// OpenXR の姿勢から変換行列を作る
  ///
  inline Mat4 fromPose(const XrPosef& pose)
  {
    const float x{ pose.orientation.x }, y{ pose.orientation.y };
    const float z{ pose.orientation.z }, w{ pose.orientation.w };
    const float xx{ x * x }, yy{ y * y }, zz{ z * z };
    const float xy{ x * y }, xz{ x * z }, yz{ y * z };
    const float wx{ w * x }, wy{ w * y }, wz{ w * z };

    return Mat4{
      1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz), 2.0f * (xz - wy), 0.0f,
      2.0f * (xy - wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx), 0.0f,
      2.0f * (xz + wy), 2.0f * (yz - wx), 1.0f - 2.0f * (xx + yy), 0.0f,
      pose.position.x, pose.position.y, pose.position.z, 1.0f };
  }

  ///
  /// 回転と平行移動だけからなる変換行列の逆行列
  ///
  inline Mat4 invertRigid(const Mat4& m)
  {
    Mat4 r{ identity() };

    // 回転部分は転置する
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        r[j * 4 + i] = m[i * 4 + j];

    // 平行移動は -R^T t
    for (int i = 0; i < 3; ++i)
      r[12 + i] = -(r[i] * m[12] + r[4 + i] * m[13] + r[8 + i] * m[14]);

    return r;
  }

  ///
  /// OpenXR の視野角から投影変換行列を作る (OpenGL ES のクリップ空間)
  ///
  inline Mat4 projection(const XrFovf& fov, float zNear, float zFar)
  {
    const float left{ std::tan(fov.angleLeft) }, right{ std::tan(fov.angleRight) };
    const float down{ std::tan(fov.angleDown) }, up{ std::tan(fov.angleUp) };
    const float width{ right - left }, height{ up - down };

    Mat4 m{};
    m[0] = 2.0f / width;
    m[5] = 2.0f / height;
    m[8] = (right + left) / width;
    m[9] = (up + down) / height;
    m[10] = -(zFar + zNear) / (zFar - zNear);
    m[11] = -1.0f;
    m[14] = -2.0f * zFar * zNear / (zFar - zNear);
    return m;
  }
}

// std::array の演算子は引数依存の名前探索で qm 名前空間から見つからないので、大域に取り込む
using qm::operator*;

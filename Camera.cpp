///
/// カメラ関連の基底クラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date November 15, 2022
///
#include "Camera.h"
#include "gg.h"

//
// カメラフレームを OpenGL テクスチャへ転送する
//
bool Camera::transmit(int eye, unsigned int texture, const int* size)
{
  return lockFrame(eye, [texture, size](const std::uint8_t* data, size_t length, int width, int height, int channels) {
    // 画像とテクスチャの大きさが違えば転送しない (バッファの範囲外を読まないようにする)
    if (width != size[0] || height != size[1]) return;

    // 3 チャンネルの BGR の画像は行の境界が詰まっている
    glPixelStorei(GL_UNPACK_ALIGNMENT, channels == 4 ? 4 : 1);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size[0], size[1],
      channels == 4 ? GL_BGRA : GL_BGR, GL_UNSIGNED_BYTE, data);
  });
}

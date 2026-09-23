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
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size[0], size[1],
      channels == 4 ? GL_BGRA : GL_BGR, GL_UNSIGNED_BYTE, data);
  });
}

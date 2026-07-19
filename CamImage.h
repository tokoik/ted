#pragma once

///
/// 静止画を使うクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///

//
// 静止画
//

// カメラ関連の処理
#include "Camera.h"

///
/// 静止画を使うクラス
///
class CamImage
  : public Camera
{
public:

  ///
  /// コンストラクタ
  ///
  CamImage();

  ///
  /// デストラクタ
  ///
  virtual ~CamImage();

  ///
  /// ファイルから入力する
  ///
  /// @param file 画像ファイル名
  /// @param cam カメラ番号
  /// @return 成功した場合は true
  ///
  bool open(const std::string& file, int cam);

  ///
  /// カメラが使用可能か判定する
  ///
  /// @param cam カメラ番号
  /// @return 使用可能な場合は true
  ///
  bool opened(int cam);

  ///
  /// このカメラでは画像の転送を行わない
  ///
  /// @param cam カメラ番号
  /// @param texture 転送先のテクスチャ
  /// @param size 転送する画像の大きさ
  /// @return 成功した場合は true
  ///
  bool CamImage::transmit(int cam, GLuint texture, const GLsizei* size);

  ///
  /// 読み込んだ画像のデータを得る
  ///
  /// @param cam カメラ番号
  /// @return 画像のデータへの読み取り専用ポインタ
  ///
  const GLubyte* getImage(int cam);
};

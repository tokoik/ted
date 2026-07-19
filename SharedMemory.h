#pragma once

///
/// 共有メモリクラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///

// 補助プログラム
#include "gg.h"
using namespace gg;

// Win32 API
#include <Windows.h>

///
/// 共有メモリクラス
///
/// @details
/// 同一PC上の処理間で姿勢行列を共有する名前付きファイルマッピング。
/// 全アクセスを同名のWin32ミューテックスで直列化し、読み書き途中の行列を見せない。
///
class SharedMemory
{
  /// 共有メモリ用のミューテックスオブジェクト
  HANDLE hMutex{ nullptr };

  /// 共有メモリのハンドル
  HANDLE hShare{ nullptr };

  /// 共有メモリの先頭へのポインタ
  GgMatrix* pShare{ nullptr };

  /// 共有メモリの全要素数
  const unsigned int size;

public:

  ///
  /// コンストラクタ
  ///
  /// @param strMutexName ミューテックスオブジェクトの名前
  /// @param strShareName 共有メモリの名前
  /// @param size 共有メモリの要素数
  ///
  SharedMemory(const LPCTSTR strMutexName, const LPCTSTR strShareName, unsigned int size);

  ///
  /// デストラクタ
  ///
  virtual ~SharedMemory();

  ///
  /// ミューテックスオブジェクトを獲得する（獲得できるまで待つ）
  ///
  bool lock() const;

  ///
  /// ミューテックスオブジェクトを獲得する（獲得できなかったら false を返す）
  ///
  /// @return 獲得できた場合は true、獲得できなかった場合は false
  ///
  bool try_lock() const;

  ///
  /// ミューテックスオブジェクトを解放する
  ///
  void unlock() const;

  ///
  /// 確保した共有メモリの要素の数を得る
  ///
  /// @return 確保した共有メモリの要素の数
  ///
  unsigned int getSize() const;

  ///
  /// 確保した共有メモリのアドレスを得る
  ///
  /// @return 確保した共有メモリのアドレス
  ///
  const GgMatrix* get() const;

  ///
  /// 共有メモリの要素を取り出す
  ///
  /// @param i 取り出す要素のインデックス
  /// @param m 取り出した要素を格納する変換行列
  ///
  void get(unsigned int i, GgMatrix& m) const;

  ///
  /// 共有メモリの要素に格納する
  ///
  /// @param i 格納する要素のインデックス
  /// @param m 格納する変換行列
  ///
  void set(unsigned int i, const GgMatrix& m);

  ///
  /// 共有メモリの複数の要素に値を設定する
  ///
  /// @param i 開始インデックス
  /// @param count 設定する要素の数
  /// @param m 設定する変換行列
  ///
  void set(unsigned int i, unsigned int count, const GgMatrix& m);

  ///
  /// 共有領域の先頭 count 要素を呼び出し側バッファへコピーする
  ///
  /// @param dst 呼び出し側バッファの先頭アドレス
  /// @param count コピーする要素の数
  /// 
  void load(GgMatrix* dst, unsigned int count) const;

  ///
  /// 呼び出し側バッファの先頭 count 要素を共有領域へコピーする
  ///
  /// @param src 呼び出し側バッファの先頭アドレス
  /// @param count コピーする要素の数
  ///
  void store(const GgMatrix* src, unsigned int count) const;

  ///
  /// ローカルとリモートの変換行列を同期する
  ///
  /// @details
  /// src[0, count) を共有領域へ公開し、残りを共有領域から src[count,size) へ取り込む。
  /// 1 回のロック内で双方向コピーし、ローカル姿勢とリモート姿勢の対応を保つ。
  ///
  void sync(GgMatrix* src, unsigned int count) const;
};

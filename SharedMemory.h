#pragma once

//
// 共有メモリの読出し
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// Win32 API
#include <Windows.h>

// 同一PC上の処理間で姿勢行列を共有する名前付きファイルマッピング。
// 全アクセスを同名のWin32ミューテックスで直列化し、読み書き途中の行列を見せない。
class SharedMemory
{
  // 共有メモリ用のミューテックスオブジェクト
  HANDLE hMutex{ nullptr };

  // 共有メモリのハンドル
  HANDLE hShare{ nullptr };

  // 共有メモリの先頭へのポインタ
  GgMatrix* pShare{ nullptr };

  // 共有メモリの全要素数
  const unsigned int size;

public:

  // コンストラクタ
  SharedMemory(const LPCTSTR strMutexName, const LPCTSTR strShareName, unsigned int size);

  // デストラクタ
  virtual ~SharedMemory();

  // ミューテックスオブジェクトを獲得する（獲得できるまで待つ）
  bool lock() const;

  // ミューテックスオブジェクトを獲得する（獲得できなかったら false を返す）
  bool try_lock() const;

  // ミューテックスオブジェクトを解放する
  void unlock() const;

  // 確保した共有メモリの要素の数を得る
  unsigned int getSize() const;

  // 確保した共有メモリのアドレスを得る
  const GgMatrix* get() const;

  // 共有メモリの要素を取り出す
  void get(unsigned int i, GgMatrix& m) const;

  // 共有メモリの要素に格納する
  void set(unsigned int i, const GgMatrix& m);

  // 共有メモリの複数の要素に値を設定する
  void set(unsigned int i, unsigned int count, const GgMatrix& m);

  // 共有領域の先頭 count 要素を呼び出し側バッファへコピーする
  void load(GgMatrix* dst, unsigned int count) const;

  // 呼び出し側バッファの先頭 count 要素を共有領域へコピーする
  void store(const GgMatrix* src, unsigned int count) const;

  // src[0,count) を共有領域へ公開し、残りを共有領域から src[count,size) へ取り込む。
  // 1回のロック内で双方向コピーし、ローカル姿勢とリモート姿勢の対応を保つ。
  void sync(GgMatrix* src, unsigned int count) const;
};

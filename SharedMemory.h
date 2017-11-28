#pragma once

//
// 共有メモリの読出し
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// Win32 API
#include <Windows.h>

class SharedMemory
{
  // 共有メモリ用のミューテックスオブジェクト
  HANDLE hMutex;

  // 共有メモリのハンドル
  HANDLE hShare;

  // 共有メモリの先頭へのポインタ
  GgMatrix *pShare;

  // 共有メモリの全要素数
  const unsigned int size;

  // 使用中の共有メモリの要素数
  unsigned int used;

public:

  // コンストラクタ
  SharedMemory(const LPCTSTR strMutexName, const LPCTSTR strShareName, unsigned int size);

  // デストラクタ
  virtual ~SharedMemory();

  // ミューテックスオブジェクトを獲得する（獲得できるまで待つ）
  bool lock() const
  {
    return WaitForSingleObject(hMutex, INFINITE) == WAIT_OBJECT_0;
  }

  // ミューテックスオブジェクトを獲得する（獲得できなかったら false を返す）
  bool try_lock() const
  {
    return WaitForSingleObject(hMutex, 0) == WAIT_OBJECT_0;
  }

  // ミューテックスオブジェクトを解放する
  void unlock() const
  {
    ReleaseMutex(hMutex);
  }

  // 確保した共有メモリのアドレスを得る
  const GgMatrix *get(unsigned int i = 0) const
  {
    return pShare + i;
  }

  // 共有メモリの全要素数を得る
  unsigned int getSize() const
  {
    return size;
  }

  // 使用中の共有メモリの要素数を得る
  unsigned int getUsed() const
  {
    return used;
  }

  // 共有メモリの既存の変換行列に値を設定して番号を返す
  unsigned int set(unsigned int i, const GgMatrix &m) const
  {
    if (i >= used) return ~0;

    if (lock())
    {
      pShare[i] = m;
      unlock();
    }

    return i;
  }

  // 共有メモリに変換行列を追加して番号を返す
  unsigned int push(const GgMatrix &m)
  {
    if (used >= size) return ~0;

    if (lock())
    {
      pShare[used] = m;
      unlock();
    }

    return used++;
  }

  // メモリの内容を共有メモリに保存する
  void store(const void *src, unsigned int begin, unsigned int count) const;

  // 共有メモリの内容をメモリに取り出す
  void load(void *dst, unsigned int begin = 0, unsigned int count = 0) const;
};

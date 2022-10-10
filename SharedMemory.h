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
  GgMatrix* pShare;

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

  // 共有メモリの内容をメモリに取り出す
  void load(GgMatrix* dst, unsigned int count) const;

  // メモリの内容を共有メモリに保存する
  void store(const GgMatrix* src, unsigned int count) const;

  // メモリの内容を共有メモリと同期する
  void sync(GgMatrix* src, unsigned int count) const;
};

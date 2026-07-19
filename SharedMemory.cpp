///
/// 共有メモリクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date July 197, 2026
///
#include "SharedMemory.h"

// 標準ライブラリ
#include <algorithm>

//
// コンストラクタ
//
SharedMemory::SharedMemory(const LPCTSTR strMutexName, const LPCTSTR strShareName, unsigned int size)
  : size{ size }
{
  // ミューテックスオブジェクトを作成する
  hMutex = CreateMutex(NULL, FALSE, strMutexName);

  if (hMutex)
  {
    // ファイルマッピングオブジェクトを作成する
    hShare = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size * sizeof(GgMatrix), strShareName);

    if (hShare)
    {
      // 行列配列として直接読み書きできるよう、ファイルマッピングをプロセス空間へ割り当てる
      pShare = static_cast<GgMatrix*>(MapViewOfFile(hShare, FILE_MAP_WRITE, 0, 0, 0));
      if (pShare) return;
      // マッピングに失敗しても、先に作成したカーネルオブジェクトを残さない
      CloseHandle(hShare);
      hShare = nullptr;
    }

    CloseHandle(hMutex);
  }
}

//
// デストラクタ
//
SharedMemory::~SharedMemory()
{
  // 構築途中で失敗した場合にも対応できるよう、各資源を独立に確認して解放する
  if (pShare) UnmapViewOfFile(pShare);
  if (hShare) CloseHandle(hShare);
  if (hMutex) CloseHandle(hMutex);
}

//
// ミューテックスオブジェクトを獲得する（獲得できるまで待つ）
//
bool SharedMemory::lock() const
{
  return WaitForSingleObject(hMutex, INFINITE) == WAIT_OBJECT_0;
}

//
// ミューテックスオブジェクトを獲得する（獲得できなかったら false を返す）
//
bool SharedMemory::try_lock() const
{
  return WaitForSingleObject(hMutex, 0) == WAIT_OBJECT_0;
}

//
// ミューテックスオブジェクトを解放する
//
void SharedMemory::unlock() const
{
  ReleaseMutex(hMutex);
}

//
// 共有メモリの全要素数を得る
//
unsigned int SharedMemory::getSize() const
{
  return size;
}

//
// 確保した共有メモリのアドレスを得る
//
const GgMatrix* SharedMemory::get() const
{
  return pShare;
}

//
// 共有メモリの要素を取り出す
//
void SharedMemory::get(unsigned int i, GgMatrix& m) const
{
  if (i >= size) return;
  if (lock())
  {
    m = pShare[i];
    unlock();
  }
}

//
// 共有メモリの要素に格納する
//
void SharedMemory::set(unsigned int i, const GgMatrix& m)
{
  if (i >= size) return;
  if (lock())
  {
    pShare[i] = m;
    unlock();
  }
}

//
// 共有メモリの複数の要素に値を設定する
//
void SharedMemory::set(unsigned int i, unsigned int count, const GgMatrix& m)
{
  if (i >= size) return;
  // iから共有領域末尾までの要素数に切り詰め、部分初期化を範囲内に限定する
  count = std::min(count, size - i);
  if (lock())
  {
    std::fill(pShare + i, pShare + i + count, m);
    unlock();
  }
}

//
// メモリの内容を共有メモリに保存する
//
void SharedMemory::store(const GgMatrix* src, unsigned int count) const
{
  if (count > size) count = size;
  if (lock())
  {
    std::copy(src, src + count, pShare);
    unlock();
  }
}

//
// メモリの内容を共有メモリと同期する
//
void SharedMemory::sync(GgMatrix* src, unsigned int count) const
{
  if (count > size) count = size;
  if (lock())
  {
    std::copy(src, src + count, pShare);
    std::copy(pShare + count, pShare + size, src + count);
    unlock();
  }
}

//
// 共有メモリの内容をメモリに取り出す
//
void SharedMemory::load(GgMatrix* dst, unsigned int count) const
{
  if (count > size) count = size;

  if (lock())
  {
    std::copy(pShare, pShare + count, dst);
    unlock();
  }
}

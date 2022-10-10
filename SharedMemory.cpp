//
// 共有メモリの読出し
//
#include "SharedMemory.h"

// コンストラクタ
SharedMemory::SharedMemory(const LPCTSTR strMutexName, const LPCTSTR strShareName, unsigned int size)
  : pShare(nullptr)
  , size(size)
{
  // ミューテックスオブジェクトを作成する
  hMutex = CreateMutex(NULL, FALSE, strMutexName);

  if (hMutex)
  {
    // ファイルマッピングオブジェクトを作成する
    hShare = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size * sizeof(GgMatrix), strShareName);

    if (hShare)
    {
      // ファイルマッピングオブジェクトをメモリにマップする
      pShare = static_cast<GgMatrix*>(MapViewOfFile(hShare, FILE_MAP_WRITE, 0, 0, 0));
      return;
    }

    CloseHandle(hMutex);
  }
}

// デストラクタ
SharedMemory::~SharedMemory()
{
  // 共有メモリが有効なら
  if (pShare)
  {
    // 共有メモリを開放する
    UnmapViewOfFile(pShare);
    CloseHandle(hShare);
    CloseHandle(hMutex);
  }
}

// ミューテックスオブジェクトを獲得する（獲得できるまで待つ）
bool SharedMemory::lock() const
{
  return WaitForSingleObject(hMutex, INFINITE) == WAIT_OBJECT_0;
}

// ミューテックスオブジェクトを獲得する（獲得できなかったら false を返す）
bool SharedMemory::try_lock() const
{
  return WaitForSingleObject(hMutex, 0) == WAIT_OBJECT_0;
}

// ミューテックスオブジェクトを解放する
void SharedMemory::unlock() const
{
  ReleaseMutex(hMutex);
}

// 共有メモリの全要素数を得る
unsigned int SharedMemory::getSize() const
{
  return size;
}

// 確保した共有メモリのアドレスを得る
const GgMatrix* SharedMemory::get() const
{
  return pShare;
}

// 共有メモリの要素を取り出す
void SharedMemory::get(unsigned int i, GgMatrix& m) const
{
  if (i >= size) return;
  if (lock())
  {
    m = pShare[i];
    unlock();
  }
}

// 共有メモリの要素に格納する
void SharedMemory::set(unsigned int i, const GgMatrix& m)
{
  if (i >= size) return;
  if (lock())
  {
    pShare[i] = m;
    unlock();
  }
}

// 共有メモリの複数の要素に値を設定する
void SharedMemory::set(unsigned int i, unsigned int count, const GgMatrix& m)
{
  if (i >= size) return;
  if (count += i >= size) count = size;
  if (lock())
  {
    std::fill(pShare + i, pShare + count, m);
    unlock();
  }
}

// メモリの内容を共有メモリに保存する
void SharedMemory::store(const GgMatrix* src, unsigned int count) const
{
  if (count > size) count = size;
  if (lock())
  {
    std::copy(src, src + count, pShare);
    unlock();
  }
}

// メモリの内容を共有メモリと同期する
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

// 共有メモリの内容をメモリに取り出す
void SharedMemory::load(GgMatrix* dst, unsigned int count) const
{
  if (count > size) count = size;

  if (lock())
  {
    std::copy(pShare, pShare + count, dst);
    unlock();
  }
}

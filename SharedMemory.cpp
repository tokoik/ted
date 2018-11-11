//
// 共有メモリの読出し
//
#include "SharedMemory.h"

// コンストラクタ
SharedMemory::SharedMemory(const LPCTSTR strMutexName, const LPCTSTR strShareName, unsigned int size)
  : pShare(nullptr)
  , size(size)
  , used(0)
{
  // ミューテックスオブジェクトを作成する
  hMutex = CreateMutex(NULL, FALSE, strMutexName);

  if (hMutex)
  {
    // ファイルマッピングオブジェクトを作成する
    hShare = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size * sizeof (GgMatrix), strShareName);

    if (hShare)
    {
      // ファイルマッピングオブジェクトをメモリにマップする
      pShare = static_cast<GgMatrix *>(MapViewOfFile(hShare, FILE_MAP_WRITE, 0, 0, 0));
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

// 確保した共有メモリのアドレスを得る
const GgMatrix *SharedMemory::get() const
{
  return pShare;
}

// 確保した共有メモリの要素を得る
const GgMatrix &SharedMemory::get(int i) const
{
  return pShare[i];
}

// 共有メモリの全要素数を得る
unsigned int SharedMemory::getSize() const
{
  return size;
}

// 使用中の共有メモリの要素数を得る
unsigned int SharedMemory::getUsed() const
{
  return used;
}

// 共有メモリの既存の変換行列に値を設定して番号を返す
unsigned int SharedMemory::set(unsigned int i, const GgMatrix &m) const
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
unsigned int SharedMemory::push(const GgMatrix &m)
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
void SharedMemory::store(const GgMatrix *src, unsigned int count) const
{
  if (count > used) count = used;
  if (lock())
  {
    std::copy(src, src + count, pShare);
    unlock();
  }
}

// 共有メモリの内容をメモリに取り出す
void SharedMemory::load(GgMatrix *dst, unsigned int count) const
{
  if (count > used) count = used;

  if (lock())
  {
    std::copy(pShare, pShare + count, dst);
    unlock();
  }
}

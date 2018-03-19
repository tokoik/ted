//
// 共有メモリの読出し
//
#include "SharedMemory.h"

// ファイルマッピングオブジェクト名
const LPCWSTR localMutexName = L"TED_LOCAL_MUTEX";
const LPCWSTR localShareName = L"TED_LOCAL_SHARE";
const LPCWSTR remoteMutexName = L"TED_REMOTE_MUTEX";
const LPCWSTR remoteShareName = L"TED_REMOTE_SHARE";

// 共有メモリ上に置く操縦者の変換行列
std::unique_ptr<SharedMemory> localAttitude(nullptr);

// 共有メモリ上に置く作業者の変換行列
std::unique_ptr<SharedMemory> remoteAttitude(nullptr);

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

// 共有メモリの確保と初期化
bool SharedMemory::initialize(unsigned int local_size, unsigned int remote_size, unsigned int count)
{
  // ローカルの変換行列を保持する共有メモリを確保する
  localAttitude.reset(new SharedMemory(localMutexName, localShareName, local_size));

  // ローカルの変換行列を保持する共有メモリが確保できたかチェックする
  if (!localAttitude->get()) return false;

  // リモートの変換行列を保持する共有メモリを確保する
  remoteAttitude.reset(new SharedMemory(remoteMutexName, remoteShareName, remote_size));

  // リモートの変換行列を保持する共有メモリが確保できたかチェックする
  if (!remoteAttitude->get()) return false;

  // ローカルの変換行列の最初の count 個をあらかじめ初期化しておく
  for (unsigned int i = 0; i < count; ++i) localAttitude->push(ggIdentity());

  // 共有メモリの確保に成功した
  return true;
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
void SharedMemory::store(const void *src, unsigned int begin, unsigned int count) const
{
  if (begin >= used) return;
  if (count == 0) count = used;
  const unsigned int last(begin + count);
  if (last > used) count -= last - used;

  if (lock())
  {
    std::copy(static_cast<const GgMatrix *>(src), static_cast<const GgMatrix *>(src) + count, pShare + begin);
    unlock();
  }
}

// 共有メモリの内容をメモリに取り出す
void SharedMemory::load(void *dst, unsigned int begin, unsigned int count) const
{
  if (begin >= used) return;
  if (count == 0) count = used;
  unsigned int last(begin + count);
  if (last > used) last = used;

  if (lock())
  {
    std::copy(pShare + begin, pShare + last, static_cast<GgMatrix *>(dst));
    unlock();
  }
}


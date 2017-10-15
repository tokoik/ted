#include "SharedMemory.h"

//
// 共有メモリの読出し
//

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

    CloseHandle(hShare);
  }

  CloseHandle(hMutex);
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

#include "SharedMemory.h"

//
// ���L�������̓Ǐo��
//

// �R���X�g���N�^
SharedMemory::SharedMemory(const LPCTSTR strMutexName, const LPCTSTR strShareName, unsigned int size)
  : pShare(nullptr)
  , size(size)
  , used(0)
{
  // �~���[�e�b�N�X�I�u�W�F�N�g���쐬����
  hMutex = CreateMutex(NULL, FALSE, strMutexName);

  if (hMutex)
  {
    // �t�@�C���}�b�s���O�I�u�W�F�N�g���쐬����
    hShare = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size * sizeof (GgMatrix), strShareName);

    if (hShare)
    {
      // �t�@�C���}�b�s���O�I�u�W�F�N�g���������Ƀ}�b�v����
      pShare = static_cast<GgMatrix *>(MapViewOfFile(hShare, FILE_MAP_WRITE, 0, 0, 0));
      return;
    }

    CloseHandle(hShare);
  }

  CloseHandle(hMutex);
}

// �f�X�g���N�^
SharedMemory::~SharedMemory()
{
  // ���L���������L���Ȃ�
  if (pShare)
  {
    // ���L���������J������
    UnmapViewOfFile(pShare);
    CloseHandle(hShare);
    CloseHandle(hMutex);
  }
}

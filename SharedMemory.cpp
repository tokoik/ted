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

// �������̓��e�����L�������ɕۑ�����
void SharedMemory::store(const void *src, unsigned int begin, unsigned int count) const
{
  if (begin >= used) return;
  if (count == 0) count = used;
  const unsigned int last(begin + count);
  if (last > used) count -= last - used;
  if (lock())
  {
    for (unsigned int i = 0; i < count; ++i) pShare[begin++] = static_cast<const GgMatrix *>(src)[i];
    unlock();
  }
}

// ���L�������̓��e���������Ɏ��o��
void SharedMemory::load(void *dst, unsigned int begin, unsigned int count) const
{
  if (begin >= used) return;
  if (count == 0) count = used;
  const unsigned int last(begin + count);
  if (last > size) count -= last - used;
  if (lock())
  {
    for (unsigned int i = 0; i < count; ++i) static_cast<GgMatrix *>(dst)[i] = pShare[begin++];
    unlock();
  }
}

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

// �~���[�e�b�N�X�I�u�W�F�N�g���l������i�l���ł���܂ő҂j
bool SharedMemory::lock() const
{
  return WaitForSingleObject(hMutex, INFINITE) == WAIT_OBJECT_0;
}

// �~���[�e�b�N�X�I�u�W�F�N�g���l������i�l���ł��Ȃ������� false ��Ԃ��j
bool SharedMemory::try_lock() const
{
  return WaitForSingleObject(hMutex, 0) == WAIT_OBJECT_0;
}

// �~���[�e�b�N�X�I�u�W�F�N�g���������
void SharedMemory::unlock() const
{
  ReleaseMutex(hMutex);
}

// �m�ۂ������L�������̃A�h���X�𓾂�
const GgMatrix *SharedMemory::get() const
{
  return pShare;
}

// ���L�������̑S�v�f���𓾂�
unsigned int SharedMemory::getSize() const
{
  return size;
}

// �g�p���̋��L�������̗v�f���𓾂�
unsigned int SharedMemory::getUsed() const
{
  return used;
}

// ���L�������̊����̕ϊ��s��ɒl��ݒ肵�Ĕԍ���Ԃ�
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

// ���L�������ɕϊ��s���ǉ����Ĕԍ���Ԃ�
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

// �������̓��e�����L�������ɕۑ�����
void SharedMemory::store(const void *src, unsigned int begin, unsigned int count) const
{
  if (begin >= used) return;
  if (count == 0) count = used;
  const unsigned int last(begin + count);
  if (last > used) count -= last - used;

  if (lock(), true)
  {
    std::copy(static_cast<const GgMatrix *>(src), static_cast<const GgMatrix *>(src) + count, pShare + begin);
    unlock();
  }
}

// ���L�������̓��e���������Ɏ��o��
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

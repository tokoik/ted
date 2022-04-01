//
// ���L�������̓Ǐo��
//
#include "SharedMemory.h"

// �R���X�g���N�^
SharedMemory::SharedMemory(const LPCTSTR strMutexName, const LPCTSTR strShareName, unsigned int size)
  : pShare(nullptr)
  , size(size)
{
  // �~���[�e�b�N�X�I�u�W�F�N�g���쐬����
  hMutex = CreateMutex(NULL, FALSE, strMutexName);

  if (hMutex)
  {
    // �t�@�C���}�b�s���O�I�u�W�F�N�g���쐬����
    hShare = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size * sizeof(GgMatrix), strShareName);

    if (hShare)
    {
      // �t�@�C���}�b�s���O�I�u�W�F�N�g���������Ƀ}�b�v����
      pShare = static_cast<GgMatrix*>(MapViewOfFile(hShare, FILE_MAP_WRITE, 0, 0, 0));
      return;
    }

    CloseHandle(hMutex);
  }
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

// ���L�������̑S�v�f���𓾂�
unsigned int SharedMemory::getSize() const
{
  return size;
}

// �m�ۂ������L�������̃A�h���X�𓾂�
const GgMatrix* SharedMemory::get() const
{
  return pShare;
}

// ���L�������̗v�f�����o��
void SharedMemory::get(unsigned int i, GgMatrix& m) const
{
  if (i >= size) return;
  if (lock())
  {
    m = pShare[i];
    unlock();
  }
}

// ���L�������̗v�f�Ɋi�[����
void SharedMemory::set(unsigned int i, const GgMatrix& m)
{
  if (i >= size) return;
  if (lock())
  {
    pShare[i] = m;
    unlock();
  }
}

// ���L�������̕����̗v�f�ɒl��ݒ肷��
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

// �������̓��e�����L�������ɕۑ�����
void SharedMemory::store(const GgMatrix* src, unsigned int count) const
{
  if (count > size) count = size;
  if (lock())
  {
    std::copy(src, src + count, pShare);
    unlock();
  }
}

// �������̓��e�����L�������Ɠ�������
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

// ���L�������̓��e���������Ɏ��o��
void SharedMemory::load(GgMatrix* dst, unsigned int count) const
{
  if (count > size) count = size;

  if (lock())
  {
    std::copy(pShare, pShare + count, dst);
    unlock();
  }
}

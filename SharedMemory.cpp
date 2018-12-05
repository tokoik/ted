//
// ���L�������̓Ǐo��
//
#include "SharedMemory.h"

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

// �m�ۂ������L�������̃A�h���X�𓾂�
const GgMatrix *SharedMemory::get() const
{
  return pShare;
}

// �m�ۂ������L�������̗v�f�𓾂�
const GgMatrix &SharedMemory::get(int i) const
{
  return pShare[i];
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
void SharedMemory::store(const GgMatrix *src, unsigned int count) const
{
  if (count > used) count = used;
  if (lock())
  {
    std::copy(src, src + count, pShare);
    unlock();
  }
}

// ���L�������̓��e���������Ɏ��o��
void SharedMemory::load(GgMatrix *dst, unsigned int count) const
{
  if (count > used) count = used;

  if (lock())
  {
    std::copy(pShare, pShare + count, dst);
    unlock();
  }
}

//
// ���L�������̓Ǐo��
//
#include "SharedMemory.h"

// �t�@�C���}�b�s���O�I�u�W�F�N�g��
const LPCWSTR localMutexName = L"TED_LOCAL_MUTEX";
const LPCWSTR localShareName = L"TED_LOCAL_SHARE";
const LPCWSTR remoteMutexName = L"TED_REMOTE_MUTEX";
const LPCWSTR remoteShareName = L"TED_REMOTE_SHARE";

// ���L��������ɒu�����c�҂̕ϊ��s��
std::unique_ptr<SharedMemory> localAttitude(nullptr);

// ���L��������ɒu����Ǝ҂̕ϊ��s��
std::unique_ptr<SharedMemory> remoteAttitude(nullptr);

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

// ���L�������̊m�ۂƏ�����
bool SharedMemory::initialize(unsigned int local_size, unsigned int remote_size, unsigned int count)
{
  // ���[�J���̕ϊ��s���ێ����鋤�L���������m�ۂ���
  localAttitude.reset(new SharedMemory(localMutexName, localShareName, local_size));

  // ���[�J���̕ϊ��s���ێ����鋤�L���������m�ۂł������`�F�b�N����
  if (!localAttitude->get()) return false;

  // �����[�g�̕ϊ��s���ێ����鋤�L���������m�ۂ���
  remoteAttitude.reset(new SharedMemory(remoteMutexName, remoteShareName, remote_size));

  // �����[�g�̕ϊ��s���ێ����鋤�L���������m�ۂł������`�F�b�N����
  if (!remoteAttitude->get()) return false;

  // ���[�J���̕ϊ��s��̍ŏ��� count �����炩���ߏ��������Ă���
  for (unsigned int i = 0; i < count; ++i) localAttitude->push(ggIdentity());

  // ���L�������̊m�ۂɐ�������
  return true;
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

  if (lock())
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


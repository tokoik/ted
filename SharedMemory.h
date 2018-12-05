#pragma once

//
// ���L�������̓Ǐo��
//

// �⏕�v���O����
#include "gg.h"
using namespace gg;

// Win32 API
#include <Windows.h>

class SharedMemory
{
  // ���L�������p�̃~���[�e�b�N�X�I�u�W�F�N�g
  HANDLE hMutex;

  // ���L�������̃n���h��
  HANDLE hShare;

  // ���L�������̐擪�ւ̃|�C���^
  GgMatrix *pShare;

  // ���L�������̑S�v�f��
  const unsigned int size;

  // �g�p���̋��L�������̗v�f��
  unsigned int used;

public:

  // �R���X�g���N�^
  SharedMemory(const LPCTSTR strMutexName, const LPCTSTR strShareName, unsigned int size);

  // �f�X�g���N�^
  virtual ~SharedMemory();

  // �~���[�e�b�N�X�I�u�W�F�N�g���l������i�l���ł���܂ő҂j
  bool lock() const
  {
    return WaitForSingleObject(hMutex, INFINITE) == WAIT_OBJECT_0;
  }

  // �~���[�e�b�N�X�I�u�W�F�N�g���l������i�l���ł��Ȃ������� false ��Ԃ��j
  bool try_lock() const
  {
    return WaitForSingleObject(hMutex, 0) == WAIT_OBJECT_0;
  }

  // �~���[�e�b�N�X�I�u�W�F�N�g���������
  void unlock() const
  {
    ReleaseMutex(hMutex);
  }

  // �m�ۂ������L�������̃A�h���X�𓾂�
  const GgMatrix *get(unsigned int i = 0) const
  {
    return pShare + i;
  }

  // ���L�������̑S�v�f���𓾂�
  unsigned int getSize() const
  {
    return size;
  }

  // �g�p���̋��L�������̗v�f���𓾂�
  unsigned int getUsed() const
  {
    return used;
  }

  // ���L�������̊����̕ϊ��s��ɒl��ݒ肵�Ĕԍ���Ԃ�
  unsigned int set(unsigned int i, const GgMatrix &m) const
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
  unsigned int push(const GgMatrix &m)
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
  void store(const void *src, unsigned int begin, unsigned int count) const;

  // ���L�������̓��e���������Ɏ��o��
  void load(void *dst, unsigned int begin = 0, unsigned int count = 0) const;
};

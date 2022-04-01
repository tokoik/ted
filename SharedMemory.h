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
  GgMatrix* pShare;

  // ���L�������̑S�v�f��
  const unsigned int size;

public:

  // �R���X�g���N�^
  SharedMemory(const LPCTSTR strMutexName, const LPCTSTR strShareName, unsigned int size);

  // �f�X�g���N�^
  virtual ~SharedMemory();

  // �~���[�e�b�N�X�I�u�W�F�N�g���l������i�l���ł���܂ő҂j
  bool lock() const;

  // �~���[�e�b�N�X�I�u�W�F�N�g���l������i�l���ł��Ȃ������� false ��Ԃ��j
  bool try_lock() const;

  // �~���[�e�b�N�X�I�u�W�F�N�g���������
  void unlock() const;

  // �m�ۂ������L�������̗v�f�̐��𓾂�
  unsigned int getSize() const;

  // �m�ۂ������L�������̃A�h���X�𓾂�
  const GgMatrix* get() const;

  // ���L�������̗v�f�����o��
  void get(unsigned int i, GgMatrix& m) const;

  // ���L�������̗v�f�Ɋi�[����
  void set(unsigned int i, const GgMatrix& m);

  // ���L�������̕����̗v�f�ɒl��ݒ肷��
  void set(unsigned int i, unsigned int count, const GgMatrix& m);

  // ���L�������̓��e���������Ɏ��o��
  void load(GgMatrix* dst, unsigned int count) const;

  // �������̓��e�����L�������ɕۑ�����
  void store(const GgMatrix* src, unsigned int count) const;

  // �������̓��e�����L�������Ɠ�������
  void sync(GgMatrix* src, unsigned int count) const;
};

#pragma once

//
// ���L�������̓Ǐo��
//

// �⏕�v���O����
#include "gg.h"
using namespace gg;

// �W�����C�u����
#include <memory>

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

  // ���L�������̊m�ۂƏ�����
  static bool initialize(unsigned int local_size, unsigned int remote_size, unsigned int count);

  // �~���[�e�b�N�X�I�u�W�F�N�g���l������i�l���ł���܂ő҂j
  bool lock() const;

  // �~���[�e�b�N�X�I�u�W�F�N�g���l������i�l���ł��Ȃ������� false ��Ԃ��j
  bool try_lock() const;

  // �~���[�e�b�N�X�I�u�W�F�N�g���������
  void unlock() const;

  // �m�ۂ������L�������̃A�h���X�𓾂�
  const GgMatrix *get() const;

  // ���L�������̑S�v�f���𓾂�
  unsigned int getSize() const;

  // �g�p���̋��L�������̗v�f���𓾂�
  unsigned int getUsed() const;

  // ���L�������̊����̕ϊ��s��ɒl��ݒ肵�Ĕԍ���Ԃ�
  unsigned int set(unsigned int i, const GgMatrix &m) const;

  // ���L�������ɕϊ��s���ǉ����Ĕԍ���Ԃ�
  unsigned int push(const GgMatrix &m);

  // �������̓��e�����L�������ɕۑ�����
  void store(const void *src, unsigned int begin, unsigned int count) const;

  // ���L�������̓��e���������Ɏ��o��
  void load(void *dst, unsigned int begin = 0, unsigned int count = 0) const;
};

// ���L��������ɒu�����c�҂̕ϊ��s��
extern std::unique_ptr<SharedMemory> localAttitude;

// ���L��������ɒu����Ǝ҂̕ϊ��s��
extern std::unique_ptr<SharedMemory> remoteAttitude;

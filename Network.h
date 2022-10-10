#pragma once

//
// �l�b�g���[�N�֘A�̏���
//

#if defined(_WIN32)
#  include <winsock2.h>
#endif
#include <cstddef>

class Network
{
  // �\�P�b�g
  SOCKET recvSock, sendSock;
  sockaddr_in recvAddr, sendAddr;

  // ����
  enum class Role : int { STANDALONE = 0, OPERATOR, WORKER } role;

public:

  // �R���X�g���N�^
  Network();

  // �f�X�g���N�^
  virtual ~Network();

  // �G���[�R�[�h�̕\��
  int getError() const;

  // ������ (role == 0 : STANDALONE, 1: OPERATOR, 2: WORKER)
  int initializeRecv(unsigned short port);
  int initializeSend(unsigned short port, const char* address);
  int initialize(int role, unsigned short port, const char* address);

  // �I������
  void finalize();

  // ���s���Ȃ�^
  bool running() const;

  // STANDALONE �Ȃ�^
  bool isStandalone() const;

  // �w���҂Ȃ�^
  bool isInstructor() const;

  // ��Ǝ҂Ȃ�^
  bool isWorker() const;

  // �����[�g�̃A�h���X�̃`�F�b�N
  bool checkRemote() const;

  // 1 �p�P�b�g��M
  int recvPacket(void* buf, int len);

  // 1 �p�P�b�g���M
  int sendPacket(const void* buf, int len) const;

  // 1 �t���[����M
  int recvData(void* buf, int len);

  // 1 �t���[�����M
  unsigned int sendData(const void* buf, int len) const;

  // EOF ���M
  int sendEof() const;
};

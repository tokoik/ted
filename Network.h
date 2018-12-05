#pragma once

#if defined(_WIN32)
#  include <winsock2.h>
#endif

class Network
{
  // �\�P�b�g
  SOCKET sendSock, recvSock;
  sockaddr_in sendAddr, recvAddr;

  // true �Ȃ瑗�M��
  bool sender;

public:

  // �G���[�R�[�h�̕\��
  int getError() const;

  // �R���X�g���N�^
  Network()
    : sendSock(INVALID_SOCKET), recvSock(INVALID_SOCKET), sender(false)
  {
  }

  // �R���X�g���N�^ (role == 0 : STANDALONE, 1: OPERATOR, 2: WORKER)
  Network(int role, unsigned short port, const char *address)
    : Network()
  {
    initialize(role, port, address);
  }

  // �f�X�g���N�^
  ~Network()
  {
    finalize();
  }

  // �������i�A�h���X���w�肵�Ă��Ȃ���Α��c�ҁC���Ă���΍�Ǝҁj
  int initialize(int role, unsigned short port, const char *address = nullptr);

  // �I������
  void finalize()
  {
    if (running())
    {
      if (sendSock != INVALID_SOCKET) closesocket(sendSock);
      if (recvSock != INVALID_SOCKET) closesocket(recvSock);
      sendSock = recvSock = INVALID_SOCKET;
    }
  }

  // ���s���Ȃ�^
  bool running() const
  {
    return sendSock != INVALID_SOCKET;
  }

  // ���M���Ȃ�^
  bool isSender() const
  {
    return sender;
  }

  // �f�[�^���M
  int send(const void *buf, unsigned int len) const;

  // �f�[�^��M
  int recv(void *buf, unsigned int len);

  // 1 �t���[�����M
  int sendFrame(const void *buf, unsigned int len) const;

  // 1 �t���[����M
  int recvFrame(void *buf, unsigned int len);
};

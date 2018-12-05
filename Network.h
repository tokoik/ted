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

  // �R���X�g���N�^
  Network();

  // �R���X�g���N�^ (role == 0 : STANDALONE, 1: OPERATOR, 2: WORKER)
  Network(int role, unsigned short port, const char *address);

  // �f�X�g���N�^
  virtual ~Network();

  // �G���[�R�[�h�̕\��
  int getError() const;

  // �������i�A�h���X���w�肵�Ă��Ȃ���Α��c�ҁC���Ă���΍�Ǝҁj
  int initialize(int role, unsigned short port, const char *address = nullptr);

  // �I������
  void finalize();

  // ���s���Ȃ�^
  bool running() const;

  // ���M���Ȃ�^
  bool isSender() const;

  // �f�[�^���M
  int send(const void *buf, unsigned int len) const;

  // �f�[�^��M
  int recv(void *buf, unsigned int len);

  // 1 �t���[�����M
  unsigned int sendFrame(const void *buf, unsigned int len) const;

  // 1 �t���[����M
  int recvFrame(void *buf, unsigned int len);
};

#include "Window.h"
#include "Network.h"
#if defined(_WIN32)
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
#endif
#include <iostream>

// �ő�f�[�^�T�C�Y
const unsigned int maxSize(65507);

// ��M�̃^�C���A�E�g (10�b)
const unsigned long timeout(10000UL);

// �G���[�R�[�h�̕\��
int Network::getError() const
{
  const int err(WSAGetLastError());

#ifdef _DEBUG
  switch (err)
  {
  case WSAEACCES: std::cerr << "WSAEACCES\n"; break;
  case WSAEADDRINUSE: std::cerr << "WSAEADDRINUSE\n"; break;
  case WSAEADDRNOTAVAIL: std::cerr << "WSAEADDRNOTAVAIL\n"; break;
  case WSAEAFNOSUPPORT: std::cerr << "WSAEAFNOSUPPORT\n"; break;
  case WSAECONNABORTED: std::cerr << "WSAECONNABORTED\n"; break;
  case WSAECONNRESET: std::cerr << "WSAECONNRESET\n"; break;
  case WSAEDESTADDRREQ: std::cerr << "WSAEDESTADDRREQ\n"; break;
  case WSAEFAULT: std::cerr << "WSAEFAULT\n"; break;
  case WSAEHOSTUNREACH: std::cerr << "WSAEHOSTUNREACH\n"; break;
  case WSAEINPROGRESS: std::cerr << "WSAEINPROGRESS\n"; break;
  case WSAEINTR: std::cerr << "WSAEINTR\n"; break;
  case WSAEINVAL: std::cerr << "WSAEINVAL\n"; break;
  case WSAEINVALIDPROCTABLE: std::cerr << "WSAEINVALIDPROCTABLE\n"; break;
  case WSAEINVALIDPROVIDER: std::cerr << "WSAEINVALIDPROVIDER\n"; break;
  case WSAEISCONN: std::cerr << "WSAEISCONN\n"; break;
  case WSAEMFILE: std::cerr << "WSAEMFILE\n"; break;
  case WSAEMSGSIZE: std::cerr << "WSAEMSGSIZE\n"; break;
  case WSAENETDOWN: std::cerr << "WSAENETDOWN\n"; break;
  case WSAENETRESET: std::cerr << "WSAENETRESET\n"; break;
  case WSAENETUNREACH: std::cerr << "WSAENETUNREACH\n"; break;
  case WSAENOBUFS: std::cerr << "WSAENOBUFS\n"; break;
  case WSAENOTCONN: std::cerr << "WSAENOTCONN\n"; break;
  case WSAENOTSOCK: std::cerr << "WSAENOTSOCK\n"; break;
  case WSAEOPNOTSUPP: std::cerr << "WSAEOPNOTSUPP\n"; break;
  case WSAEPROTONOSUPPORT: std::cerr << "WSAEPROTONOSUPPORT\n"; break;
  case WSAEPROTOTYPE: std::cerr << "WSAEPROTOTYPE\n"; break;
  case WSAEPROVIDERFAILEDINIT: std::cerr << "WSAEPROVIDERFAILEDINIT\n"; break;
  case WSAESHUTDOWN: std::cerr << "WSAESHUTDOWN\n"; break;
  case WSAESOCKTNOSUPPORT: std::cerr << "WSAESOCKTNOSUPPORT\n"; break;
  case WSAETIMEDOUT: std::cerr << "WSAETIMEDOUT\n"; break;
  case WSAEWOULDBLOCK: std::cerr << "WSAEWOULDBLOCK\n"; break;
  case WSANOTINITIALISED: std::cerr << "WSANOTINITIALISED\n"; break;
  default: break;
  }
#endif

  return err;
}

// �l�b�g���[�N�ݒ�̏�����
int Network::initialize(int role, unsigned short port, const char *address)
{
  // role == 0 �Ȃ� STANDALONE
  if (role == 0) return 0;

  // ���M�p UDP �\�P�b�g�̍쐬
  sendSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sendSock == INVALID_SOCKET)
  {
    const int ret(getError());
    NOTIFY("���M���\�P�b�g���쐬�ł��܂���B");
    return ret;
  }

  // ��M�p UDP �\�P�b�g�̍쐬
  recvSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (recvSock == INVALID_SOCKET)
  {
    const int ret(getError());
    NOTIFY("��M���\�P�b�g���쐬�ł��܂���B");
    return ret;
  }

  // �z�X�g�̃A�h���X�̏����ݒ�
  memset(sendAddr.sin_zero, 0, sizeof sendAddr.sin_zero);
  memset(recvAddr.sin_zero, 0, sizeof recvAddr.sin_zero);
  sendAddr.sin_family = recvAddr.sin_family = AF_INET;

  // �z�X�g�� IP �A�h���X
  if (inet_pton(AF_INET, address, &sendAddr.sin_addr.s_addr) <= 0)
  {
    const int ret(getError());
    NOTIFY("IP �A�h���X���s���ł��B");
    return ret;
  }

  if (role == 1)
  {
    // ���c�҂Ƃ��ē��삷��
    sendAddr.sin_port = htons(port + 1);
    recvAddr.sin_port = htons(port);
  }
  else if (role == 2)
  {
    // ��Ǝ҂Ƃ��ē��삷��
    sendAddr.sin_port = htons(port);
    recvAddr.sin_port = htons(port + 1);
    sender = true;
  }

  // ��M�͂��ׂẴA�h���X���󂯓����
  recvAddr.sin_addr.s_addr = INADDR_ANY;

  // �\�P�b�g�Ƀz�X�g�̃A�h���X���֘A�t����
  if (bind(recvSock, reinterpret_cast<sockaddr *>(&recvAddr), static_cast<int>(sizeof recvAddr)))
  {
    const int ret(getError());
    NOTIFY("BIND �Ɏ��s���܂����B");
    return ret;
  }

  // ��M�̃^�C���A�E�g��ݒ肷��
  if (setsockopt(recvSock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeout), sizeof timeout))
  {
    const int ret(getError());
    NOTIFY("�^�C���A�E�g���ݒ�ł��܂���B");
    return ret;
  }

  return 0;
}

// �f�[�^���M
int Network::send(const void *buf, unsigned int len) const
{
  if (sendAddr.sin_addr.s_addr == INADDR_ANY) return 0;
  return sendto(sendSock, static_cast<const char *>(buf), len,
    0, reinterpret_cast<const sockaddr *>(&sendAddr), sizeof sendAddr);
}

// �f�[�^��M
int Network::recv(void *buf, unsigned int len)
{
  // �A�h���X�f�[�^�̒���
  sockaddr_in sendAddr;
  int fromlen(static_cast<int>(sizeof sendAddr));

  // �f�[�^����M����
  const int ret(recvfrom(recvSock, static_cast<char *>(buf), len, 0,
    reinterpret_cast<sockaddr *>(&sendAddr), &fromlen));

  return ret;
}

// �p�P�b�g�̃��C�A�E�g
struct Packet
{
  // �w�b�_�̃��C�A�E�g
  struct Header
  {
    // �V�[�P���X�ԍ�
    unsigned short sequence;

    // �c��̃p�P�b�g��
    unsigned short count;
  };

  // �p�P�b�g�w�b�_
  Header head;

  // �y�C���[�h
  char data[maxSize - sizeof Packet::head];
};

// 1 �t���[�����M
unsigned int Network::sendFrame(const void *buf, unsigned int len) const
{
  // �p�P�b�g
  Packet packet;

  // �V�[�P���X�ԍ��̏����l�� 0
  packet.head.sequence = 0;

  // �c��̃p�P�b�g���̓f�[�^�𑗂肫��̂ɕK�v�ȃy�C���[�h�̐�
  packet.head.count = (len - 1) / static_cast<unsigned short>(sizeof packet.data) + 1;

  // �����Ă��Ȃ��p�P�b�g�������
  while (packet.head.count > 0)
  {
    // ���M����f�[�^�̐擪
    const char *const ptr(static_cast<const char *>(buf) + packet.head.sequence * sizeof packet.data);

    // �p�P�b�g�T�C�Y
    const unsigned int size(len > sizeof packet.data ? sizeof packet.data : len);

    // �y�C���[�h�� 1 �p�P�b�g���f�[�^���R�s�[����
    memcpy(packet.data, ptr, size);

#ifdef _DEBUG
    // �V�[�P���X�ԍ��E�c��p�P�b�g���E�f�[�^�T�C�Y
    std::cerr << "seq:" << packet.head.sequence << ", cnt:" << packet.head.count << ", size:" << size << "\n";
#endif

    // 1 �p�P�b�g���f�[�^�𑗐M����
    const int ret(send(&packet, size + sizeof packet.head));

    // ���M�����f�[�^�T�C�Y���m���߂�
    if (ret <= 0)
    {
      // ���M���s
      if (ret == SOCKET_ERROR) getError();
      return ret;
    }

    // �c��̃f�[�^��
    len -= ret - sizeof packet.head;

    // �V�[�P���X�ԍ���i�߂�
    ++packet.head.sequence;

    // �c��̃p�P�b�g�������炷
    --packet.head.count;
  }

  // ����؂�Ȃ������f�[�^��
  return len;
}

// 1 �t���[����M
int Network::recvFrame(void *buf, unsigned int len)
{
  // �p�P�b�g
  Packet packet;

  // ��M����p�P�b�g���̏��
  const int limit((len - 1) / sizeof packet.data + 1);

  // ��M���ׂ��p�P�b�g��
  int total(-1);

  // ��M�����p�P�b�g��
  int count(0);

  // �Ō�̃p�P�b�g��������܂�
  do
  {
    // 1 �p�P�b�g���f�[�^����M����
    const int ret(recv(&packet, sizeof packet));

    // ��M�����f�[�^�T�C�Y���m���߂�
    if (ret <= 0)
    {
      // ��M���s
      if (ret == SOCKET_ERROR) getError();
      return ret;
    }

    // ��M�����p�P�b�g�����`�F�b�N
    if (++count > limit)
    {
      // �p�P�b�g������������
#ifdef _DEBUG
      std::cerr << "too many packets\n";
#endif
      return -1;
    }

    // �y�C���[�h�̃T�C�Y
    const int size(ret - sizeof packet.head);

    // �y�C���[�h�̃T�C�Y���`�F�b�N
    if (size < 0)
    {
      // �Z������p�P�b�g
#ifdef _DEBUG
      std::cerr << "too short\n";
#endif
      return -1;
    }

#ifdef _DEBUG
    // �V�[�P���X�ԍ��E�c��p�P�b�g���E�f�[�^�T�C�Y
    std::cerr << packet.head.sequence << ", " << packet.head.count << ", " << size << "\n";
#endif

    // �ۑ���̈ʒu
    const unsigned int pos(packet.head.sequence * sizeof packet.data);

    // �ۑ���Ɏ��܂邩�ǂ����`�F�b�N
    if (pos > len - size)
    {
      // �I�[�o�[�t���[
#ifdef _DEBUG
      std::cerr << "buffer overflow\n";
#endif
      return -1;
    }

    // �ۑ���̗̈�̐擪
    char *const ptr(static_cast<char *>(buf) + pos);

    // �y�C���[�h��ۑ�����
    memcpy(ptr, packet.data, size);

    // �擪�p�P�b�g���ێ�����p�P�b�g����ۑ�����
    if (packet.head.sequence == 0) total = packet.head.count;
  }
  while (packet.head.count > 1);

  // �擪�p�P�b�g����M���Ă��Ȃ�����M�����p�P�b�g������������Ύ��s
  if (count > total)
  {
    // ��M���s
#ifdef _DEBUG
    std::cerr << "broken packet\n";
#endif
    return -1;
  }

  // �󂯎�����p�P�b�g��
  return count;
}

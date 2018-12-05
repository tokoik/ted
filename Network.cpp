#include "Network.h"
#include "Window.h"
#if defined(_WIN32)
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
#endif
#include <iostream>

// �ő�f�[�^�T�C�Y
const unsigned int maxSize(65507);

// ��M�̃^�C���A�E�g (10�b)
const unsigned long timeout(10000UL);

// �R���X�g���N�^
Network::Network()
  : role(STANDALONE), recvSock(INVALID_SOCKET), sendSock(INVALID_SOCKET)
{
}

// �f�X�g���N�^
Network::~Network()
{
  finalize();
}

// �G���[�R�[�h�̕\��
int Network::getError() const
{
  const int err(WSAGetLastError());

#if DEBUG
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

// ��M���̏�����
int Network::initializeRecv(unsigned short port)
{
  // ��M�p UDP �\�P�b�g�̍쐬
  recvSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (recvSock == INVALID_SOCKET)
  {
    const int ret(getError());
    NOTIFY("��M���\�P�b�g���쐬�ł��܂���B");
    return ret;
  }

  // �A�h���X�t�@�~���[
  recvAddr.sin_family = AF_INET;

  // ��M���̃|�[�g
  recvAddr.sin_port = htons(port);

  // ��M���� IP �A�h���X
  recvAddr.sin_addr.s_addr = INADDR_ANY;

  // 0 �ɂ���Ƃ���
  memset(recvAddr.sin_zero, 0, sizeof recvAddr.sin_zero);

  // ��M���̃\�P�b�g�Ƀz�X�g�̃A�h���X���֘A�t����
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

// ���M���̏�����
int Network::initializeSend(unsigned short port, const char *address)
{
  // ���M�p UDP �\�P�b�g�̍쐬
  sendSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sendSock == INVALID_SOCKET)
  {
    const int ret(getError());
    NOTIFY("���M���\�P�b�g���쐬�ł��܂���B");
    return ret;
  }

  // �A�h���X�t�@�~���[
  sendAddr.sin_family = AF_INET;

  // ���M��̃|�[�g
  sendAddr.sin_port = htons(port);

  // ���M��� IP �A�h���X
  if (inet_pton(AF_INET, address, &sendAddr.sin_addr.s_addr) <= 0)
  {
    const int ret(getError());
    NOTIFY("���M��� IP �A�h���X���ݒ�ł��܂���B");
    return ret;
  }

  // 0 �ɂ���Ƃ���
  memset(sendAddr.sin_zero, 0, sizeof sendAddr.sin_zero);

  return 0;
}

// �l�b�g���[�N�ݒ�̏�����
int Network::initialize(int role, unsigned short port, const char *address)
{
  // ����
  this->role = static_cast<Role>(role);

  // role �� OPERATOR �ł� WORKER �ł��Ȃ���Ή������Ȃ�
  if (role != OPERATOR && role != WORKER) return 0;

  // ��M��������������
  const int ret(initializeRecv(port + role - 1));
  if (ret != 0) return ret;

  // ���M��������������
  return initializeSend(port + 2 - role, address);
}

// �I������
void Network::finalize()
{
  if (running())
  {
    if (recvSock != INVALID_SOCKET) closesocket(recvSock);
    if (sendSock != INVALID_SOCKET) closesocket(sendSock);
    sendSock = recvSock = INVALID_SOCKET;
  }
}

// ���s���Ȃ�^
bool Network::running() const
{
  return sendSock != INVALID_SOCKET;
}

// STANDALONE �Ȃ�^
bool Network::isStandalone() const
{
  return role == STANDALONE;
}

// OPERATOR �Ȃ�^
bool Network::isOperator() const
{
  return role == OPERATOR;
}

// WORKDER �Ȃ�^
bool Network::isWorker() const
{
  return role == WORKER;
}

// �����[�g�̃A�h���X
bool Network::checkRemote() const
{
#if DEBUG
  std::cerr << '['
    << static_cast<int>(recvAddr.sin_addr.S_un.S_un_b.s_b1) << '.'
    << static_cast<int>(recvAddr.sin_addr.S_un.S_un_b.s_b2) << '.'
    << static_cast<int>(recvAddr.sin_addr.S_un.S_un_b.s_b3) << '.'
    << static_cast<int>(recvAddr.sin_addr.S_un.S_un_b.s_b4) << "]\n";
#endif
  return recvAddr.sin_addr.s_addr == sendAddr.sin_addr.s_addr;
}

// 1 �p�P�b�g��M
int Network::recvPacket(void *buf, unsigned int len)
{
  // �A�h���X�f�[�^�̒���
  int fromlen(static_cast<int>(sizeof recvAddr));

  // �f�[�^����M����
  const int ret(recvfrom(recvSock, static_cast<char *>(buf), len, 0,
    reinterpret_cast<sockaddr *>(&recvAddr), &fromlen));

  return ret;
}

// 1 �p�P�b�g���M
int Network::sendPacket(const void *buf, unsigned int len) const
{
  return sendto(sendSock, static_cast<const char *>(buf), len,
    0, reinterpret_cast<const sockaddr *>(&sendAddr), sizeof sendAddr);
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

// 1 �t���[����M
int Network::recvData(void *buf, unsigned int len)
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
    const int ret(recvPacket(&packet, sizeof packet));

    // ���� 0 �̃p�P�b�g���󂯎������I���
    if (ret == 0) return 0;

    // �߂�l�����Ȃ�G���[
    if (ret < 0)
    {
      // ��M���s
      if (ret == SOCKET_ERROR) getError();
      return ret;
    }

    // ��M�����p�P�b�g�����`�F�b�N
    if (++count > limit)
    {
      // �p�P�b�g������������
#if DEBUG
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
#if DEBUG
      std::cerr << "too short\n";
#endif
      return -1;
    }

#if DEBUG
    // �V�[�P���X�ԍ��E�c��p�P�b�g���E�f�[�^�T�C�Y
    std::cerr << "recvPacket seq:" << packet.head.sequence << ", cnt:" << packet.head.count << ", size:" << size << "\n";
#endif

    // �ۑ���̈ʒu
    const unsigned int pos(packet.head.sequence * sizeof packet.data);

    // �ۑ���Ɏ��܂邩�ǂ����`�F�b�N
    if (pos > len - size)
    {
      // �I�[�o�[�t���[
#if DEBUG
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
  } while (packet.head.count > 1);

  // �擪�p�P�b�g����M���Ă��Ȃ�����M�����p�P�b�g������������Ύ��s
  if (count > total)
  {
    // ��M���s
#if DEBUG
    std::cerr << "broken packet\n";
#endif
    return -1;
  }

  // �󂯎�����p�P�b�g��
  return count;
}

// 1 �t���[�����M
unsigned int Network::sendData(const void *buf, unsigned int len) const
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

#if DEBUG
    // �V�[�P���X�ԍ��E�c��p�P�b�g���E�f�[�^�T�C�Y
    std::cerr << "sendPacket seq:" << packet.head.sequence << ", cnt:" << packet.head.count << ", size:" << size << "\n";
#endif

    // 1 �p�P�b�g���f�[�^�𑗐M����
    const int ret(sendPacket(&packet, size + sizeof packet.head));

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

// EOF ���M
int Network::sendEof() const
{
  char c;
  return sendPacket(&c, 0);
}

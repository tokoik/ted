#pragma once

#if defined(_WIN32)
#  include <winsock2.h>
#endif

class Network
{
  // ソケット
  SOCKET sendSock, recvSock;
  sockaddr_in sendAddr, recvAddr;

  // true なら送信側
  bool sender;

public:

  // エラーコードの表示
  int getError() const;

  // コンストラクタ
  Network()
    : sendSock(INVALID_SOCKET), recvSock(INVALID_SOCKET), sender(false)
  {
  }

  // コンストラクタ (role == 0 : STANDALONE, 1: OPERATOR, 2: WORKER)
  Network(int role, unsigned short port, const char *address)
    : Network()
  {
    initialize(role, port, address);
  }

  // デストラクタ
  ~Network()
  {
    finalize();
  }

  // 初期化（アドレスを指定していなければ操縦者，していれば作業者）
  int initialize(int role, unsigned short port, const char *address = nullptr);

  // 終了処理
  void finalize()
  {
    if (running())
    {
      if (sendSock != INVALID_SOCKET) closesocket(sendSock);
      if (recvSock != INVALID_SOCKET) closesocket(recvSock);
      sendSock = recvSock = INVALID_SOCKET;
    }
  }

  // 実行中なら真
  bool running() const
  {
    return sendSock != INVALID_SOCKET;
  }

  // 送信側なら真
  bool isSender() const
  {
    return sender;
  }

  // データ送信
  int send(const void *buf, unsigned int len) const;

  // データ受信
  int recv(void *buf, unsigned int len);

  // 1 フレーム送信
  int sendFrame(const void *buf, unsigned int len) const;

  // 1 フレーム受信
  int recvFrame(void *buf, unsigned int len);
};

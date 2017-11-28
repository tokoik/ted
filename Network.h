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

  // コンストラクタ
  Network();

  // コンストラクタ (role == 0 : STANDALONE, 1: OPERATOR, 2: WORKER)
  Network(int role, unsigned short port, const char *address);

  // デストラクタ
  virtual ~Network();

  // エラーコードの表示
  int getError() const;

  // 初期化（アドレスを指定していなければ操縦者，していれば作業者）
  int initialize(int role, unsigned short port, const char *address = nullptr);

  // 終了処理
  void finalize();

  // 実行中なら真
  bool running() const;

  // 送信側なら真
  bool isSender() const;

  // データ送信
  int send(const void *buf, unsigned int len) const;

  // データ受信
  int recv(void *buf, unsigned int len);

  // 1 フレーム送信
  unsigned int sendFrame(const void *buf, unsigned int len) const;

  // 1 フレーム受信
  int recvFrame(void *buf, unsigned int len);
};

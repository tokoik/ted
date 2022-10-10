#pragma once

//
// ネットワーク関連の処理
//

#if defined(_WIN32)
#  include <winsock2.h>
#endif
#include <cstddef>

class Network
{
  // ソケット
  SOCKET recvSock, sendSock;
  sockaddr_in recvAddr, sendAddr;

  // 役割
  enum class Role : int { STANDALONE = 0, OPERATOR, WORKER } role;

public:

  // コンストラクタ
  Network();

  // デストラクタ
  virtual ~Network();

  // エラーコードの表示
  int getError() const;

  // 初期化 (role == 0 : STANDALONE, 1: OPERATOR, 2: WORKER)
  int initializeRecv(unsigned short port);
  int initializeSend(unsigned short port, const char* address);
  int initialize(int role, unsigned short port, const char* address);

  // 終了処理
  void finalize();

  // 実行中なら真
  bool running() const;

  // STANDALONE なら真
  bool isStandalone() const;

  // 指導者なら真
  bool isInstructor() const;

  // 作業者なら真
  bool isWorker() const;

  // リモートのアドレスのチェック
  bool checkRemote() const;

  // 1 パケット受信
  int recvPacket(void* buf, int len);

  // 1 パケット送信
  int sendPacket(const void* buf, int len) const;

  // 1 フレーム受信
  int recvData(void* buf, int len);

  // 1 フレーム送信
  unsigned int sendData(const void* buf, int len) const;

  // EOF 送信
  int sendEof() const;
};

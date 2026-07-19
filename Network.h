#pragma once

//
// ネットワーク関連の処理
//

#if defined(_WIN32)
#  include <winsock2.h>
#endif

// TED同士で姿勢と画像を交換するためのUDP通信を管理する。
// 大きなフレームは独自ヘッダ付きの複数データグラムへ分割し、受信側で順序を復元する。
class Network
{
  // 受信元の検証と送信先の指定を分離するため、送受信ソケットとアドレスを別々に保持する
  SOCKET recvSock{ INVALID_SOCKET };
  SOCKET sendSock{ INVALID_SOCKET };
  sockaddr_in recvAddr{};
  sockaddr_in sendAddr{};

  // 役割
  enum class Role : int { STANDALONE = 0, OPERATOR, WORKER } role{ Role::STANDALONE };

public:

  // コンストラクタ
  Network() = default;

  // デストラクタ
  virtual ~Network();

  // エラーコードの表示
  int getError() const;

  // 役割に応じて向かい合う送受信ポートを選ぶ。
  // role == 0 : STANDALONE, 1: OPERATOR, 2: WORKER
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

  // 最後に受信したデータグラムが設定済みの通信相手から届いたものか確認する
  bool checkRemote() const;

  // 1 パケット受信
  int recvPacket(void* buf, int len);

  // 1 パケット送信
  int sendPacket(const void* buf, int len) const;

  // 分割データグラムを連番位置へ復元する。範囲外・重複・別送信元を検査し、
  // 成功時は復元したバイト数、EOFは0、失敗時は負値を返す。
  int recvData(void* buf, int len);

  // 1フレームをUDPの最大ペイロード以下に分割し、復元用の残パケット数を付けて送る
  unsigned int sendData(const void* buf, int len) const;

  // 相手側の受信スレッドを終了させるため、長さ0のデータグラムを送る
  int sendEof() const;
};

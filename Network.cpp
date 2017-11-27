#include "Window.h"
#include "Network.h"
#if defined(_WIN32)
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
#endif
#include <iostream>

// 最大データサイズ
const unsigned int maxSize(65507);

// 受信のタイムアウト (10秒)
const unsigned long timeout(10000UL);

// エラーコードの表示
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

// ネットワーク設定の初期化
int Network::initialize(int role, unsigned short port, const char *address)
{
  // role == 0 なら STANDALONE
  if (role == 0) return 0;

  // 送信用 UDP ソケットの作成
  sendSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sendSock == INVALID_SOCKET)
  {
    const int ret(getError());
    NOTIFY("送信側ソケットが作成できません。");
    return ret;
  }

  // 受信用 UDP ソケットの作成
  recvSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (recvSock == INVALID_SOCKET)
  {
    const int ret(getError());
    NOTIFY("受信側ソケットが作成できません。");
    return ret;
  }

  // ホストのアドレスの初期設定
  memset(sendAddr.sin_zero, 0, sizeof sendAddr.sin_zero);
  memset(recvAddr.sin_zero, 0, sizeof recvAddr.sin_zero);
  sendAddr.sin_family = recvAddr.sin_family = AF_INET;

  // ホストの IP アドレス
  if (inet_pton(AF_INET, address, &sendAddr.sin_addr.s_addr) <= 0)
  {
    const int ret(getError());
    NOTIFY("IP アドレスが不正です。");
    return ret;
  }

  if (role == 1)
  {
    // 操縦者として動作する
    sendAddr.sin_port = htons(port + 1);
    recvAddr.sin_port = htons(port);
  }
  else if (role == 2)
  {
    // 作業者として動作する
    sendAddr.sin_port = htons(port);
    recvAddr.sin_port = htons(port + 1);
    sender = true;
  }

  // 受信はすべてのアドレスを受け入れる
  recvAddr.sin_addr.s_addr = INADDR_ANY;

  // ソケットにホストのアドレスを関連付ける
  if (bind(recvSock, reinterpret_cast<sockaddr *>(&recvAddr), static_cast<int>(sizeof recvAddr)))
  {
    const int ret(getError());
    NOTIFY("BIND に失敗しました。");
    return ret;
  }

  // 受信のタイムアウトを設定する
  if (setsockopt(recvSock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char *>(&timeout), sizeof timeout))
  {
    const int ret(getError());
    NOTIFY("タイムアウトが設定できません。");
    return ret;
  }

  return 0;
}

// データ送信
int Network::send(const void *buf, unsigned int len) const
{
  if (sendAddr.sin_addr.s_addr == INADDR_ANY) return 0;
  return sendto(sendSock, static_cast<const char *>(buf), len,
    0, reinterpret_cast<const sockaddr *>(&sendAddr), sizeof sendAddr);
}

// データ受信
int Network::recv(void *buf, unsigned int len)
{
  // アドレスデータの長さ
  sockaddr_in sendAddr;
  int fromlen(static_cast<int>(sizeof sendAddr));

  // データを受信する
  const int ret(recvfrom(recvSock, static_cast<char *>(buf), len, 0,
    reinterpret_cast<sockaddr *>(&sendAddr), &fromlen));

  return ret;
}

// パケットのレイアウト
struct Packet
{
  // ヘッダのレイアウト
  struct Header
  {
    // シーケンス番号
    unsigned short sequence;

    // 残りのパケット数
    unsigned short count;
  };

  // パケットヘッダ
  Header head;

  // ペイロード
  char data[maxSize - sizeof Packet::head];
};

// 1 フレーム送信
unsigned int Network::sendFrame(const void *buf, unsigned int len) const
{
  // パケット
  Packet packet;

  // シーケンス番号の初期値は 0
  packet.head.sequence = 0;

  // 残りのパケット数はデータを送りきるのに必要なペイロードの数
  packet.head.count = (len - 1) / static_cast<unsigned short>(sizeof packet.data) + 1;

  // 送っていないパケットがある間
  while (packet.head.count > 0)
  {
    // 送信するデータの先頭
    const char *const ptr(static_cast<const char *>(buf) + packet.head.sequence * sizeof packet.data);

    // パケットサイズ
    const unsigned int size(len > sizeof packet.data ? sizeof packet.data : len);

    // ペイロードに 1 パケット分データをコピーする
    memcpy(packet.data, ptr, size);

#ifdef _DEBUG
    // シーケンス番号・残りパケット数・データサイズ
    std::cerr << "seq:" << packet.head.sequence << ", cnt:" << packet.head.count << ", size:" << size << "\n";
#endif

    // 1 パケット分データを送信する
    const int ret(send(&packet, size + sizeof packet.head));

    // 送信したデータサイズを確かめる
    if (ret <= 0)
    {
      // 送信失敗
      if (ret == SOCKET_ERROR) getError();
      return ret;
    }

    // 残りのデータ量
    len -= ret - sizeof packet.head;

    // シーケンス番号を進める
    ++packet.head.sequence;

    // 残りのパケット数を減らす
    --packet.head.count;
  }

  // 送り切れなかったデータ量
  return len;
}

// 1 フレーム受信
int Network::recvFrame(void *buf, unsigned int len)
{
  // パケット
  Packet packet;

  // 受信するパケット数の上限
  const int limit((len - 1) / sizeof packet.data + 1);

  // 受信すべきパケット数
  int total(-1);

  // 受信したパケット数
  int count(0);

  // 最後のパケットを見つけるまで
  do
  {
    // 1 パケット分データを受信する
    const int ret(recv(&packet, sizeof packet));

    // 受信したデータサイズを確かめる
    if (ret <= 0)
    {
      // 受信失敗
      if (ret == SOCKET_ERROR) getError();
      return ret;
    }

    // 受信したパケット数をチェック
    if (++count > limit)
    {
      // パケット数が多すぎる
#ifdef _DEBUG
      std::cerr << "too many packets\n";
#endif
      return -1;
    }

    // ペイロードのサイズ
    const int size(ret - sizeof packet.head);

    // ペイロードのサイズをチェック
    if (size < 0)
    {
      // 短すぎるパケット
#ifdef _DEBUG
      std::cerr << "too short\n";
#endif
      return -1;
    }

#ifdef _DEBUG
    // シーケンス番号・残りパケット数・データサイズ
    std::cerr << packet.head.sequence << ", " << packet.head.count << ", " << size << "\n";
#endif

    // 保存先の位置
    const unsigned int pos(packet.head.sequence * sizeof packet.data);

    // 保存先に収まるかどうかチェック
    if (pos > len - size)
    {
      // オーバーフロー
#ifdef _DEBUG
      std::cerr << "buffer overflow\n";
#endif
      return -1;
    }

    // 保存先の領域の先頭
    char *const ptr(static_cast<char *>(buf) + pos);

    // ペイロードを保存する
    memcpy(ptr, packet.data, size);

    // 先頭パケットが保持するパケット数を保存する
    if (packet.head.sequence == 0) total = packet.head.count;
  }
  while (packet.head.count > 1);

  // 先頭パケットを受信していないか受信したパケット数が多すぎれば失敗
  if (count > total)
  {
    // 受信失敗
#ifdef _DEBUG
    std::cerr << "broken packet\n";
#endif
    return -1;
  }

  // 受け取ったパケット数
  return count;
}

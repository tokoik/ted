//
// ネットワーク関連の処理
//
#include "Network.h"

// Winsock 2 
#if defined(_WIN32)
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
#endif

// ウィンドウ関連の処理
#include "Window.h"

// 最大データサイズ
const unsigned int maxSize(65507);

// 受信のタイムアウト (10秒)
const unsigned long timeout(10000UL);

// コンストラクタ
Network::Network()
  : role(STANDALONE), recvSock(INVALID_SOCKET), sendSock(INVALID_SOCKET)
{
}

// デストラクタ
Network::~Network()
{
  finalize();
}

// エラーコードの表示
int Network::getError() const
{
  const int err(WSAGetLastError());

#if defined(DEBUG)
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

// 受信側の初期化
int Network::initializeRecv(unsigned short port)
{
  // 受信用 UDP ソケットの作成
  recvSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (recvSock == INVALID_SOCKET)
  {
    const int ret(getError());
    NOTIFY("受信側ソケットが作成できません。");
    return ret;
  }

  // アドレスファミリー
  recvAddr.sin_family = AF_INET;

  // 受信元のポート
  recvAddr.sin_port = htons(port);

  // 受信元の IP アドレス
  recvAddr.sin_addr.s_addr = INADDR_ANY;

  // 0 にするところ
  memset(recvAddr.sin_zero, 0, sizeof recvAddr.sin_zero);

  // 受信側のソケットにホストのアドレスを関連付ける
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

// 送信側の初期化
int Network::initializeSend(unsigned short port, const char *address)
{
  // 送信用 UDP ソケットの作成
  sendSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sendSock == INVALID_SOCKET)
  {
    const int ret(getError());
    NOTIFY("送信側ソケットが作成できません。");
    return ret;
  }

  // アドレスファミリー
  sendAddr.sin_family = AF_INET;

  // 送信先のポート
  sendAddr.sin_port = htons(port);

  // 送信先の IP アドレス
  if (inet_pton(AF_INET, address, &sendAddr.sin_addr.s_addr) <= 0)
  {
    const int ret(getError());
    NOTIFY("送信先の IP アドレスが設定できません。");
    return ret;
  }

  // 0 にするところ
  memset(sendAddr.sin_zero, 0, sizeof sendAddr.sin_zero);

  return 0;
}

// ネットワーク設定の初期化
int Network::initialize(int role, unsigned short port, const char *address)
{
  // 役割
  this->role = static_cast<Role>(role);

  // role が OPERATOR でも WORKER でもなければ何もしない
  if (role != OPERATOR && role != WORKER) return 0;

  // 受信側を初期化する
  const int ret(initializeRecv(port + role - 1));
  if (ret != 0) return ret;

  // 送信側を初期化する
  return initializeSend(port + 2 - role, address);
}

// 終了処理
void Network::finalize()
{
  if (running())
  {
    if (recvSock != INVALID_SOCKET) closesocket(recvSock);
    if (sendSock != INVALID_SOCKET) closesocket(sendSock);
    sendSock = recvSock = INVALID_SOCKET;
  }
}

// 実行中なら真
bool Network::running() const
{
  return sendSock != INVALID_SOCKET;
}

// STANDALONE なら真
bool Network::isStandalone() const
{
  return role == STANDALONE;
}

// OPERATOR なら真
bool Network::isInstructor() const
{
  return role == OPERATOR;
}

// WORKDER なら真
bool Network::isWorker() const
{
  return role == WORKER;
}

// リモートのアドレス
bool Network::checkRemote() const
{
#if defined(DEBUG)
  std::cerr << '['
    << static_cast<int>(recvAddr.sin_addr.S_un.S_un_b.s_b1) << '.'
    << static_cast<int>(recvAddr.sin_addr.S_un.S_un_b.s_b2) << '.'
    << static_cast<int>(recvAddr.sin_addr.S_un.S_un_b.s_b3) << '.'
    << static_cast<int>(recvAddr.sin_addr.S_un.S_un_b.s_b4) << "]\n";
#endif
  return recvAddr.sin_addr.s_addr == sendAddr.sin_addr.s_addr;
}

// 1 パケット受信
int Network::recvPacket(void *buf, unsigned int len)
{
  // アドレスデータの長さ
  int fromlen(static_cast<int>(sizeof recvAddr));

  // データを受信する
  const int ret(recvfrom(recvSock, static_cast<char *>(buf), len, 0,
    reinterpret_cast<sockaddr *>(&recvAddr), &fromlen));

  return ret;
}

// 1 パケット送信
int Network::sendPacket(const void *buf, unsigned int len) const
{
  return sendto(sendSock, static_cast<const char *>(buf), len,
    0, reinterpret_cast<const sockaddr *>(&sendAddr), sizeof sendAddr);
}

// パケットのレイアウト
struct Packet
{
  // 残りのパケット数
  int count;

  // ペイロード
  char data[maxSize - sizeof Packet::count];
};

// 1 フレーム受信
int Network::recvData(void *buf, unsigned int len)
{
  // パケット
  Packet packet;

  // 受信するパケット数の上限
  const int limit((len - 1) / sizeof packet.data + 1);

  // 受信すべきパケット数
  int total(-1);

  // 受信したデータサイズ
  int bytes;

  // 全部のパケットを受信するまで
  for (int count = 0, drop = 0;;)
  {
    // 1 パケット分データを受信する
    bytes = recvPacket(&packet, sizeof packet);

    // 長さ 0 のパケットを受け取ったら終わり
    if (bytes == 0) return 0;

    // 戻り値が負ならエラー
    if (bytes < 0)
    {
      // 受信失敗
      if (bytes == SOCKET_ERROR) getError();
      return bytes;
    }

    // 先頭のパケットだったら
    if (packet.count < 0)
    {
      // 受信したデータを捨てて最初からやり直す
      count = 0;

      // 先頭パケットが保持するパケット数を保存する
      total = packet.count = -packet.count;
    }

    // total が負のままだったらまだ先頭パケットを受信していないので
    if (total < 0)
    {
      // 破棄するパケット数をカウントして
      if (++drop < maxDropPackets)
      {
        // 規定数に達していなければ破棄
        continue;
      }
      else
      {
        // 規定数に達していればエラー
        return -1;
      }
    }

    // 受信したパケット数をチェック
    if (++count > limit)
    {
      // パケット数が多すぎる
#if defined(DEBUG)
      std::cerr << "too many packets\n";
#endif
      return -1;
    }

    // ペイロードのサイズ
    const int size(bytes - sizeof packet.count);

    // ペイロードのサイズをチェック
    if (size < 0)
    {
      // 短すぎるパケット
#if defined(DEBUG)
      std::cerr << "too short\n";
#endif
      return -1;
    }

#if defined(DEBUG)
    // シーケンス番号・データサイズ
    std::cerr << "recv seq:" << total - packet.count << ", size:" << size << "\n";
#endif

    // 保存先の位置
    const unsigned int pos((total - packet.count) * sizeof packet.data);

    // 保存先に収まるかどうかチェック
    if (pos > len - size)
    {
      // オーバーフロー
#if defined(DEBUG)
      std::cerr << "buffer overflow\n";
#endif
      return -1;
    }

    // 保存先の領域の先頭
    char *const ptr(static_cast<char *>(buf) + pos);

    // ペイロードを保存する
    memcpy(ptr, packet.data, size);

    // 全部のパケットを受け取っていれば終わる
    if (count >= total) break;
  }

  // 受け取ったパケット数
  return total;
}

// 1 フレーム送信
unsigned int Network::sendData(const void *buf, unsigned int len) const
{
  // パケット
  Packet packet;

  // 残りのパケット数はデータを送りきるのに必要なペイロードの数
  int total((len - 1) / static_cast<unsigned short>(sizeof packet.data) + 1);

  // 最初のパケットでは残りのパケット数の符号を反転する
  packet.count = -total;

  // 送っていないパケットがある間
  for (int count = 0; total > 0; ++count)
  {
    // 送信するデータの先頭
    const char *const ptr(static_cast<const char *>(buf) + count * sizeof packet.data);

    // パケットサイズ
    const unsigned int size(len > sizeof packet.data ? sizeof packet.data : len);

    // ペイロードに 1 パケット分データをコピーする
    memcpy(packet.data, ptr, size);

#if defined(DEBUG)
    // シーケンス番号・データサイズ
    std::cerr << "send seq:" << count << ", size:" << size << "\n";
#endif

    // 1 パケット分データを送信する
    const int ret(sendPacket(&packet, size + sizeof packet.count));

    // 送信したデータサイズを確かめる
    if (ret <= 0)
    {
      // 送信失敗
      if (ret == SOCKET_ERROR) getError();
      return ret;
    }

    // 残りのデータ量
    len -= ret - sizeof packet.count;

    // 残りのパケット数を減らす
    packet.count = --total;
  }

  // 送り切れなかったデータ量
  return len;
}

// EOF 送信
int Network::sendEof() const
{
  char c;
  return sendPacket(&c, 0);
}

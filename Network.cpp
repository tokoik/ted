///
/// ネットワーク関連の処理クラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///
#include "Network.h"

// ウィンドウ関連の処理
#include "GgApp.h"

// Winsock 2 
#if defined(_WIN32)
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
#endif

// 標準ライブラリ
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <climits>

// 最大データサイズ
const std::size_t maxSize{ 1470 };

// 受信のタイムアウト (500ミリ秒)
const unsigned long timeout{ 500UL };

//
// デストラクタ
//
Network::~Network()
{
  finalize();
}

//
// エラーコードの表示
//
int Network::getError() const
{
  const int err(WSAGetLastError());

#if defined(_DEBUG)
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

//
// 受信側の初期化
//
int Network::initializeRecv(unsigned short port)
{
  // 受信用 UDP ソケットの作成
  recvSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (recvSock == INVALID_SOCKET)
  {
    const int ret{ getError() };
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
  if (bind(recvSock, reinterpret_cast<sockaddr*>(&recvAddr), static_cast<int>(sizeof recvAddr)))
  {
    const int ret{ getError() };
    NOTIFY("BIND に失敗しました。");
    return ret;
  }

  // 受信のタイムアウトを設定する
  if (setsockopt(recvSock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof timeout))
  {
    const int ret{ getError() };
    NOTIFY("タイムアウトが設定できません。");
    return ret;
  }

  return 0;
}

//
// 送信側の初期化
//
int Network::initializeSend(unsigned short port, const char* address)
{
  // 送信用 UDP ソケットの作成
  sendSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sendSock == INVALID_SOCKET)
  {
    const int ret{ getError() };
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
    const int ret{ getError() };
    NOTIFY("送信先の IP アドレスが設定できません。");
    return ret;
  }

  // 0 にするところ
  memset(sendAddr.sin_zero, 0, sizeof sendAddr.sin_zero);

  return 0;
}

//
// ネットワーク設定の初期化
//
int Network::initialize(int role, unsigned short port, const char* address)
{
  // 再初期化時や途中で失敗した初期化のソケットを残さない
  finalize();

  // 役割
  this->role = static_cast<Role>(role);

  // role が OPERATOR でも WORKER でもなければ何もしない
  if (this->role != Role::OPERATOR && this->role != Role::WORKER) return 0;

  // 受信側を初期化する
  const int ret{ initializeRecv(port + role - 1) };
  if (ret != 0)
  {
    finalize();
    return ret;
  }

  // 送信側を初期化し、失敗時は先に作成した受信ソケットも閉じる
  const int sendResult{ initializeSend(port + 2 - role, address) };
  if (sendResult != 0) finalize();
  return sendResult;
}

//
// 終了処理
//
void Network::finalize()
{
  if (recvSock != INVALID_SOCKET) closesocket(recvSock);
  if (sendSock != INVALID_SOCKET) closesocket(sendSock);
  sendSock = recvSock = INVALID_SOCKET;
  lastFrameIdValid = false;
}

//
// 実行中かどうか調べる
//
bool Network::running() const
{
  return sendSock != INVALID_SOCKET;
}

//
// STANDALONE かどうか調べる
//
bool Network::isStandalone() const
{
  return role == Role::STANDALONE;
}

//
// OPERATOR かどうか調べる
//
bool Network::isInstructor() const
{
  return role == Role::OPERATOR;
}

//
// WORKER かどうか調べる
//
bool Network::isWorker() const
{
  return role == Role::WORKER;
}

//
// リモートのアドレスを確認する
//
bool Network::checkRemote() const
{
#if defined(_DEBUG)
  std::cerr << '['
    << static_cast<int>(recvAddr.sin_addr.S_un.S_un_b.s_b1) << '.'
    << static_cast<int>(recvAddr.sin_addr.S_un.S_un_b.s_b2) << '.'
    << static_cast<int>(recvAddr.sin_addr.S_un.S_un_b.s_b3) << '.'
    << static_cast<int>(recvAddr.sin_addr.S_un.S_un_b.s_b4) << "]\n";
#endif
  return recvAddr.sin_addr.s_addr == sendAddr.sin_addr.s_addr;
}

//
// 1 パケット受信
//
int Network::recvPacket(void* buf, int len)
{
  // アドレスデータの長さ
  int fromlen{ static_cast<int>(sizeof recvAddr) };

  return recvfrom(recvSock, static_cast<char*>(buf), len, 0,
    reinterpret_cast<sockaddr*>(&recvAddr), &fromlen);
}

//
// 1 パケット送信
//
int Network::sendPacket(const void* buf, int len) const
{
  return sendto(sendSock, static_cast<const char*>(buf), len,
    0, reinterpret_cast<const sockaddr*>(&sendAddr), sizeof sendAddr);
}

//
// 1 データグラムのレイアウト
//
// countはフレーム先頭だけ負の総パケット数、以降は正の残パケット数とする。
// 受信側は total - count を格納位置として使うため、UDPで順序が入れ替わっても復元できる。
// 受信側は count が負のパケットをフレーム境界として、以前の未完成フレームを破棄する。
//
struct Packet
{
  // フレーム番号 (シリアル番号)
  unsigned short frameId;

  // 残りのパケット数
  int count;

  // ペイロード
  char data[maxSize - sizeof(unsigned short) - sizeof(int)];
};

//
// 1 フレーム受信
//
int Network::recvData(void* buf, int len)
{
  // UDPの内容は欠損・重複・偽装があり得るため、呼び出し側バッファへ書く前に
  // 送信元、総数、連番、ペイロード長をすべて検査する。
  if (!buf || len <= 0) return -1;

  // パケット
  Packet packet;

  // 受信するパケット数の上限
  const int limit{ (len - 1) / static_cast<int>(sizeof packet.data) + 1 };

  // 受信すべきパケット数
  int total{ -1 };

  // 現在受信中のフレームID
  unsigned short currentFrameId{ 0 };

  // 受信済みのシーケンス番号と、復元したフレームのバイト数
  std::vector<bool> received;
  int receivedBytes{ 0 };

  // 前回の受信完了から2.0秒以上経過していたら再同期（履歴を無効化）する
  if (lastFrameIdValid)
  {
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRecvTime).count();
    if (elapsed > 2000) // 2秒
    {
      lastFrameIdValid = false;
    }
  }

  // 全部のパケットを受信するまで
  for (int count = 0, drop = 0;;)
  {
    // 1 パケット分データを受信する
    const int bytes{ recvPacket(&packet, sizeof packet) };

    // 長さ 0 のパケットを受け取ったら終わり
    if (bytes == 0) return 0;

    // 戻り値が負ならエラー
    if (bytes < 0)
    {
      // 受信失敗
      if (bytes == SOCKET_ERROR) getError();
      return bytes;
    }

    // 設定した相手以外から届いたパケットはフレームに混ぜない
    if (!checkRemote()) continue;

    // ヘッダフィールド（frameId, count）を含まないパケットはプロトコル違反
    if (bytes < static_cast<int>(sizeof packet.frameId + sizeof packet.count)) return -1;

    // 負のcountを持つパケットをフレーム境界として、以前の未完成フレームを破棄する
    if (packet.count < 0)
    {
      // すでに何らかのパケットを受信し始めている場合、
      // 新しいパケットのframeIdが現在受信中のものと同じか古ければ破棄する（周回を考慮した16bit比較）
      if (total >= 0 && static_cast<int16_t>(packet.frameId - currentFrameId) <= 0)
      {
        continue;
      }

      // 前回の呼び出しで完了/受信開始したフレームIDよりも古い開始パケット（遅延パケット）であれば破棄する
      if (lastFrameIdValid && static_cast<int16_t>(packet.frameId - lastFrameId) <= 0)
      {
        continue;
      }

      // 受信したデータを捨てて最初からやり直す
      count = 0;
      currentFrameId = packet.frameId;

      // 先頭パケットが保持するパケット数を保存する
      if (packet.count == INT_MIN) return -1;
      total = packet.count = -packet.count;
      if (total <= 0 || total > limit) return -1;
      received.assign(total, false);
    }

    // 現在の受信中フレームIDと異なる遅延パケットは破棄する
    if (total >= 0 && packet.frameId != currentFrameId)
    {
      continue;
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

    // 残りパケット数は 1..total の範囲でなければならない
    if (packet.count <= 0 || packet.count > total)
    {
      // パケット数が多すぎる
#if defined(_DEBUG)
      std::cerr << "invalid packet count\n";
#endif
      return -1;
    }

    // シーケンス番号とペイロードのサイズ
    const int sequence{ total - packet.count };
    const int size{ bytes - static_cast<int>(sizeof packet.frameId + sizeof packet.count) };

    // ペイロードのサイズをチェック
    if (size < 0 || (sequence + 1 < total && size != static_cast<int>(sizeof packet.data)))
    {
      // 短すぎるパケット
#if defined(_DEBUG)
      std::cerr << "too short\n";
#endif
      return -1;
    }

#if defined(_DEBUG)
    // シーケンス番号・データサイズ
    std::cerr << "recv seq:" << total - packet.count << ", size:" << size << "\n";
#endif

    // 保存先の位置
    const int pos{ sequence * static_cast<int>(sizeof packet.data) };

    // 保存先に収まるかどうかチェック
    if (pos < 0 || size > len || pos > len - size)
    {
      // オーバーフロー
#if defined(_DEBUG)
      std::cerr << "buffer overflow\n";
#endif
      return -1;
    }

    // UDPの再送・重複で未受信領域が残ったまま完了扱いにしない
    if (received[sequence]) continue;

    // 保存先の領域の先頭
    char* const ptr{ static_cast<char*>(buf) + pos };

    // ペイロードを保存する
    memcpy(ptr, packet.data, size);

    received[sequence] = true;
    ++count;
    receivedBytes = std::max(receivedBytes, pos + size);

    // 全部のパケットを受け取っていれば終わる
    if (count >= total) break;
  }

  // 最後に完了したフレームIDを記録する
  lastFrameId = currentFrameId;
  lastFrameIdValid = true;
  lastRecvTime = std::chrono::steady_clock::now();

  // 呼び出し側がフレーム内部の境界を検証できるよう、実バイト数を返す
  return receivedBytes;
}

//
// 1 フレーム送信
//
unsigned int Network::sendData(const void* buf, int len) const
{
  if (!buf || len <= 0) return static_cast<unsigned int>(-1);

  // パケット
  Packet packet;

  // フレームIDを設定してインクリメント
  const unsigned short frameId{ sendFrameId++ };
  packet.frameId = frameId;

  // 残りのパケット数はデータを送りきるのに必要なペイロードの数
  int total{ (len - 1) / static_cast<int>(sizeof packet.data) + 1 };

  // フレーム境界をTCP of 接続なしで識別できるよう、先頭だけ総数を負にする
  packet.count = -total;

  // 送っていないパケットがある間
  for (int count = 0; total > 0; ++count)
  {
    packet.frameId = frameId;

    // 送信するデータの先頭
    const char* const ptr{ static_cast<const char*>(buf) + count * sizeof packet.data };

    // パケットサイズ
    const int dlen{ static_cast<int>(sizeof packet.data) };
    const int size{ len > dlen ? dlen : len };

    // ペイロードに 1 パケット分データをコピーする
    memcpy(packet.data, ptr, size);

#if defined(_DEBUG)
    // シーケンス番号・データサイズ
    std::cerr << "send seq:" << count << ", size:" << size << "\n";
#endif

    // 1 パケット分データを送信する
    const int ret{ sendPacket(&packet, size + sizeof packet.frameId + sizeof packet.count) };

    // 送信したデータサイズを確かめる
    if (ret <= 0)
    {
      // 送信失敗
      if (ret == SOCKET_ERROR) getError();
      return ret;
    }

    // 残りのデータ量
    len -= ret - sizeof packet.frameId - sizeof packet.count;

    // 残りのパケット数を減らす
    packet.count = --total;
  }

  // 送り切れなかったデータ量
  return len;
}

//
// EOF 送信
//
int Network::sendEof() const
{
  char c;
  return sendPacket(&c, 0);
}

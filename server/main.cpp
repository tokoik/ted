///
/// ted-server: Quest と指示者側 TED の間を中継する UDP リレー (フェーズ0)
///
/// @file
/// @author Kohe Tokoi
/// @date July 31, 2026
///
/// @details
/// Quest 側 (ted-quest, WORKER役) と指示者側 PC (ted-openxr, OPERATOR役) は、
/// これまでの peer-to-peer 直結ではなく、この ted-server を挟んで通信する。
/// ted-server は Quest から見れば OPERATOR、指示者から見れば WORKER として振る舞い、
/// 受け取ったフレームをそのまま相手側へ転送するだけの薄い中継である。
/// プロトコルは Network.h/.cpp（リポジトリルートから複製、GgApp.h は同梱のスタブに置換）を
/// そのまま流用し、フレームの中身（画像・姿勢データ）は一切解釈しない。
///
/// 使い方:
///   ted-server.exe <questPort> <questAddress> <instructorPort> <instructorAddress>
///
/// 設定変更コマンド用の WebSocket チャンネルやブラウザ制御UIとの連携は、
/// この UDP 中継が実機で確認できてから後続フェーズで追加する。
///

#include "Network.h"

#if defined(_WIN32)
#  include <winsock2.h>
#  pragma comment(lib, "ws2_32.lib")
#endif

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// Camera.h の maxFrameSize と同じ値を保つこと（画像1フレーム分の上限）
constexpr int maxFrameSize{ 1024 * 1024 };

// Ctrl+C 等で終了要求があったら false にする
std::atomic<bool> serverRunning{ true };

#if defined(_WIN32)
BOOL WINAPI consoleCtrlHandler(DWORD ctrlType)
{
  switch (ctrlType)
  {
  case CTRL_C_EVENT:
  case CTRL_CLOSE_EVENT:
  case CTRL_BREAK_EVENT:
    serverRunning = false;
    return TRUE;
  default:
    return FALSE;
  }
}
#endif

///
/// from から受け取ったフレームをそのまま to へ転送し続ける
///
/// @param from 受信元
/// @param to 転送先
/// @param label ログ表示用のラベル
///
void relay(Network& from, Network& to, const char* label)
{
  std::vector<char> buf(maxFrameSize);

  while (serverRunning)
  {
    // recvData は約500ms でタイムアウトして負値を返すので、
    // その都度 serverRunning を確認しながらブロッキング受信する
    const int bytes{ from.recvData(buf.data(), static_cast<int>(buf.size())) };

    if (bytes > 0)
    {
      to.sendData(buf.data(), bytes);
    }
    else if (bytes == 0)
    {
      // 長さ0パケット（相手側の受信スレッド終了要求）はそのまま転送する
      to.sendEof();
    }
    // 負値（タイムアウト・破損パケット等）は無視して受信を続ける
  }

  std::cerr << label << ": stopped\n";
}

int main(int argc, char* argv[])
{
  if (argc != 5)
  {
    std::cerr <<
      "usage: ted-server <questPort> <questAddress> <instructorPort> <instructorAddress>\n";
    return EXIT_FAILURE;
  }

  const unsigned short questPort{ static_cast<unsigned short>(std::atoi(argv[1])) };
  const std::string questAddress{ argv[2] };
  const unsigned short instructorPort{ static_cast<unsigned short>(std::atoi(argv[3])) };
  const std::string instructorAddress{ argv[4] };

#if defined(_WIN32)
  // Winsock 2 を初期化する
  WSAData wsaData;
  if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
  {
    std::cerr << "Windows Sockets 2 の初期化に失敗しました。\n";
    return EXIT_FAILURE;
  }
  atexit(reinterpret_cast<void(*)()>(WSACleanup));

  SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#endif

  // Quest 側からは OPERATOR、指示者側からは WORKER として振る舞う
  Network toQuest, toInstructor;

  if (toQuest.initialize(1, questPort, questAddress.c_str()) != 0
    || !toQuest.running())
  {
    std::cerr << "Quest 側ソケットの初期化に失敗しました。\n";
    return EXIT_FAILURE;
  }

  if (toInstructor.initialize(2, instructorPort, instructorAddress.c_str()) != 0
    || !toInstructor.running())
  {
    std::cerr << "指示者側ソケットの初期化に失敗しました。\n";
    return EXIT_FAILURE;
  }

  std::cerr << "ted-server: relaying " << questAddress << ":" << questPort
    << " <-> " << instructorAddress << ":" << instructorPort << "\n";

  // Quest -> 指示者、指示者 -> Quest を別スレッドで双方向中継する
  std::thread questToInstructor(relay, std::ref(toQuest), std::ref(toInstructor), "quest->instructor");
  std::thread instructorToQuest(relay, std::ref(toInstructor), std::ref(toQuest), "instructor->quest");

  questToInstructor.join();
  instructorToQuest.join();

  return EXIT_SUCCESS;
}

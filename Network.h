#pragma once

#include <chrono>

///
/// ネットワーク関連の処理クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///

#if defined(_WIN32)
#  include <winsock2.h>
#endif

///
/// UDP通信を管理するクラス
///
/// @details
/// TED同士で姿勢と画像を交換するためのUDP通信を管理する。
/// 大きなフレームは独自ヘッダ付きの複数データグラムへ分割し、受信側で順序を復元する。
///
class Network
{
  // 受信元の検証と送信先の指定を分離するため、
  // 送受信ソケットとアドレスを別々に保持する
  // 受信ソケットと送信ソケットは同じポート番号を使うが、
  // 送信先のアドレスは相手側のIPアドレスを指定する

  /// 受信ソケット
  SOCKET recvSock{ INVALID_SOCKET };

  /// 送信ソケット
  SOCKET sendSock{ INVALID_SOCKET };

  /// 受信アドレス
  sockaddr_in recvAddr{};

  /// 送信アドレス
  sockaddr_in sendAddr{};

  /// 送信フレームID（シリアル番号）
  mutable unsigned short sendFrameId{ 0 };

  /// 最後に受信完了または開始したフレームID
  unsigned short lastFrameId{ 0 };

  /// lastFrameIdが有効かどうか（再接続時に無効化する）
  bool lastFrameIdValid{ false };

  /// 最後に正常にフレーム受信完了した時点の時刻
  std::chrono::steady_clock::time_point lastRecvTime;

  /// 役割
  ///
  /// @details
  /// 役割に応じて向かい合う送受信ポートを選ぶ。
  /// role == 0 : STANDALONE  (単独), 1: OPERATOR (指導者), 2: WORKER (作業者)
  ///
  enum class Role : int { STANDALONE = 0, OPERATOR, WORKER } role{ Role::STANDALONE };

public:

  ///
  /// コンストラクタ
  ///
  Network() = default;

  ///
  /// デストラクタ
  ///
  virtual ~Network();

  ///
  /// エラーコードの取得
  ///
  /// @return エラーコード
  ///
  int getError() const;

  int initializeRecv(unsigned short port);
  int initializeSend(unsigned short port, const char* address);
  int initialize(int role, unsigned short port, const char* address);

  ///
  /// 終了処理
  ///
  void finalize();

  ///
  /// 実行中かどうか調べる
  ///
  /// @return 実行中なら true
  ///
  bool running() const;

  ///
  /// STANDALONE かどうか調べる
  ///
  /// @return STANDALONE なら true
  ///
  bool isStandalone() const;

  ///
  /// 指導者かどうか調べる
  ///
  /// @return 指導者なら true
  ///
  bool isInstructor() const;

  ///
  /// 作業者かどうか調べる
  ///
  /// @return 作業者なら true
  ///
  bool isWorker() const;

  ///
  /// 最後に受信したデータグラムが設定済みの通信相手から届いたものか確認する
  ///
  /// @return 設定済みの通信相手から届いた場合は true
  ///
  bool checkRemote() const;

  ///
  /// 1 パケット受信
  ///
  /// @param buf 受信バッファ
  /// @param len 受信バッファの長さ
  /// @return 受信したバイト数
  ///
  int recvPacket(void* buf, int len);

  ///
  /// 1 パケット送信
  ///
  /// @param buf 送信バッファ
  /// @param len 送信バッファの長さ
  /// @return 送信したバイト数
  ///
  int sendPacket(const void* buf, int len) const;

  ///
  /// 1 フレーム受信
  ///
  /// @param buf 受信バッファ
  /// @param len 受信バッファの長さ
  /// @return 受信したバイト数、EOFは0、失敗時は負値
  ///
  /// @details
  /// 分割データグラムを連番位置へ復元する。範囲外・重複・別送信元を検査し、
  /// 成功時は復元したバイト数、EOFは0、失敗時は負値を返す。
  int recvData(void* buf, int len);

  ///
  /// 1 フレーム送信
  ///
  /// @param buf 送信バッファ
  /// @param len 送信バッファの長さ
  /// @return 送信したバイト数
  /// 
  /// @details
  /// 1フレームをUDPの最大ペイロード以下に分割し、復元用の残パケット数を付けて送る
  ///
  unsigned int sendData(const void* buf, int len) const;

  ///
  /// 相手側の受信スレッドを終了させるため、長さ0のデータグラムを送る
  ///
  /// @return 送信したバイト数
  ///
  int sendEof() const;
};

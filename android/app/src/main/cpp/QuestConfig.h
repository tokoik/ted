#pragma once

///
/// Quest 3 版の設定
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
/// @details
/// アプリ専用の外部ストレージ
/// (/sdcard/Android/data/com.example.ted_openxr/files/ted_quest.json) から読み込む。
/// ファイルが無ければ既定値で作成するので、adb pull / push で編集する。
/// キー名は PC 版 config.json と同じ意味のものは同じ名前にしている。
///

#include <array>
#include <string>

struct QuestConfig
{
  /// 通信相手 (中継サーバまたは指示者 PC) の IP アドレス
  std::string host{ "192.168.0.7" };

  /// 通信に使うポート番号 (作業者は port + 1 で受信し port へ送信する)
  int port{ 12345 };

  /// パススルーカメラの映像を送信するなら true
  bool send_images{ true };

  /// パススルーカメラから取得する画像の幅と高さ
  int camera_width{ 1280 };
  int camera_height{ 960 };

  /// 送信する JPEG 画像の品質 (0～100)
  int transmit_quality{ 50 };

  /// 送信する画像のフレームレートの上限 (0 なら取得できただけ送る)
  double transmit_fps{ 30.0 };

  /// 姿勢を送信する間隔 (ミリ秒, PC 版の minDelay と同じ既定値)
  int send_interval{ 10 };

  /// パススルー映像を背景に表示するなら true
  bool passthrough{ true };

  /// 自分の手のモデルを表示するなら true
  bool show_local_hands{ true };

  /// 受信した指示者の手のモデルを表示するなら true
  bool show_remote_hands{ true };

  /// 指示者の手を重畳するときに頭部座標系で加える平行移動 (メートル)
  std::array<float, 3> remote_hand_position{ 0.0f, 0.0f, 0.0f };

  ///
  /// 設定ファイルを読み込む
  ///
  /// @param path 設定ファイルのパス
  /// @return 読み込めたら true (読み込めなかった項目は既定値のまま)
  ///
  bool load(const std::string& path);

  ///
  /// 設定ファイルを保存する
  ///
  /// @param path 設定ファイルのパス
  /// @return 保存できたら true
  ///
  bool save(const std::string& path) const;
};

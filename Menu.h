#pragma once

///
/// メニュークラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///

// ウィンドウ関連の処理
#include "GgApp.h"

// シーングラフ
#include "Scene.h"

// 姿勢
#include "Attitude.h"

// ビデオキャプチャ
#include "CamMf.h"

///
/// メニュークラス
///
class Menu
{
  ///
  /// カメラデータの UI 候補をキャッシュする構造体
  ///
  /// @details
  /// Media Foundationの列挙は重いため、デバイスまたはコーデックが変わった時だけ
  /// UI 候補を再生成するための左右カメラ別キャッシュ
  struct CameraMenuCache
  {
    int lastDeviceId = -2;
    std::vector<CamMf::VideoFormat> formats;
    std::vector<std::string> codecs;
    std::vector<std::string> resolutions;
    std::string lastCodec = "";
  } cameraMenuCache[camCount];

  /// キャッシュを更新するヘルパー関数
  void updateCameraMenuCache(int cam);

  /// このアプリケーション
  GgApp* app{ nullptr };

  /// メニューを表示するウィンドウ
  GgApp::Window& window;

  /// メニューで操作する姿勢
  Attitude& attitude;

  /// 設定ファイルの再読み込みが描画側での反映を待っている場合は true
  bool configReloadPending{ false };

  /// 再読み込みした設定を描画側で反映できなかった場合に戻す設定
  Config previousConfig;

  /// データ読み込みエラー表示ウィンドウの表示フラグ
  bool showNodataWindow{ false };

  /// 表示設定ウィンドウの表示フラグ
  bool showDisplayWindow{ true };

  /// 姿勢設定ウィンドウの表示フラグ
  bool showAttitudeWindow{ true };

  /// 入力設定ウィンドウの表示フラグ
  bool showInputWindow{ true };

  /// 起動時設定ウィンドウの表示フラグ
  bool showStartupWindow{ false };

  /// セカンダリディスプレイの設定の一時コピー
  int secondary{ 0 };

  /// フルスクリーン設定の一時コピー
  bool fullscreen{ false };

  /// クアッドバッファ設定の一時コピー
  bool quadbuffer{ false };

  /// 共有メモリのサイズの一時コピー
  int memorysize[2]{ localShareSize, remoteShareSize };

  ///
  /// メニューバー
  ///
  void menuBar();

  ///
  /// 設定ファイルを読み込み描画オブジェクトを差し替える
  ///
  int loadConfig();

  ///
  /// データ読み込みエラー表示ウィンドウ
  ///
  void nodataWindow();

  ///
  /// 表示設定ウィンドウ
  ///
  void displayWindow();

  ///
  /// 姿勢設定ウィンドウ
  ///
  void attitudeWindow();

  ///
  /// 入力設定ウィンドウ
  ///
  void inputWindow();

  ///
  /// 起動時設定ウィンドウ
  ///
  void startupWindow();

public:

  ///
  /// コンストラクタ
  ///
  /// @param app このアプリケーション
  /// @param window メニューを表示するウィンドウ
  /// @param attitude メニューで操作する姿勢
  ///
  Menu(GgApp* app, GgApp::Window& window, Attitude& attitude);

  ///
  /// デストラクタ
  ///
  virtual ~Menu();

  ///
  /// 設定ファイルの再読み込みが要求されているか調べる
  ///
  /// @return 再読み込みした設定が描画側での反映を待っている場合は true
  ///
  bool isConfigReloadPending() const { return configReloadPending; }

  ///
  /// 設定ファイルの描画側への反映結果を通知する
  ///
  /// @param status 反映に成功した場合は true
  ///
  void finishConfigReload(bool status);

  ///
  /// メニューの表示
  ///
  void show();
};


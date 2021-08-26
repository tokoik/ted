#pragma once

//
// メニュー
//

// ウィンドウ関連の処理
#include "GgApp.h"

// シーングラフ
#include "Scene.h"

// 姿勢
#include "Attitude.h"

class Menu
{
  // このアプリケーション
  GgApp* app;

  // メニューを表示するウィンドウ
  Window& window;

  // メニューで操作するシーン
  Scene& scene;

  // メニューで操作する姿勢
  Attitude& attitude;

  // サブウィンドウのオン・オフ
  bool showNodataWindow;
  bool showDisplayWindow;
  bool showAttitudeWindow;
  bool showInputWindow;
  bool showStartupWindow;

  // 起動時設定のコピー
  int secondary;
  bool fullscreen;
  bool quadbuffer;
  int memorysize[2];

  // メニューバー
  void menuBar();

  // データ読み込みエラー表示ウィンドウ
  void nodataWindow();

  // 表示設定ウィンドウ
  void displayWindow();

  // 姿勢設定ウィンドウ
  void attitudeWindow();

  // 入力設定ウィンドウ
  void Menu::inputWindow();
    
  // 起動時設定ウィンドウ
  void startupWindow();

public:

  // コンストラクタ
  Menu(GgApp* app, Window& window, Scene& scene, Attitude& attitude);

  // デストラクタ
  virtual ~Menu();

  // メニューの表示
  void show();
};

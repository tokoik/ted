#pragma once

//
// メニュー
//

// ウィンドウ関連の処理
#include "Window.h"

class Menu
{

  // メニューを表示するウィンドウ
  Window& window;

  // サブウィンドウのオン・オフ
  bool showNodataWindow;
  bool showDisplayWindow;
  bool showCameraWindow;
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

  // カメラ設定ウィンドウ
  void cameraWindow();

  // 起動時設定ウィンドウ
  void startupWindow();

public:

  // コンストラクタ
  Menu(Window& window);

  // デストラクタ
  virtual ~Menu();

  // メニューの表示
  void show();
};

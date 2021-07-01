#pragma once

//
// メニュー
//

// ウィンドウ関連の処理
#include "Window.h"

// シーングラフ
#include "Scene.h"

// 姿勢
#include "Attitude.h"

class Menu
{

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

  // 起動時設定ウィンドウ
  void startupWindow();

public:

  // コンストラクタ
  Menu(Window& window, Scene& scene, Attitude& attitude);

  // デストラクタ
  virtual ~Menu();

  // メニューの表示
  void show();
};

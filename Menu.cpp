//
// メニュー
//
#include "Menu.h"

// ネットワーク関連の処理
#include "Network.h"

// カメラ関連の処理
#include "CamCv.h"
#include "CamOv.h"
#include "CamImage.h"
#include "CamRemote.h"

// Dear ImGui
#include "imgui.h"

// Native File Dialog
#include "nfd.h"
#pragma comment(lib, "nfd.lib")

// 標準ライブラリ
#include <cstdlib>

//
// 設定ファイルの読み込み
//
int loadConfig()
{
  // ファイルパスの取得先
  nfdchar_t* openPath{ nullptr };

  // ダイアログを開く
  static constexpr nfdfilteritem_t filter[]{ {"JSON", "json"}, {"TEXT", "txt"} };
  const nfdresult_t result{ NFD_OpenDialog(&openPath,
    filter, static_cast<nfdfiltersize_t>(std::size(filter)),
    nullptr) };

  // ファイルパスが取得できたら
  if (result == NFD_OKAY)
  {
    // 設定ファイルを読み込んで
    const bool status{ defaults.load(openPath) };

    // ファイルパスに使ったメモリを解放して
    NFD_FreePath(openPath);

    // 設定ファイルの読み込み結果を返す
    return status ? 1 : 0;
  }

  // ダイアログをキャンセルしたときはエラーにしない
  return result == NFD_CANCEL ? -1 : 0;
}

//
// 設定ファイルの保存
//
int saveConfig()
{
  // ファイルパスの取得先
  nfdchar_t* savePath{ nullptr };

  // ダイアログを開く
  static constexpr nfdfilteritem_t filter[]{ {"JSON", "json"} };
  const nfdresult_t result{ NFD_SaveDialog(&savePath,
    filter, static_cast<nfdfiltersize_t>(std::size(filter)),
    nullptr, defaults.config_file.c_str()) };

  // ファイルパスが取得できたら
  if (result == NFD_OKAY)
  {
    // 設定ファイルを読み込んで
    const bool status{ defaults.save(savePath) };

    // ファイルパスに使ったメモリを解放して
    NFD_FreePath(savePath);

    // 設定ファイルの読み込み結果を返す
    return status ? 1 : 0;
  }

  // ダイアログをキャンセルしたときはエラーにしない
  return result == NFD_CANCEL ? -1 : 0;
}

//
// データ読み込みエラー表示ウィンドウ
//
void Menu::nodataWindow()
{
  constexpr int w{ 238 }, h{ 92 };
  const auto& size{ window.getSize() };
  const float x{ (size[0] - w) * 0.5f }, y{ (size[1] - h) * 0.5f };
  ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Appearing);
  ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Appearing);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);

  ImGui::Begin(u8"エラー", &showNodataWindow);
  ImGui::Text(u8"ファイルの読み書きに失敗しました");
  if (ImGui::Button(u8"閉じる")) showNodataWindow = false;
  ImGui::End();
}

//
// 表示設定ウィンドウ
//
void Menu::displayWindow()
{
  ImGui::SetNextWindowPos(ImVec2(2, 28), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(178, 422), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);

  ImGui::Begin(u8"表示設定", &showDisplayWindow);
  ImGui::Text("Frame rate: %6.2f fps", ImGui::GetIO().Framerate);
  if (ImGui::RadioButton(u8"Monocular", &defaults.display_mode, MONOCULAR)) window.stopOculus();
  if (ImGui::RadioButton(u8"Interlace", &defaults.display_mode, INTERLACE)) window.stopOculus();
  if (ImGui::RadioButton(u8"Top and Bottom", &defaults.display_mode, TOP_AND_BOTTOM)) window.stopOculus();
  if (ImGui::RadioButton(u8"Side by Side", &defaults.display_mode, SIDE_BY_SIDE)) window.stopOculus();
  if (ImGui::RadioButton(u8"Quad Buffer", &defaults.display_mode, QUADBUFFER)) window.stopOculus();
  int mode{ defaults.display_mode };
  if (ImGui::RadioButton("Oculus", &mode, OCULUS) && window.startOculus()) defaults.display_mode = mode;
  ImGui::Checkbox("Game Pad", &defaults.use_controller);
  if (ImGui::Checkbox("Leap Motion", &defaults.use_leap_motion))
  {
    if (defaults.use_leap_motion)
      if (!scene.startLeapMotion()) defaults.use_leap_motion = false;
    else
      scene.stopLeapMotion();
  }
  ImGui::Checkbox(u8"ミラー表示", &window.showMirror);
  ImGui::Checkbox(u8"シーン表示", &window.showScene);
  if (ImGui::SliderFloat(u8"前方面", &defaults.display_near, 0.01f, defaults.display_far))
    window.update();
  if (ImGui::SliderFloat(u8"後方面", &defaults.display_far, defaults.display_near, 10.0f))
    window.update();

  ImGui::End();
}

//
// 姿勢設定ウィンドウ
//
void Menu::attitudeWindow()
{
  ImGui::SetNextWindowPos(ImVec2(182, 28), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(218, 326), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);

  ImGui::Begin(u8"姿勢設定", &showAttitudeWindow);
  ImGui::Text(u8"前景");
  ImGui::InputFloat3(u8"位置", attitude.position.data(), "%6.2f");
  if (ImGui::SliderInt(u8"ズーム率", attitude.foreAdjust.data(), -99, 99))
    window.update();
  if (ImGui::SliderInt(u8"視差", &attitude.parallax, -99, 99))
    window.update();
  ImGui::Text(u8"背景");
  if (ImGui::SliderInt(u8"焦点距離", attitude.backAdjust.data(), -99, 99))
    window.update();
  if (ImGui::SliderInt(u8"間隔", &attitude.offset, -99, 99))
    window.update();
  if (ImGui::SliderInt2(u8"画角", &attitude.circleAdjust[0], -99, 99))
    window.updateCircle();
  if (ImGui::SliderInt2(u8"中心", &attitude.circleAdjust[2], -99, 99))
    window.updateCircle();
  if (ImGui::Button(u8"リセット"))
    window.reset();
  ImGui::End();
}

//
// 入力設定ウィンドウ
//
void Menu::inputWindow()
{
  // 入力設定ウィンドウ
  ImGui::SetNextWindowPos(ImVec2(402, 28), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(334, 496), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);
  ImGui::Begin(u8"入力設定", &showInputWindow);

  static constexpr char *camId[]{ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };

  static int ltype = -1;
  ImGui::RadioButton(u8"左イメージ", &ltype, 0);
  ImGui::SameLine();
  ImGui::Text("%s", defaults.camera_image[camL].c_str());
  ImGui::RadioButton(u8"左ムービー", &ltype, 1);
  ImGui::SameLine();
  ImGui::Text("%s", defaults.camera_movie[camL].c_str());
  ImGui::RadioButton(u8"左カメラ", &ltype, 2);
  ImGui::Combo(u8"左カメラの番号", &defaults.camera_id[camL], camId, IM_ARRAYSIZE(camId));

  static int lgain = 0;
  if (ImGui::ArrowButton("##gain_left_left", ImGuiDir_Left)) --lgain;
  ImGui::SameLine();
  if (ImGui::ArrowButton("##gain_left_right", ImGuiDir_Right)) ++lgain;
  ImGui::SameLine();
  ImGui::Text(u8"利得 %1d", lgain);
  ImGui::SameLine();

  static int lexp = 0;
  if (ImGui::ArrowButton("##exp_left_left", ImGuiDir_Left)) --lexp;
  ImGui::SameLine();
  if (ImGui::ArrowButton("##exp_left_right", ImGuiDir_Right)) ++lexp;
  ImGui::SameLine();
  ImGui::Text(u8"露出 %1d", lexp);
  ImGui::Separator();

  static int rtype = -1;
  ImGui::RadioButton(u8"右イメージ", &rtype, 0);
  ImGui::SameLine();
  ImGui::Text("%s", defaults.camera_image[camR].c_str());
  ImGui::RadioButton(u8"右ムービー", &rtype, 1);
  ImGui::SameLine();
  ImGui::Text("%s", defaults.camera_movie[camR].c_str());
  ImGui::RadioButton(u8"右カメラ", &rtype, 2);
  ImGui::Combo(u8"右カメラの番号", &defaults.camera_id[camR], camId, IM_ARRAYSIZE(camId));

  static int rgain = 0;
  if (ImGui::ArrowButton("##gain_right_left", ImGuiDir_Left)) --rgain;
  ImGui::SameLine();
  if (ImGui::ArrowButton("##gain_right_right", ImGuiDir_Right)) ++rgain;
  ImGui::SameLine();
  ImGui::Text(u8"利得 %1d", rgain);
  ImGui::SameLine();

  static int rexp = 0;
  if (ImGui::ArrowButton("##exp_right_left", ImGuiDir_Left)) --rexp;
  ImGui::SameLine();
  if (ImGui::ArrowButton("##exp_right_right", ImGuiDir_Right)) ++rexp;
  ImGui::SameLine();
  ImGui::Text(u8"露出 %1d", rexp);
  ImGui::Separator();

  ImGui::InputInt2(u8"カメラの画素数", defaults.camera_size.data());

  if (ImGui::RadioButton(u8"Ovrvison Pro", &ltype, 3)) rtype = ltype;

  static int item_current = 3;
  static constexpr char *items[]{
    "2560 x 1920 @ 15fps",
    "1920 x 1080 @ 30fps",
    "1280 x 960 @ 45fps",
    "960 x 950 @ 60fps",
    "1280 x 800 @ 60fps",
    "640 x 480 @ 90fps",
    "320 x 240 @ 120fps",
    "1280 x 960 @ 15fps (USB2.0)",
    "640 x 480 @ 30fps (USB2.0)"
  };
  ImGui::Combo(u8"プロパティ", &item_current, items, IM_ARRAYSIZE(items));

  if (ImGui::RadioButton(u8"RealSense", &ltype, 4)) rtype = ltype;

  ImGui::Button(u8"設定");

#if 0
  if (ImGui::BeginCombo(u8"入力源", devName))
  {
    for (auto d = list.getDevice().begin(); d != list.getDevice().end(); ++d)
    {
      const bool selected(d == device);
      if (ImGui::Selectable(d->name.c_str(), selected))
        devName = (device = d)->name.c_str();
      if (selected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  // キャプチャするサイズとフレームレート
  ImGui::InputInt2(u8"サイズ", size.data());
  ImGui::InputInt(u8"レート", &rate);

  // 利得と露光
  if (ImGui::InputInt(u8"利得", &gain)) camera.setGain(gain);
  if (ImGui::InputInt(u8"露光", &exposure)) camera.setExposure(exposure);

  // 現在のコーデックの見出しを作る
  char defaultCodec[sizeof fourcc + 3];
  memcpy(defaultCodec + 1, fourcc.data(), sizeof fourcc);
  defaultCodec[0] = isprint(defaultCodec[1]) ? '(' : 0;
  defaultCodec[sizeof fourcc + 1] = ')';
  defaultCodec[sizeof fourcc + 2] = 0;

  // 現在のコーデックの見出しを加えたコーデックの一覧
  static const char* const codecs[]{ defaultCodec, "MJPG", "H264", "YUY2", "NV12" };

  // コーデックを選択する
  int encode(0);
  if (ImGui::Combo(u8"符号化", &encode, codecs, sizeof codecs / sizeof codecs[0]) && encode > 0)
  {
    // 選択したコーデックを保存する
    memcpy(fourcc.data(), codecs[encode], sizeof fourcc);
  }

  // デバイスプリファレンスを選択する
  ImGui::RadioButton("ANY", &pref, cv::CAP_ANY);
  ImGui::SameLine();
  ImGui::RadioButton("DSHOW", &pref, cv::CAP_DSHOW);
  ImGui::SameLine();
  ImGui::RadioButton("MSMF", &pref, cv::CAP_MSMF);

  // デバイス設定の変更
  if (ImGui::Button(u8"設定"))
  {
    // キャプチャを停止
    camera.stop();

    // カメラを閉じる
    camera.close();

    // 新しい設定でデバイスを開く
    if (
      (device->id >= 0 && camera.open(device->id, size[0], size[1], rate, fourcc.data(), pref))
      ||
      (!device->name.empty() && camera.open(device->name, size[0], size[1], rate, fourcc.data(), pref))
      )
    {
      // 切り替えたデバイスの特性を取り出す
      size[0] = camera.getWidth();
      size[1] = camera.getHeight();
      rate = static_cast<int>(camera.getFps());
      camera.getCodec(fourcc.data());

      // 今まで使っていたテクスチャを捨てて新しいテクスチャを作る
      image.create(camera.getWidth(), camera.getHeight());

      // キャプチャを再開
      camera.start();
    }
  }

  // デバイスの状態表示
  if (!camera.running())
  {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), u8"入力源が使用できません");
  }

#endif

  // 入力設定ウィンドウの設定終了
  ImGui::End();
}

#if 0
//
// 前景設定ウィンドウ
//
void Menu::objectWindow()
{
  // Object 設定ウィンドウ
  ImGui::SetNextWindowPos(ImVec2(4, 30), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(336, 168), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);
  ImGui::Begin(u8"表示設定", &showObjectWindow);

  // 表示方式
  int* type{ const_cast<int*>(&develop->type) };
  ImGui::RadioButton(u8"縦置き", type, octatech::DevelopShader::PORTRAIT);
  ImGui::SameLine();
  ImGui::RadioButton(u8"天板・地板", type, octatech::DevelopShader::TOP_BOTTOM);
  ImGui::SameLine();
  ImGui::RadioButton(u8"横置き", type, octatech::DevelopShader::LANDSCAPE);
  ImGui::RadioButton(u8"天板", type, octatech::DevelopShader::TOP);
  ImGui::SameLine();
  ImGui::RadioButton(u8"側板", type, octatech::DevelopShader::BODY);
  ImGui::SameLine();
  ImGui::RadioButton(u8"地板", type, octatech::DevelopShader::BOTTOM);
  window.selectInterface(*type);

  // テクスチャをリピートするか
  ImGui::SameLine();
  if (ImGui::Checkbox(u8"画像反復", &repeat)) image.setRepeat(repeat);

  // 図形のスケール
  auto& scale{ image.config.scale };
  if (ImGui::DragFloat2(u8"ドラムサイズ", scale.data(), 0.01f, 0.0f, 20.0f, "%.3f"))
    develop->setScale(scale[0], scale[1], scale[2] = scale[0]);

  // 図形に投影するテクスチャの投影中心
  auto& origin{ image.config.origin };
  if (ImGui::DragFloat2(u8"カメラ位置", origin.data(), 0.01f, -10.0f, 10.0f, "%.3f"))
    origin[2] = 0.0f;

  // Object 設定ウィンドウの設定終了
  ImGui::End();
}

//
// トリムウィンドウ
//
void Menu::trimWindow()
{
  // ImGui のフレームに二つ目の ImGui のウィンドウを描く
  ImGui::SetNextWindowPos(ImVec2(344, 30), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(380, 136), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);
  ImGui::Begin(u8"ムービー切出し", &showTrimWindow);

  // ムービーファイルの再生開始位置と終了位置
  double in(camera.in), out(camera.out), start(0.0), end(camera.getFrames());
  if (ImGui::SliderScalar(u8"開始点", ImGuiDataType_Double, &in, &start, &end, "%.0f")
    && in >= start && in <= camera.out) camera.in = in;
  if (ImGui::SliderScalar(u8"終了点", ImGuiDataType_Double, &out, &start, &end, "%.0f")
    && out >= camera.in && out <= end) camera.out = out;

  // 進捗
  const auto frame(camera.getPosition());
  const auto count(static_cast<unsigned int>(frame));
  const auto total(static_cast<unsigned int>(end));
  char buf[32];
#if defined(__APPLE__)
  sprintf(buf, "%u/%u", count, total);
#else
  sprintf_s(buf, sizeof buf, "%u/%u", count, total);
#endif
  ImGui::ProgressBar(static_cast<float>(frame / end), ImVec2(0.0f, 0.0f), buf);

  // Trimming ウィンドウの設定終了
  ImGui::End();
}
#endif

//
// 起動時設定ウィンドウ
//
void Menu::startupWindow()
{
  constexpr GLsizei w{ 212 }, h{ 236 };
  const auto& size{ window.getSize() };
  const float x{ (size[0] - w) * 0.5f }, y{ (size[1] - h) * 0.5f };
  ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Appearing);
  ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Appearing);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);

  ImGui::Begin(u8"起動時設定", &showStartupWindow);
  ImGui::Checkbox(u8"クアッドバッファステレオ", &quadbuffer);
  ImGui::Checkbox(u8"フルスクリーン", &fullscreen);

  ImGui::Text(u8"セカンダリモニタの番号");
  ImGui::InputInt("", &secondary);
  secondary = std::max(secondary, 0);

  ImGui::Text(u8"共有メモリのサイズ");
  ImGui::InputInt2("", memorysize);
  memorysize[0] = std::max(memorysize[0], localShareSize);
  memorysize[1] = std::max(memorysize[1], remoteShareSize);

  if (ImGui::Button(u8"設定を保存"))
  {
    // 保存に失敗したときのために元の値を取っておく
    std::swap(defaults.display_secondary,secondary);
    std::swap(defaults.display_fullscreen, fullscreen);
    std::swap(defaults.display_quadbuffer, quadbuffer);
    std::swap(defaults.local_share_size, memorysize[0]);
    std::swap(defaults.remote_share_size, memorysize[1]);

    // 設定ファイルを保存する
    if (saveConfig() <= 0)
    {
      // 保存に失敗したかキャンセルしたので元の値を復帰する
      defaults.display_secondary = secondary;
      defaults.display_fullscreen = fullscreen;
      defaults.display_quadbuffer = quadbuffer;
      defaults.local_share_size = memorysize[0];
      defaults.remote_share_size = memorysize[1];
    }
    else
    {
      // このウィンドウを閉じる
      showStartupWindow = false;
    }
  }
  ImGui::SameLine();
  if (ImGui::Button(u8"キャンセル"))
  {
    // このウィンドウを閉じる
    showStartupWindow = false;
  }
  ImGui::End();
}

//
// メニューバー
//
void Menu::menuBar()
{
  // メインメニューバー
  if (ImGui::BeginMainMenuBar())
  {
    // File メニュー
    if (ImGui::BeginMenu(u8"ファイル"))
    {
      // ファイルダイアログから得るパス
      nfdchar_t* filepath(NULL);

      // 設定ファイルを開く
      if (ImGui::MenuItem(u8"設定ファイルを読み込む"))
        showNodataWindow = !loadConfig();

      // 設定ファイルを保存する
      else if (ImGui::MenuItem(u8"設定ファイルを保存する"))
        showNodataWindow = !saveConfig();

      // 起動時設定
      else if (ImGui::MenuItem(u8"起動時設定", nullptr, &showStartupWindow))
      {
        // 起動時設定のコピーを取っておく
        secondary = defaults.display_secondary;
        fullscreen = defaults.display_fullscreen;
        quadbuffer = defaults.display_quadbuffer;
        memorysize[0] = defaults.local_share_size;
        memorysize[1] = defaults.remote_share_size;
      }

      // 終了
      else if (ImGui::MenuItem(u8"終了")) window.setClose();

      // File メニュー修了
      ImGui::EndMenu();
    }

    // Window メニュー
    else if (ImGui::BeginMenu(u8"ウィンドウ"))
    {
      // 表示設定ウィンドウを開くか
      ImGui::MenuItem(u8"表示設定", NULL, &showDisplayWindow);

      // 姿勢設定ウィンドウを開くか
      ImGui::MenuItem(u8"姿勢設定", NULL, &showAttitudeWindow);

      // 入力設定ウィンドウを開くか
      ImGui::MenuItem(u8"入力設定", NULL, &showInputWindow);

      // Window メニュー終了
      ImGui::EndMenu();
    }

    // メインメニューバー終了
    ImGui::EndMainMenuBar();
  }
}

//
// コンストラクタ
//
Menu::Menu(Window& window, Scene& scene, Attitude& attitude)
  : window{ window }
  , scene{ scene }
  , attitude{ attitude }
  , showNodataWindow{ false }
  , showDisplayWindow{ true }
  , showAttitudeWindow{ true }
  , showInputWindow{ true }
  , showStartupWindow{ false }
  , secondary{ 0 }
  , fullscreen{ false }
  , quadbuffer{ false }
  , memorysize{ localShareSize, remoteShareSize }
{
  NFD_Init();
}

//
// デストラクタ
//
Menu::~Menu()
{
  NFD_Quit();
}

//
// メニューの表示
//
void Menu::show()
{
  ImGui::NewFrame();
  menuBar();
  if (showNodataWindow) nodataWindow();
  if (showDisplayWindow) displayWindow();
  if (showAttitudeWindow) attitudeWindow();
  if (showInputWindow) inputWindow();
  if (showStartupWindow) startupWindow();
  ImGui::Render();
}

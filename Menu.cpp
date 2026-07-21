///
/// メニュークラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///
#include "Menu.h"

// ネットワーク関連の処理
#include "Network.h"

// カメラ関連の処理
#include "CamMf.h"
#include "CamOv.h"
#include "CamImage.h"
#include "CamRemote.h"

// Dear ImGui
#include "imgui.h"

// Native File Dialog
#include "nfd.h"

// 標準ライブラリ
#include <cstdlib>
#include <iostream>

//
// 設定ファイルの読み込み
//
int Menu::loadConfig()
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
    // 描画オブジェクトの生成に失敗したとき戻せるよう設定を保存する
    previousConfig = defaults;

    // 設定ファイルを読み込み、描画側へ再構築を要求する
    bool status{ false };
    try
    {
      status = defaults.load(openPath);
      configReloadPending = status;
    }
    catch (const std::exception& error)
    {
#if defined(_DEBUG)
      std::cerr << "Failed to reload configuration: " << error.what() << std::endl;
#endif
      showNodataWindow = true;
    }
    catch (...)
    {
#if defined(_DEBUG)
      std::cerr << "Failed to reload configuration: unknown exception" << std::endl;
#endif
      showNodataWindow = true;
    }

    if (!status) defaults = previousConfig;

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
// ファイルパスの取得
//
void getFilePath(std::string& path, const nfdfilteritem_t* filter)
{
  // ファイルダイアログから得るパス
  nfdchar_t* filepath{ nullptr };

  // ファイルダイアログを開く
  if (NFD_OpenDialog(&filepath, filter, 1, nullptr) == NFD_OKAY)
  {
    path = filepath;
    NFD_FreePath(filepath);
  }
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
  ImGui::SetNextWindowPos(ImVec2(2, 25), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(165, 428), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);

  ImGui::Begin(u8"表示設定", &showDisplayWindow);

  // 表示モードの選択
  int display_mode{ defaults.display_mode };
  ImGui::RadioButton(u8"単眼視", &display_mode, MONOCULAR);
  ImGui::RadioButton(u8"上下分割", &display_mode, TOP_AND_BOTTOM);
  ImGui::RadioButton(u8"左右分割", &display_mode, SIDE_BY_SIDE);
  ImGui::RadioButton(u8"左右重畳", &display_mode, OVERLAY);
  ImGui::RadioButton(u8"Quad Buffer", &display_mode, QUADBUFFER);
  ImGui::RadioButton(u8"OpenXR", &display_mode, OPENXR);

  // 表示モードが変更されたとき
  if (display_mode != defaults.display_mode)
  {
    // OpenXR に切り替えたなら
    if (display_mode == OPENXR)
    {
      // OpenXR を起動して表示モードをそれに切り替える
      if (window.startHMD()) defaults.display_mode = display_mode;
    }
    // OpenXR 以外に切り替えたなら
    else
    {
      // Quadbuffer Stereo が使えなければそれには切り替えない
      if (display_mode != QUADBUFFER || defaults.display_quadbuffer)
      {
        // それまで OpenXR を使っていたなら止める
        if (defaults.display_mode == OPENXR) window.stopHMD();

        // 表示モードを切り替える
        defaults.display_mode = display_mode;
      }
    }

    // ビューポートを更新する
    window.resetViewport();
  }

  // ゲームパッドを有効にするかどうか
  ImGui::Checkbox("Game Pad", &defaults.use_controller);

  // Leap Motion を有効にするかどうか
  bool use_leap_motion{ defaults.use_leap_motion };
  if (ImGui::Checkbox("Leap Motion", &use_leap_motion))
  {
    // それまで LeapMotion を使っていないとき
    if (!defaults.use_leap_motion)
    {
      // Leap Motion を使うなら起動する
      if (use_leap_motion && Scene::startLeapMotion())
        defaults.use_leap_motion = true;
    }
    // それまで使っていた Leap Motion を止めるとき
    else if (!use_leap_motion)
    {
      // Leap Motion を止める
      Scene::stopLeapMotion();
      defaults.use_leap_motion = false;
    }
  }

  // 表示関係
  ImGui::Checkbox(u8"ヘッドトラッキング", &defaults.camera_tracking);
  ImGui::Checkbox(u8"ミラー表示", &window.showMirror);
  ImGui::Checkbox(u8"シーン表示", &window.showScene);
  char scene_file[MAX_PATH]{ "" };
  if (defaults.scene.is<std::string>())
  {
    // JSONの長いパスでも固定長のImGui入力バッファを越えないよう末尾を切り詰める
    strncpy_s(scene_file, defaults.scene.get<std::string>().c_str(), _TRUNCATE);
  }
  if (ImGui::InputText(u8"シーン", scene_file, sizeof scene_file))
  {
    defaults.scene = picojson::value(std::string(scene_file));
  }
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
  ImGui::SetNextWindowPos(ImVec2(168, 25), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(200, 304), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);

  ImGui::Begin(u8"姿勢設定", &showAttitudeWindow);
  ImGui::Text(u8"前景");
  ImGui::InputFloat3(u8"位置", attitude.position.data(), "%6.2f");
  if (ImGui::SliderInt(u8"ズーム率", attitude.foreAdjust.data(), -99, 99))
    window.update();
  if (ImGui::SliderInt(u8"視差##前景", &attitude.parallax, -99, 99))
    window.update();
  ImGui::Text(u8"背景");
  if (ImGui::SliderInt(u8"焦点距離", attitude.backAdjust.data(), -99, 99))
    window.update();
  if (ImGui::SliderInt(u8"視差##背景", &attitude.offset, -99, 99))
    window.update();
  if (ImGui::SliderInt2(u8"画角", &attitude.circleAdjust[0], -99, 99))
    window.updateCircle();
  if (ImGui::SliderInt2(u8"中心", &attitude.circleAdjust[2], -99, 99))
    window.updateCircle();
  if (ImGui::Button(u8"回復"))
    window.reset();
  ImGui::End();
}

//
// 入力設定ウィンドウ
//
void Menu::inputWindow()
{
  // 入力設定ウィンドウ
  ImGui::SetNextWindowPos(ImVec2(369, 25), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(200, 674), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);
  ImGui::Begin(u8"入力設定", &showInputWindow);

  ImGui::RadioButton(u8"静止画像", &defaults.input_mode, InputMode::IMAGE);
  static const nfdfilteritem_t image_filter[]{ "Images", "png,jpg,jpeg,jfif,bmp,dib" };
  if (ImGui::Button(u8"左画像"))
    getFilePath(defaults.camera_image[camL], image_filter);
  ImGui::SameLine();
  ImGui::Text(defaults.camera_image[camL].c_str());
  if (ImGui::Button(u8"右画像"))
    getFilePath(defaults.camera_image[camR], image_filter);
  ImGui::SameLine();
  ImGui::Text(defaults.camera_image[camR].c_str());

  ImGui::RadioButton(u8"動画像", &defaults.input_mode, InputMode::MOVIE);
  static const nfdfilteritem_t movie_filter[]{ "Movies", "mp4,m4v,mov,avi,wmv,ogg" };
  if (ImGui::Button(u8"左動画"))
    getFilePath(defaults.camera_movie[camL], movie_filter);
  ImGui::SameLine();
  ImGui::Text(defaults.camera_movie[camL].c_str());
  if (ImGui::Button(u8"右動画"))
    getFilePath(defaults.camera_movie[camR], movie_filter);
  ImGui::SameLine();
  ImGui::Text(defaults.camera_movie[camR].c_str());

  ImGui::RadioButton(u8"カメラ", &defaults.input_mode, InputMode::CAMERA);

  // デバイスリストの取得
  const auto& devices{ CamMf::getDeviceList() };

  for (int cam = 0; cam < camCount; ++cam)
  {
    ImGui::PushID(cam);
    const char* sideLabel{ (cam == camL) ? u8"左カメラ" : u8"右カメラ" };

    // キャッシュ更新
    updateCameraMenuCache(cam);
    const auto& cache{ cameraMenuCache[cam] };

    // デバイス選択コンボボックス
    int currentDevice{ defaults.camera_id[cam] };
    std::string deviceComboLabel{ u8"未選択" };
    if (currentDevice >= 0 && currentDevice < devices.size()) {
      deviceComboLabel = devices[currentDevice];
    }

    if (ImGui::BeginCombo(sideLabel, deviceComboLabel.c_str()))
    {
      for (int i = 0; i < devices.size(); ++i)
      {
        bool isSelected = (currentDevice == i);
        if (ImGui::Selectable(devices[i].c_str(), isSelected))
        {
          defaults.camera_id[cam] = i;
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    // コーデック選択コンボボックス
    std::string currentCodec = defaults.camera_codec[cam];
    if (ImGui::BeginCombo(u8"符号化", currentCodec.c_str()))
    {
      for (const auto& codec : cache.codecs)
      {
        bool isSelected = (currentCodec == codec);
        if (ImGui::Selectable(codec.c_str(), isSelected))
        {
          defaults.camera_codec[cam] = codec;
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    // 解像度選択コンボボックス
    std::string currentResolution = defaults.camera_resolution[cam];
    if (ImGui::BeginCombo(u8"解像度", currentResolution.c_str()))
    {
      for (const auto& res : cache.resolutions)
      {
        bool isSelected = (currentResolution == res);
        if (ImGui::Selectable(res.c_str(), isSelected))
        {
          defaults.camera_resolution[cam] = res;
        }
        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    ImGui::PopID();
  }

  ImGui::RadioButton(u8"Ovrvision", &defaults.input_mode, InputMode::OVRVISION);
  static constexpr char *properties[]{
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
  ImGui::Combo(u8"特性", &defaults.ovrvision_property, properties, IM_ARRAYSIZE(properties));

  ImGui::RadioButton(u8"リモート", &defaults.input_mode, InputMode::REMOTE);
  static constexpr char* roles[]{
    u8"単独",
    u8"指導者",
    u8"作業者"
  };
  ImGui::Combo(u8"役割", &defaults.role, roles, IM_ARRAYSIZE(roles));
  char address[16]{ "0.0.0.0" };
  // 外部設定を固定長UIバッファへ移すため、必ず終端できる長さに制限する
  strncpy_s(address, defaults.address.c_str(), _TRUNCATE);
  if (ImGui::InputText(u8"アドレス", address, sizeof address))
    defaults.address = address;
  ImGui::InputInt(u8"ポート", &defaults.port);

  ImGui::Text(u8"シェーダ");
  // 設定ファイル由来のパスをImGuiの固定長編集領域へ安全に複製し、編集後にstd::stringへ戻す
  char vertex_shader[MAX_PATH]{ "" };
  strncpy_s(vertex_shader, defaults.vertex_shader.c_str(), _TRUNCATE);
  if (ImGui::InputText(u8"頂点", vertex_shader, sizeof vertex_shader))
    defaults.vertex_shader = vertex_shader;
  char fragment_shader[MAX_PATH]{ "" };
  strncpy_s(fragment_shader, defaults.fragment_shader.c_str(), _TRUNCATE);
  if (ImGui::InputText(u8"画素", fragment_shader, sizeof fragment_shader))
    defaults.fragment_shader = fragment_shader;

  if (ImGui::Button(u8"設定")) app->selectInput();

  // 入力設定ウィンドウの設定終了
  ImGui::End();
}

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

  ImGui::Text(u8"セカンダリモニタ");
  ImGui::InputInt(u8"番号", &secondary);
  secondary = std::max(secondary, 0);

  ImGui::Text(u8"共有メモリ");
  ImGui::InputInt2(u8"サイズ", memorysize);
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

    // フレームレートの表示
    ImGui::Text("%6.2f fps", ImGui::GetIO().Framerate);

    // メインメニューバー終了
    ImGui::EndMainMenuBar();
  }
}

//
// コンストラクタ
//
Menu::Menu(GgApp* app, GgApp::Window& window, Attitude& attitude)
  : app{ app }
  , window { window }
  , attitude{ attitude }
{
  NFD_Init();
}

//
// 設定ファイルの描画側への反映結果を通知する
//
void Menu::finishConfigReload(bool status)
{
  // 反映できなければ読み込み前の設定に戻す
  if (!status) defaults = previousConfig;

  configReloadPending = false;
  showNodataWindow = !status;
}

// GUID から人間が読める形式の名前を返す
static std::string SubTypeToNameLocal(const GUID& subType)
{
  if (subType == MFVideoFormat_YUY2) return "YUY2";
  if (subType == MFVideoFormat_NV12) return "NV12";
  if (subType == MFVideoFormat_MJPG) return "MJPG";
  if (subType == MFVideoFormat_H264) return "H264";
  if (subType == MFVideoFormat_RGB32) return "RGB32";
  return "";
}

// キャッシュを更新するヘルパー関数
void Menu::updateCameraMenuCache(int cam)
{
  auto& cache{ cameraMenuCache[cam] };
  const auto& devices{ CamMf::getDeviceList() };
  int currentDevice{ defaults.camera_id[cam] };

  // デバイスが無効な場合はクリアして終了
  if (currentDevice < 0 || currentDevice >= devices.size())
  {
    cache.lastDeviceId = currentDevice;
    cache.formats.clear();
    cache.codecs.clear();
    cache.resolutions.clear();
    cache.lastCodec = "";
    return;
  }

  // デバイスが変わったとき
  if (cache.lastDeviceId != currentDevice)
  {
    cache.lastDeviceId = currentDevice;
    CamMf::getDeviceFormats(currentDevice, cache.formats);

    // コーデックリスト構築
    cache.codecs.clear();
    for (const auto& fmt : cache.formats)
    {
      std::string name = SubTypeToNameLocal(fmt.subType);
      if (!name.empty() && std::find(cache.codecs.begin(), cache.codecs.end(), name) == cache.codecs.end())
      {
        cache.codecs.push_back(name);
      }
    }

    // もし現在の config コーデックがなければ最初のコーデックをセット
    if (std::find(cache.codecs.begin(), cache.codecs.end(), defaults.camera_codec[cam]) == cache.codecs.end())
    {
      if (!cache.codecs.empty()) defaults.camera_codec[cam] = cache.codecs[0];
      else defaults.camera_codec[cam] = "";
    }
  }

  // コーデックが変わったとき
  if (cache.lastCodec != defaults.camera_codec[cam])
  {
    cache.lastCodec = defaults.camera_codec[cam];
    cache.resolutions.clear();

    for (const auto& fmt : cache.formats)
    {
      if (SubTypeToNameLocal(fmt.subType) == cache.lastCodec)
      {
        char buf[32];
        sprintf_s(buf, "%d x %d", fmt.width, fmt.height);
        std::string resStr(buf);
        if (std::find(cache.resolutions.begin(), cache.resolutions.end(), resStr) == cache.resolutions.end())
        {
          cache.resolutions.push_back(resStr);
        }
      }
    }

    // もし現在の config 解像度がなければ最初の解像度をセット
    if (std::find(cache.resolutions.begin(), cache.resolutions.end(), defaults.camera_resolution[cam]) == cache.resolutions.end())
    {
      if (!cache.resolutions.empty()) defaults.camera_resolution[cam] = cache.resolutions[0];
      else defaults.camera_resolution[cam] = "";
    }
  }
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


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
#include "CamOv.h"
#include "CamImage.h"
#include "CamRemote.h"

// Dear ImGui
#include "imgui.h"

// Native File Dialog
#include "nfd.h"

// 標準ライブラリ
#include <cmath>
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
    // 現在の設定を新しい設定のデフォルトにする
    Config candidate{ config };

    // 設定ファイルの読み込み結果
    bool status{ false };

    // 設定ファイルを読み込んで新しい設定の候補に上書きする
    try
    {
      status = candidate.load(openPath);
      if (status) pendingConfig = std::move(candidate);
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
int Menu::saveConfig()
{
  // ファイルパスの取得先
  nfdchar_t* savePath{ nullptr };

  // ダイアログを開く
  static constexpr nfdfilteritem_t filter[]{ {"JSON", "json"} };
  const nfdresult_t result{ NFD_SaveDialog(&savePath,
    filter, static_cast<nfdfiltersize_t>(std::size(filter)),
    nullptr, config.config_file.c_str()) };

  // ファイルパスが取得できたら
  if (result == NFD_OKAY)
  {
    // 設定ファイルを読み込んで
    const bool status{ config.save(savePath) };

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
void Menu::getFilePath(std::string& path, const nfdfilteritem_t* filter)
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
  ImGui::SetNextWindowSize(ImVec2(165, 512), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);

  ImGui::Begin(u8"表示設定", &showDisplayWindow);

  // 表示モードの選択
  int display_mode{ config.display_mode };
  ImGui::RadioButton(u8"単眼視", &display_mode, MONOCULAR);
  ImGui::RadioButton(u8"上下分割", &display_mode, TOP_AND_BOTTOM);
  ImGui::RadioButton(u8"左右分割", &display_mode, SIDE_BY_SIDE);
  ImGui::RadioButton(u8"左右重畳", &display_mode, OVERLAY);
  ImGui::RadioButton(u8"Quad Buffer", &display_mode, QUADBUFFER);
  ImGui::RadioButton(u8"HMD (OpenXR)", &display_mode, OPENXR);

  // 表示モードが変更されたとき
  if (display_mode != config.display_mode)
  {
    window.setDisplayMode(display_mode);
  }

  // ゲームパッドを有効にするかどうか
  ImGui::Checkbox(u8"ゲームパッド", &config.use_controller);

  // ハンドトラッキングを有効にするかどうか
  bool use_hand_tracking{ config.hand_tracking != HAND_TRACKING_NONE };
  if (ImGui::Checkbox(u8"ハンドトラッキング", &use_hand_tracking))
  {
    app.setHandTrackingMode(use_hand_tracking ? HAND_TRACKING_LEAP_MOTION : HAND_TRACKING_NONE);
  }

  // デバイスを選択する（常に表示するがチェックボックスが OFF のときはグレーアウト）
  ImGui::BeginDisabled(!use_hand_tracking);
  ImGui::Indent(16.0f);

  // Leap Motion と OpenXR のどちらを使うかラジオボタンで選択する
  int device{ config.hand_tracking == HAND_TRACKING_OPENXR ? HAND_TRACKING_OPENXR : HAND_TRACKING_LEAP_MOTION };
  if (ImGui::RadioButton(u8"Leap Motion", &device, HAND_TRACKING_LEAP_MOTION))
  {
    app.setHandTrackingMode(HAND_TRACKING_LEAP_MOTION);
  }
  if (ImGui::RadioButton(u8"OpenXR", &device, HAND_TRACKING_OPENXR))
  {
    app.setHandTrackingMode(HAND_TRACKING_OPENXR);
  }

  ImGui::Unindent(16.0f);
  ImGui::EndDisabled();

  // ヘッドトラッキングするかどうか
  ImGui::Checkbox(u8"ヘッドトラッキング", &config.head_tracking);

  // リモート映像を姿勢に合わせて安定化するかどうか
  ImGui::Checkbox(u8"リモート安定化", &config.remote_stabilize);

  // 表示関係
  bool showMirror{ window.isMirrorVisible() };
  if (ImGui::Checkbox(u8"ミラー表示", &showMirror)) window.setMirrorVisible(showMirror);
  bool showScene{ window.isSceneVisible() };
  if (ImGui::Checkbox(u8"シーン表示", &showScene)) window.setSceneVisible(showScene);
  char scene_file[MAX_PATH]{ "" };
  if (config.scene.is<std::string>())
  {
    // JSONの長いパスでも固定長のImGui入力バッファを越えないよう末尾を切り詰める
    strncpy_s(scene_file, config.scene.get<std::string>().c_str(), _TRUNCATE);
  }
  if (ImGui::InputText(u8"シーン", scene_file, sizeof scene_file))
  {
    config.scene = picojson::value(std::string(scene_file));
  }

  // クリップ面の設定
  float nearPlane{ config.display_near };
  if (ImGui::SliderFloat(u8"前方面", &nearPlane, 0.01f, config.display_far))
    window.setClipPlanes(nearPlane, config.display_far);
  float farPlane{ config.display_far };
  if (ImGui::SliderFloat(u8"後方面", &farPlane, config.display_near, 10.0f))
    window.setClipPlanes(config.display_near, farPlane);

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
  auto position{ attitude.getPosition() };
  if (ImGui::InputFloat3(u8"位置", position.data(), "%6.2f"))
    attitude.setPosition(position);
  auto foreAdjust{ attitude.getForeAdjust() };
  if (ImGui::SliderInt(u8"ズーム率", foreAdjust.data(), -99, 99))
  {
    attitude.setForeAdjust(foreAdjust);
    window.update();
  }
  int parallax{ attitude.getParallax() };
  if (ImGui::SliderInt(u8"視差##前景", &parallax, -99, 99))
  {
    attitude.setParallax(parallax);
    window.update();
  }
  ImGui::Text(u8"背景");
  auto backAdjust{ attitude.getBackAdjust() };
  if (ImGui::SliderInt(u8"焦点距離", backAdjust.data(), -99, 99))
  {
    attitude.setBackAdjust(backAdjust);
    window.update();
  }
  int offset{ attitude.getOffset() };
  if (ImGui::SliderInt(u8"視差##背景", &offset, -99, 99))
  {
    attitude.setOffset(offset);
    window.update();
  }
  auto circleAdjust{ attitude.getCircleAdjust() };
  if (ImGui::SliderInt2(u8"画角", circleAdjust.data(), -99, 99))
  {
    attitude.setCircleAdjust(circleAdjust);
    window.updateCircle();
  }
  if (ImGui::SliderInt2(u8"中心", circleAdjust.data() + 2, -99, 99))
  {
    attitude.setCircleAdjust(circleAdjust);
    window.updateCircle();
  }
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
  ImGui::SetNextWindowSize(ImVec2(211, 618), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);

  ImGui::Begin(u8"入力設定", &showInputWindow);

  ImGui::RadioButton(u8"静止画像", &config.input_mode, InputMode::IMAGE);
  static const nfdfilteritem_t image_filter[]{ "Images", "png,jpg,jpeg,jfif,bmp,dib" };
  if (ImGui::Button(u8"左画像"))
    getFilePath(config.camera_image[camL], image_filter);
  ImGui::SameLine();
  ImGui::TextUnformatted(config.camera_image[camL].c_str());
  if (ImGui::Button(u8"右画像"))
    getFilePath(config.camera_image[camR], image_filter);
  ImGui::SameLine();
  ImGui::TextUnformatted(config.camera_image[camR].c_str());

  ImGui::RadioButton(u8"動画像", &config.input_mode, InputMode::MOVIE);
  static const nfdfilteritem_t movie_filter[]{ "Movies", "mp4,m4v,mov,avi,wmv,ogg" };
  if (ImGui::Button(u8"左動画"))
    getFilePath(config.camera_movie[camL], movie_filter);
  ImGui::SameLine();
  ImGui::TextUnformatted(config.camera_movie[camL].c_str());
  if (ImGui::Button(u8"右動画"))
    getFilePath(config.camera_movie[camR], movie_filter);
  ImGui::SameLine();
  ImGui::TextUnformatted(config.camera_movie[camR].c_str());

  ImGui::RadioButton(u8"カメラ", &config.input_mode, InputMode::CAMERA);

  // デバイスリストの取得
  const auto& devices{ CameraCapabilities::getDeviceList() };

  for (int cam = 0; cam < camCount; ++cam)
  {
    ImGui::PushID(cam);
    const char* sideLabel{ (cam == camL) ? u8"左カメラ" : u8"右カメラ" };

    // キャッシュ更新
    updateCameraMenuCache(cam);
    const auto& cache{ cameraMenuCache[cam] };

    // 折りたたみ時にも選択内容を確認できるよう、見出し自体を要約行にする
    int currentDevice{ config.camera_id[cam] };
    const bool hasDevice{ currentDevice >= 0 && currentDevice < devices.size() };
    std::string deviceName{ u8"未選択" };
    if (hasDevice)
    {
      deviceName = devices[currentDevice];
    }

    char fpsText[32];
    if (std::abs(config.camera_fps[cam] - std::round(config.camera_fps[cam])) < 0.005)
      sprintf_s(fpsText, "%.0f fps", config.camera_fps[cam]);
    else
      sprintf_s(fpsText, "%.2f fps", config.camera_fps[cam]);

    const char* side{ cam == camL ? u8"左" : u8"右" };
    std::string summary{ side };
    summary += ": " + deviceName;
    if (hasDevice)
    {
      summary += " | " + config.camera_codec[cam]
        + " | " + config.camera_resolution[cam]
        + " | " + fpsText;
    }
    // 表示内容が変わっても開閉状態を維持するため、### 以降を固定 ID として使う
    summary += "###camera_properties";

    const bool showProperties{ ImGui::CollapsingHeader(summary.c_str()) };
    if (ImGui::IsItemHovered())
    {
      ImGui::SetTooltip(u8"%sカメラ\nデバイス: %s\n符号化: %s\n解像度: %s\nFPS: %s",
        side, deviceName.c_str(), config.camera_codec[cam].c_str(),
        config.camera_resolution[cam].c_str(), fpsText);
    }

    if (showProperties)
    {
      // デバイス選択コンボボックス
      std::string deviceComboLabel{ u8"未選択" };
      if (currentDevice >= 0 && currentDevice < devices.size())
      {
        deviceComboLabel = devices[currentDevice];
      }

      if (ImGui::BeginCombo(u8"デバイス", deviceComboLabel.c_str()))
      {
        for (int i = 0; i < devices.size(); ++i)
        {
          bool isSelected = (currentDevice == i);
          if (ImGui::Selectable(devices[i].c_str(), isSelected))
          {
            config.camera_id[cam] = i;
          }
          if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }

      // コーデック選択コンボボックス
      std::string currentCodec = config.camera_codec[cam];
      if (ImGui::BeginCombo(u8"符号化", currentCodec.c_str()))
      {
        for (const auto& codec : cache.codecs)
        {
          bool isSelected = (currentCodec == codec);
          if (ImGui::Selectable(codec.c_str(), isSelected))
          {
            config.camera_codec[cam] = codec;
          }
          if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }

      // 解像度選択コンボボックス
      std::string currentResolution = config.camera_resolution[cam];
      if (ImGui::BeginCombo(u8"解像度", currentResolution.c_str()))
      {
        for (const auto& res : cache.resolutions)
        {
          bool isSelected = (currentResolution == res);
          if (ImGui::Selectable(res.c_str(), isSelected))
          {
            config.camera_resolution[cam] = res;
          }
          if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }

      // フレームレート選択コンボボックス
      char currentFps[32];
      sprintf_s(currentFps, "%.2f fps", config.camera_fps[cam]);
      if (ImGui::BeginCombo(u8"FPS", currentFps))
      {
        for (const double fps : cache.frameRates)
        {
          char label[32];
          sprintf_s(label, "%.2f fps", fps);
          const bool isSelected{ std::abs(config.camera_fps[cam] - fps) < 0.005 };
          if (ImGui::Selectable(label, isSelected))
          {
            config.camera_fps[cam] = fps;
          }
          if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }
    }

    ImGui::PopID();
  }

  // Ovrvision Pro の入力設定
  ImGui::RadioButton(u8"Ovrvision", &config.input_mode, InputMode::OVRVISION);
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
  ImGui::Combo(u8"特性", &config.ovrvision_property, properties, IM_ARRAYSIZE(properties));

  // リモート入力の設定
  ImGui::RadioButton(u8"リモート", &config.input_mode, InputMode::REMOTE);

  // リモート入力の役割を選択するコンボボックス
  static constexpr char* roles[]{
    u8"単独",
    u8"指導者",
    u8"作業者"
  };
  ImGui::Combo(u8"役割", &config.role, roles, IM_ARRAYSIZE(roles));

  // 外部設定を固定長UIバッファへ移すため、必ず終端できる長さに制限する
  char address[16]{ "0.0.0.0" };
  strncpy_s(address, config.address.c_str(), _TRUNCATE);
  if (ImGui::InputText(u8"アドレス", address, sizeof address))
    config.address = address;
  ImGui::InputInt(u8"ポート", &config.port);

  if (ImGui::CollapsingHeader(u8"送信設定"))
  {
    ImGui::InputInt2(u8"伝送解像度", config.transmit_size.data());
    config.transmit_size[0] = std::max(config.transmit_size[0], 0);
    config.transmit_size[1] = std::max(config.transmit_size[1], 0);
    ImGui::InputDouble(u8"伝送 fps", &config.transmit_fps, 1.0, 5.0, "%.1f");
    config.transmit_fps = std::max(config.transmit_fps, 0.0);
    ImGui::SliderInt(u8"JPEG 品質", &config.transmit_quality, 0, 100);
    ImGui::TextDisabled(u8"解像度 0 x 0 は取得画像のまま送信");
  }

  if (ImGui::CollapsingHeader(u8"平面展開設定"))
  {
    int remoteSize[]{ config.remote_texture_width, config.remote_texture_height };
    if (ImGui::InputInt2(u8"展開解像度", remoteSize))
    {
      config.remote_texture_width = std::max(remoteSize[0], 1);
      config.remote_texture_height = std::max(remoteSize[1], 1);
    }
    ImGui::InputInt(u8"メッシュ分割数", &config.remote_texture_samples);
    config.remote_texture_samples = std::max(config.remote_texture_samples, 4);
    ImGui::InputFloat(u8"水平 FOV", &config.remote_fov_x, 0.01f, 0.1f, "%.3f");
    ImGui::InputFloat(u8"垂直 FOV", &config.remote_fov_y, 0.01f, 0.1f, "%.3f");
    config.remote_fov_x = std::clamp(config.remote_fov_x, 0.001f, 1.56f);
    config.remote_fov_y = std::clamp(config.remote_fov_y, 0.001f, 1.56f);
  }

  ImGui::Text(u8"シェーダ");

  // 設定ファイル由来のパスをImGuiの固定長編集領域へ安全に複製し編集後に std::string へ戻す
  char vertex_shader[MAX_PATH]{ "" };
  strncpy_s(vertex_shader, config.vertex_shader.c_str(), _TRUNCATE);
  if (ImGui::InputText(u8"頂点", vertex_shader, sizeof vertex_shader))
    config.vertex_shader = vertex_shader;
  char fragment_shader[MAX_PATH]{ "" };
  strncpy_s(fragment_shader, config.fragment_shader.c_str(), _TRUNCATE);
  if (ImGui::InputText(u8"画素", fragment_shader, sizeof fragment_shader))
    config.fragment_shader = fragment_shader;

  if (ImGui::Button(u8"設定")) app.selectInput();

  // 入力設定ウィンドウの設定終了
  ImGui::End();
}

//
// 起動時設定ウィンドウ
//
void Menu::startupWindow()
{
  constexpr GLsizei w{ 166 }, h{ 220 };
  const auto& size{ window.getSize() };
  const float x{ (size[0] - w) * 0.5f }, y{ (size[1] - h) * 0.5f };
  ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Appearing);
  ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Appearing);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);

  ImGui::Begin(u8"起動時設定", &showStartupWindow);
  ImGui::Checkbox("Quad Buffer", &quadbuffer);
  ImGui::Checkbox("Full Screen", &fullscreen);

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
    std::swap(config.display_secondary, secondary);
    std::swap(config.display_fullscreen, fullscreen);
    std::swap(config.display_quadbuffer, quadbuffer);
    std::swap(config.local_share_size, memorysize[0]);
    std::swap(config.remote_share_size, memorysize[1]);

    // 設定ファイルを保存する
    if (saveConfig() <= 0)
    {
      // 保存に失敗したかキャンセルしたので元の値を復帰する
      config.display_secondary = secondary;
      config.display_fullscreen = fullscreen;
      config.display_quadbuffer = quadbuffer;
      config.local_share_size = memorysize[0];
      config.remote_share_size = memorysize[1];
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
        secondary = config.display_secondary;
        fullscreen = config.display_fullscreen;
        quadbuffer = config.display_quadbuffer;
        memorysize[0] = config.local_share_size;
        memorysize[1] = config.remote_share_size;
      }

      // 終了
      else if (ImGui::MenuItem(u8"終了")) window.setClose();

      // File メニュー修了
      ImGui::EndMenu();
    }

    // Window メニュー
    if (ImGui::BeginMenu(u8"ウィンドウ"))
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
Menu::Menu(GgApp& app, GgApp::Window& window, Attitude& attitude, Config& config)
  : app{ app }
  , window { window }
  , attitude{ attitude }
  , config{ config }
{
  NFD_Init();
}

//
// 設定ファイルの描画側への反映結果を通知する
//
void Menu::finishConfigReload(bool status)
{
  // 描画オブジェクトを構築できた場合だけ候補設定を確定する
  if (status && pendingConfig) config = std::move(*pendingConfig);

  pendingConfig.reset();
  showNodataWindow = !status;
}

// キャッシュを更新するヘルパー関数
void Menu::updateCameraMenuCache(int cam)
{
  auto& cache{ cameraMenuCache[cam] };
  const auto& devices{ CameraCapabilities::getDeviceList() };
  int currentDevice{ config.camera_id[cam] };

  // デバイスが無効な場合はクリアして終了
  if (currentDevice < 0 || currentDevice >= devices.size())
  {
    cache.lastDeviceId = currentDevice;
    cache.capabilities.clear();
    cache.codecs.clear();
    cache.resolutions.clear();
    cache.frameRates.clear();
    cache.lastCodec = "";
    cache.lastResolution = "";
    return;
  }

  // デバイスが変わったとき
  if (cache.lastDeviceId != currentDevice)
  {
    cache.lastDeviceId = currentDevice;
    CameraCapabilities::getCapabilities(currentDevice, cache.capabilities);
    cache.lastCodec = "";
    cache.lastResolution = "";

    // コーデックリスト構築
    cache.codecs.clear();
    for (const auto& capability : cache.capabilities)
    {
      const auto& name{ capability.codec };
      if (!name.empty() && std::find(cache.codecs.begin(), cache.codecs.end(), name) == cache.codecs.end())
      {
        cache.codecs.push_back(name);
      }
    }

    // もし現在の config コーデックがなければ最初のコーデックをセット
    if (std::find(cache.codecs.begin(), cache.codecs.end(), config.camera_codec[cam]) == cache.codecs.end())
    {
      if (!cache.codecs.empty()) config.camera_codec[cam] = cache.codecs[0];
      else config.camera_codec[cam] = "";
    }
  }

  // コーデックが変わったとき
  if (cache.lastCodec != config.camera_codec[cam])
  {
    cache.lastCodec = config.camera_codec[cam];
    cache.lastResolution = "";
    cache.resolutions.clear();

    for (const auto& capability : cache.capabilities)
    {
      if (capability.codec == cache.lastCodec)
      {
        char buf[32];
        sprintf_s(buf, "%d x %d", capability.width, capability.height);
        std::string resStr(buf);
        if (std::find(cache.resolutions.begin(), cache.resolutions.end(), resStr) == cache.resolutions.end())
        {
          cache.resolutions.push_back(resStr);
        }
      }
    }

    // もし現在の config 解像度がなければ最初の解像度をセット
    if (std::find(cache.resolutions.begin(), cache.resolutions.end(), config.camera_resolution[cam]) == cache.resolutions.end())
    {
      if (!cache.resolutions.empty()) config.camera_resolution[cam] = cache.resolutions[0];
      else config.camera_resolution[cam] = "";
    }
  }

  // 解像度が変わったとき、そのコーデック・解像度で利用できる FPS を列挙する
  if (cache.lastResolution != config.camera_resolution[cam])
  {
    cache.lastResolution = config.camera_resolution[cam];
    cache.frameRates.clear();

    int width{ 0 }, height{ 0 };
    sscanf_s(cache.lastResolution.c_str(), "%d x %d", &width, &height);
    for (const auto& capability : cache.capabilities)
    {
      if (capability.codec == cache.lastCodec
        && capability.width == width && capability.height == height)
      {
        const auto found{ std::find_if(cache.frameRates.begin(), cache.frameRates.end(),
          [&capability](double fps) { return std::abs(fps - capability.fps) < 0.005; }) };
        if (found == cache.frameRates.end()) cache.frameRates.push_back(capability.fps);
      }
    }
    std::sort(cache.frameRates.begin(), cache.frameRates.end(), std::greater<double>());

    const auto selected{ std::find_if(cache.frameRates.begin(), cache.frameRates.end(),
      [this, cam](double fps) { return std::abs(fps - config.camera_fps[cam]) < 0.005; }) };
    if (selected == cache.frameRates.end())
    {
      config.camera_fps[cam] = cache.frameRates.empty() ? 0.0 : cache.frameRates.front();
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


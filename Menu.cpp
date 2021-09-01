//
// ���j���[
//
#include "Menu.h"

// �l�b�g���[�N�֘A�̏���
#include "Network.h"

// �J�����֘A�̏���
#include "CamCv.h"
#include "CamOv.h"
#include "CamImage.h"
#include "CamRemote.h"

// Dear ImGui
#include "imgui.h"

// Native File Dialog
#include "nfd.h"
#pragma comment(lib, "nfd.lib")

// �W�����C�u����
#include <cstdlib>

//
// �ݒ�t�@�C���̓ǂݍ���
//
int loadConfig()
{
  // �t�@�C���p�X�̎擾��
  nfdchar_t* openPath{ nullptr };

  // �_�C�A���O���J��
  static constexpr nfdfilteritem_t filter[]{ {"JSON", "json"}, {"TEXT", "txt"} };
  const nfdresult_t result{ NFD_OpenDialog(&openPath,
    filter, static_cast<nfdfiltersize_t>(std::size(filter)),
    nullptr) };

  // �t�@�C���p�X���擾�ł�����
  if (result == NFD_OKAY)
  {
    // �ݒ�t�@�C����ǂݍ����
    const bool status{ defaults.load(openPath) };

    // �t�@�C���p�X�Ɏg�������������������
    NFD_FreePath(openPath);

    // �ݒ�t�@�C���̓ǂݍ��݌��ʂ�Ԃ�
    return status ? 1 : 0;
  }

  // �_�C�A���O���L�����Z�������Ƃ��̓G���[�ɂ��Ȃ�
  return result == NFD_CANCEL ? -1 : 0;
}

//
// �ݒ�t�@�C���̕ۑ�
//
int saveConfig()
{
  // �t�@�C���p�X�̎擾��
  nfdchar_t* savePath{ nullptr };

  // �_�C�A���O���J��
  static constexpr nfdfilteritem_t filter[]{ {"JSON", "json"} };
  const nfdresult_t result{ NFD_SaveDialog(&savePath,
    filter, static_cast<nfdfiltersize_t>(std::size(filter)),
    nullptr, defaults.config_file.c_str()) };

  // �t�@�C���p�X���擾�ł�����
  if (result == NFD_OKAY)
  {
    // �ݒ�t�@�C����ǂݍ����
    const bool status{ defaults.save(savePath) };

    // �t�@�C���p�X�Ɏg�������������������
    NFD_FreePath(savePath);

    // �ݒ�t�@�C���̓ǂݍ��݌��ʂ�Ԃ�
    return status ? 1 : 0;
  }

  // �_�C�A���O���L�����Z�������Ƃ��̓G���[�ɂ��Ȃ�
  return result == NFD_CANCEL ? -1 : 0;
}

//
// �t�@�C���p�X�̎擾
//
void getFilePath(std::string& path, const nfdfilteritem_t* filter)
{
  // �t�@�C���_�C�A���O���瓾��p�X
  nfdchar_t* filepath{ nullptr };

  // �t�@�C���_�C�A���O���J��
  if (NFD_OpenDialog(&filepath, filter, 1, nullptr) == NFD_OKAY) path = filepath;
}

//
// �f�[�^�ǂݍ��݃G���[�\���E�B���h�E
//
void Menu::nodataWindow()
{
  constexpr int w{ 238 }, h{ 92 };
  const auto& size{ window.getSize() };
  const float x{ (size[0] - w) * 0.5f }, y{ (size[1] - h) * 0.5f };
  ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Appearing);
  ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Appearing);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);

  ImGui::Begin(u8"�G���[", &showNodataWindow);
  ImGui::Text(u8"�t�@�C���̓ǂݏ����Ɏ��s���܂���");
  if (ImGui::Button(u8"����")) showNodataWindow = false;
  ImGui::End();
}

//
// �\���ݒ�E�B���h�E
//
void Menu::displayWindow()
{
  ImGui::SetNextWindowPos(ImVec2(2, 28), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(178, 422), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);

  ImGui::Begin(u8"�\���ݒ�", &showDisplayWindow);
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
  ImGui::Checkbox(u8"�~���[�\��", &window.showMirror);
  ImGui::Checkbox(u8"�V�[���\��", &window.showScene);
  if (ImGui::SliderFloat(u8"�O����", &defaults.display_near, 0.01f, defaults.display_far))
    window.update();
  if (ImGui::SliderFloat(u8"�����", &defaults.display_far, defaults.display_near, 10.0f))
    window.update();

  ImGui::End();
}

//
// �p���ݒ�E�B���h�E
//
void Menu::attitudeWindow()
{
  ImGui::SetNextWindowPos(ImVec2(182, 28), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(218, 326), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);

  ImGui::Begin(u8"�p���ݒ�", &showAttitudeWindow);
  ImGui::Text(u8"�O�i");
  ImGui::InputFloat3(u8"�ʒu", attitude.position.data(), "%6.2f");
  if (ImGui::SliderInt(u8"�Y�[����", attitude.foreAdjust.data(), -99, 99))
    window.update();
  if (ImGui::SliderInt(u8"����", &attitude.parallax, -99, 99))
    window.update();
  ImGui::Text(u8"�w�i");
  if (ImGui::SliderInt(u8"�œ_����", attitude.backAdjust.data(), -99, 99))
    window.update();
  if (ImGui::SliderInt(u8"�Ԋu", &attitude.offset, -99, 99))
    window.update();
  if (ImGui::SliderInt2(u8"��p", &attitude.circleAdjust[0], -99, 99))
    window.updateCircle();
  if (ImGui::SliderInt2(u8"���S", &attitude.circleAdjust[2], -99, 99))
    window.updateCircle();
  if (ImGui::Button(u8"��"))
    window.reset();
  ImGui::End();
}

//
// ���͐ݒ�E�B���h�E
//
void Menu::inputWindow()
{
  // ���͐ݒ�E�B���h�E
  ImGui::SetNextWindowPos(ImVec2(402, 28), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(294, 632), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);
  ImGui::Begin(u8"���͐ݒ�", &showInputWindow);

  ImGui::RadioButton(u8"�Î~�摜", &defaults.input_mode, InputMode::IMAGE);
  static const nfdfilteritem_t image_filter[]{ "Images", "png,jpg,jpeg,jfif,bmp,dib" };
  if (ImGui::Button(u8"���摜�t�@�C��"))
    getFilePath(defaults.camera_image[camL], image_filter);
  ImGui::SameLine();
  ImGui::Text(defaults.camera_image[camL].c_str());
  if (ImGui::Button(u8"�E�摜�t�@�C��"))
    getFilePath(defaults.camera_image[camR], image_filter);
  ImGui::SameLine();
  ImGui::Text(defaults.camera_image[camR].c_str());

  ImGui::RadioButton(u8"���摜", &defaults.input_mode, InputMode::MOVIE);
  static const nfdfilteritem_t movie_filter[]{ "Movies", "mp4,m4v,mov,avi,wmv,ogg" };
  if (ImGui::Button(u8"������t�@�C��"))
    getFilePath(defaults.camera_movie[camL], movie_filter);
  ImGui::SameLine();
  ImGui::Text(defaults.camera_movie[camL].c_str());
  if (ImGui::Button(u8"�E����t�@�C��"))
    getFilePath(defaults.camera_movie[camR], movie_filter);
  ImGui::SameLine();
  ImGui::Text(defaults.camera_movie[camR].c_str());

  ImGui::RadioButton(u8"�J����", &defaults.input_mode, InputMode::CAMERA);
  ImGui::InputInt(u8"���J�����ԍ�", &defaults.camera_id[camL]);
  ImGui::InputInt(u8"�E�J�����ԍ�", &defaults.camera_id[camR]);
  ImGui::InputInt2(u8"�J������f��", defaults.camera_size.data());

  ImGui::RadioButton(u8"Ovrvision", &defaults.input_mode, InputMode::OVRVISION);
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
  ImGui::Combo(u8"�v���p�e�B", &defaults.ovrvision_property, items, IM_ARRAYSIZE(items));

  ImGui::RadioButton(u8"RealSense", &defaults.input_mode, InputMode::REALSENSE);

  ImGui::RadioButton(u8"�����[�g", &defaults.input_mode, InputMode::REMOTE);
  char address[16]{ "0.0.0.0" };
  strcpy(address, defaults.address.c_str());
  if (ImGui::InputText(u8"IP Address", address, sizeof address))
    defaults.address = address;
  ImGui::InputInt(u8"Port", &defaults.port);

  ImGui::Text(u8"�V�F�[�_");
  char vertex_shader[MAX_PATH]{ "" };
  strcpy(vertex_shader, defaults.vertex_shader.c_str());
  if (ImGui::InputText(u8"Vertex", vertex_shader, sizeof vertex_shader))
    defaults.vertex_shader = vertex_shader;
  char fragment_shader[MAX_PATH]{ "" };
  strcpy(fragment_shader, defaults.fragment_shader.c_str());
  if (ImGui::InputText(u8"Fragment", fragment_shader, sizeof fragment_shader))
    defaults.fragment_shader = fragment_shader;

  if (ImGui::Button(u8"�ݒ�")) app->selectInput();

  // ���͐ݒ�E�B���h�E�̐ݒ�I��
  ImGui::End();
}

#if 0
//
// �O�i�ݒ�E�B���h�E
//
void Menu::objectWindow()
{
  // Object �ݒ�E�B���h�E
  ImGui::SetNextWindowPos(ImVec2(4, 30), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(336, 168), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);
  ImGui::Begin(u8"�\���ݒ�", &showObjectWindow);

  // �\������
  int* type{ const_cast<int*>(&develop->type) };
  ImGui::RadioButton(u8"�c�u��", type, octatech::DevelopShader::PORTRAIT);
  ImGui::SameLine();
  ImGui::RadioButton(u8"�V�E�n��", type, octatech::DevelopShader::TOP_BOTTOM);
  ImGui::SameLine();
  ImGui::RadioButton(u8"���u��", type, octatech::DevelopShader::LANDSCAPE);
  ImGui::RadioButton(u8"�V��", type, octatech::DevelopShader::TOP);
  ImGui::SameLine();
  ImGui::RadioButton(u8"����", type, octatech::DevelopShader::BODY);
  ImGui::SameLine();
  ImGui::RadioButton(u8"�n��", type, octatech::DevelopShader::BOTTOM);
  window.selectInterface(*type);

  // �e�N�X�`�������s�[�g���邩
  ImGui::SameLine();
  if (ImGui::Checkbox(u8"�摜����", &repeat)) image.setRepeat(repeat);

  // �}�`�̃X�P�[��
  auto& scale{ image.config.scale };
  if (ImGui::DragFloat2(u8"�h�����T�C�Y", scale.data(), 0.01f, 0.0f, 20.0f, "%.3f"))
    develop->setScale(scale[0], scale[1], scale[2] = scale[0]);

  // �}�`�ɓ��e����e�N�X�`���̓��e���S
  auto& origin{ image.config.origin };
  if (ImGui::DragFloat2(u8"�J�����ʒu", origin.data(), 0.01f, -10.0f, 10.0f, "%.3f"))
    origin[2] = 0.0f;

  // Object �ݒ�E�B���h�E�̐ݒ�I��
  ImGui::End();
}

//
// �g�����E�B���h�E
//
void Menu::trimWindow()
{
  // ImGui �̃t���[���ɓ�ڂ� ImGui �̃E�B���h�E��`��
  ImGui::SetNextWindowPos(ImVec2(344, 30), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(380, 136), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);
  ImGui::Begin(u8"���[�r�[�؏o��", &showTrimWindow);

  // ���[�r�[�t�@�C���̍Đ��J�n�ʒu�ƏI���ʒu
  double in(camera.in), out(camera.out), start(0.0), end(camera.getFrames());
  if (ImGui::SliderScalar(u8"�J�n�_", ImGuiDataType_Double, &in, &start, &end, "%.0f")
    && in >= start && in <= camera.out) camera.in = in;
  if (ImGui::SliderScalar(u8"�I���_", ImGuiDataType_Double, &out, &start, &end, "%.0f")
    && out >= camera.in && out <= end) camera.out = out;

  // �i��
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

  // Trimming �E�B���h�E�̐ݒ�I��
  ImGui::End();
}
#endif

//
// �N�����ݒ�E�B���h�E
//
void Menu::startupWindow()
{
  constexpr GLsizei w{ 212 }, h{ 236 };
  const auto& size{ window.getSize() };
  const float x{ (size[0] - w) * 0.5f }, y{ (size[1] - h) * 0.5f };
  ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Appearing);
  ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Appearing);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);

  ImGui::Begin(u8"�N�����ݒ�", &showStartupWindow);
  ImGui::Checkbox(u8"�N�A�b�h�o�b�t�@�X�e���I", &quadbuffer);
  ImGui::Checkbox(u8"�t���X�N���[��", &fullscreen);

  ImGui::Text(u8"�Z�J���_�����j�^�̔ԍ�");
  ImGui::InputInt("", &secondary);
  secondary = std::max(secondary, 0);

  ImGui::Text(u8"���L�������̃T�C�Y");
  ImGui::InputInt2("", memorysize);
  memorysize[0] = std::max(memorysize[0], localShareSize);
  memorysize[1] = std::max(memorysize[1], remoteShareSize);

  if (ImGui::Button(u8"�ݒ��ۑ�"))
  {
    // �ۑ��Ɏ��s�����Ƃ��̂��߂Ɍ��̒l������Ă���
    std::swap(defaults.display_secondary,secondary);
    std::swap(defaults.display_fullscreen, fullscreen);
    std::swap(defaults.display_quadbuffer, quadbuffer);
    std::swap(defaults.local_share_size, memorysize[0]);
    std::swap(defaults.remote_share_size, memorysize[1]);

    // �ݒ�t�@�C����ۑ�����
    if (saveConfig() <= 0)
    {
      // �ۑ��Ɏ��s�������L�����Z�������̂Ō��̒l�𕜋A����
      defaults.display_secondary = secondary;
      defaults.display_fullscreen = fullscreen;
      defaults.display_quadbuffer = quadbuffer;
      defaults.local_share_size = memorysize[0];
      defaults.remote_share_size = memorysize[1];
    }
    else
    {
      // ���̃E�B���h�E�����
      showStartupWindow = false;
    }
  }
  ImGui::SameLine();
  if (ImGui::Button(u8"�L�����Z��"))
  {
    // ���̃E�B���h�E�����
    showStartupWindow = false;
  }
  ImGui::End();
}

//
// ���j���[�o�[
//
void Menu::menuBar()
{
  // ���C�����j���[�o�[
  if (ImGui::BeginMainMenuBar())
  {
    // File ���j���[
    if (ImGui::BeginMenu(u8"�t�@�C��"))
    {
      // �t�@�C���_�C�A���O���瓾��p�X
      nfdchar_t* filepath(NULL);

      // �ݒ�t�@�C�����J��
      if (ImGui::MenuItem(u8"�ݒ�t�@�C����ǂݍ���"))
        showNodataWindow = !loadConfig();

      // �ݒ�t�@�C����ۑ�����
      else if (ImGui::MenuItem(u8"�ݒ�t�@�C����ۑ�����"))
        showNodataWindow = !saveConfig();

      // �N�����ݒ�
      else if (ImGui::MenuItem(u8"�N�����ݒ�", nullptr, &showStartupWindow))
      {
        // �N�����ݒ�̃R�s�[������Ă���
        secondary = defaults.display_secondary;
        fullscreen = defaults.display_fullscreen;
        quadbuffer = defaults.display_quadbuffer;
        memorysize[0] = defaults.local_share_size;
        memorysize[1] = defaults.remote_share_size;
      }

      // �I��
      else if (ImGui::MenuItem(u8"�I��")) window.setClose();

      // File ���j���[�C��
      ImGui::EndMenu();
    }

    // Window ���j���[
    else if (ImGui::BeginMenu(u8"�E�B���h�E"))
    {
      // �\���ݒ�E�B���h�E���J����
      ImGui::MenuItem(u8"�\���ݒ�", NULL, &showDisplayWindow);

      // �p���ݒ�E�B���h�E���J����
      ImGui::MenuItem(u8"�p���ݒ�", NULL, &showAttitudeWindow);

      // ���͐ݒ�E�B���h�E���J����
      ImGui::MenuItem(u8"���͐ݒ�", NULL, &showInputWindow);

      // Window ���j���[�I��
      ImGui::EndMenu();
    }

    // ���C�����j���[�o�[�I��
    ImGui::EndMainMenuBar();
  }
}

//
// �R���X�g���N�^
//
Menu::Menu(GgApp* app, Window& window, Scene& scene, Attitude& attitude)
  : app{ app }
  , window { window }
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
// �f�X�g���N�^
//
Menu::~Menu()
{
  NFD_Quit();
}

//
// ���j���[�̕\��
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

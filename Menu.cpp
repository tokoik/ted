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

// �V�[���O���t
#include "Scene.h"

// Dear ImGui
#include "imgui.h"

// Native File Dialog
#include "nfd.h"
#pragma comment(lib, "nfd.lib")

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
// �f�[�^�ǂݍ��݃G���[�\���E�B���h�E
//
void Menu::nodataWindow()
{
  const auto& size{ window.getSize() };
  constexpr GLsizei w{ 260 }, h{ 98 };
  ImGui::SetNextWindowPos(ImVec2((size[0] - w) * 0.5f, (size[1] - h) * 0.5f));
  ImGui::SetNextWindowSize(ImVec2(w, h));
  ImGui::SetNextWindowCollapsed(false);

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
  ImGui::SetNextWindowPos(ImVec2(4, 32), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(192, 258), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);

  ImGui::Begin(u8"�\���ݒ�", &showDisplayWindow);
  ImGui::Text("Frame rate: %6.2f fps", ImGui::GetIO().Framerate);
  if (ImGui::RadioButton(u8"Monocular", &defaults.display_mode, MONOCULAR)) window.stopOculus();
  if (ImGui::RadioButton(u8"Interlace", &defaults.display_mode, INTERLACE)) window.stopOculus();
  if (ImGui::RadioButton(u8"Top and Bottom", &defaults.display_mode, TOP_AND_BOTTOM)) window.stopOculus();
  if (ImGui::RadioButton(u8"Side by Side", &defaults.display_mode, SIDE_BY_SIDE)) window.stopOculus();
  if (ImGui::RadioButton(u8"Quad Buffer", &defaults.display_mode, QUADBUFFER)) window.stopOculus();
  int mode{ defaults.display_mode };
  if (ImGui::RadioButton("Oculus", &mode, OCULUS) && window.startOculus()) defaults.display_mode = mode;
  ImGui::End();
}

//
// �J�����ݒ�E�B���h�E
//
void Menu::cameraWindow()
{
  ImGui::SetNextWindowPos(ImVec2(4, 294), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(192, 258), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);

  ImGui::Begin(u8"�J�����ݒ�", &showCameraWindow);
  if (ImGui::SliderFloat(u8"�O����", &defaults.display_near, 0.01f, defaults.display_far))
    window.update();
  if (ImGui::SliderFloat(u8"�����", &defaults.display_far, defaults.display_near, 10.0f))
    window.update();
  if (ImGui::SliderFloat(u8"�Y�[��", &defaults.display_zoom, 1.0f, 10.0f))
    window.update();
  ImGui::End();
}

#if 0
//
// �V�[���ݒ�E�B���h�E
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
// ���͐ݒ�E�B���h�E
//
void Menu::inputWindow(const ShaderList& list, std::vector<DeviceData>::const_iterator& device)
{
  // ���͐ݒ�E�B���h�E
  ImGui::SetNextWindowPos(ImVec2(4, 470), ImGuiCond_Once);
  ImGui::SetNextWindowSize(ImVec2(336, 296), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);
  ImGui::Begin(u8"���͐ݒ�", &showInputWindow);

  // �J�����̐ݒ�̕ʖ�������Ă���
  auto& size(image.config.size);
  auto& rate(image.config.rate);
  auto& fourcc(image.config.fourcc);

  // �f�o�C�X�ԍ��̑I��
  if (ImGui::BeginCombo(u8"���͌�", devName))
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

  // �L���v�`������T�C�Y�ƃt���[�����[�g
  ImGui::InputInt2(u8"�T�C�Y", size.data());
  ImGui::InputInt(u8"���[�g", &rate);

  // �����ƘI��
  if (ImGui::InputInt(u8"����", &gain)) camera.setGain(gain);
  if (ImGui::InputInt(u8"�I��", &exposure)) camera.setExposure(exposure);

  // ���݂̃R�[�f�b�N�̌��o�������
  char defaultCodec[sizeof fourcc + 3];
  memcpy(defaultCodec + 1, fourcc.data(), sizeof fourcc);
  defaultCodec[0] = isprint(defaultCodec[1]) ? '(' : 0;
  defaultCodec[sizeof fourcc + 1] = ')';
  defaultCodec[sizeof fourcc + 2] = 0;

  // ���݂̃R�[�f�b�N�̌��o�����������R�[�f�b�N�̈ꗗ
  static const char* const codecs[]{ defaultCodec, "MJPG", "H264", "YUY2", "NV12" };

  // �R�[�f�b�N��I������
  int encode(0);
  if (ImGui::Combo(u8"������", &encode, codecs, sizeof codecs / sizeof codecs[0]) && encode > 0)
  {
    // �I�������R�[�f�b�N��ۑ�����
    memcpy(fourcc.data(), codecs[encode], sizeof fourcc);
  }

  // �f�o�C�X�v���t�@�����X��I������
  ImGui::RadioButton("ANY", &pref, cv::CAP_ANY);
  ImGui::SameLine();
  ImGui::RadioButton("DSHOW", &pref, cv::CAP_DSHOW);
  ImGui::SameLine();
  ImGui::RadioButton("MSMF", &pref, cv::CAP_MSMF);

  // �f�o�C�X�ݒ�̕ύX
  if (ImGui::Button(u8"�ݒ�"))
  {
    // �L���v�`�����~
    camera.stop();

    // �J���������
    camera.close();

    // �V�����ݒ�Ńf�o�C�X���J��
    if (
      (device->id >= 0 && camera.open(device->id, size[0], size[1], rate, fourcc.data(), pref))
      ||
      (!device->name.empty() && camera.open(device->name, size[0], size[1], rate, fourcc.data(), pref))
      )
    {
      // �؂�ւ����f�o�C�X�̓��������o��
      size[0] = camera.getWidth();
      size[1] = camera.getHeight();
      rate = static_cast<int>(camera.getFps());
      camera.getCodec(fourcc.data());

      // ���܂Ŏg���Ă����e�N�X�`�����̂ĂĐV�����e�N�X�`�������
      image.create(camera.getWidth(), camera.getHeight());

      // �L���v�`�����ĊJ
      camera.start();
    }
  }

  // �f�o�C�X�̏�ԕ\��
  if (!camera.running())
  {
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), u8"���͌����g�p�ł��܂���");
  }

  // ���͐ݒ�E�B���h�E�̐ݒ�I��
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
  const auto& size{ window.getSize() };
  constexpr GLsizei w{ 232 }, h{ 252 };
  ImGui::SetNextWindowPos(ImVec2((size[0] - w) * 0.5f, (size[1] - h) * 0.5f));
  ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Once);
  ImGui::SetNextWindowCollapsed(false);

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
      // �J�����ݒ�E�B���h�E���J����
      ImGui::MenuItem(u8"�J�����ݒ�", NULL, &showDisplayWindow);

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
Menu::Menu(Window& window)
  : window{ window }
  , showNodataWindow{ false }
  , showDisplayWindow{ true }
  , showCameraWindow{ true }
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
  if (showCameraWindow) cameraWindow();
  if (showStartupWindow) startupWindow();
  ImGui::Render();
}

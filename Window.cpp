//
// �E�B���h�E�֘A�̏���
//
#include "Window.h"

// Oculus Rift �֘A�̏���
#include "Oculus.h"

// �p��
#include "Attitude.h"

// �V�[���O���t
#include "Scene.h"

// �W�����C�u����
#include <fstream>

// Dear ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// �W���C�X�e�B�b�N�ԍ��̍ő�l
constexpr int maxJoystick{ 4 };

// Oculus Rift �̃Z�b�V����
Oculus* Window::oculus{ nullptr };

//
// �R���X�g���N�^
//
Window::Window(int width, int height, const char *title, GLFWmonitor *monitor, GLFWwindow *share)
  : window{ nullptr }
  , camera{ nullptr }
  , showScene{ true }
  , showMirror{ true }
  , showMenu{ true }
  , key{ GLFW_KEY_UNKNOWN }
  , joy{ -1 }
  , zoom{ 1.0f }
  , focal{ 1.0f }
  , parallax{ defaultParallax }
  , offset{ 0.0f }
{
  // �ŏ��̃C���X�^���X�ň�x�������s
  static bool firstTime{ true };
  if (firstTime)
  {
    // �N���b�h�o�b�t�@�X�e���I���[�h��L���ɂ���
    if (defaults.display_quadbuffer)
    {
      glfwWindowHint(GLFW_STEREO, GLFW_TRUE);
    }

    // SRGB ���[�h�Ń����_�����O�ł���悤�ɂ���
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    // OpenGL �̃o�[�W�������w�肷��
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Core Profile ��I������
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef IMGUI_VERSION
    // ImGui �̃o�[�W�������`�F�b�N����
    IMGUI_CHECKVERSION();

    // ImGui �̃R���e�L�X�g���쐬����
    ImGui::CreateContext();

    // �v���O�����I�����ɂ� ImGui �̃R���e�L�X�g��j������
    atexit([] { ImGui::DestroyContext(); });
#endif

    // ���s�ς݂̈������
    firstTime = false;
  }

  // GLFW �̃E�B���h�E���J��
  const_cast<GLFWwindow *>(window) = glfwCreateWindow(width, height, title, monitor, share);

  // �E�B���h�E���J����Ȃ�������߂�
  if (!window) return;

  //
  // �����ݒ�
  //

  // �w�b�h�g���b�L���O�̕ϊ��s�������������
  for (auto &m : mo) m = ggIdentity();

  // �ݒ������������
  reset();

  //
  // �\���p�̃E�B���h�E�̐ݒ�
  //

  // ���݂̃E�B���h�E�������Ώۂɂ���
  glfwMakeContextCurrent(window);

  // �Q�[���O���t�B�b�N�X���_�̓s���ɂ��ƂÂ����������s��
  ggInit();

  // �t���X�N���[�����[�h�ł��}�E�X�J�[�\����\������
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  // ���̃C���X�^���X�� this �|�C���^���L�^���Ă���
  glfwSetWindowUserPointer(window, this);

  // �}�E�X�{�^���𑀍삵���Ƃ��̏�����o�^����
  glfwSetMouseButtonCallback(window, mouse);

  // �}�E�X�z�C�[�����쎞�ɌĂяo��������o�^����
  glfwSetScrollCallback(window, wheel);

  // �L�[�{�[�h�𑀍삵�����̏�����o�^����
  glfwSetKeyCallback(window, keyboard);

  // �E�B���h�E�̃T�C�Y�ύX���ɌĂяo��������o�^����
  glfwSetFramebufferSizeCallback(window, resize);

  //
  // �W���C�X�e�B�b�N�̐ݒ�
  //

  // �W���C�X�e�b�N�̗L���𒲂ׂĔԍ������߂�
  for (int i = 0; i < maxJoystick; ++i)
  {
    if (glfwJoystickPresent(i))
    {
      // ���݂���W���C�X�e�B�b�N�ԍ�
      joy = i;

      // �X�e�B�b�N�̒����ʒu�����߂�
      int axesCount;
      const auto *const axes(glfwGetJoystickAxes(joy, &axesCount));

      if (axesCount > 3)
      {
        // �N������̃X�e�B�b�N�̈ʒu����ɂ���
        origin[0] = axes[0];
        origin[1] = axes[1];
        origin[2] = axes[2];
        origin[3] = axes[3];
      }

      break;
    }
  }

  //
  // OpenGL �̐ݒ�
  //

  // �w�i�F
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

#ifdef IMGUI_VERSION
  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 430");

  //
  // ���[�U�C���^�t�F�[�X�̏���
  //
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  //ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can also read multiple fonts and use ImGui::PushFont()/PopFont() to select them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
  // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
  // - Read 'docs/FONTS.txt' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
  //io.Fonts->AddFontDefault();
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
  const ImFont* const font{ io.Fonts->AddFontFromFileTTF("Mplus1-Regular.ttf", 22.0f, NULL, io.Fonts->GetGlyphRangesJapanese()) };
  IM_ASSERT(font != NULL);
#endif

  // ���e�ϊ��s��E�r���[�|�[�g������������
  resize(window, width, height);
}

//
// �f�X�g���N�^
//
Window::~Window()
{
  // �E�B���h�E���J����Ă��Ȃ�������߂�
  if (!window) return;

#ifdef IMGUI_VERSION
  // Shutdown Platform/Renderer bindings
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
#endif

  // �\���p�̃E�B���h�E��j������
  glfwDestroyWindow(window);
}

//
// �C�x���g���擾���ă��[�v���p������Ȃ�^��Ԃ�
//
Window::operator bool()
{
  // �C�x���g�����o��
  glfwPollEvents();

  // �E�B���h�E�����ׂ��Ȃ� false
  if (glfwWindowShouldClose(window)) return false;

#if defined(LEAP_INTERPORATE_FRAME)
  // Leap Motion �̕ϊ��s����X�V����
  Scene::update();
#endif

  // ���x�������ɔ�Ⴓ����
  const auto speedFactor((fabs(attitude.position[2]) + 0.2f));

  //
  // �W���C�X�e�B�b�N�ɂ�鑀��
  //

  // �W���C�X�e�B�b�N���L���Ȃ�
  if (joy >= 0 && defaults.use_controller)
  {
    // �{�^��
    int btnsCount;
    const auto *const btns(glfwGetJoystickButtons(joy, &btnsCount));

    // �X�e�B�b�N
    int axesCount;
    const auto *const axes(glfwGetJoystickAxes(joy, &axesCount));

    // �X�e�B�b�N�̑��x�W��
    const auto axesSpeedFactor(axesSpeedScale * speedFactor);

    // L �{�^���� R �{�^���̏��
    const auto lrButton(btns[4] | btns[5]);

    if (axesCount > 3)
    {
      // ���̂����E�Ɉړ�����
      attitude.position[0] += (axes[0] - origin[0]) * axesSpeedFactor;

      // L �{�^���� R �{�^���𓯎��ɉ����Ă����
      if (lrButton)
      {
        // ���̂�O��Ɉړ�����
        attitude.position[2] += (axes[1] - origin[1]) * axesSpeedFactor;
      }
      else
      {
        // ���̂��㉺�Ɉړ�����
        attitude.position[1] += (axes[1] - origin[1]) * axesSpeedFactor;
      }

      // ���̂����E�ɉ�]����
      attitude.orientation.rotate(ggRotateQuaternion(0.0f, 1.0f, 0.0f, (axes[2] - origin[2]) * axesSpeedFactor));

      // ���̂��㉺�ɉ�]����
      attitude.orientation.rotate(ggRotateQuaternion(1.0f, 0.0f, 0.0f, (axes[3] - origin[3]) * axesSpeedFactor));
    }

    // B, X �{�^���̏��
    const auto parallaxButton(btns[1] - btns[2]);

    // B, X �{�^���ɕω��������
    if (parallaxButton)
    {
      // L �{�^���� R �{�^���𓯎��ɉ����Ă����
      if (lrButton)
      {
        // �w�i�����E�ɉ�]����
        if (parallaxButton > 0)
        {
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[0];
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[0].conjugate();
        }
        else
        {
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[0].conjugate();
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[0];
        }
      }
      else
      {
        // �X�N���[���̊Ԋu�𒲐�����
        offset = offsetDefault + offsetStep * (attitude.offset += parallaxButton);
      }
    }

    // Y, A �{�^���̏��
    const auto zoomButton(btns[3] - btns[0]);

    // Y, A �{�^���ɕω��������
    if (zoomButton)
    {
      // L �{�^���� R �{�^���𓯎��ɉ����Ă����
      if (lrButton)
      {
        // �w�i���㉺�ɉ�]����
        if (zoomButton > 0)
        {
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[1];
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[1].conjugate();
        }
        else
        {
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[1].conjugate();
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[1];
        }
      }
      else
      {
        // �œ_�����𒲐�����
        focal = 1.0f / (1.0f - backFocalStep * (attitude.backAdjust[0] += zoomButton));
      }
    }

    // �\���L�[�̍��E�{�^���̏��
    const auto textureXButton(btns[11] - btns[13]);

    // �\���L�[�̍��E�{�^���ɕω��������
    if (textureXButton)
    {
      // L �{�^���� R �{�^���𓯎��ɉ����Ă����
      if (lrButton)
      {
        // �w�i�ɑ΂��鉡�����̉�p�𒲐�����
        circle[0] = defaults.camera_fov_x + fovStep * (attitude.circleAdjust[0] += textureXButton);
      }
      else
      {
        // �w�i�̉��ʒu�𒲐�����
        circle[2] = defaults.camera_center_x + shiftStep * (attitude.circleAdjust[2] += textureXButton);
      }
    }

    // �\���L�[�̏㉺�{�^���̏��
    const auto textureYButton(btns[10] - btns[12]);

    // �\���L�[�̏㉺�{�^���ɕω��������
    if (textureYButton)
    {
      // L �{�^���� R �{�^���𓯎��ɉ����Ă����
      if (lrButton)
      {
        // �w�i�ɑ΂���c�����̉�p�𒲐�����
        circle[1] = defaults.camera_fov_y + fovStep * (attitude.circleAdjust[1] += textureYButton);
      }
      else
      {
        // �w�i�̏c�ʒu�𒲐�����
        circle[3] = defaults.camera_center_y + shiftStep * (attitude.circleAdjust[3] += textureYButton);
      }
    }

    // �ݒ�����Z�b�g����
    if (btns[7]) reset();
  }

  //
  // �}�E�X�ɂ�鑀��
  //

  // �}�E�X�̈ʒu
  double x, y;

#ifdef IMGUI_VERSION

  if (showMenu)
  {
    // ImGui �̐V�K�t���[�����쐬����
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    // ImGui ���}�E�X���g���Ƃ��� Window �N���X�̃}�E�X�ʒu���X�V���Ȃ�
    if (ImGui::GetIO().WantCaptureMouse) return true;
  }

  // �}�E�X�̌��݈ʒu�𒲂ׂ�
  const ImGuiIO &io(ImGui::GetIO());
  x = io.MousePos.x;
  y = io.MousePos.y;

#else

  // �}�E�X�̈ʒu�𒲂ׂ�
  glfwGetCursorPos(window, &x, &y);

#endif

  // ���{�^���h���b�O
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1))
  {
    // ���̂̈ʒu���ړ�����
    const auto speed(speedScale * speedFactor);
    attitude.position[0] += static_cast<GLfloat>(x - cx) * speed;
    attitude.position[1] += static_cast<GLfloat>(cy - y) * speed;
    cx = x;
    cy = y;
  }

  // �E�{�^���h���b�O
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2))
  {
    // ���̂���]����
    attitude.orientation.motion(static_cast<float>(x), static_cast<float>(y));
  }

<<<<<<< HEAD
  //
  // �L�[�{�[�h�ɂ�鑀��
  //

  // �V�t�g�L�[�̏��
  const auto shiftKey(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT));

  // �R���g���[���L�[�̏��
  const auto ctrlKey(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL));

  // �I���^�l�[�g�L�[�̏��
  const auto altKey(glfwGetKey(window, GLFW_KEY_LEFT_ALT) || glfwGetKey(window, GLFW_KEY_RIGHT_ALT));

  // �E���L�[����
  if (glfwGetKey(window, GLFW_KEY_RIGHT))
  {
    if (altKey)
    {
      // �w�i���E�ɉ�]����
      if (!ctrlKey) attitude.eyeOrientation[camL] *= qrStep[0].conjugate();
      if (!shiftKey) attitude.eyeOrientation[camR] *= qrStep[0].conjugate();
    }
    else if (ctrlKey)
    {
      // �w�i�ɑ΂��鉡�����̉�p���L����
      circle[0] = defaults.camera_fov_x + fovStep * ++attitude.circleAdjust[0];
    }
    else if (shiftKey)
    {
      // �w�i���E�ɂ��炷
      circle[2] = defaults.camera_center_x + shiftStep * ++attitude.circleAdjust[2];
    }
    else if (defaults.display_mode != MONOCULAR)
    {
      // �X�N���[���̊Ԋu���g�傷��
      offset = offsetDefault + offsetStep * ++attitude.offset;
    }
  }

  // �����L�[����
  if (glfwGetKey(window, GLFW_KEY_LEFT))
  {
    if (altKey)
    {
      // �w�i�����ɉ�]����
      if (!ctrlKey) attitude.eyeOrientation[camL] *= qrStep[0];
      if (!shiftKey) attitude.eyeOrientation[camR] *= qrStep[0];
    }
    else if (ctrlKey)
    {
      // �w�i�ɑ΂��鉡�����̉�p�����߂�
      circle[0] = defaults.camera_fov_x + fovStep * --attitude.circleAdjust[0];
    }
    else if (shiftKey)
    {
      // �w�i�����ɂ��炷
      circle[2] = defaults.camera_center_x + shiftStep * --attitude.circleAdjust[2];
    }
    else if (defaults.display_mode != MONOCULAR)
    {
      // �������k������
      offset = offsetDefault + offsetStep * --attitude.offset;
    }
  }

  // ����L�[����
  if (glfwGetKey(window, GLFW_KEY_UP))
  {
    if (altKey)
    {
      // �w�i����ɉ�]����
      if (!ctrlKey) attitude.eyeOrientation[camL] *= qrStep[1];
      if (!shiftKey) attitude.eyeOrientation[camR] *= qrStep[1];
    }
    else if (ctrlKey)
    {
      // �w�i�ɑ΂���c�����̉�p���L����
      circle[1] = defaults.camera_fov_y + fovStep * ++attitude.circleAdjust[1];
    }
    else if (shiftKey)
    {
      // �w�i����ɂ��炷
      circle[3] = defaults.camera_center_y + shiftStep * ++attitude.circleAdjust[3];
    }
    else
    {
      // �œ_���������΂�
      focal = 1.0f / (1.0f - backFocalStep * ++attitude.backAdjust[0]);
    }
  }

  // �����L�[����
  if (glfwGetKey(window, GLFW_KEY_DOWN))
  {
    if (altKey)
    {
      // �w�i�����ɉ�]����
      if (!ctrlKey) attitude.eyeOrientation[camL] *= qrStep[1].conjugate();
      if (!shiftKey) attitude.eyeOrientation[camR] *= qrStep[1].conjugate();
    }
    else if (ctrlKey)
    {
      // �w�i�ɑ΂���c�����̉�p�����߂�
      circle[1] = defaults.camera_fov_y + fovStep * --attitude.circleAdjust[1];
    }
    else if (shiftKey)
    {
      // �w�i�����ɂ��炷
      circle[3] = defaults.camera_center_y + shiftStep * --attitude.circleAdjust[3];
    }
    else
    {
      // �œ_�������k�߂�
      focal = 1.0f / (1.0f - backFocalStep * --attitude.backAdjust[0]);
    }
  }

  // 'P' �L�[�̑���
  if (glfwGetKey(window, GLFW_KEY_P))
  {
    // 'P' �L�[�̑���
    if (glfwGetKey(window, GLFW_KEY_P))
    {
      // �����𒲐�����
      parallax = defaultParallax + parallaxStep * (shiftKey ? --attitude.parallax : ++attitude.parallax);
    }

    // 'Z' �L�[�̑���
    else if (glfwGetKey(window, GLFW_KEY_Z))
    {
      // �Y�[�����𒲐�����
      zoom = 1.0f / (1.0f - zoomStep * (shiftKey ? --attitude.foreAdjust[0] : ++attitude.foreAdjust[0]));
    }

    // �������e�ϊ��s����X�V����
    update();
  }

  // �J�����̐���
  if (camera)
  {
    // 'E' �L�[�̑���
    if (glfwGetKey(window, GLFW_KEY_E))
    {
      if (shiftKey)
      {
        // �I�o��������
        camera->decreaseExposure();
      }
      else
      {
        // �I�o���グ��
        camera->increaseExposure();
      }
    }

    // 'G' �L�[�̑���
    if (glfwGetKey(window, GLFW_KEY_G))
    {
      if (shiftKey)
      {
        // ������������
        camera->decreaseGain();
      }
      else
      {
        // �������グ��
        camera->increaseGain();
      }
    }
  }

=======
>>>>>>> 6a7aec8 (add attitude menu)
  return true;
}

//
// �J���[�o�b�t�@�����ւ��ăC�x���g�����o��
//
//   �E�}�`�̕`��I����ɌĂяo��
//   �E�_�u���o�b�t�@�����O�̃o�b�t�@�̓���ւ����s��
//
void Window::swapBuffers()
{
  // �G���[�`�F�b�N
  ggError();

  // Oculus Rift �g�p��
  if (oculus)
  {
    // �`�悵���t���[���� Oculus Rift �ɓ]������
    oculus->submit();

    // �`�悵���t���[���� Oculus Rift �ɓ]������
    if (showMirror) oculus->submitMirror(width, height);

#ifdef IMGUI_VERSION
    if (showMenu)
    {
      // ���[�U�C���^�t�F�[�X��`�悷��
      ImDrawData *const imDrawData(ImGui::GetDrawData());
      if (imDrawData) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
    }
#endif

    // �c���Ă��� OpenGL �R�}���h�����s����
    glFlush();
  }
  else
  {
#ifdef IMGUI_VERSION
    if (showMenu)
    {
      // ���[�U�C���^�t�F�[�X��`�悷��
      ImDrawData *const imDrawData(ImGui::GetDrawData());
      if (imDrawData) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
    }
#endif

    // �J���[�o�b�t�@�����ւ���
    glfwSwapBuffers(window);
  }
}

//
// �E�B���h�E�̃T�C�Y�ύX���̏���
//
//   �E�E�B���h�E�̃T�C�Y�ύX���ɃR�[���o�b�N�֐��Ƃ��ČĂяo�����
//   �E�E�B���h�E�̍쐬���ɂ͖����I�ɌĂяo��
//
void Window::resize(GLFWwindow *window, int width, int height)
{
  // ���̃C���X�^���X�� this �|�C���^�𓾂�
  Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

  if (instance)
  {
    // �E�B���h�E�̃T�C�Y��ۑ�����
    instance->size[0] = width;
    instance->size[1] = height;

    // Oculus Rift �g�p���ȊO
    if (!instance->oculus)
    {
      if (defaults.display_mode == INTERLACE)
      {
        // VR ���̃f�B�X�v���C�ł͕\���̈�̉��������r���[�|�[�g�ɂ���
        width /= 2;

        // VR ���̃f�B�X�v���C�̃A�X�y�N�g��͕\���̈�̉������ɂȂ�
        instance->aspect = static_cast<GLfloat>(width) / static_cast<GLfloat>(height);
      }
      else
      {
        // ���T�C�Y��̃f�B�X�v���C�̂̃A�X�y�N�g������߂�
        instance->aspect = defaults.display_aspect
          ? defaults.display_aspect
          : static_cast<GLfloat>(width) / static_cast<GLfloat>(height);

        // ���T�C�Y��̃r���[�|�[�g��ݒ肷��
        switch (defaults.display_mode)
        {
        case SIDE_BY_SIDE:

          // �E�B���h�E�̉��������r���[�|�[�g�ɂ���
          width /= 2;
          break;

        case TOP_AND_BOTTOM:

          // �E�B���h�E�̏c�������r���[�|�[�g�ɂ���
          height /= 2;
          break;

        default:
          break;
        }
      }
    }

    // �r���[�|�[�g (Oculus Rift �̏ꍇ�̓~���[�\���p�̃E�B���h�E) �̑傫����ۑ����Ă���
    instance->width = width;
    instance->height = height;

    // �w�i�`��p�̃��b�V���̏c���̊i�q�_�������߂�
    instance->samples[0] = static_cast<GLsizei>(sqrt(instance->aspect * defaults.camera_texture_samples));
    instance->samples[1] = defaults.camera_texture_samples / instance->samples[0];

    // �w�i�`��p�̃��b�V���̏c���̊i�q�Ԋu�����߂�
    instance->gap[0] = 2.0f / static_cast<GLfloat>(instance->samples[0] - 1);
    instance->gap[1] = 2.0f / static_cast<GLfloat>(instance->samples[1] - 1);

    // �������e�ϊ��s������߂�
    instance->update();

    // �g���b�N�{�[�������͈̔͂�ݒ肷��
    attitude.orientation.region(static_cast<float>(width) / angleScale, static_cast<float>(height) / angleScale);
  }
}

//
// �}�E�X�{�^���𑀍삵���Ƃ��̏���
//
//   �E�}�E�X�{�^�����������Ƃ��ɃR�[���o�b�N�֐��Ƃ��ČĂяo�����
//
void Window::mouse(GLFWwindow *window, int button, int action, int mods)
{
#ifdef IMGUI_VERSION
  // ImGui ���}�E�X���g���Ƃ��� Window �N���X�̃}�E�X�ʒu���X�V���Ȃ�
  if (ImGui::GetIO().WantCaptureMouse) return;
#endif

  // ���̃C���X�^���X�� this �|�C���^�𓾂�
  Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

  if (instance)
  {
    // �}�E�X�̌��݈ʒu�����o��
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    switch (button)
    {
    case GLFW_MOUSE_BUTTON_1:

      // ���{�^�������������̏���
      if (action)
      {
        // �h���b�O�J�n�ʒu��ۑ�����
        instance->cx = x;
        instance->cy = y;
      }
      break;

    case GLFW_MOUSE_BUTTON_2:

      // �E�{�^�������������̏���
      if (action)
      {
        // �g���b�N�{�[�������J�n
        attitude.orientation.begin(static_cast<float>(x), static_cast<float>(y));
      }
      else
      {
        // �g���b�N�{�[�������I��
        attitude.orientation.end(static_cast<float>(x), static_cast<float>(y));
      }
      break;

    case GLFW_MOUSE_BUTTON_3:

      // ���{�^�������������̏���
      break;

    default:
      break;
    }
  }
}

//
// �}�E�X�z�C�[�����쎞�̏���
//
//   �E�}�E�X�z�C�[���𑀍삵�����ɃR�[���o�b�N�֐��Ƃ��ČĂяo�����
//
void Window::wheel(GLFWwindow *window, double x, double y)
{
#ifdef IMGUI_VERSION
  // ImGui ���}�E�X���g���Ƃ��� Window �N���X�̃}�E�X�z�C�[���̉�]�ʂ��X�V���Ȃ�
  if (ImGui::GetIO().WantCaptureMouse) return;
#endif

  // ���̃C���X�^���X�� this �|�C���^�𓾂�
  Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

  if (instance)
  {
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT))
    {
      // �Y�[�����𒲐�����
      attitude.foreAdjust[0] += static_cast<int>(y);

      // �������e�ϊ��s����X�V����
      instance->update();
    }
    else
    {
      // ���̂�O��Ɉړ�����
      const GLfloat advSpeed((fabs(attitude.position[2]) * 5.0f + 1.0f) * wheelYStep * static_cast<GLfloat>(y));
      attitude.position[2] += advSpeed;
    }
  }
}

//
// �L�[�{�[�h���^�C�v�������̏���
//
//   �D�L�[�{�[�h���^�C�v�������ɃR�[���o�b�N�֐��Ƃ��ČĂяo�����
//
void Window::keyboard(GLFWwindow *window, int key, int scancode, int action, int mods)
{
#ifdef IMGUI_VERSION
  // ImGui ���L�[�{�[�h���g���Ƃ��̓L�[�{�[�h�̏������s��Ȃ�
  if (ImGui::GetIO().WantCaptureKeyboard) return;
#endif

  // ���̃C���X�^���X�� this �|�C���^�𓾂�
  Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

  if (instance)
  {
    if (action != GLFW_RELEASE)
    {
      // �V�t�g�L�[�̏��
      const auto shiftKey(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)
        || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT));

      // �R���g���[���L�[�̏��
      const auto ctrlKey(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)
        || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL));

      // �I���^�l�[�g�L�[�̏��
      const auto altKey(glfwGetKey(window, GLFW_KEY_LEFT_ALT)
        || glfwGetKey(window, GLFW_KEY_RIGHT_ALT));

      // �Ō�Ƀ^�C�v�����L�[���o���Ă���
      instance->key = key;

      // �L�[�{�[�h����ɂ�鏈��
      switch (key)
      {
      case GLFW_KEY_R:

        // �ݒ�����Z�b�g����
        instance->reset();
        break;

      case GLFW_KEY_M:

        // ���j���[�\���� ON/OFF ����
        instance->showMenu = !instance->showMenu;
        break;

      case GLFW_KEY_Z:

        // �Y�[�����𒲐�����
        if (shiftKey)
          --attitude.foreAdjust[0];
        else
          ++attitude.foreAdjust[0];

        // �������e�ϊ��s����X�V����
        instance->update();
        break;

      case GLFW_KEY_P:

        // HMD �̂Ƃ��͎����𒲐����Ȃ�
        if (oculus) break;

        // �����𒲐�����
        if (shiftKey)
          --attitude.parallax;
        else
          ++attitude.parallax;

        // �������e�ϊ��s����X�V����
        instance->update();
        break;

      case GLFW_KEY_E:

        // �J�������L���łȂ���ΘI�o�𒲐����Ȃ�
        if (!instance->camera) break;

        // �I�o�𒲐�����
        if (shiftKey)
          instance->camera->decreaseExposure();
        else
          instance->camera->increaseExposure();
        break;

      case GLFW_KEY_G:

        // �J�������L���łȂ���ΘI�o�𒲐����Ȃ�
        if (!instance->camera) break;

        // �����𒲐�����
        if (shiftKey)
          instance->camera->decreaseGain();
        else
          instance->camera->increaseGain();
        break;

      case GLFW_KEY_UP:

        if (ctrlKey)
        {
          // �w�i�ɑ΂���c�����̉�p���L����
          ++attitude.circleAdjust[1];
        }
        else if (shiftKey)
        {
          // �w�i����ɂ��炷
          ++attitude.circleAdjust[3];
        }
        else if (altKey)
        {
          // �w�i����ɉ�]����
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[1];
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[1].conjugate();
        }
        else
        {
          // �w�i�ɑ΂���œ_���������΂�
          ++attitude.backAdjust[0];
        }

        // �J�����̉�p�ƒ��S�ʒu���X�V����
        instance->updateCircle();
        break;

      case GLFW_KEY_DOWN:

        if (ctrlKey)
        {
          // �w�i�ɑ΂���c�����̉�p�����߂�
          --attitude.circleAdjust[1];
        }
        else if (shiftKey)
        {
          // �w�i�����ɂ��炷
          --attitude.circleAdjust[3];
        }
        else if (altKey)
        {
          // �w�i�����ɉ�]����
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[1].conjugate();
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[1];
        }
        else
        {
          // �w�i�ɑ΂���œ_�������k�߂�
          --attitude.backAdjust[0];
        }

        // �J�����̉�p�ƒ��S�ʒu���X�V����
        instance->updateCircle();
        break;

      case GLFW_KEY_RIGHT:

        if (ctrlKey)
        {
          // �w�i�ɑ΂��鉡�����̉�p���L����
          ++attitude.circleAdjust[0];
        }
        else if (shiftKey)
        {
          // �w�i���E�ɂ��炷
          ++attitude.circleAdjust[2];
        }
        else if (altKey)
        {
          // �w�i���E�ɉ�]����
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[0].conjugate();
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[0];
        }
        else if (defaults.display_mode != MONOCULAR)
        {
          // �X�N���[���̊Ԋu���g�傷��
          ++attitude.offset;
        }

        // �J�����̉�p�ƒ��S�ʒu���X�V����
        instance->updateCircle();
        break;

      case GLFW_KEY_LEFT:

        if (ctrlKey)
        {
          // �w�i�ɑ΂��鉡�����̉�p�����߂�
          --attitude.circleAdjust[0];
        }
        else if (shiftKey)
        {
          // �w�i�����ɂ��炷
          --attitude.circleAdjust[2];
        }
        else if (altKey)
        {
          // �w�i�����ɉ�]����
          attitude.eyeOrientation[camL] *= attitude.eyeOrientationStep[0];
          attitude.eyeOrientation[camR] *= attitude.eyeOrientationStep[0].conjugate();
        }
        else if (defaults.display_mode != MONOCULAR)
        {
          // �������k������
          --attitude.offset;
        }

        // �J�����̉�p�ƒ��S�ʒu���X�V����
        instance->updateCircle();
        break;

      case GLFW_KEY_ESCAPE:
        break;

      case GLFW_KEY_BACKSPACE:
      case GLFW_KEY_DELETE:
        break;

      default:
        break;
      }
    }
  }
}

//
// �ݒ�l�̏�����
//
void Window::reset()
{
  // ���̂̈ʒu
  attitude.position = attitude.initialPosition;

  // ���̂̉�]
  attitude.orientation.reset(attitude.initialOrientation.getQuaternion());

  // �J�������Ƃ̎p���̕␳�l
  for (int cam = 0; cam < camCount; ++cam) attitude.eyeOrientation[cam] = attitude.initialEyeOrientation[cam];

  // �O�i�ɑ΂���œ_�����ƒ��S�ʒu�̕␳�l
  attitude.foreAdjust = attitude.initialForeAdjust;

  // �w�i�ɑ΂���œ_�����ƒ��S�ʒu�̕␳�l
  attitude.backAdjust = attitude.initialBackAdjust;

  // �w�i�̎���
  attitude.parallax = attitude.initialParallax;

  // �w�i�̃X�N���[���̊Ԋu
  attitude.offset = attitude.initialOffset;

  // �������e�ϊ��s����X�V����
  update();

  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu�̕␳�l
  attitude.circleAdjust = attitude.initialCircleAdjust;

  // �J�����̉�p�ƒ��S�ʒu���X�V����
  updateCircle();
}

//
// Oculus Rift ���N������
//
bool Window::startOculus()
{
  // Oculus Rift �̃R���e�L�X�g�����
  oculus = Oculus::initialize(zoom, &aspect, mp, screen);

  // ���Ȃ�������G���[
  if (oculus == nullptr) return false;

  // Oculus Rift �̋N���ɐ�������
  return true;
}

//
// Oculus Rift ���~����
//
void Window::stopOculus()
{
  if (oculus) oculus->terminate();
}

//
// �`�悷��ڂ�I������
//
void Window::select(int eye)
{
  switch (defaults.display_mode)
  {
  case MONOCULAR:

    // �E�B���h�E�S�̂��r���[�|�[�g�ɂ���
    glViewport(0, 0, width, height);

    // �f�v�X�o�b�t�@����������
    glClear(GL_DEPTH_BUFFER_BIT);

    break;

  case TOP_AND_BOTTOM:

    if (eye == camL)
    {
      // �f�B�X�v���C�̏㔼�������ɕ`�悷��
      glViewport(0, height, width, height);

      // �f�v�X�o�b�t�@����������
      glClear(GL_DEPTH_BUFFER_BIT);
    }
    else
    {
      // �f�B�X�v���C�̉����������ɕ`�悷��
      glViewport(0, 0, width, height);
    }
    break;

  case INTERLACE:
  case SIDE_BY_SIDE:

    if (eye == camL)
    {
      // �f�B�X�v���C�̍����������ɕ`�悷��
      glViewport(0, 0, width, height);

      // �f�v�X�o�b�t�@����������
      glClear(GL_DEPTH_BUFFER_BIT);
    }
    else
    {
      // �f�B�X�v���C�̉E���������ɕ`�悷��
      glViewport(width, 0, width, height);
    }
    break;

  case QUADBUFFER:

    // ���E�̃o�b�t�@�ɕ`�悷��
    glDrawBuffer(eye == camL ? GL_BACK_LEFT : GL_BACK_RIGHT);

    // �f�v�X�o�b�t�@����������
    glClear(GL_DEPTH_BUFFER_BIT);
    break;

  case OCULUS:

    if (oculus)
    {
      // �`�悷�� Oculus Rift �̖ڂ�I������
      oculus->select(eye, po[eye], qo[eye]);

      // �l��������ϊ��s������߂�
      mo[eye] = qo[eye].getMatrix();

      // �w�b�h�g���b�L���O�̕ϊ��s������L�������ɕۑ�����
      Scene::setLocalAttitude(eye, mo[eye].transpose());

      // mv �͕ύX���Ȃ��̂Ŗ߂�
      return;
    }

  default:
    break;
  }

  // �ڂ����炷����ɑO�i�𓮂���
  mv[eye] = ggTranslate(eye == camL ? parallax : -parallax, 0.0f, 0.0f);
}

//
// �`����J�n����
//
bool Window::start()
{
  // Oculus Rift ���g���Ă��Ȃ�������֌W�Ȃ�
  if (!oculus) return true;

  // ���f���ϊ��s���ݒ肷��
  mm = ggTranslate(attitude.position) * attitude.orientation.getMatrix();

  // ���f���ϊ��s������L�������ɕۑ�����
  Scene::setup(mm);

  // Oculus Rift �̕`����J�n����
  auto status{ oculus->start(mv) };

  // Oculus Rift �ɕ\���\�Ȃ� true
  if (status == Oculus::VISIBLE) return true;

  // �I���悤���オ�o�Ă���΃E�B���h�E�����
  if (status == Oculus::WANTQUIT) setClose();

  // Oculus Rift �ɕ\�����Ȃ�
  return false;
}

//
// �������e�ϊ��s����X�V����
//
//   �E�E�B���h�E�̃T�C�Y�ύX����J�����p�����[�^�̕ύX���ɌĂяo��
//
void Window::update()
{
  // �O�i�̃Y�[�������X�V����
  zoom = 1.0f / (1.0f - zoomStep * attitude.foreAdjust[0]);

  // �w�i�̏œ_�������X�V����
  focal = 1.0f / (1.0f - backFocalStep * attitude.backAdjust[0]);

  // �w�i�̎������X�V����
  parallax = defaultParallax + parallaxStep * attitude.parallax;

  // �w�i�̃X�N���[���̊Ԋu���X�V����
  offset = offsetDefault + offsetStep * attitude.offset;

  // Oculus Rift �g�p���͓������e�ϊ��s�񂾂��X�V����
  if (oculus)
  {
    oculus->getPerspective(zoom, mp);
    return;
  }

  // �X�N���[���̍����ƕ�
  const GLfloat screenHeight{ defaults.display_center / defaults.display_distance };
  const GLfloat screenWidth{ screenHeight * aspect };

  // �����ɂ��X�N���[���̃V�t�g��
  const GLfloat shift{ defaults.display_mode != MONOCULAR
    ? parallax * defaults.display_near / defaults.display_distance : 0.0f };

  // �O���ʂ� defaults.display_near �Ȃ̂ŃX�N���[��������ɍ��킹��
  const GLfloat zf{ defaults.display_near / zoom };

  // ���ڂ̎���
  const GLfloat fovL[]
  {
    -screenWidth + shift,
    screenWidth + shift,
    -screenHeight,
    screenHeight
  };

  // ���ڂ̓������e�ϊ��s������߂�
  mp[0].loadFrustum(fovL[0] * zf, fovL[1] * zf, fovL[2] * zf, fovL[3] * zf,
    defaults.display_near, defaults.display_far);

  // ���ڂ̃X�N���[���̃T�C�Y�ƒ��S�ʒu
  screen[0][0] = (fovL[1] - fovL[0]) * 0.5f;
  screen[0][1] = (fovL[3] - fovL[2]) * 0.5f;
  screen[0][2] = (fovL[1] + fovL[0]) * 0.5f;
  screen[0][3] = (fovL[3] + fovL[2]) * 0.5f;

  // Oculus Rift �ȊO�̗��̎��\���̏ꍇ
  if (defaults.display_mode != MONOCULAR)
  {
    // �E�ڂ̎���
    const GLfloat fovR[] =
    {
      -screenWidth - shift,
      screenWidth - shift,
      -screenHeight,
      screenHeight,
    };

    // �E�̓������e�ϊ��s������߂�
    mp[1].loadFrustum(fovR[0] * zf, fovR[1] * zf, fovR[2] * zf, fovR[3] * zf,
      defaults.display_near, defaults.display_far);

    // �E�ڂ̃X�N���[���̃T�C�Y�ƒ��S�ʒu
    screen[1][0] = (fovR[1] - fovR[0]) * 0.5f;
    screen[1][1] = (fovR[3] - fovR[2]) * 0.5f;
    screen[1][2] = (fovR[1] + fovR[0]) * 0.5f;
    screen[1][3] = (fovR[3] + fovR[2]) * 0.5f;
  }
}

//
// �J�����̉�p�ƒ��S�ʒu���X�V����
//
void Window::updateCircle()
{
  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu
  circle[0] = defaults.camera_fov_x + fovStep * attitude.circleAdjust[0];
  circle[1] = defaults.camera_fov_y + fovStep * attitude.circleAdjust[1];
  circle[2] = defaults.camera_center_x + shiftStep * attitude.circleAdjust[2];
  circle[3] = defaults.camera_center_y + shiftStep * attitude.circleAdjust[3];
}

// �`�����������
void Window::commit(int eye)
{
  if (oculus) oculus->commit(eye);
}

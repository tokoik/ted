//
// �E�B���h�E�֘A�̏���
//
#include "Window.h"

// Oculus Rift SDK ���C�u���� (LibOVR) �̑g�ݍ���
#if defined(_WIN32)
// �R���t�B�M�����[�V�����𒲂ׂ�
#  if defined(_DEBUG)
// �f�o�b�O�r���h�̃��C�u�����������N����
#    pragma comment(lib, "libOVRd.lib")
#  else
// �����[�X�r���h�̃��C�u�����������N����
#    pragma comment(lib, "libOVR.lib")
#  endif
#endif

// Dear ImGui ���g���Ƃ�
#if defined(IMGUI_VERSION)
#  include "imgui_impl_glfw.h"
#  include "imgui_impl_opengl3.h"
#endif

// �V�[���O���t
#include "Scene.h"

// �W�����C�u����
#include <fstream>

// �W���C�X�e�B�b�N�ԍ��̍ő�l
constexpr int maxJoystick{ 4 };

#if OVR_PRODUCT_VERSION > 0
// GetDefaultAdapterLuid �̂���
#  if defined(_WIN32)
#    include <dxgi.h>
#    pragma comment(lib, "dxgi.lib")
#  endif

static ovrGraphicsLuid GetDefaultAdapterLuid()
{
  ovrGraphicsLuid luid = ovrGraphicsLuid();

#  if defined(_WIN32)
  IDXGIFactory* factory = nullptr;

  if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&factory))))
  {
    IDXGIAdapter* adapter = nullptr;

    if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
    {
      DXGI_ADAPTER_DESC desc;

      adapter->GetDesc(&desc);
      memcpy(&luid, &desc.AdapterLuid, sizeof(luid));
      adapter->Release();
    }

    factory->Release();
  }
#  endif

  return luid;
}

static int Compare(const ovrGraphicsLuid& lhs, const ovrGraphicsLuid& rhs)
{
  return memcmp(&lhs, &rhs, sizeof(ovrGraphicsLuid));
}
#endif

//
// �R���X�g���N�^
//
Window::Window(Config& defaults, GLFWwindow* share)
  : defaults{ defaults }
  , config{ defaults }
  , window{ nullptr }
  , camera{ nullptr }
  , session{ nullptr }
  , showScene{ true }
  , showMirror{ true }
  , showMenu{ true }
  , oculusFbo{ 0, 0 }
  , mirrorFbo{ 0 }
  , mirrorTexture{ nullptr }
  , zoomChange{ 0 }
  , focalChange{ 0 }
  , circleChange{ 0, 0, 0, 0 }
  , key{ GLFW_KEY_UNKNOWN }
  , joy{ -1 }
#if OVR_PRODUCT_VERSION > 0
  , frameIndex{ 0LL }
  , oculusDepth{ 0, 0 }
#endif
{
  GLFWmonitor* monitor{ nullptr };

  // �t���X�N���[���\��
  if (config.display_fullscreen)
  {
    // �ڑ�����Ă��郂�j�^�̐��𐔂���
    int mcount;
    GLFWmonitor** const monitors(glfwGetMonitors(&mcount));

    // ���j�^�̑��݃`�F�b�N
    if (mcount == 0)
    {
      NOTIFY("GLFW �̏����������Ă��Ȃ����\���\�ȃf�B�X�v���C��������܂���B");
      exit(EXIT_FAILURE);
    }

    // �Z�J���_�����j�^������΂�����g��
    monitor = monitors[mcount > config.display_secondary ? config.display_secondary : 0];

    // ���j�^�̃��[�h�𒲂ׂ�
    const GLFWvidmode* mode(glfwGetVideoMode(monitor));

    // �E�B���h�E�̃T�C�Y���f�B�X�v���C�̃T�C�Y�ɂ���
    width = mode->width;
    height = mode->height;
  }
  else
  {
    // �v���C�}�����j�^���E�B���h�E���[�h�Ŏg��
    monitor = nullptr;

    // �E�B���h�E�̃T�C�Y�Ƀf�t�H���g�l��ݒ肷��
    width = config.display_width;
    height = config.display_height;
  }

  // �ŏ��̃C���X�^���X�̂Ƃ����� true
  static bool firstTime(true);

  // �ŏ��̃C���X�^���X�ň�x�������s
  if (firstTime)
  {
    // �J���������̒����X�e�b�v�����߂�
    qrStep[0].loadRotate(0.0f, 1.0f, 0.0f, parallaxStep);
    qrStep[1].loadRotate(1.0f, 0.0f, 0.0f, parallaxStep);

    // Oculus Rift �ɕ\������Ƃ�
    if (config.display_mode == OCULUS)
    {
      // Oculus Rift (LibOVR) ������������
      ovrInitParams initParams = { ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0 };
      if (OVR_FAILURE(ovr_Initialize(&initParams)))
      {
        // LibOVR �̏������Ɏ��s����
        NOTIFY("LibOVR ���������ł��܂���B");
        return;
      }

      // �v���O�����I�����ɂ� LibOVR ���I������
      atexit(ovr_Shutdown);

      // Oculus Rift �̃Z�b�V�������쐬����
      ovrGraphicsLuid luid;
      if (OVR_FAILURE(ovr_Create(&const_cast<ovrSession>(session), &luid)))
      {
        // Oculus Rift �̃Z�b�V�����̍쐬�Ɏ��s����
        NOTIFY("Oculus Rift �̃Z�b�V�������쐬�ł��܂���B");
        return;
      }

#if OVR_PRODUCT_VERSION > 0
      // �f�t�H���g�̃O���t�B�b�N�X�A�_�v�^���g���Ă��邩�m���߂�
      if (Compare(luid, GetDefaultAdapterLuid()))
      {
        // �f�t�H���g�̃O���t�B�b�N�X�A�_�v�^���g�p����Ă��Ȃ�
        NOTIFY("OpenGL �ł̓f�t�H���g�̃O���t�B�b�N�X�A�_�v�^�ȊO�g�p�ł��܂���B");
      }
#else
      // (LUID �� OpenGL �ł͎g���Ă��Ȃ��炵��)
#endif

      // Oculus �ł̓_�u���o�b�t�@���[�h�ɂ��Ȃ�
      glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_FALSE);
    }
    else if (config.display_mode == QUADBUFFER)
    {
      // �N���b�h�o�b�t�@�X�e���I���[�h�ɂ���
      glfwWindowHint(GLFW_STEREO, GLFW_TRUE);
    }

    // SRGB ���[�h�Ń����_�����O����
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

    // OpenGL �̃o�[�W�������w�肷��
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Core Profile ��I������
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#if defined(IMGUI_VERSION)
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
  const_cast<GLFWwindow*>(window) = glfwCreateWindow(width, height, windowTitle, monitor, share);

  // �E�B���h�E���J���ꂽ���ǂ����m���߂�
  if (!window)
  {
    // �E�B���h�E���J����Ȃ�����
    NOTIFY("GLFW �̃E�B���h�E���쐬�ł��܂���B");
    return;
  }

  //
  // �����ݒ�
  //

  // �w�b�h�g���b�L���O�̕ϊ��s�������������
  for (auto& m : mo) m = ggIdentity();

  // �ݒ������������
  reset();

  //
  // �\���p�̃E�B���h�E�̐ݒ�
  //

  // ���݂̃E�B���h�E�������Ώۂɂ���
  glfwMakeContextCurrent(window);

  // �Q�[���O���t�B�b�N�X���_�̓s���ɂ��ƂÂ����������s��
  ggInit();

  // Oculus Rift �ւ̕\���ł̓X���b�v�Ԋu��҂��Ȃ�
  glfwSwapInterval(config.display_mode == OCULUS ? 0 : 1);

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
      const auto* const axes(glfwGetJoystickAxes(joy, &axesCount));

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

  // sRGB �J���[�X�y�[�X���g��
  glEnable(GL_FRAMEBUFFER_SRGB);

  // �w�i�F
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

  //
  // Oculus Rift �̐ݒ�
  //

  // Oculus Rift �ɕ\������Ƃ�
  if (session)
  {
    // Oculus Rift �̏������o��
    hmdDesc = ovr_GetHmdDesc(session);

#  if defined(DEBUG)
    // Oculus Rift �̏���\������
    std::cerr
      << "\nProduct name: " << hmdDesc.ProductName
      << "\nResolution:   " << hmdDesc.Resolution.w << " x " << hmdDesc.Resolution.h
      << "\nDefault Fov:  (" << hmdDesc.DefaultEyeFov[ovrEye_Left].LeftTan
      << "," << hmdDesc.DefaultEyeFov[ovrEye_Left].DownTan
      << ") - (" << hmdDesc.DefaultEyeFov[ovrEye_Left].RightTan
      << "," << hmdDesc.DefaultEyeFov[ovrEye_Left].UpTan
      << ")\n              (" << hmdDesc.DefaultEyeFov[ovrEye_Right].LeftTan
      << "," << hmdDesc.DefaultEyeFov[ovrEye_Right].DownTan
      << ") - (" << hmdDesc.DefaultEyeFov[ovrEye_Right].RightTan
      << "," << hmdDesc.DefaultEyeFov[ovrEye_Right].UpTan
      << ")\nMaximum Fov:  (" << hmdDesc.MaxEyeFov[ovrEye_Left].LeftTan
      << "," << hmdDesc.MaxEyeFov[ovrEye_Left].DownTan
      << ") - (" << hmdDesc.MaxEyeFov[ovrEye_Left].RightTan
      << "," << hmdDesc.MaxEyeFov[ovrEye_Left].UpTan
      << ")\n              (" << hmdDesc.MaxEyeFov[ovrEye_Right].LeftTan
      << "," << hmdDesc.MaxEyeFov[ovrEye_Right].DownTan
      << ") - (" << hmdDesc.MaxEyeFov[ovrEye_Right].RightTan
      << "," << hmdDesc.MaxEyeFov[ovrEye_Right].UpTan
      << ")\n" << std::endl;
#  endif

    // Oculus Rift �ɓ]������`��f�[�^���쐬����
#if OVR_PRODUCT_VERSION > 0
    layerData.Header.Type = ovrLayerType_EyeFov;
#else
    layerData.Header.Type = ovrLayerType_EyeFovDepth;
#endif
    layerData.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // OpenGL �Ȃ̂ō��������_

    // Oculus Rift �\���p�� FBO ���쐬����
    for (int eye = 0; eye < ovrEye_Count; ++eye)
    {
      // Oculus Rift �̎�����擾����
      const auto& eyeFov(hmdDesc.DefaultEyeFov[eye]);
#if OVR_PRODUCT_VERSION > 0
      layerData.Fov[eye] = eyeFov;

      const auto& fov(eyeFov);
#else
      layerData.EyeFov.Fov[eye] = eyeFov;

      // Oculus Rift �̃����Y�␳���̐ݒ�l���擾����
      eyeRenderDesc[eye] = ovr_GetRenderDesc(session, ovrEyeType(eye), eyeFov);

      // Oculus Rift �̕Жڂ̓��̈ʒu����̕ψʂ����߂�
      const auto& offset(eyeRenderDesc[eye].HmdToEyeViewOffset);
      mv[eye] = ggTranslate(-offset.x, -offset.y, -offset.z);

      const auto& fov(eyeRenderDesc[eye].Fov);
#endif

      // �Y�[����
      const auto zf(config.display_zoom * config.display_near);

      // �Жڂ̓������e�ϊ��s������߂�
      mp[eye].loadFrustum(-fov.LeftTan * zf, fov.RightTan * zf, -fov.DownTan * zf, fov.UpTan * zf,
        config.display_near, config.display_far);

      // �Жڂ̃X�N���[���̃T�C�Y�ƒ��S�ʒu
      screen[eye][0] = (fov.RightTan + fov.LeftTan) * 0.5f;
      screen[eye][1] = (fov.UpTan + fov.DownTan) * 0.5f;
      screen[eye][2] = (fov.RightTan - fov.LeftTan) * 0.5f;
      screen[eye][3] = (fov.UpTan - fov.DownTan) * 0.5f;

      // Oculus Rift �\���p�� FBO �̃T�C�Y
      const auto size(ovr_GetFovTextureSize(session, ovrEyeType(eye), eyeFov, 1.0f));
#if OVR_PRODUCT_VERSION > 0
      layerData.Viewport[eye].Pos = OVR::Vector2i(0, 0);
      layerData.Viewport[eye].Size = size;

      // Oculus Rift �\���p�� FBO �̃J���[�o�b�t�@�Ƃ��Ďg���e�N�X�`���Z�b�g�̓���
      const ovrTextureSwapChainDesc colorDesc =
      {
        ovrTexture_2D,                    // Type
        OVR_FORMAT_R8G8B8A8_UNORM_SRGB,   // Format
        1,                                // ArraySize
        size.w,                           // Width
        size.h,                           // Height
        1,                                // MipLevels
        1,                                // SampleCount
        ovrFalse,                         // StaticImage
        0, 0
      };

      // Oculus Rift �\���p�� FBO �̃o�b�t�@�Ƃ��Ďg���e�N�X�`���Z�b�g���쐬����
      layerData.ColorTexture[eye] = nullptr;
      if (OVR_SUCCESS(ovr_CreateTextureSwapChainGL(session, &colorDesc, &layerData.ColorTexture[eye])))
      {
        // Oculus Rift �\���p�� FBO �̃J���[�o�b�t�@�Ƃ��Ďg���e�N�X�`���Z�b�g���쐬����
        int length(0);
        if (OVR_SUCCESS(ovr_GetTextureSwapChainLength(session, layerData.ColorTexture[eye], &length)))
        {
          for (int i = 0; i < length; ++i)
          {
            GLuint texId;
            ovr_GetTextureSwapChainBufferGL(session, layerData.ColorTexture[eye], i, &texId);
            glBindTexture(GL_TEXTURE_2D, texId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
          }
        }

        // Oculus Rift �\���p�� FBO �̃f�v�X�o�b�t�@�Ƃ��Ďg���e�N�X�`�����쐬����
        glGenTextures(1, &oculusDepth[eye]);
        glBindTexture(GL_TEXTURE_2D, oculusDepth[eye]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, size.w, size.h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      }
#else
      layerData.EyeFov.Viewport[eye].Pos = OVR::Vector2i(0, 0);
      layerData.EyeFov.Viewport[eye].Size = size;

      // Oculus Rift �\���p�� FBO �̃J���[�o�b�t�@�Ƃ��Ďg���e�N�X�`���Z�b�g���쐬����
      ovrSwapTextureSet* colorTexture;
      ovr_CreateSwapTextureSetGL(session, GL_SRGB8_ALPHA8, size.w, size.h, &colorTexture);
      layerData.EyeFov.ColorTexture[eye] = colorTexture;

      // Oculus Rift �\���p�� FBO �̃f�v�X�o�b�t�@�Ƃ��Ďg���e�N�X�`���Z�b�g�̍쐬
      ovrSwapTextureSet* depthTexture;
      ovr_CreateSwapTextureSetGL(session, GL_DEPTH_COMPONENT32F, size.w, size.h, &depthTexture);
      layerData.EyeFovDepth.DepthTexture[eye] = depthTexture;
#endif
    }

#if OVR_PRODUCT_VERSION > 0
    // Oculus Rift �̉�ʂ̃A�X�y�N�g������߂�
    aspect = static_cast<GLfloat>(layerData.Viewport[ovrEye_Left].Size.w)
      / static_cast<GLfloat>(layerData.Viewport[ovrEye_Left].Size.h);

    // �~���[�\���p�� FBO ���쐬����
    const ovrMirrorTextureDesc mirrorDesc
    {
      OVR_FORMAT_R8G8B8A8_UNORM_SRGB,   // Format
      width,                            // Width
      height,                           // Height
      0
    };

    if (OVR_SUCCESS(ovr_CreateMirrorTextureGL(session, &mirrorDesc, &mirrorTexture)))
    {
      // �~���[�\���p�� FBO �̃J���[�o�b�t�@�Ƃ��Ďg���e�N�X�`�����쐬����
      GLuint texId;
      if (OVR_SUCCESS(ovr_GetMirrorTextureBufferGL(session, mirrorTexture, &texId)))
      {
        glGenFramebuffers(1, &mirrorFbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFbo);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
        glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
      }
    }

    // �p���̃g���b�L���O�ɂ����鏰�̍����� 0 �ɐݒ肷��
    ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);
#else
    // Oculus Rift �̉�ʂ̃A�X�y�N�g������߂�
    aspect = static_cast<GLfloat>(layerData.EyeFov.Viewport[ovrEye_Left].Size.w)
      / static_cast<GLfloat>(layerData.EyeFov.Viewport[ovrEye_Left].Size.h);

    // �~���[�\���p�� FBO ���쐬����
    if (OVR_SUCCESS(ovr_CreateMirrorTextureGL(session, GL_SRGB8_ALPHA8, width, height, reinterpret_cast<ovrTexture**>(&mirrorTexture))))
    {
      glGenFramebuffers(1, &mirrorFbo);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFbo);
      glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTexture->OGL.TexId, 0);
      glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

    // TimeWarp �Ɏg���ϊ��s��̐��������o��
    auto& posTimewarpProjectionDesc(layerData.EyeFovDepth.ProjectionDesc);
    posTimewarpProjectionDesc.Projection22 = (mp[ovrEye_Left].get()[4 * 2 + 2] + mp[ovrEye_Left].get()[4 * 3 + 2]) * 0.5f;
    posTimewarpProjectionDesc.Projection23 = mp[ovrEye_Left].get()[4 * 2 + 3] * 0.5f;
    posTimewarpProjectionDesc.Projection32 = mp[ovrEye_Left].get()[4 * 3 + 2];
#endif

    // Oculus Rift �̃����_�����O�p�� FBO ���쐬����
    glGenFramebuffers(ovrEye_Count, oculusFbo);
  }

#if defined(IMGUI_VERSION)
  //
  // ���[�U�C���^�t�F�[�X�̏���
  //
  //ImGuiIO& io = ImGui::GetIO(); (void)io;
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  //ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
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
  //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
  //IM_ASSERT(font != NULL);

  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 430");
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

  // Oculus Rift �g�p��
  if (session)
  {
    // �~���[�\���p�� FBO ���폜����
    if (mirrorFbo) glDeleteFramebuffers(1, &mirrorFbo);

    // �~���[�\���Ɏg�����e�N�X�`�����J������
#if OVR_PRODUCT_VERSION > 0
    if (mirrorTexture) ovr_DestroyMirrorTexture(session, mirrorTexture);
#else
    if (mirrorTexture)
    {
      glDeleteTextures(1, &mirrorTexture->OGL.TexId);
      ovr_DestroyMirrorTexture(session, reinterpret_cast<ovrTexture*>(mirrorTexture));
    }
#endif

    // Oculus Rift �̃����_�����O�p�� FBO ���폜����
    glDeleteFramebuffers(ovrEye_Count, oculusFbo);

    // Oculus Rift �\���p�� FBO ���폜����
    for (int eye = 0; eye < ovrEye_Count; ++eye)
    {
      // �����_�����O�^�[�Q�b�g�Ɏg�����e�N�X�`�����J������
#if OVR_PRODUCT_VERSION > 0
      if (layerData.ColorTexture[eye])
      {
        ovr_DestroyTextureSwapChain(session, layerData.ColorTexture[eye]);
        layerData.ColorTexture[eye] = nullptr;
      }
#else
      auto* const colorTexture(layerData.EyeFov.ColorTexture[eye]);
      for (int i = 0; i < colorTexture->TextureCount; ++i)
      {
        const auto* const ctex(reinterpret_cast<ovrGLTexture*>(&colorTexture->Textures[i]));
        glDeleteTextures(1, &ctex->OGL.TexId);
      }
      ovr_DestroySwapTextureSet(session, colorTexture);
#endif

      // �f�v�X�o�b�t�@�Ƃ��Ďg�����e�N�X�`�����J������
#if OVR_PRODUCT_VERSION > 0
      glDeleteTextures(1, &oculusDepth[eye]);
      oculusDepth[eye] = 0;
#else
      auto* const depthTexture(layerData.EyeFovDepth.DepthTexture[eye]);
      for (int i = 0; i < depthTexture->TextureCount; ++i)
      {
        const auto* const dtex(reinterpret_cast<ovrGLTexture*>(&depthTexture->Textures[i]));
        glDeleteTextures(1, &dtex->OGL.TexId);
      }
      ovr_DestroySwapTextureSet(session, depthTexture);
#endif
    }

    // Oculus Rift �̃Z�b�V������j������
    ovr_Destroy(session);
    const_cast<ovrSession>(session) = nullptr;
  }

#if defined(IMGUI_VERSION)
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
  const auto speedFactor((fabs(config.position[2]) + 0.2f));

  //
  // �W���C�X�e�B�b�N�ɂ�鑀��
  //

  // �W���C�X�e�B�b�N���L���Ȃ�
  if (joy >= 0)
  {
    // �{�^��
    int btnsCount;
    const auto* const btns{ glfwGetJoystickButtons(joy, &btnsCount) };

    for (int i = 0; i < btnsCount; ++i)
    {
      if (btns[i] != 0) std::cout << "btns[" << i << "] = " << (int)btns[i] << "\n";
    }

    // �X�e�B�b�N
    int axesCount;
    const auto* const axes{ glfwGetJoystickAxes(joy, &axesCount) };

    for (int i = 0; i < axesCount; ++i)
    {
      if (fabs(axes[i]) > 0.1f) std::cout << "axes[" << i << "] = " << axes[i] << "\n";
    }

    // �X�e�B�b�N�̑��x�W��
    const auto axesSpeedFactor{ axesSpeedScale * speedFactor };

    // L �{�^���� R �{�^���̏��
    const auto lrButton{ btns[4] | btns[5] };

    // �E�A�i���O�X�e�B�b�N
    if (axesCount > 3)
    {
      // ���̂����E�Ɉړ�����
      config.position[0] += (axes[0] - origin[0]) * axesSpeedFactor;

      // L �{�^���� R �{�^���𓯎��ɉ����Ă����
      if (lrButton)
      {
        // ���̂�O��Ɉړ�����
        config.position[2] += (axes[1] - origin[1]) * axesSpeedFactor;
      }
      else
      {
        // ���̂��㉺�Ɉړ�����
        config.position[1] += (axes[1] - origin[1]) * axesSpeedFactor;
      }

      // ���̂����E�ɉ�]����
      trackball.rotate(ggRotateQuaternion(0.0f, 1.0f, 0.0f, (axes[2] - origin[2]) * axesSpeedFactor));

      // ���̂��㉺�ɉ�]����
      trackball.rotate(ggRotateQuaternion(1.0f, 0.0f, 0.0f, (axes[3] - origin[3]) * axesSpeedFactor));
    }

    // B, X �{�^���̏��
    const auto parallaxButton{ btns[3] - btns[0] };

    // B, X �{�^���ɕω��������
    if (parallaxButton)
    {
      // L �{�^���� R �{�^���𓯎��ɉ����Ă����
      if (lrButton)
      {
        // �w�i�����E�ɉ�]����
        if (parallaxButton > 0)
        {
          config.parallax_offset[camL] *= qrStep[0];
          config.parallax_offset[camR] *= qrStep[0].conjugate();
        }
        else
        {
          config.parallax_offset[camL] *= qrStep[0].conjugate();
          config.parallax_offset[camR] *= qrStep[0];
        }
      }
      else
      {
        // �X�N���[���̊Ԋu�𒲐�����
        config.display_offset += parallaxButton * offsetStep;
      }
    }

    // Y, A �{�^���̏��
    const auto zoomButton{ btns[2] - btns[3] };

    // Y, A �{�^���ɕω��������
    if (zoomButton)
    {
      // L �{�^���� R �{�^���𓯎��ɉ����Ă����
      if (lrButton)
      {
        // �w�i���㉺�ɉ�]����
        if (zoomButton > 0)
        {
          config.parallax_offset[camL] *= qrStep[1];
          config.parallax_offset[camR] *= qrStep[1].conjugate();
        }
        else
        {
          config.parallax_offset[camL] *= qrStep[1].conjugate();
          config.parallax_offset[camR] *= qrStep[1];
        }
      }
      else
      {
        // �œ_�����𒲐�����
        const int change{ focalChange + zoomButton };
        if (change < focalMax)
        {
          focalChange = change;
          config.display_focal = defaults.display_focal * focalMax / (focalMax - change);
        }
      }
    }

    // �\���L�[�̍��E�{�^���̏��
    const auto textureXButton{ btns[13] - btns[15] };

    // �\���L�[�̍��E�{�^���ɕω��������
    if (textureXButton)
    {
      // L �{�^���� R �{�^���𓯎��ɉ����Ă����
      if (lrButton)
      {
        // �w�i�ɑ΂��鉡�����̉�p�𒲐�����
        config.circle[0] = defaults.circle[0] + static_cast<GLfloat>(circleChange[0] += textureXButton) * shiftStep;
      }
      else
      {
        // �w�i�̉��ʒu�𒲐�����
        config.circle[2] = defaults.circle[2] + static_cast<GLfloat>(circleChange[2] += textureXButton) * shiftStep;
      }
    }

    // �\���L�[�̏㉺�{�^���̏��
    const auto textureYButton{ btns[12] - btns[14] };

    // �\���L�[�̏㉺�{�^���ɕω��������
    if (textureYButton)
    {
      // L �{�^���� R �{�^���𓯎��ɉ����Ă����
      if (lrButton)
      {
        // �w�i�ɑ΂���c�����̉�p�𒲐�����
        config.circle[1] = defaults.circle[1] + static_cast<GLfloat>(circleChange[1] += textureYButton) * shiftStep;
      }
      else
      {
        // �w�i�̏c�ʒu�𒲐�����
        config.circle[3] = defaults.circle[3] + static_cast<GLfloat>(circleChange[3] += textureYButton) * shiftStep;
      }
    }

    // �ݒ�����Z�b�g����
    if (btns[7])
    {
      reset();
      updateProjectionMatrix();
    }
  }

  //
  // �}�E�X�ɂ�鑀��
  //

  // �}�E�X�̈ʒu
  double x, y;

#if defined(IMGUI_VERSION)

  // ImGui �̐V�K�t���[�����쐬����
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();

  // �}�E�X�J�[�\���� ImGui �̃E�B���h�E��ɂ������� Window �N���X�̃}�E�X�ʒu���X�V���Ȃ�
  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return true;

  // �}�E�X�̌��݈ʒu�𒲂ׂ�
  const ImGuiIO& io{ ImGui::GetIO() };
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
    const auto speed{ speedScale * speedFactor };
    config.position[0] += static_cast<GLfloat>(x - cx) * speed;
    config.position[1] += static_cast<GLfloat>(cy - y) * speed;
    cx = x;
    cy = y;
  }

  // �E�{�^���h���b�O
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2))
  {
    // ���̂���]����
    trackball.motion(float(x), float(y));
  }

  //
  // �L�[�{�[�h�ɂ�鑀��
  //

#if defined(IMGUI_VERSION)
    // ImGui ���L�[�{�[�h���g���Ƃ��̓L�[�{�[�h�̏������s��Ȃ�
  if (ImGui::GetIO().WantCaptureKeyboard) return true;
#endif

  // �V�t�g�L�[�̏��
  const auto shiftKey{ glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) };

  // �R���g���[���L�[�̏��
  const auto ctrlKey{ glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) };

  // �I���^�l�[�g�L�[�̏��
  const auto altKey{ glfwGetKey(window, GLFW_KEY_LEFT_ALT) || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) };

  // �E���L�[����
  if (glfwGetKey(window, GLFW_KEY_RIGHT))
  {
    if (altKey)
    {
      // �w�i���E�ɉ�]����
      if (!ctrlKey) config.parallax_offset[camL] *= qrStep[0].conjugate();
      if (!shiftKey) config.parallax_offset[camR] *= qrStep[0].conjugate();
    }
    else if (ctrlKey)
    {
      // �w�i�ɑ΂��鉡�����̉�p���L����
      config.circle[0] = defaults.circle[0] + static_cast<GLfloat>(++circleChange[0]) * shiftStep;
    }
    else if (shiftKey)
    {
      // �w�i���E�ɂ��炷
      config.circle[2] = defaults.circle[2] + static_cast<GLfloat>(++circleChange[2]) * shiftStep;
    }
    else if (config.display_mode != MONO)
    {
      // �X�N���[���̊Ԋu���g�傷��
      config.display_offset += offsetStep;
    }
  }

  // �����L�[����
  if (glfwGetKey(window, GLFW_KEY_LEFT))
  {
    if (altKey)
    {
      // �w�i�����ɉ�]����
      if (!ctrlKey) config.parallax_offset[camL] *= qrStep[0];
      if (!shiftKey) config.parallax_offset[camR] *= qrStep[0];
    }
    else if (ctrlKey)
    {
      // �w�i�ɑ΂��鉡�����̉�p�����߂�
      config.circle[0] = defaults.circle[0] + static_cast<GLfloat>(--circleChange[0]) * shiftStep;
    }
    else if (shiftKey)
    {
      // �w�i�����ɂ��炷
      config.circle[2] = defaults.circle[2] + static_cast<GLfloat>(--circleChange[2]) * shiftStep;
    }
    else if (config.display_mode != MONO)
    {
      // �������k������
      config.display_offset -= offsetStep;
    }
  }

  // ����L�[����
  if (glfwGetKey(window, GLFW_KEY_UP))
  {
    if (altKey)
    {
      // �w�i����ɉ�]����
      if (!ctrlKey) config.parallax_offset[camL] *= qrStep[1];
      if (!shiftKey) config.parallax_offset[camR] *= qrStep[1];
    }
    else if (ctrlKey)
    {
      // �w�i�ɑ΂���c�����̉�p���L����
      config.circle[1] = defaults.circle[1] + static_cast<GLfloat>(++circleChange[1]) * shiftStep;
    }
    else if (shiftKey)
    {
      // �w�i����ɂ��炷
      config.circle[3] = defaults.circle[3] + static_cast<GLfloat>(++circleChange[3]) * shiftStep;
    }
    else
    {
      // �œ_���������΂�
      const int change{ focalChange + 1 };
      if (change < focalMax)
      {
        focalChange = change;
        config.display_focal = defaults.display_focal * focalMax / (focalMax - change);
      }
    }
  }

  // �����L�[����
  if (glfwGetKey(window, GLFW_KEY_DOWN))
  {
    if (altKey)
    {
      // �w�i�����ɉ�]����
      if (!ctrlKey) config.parallax_offset[camL] *= qrStep[1].conjugate();
      if (!shiftKey) config.parallax_offset[camR] *= qrStep[1].conjugate();
    }
    else if (ctrlKey)
    {
      // �w�i�ɑ΂���c�����̉�p�����߂�
      config.circle[1] = defaults.circle[2] + static_cast<GLfloat>(--circleChange[1]) * shiftStep;
    }
    else if (shiftKey)
    {
      // �w�i�����ɂ��炷
      config.circle[3] = defaults.circle[3] + static_cast<GLfloat>(--circleChange[3]) * shiftStep;
    }
    else
    {
      // �œ_�������k�߂�
      const int change{ focalChange - 1 };
      if (change < focalMax)
      {
        focalChange = change;
        config.display_focal = defaults.display_focal * focalMax / (focalMax - change);
      }
    }
  }

  // 'P' �L�[�̑���
  if (glfwGetKey(window, GLFW_KEY_P))
  {
    // �����𒲐�����
    config.parallax += shiftKey ? -parallaxStep : parallaxStep;

    // �������e�ϊ��s����X�V����
    updateProjectionMatrix();
  }

  // 'Z' �L�[�̑���
  if (glfwGetKey(window, GLFW_KEY_Z))
  {
    // �Y�[�����𒲐�����
    const int change{ zoomChange + (shiftKey ? 1 : -1) };
    if (change < zoomMax)
    {
      zoomChange = change;
      config.display_zoom = defaults.display_zoom * zoomMax / (zoomMax - change);
    }

    // �������e�ϊ��s����X�V����
    updateProjectionMatrix();
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

  return true;
}

//
// �`��J�n
//
//   �E�}�`�̕`��J�n�O�ɌĂяo��
//
bool Window::start()
{
  // Oculus Rift �g�p��
  if (session)
  {
#if OVR_PRODUCT_VERSION > 0
    // �Z�b�V�����̏�Ԃ��擾����
    ovrSessionStatus sessionStatus;
    ovr_GetSessionStatus(session, &sessionStatus);

    // �A�v���P�[�V�������I����v�����Ă���Ƃ��̓E�B���h�E�̃N���[�Y�t���O�𗧂Ă�
    if (sessionStatus.ShouldQuit) glfwSetWindowShouldClose(window, GLFW_TRUE);

    // ���݂̏�Ԃ��g���b�L���O�̌��_�ɂ���
    if (sessionStatus.ShouldRecenter) ovr_RecenterTrackingOrigin(session);

    // HmdToEyeOffset �Ȃǂ͎��s���ɕω�����̂Ŗ��t���[�� ovr_GetRenderDesc() �� ovrEyeRenderDesc ���擾����
    const ovrEyeRenderDesc eyeRenderDesc[]
    {
      ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]),
      ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1])
    };

    // Oculus Rift �̍��E�̖ڂ̃g���b�L���O�̈ʒu����̕ψʂ����߂�
    const ovrPosef hmdToEyePose[]
    {
      eyeRenderDesc[0].HmdToEyePose,
      eyeRenderDesc[1].HmdToEyePose
    };

    // ���_�̎p�������擾����
    ovr_GetEyePoses(session, frameIndex, ovrTrue, hmdToEyePose, layerData.RenderPose, &layerData.SensorSampleTime);

    // Oculus Rift �̗��ڂ̓��̈ʒu����̕ψʂ����߂�
    mv[0] = ggTranslate(-hmdToEyePose[0].Position.x, -hmdToEyePose[0].Position.y, -hmdToEyePose[0].Position.z);
    mv[1] = ggTranslate(-hmdToEyePose[1].Position.x, -hmdToEyePose[1].Position.y, -hmdToEyePose[1].Position.z);

#else
    // �t���[���̃^�C�~���O�v���J�n
    const auto ftiming(ovr_GetPredictedDisplayTime(session, 0));

    // sensorSampleTime �̎擾�͉\�Ȍ��� ovr_GetTrackingState() �̋߂��ōs��
    layerData.EyeFov.SensorSampleTime = ovr_GetTimeInSeconds();

    // �w�b�h�g���b�L���O�̏�Ԃ��擾����
    const auto hmdState(ovr_GetTrackingState(session, ftiming, ovrTrue));

    // ���E�Ԋu
    const ovrVector3f hmdToEyeViewOffset[]
    {
      eyeRenderDesc[0].HmdToEyeViewOffset,
      eyeRenderDesc[1].HmdToEyeViewOffset
    };

    // ���_�̎p���������߂�
    ovr_CalcEyePoses(hmdState.HeadPose.ThePose, hmdToEyeViewOffset, eyePose);
#endif
  }

  // ���f���ϊ��s���ݒ肷��
  mm = ggTranslate(config.position[0], config.position[1], -config.position[2]) * trackball.getMatrix();

  // ���f���ϊ��s������L�������ɕۑ�����
  Scene::setup(mm);

  return true;
}

//
// �}�`��������ڂ�I������
//
// �E���_���ƂɌĂяo��
//
void Window::select(int eye)
{
  // Oculus Rift �ɕ\������
  if (session)
  {
#if OVR_PRODUCT_VERSION > 0
    // Oculus Rift �Ƀ����_�����O���� FBO �ɐ؂�ւ���
    if (layerData.ColorTexture[eye])
    {
      // FBO �̃J���[�o�b�t�@�Ɏg�����݂̃e�N�X�`���̃C���f�b�N�X���擾����
      int curIndex;
      ovr_GetTextureSwapChainCurrentIndex(session, layerData.ColorTexture[eye], &curIndex);

      // FBO �̃J���[�o�b�t�@�Ɏg���e�N�X�`�����擾����
      GLuint curTexId;
      ovr_GetTextureSwapChainBufferGL(session, layerData.ColorTexture[eye], curIndex, &curTexId);

      // FBO ��ݒ肷��
      glBindFramebuffer(GL_FRAMEBUFFER, oculusFbo[eye]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, oculusDepth[eye], 0);

      // �r���[�|�[�g��ݒ肷��
      const auto& vp{ layerData.Viewport[eye] };
      glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
    }

    // Oculus Rift �̕Жڂ̈ʒu�Ɖ�]���擾����
    const auto& o{ layerData.RenderPose[eye].Orientation };
    const auto& p{ layerData.RenderPose[eye].Position };
#else
    // �����_�[�^�[�Q�b�g�ɕ`�悷��O�Ƀ����_�[�^�[�Q�b�g�̃C���f�b�N�X���C���N�������g����
    auto* const colorTexture(layerData.EyeFov.ColorTexture[eye]);
    colorTexture->CurrentIndex = (colorTexture->CurrentIndex + 1) % colorTexture->TextureCount;
    auto* const depthTexture(layerData.EyeFovDepth.DepthTexture[eye]);
    depthTexture->CurrentIndex = (depthTexture->CurrentIndex + 1) % depthTexture->TextureCount;

    // �����_�[�^�[�Q�b�g��؂�ւ���
    glBindFramebuffer(GL_FRAMEBUFFER, oculusFbo[eye]);
    const auto& ctex(reinterpret_cast<ovrGLTexture*>(&colorTexture->Textures[colorTexture->CurrentIndex]));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctex->OGL.TexId, 0);
    const auto& dtex(reinterpret_cast<ovrGLTexture*>(&depthTexture->Textures[depthTexture->CurrentIndex]));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dtex->OGL.TexId, 0);

    // �r���[�|�[�g��ݒ肷��
    const auto& vp(layerData.EyeFov.Viewport[eye]);
    glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);

    // Oculus Rift �̕Жڂ̈ʒu�Ɖ�]���擾����
    const auto& p(eyePose[eye].Position);
    const auto& o(eyePose[eye].Orientation);
#endif

    // Oculus Rift �̕Жڂ̈ʒu��ۑ�����
    po[eye][0] = p.x;
    po[eye][1] = p.y;
    po[eye][2] = p.z;
    po[eye][3] = 1.0f;

    // Oculus Rift �̕Жڂ̉�]��ۑ�����
    qo[eye] = GgQuaternion(o.x, o.y, o.z, -o.w);

    // �l��������ϊ��s������߂�
    mo[eye] = qo[eye].getMatrix();

    // �w�b�h�g���b�L���O�̕ϊ��s������L�������ɕۑ�����
    Scene::setLocalAttitude(eye, mo[eye].transpose());

    // �f�v�X�o�b�t�@����������
    glClear(GL_DEPTH_BUFFER_BIT);

    // Oculus Rift �ɑ΂��鏈���͂����ŏI��
    return;
  }

  // Oculus Rift �ȊO�ɕ\������
  switch (config.display_mode)
  {
  case MONO:

    // �E�B���h�E�S�̂��r���[�|�[�g�ɂ���
    glViewport(0, 0, width, height);

    // �f�v�X�o�b�t�@����������
    glClear(GL_DEPTH_BUFFER_BIT);

    break;

  case TOPANDBOTTOM:

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

  case LINEBYLINE:
  case SIDEBYSIDE:

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

  default:
    break;
  }

  // �ڂ����炷����ɃV�[���𓮂���
  mv[eye] = ggTranslate(eye == camL ? config.parallax : -config.parallax, 0.0f, 0.0f);
}

//
// �}�`�̕`�����������
//
void Window::commit(int eye)
{
#if OVR_PRODUCT_VERSION > 0
  if (session)
  {
    // GL_COLOR_ATTACHMENT0 �Ɋ��蓖�Ă�ꂽ�e�N�X�`���� wglDXUnlockObjectsNV() �ɂ����
    // �A�����b�N����邽�߂Ɏ��̃t���[���̏����ɂ����Ė����� GL_COLOR_ATTACHMENT0 ��
    // FBO �Ɍ��������̂������
    glBindFramebuffer(GL_FRAMEBUFFER, oculusFbo[eye]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

    // �ۗ����̕ύX�� layerData.ColorTexture[eye] �ɔ��f���C���f�b�N�X���X�V����
    ovr_CommitTextureSwapChain(session, layerData.ColorTexture[eye]);
  }
#endif
}

//
// �J���[�o�b�t�@�����ւ��ăC�x���g�����o��
//
//   �E�}�`�̕`��I����ɌĂяo��
//   �E�_�u���o�b�t�@�����O�̃o�b�t�@�̓���ւ����s��
//   �E�L�[�{�[�h���쓙�̃C�x���g�����o��
//
void Window::swapBuffers()
{
  // �G���[�`�F�b�N
  ggError();

  // Oculus Rift �g�p��
  if (session)
  {
#if OVR_PRODUCT_VERSION > 0
    // �`��f�[�^�� Oculus Rift �ɓ]������
    const auto* const layers(&layerData.Header);
    if (OVR_FAILURE(ovr_SubmitFrame(session, frameIndex++, nullptr, &layers, 1)))
#else
    // Oculus Rift ��̕`��ʒu�Ɗg�嗦�����߂�
    ovrViewScaleDesc viewScaleDesc;
    viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
    viewScaleDesc.HmdToEyeViewOffset[0] = eyeRenderDesc[0].HmdToEyeViewOffset;
    viewScaleDesc.HmdToEyeViewOffset[1] = eyeRenderDesc[1].HmdToEyeViewOffset;

    // �`��f�[�^���X�V����
    layerData.EyeFov.RenderPose[0] = eyePose[0];
    layerData.EyeFov.RenderPose[1] = eyePose[1];

    // �`��f�[�^�� Oculus Rift �ɓ]������
    const auto* const layers(&layerData.Header);
    if (OVR_FAILURE(ovr_SubmitFrame(session, 0, &viewScaleDesc, &layers, 1)))
#endif
    {
      // �]���Ɏ��s������ Oculus Rift �̐ݒ���ŏ������蒼���K�v������炵��
      NOTIFY("Oculus Rift �ւ̃f�[�^�̓]���Ɏ��s���܂����B");

      // ���ǂ߂�ǂ������̂ŃE�B���h�E����Ă��܂�
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    // �����_�����O���ʂ��~���[�\���p�̃t���[���o�b�t�@�ɂ��]������
    if (showMirror)
    {
      glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFbo);
      glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#if OVR_PRODUCT_VERSION > 0
      glBlitFramebuffer(0, height, width, 0, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
#else
      const auto w(mirrorTexture->OGL.Header.TextureSize.w);
      const auto h(mirrorTexture->OGL.Header.TextureSize.h);
      glBlitFramebuffer(0, h, w, 0, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
#endif
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

#if defined(IMGUI_VERSION)
    // ���[�U�C���^�t�F�[�X��`�悷��
    if (showMenu)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      ImDrawData* const imDrawData(ImGui::GetDrawData());
      if (imDrawData) ImGui_ImplOpenGL3_RenderDrawData(imDrawData);
    }
#endif

    // �c���Ă��� OpenGL �R�}���h�����s����
    glFlush();
  }
  else
  {
#if defined(IMGUI_VERSION)
    // ���[�U�C���^�t�F�[�X��`�悷��
    if (showMenu)
    {
      ImDrawData* const imDrawData(ImGui::GetDrawData());
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
void Window::resize(GLFWwindow* window, int width, int height)
{
  // ���̃C���X�^���X�� this �|�C���^�𓾂�
  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

  if (instance)
  {
    // Oculus Rift �g�p���ȊO
    if (!instance->session)
    {
      if (instance->config.display_mode == LINEBYLINE)
      {
        // VR ���̃f�B�X�v���C�ł͕\���̈�̉��������r���[�|�[�g�ɂ���
        width /= 2;

        // VR ���̃f�B�X�v���C�̃A�X�y�N�g��͕\���̈�̉������ɂȂ�
        instance->aspect = static_cast<GLfloat>(width) / static_cast<GLfloat>(height);
      }
      else
      {
        // ���T�C�Y��̃f�B�X�v���C�̂̃A�X�y�N�g������߂�
        instance->aspect = instance->config.display_aspect
          ? instance->config.display_aspect
          : static_cast<GLfloat>(width) / static_cast<GLfloat>(height);

        // ���T�C�Y��̃r���[�|�[�g��ݒ肷��
        switch (instance->config.display_mode)
        {
        case SIDEBYSIDE:

          // �E�B���h�E�̉��������r���[�|�[�g�ɂ���
          width /= 2;
          break;

        case TOPANDBOTTOM:

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
    instance->samples[0] = static_cast<GLsizei>(sqrt(instance->aspect * instance->config.camera_texture_samples));
    instance->samples[1] = instance->config.camera_texture_samples / instance->samples[0];

    // �w�i�`��p�̃��b�V���̏c���̊i�q�Ԋu�����߂�
    instance->gap[0] = 2.0f / static_cast<GLfloat>(instance->samples[0] - 1);
    instance->gap[1] = 2.0f / static_cast<GLfloat>(instance->samples[1] - 1);

    // �������e�ϊ��s������߂�
    instance->updateProjectionMatrix();

    // �g���b�N�{�[�������͈̔͂�ݒ肷��
    instance->trackball.region(static_cast<float>(width) / angleScale, static_cast<float>(height) / angleScale);
  }
}

//
// �}�E�X�{�^���𑀍삵���Ƃ��̏���
//
//   �E�}�E�X�{�^�����������Ƃ��ɃR�[���o�b�N�֐��Ƃ��ČĂяo�����
//
void Window::mouse(GLFWwindow* window, int button, int action, int mods)
{
#if defined(IMGUI_VERSION)
  // �}�E�X�J�[�\���� ImGui �̃E�B���h�E��ɂ������� Window �N���X�̃}�E�X�ʒu���X�V���Ȃ�
  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;
#endif

  // ���̃C���X�^���X�� this �|�C���^�𓾂�
  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

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
        instance->trackball.begin(float(x), float(y));
      }
      else
      {
        // �g���b�N�{�[�������I��
        instance->trackball.end(float(x), float(y));
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
void Window::wheel(GLFWwindow* window, double x, double y)
{
#if defined(IMGUI_VERSION)
  // �}�E�X�J�[�\���� ImGui �̃E�B���h�E��ɂ������� Window �N���X�̃}�E�X�z�C�[���̉�]�ʂ��X�V���Ȃ�
  if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;
#endif

  // ���̃C���X�^���X�� this �|�C���^�𓾂�
  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

  if (instance)
  {
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT))
    {
      // �Y�[�����𒲐�����
      const int change{ instance->zoomChange + static_cast<int>(y) };
      if (change < zoomMax)
      {
        instance->zoomChange = change;
        instance->config.display_zoom = instance->defaults.display_zoom * zoomMax / (zoomMax - change);
      }

      // �������e�ϊ��s����X�V����
      instance->updateProjectionMatrix();
    }
    else
    {
      // ���̂�O��Ɉړ�����
      const GLfloat advSpeed((fabs(instance->config.position[2]) * 5.0f + 1.0f) * wheelYStep * static_cast<GLfloat>(y));
      instance->config.position[2] += advSpeed;
    }
  }
}

//
// �L�[�{�[�h���^�C�v�������̏���
//
//   �D�L�[�{�[�h���^�C�v�������ɃR�[���o�b�N�֐��Ƃ��ČĂяo�����
//
void Window::keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
#if defined(IMGUI_VERSION)
  // ImGui ���L�[�{�[�h���g���Ƃ��̓L�[�{�[�h�̏������s��Ȃ�
  if (ImGui::GetIO().WantCaptureKeyboard) return;
#endif

  // ���̃C���X�^���X�� this �|�C���^�𓾂�
  Window* const instance{ static_cast<Window*>(glfwGetWindowUserPointer(window)) };

  if (instance)
  {
    if (action == GLFW_PRESS)
    {
      // �Ō�Ƀ^�C�v�����L�[���o���Ă���
      instance->key = key;

      // �L�[�{�[�h����ɂ�鏈��
      switch (key)
      {
      case GLFW_KEY_HOME:
      case GLFW_KEY_R:

        // �ݒ�����Z�b�g����
        instance->reset();
        instance->updateProjectionMatrix();
        break;

      case GLFW_KEY_SPACE:

        // �V�[���̕\���� ON/OFF ����
        instance->showScene = !instance->showScene;
        break;

      case GLFW_KEY_M:

        // �~���[�\���� ON/OFF ����
        instance->showMirror = !instance->showMirror;
        break;

      case GLFW_KEY_N:

        // ���j���[�\���� ON/OFF ����
        instance->showMenu = !instance->showMenu;
        break;

      case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

      case GLFW_KEY_BACKSPACE:
      case GLFW_KEY_DELETE:
        break;

      case GLFW_KEY_UP:
        break;

      case GLFW_KEY_DOWN:
        break;

      case GLFW_KEY_RIGHT:
        break;

      case GLFW_KEY_LEFT:
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
  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu
  config.circle = defaults.circle;

  // �X�N���[���̊Ԋu
  config.display_offset = defaults.display_offset;

  // �Y�[����
  config.display_zoom = defaults.display_zoom;

  // �œ_����
  config.display_focal = defaults.display_focal;

  // ����
  config.parallax = defaults.display_mode != MONO ? defaults.parallax : 0.0f;

  // �J���������̕␳�l
  config.parallax_offset = defaults.parallax_offset;
  
  // ���̂̈ʒu
  config.position = defaults.position;

  // ���̂̉�]
  trackball.reset(defaults.orientation);
}

//
// �������e�ϊ��s������߂�
//
//   �E�E�B���h�E�̃T�C�Y�ύX����J�����p�����[�^�̕ύX���ɌĂяo��
//
void Window::updateProjectionMatrix()
{
  if (session)
  {
    // Oculus Rift �\���p�� FBO ���쐬����
    for (int eye = 0; eye < ovrEye_Count; ++eye)
    {
      // Oculus Rift �̎�����擾����
      const auto& eyeFov{ hmdDesc.DefaultEyeFov[eye] };
#if OVR_PRODUCT_VERSION > 0
      layerData.Fov[eye] = eyeFov;

      const auto& fov(eyeFov);
#else
      layerData.EyeFov.Fov[eye] = eyeFov;

      // Oculus Rift �̃����Y�␳���̐ݒ�l���擾����
      eyeRenderDesc[eye] = ovr_GetRenderDesc(session, ovrEyeType(eye), eyeFov);

      // Oculus Rift �̕Жڂ̓��̈ʒu����̕ψʂ����߂�
      const auto& offset(eyeRenderDesc[eye].HmdToEyeViewOffset);
      mv[eye] = ggTranslate(-offset.x, -offset.y, -offset.z);

      const auto& fov(eyeRenderDesc[eye].Fov);
#endif

      // �Y�[����
      const auto zf{ config.display_zoom * config.display_near };

      // �Жڂ̓������e�ϊ��s������߂�
      mp[eye].loadFrustum(-fov.LeftTan * zf, fov.RightTan * zf, -fov.DownTan * zf, fov.UpTan * zf,
        config.display_near, config.display_far);
    }
  }
  else
  {
    // �Y�[����
    const auto zf{ config.display_zoom * config.display_near };

    // �X�N���[���̍����ƕ�
    const auto screenHeight{ config.display_center / config.display_distance };
    const auto screenWidth{ screenHeight * aspect };

    // �����ɂ��X�N���[���̃V�t�g��
    GLfloat shift{ config.parallax * config.display_near / config.display_distance };

    // ���ڂ̎���
    const GLfloat fovL[]
    {
      -screenWidth + shift,
      screenWidth + shift,
      -screenHeight,
      screenHeight
    };

    // ���ڂ̓������e�ϊ��s������߂�
    mp[ovrEye_Left].loadFrustum(fovL[0] * zf, fovL[1] * zf, fovL[2] * zf, fovL[3] * zf,
      config.display_near, config.display_far);

    // ���ڂ̃X�N���[���̃T�C�Y�ƒ��S�ʒu
    screen[ovrEye_Left][0] = (fovL[1] - fovL[0]) * 0.5f;
    screen[ovrEye_Left][1] = (fovL[3] - fovL[2]) * 0.5f;
    screen[ovrEye_Left][2] = (fovL[1] + fovL[0]) * 0.5f;
    screen[ovrEye_Left][3] = (fovL[3] + fovL[2]) * 0.5f;

    // Oculus Rift �ȊO�̗��̎��\���̏ꍇ
    if (config.display_mode != MONO)
    {
      // �E�ڂ̎���
      const GLfloat fovR[]
      {
        -screenWidth - shift,
        screenWidth - shift,
        -screenHeight,
        screenHeight,
      };

      // �E�̓������e�ϊ��s������߂�
      mp[ovrEye_Right].loadFrustum(fovR[0] * zf, fovR[1] * zf, fovR[2] * zf, fovR[3] * zf,
        config.display_near, config.display_far);

      // �E�ڂ̃X�N���[���̃T�C�Y�ƒ��S�ʒu
      screen[ovrEye_Right][0] = (fovR[1] - fovR[0]) * 0.5f;
      screen[ovrEye_Right][1] = (fovR[3] - fovR[2]) * 0.5f;
      screen[ovrEye_Right][2] = (fovR[1] + fovR[0]) * 0.5f;
      screen[ovrEye_Right][3] = (fovR[3] + fovR[2]) * 0.5f;
    }
  }
}

// �J���������̒����X�e�b�v
std::array<GgQuaternion, camCount> Window::qrStep;

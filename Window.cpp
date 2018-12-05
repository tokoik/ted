//
// �E�B���h�E�֘A�̏���
//
#include "Window.h"

// ���L������
#include "SharedMemory.h"

// Oculus Rift �̖ڂ̐��Ǝ��ʎq
const int eyeCount(ovrEye_Count);

// Oculus Rift SDK ���C�u���� (LibOVR) �̑g�ݍ���
#if defined(_WIN32)
#  pragma comment(lib, "libOVR.lib")
#endif

// �ݒ�t�@�C���� JSON �œǂݏ�������
#include "picojson.h"

// �W�����C�u����
#include <iostream>
#include <fstream>

// �W���C�X�e�B�b�N�ԍ��̍ő�l
const int maxJoystick(4);

#if OVR_PRODUCT_VERSION > 0
// GetDefaultAdapterLuid �̂���
#if defined(_WIN32)
#include <dxgi.h>
#  pragma comment(lib, "dxgi.lib")
#endif

static ovrGraphicsLuid GetDefaultAdapterLuid()
{
  ovrGraphicsLuid luid = ovrGraphicsLuid();

#if defined(_WIN32)
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
#endif

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
Window::Window(int width, int height, const char *title, GLFWmonitor *monitor, GLFWwindow *share)
  : window(nullptr), camera(nullptr), session(nullptr)
  , showScene(true), showMirror(true)
  , oculusFbo{ 0, 0 }, mirrorFbo(0), mirrorTexture(nullptr)
  , zoomChange(0), focalChange(0), circleChange{ 0, 0, 0, 0 }
  , key(GLFW_KEY_UNKNOWN), joy(-1)
#if OVR_PRODUCT_VERSION > 0
  , frameIndex(0LL)
  , oculusDepth{ 0, 0 }
#endif
{
  // �ŏ��̃C���X�^���X�̂Ƃ����� true
  static bool firstTime(true);

  // �ŏ��̃C���X�^���X�ň�x�������s
  if (firstTime)
  {
    // Oculus Rift �ɕ\������Ƃ�
    if (defaults.display_mode == OCULUS)
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
      glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);
    }
    else if (defaults.display_mode == QUADBUFFER)
    {
      // �N���b�h�o�b�t�@�X�e���I���[�h�ɂ���
      glfwWindowHint(GLFW_STEREO, GL_TRUE);
    }

    // SRGB ���[�h�Ń����_�����O����
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);

    // ���s�ς݂̈������
    firstTime = false;
  }

  // GLFW �̃E�B���h�E���J��
  const_cast<GLFWwindow *>(window) = glfwCreateWindow(width, height, title, monitor, share);

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

  // �����ݒ��ǂݍ���
  std::ifstream attitude("attitude.json");
  if (attitude)
  {
    // �ݒ���e�̓ǂݍ���
    picojson::value v;
    attitude >> v;
    attitude.close();

    // �ݒ���e�̃p�[�X
    picojson::object &o(v.get<picojson::object>());

    // �����ʒu
    const auto &v_position(o.find("position"));
    if (v_position != o.end() && v_position->second.is<picojson::array>())
    {
      picojson::array &p(v_position->second.get<picojson::array>());
      for (int i = 0; i < 3; ++i) startPosition[i] = static_cast<GLfloat>(p[i].get<double>());
    }

    // �����p��
    const auto &v_orientation(o.find("orientation"));
    if (v_orientation != o.end() && v_orientation->second.is<picojson::array>())
    {
      picojson::array &a(v_orientation->second.get<picojson::array>());
      for (int i = 0; i < 4; ++i) startOrientation[i] = static_cast<GLfloat>(a[i].get<double>());
    }

    // �����Y�[����
    const auto &v_zoom(o.find("zoom"));
    if (v_zoom != o.end() && v_zoom->second.is<double>())
      zoomChange = static_cast<int>(v_zoom->second.get<double>());

    // �œ_�����̏����l
    const auto &v_focal(o.find("focal"));
    if (v_focal != o.end() && v_focal->second.is<double>())
      focalChange = static_cast<int>(v_focal->second.get<double>());

    // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu�̏����l
    const auto &v_circle(o.find("circle"));
    if (v_circle != o.end() && v_circle->second.is<picojson::array>())
    {
      picojson::array &c(v_circle->second.get<picojson::array>());
      for (int i = 0; i < 4; ++i) circleChange[i] = static_cast<int>(c[i].get<double>());
    }

    // �����̏����l
    const auto &v_parallax(o.find("parallax"));
    if (v_parallax != o.end() && v_parallax->second.is<double>())
      initialParallax = static_cast<GLfloat>(v_parallax->second.get<double>());
  }

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
  glfwSwapInterval(defaults.display_mode == OCULUS ? 0 : 1);

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

#  if DEBUG
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
    for (int eye = 0; eye < eyeCount; ++eye)
    {
      // Oculus Rift �̎�����擾����
      const auto &eyeFov(hmdDesc.DefaultEyeFov[eye]);
#if OVR_PRODUCT_VERSION > 0
      layerData.Fov[eye] = eyeFov;

      const auto &fov(eyeFov);
#else
      layerData.EyeFov.Fov[eye] = eyeFov;

      // Oculus Rift �̃����Y�␳���̐ݒ�l���擾����
      eyeRenderDesc[eye] = ovr_GetRenderDesc(session, ovrEyeType(eye), eyeFov);

      // Oculus Rift �̕Жڂ̓��̈ʒu����̕ψʂ����߂�
      const auto &offset(eyeRenderDesc[eye].HmdToEyeViewOffset);
      mv[eye] = ggTranslate(-offset.x, -offset.y, -offset.z);

      const auto &fov(eyeRenderDesc[eye].Fov);
#endif

      // �Жڂ̓������e�ϊ��s������߂�
      mp[eye].loadFrustum(-fov.LeftTan * defaults.display_near, fov.RightTan * defaults.display_near,
        -fov.DownTan * defaults.display_near, fov.UpTan * defaults.display_near,
        defaults.display_near, defaults.display_far);

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
      ovrSwapTextureSet *colorTexture;
      ovr_CreateSwapTextureSetGL(session, GL_SRGB8_ALPHA8, size.w, size.h, &colorTexture);
      layerData.EyeFov.ColorTexture[eye] = colorTexture;

      // Oculus Rift �\���p�� FBO �̃f�v�X�o�b�t�@�Ƃ��Ďg���e�N�X�`���Z�b�g�̍쐬
      ovrSwapTextureSet *depthTexture;
      ovr_CreateSwapTextureSetGL(session, GL_DEPTH_COMPONENT32F, size.w, size.h, &depthTexture);
      layerData.EyeFovDepth.DepthTexture[eye] = depthTexture;
#endif
    }

#if OVR_PRODUCT_VERSION > 0
    // Oculus Rift �̉�ʂ̃A�X�y�N�g������߂�
    aspect = static_cast<GLfloat>(layerData.Viewport[ovrEye_Left].Size.w)
      / static_cast<GLfloat>(layerData.Viewport[ovrEye_Left].Size.h);

    // �~���[�\���p�� FBO ���쐬����
    const ovrMirrorTextureDesc mirrorDesc =
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
    if (OVR_SUCCESS(ovr_CreateMirrorTextureGL(session, GL_SRGB8_ALPHA8, width, height, reinterpret_cast<ovrTexture **>(&mirrorTexture))))
    {
      glGenFramebuffers(1, &mirrorFbo);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFbo);
      glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTexture->OGL.TexId, 0);
      glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
      glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

    // TimeWarp �Ɏg���ϊ��s��̐��������o��
    auto &posTimewarpProjectionDesc(layerData.EyeFovDepth.ProjectionDesc);
    posTimewarpProjectionDesc.Projection22 = (mp[ovrEye_Left].get()[4 * 2 + 2] + mp[ovrEye_Left].get()[4 * 3 + 2]) * 0.5f;
    posTimewarpProjectionDesc.Projection23 = mp[ovrEye_Left].get()[4 * 2 + 3] * 0.5f;
    posTimewarpProjectionDesc.Projection32 = mp[ovrEye_Left].get()[4 * 3 + 2];
#endif

    // Oculus Rift �̃����_�����O�p�� FBO ���쐬����
    glGenFramebuffers(eyeCount, oculusFbo);
  }

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

  // �ݒ�l��ۑ�����
  std::ofstream attitude("attitude.json");
  if (attitude)
  {
    picojson::object o;

    // �ʒu
    picojson::array p(3);
    p[0] = picojson::value(ox);
    p[1] = picojson::value(oy);
    p[2] = picojson::value(oz);
    o.insert(std::make_pair("position", picojson::value(p)));

    // �p��
    picojson::array a;
    for (int i = 0; i < 4; ++i) a.push_back(picojson::value(trackball.getQuaternion().get()[i]));
    o.insert(std::make_pair("orientation", picojson::value(a)));

    // �Y�[����
    o.insert(std::make_pair("zoom", picojson::value(static_cast<double>(zoomChange))));

    // �œ_����
    o.insert(std::make_pair("focal", picojson::value(static_cast<double>(focalChange))));

    // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu
    picojson::array c;
    for (int i = 0; i < 4; ++i) c.push_back(picojson::value(static_cast<double>(circleChange[i])));
    o.insert(std::make_pair("circle", picojson::value(c)));

    // ����
    o.insert(std::make_pair("parallax", picojson::value(defaults.display_mode != MONO ? parallax : initialParallax)));

    // �ݒ���e���V���A���C�Y���ĕۑ�
    picojson::value v(o);
    attitude << v.serialize(true);
    attitude.close();
  }

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
      ovr_DestroyMirrorTexture(session, reinterpret_cast<ovrTexture *>(mirrorTexture));
    }
#endif

    // Oculus Rift �̃����_�����O�p�� FBO ���폜����
    glDeleteFramebuffers(eyeCount, oculusFbo);

    // Oculus Rift �\���p�� FBO ���폜����
    for (int eye = 0; eye < eyeCount; ++eye)
    {
      // �����_�����O�^�[�Q�b�g�Ɏg�����e�N�X�`�����J������
#if OVR_PRODUCT_VERSION > 0
      if (layerData.ColorTexture[eye])
      {
        ovr_DestroyTextureSwapChain(session, layerData.ColorTexture[eye]);
        layerData.ColorTexture[eye] = nullptr;
      }
#else
      auto *const colorTexture(layerData.EyeFov.ColorTexture[eye]);
      for (int i = 0; i < colorTexture->TextureCount; ++i)
      {
        const auto *const ctex(reinterpret_cast<ovrGLTexture *>(&colorTexture->Textures[i]));
        glDeleteTextures(1, &ctex->OGL.TexId);
      }
      ovr_DestroySwapTextureSet(session, colorTexture);
#endif

      // �f�v�X�o�b�t�@�Ƃ��Ďg�����e�N�X�`�����J������
#if OVR_PRODUCT_VERSION > 0
      glDeleteTextures(1, &oculusDepth[eye]);
      oculusDepth[eye] = 0;
#else
      auto *const depthTexture(layerData.EyeFovDepth.DepthTexture[eye]);
      for (int i = 0; i < depthTexture->TextureCount; ++i)
      {
        const auto *const dtex(reinterpret_cast<ovrGLTexture *>(&depthTexture->Textures[i]));
        glDeleteTextures(1, &dtex->OGL.TexId);
      }
      ovr_DestroySwapTextureSet(session, depthTexture);
#endif
    }

    // Oculus Rift �̃Z�b�V������j������
    ovr_Destroy(session);
    const_cast<ovrSession>(session) = nullptr;
  }

  // �\���p�̃E�B���h�E��j������
  glfwDestroyWindow(window);
}

//
// �`��J�n
//
//   �E�}�`�̕`��J�n�O�ɌĂяo��
//
bool Window::start()
{
  // ���f���ϊ��s���ݒ肷��
  mm = ggTranslate(ox, oy, -oz) * trackball.getMatrix();

  // ���f���ϊ��s������L�������ɕۑ�����
  localAttitude->set(camCount, mm);

  // Oculus Rift �g�p��
  if (session)
  {
#if OVR_PRODUCT_VERSION > 0
    // �Z�b�V�����̏�Ԃ��擾����
    ovrSessionStatus sessionStatus;
    ovr_GetSessionStatus(session, &sessionStatus);

    // �A�v���P�[�V�������I����v�����Ă���Ƃ��̓E�B���h�E�̃N���[�Y�t���O�𗧂Ă�
    if (sessionStatus.ShouldQuit) glfwSetWindowShouldClose(window, GL_TRUE);

    // ���݂̏�Ԃ��g���b�L���O�̌��_�ɂ���
    if (sessionStatus.ShouldRecenter) ovr_RecenterTrackingOrigin(session);

    // HmdToEyeOffset �Ȃǂ͎��s���ɕω�����̂Ŗ��t���[�� ovr_GetRenderDesc() �� ovrEyeRenderDesc ���擾����
    const ovrEyeRenderDesc eyeRenderDesc[] =
    {
      ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]),
      ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1])
    };

    // Oculus Rift �̍��E�̖ڂ̃g���b�L���O�̈ʒu����̕ψʂ����߂�
    const ovrPosef hmdToEyePose[] =
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
    const ovrVector3f hmdToEyeViewOffset[] =
    {
      eyeRenderDesc[0].HmdToEyeViewOffset,
      eyeRenderDesc[1].HmdToEyeViewOffset
    };

    // ���_�̎p���������߂�
    ovr_CalcEyePoses(hmdState.HeadPose.ThePose, hmdToEyeViewOffset, eyePose);
#endif
  }

  return true;
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
#if DEBUG
  // �G���[�`�F�b�N
  ggError(__FILE__, __LINE__);
#endif

  // Oculus Rift �g�p��
  if (session)
  {
#if OVR_PRODUCT_VERSION > 0
    // �`��f�[�^�� Oculus Rift �ɓ]������
    const auto *const layers(&layerData.Header);
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
    const auto *const layers(&layerData.Header);
    if (OVR_FAILURE(ovr_SubmitFrame(session, 0, &viewScaleDesc, &layers, 1)))
#endif
    {
      // �]���Ɏ��s������ Oculus Rift �̐ݒ���ŏ������蒼���K�v������炵��
      NOTIFY("Oculus Rift �ւ̃f�[�^�̓]���Ɏ��s���܂����B");

      // ���ǂ߂�ǂ������̂ŃE�B���h�E����Ă��܂�
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (showMirror)
    {
      // �����_�����O���ʂ��~���[�\���p�̃t���[���o�b�t�@�ɂ��]������
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

    // �c���Ă��� OpenGL �R�}���h�����s����
    glFlush();
  }
  else
  {
    // �J���[�o�b�t�@�����ւ���
    glfwSwapBuffers(window);
  }

  // �C�x���g�����o��
  glfwPollEvents();

  //
  // �}�E�X�ɂ�鑀��
  //

  // �}�E�X�̈ʒu�𒲂ׂ�
  double x, y;
  glfwGetCursorPos(window, &x, &y);

  // ���x�������ɔ�Ⴓ����
  const auto speedFactor((fabs(oz) + 0.2f));

  // ���{�^���h���b�O
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1))
  {
    // ���̂̈ʒu���ړ�����
    const auto speed(speedScale * speedFactor);
    ox += static_cast<GLfloat>(x - cx) * speed;
    oy += static_cast<GLfloat>(cy - y) * speed;
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

  // �V�t�g�L�[�̏��
  const auto shiftKey(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT));

  // �R���g���[���L�[�̏��
  const auto ctrlKey(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL));

  // �E���L�[����
  if (glfwGetKey(window, GLFW_KEY_RIGHT))
  {
    if (ctrlKey)
    {
      // �w�i�ɑ΂��鉡�����̉�p���L����
      circle[0] = defaults.fisheye_fov_x + static_cast<GLfloat>(++circleChange[0]) * shiftStep;
    }
    else if (shiftKey)
    {
      // �w�i���E�ɂ��炷
      circle[2] = defaults.fisheye_center_x + static_cast<GLfloat>(++circleChange[2]) * shiftStep;
    }
    else if (defaults.display_mode != MONO)
    {
      // �������g�傷��
      parallax += parallaxStep;
      updateProjectionMatrix();
    }
  }

  // �����L�[����
  if (glfwGetKey(window, GLFW_KEY_LEFT))
  {
    if (ctrlKey)
    {
      // �w�i�ɑ΂��鉡�����̉�p�����߂�
      circle[0] = defaults.fisheye_fov_x + static_cast<GLfloat>(--circleChange[0]) * shiftStep;
    }
    else if (shiftKey)
    {
      // �w�i�����ɂ��炷
      circle[2] = defaults.fisheye_center_x + static_cast<GLfloat>(--circleChange[2]) * shiftStep;
    }
    else if (defaults.display_mode != MONO)
    {
      // �������k������
      parallax -= parallaxStep;
      updateProjectionMatrix();
    }
  }

  // ����L�[����
  if (glfwGetKey(window, GLFW_KEY_UP))
  {
    if (ctrlKey)
    {
      // �w�i�ɑ΂���c�����̉�p���L����
      circle[1] = defaults.fisheye_fov_y + static_cast<GLfloat>(++circleChange[1]) * shiftStep;
    }
    else if (shiftKey)
    {
      // �w�i����ɂ��炷
      circle[3] = defaults.fisheye_center_y + static_cast<GLfloat>(++circleChange[3]) * shiftStep;
    }
    else
    {
      // �œ_���������΂�
      focal = focalStep / (focalStep - static_cast<GLfloat>(++focalChange));
    }
  }

  // �����L�[����
  if (glfwGetKey(window, GLFW_KEY_DOWN))
  {
    if (ctrlKey)
    {
      // �w�i�ɑ΂���c�����̉�p�����߂�
      circle[1] = defaults.fisheye_fov_y + static_cast<GLfloat>(--circleChange[1]) * shiftStep;
    }
    else if (shiftKey)
    {
      // �w�i�����ɂ��炷
      circle[3] = defaults.fisheye_center_y + static_cast<GLfloat>(--circleChange[3]) * shiftStep;
    }
    else
    {
      // �œ_�������k�߂�
      focal = focalStep / (focalStep - static_cast<GLfloat>(--focalChange));
    }
  }

  // �J�����̐���
  if (camera)
  {
    // E �L�[
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

    // G �L�[
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

  //
  // �W���C�X�e�B�b�N�ɂ�鑀��
  //

  // �W���C�X�e�B�b�N���L���Ȃ�
  if (joy >= 0)
  {
    // �{�^��
    int btnsCount;
    const auto *const btns(glfwGetJoystickButtons(joy, &btnsCount));

    // �X�e�B�b�N
    int axesCount;
    const auto *const axes(glfwGetJoystickAxes(joy, &axesCount));

    // �X�e�B�b�N�̑��x�W��
    const auto axesSpeedFactor(axesSpeedScale * speedFactor);

    if (axesCount > 3)
    {
      // ���̂����E�Ɉړ�����
      ox += (axes[0] - origin[0]) * axesSpeedFactor;

      // RB �{�^���𓯎��ɉ����Ă����
      if (btns[5])
      {
        // ���̂�O��Ɉړ�����
        oz += (axes[1] - origin[1]) * axesSpeedFactor;
      }
      else
      {
        // ���̂��㉺�Ɉړ�����
        oy += (axes[1] - origin[1]) * axesSpeedFactor;
      }

      // ���̂����E�ɉ�]����
      trackball.rotate(ggRotateQuaternion(0.0f, 1.0f, 0.0f, (axes[2] - origin[2]) * axesSpeedFactor));

      // ���̂��㉺�ɉ�]����
      trackball.rotate(ggRotateQuaternion(1.0f, 0.0f, 0.0f, (axes[3] - origin[3]) * axesSpeedFactor));
    }

    // B, X �{�^���̏��
    const auto zoomButton(btns[1] - btns[2]);

    // Y, A �{�^���̏��
    const auto parallaxButton(btns[3] - btns[0]);

    // B, X �{�^���ɕω��������
    if (parallaxButton)
    {
      // �����𒲐�����
      parallax += parallaxStep * static_cast<GLfloat>(parallaxButton);
      updateProjectionMatrix();
    }

    // Y, A �{�^���ɕω��������
    if (zoomButton)
    {
      // �Y�[�����𒲐�����
      zoom = (defaults.display_zoom != 0.0f ? 1.0f / defaults.display_zoom : 1.0f)
        + static_cast<GLfloat>(zoomChange += zoomButton) * zoomStep;

      // �������e�ϊ��s����X�V����
      updateProjectionMatrix();
    }

    // �\���L�[�̍��E�{�^���̏��
    const auto textureXButton(btns[11] - btns[13]);

    // �\���L�[�̍��E�{�^���ɕω��������
    if (textureXButton)
    {
      // RB �{�^���𓯎��ɉ����Ă����
      if (btns[5])
      {
        // �w�i�ɑ΂��鉡�����̉�p�𒲐�����
        circle[0] = defaults.fisheye_fov_x + static_cast<GLfloat>(circleChange[0] += textureXButton) * shiftStep;
      }
      else
      {
        // �w�i�̉��ʒu�𒲐�����
        circle[2] = defaults.fisheye_center_x + static_cast<GLfloat>(circleChange[2] += textureXButton) * shiftStep;
      }
    }

    // �\���L�[�̏㉺�{�^���̏��
    const auto textureYButton(btns[10] - btns[12]);

    // �\���L�[�̏㉺�{�^���ɕω��������
    if (textureYButton)
    {
      // RB �{�^���𓯎��ɉ����Ă����
      if (btns[5])
      {
        // �w�i�ɑ΂���c�����̉�p�𒲐�����
        circle[1] = defaults.fisheye_fov_y + static_cast<GLfloat>(circleChange[1] += textureXButton) * shiftStep;
      }
      else
      {
        // �w�i�̏c�ʒu�𒲐�����
        circle[3] = defaults.fisheye_center_y + static_cast<GLfloat>(circleChange[3] += textureYButton) * shiftStep;
      }
    }

    // �ݒ�����Z�b�g����
    if (btns[7])
    {
      reset();
      updateProjectionMatrix();
    }
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
    // Oculus Rift �g�p���ȊO
    if (!instance->session)
    {
      if (defaults.display_mode == LINEBYLINE)
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
        case SIDEBYSIDE:

          // �E�B���h�E�̉��������r���[�|�[�g�ɂ���
          width /= 2;
          break;

        case TOPANDBOTTOM:

          // �E�B���h�E�̏c�������r���[�|�[�g�ɂ���
          height /= 2;
          break;

        default:

          // �E�B���h�E�S�̂��r���[�|�[�g�ɂ��Ă���
          glViewport(0, 0, width, height);
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
void Window::mouse(GLFWwindow *window, int button, int action, int mods)
{
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
        instance->trackball.start(float(x), float(y));
      }
      else
      {
        // �g���b�N�{�[�������I��
        instance->trackball.stop(float(x), float(y));
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
  // ���̃C���X�^���X�� this �|�C���^�𓾂�
  Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

  if (instance)
  {
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT))
    {
      // �Y�[�����𒲐�����
      instance->zoom = (defaults.display_zoom != 0.0f ? 1.0f / defaults.display_zoom : 1.0f)
        + static_cast<GLfloat>(instance->zoomChange += static_cast<int>(y)) * zoomStep;

      // �������e�ϊ��s����X�V����
      instance->updateProjectionMatrix();
    }
    else
    {
      // ���̂�O��Ɉړ�����
      const GLfloat advSpeed((fabs(instance->oz) * 5.0f + 1.0f) * wheelYStep * static_cast<GLfloat>(y));
      instance->oz += advSpeed;
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
  // ���̃C���X�^���X�� this �|�C���^�𓾂�
  Window *const instance(static_cast<Window *>(glfwGetWindowUserPointer(window)));

  if (instance)
  {
    if (action == GLFW_PRESS)
    {
      // �Ō�Ƀ^�C�v�����L�[���o���Ă���
      instance->key = key;

      // �L�[�{�[�h����ɂ�鏈��
      switch (key)
      {
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
  // ���̂̈ʒu
  ox = startPosition[0];
  oy = startPosition[1];
  oz = startPosition[2];

  // ���̂̉�]
  trackball.reset();
  trackball.rotate(ggQuaternion(startOrientation));

  // �Y�[����
  zoom = (defaults.display_zoom != 0.0f ? 1.0f / defaults.display_zoom : 1.0f)
    + static_cast<GLfloat>(zoomChange) * zoomStep;

  // �œ_����
  focal = focalStep / (focalStep - static_cast<GLfloat>(focalChange));

  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu
  circle[0] = defaults.fisheye_fov_x + static_cast<GLfloat>(circleChange[0]) * shiftStep;
  circle[1] = defaults.fisheye_fov_y + static_cast<GLfloat>(circleChange[1]) * shiftStep;
  circle[2] = defaults.fisheye_center_x + static_cast<GLfloat>(circleChange[2]) * shiftStep;
  circle[3] = defaults.fisheye_center_y + static_cast<GLfloat>(circleChange[3]) * shiftStep;

  // ����
  parallax = defaults.display_mode != MONO ? initialParallax : 0.0f;
}

//
// �������e�ϊ��s������߂�
//
//   �E�E�B���h�E�̃T�C�Y�ύX����J�����p�����[�^�̕ύX���ɌĂяo��
//
void Window::updateProjectionMatrix()
{
  // Oculus Rift ��g�p��
  if (!session)
  {
    // �Y�[����
    const auto zf(zoom * defaults.display_near);

    // �X�N���[���̍����ƕ�
    const auto screenHeight(defaults.display_center / defaults.display_distance);
    const auto screenWidth(screenHeight * aspect);

    // �����ɂ��X�N���[���̃I�t�Z�b�g��
    const auto offset(parallax * defaults.display_near / defaults.display_distance);

    // ���ڂ̎���
    const GLfloat fovL[] =
    {
      -screenWidth + offset,
      screenWidth + offset,
      -screenHeight,
      screenHeight
    };

    // ���ڂ̓������e�ϊ��s������߂�
    mp[ovrEye_Left].loadFrustum(fovL[0] * zf, fovL[1] * zf, fovL[2] * zf, fovL[3] * zf,
      defaults.display_near, defaults.display_far);

    // ���ڂ̃X�N���[���̃T�C�Y�ƒ��S�ʒu
    screen[ovrEye_Left][0] = (fovL[1] - fovL[0]) * 0.5f;
    screen[ovrEye_Left][1] = (fovL[3] - fovL[2]) * 0.5f;
    screen[ovrEye_Left][2] = (fovL[1] + fovL[0]) * 0.5f;
    screen[ovrEye_Left][3] = (fovL[3] + fovL[2]) * 0.5f;

    // Oculus Rift �ȊO�̗��̎��\���̏ꍇ
    if (defaults.display_mode != MONO)
    {
      // �E�ڂ̎���
      const GLfloat fovR[] =
      {
        -screenWidth - offset,
        screenWidth - offset,
        -screenHeight,
        screenHeight,
      };

      // �E�̓������e�ϊ��s������߂�
      mp[ovrEye_Right].loadFrustum(fovR[0] * zf, fovR[1] * zf, fovR[2] * zf, fovR[3] * zf,
        defaults.display_near, defaults.display_far);

      // �E�ڂ̃X�N���[���̃T�C�Y�ƒ��S�ʒu
      screen[ovrEye_Right][0] = (fovR[1] - fovR[0]) * 0.5f;
      screen[ovrEye_Right][1] = (fovR[3] - fovR[2]) * 0.5f;
      screen[ovrEye_Right][2] = (fovR[1] + fovR[0]) * 0.5f;
      screen[ovrEye_Right][3] = (fovR[3] + fovR[2]) * 0.5f;
    }
  }
}

//
// �}�`��������ڂ�I������
//
void Window::select(int eye)
{
  // Oculus Rift �g�p��
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
      const auto &vp(layerData.Viewport[eye]);
      glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);
    }

    // Oculus Rift �̕Жڂ̈ʒu�Ɖ�]���擾����
    const auto &o(layerData.RenderPose[eye].Orientation);
    const auto &p(layerData.RenderPose[eye].Position);
#else
    // �����_�[�^�[�Q�b�g�ɕ`�悷��O�Ƀ����_�[�^�[�Q�b�g�̃C���f�b�N�X���C���N�������g����
    auto *const colorTexture(layerData.EyeFov.ColorTexture[eye]);
    colorTexture->CurrentIndex = (colorTexture->CurrentIndex + 1) % colorTexture->TextureCount;
    auto *const depthTexture(layerData.EyeFovDepth.DepthTexture[eye]);
    depthTexture->CurrentIndex = (depthTexture->CurrentIndex + 1) % depthTexture->TextureCount;

    // �����_�[�^�[�Q�b�g��؂�ւ���
    glBindFramebuffer(GL_FRAMEBUFFER, oculusFbo[eye]);
    const auto &ctex(reinterpret_cast<ovrGLTexture *>(&colorTexture->Textures[colorTexture->CurrentIndex]));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctex->OGL.TexId, 0);
    const auto &dtex(reinterpret_cast<ovrGLTexture *>(&depthTexture->Textures[depthTexture->CurrentIndex]));
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dtex->OGL.TexId, 0);

    // �r���[�|�[�g��ݒ肷��
    const auto &vp(layerData.EyeFov.Viewport[eye]);
    glViewport(vp.Pos.x, vp.Pos.y, vp.Size.w, vp.Size.h);

    // Oculus Rift �̕Жڂ̈ʒu�Ɖ�]���擾����
    const auto &p(eyePose[eye].Position);
    const auto &o(eyePose[eye].Orientation);
#endif

    // Oculus Rift �̕Жڂ̈ʒu��ۑ�����
    po[eye][0] = p.x;
    po[eye][1] = p.y;
    po[eye][2] = p.z;
    po[eye][3] = 1.0f;

    // Oculus Rift �̕Жڂ̉�]��ۑ�����
    qo[eye] = GgQuaternion(o.x, o.y, o.z, -o.w);

    // �J���[�o�b�t�@�ƃf�v�X�o�b�t�@����������
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return;
  }

  // Oculus Rift �ȊO�ɕ\������Ƃ�
  switch (defaults.display_mode)
  {
  case TOPANDBOTTOM:

    if (eye == camL)
    {
      // �f�B�X�v���C�̏㔼�������ɕ`�悷��
      glViewport(0, height, width, height);

      // �J���[�o�b�t�@�ƃf�v�X�o�b�t�@����������
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    else
    {
      // �f�B�X�v���C�̉����������ɕ`�悷��
      glViewport(0, 0, width, height);
    }
    break;

  case SIDEBYSIDE:

    if (eye == camL)
    {
      // �f�B�X�v���C�̍����������ɕ`�悷��
      glViewport(0, 0, width, height);

      // �J���[�o�b�t�@�ƃf�v�X�o�b�t�@����������
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

    // �J���[�o�b�t�@�ƃf�v�X�o�b�t�@����������
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    break;

  default:
    break;
  }

  // �w�b�h�g���b�L���O�ɂ�鎋���̉�]���s��Ȃ�
  qo[eye] = ggIdentityQuaternion();

  // �ڂ����炷����ɃV�[���𓮂���
  mv[eye] = ggTranslate(eye == camL ? parallax : -parallax, 0.0f, 0.0f);
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
// Oculus Rift �̃w�b�h���b�L���O�ɂ���]�̕ϊ��s��𓾂�
//
GgMatrix Window::getMo(int eye) const
{
  // �l��������ϊ��s������߂�
  GgMatrix mo(qo[eye].getMatrix());

  // ���s�ړ��𔽉f����

  // ���[�J���̃w�b�h�g���b�L���O�������L�������ɕۑ�����
  localAttitude->set(eye, mo.transpose());

  return mo;
}

// ���̂̏����ʒu�Ǝp��
GLfloat Window::startPosition[] = { 0.0f, 0.0f, 0.0f };
GLfloat Window::startOrientation[] = { 0.0f, 0.0f, 0.0f, 1.0f };

// �����̏����l (�P�� m)
GLfloat Window::initialParallax(0.032f);

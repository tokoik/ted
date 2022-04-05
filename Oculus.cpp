//
// �E�B���h�E�֘A�̏���
//
#include "Window.h"

// Oculus Rift �֘A�̏���
#include "Oculus.h"

// �p��
#include "Attitude.h"

// Oculus SDK ���C�u���� (LibOVR) �̑g�ݍ���
#if defined(_WIN32)
// �R���t�B�M�����[�V�����𒲂ׂ�
#  if defined(_DEBUG)
// �f�o�b�O�r���h�̃��C�u�����������N����
#    pragma comment(lib, "libOVRd.lib")
#  else
// �����[�X�r���h�̃��C�u�����������N����
#    pragma comment(lib, "libOVR.lib")
#  endif
// LibOVR 1.0 �ȍ~�Ȃ�
#  if OVR_PRODUCT_VERSION >= 1
// GetDefaultAdapterLuid ���g�����߂� dxgi.lib �������N����
#    include <dxgi.h>
#    pragma comment(lib, "dxgi.lib")

//
// �f�t�H���g�̃O���t�B�b�N�A�_�v�^�� LUID �𒲂ׂ�
//
inline ovrGraphicsLuid GetDefaultAdapterLuid()
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

// �g�p���Ă���O���t�B�b�N�X�A�_�v�^���f�t�H���g�Ɣ�r����
inline int Compare(const ovrGraphicsLuid& lhs, const ovrGraphicsLuid& rhs)
{
  return memcmp(&lhs, &rhs, sizeof(ovrGraphicsLuid));
}
#  endif
#endif

//
// �R���X�g���N�^
//
Oculus::Oculus()
  : oculusFbo{ 0 }
  , mirrorFbo{ 0 }
  , mirrorTexture{ nullptr }
#if OVR_PRODUCT_VERSION >= 1
  , frameIndex{ 0LL }
  , oculusDepth{ 0 }
  , mirrorWidth{ 1280 }
  , mirrorHeight{ 640 }
#endif
{
}

//
// Oculus Rift ������������.
//
//   window Oculus Rift �Ɋ֘A�t����E�B���h�E.
//   zoom �V�[���̃Y�[����.
//   aspect Oculus Rift �̃A�X�y�N�g��̊i�[��̃|�C���^.
//   mp �������e�ϊ��s��.
//   screen �w�i�ɑ΂���X�N���[���̃T�C�Y.
//   �߂�l Oculus Rift �̏������ɐ���������R���e�L�X�g�̃|�C���^.
//
Oculus* Oculus::initialize(GLfloat zoom, GLfloat* aspect, GgMatrix* mp, GgVector* screen)
{
  // Oculus Rift �̃R���e�L�X�g
  static Oculus oculus;

  // Oculus Rift �̃Z�b�V����
  auto& session{ oculus.session };

  // ���� Oculus Rift �̃Z�b�V�������쐬����Ă���Ή������Ȃ��Ŗ߂�
  if (session) return &oculus;

  // ��x�������s����
  static bool firstTime{ true };
  if (firstTime)
  {
    // Oculus Rift (LibOVR) ���������p�����[�^
    ovrInitParams initParams{ ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0 };

    // Oculus Rift �̏������Ɏ��s������߂�
    if (OVR_FAILURE(ovr_Initialize(&initParams))) return nullptr;

    // �v���O�����I�����ɂ� LibOVR ���I������
    atexit(ovr_Shutdown);

    // ���s�ς݂̈������
    firstTime = false;
  }

  // Oculus Rift �̃Z�b�V�������쐬����
  ovrGraphicsLuid luid;
  if (OVR_FAILURE(ovr_Create(&session, &luid))) return nullptr;

#if OVR_PRODUCT_VERSION >= 1
  // �f�t�H���g�̃O���t�B�b�N�X�A�_�v�^���g���Ă��邩�m���߂�
  if (Compare(luid, GetDefaultAdapterLuid())) return nullptr;
#else
  // (LUID �� OpenGL �ł͎g���Ă��Ȃ��炵��)
#endif

  // Oculus Rift �̏������o��
  auto& hmdDesc{ oculus.hmdDesc };
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
  auto& layerData{ oculus.layerData };
#if OVR_PRODUCT_VERSION >= 1
  layerData.Header.Type = ovrLayerType_EyeFov;
#else
  layerData.Header.Type = ovrLayerType_EyeFovDepth;
#endif

  // OpenGL �Ȃ̂ō��������_
  layerData.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;

  // Oculus Rift �̃����_�����O�p�� FBO ���쐬����
  glGenFramebuffers(ovrEye_Count, oculus.oculusFbo);

#  if OVR_PRODUCT_VERSION >= 1
  // FBO �̃f�v�X�o�b�t�@�Ƃ��Ďg���e�N�X�`�����쐬����
  glGenTextures(ovrEye_Count, oculus.oculusDepth);
#  endif

  // �O���ʂ� defaults.display_near �Ȃ̂ŃX�N���[��������ɍ��킹��
  const GLfloat zf{ defaults.display_near / zoom };

  // Oculus Rift �\���p�� FBO ���쐬����
  for (int eye = 0; eye < ovrEye_Count; ++eye)
  {
    // Oculus Rift �̎�����擾����
    const auto &eyeFov(hmdDesc.DefaultEyeFov[eye]);
#if OVR_PRODUCT_VERSION >= 1
    layerData.Fov[eye] = eyeFov;

    // Oculus Rift �̎��ۂ̎���
    const auto& fov(eyeFov);
#else
    layerData.EyeFov.Fov[eye] = eyeFov;

    // Oculus Rift �̃����Y�␳���̐ݒ�l���擾����
    auto& eyeRenderDesc{ oculus.eyeRenderDesc };
    eyeRenderDesc[eye] = ovr_GetRenderDesc(session, ovrEyeType(eye), eyeFov);

    // Oculus Rift �̕Жڂ̓��̈ʒu����̕ψʂ����߂�
    const auto &offset(eyeRenderDesc[eye].HmdToEyeViewOffset);
    mv[eye] = ggTranslate(-offset.x, -offset.y, -offset.z);

    // Oculus Rift �̎��ۂ̎���
    const auto &fov(eyeRenderDesc.Fov);
#endif

    // �����̒����l
    const float p{ static_cast<float>((1 - eye * 2) * attitude.parallax) * parallaxStep };

    // �Жڂ̓������e�ϊ��s������߂�
    mp[eye].loadFrustum(
      (p - fov.LeftTan) * zf, (p + fov.RightTan) * zf,
      -fov.DownTan * zf, fov.UpTan * zf,
      defaults.display_near, defaults.display_far
    );

    // �Жڂ̃X�N���[���̃T�C�Y�ƒ��S�ʒu
    screen[eye][0] = (fov.RightTan + fov.LeftTan) * 0.5f;
    screen[eye][1] = (fov.UpTan + fov.DownTan) * 0.5f;
    screen[eye][2] = (fov.RightTan - fov.LeftTan) * 0.5f;
    screen[eye][3] = (fov.UpTan - fov.DownTan) * 0.5f;

    // Oculus Rift �\���p�� FBO �̃T�C�Y
    const auto size(ovr_GetFovTextureSize(session, ovrEyeType(eye), eyeFov, 1.0f));
#if OVR_PRODUCT_VERSION >= 1
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
    if (OVR_FAILURE(ovr_CreateTextureSwapChainGL(session,
      &colorDesc, &layerData.ColorTexture[eye]))) return false;

    // Oculus Rift �\���p�� FBO �̃J���[�o�b�t�@�Ƃ��Ďg���e�N�X�`���Z�b�g���쐬����
    int length(0);
    if (OVR_FAILURE(ovr_GetTextureSwapChainLength(session,
      layerData.ColorTexture[eye], &length))) return false;

    // �e�N�X�`���`�F�C���̌X�̗v�f�ɂ���
    for (int i = 0; i < length; ++i)
    {
      // �e�N�X�`���̃p�����[�^��ݒ肷��
      GLuint texId;
      ovr_GetTextureSwapChainBufferGL(session, layerData.ColorTexture[eye], i, &texId);
      glBindTexture(GL_TEXTURE_2D, texId);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // FBO �̃f�v�X�o�b�t�@�Ɏg���e�N�X�`���̃p�����[�^��ݒ肷��
    glBindTexture(GL_TEXTURE_2D, oculus.oculusDepth[eye]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, size.w, size.h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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

#if OVR_PRODUCT_VERSION >= 1
  // Oculus Rift �̉�ʂ̃A�X�y�N�g������߂�
  * aspect = static_cast<GLfloat>(layerData.Viewport[ovrEye_Left].Size.w)
    / static_cast<GLfloat>(layerData.Viewport[ovrEye_Left].Size.h);

  // �~���[�\���p�� FBO �J���[�o�b�t�@�Ƃ��Ďg���e�N�X�`���̓���
  const ovrMirrorTextureDesc mirrorDesc =
  {
    OVR_FORMAT_R8G8B8A8_UNORM_SRGB,                   // Format
    oculus.mirrorWidth = defaults.display_size[0],    // Width
    oculus.mirrorHeight = defaults.display_size[1],   // Height
    0
  };

  // �~���[�\���p�� FBO �̃J���[�o�b�t�@�Ƃ��Ďg���e�N�X�`�����쐬����
  if (OVR_FAILURE(ovr_CreateMirrorTextureGL(session,
    &mirrorDesc, &oculus.mirrorTexture))) return nullptr;

  // �~���[�\���p�� FBO �̃J���[�o�b�t�@�Ɏg���e�N�X�`���𓾂�
  GLuint texId;
  if (OVR_FAILURE(ovr_GetMirrorTextureBufferGL(session,
    oculus.mirrorTexture, &texId))) return nullptr;

  // �~���[�\���p�� FBO ���쐬����
  glGenFramebuffers(1, &oculus.mirrorFbo);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, oculus.mirrorFbo);
  glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
  glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

  // �p���̃g���b�L���O�ɂ����鏰�̍����� 0 �ɐݒ肷��
  ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);
#else
  // Oculus Rift �̉�ʂ̃A�X�y�N�g������߂�
  *aspect = static_cast<GLfloat>(layerData.EyeFov.Viewport[ovrEye_Left].Size.w)
    / static_cast<GLfloat>(layerData.EyeFov.Viewport[ovrEye_Left].Size.h);

  // �~���[�\���p�� FBO �̃J���[�o�b�t�@�Ɏg���e�N�X�`�����쐬����
  if (OVR_SUCCESS(ovr_CreateMirrorTextureGL(session, GL_SRGB8_ALPHA8,
    window.width, window.height, reinterpret_cast<ovrTexture **>(&oculus.mirrorTexture))))
  {
    // �~���[�\���p�� FBO ���쐬����
    glGenFramebuffers(1, &oculus.mirrorFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, oculus.mirrorFbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, oculus.mirrorTexture->OGL.TexId, 0);
    glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  }

  // TimeWarp �Ɏg���ϊ��s��̐��������o��
  auto &posTimewarpProjectionDesc(layerData.EyeFovDepth.ProjectionDesc);
  posTimewarpProjectionDesc.Projection22 = (mp[ovrEye_Left].get()[4 * 2 + 2] + mp[ovrEye_Left].get()[4 * 3 + 2]) * 0.5f;
  posTimewarpProjectionDesc.Projection23 = mp[ovrEye_Left].get()[4 * 2 + 3] * 0.5f;
  posTimewarpProjectionDesc.Projection32 = mp[ovrEye_Left].get()[4 * 3 + 2];
#endif

  // �~���[�\���̓t�����g�o�b�t�@�ɕ`��
  glDrawBuffer(GL_FRONT);

  // Oculus Rift �ւ̕\���ł̓X���b�v�Ԋu��҂��Ȃ�
  glfwSwapInterval(0);

  // Oculus Rift �̏������ɐ�������
  return &oculus;
}

//
// Oculus Rift ���~����.
//
void Oculus::terminate()
{
  // Oculus Rift �̃Z�b�V�������쐬����Ă����
  if (session != nullptr)
  {
    // �~���[�\���p�� FBO ���폜����
    if (mirrorFbo)
    {
      glDeleteFramebuffers(1, &mirrorFbo);
      mirrorFbo = 0;
    }

    // �~���[�\���Ɏg�����e�N�X�`�����J������
    if (mirrorTexture)
    {
#if OVR_PRODUCT_VERSION >= 1
      ovr_DestroyMirrorTexture(session, mirrorTexture);
#else
      glDeleteTextures(1, &mirrorTexture->OGL.TexId);
      ovr_DestroyMirrorTexture(session, reinterpret_cast<ovrTexture *>(mirrorTexture));
#endif
      mirrorTexture = nullptr;
    }

    // Oculus Rift �\���p�� FBO ���폜����
    for (int eye = 0; eye < ovrEye_Count; ++eye)
    {
      // Oculus Rift �̃����_�����O�p�� FBO ���폜����
      if (oculusFbo[eye] != 0)
      {
        glDeleteFramebuffers(eye, oculusFbo + eye);
        oculusFbo[eye] = 0;
      }

#if OVR_PRODUCT_VERSION >= 1
      // �����_�����O�^�[�Q�b�g�Ɏg�����e�N�X�`�����J������
      if (layerData.ColorTexture[eye])
      {
        ovr_DestroyTextureSwapChain(session, layerData.ColorTexture[eye]);
        layerData.ColorTexture[eye] = nullptr;
      }

      // �f�v�X�o�b�t�@�Ƃ��Ďg�����e�N�X�`�����J������
      if (oculusDepth[eye] != 0)
      {
        glDeleteTextures(eye, oculusDepth + eye);
        oculusDepth[eye] = 0;
      }
#else
      // �����_�����O�^�[�Q�b�g�Ɏg�����e�N�X�`�����J������
      auto *const colorTexture(layerData.EyeFov.ColorTexture[eye]);
      for (int i = 0; i < colorTexture->TextureCount; ++i)
      {
        const auto *const ctex(reinterpret_cast<ovrGLTexture *>(&colorTexture->Textures[i]));
        glDeleteTextures(1, &ctex->OGL.TexId);
      }
      ovr_DestroySwapTextureSet(session, colorTexture);

      // �f�v�X�o�b�t�@�Ƃ��Ďg�����e�N�X�`�����J������
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
    session = nullptr;
  }

  // �ʏ�̕\���̓o�b�N�o�b�t�@�ɕ`��
  glDrawBuffer(GL_BACK);

  // �ʏ�̕\���̓X���b�v�Ԋu��҂�
  glfwSwapInterval(1);
}

//
// Oculus Rift �̓������e�ϊ��s������߂�.
//
//   zoom �V�[���̃Y�[����.
//   mp �������e�ϊ��s��̔z��.
//
void Oculus::getPerspective(GLfloat zoom, GgMatrix* mp) const
{
  // �O���ʂ� defaults.display_near �Ȃ̂ŃX�N���[��������ɍ��킹��
  const GLfloat zf{ defaults.display_near / zoom };

  for (int eye = 0; eye < ovrEye_Count; ++eye)
  {
    // Oculus Rift �̎�����擾����
    const auto &eyeFov(hmdDesc.DefaultEyeFov[eye]);

    // Oculus Rift �̎��ۂ̎���
#if OVR_PRODUCT_VERSION >= 1
    const auto& fov(eyeFov);
#else
    // Oculus Rift �̃����Y�␳���̐ݒ�l���擾����
    auto& eyeRenderDesc{ oculus.eyeRenderDesc };
    eyeRenderDesc[eye] = ovr_GetRenderDesc(session, ovrEyeType(eye), eyeFov);

    // Oculus Rift �̎��ۂ̎���
    const auto &fov(eyeRenderDesc.Fov);
#endif

    // �����̒����l
    const float p{ static_cast<float>((1 - eye * 2) * attitude.parallax) * parallaxStep };

    // �Жڂ̓������e�ϊ��s������߂�
    mp[eye].loadFrustum(
      (p - fov.LeftTan) * zf, (p + fov.RightTan) * zf,
      -fov.DownTan * zf, fov.UpTan * zf,
      defaults.display_near, defaults.display_far
    );
  }
}

//
// Oculus Rift �ɕ\������}�`�̕`����J�n����.
//
//   mv ���ꂼ��̖ڂ̈ʒu�ɕ��s�ړ����� GgMatrix �^�̕ϊ��s��.
//   �߂�l �`�悪�\�Ȃ� VISIBLE, �s�\�Ȃ� INVISIBLE, �I���v��������� WANTQUIT.
//
enum Oculus::OculusStatus Oculus::start(GgMatrix *mv)
{
  // ���� Oculus Rift �̃Z�b�V�������쐬����Ă��Ȃ��Ƃ�������
  assert(session != nullptr);

#if OVR_PRODUCT_VERSION >= 1
  // �Z�b�V�����̏�Ԃ��擾����
  ovrSessionStatus sessionStatus;
  ovr_GetSessionStatus(session, &sessionStatus);

  // �A�v���P�[�V�������I����v�����Ă���Ƃ��̓E�B���h�E�̃N���[�Y�t���O�𗧂Ă�
  if (sessionStatus.ShouldQuit) return WANTQUIT;

  // ���݂̏�Ԃ��g���b�L���O�̌��_�ɂ���
  if (sessionStatus.ShouldRecenter) ovr_RecenterTrackingOrigin(session);

  // Oculus Rift �ɕ\������Ă��Ȃ��Ƃ��͖߂�
  if (!sessionStatus.IsVisible) return INVISIBLE;

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
  mv[0].loadTranslate(-hmdToEyePose[0].Position.x, -hmdToEyePose[0].Position.y, -hmdToEyePose[0].Position.z);
  mv[1].loadTranslate(-hmdToEyePose[1].Position.x, -hmdToEyePose[1].Position.y, -hmdToEyePose[1].Position.z);

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

  return VISIBLE;
}

//
// �}�`��`�悷�� Oculus Rift �̖ڂ�I������.
//
//   eye �}�`��`�悷�� Oculus Rift �̖�.
//   po GgVecotr �^�̕ϐ��� eye �Ɏw�肵���ڂ̈ʒu�̓������W���i�[�����.
//   qo GgQuaternion �^�̕ϐ��� eye �Ɏw�肵���ڂ̕������l�����Ŋi�[�����.
//
void Oculus::select(int eye, GgVector &po, GgQuaternion &qo)
{
  // ���� Oculus Rift �̃Z�b�V�������쐬����Ă��Ȃ��Ƃ�������
  assert(session != nullptr);

#if OVR_PRODUCT_VERSION >= 1
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
  po[0] = p.x;
  po[1] = p.y;
  po[2] = p.z;
  po[3] = 1.0f;

  // Oculus Rift �̕Жڂ̉�]��ۑ�����
  qo = GgQuaternion(o.x, o.y, o.z, -o.w);

  // �f�v�X�o�b�t�@����������
  glClear(GL_DEPTH_BUFFER_BIT);
}

//
// Oculus Rift �ɕ\������}�`�̕`�����������.
//
//   eye �}�`�̕`����������� Oculus Rift �̖�.
//
void Oculus::commit(int eye)
{
  // ���� Oculus Rift �̃Z�b�V�������쐬����Ă��Ȃ��Ƃ�������
  assert(session != nullptr);

#if OVR_PRODUCT_VERSION >= 1
  // GL_COLOR_ATTACHMENT0 �Ɋ��蓖�Ă�ꂽ�e�N�X�`���� wglDXUnlockObjectsNV() �ɂ����
  // �A�����b�N����邽�߂Ɏ��̃t���[���̏����ɂ����Ė����� GL_COLOR_ATTACHMENT0 ��
  // FBO �Ɍ��������̂������
  glBindFramebuffer(GL_FRAMEBUFFER, oculusFbo[eye]);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

  // �ۗ����̕ύX�� layerData.ColorTexture[eye] �ɔ��f���C���f�b�N�X���X�V����
  ovr_CommitTextureSwapChain(session, layerData.ColorTexture[eye]);
#endif
}

//
// �`�悵���t���[���� Oculus Rift �ɓ]������.
//
//   �߂�l Oculus Rift �ւ̓]�������������� true.
//
bool Oculus::submit()
{
  // ���� Oculus Rift �̃Z�b�V�������쐬����Ă��Ȃ��Ƃ�������
  assert(session != nullptr);

#if OVR_PRODUCT_VERSION >= 1
  // �`��f�[�^�� Oculus Rift �ɓ]������
  const auto *const layers(&layerData.Header);
  if (OVR_FAILURE(ovr_SubmitFrame(session, frameIndex++, nullptr, &layers, 1))) return false;
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
  if (OVR_FAILURE(ovr_SubmitFrame(session, 0, &viewScaleDesc, &layers, 1))) return false;
#endif

  // �ʏ�̃t���[���o�b�t�@�ւ̕`��ɖ߂�
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

  // Oculus Rift �ւ̓]���ɐ�������
  return true;
}

//
// Oculus Rift �ɕ`�悵���t���[�����~���[�\������.
//
//   width �~���[�\�����s���t���[���o�b�t�@��̗̈�̕�.
//   height �~���[�\�����s���t���[���o�b�t�@��̗̈�̍���.
//
void Oculus::submitMirror(GLsizei width, GLsizei height)
{
  // �]������ FBO �̗̈�
#  if OVR_PRODUCT_VERSION >= 1
  const auto& sx1{ mirrorWidth };
  const auto& sy1{ mirrorHeight };
#  else
  const auto& sx1{ mirrorTexture->OGL.Header.TextureSize.w };
  const auto& sy1{ mirrorTexture->OGL.Header.TextureSize.h };
#  endif

  // �~���[�\�����s���t���[���o�b�t�@��̗̈�
  GLint dx0{ 0 }, dx1{ width }, dy0{ 0 }, dy1{ height };

  // �~���[�\�����E�B���h�E����͂ݏo�Ȃ��悤�ɂ���
  if ((width *= sy1) < (height *= sx1))
  {
    const GLint ty1{ width / sx1 };
    dy0 = (dy1 - ty1) / 2;
    dy1 = dy0 + ty1;
  }
  else
  {
    const GLint tx1{ height / sy1 };
    dx0 = (dx1 - tx1) / 2;
    dx1 = dx0 + tx1;
  }

  // �����_�����O���ʂ��~���[�\���p�̃t���[���o�b�t�@�ɂ��]������
  glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFbo);
  glBlitFramebuffer(0, sy1, sx1, 0, dx0, dy0, dx1, dy1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

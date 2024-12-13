#pragma once

//
// �E�B���h�E�֘A�̏���
//

// �e��ݒ�
#include "Config.h"

// �J�����֘A�̏���
#include "Camera.h"

// Windows API �֘A�̐ݒ�
#if defined(_WIN32)
#  define NOMINMAX
#  define GLFW_EXPOSE_NATIVE_WIN32
#  define GLFW_EXPOSE_NATIVE_WGL
#  include <GLFW/glfw3native.h>
#  define OVR_OS_WIN32
#  define NOTIFY(msg) MessageBox(NULL, TEXT(msg), TEXT("TED"), MB_ICONERROR | MB_OK)
#else
#  define NOTIFY(msg) std::cerr << msg << '\n'
#endif

// Oculus Rift SDK ���C�u���� (LibOVR) �̑g�ݍ���
#include <OVR_CAPI_GL.h>
#include <Extras/OVR_Math.h>

// Dear ImGui ���g��
#include "imgui.h"

//
// �E�B���h�E�֘A�̏�����S������N���X
//
class Window
{
  // �ݒ�
  const Config& defaults;

  // �ݒ�̃R�s�[
  Config config;

  // �E�B���h�E�̎��ʎq
  GLFWwindow* const window;

  // ���̃E�B���h�E�Ő��䂷��J����
  Camera* camera;

  // �r���[�|�[�g�̕��ƍ���
  int width, height;

  // �r���[�|�[�g�̃A�X�y�N�g��
  GLfloat aspect;

  // ���b�V���̏c���̊i�q�_��
  std::array<GLsizei, 2> samples;

  // ���b�V���̏c���̊i�q�Ԋu
  std::array<GLfloat, 2> gap;

  // �Ō�Ƀ^�C�v�����L�[
  int key;

  // �W���C�X�e�B�b�N�̔ԍ�
  int joy;

  // �X�e�B�b�N�̒����ʒu
  std::array<float, 4> origin;

  // �h���b�O�J�n�ʒu
  double cx, cy;

  //
  // ���f���ϊ�
  //

  // �g���b�N�{�[������
  GgTrackball trackball;

  // ���f���ϊ��s��
  GgMatrix mm;

  // �w�b�h�g���b�L���O�̕ϊ��s��
  std::array<GgMatrix, camCount> mo;

  //
  // �r���[�ϊ�
  //

  // �J���������̕␳�X�e�b�v
  static std::array<GgQuaternion, camCount> qrStep;

  // �r���[�ϊ�
  std::array<GgMatrix, camCount> mv;

  //
  // ���e�ϊ�
  //

  // �Y�[�����̕ω���
  int zoomChange;

  // ���e�ϊ�
  std::array<GgMatrix, camCount> mp;

  //
  // �w�i�摜
  //

  // �X�N���[���̕��ƍ���
  std::array<GgVector, camCount> screen;

  // �œ_�����̕ω���
  int focalChange;

  // �w�i�e�N�X�`���̔��a�ƒ��S�̕ω���
  std::array<int, 4> circleChange;

  //
  // Oculus Rift
  //

  // Oculus Rift �̃Z�b�V����
  const ovrSession session;

  // Oculus Rift �̏��
  ovrHmdDesc hmdDesc;

  // Oculus Rift �\���p�� FBO
  GLuint oculusFbo[ovrEye_Count];

#if OVR_PRODUCT_VERSION > 0
  // Oculus Rift �Ƀ����_�����O����t���[���̔ԍ�
  long long frameIndex;

  // Oculus Rift �ւ̕`����
  ovrLayerEyeFov layerData;

  // Oculus Rift �\���p�� FBO �̃f�v�X�e�N�X�`��
  std::array<GLuint, ovrEye_Count> oculusDepth;

  // �~���[�\���p�� FBO �̃J���[�e�N�X�`��
  ovrMirrorTexture mirrorTexture;
#else
  // Oculus Rift �̃����_�����O���
  std::array<ovrEyeRenderDesc, ovrEye_Count> eyeRenderDesc;

  // Oculus Rift �̎��_���
  std::array<ovrPosef, ovrEye_Count> eyePose;

  // Oculus Rift �ɓ]������`��f�[�^
  ovrLayer_Union layerData;

  // �~���[�\���p�� FBO �̃J���[�e�N�X�`��
  ovrGLTexture* mirrorTexture;
#endif

  // �~���[�\���p�� FBO
  GLuint mirrorFbo;

  //
  // �������e�ϊ��s������߂�
  //
  //   �E�E�B���h�E�̃T�C�Y�ύX����J�����p�����[�^�̕ύX���ɌĂяo��
  //
  void updateProjectionMatrix();

public:

  //
  // �R���X�g���N�^
  //
  Window(Config& config, GLFWwindow* share = nullptr);

  //
  // �R�s�[�R���X�g���N�^�𕕂���
  //
  Window(const Window& w) = delete;

  //
  // ����𕕂���
  //
  Window& operator=(const Window& w) = delete;

  //
  // �f�X�g���N�^
  //
  virtual ~Window();

  //
  // �E�B���h�E�̎��ʎq�̎擾
  //
  GLFWwindow* get() const
  {
    return window;
  }

  //
  // �E�B���h�E�����悤�w������
  //
  void setClose(int close = GLFW_TRUE) const
  {
    glfwSetWindowShouldClose(window, close);
  }

  //
  // �E�B���h�E�����ׂ����𔻒肷��
  //
  int shouldClose() const
  {
    // �E�B���h�E�����ׂ��Ȃ�^��Ԃ�
    return glfwWindowShouldClose(window);
  }

  //
  // �C�x���g���擾���ă��[�v���p������Ȃ�^��Ԃ�
  //
  operator bool();

  //
  // �`��J�n
  //
  //   �E�}�`�̕`��J�n�O�ɌĂяo��
  //
  bool start();

  //
  // �J���[�o�b�t�@�����ւ��ăC�x���g�����o��
  //
  //   �E�}�`�̕`��I����ɌĂяo��
  //   �E�_�u���o�b�t�@�����O�̃o�b�t�@�̓���ւ����s��
  //   �E�L�[�{�[�h���쓙�̃C�x���g�����o��
  //
  void swapBuffers();

  //
  // �E�B���h�E�̃T�C�Y�ύX���̏���
  //
  //   �E�E�B���h�E�̃T�C�Y�ύX���ɃR�[���o�b�N�֐��Ƃ��ČĂяo�����
  //   �E�E�B���h�E�̍쐬���ɂ͖����I�ɌĂяo��
  //
  static void resize(GLFWwindow* window, int width, int height);

  //
  // �}�E�X�{�^���𑀍삵���Ƃ��̏���
  //
  //   �E�}�E�X�{�^�����������Ƃ��ɃR�[���o�b�N�֐��Ƃ��ČĂяo�����
  //
  static void mouse(GLFWwindow* window, int button, int action, int mods);

  //
  // �}�E�X�z�C�[�����쎞�̏���
  //
  //   �E�}�E�X�z�C�[���𑀍삵�����ɃR�[���o�b�N�֐��Ƃ��ČĂяo�����
  //
  static void wheel(GLFWwindow* window, double x, double y);

  //
  // �L�[�{�[�h���^�C�v�������̏���
  //
  //   �D�L�[�{�[�h���^�C�v�������ɃR�[���o�b�N�֐��Ƃ��ČĂяo�����
  //
  static void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);

  //
  // �ݒ�l�̕ۑ�
  // 
  bool save(const std::string& file)
  {
    config.orientation = trackball.getQuaternion();
    return config.save(file);
  }

  //
  // �ݒ�l�̏�����
  //
  void reset();

  //
  // ���̃E�B���h�E�Ő��䂷��J������ݒ肷��
  //
  void setControlCamera(Camera* cam)
  {
    camera = cam;
  }

  //
  // �}�`��������ڂ�I������
  //
  void select(int eye);

  //
  // �}�`�̕`�����������
  //
  void commit(int eye);

  //
  // ���f���ϊ��s��𓾂�
  //
  const GgMatrix& getMm() const
  {
    return mm;
  }

  //
  // �r���[�ϊ��s��𓾂�
  //
  const GgMatrix& getMv(int eye) const
  {
    return mv[eye];
  }

  //
  // Oculus Rift �̃w�b�h���b�L���O�ɂ���]�̕␳�l�̎l�����𓾂�
  //
  const GgQuaternion& getQa(int eye) const
  {
    return config.parallax_offset[eye];
  }

  //
  // Oculus Rift �̃w�b�h���b�L���O�ɂ���]�̕ϊ��s��𓾂�
  //
  const GgMatrix& getMo(int eye) const
  {
    return mo[eye];
  }

  //
  // �v���W�F�N�V�����ϊ��s��𓾂�
  //
  const GgMatrix& getMp(int eye) const
  {
    return mp[eye];
  }

  //
  // ���b�V���̏c���̊i�q�_�������o��
  //
  const GLsizei* getSamples() const
  {
    return samples.data();
  }

  //
  // ���b�V���̏c���̊i�q�Ԋu�����o��
  //
  const GLfloat* getGap() const
  {
    return gap.data();
  }

  //
  // �X�N���[���̕��ƍ��������o��
  //
  const GgVector& getScreen(int eye) const
  {
    return screen[eye];
  }

  //
  // �œ_���������o��
  //
  GLfloat getFocal() const
  {
    return config.display_focal;
  }

  //
  // �X�N���[���̊Ԋu�����o��
  //
  GLfloat getOffset(int eye = 0) const
  {
    return static_cast<GLfloat>(1 - (eye & 1) * 2) * config.display_offset;
  }

  //
  // �w�i�e�N�X�`���̔��a�ƒ��S�����o��
  //
  const GgVector& getCircle() const
  {
    return config.circle;
  }

  //
  // �E�B���h�E�̃A�X�y�N�g������o��
  //
  GLfloat getAspect() const
  {
    return aspect;
  }

  // �V�[���\��
  bool showScene;

  // �~���[�\��
  bool showMirror;

  // ���j���[�\��
  bool showMenu;
};

#pragma once

//
// �E�B���h�E�֘A�̏���
//

// �e��ݒ�
#include "config.h"

// �J�����֘A�̏���
#include "Camera.h"

// Windows API �֘A�̐ݒ�
#if defined(_WIN32)
#  define NOMINMAX
#  define GLFW_EXPOSE_NATIVE_WIN32
#  define GLFW_EXPOSE_NATIVE_WGL
#  include <GLFW/glfw3native.h>
#  undef APIENTRY
#  define OVR_OS_WIN32
#  define NOTIFY(msg) MessageBox(NULL, TEXT(msg), TEXT("TED"), MB_ICONERROR | MB_OK)
#else
#  define NOTIFY(msg) std::cerr << msg << '\n'
#endif

// Oculus Rift SDK ���C�u���� (LibOVR) �̑g�ݍ���
#include <OVR_CAPI_GL.h>
#include <Extras/OVR_Math.h>

//
// �E�B���h�E�֘A�̏�����S������N���X
//
class Window
{
  // �E�B���h�E�̎��ʎq
  GLFWwindow *const window;

  // ���̃E�B���h�E�Ő��䂷��J����
  Camera *camera;

  // �r���[�|�[�g�̕��ƍ���
  int width, height;

  // �r���[�|�[�g�̃A�X�y�N�g��
  GLfloat aspect;

  // ���b�V���̏c���̊i�q�_��
  GLsizei samples[2];

  // ���b�V���̏c���̊i�q�Ԋu
  GLfloat gap[2];

  // �Ō�Ƀ^�C�v�����L�[
  int key;

  // �W���C�X�e�B�b�N�̔ԍ�
  int joy;

  // �X�e�B�b�N�̒����ʒu
  float origin[4];

  // �h���b�O�J�n�ʒu
  double cx, cy;

  //
  // ���f���ϊ�
  //

  // ���̂̈ʒu
  GLfloat ox, oy, oz;

  // ���̂̏����ʒu
  static GLfloat startPosition[3];

  // �g���b�N�{�[������
  GgTrackball trackball;

  // ���̂̏����p��
  static GLfloat startOrientation[4];

  // ���f���ϊ��s��
  GgMatrix mm;

  //
  // �r���[�ϊ�
  //

  // �w�b�h�g���b�L���O�ɂ���]
  GgQuaternion qo[camCount];

  // �w�b�h�g���b�L���O�ɂ��ʒu
  GLfloat po[camCount][4];

  // �r���[�ϊ�
  GgMatrix mv[camCount];

  //
  // ���e�ϊ�
  //

  // ���̂ɑ΂���Y�[����
  GLfloat zoom;

  // �Y�[�����̕ω���
  int zoomChange;

  // ����
  GLfloat parallax;

  // �����̏����l
  static GLfloat initialParallax;

  // ���e�ϊ�
  GgMatrix mp[camCount];

  //
  // �w�i�摜
  //

  // �X�N���[���̕��ƍ���
  GLfloat screen[camCount][4];

  // �œ_����
  GLfloat focal;

  // �œ_�����̕ω���
  int focalChange;

  // �w�i�e�N�X�`���̔��a�ƒ��S
  GLfloat circle[4];

  // �w�i�e�N�X�`���̔��a�ƒ��S�̕ω���
  int circleChange[4];

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
  GLuint oculusDepth[ovrEye_Count];

  // �~���[�\���p�� FBO �̃J���[�e�N�X�`��
  ovrMirrorTexture mirrorTexture;
#else
  // Oculus Rift �̃����_�����O���
  ovrEyeRenderDesc eyeRenderDesc[ovrEye_Count];

  // Oculus Rift �̎��_���
  ovrPosef eyePose[ovrEye_Count];

  // Oculus Rift �ɓ]������`��f�[�^
  ovrLayer_Union layerData;

  // �~���[�\���p�� FBO �̃J���[�e�N�X�`��
  ovrGLTexture *mirrorTexture;
#endif

  // �~���[�\���p�� FBO
  GLuint mirrorFbo;

  //
  // �������e�ϊ��s������߂�
  //
  //   �E�E�B���h�E�̃T�C�Y�ύX����J�����p�����[�^�̕ύX���ɌĂяo��
  //
  void updateProjectionMatrix();

  //
  // �R�s�[�R���X�g���N�^ (�R�s�[�֎~)
  //
  Window(const Window &w);

  //
  // ��� (����֎~)
  //
  Window &operator=(const Window &w);

public:

  //
  // �R���X�g���N�^
  //
  Window(int width = 640, int height = 480, const char *title = "GLFW Window"
    , GLFWmonitor *monitor = nullptr, GLFWwindow *share = nullptr
  );

  //
  // �f�X�g���N�^
  //
  virtual ~Window();

  //
  // �E�B���h�E�̎��ʎq�̎擾
  //
  const GLFWwindow *get() const
  {
    return window;
  }

  //
  // �E�B���h�E�����ׂ����𔻒肷��
  //
  //   �E�`�惋�[�v�̌p�������Ƃ��Ďg��
  //
  bool shouldClose() const
  {
    // �E�B���h�E����邩 ESC �L�[���^�C�v����Ă���ΐ^
    return glfwWindowShouldClose(window) || glfwGetKey(window, GLFW_KEY_ESCAPE);
  }

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
  static void resize(GLFWwindow *window, int width, int height);

  //
  // �}�E�X�{�^���𑀍삵���Ƃ��̏���
  //
  //   �E�}�E�X�{�^�����������Ƃ��ɃR�[���o�b�N�֐��Ƃ��ČĂяo�����
  //
  static void mouse(GLFWwindow *window, int button, int action, int mods);

  //
  // �}�E�X�z�C�[�����쎞�̏���
  //
  //   �E�}�E�X�z�C�[���𑀍삵�����ɃR�[���o�b�N�֐��Ƃ��ČĂяo�����
  //
  static void wheel(GLFWwindow *window, double x, double y);

  //
  // �L�[�{�[�h���^�C�v�������̏���
  //
  //   �D�L�[�{�[�h���^�C�v�������ɃR�[���o�b�N�֐��Ƃ��ČĂяo�����
  //
  static void keyboard(GLFWwindow *window, int key, int scancode, int action, int mods);

  //
  // �ݒ�l�̏�����
  //
  void reset();

  //
  // ���̃E�B���h�E�Ő��䂷��J������ݒ肷��
  //
  void setControlCamera(Camera *cam)
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
  const GgMatrix &getMm() const
  {
    return mm;
  }

  //
  // �r���[�ϊ��s��𓾂�
  //
  const GgMatrix &getMv(int eye) const
  {
    return mv[eye];
  }

  //
  // Oculus Rift �̃w�b�h���b�L���O�ɂ��ړ��𓾂�
  //
  const GLfloat *getPo(int eye) const
  {
    return po[eye];
  }

  //
  // Oculus Rift �̃w�b�h���b�L���O�ɂ���]�̎l�����𓾂�
  //
  const GgQuaternion &getQo(int eye) const
  {
    return qo[eye];
  }

  //
  // Oculus Rift �̃w�b�h���b�L���O�ɂ���]�̕ϊ��s��𓾂�
  //
  GgMatrix getMo(int eye) const;

  //
  // �v���W�F�N�V�����ϊ��s��𓾂�
  //
  const GgMatrix &getMp(int eye) const
  {
    return mp[eye];
  }

  //
  // ���b�V���̏c���̊i�q�_�������o��
  //
  const GLsizei *getSamples() const
  {
    return samples;
  }

  //
  // ���b�V���̏c���̊i�q�Ԋu�����o��
  //
  const GLfloat *getGap() const
  {
    return gap;
  }

  //
  // �X�N���[���̕��ƍ��������o��
  //
  const GLfloat *getScreen(int eye) const
  {
    return screen[eye];
  }

  //
  // �œ_���������o��
  //
  GLfloat getFocal() const
  {
    return focal;
  }

  //
  // �w�i�e�N�X�`���̔��a�ƒ��S�����o��
  //
  const GLfloat *getCircle() const
  {
    return circle;
  }

  //
  // �E�B���h�E�̃A�X�y�N�g������o��
  //
  GLfloat getAspect() const
  {
    return aspect;
  }

  // ���̂̕\��
  bool showScene;

  // �~���[�\��
  bool showMirror;
};

#pragma once

//
// �E�B���h�E�֘A�̏���
//

// �J�����֘A�̏���
#include "Camera.h"

// ���b�Z�[�W�{�b�N�X
#if defined(_WIN32)
#  define NOTIFY(msg) MessageBox(NULL, TEXT(msg), TEXT("TED"), MB_ICONERROR | MB_OK)
#else
#  define NOTIFY(msg) std::cerr << msg << '\n'
#endif

// Oculus Rift �֘A�̏���
class Oculus;

//
// �E�B���h�E�֘A�̏�����S������N���X
//
class Window
{
  // �E�B���h�E�̎��ʎq
  GLFWwindow* const window;

  // �E�B���h�E�̃T�C�Y
  std::array<GLsizei, 2> size;

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

  // ���̃E�B���h�E�Ő��䂷��J����
  Camera* camera;

  //
  // �w�b�h�g���b�L���O
  //

  // �w�b�h�g���b�L���O�ɂ���]
  std::array<GgQuaternion, camCount> qo;

  // �w�b�h�g���b�L���O�ɂ��ʒu
  std::array<GgVector, camCount> po;

  // �w�b�h�g���b�L���O�̕ϊ��s��
  std::array<GgMatrix, camCount> mo;

  //
  // ���W�ϊ�
  //

  // ���f���ϊ��s��
  GgMatrix mm;

  // �r���[�ϊ��s��
  std::array<GgMatrix, camCount> mv;

  // ���e�ϊ��s��
  std::array<GgMatrix, camCount> mp;

  // �Y�[����
  GLfloat zoom;

  //
  // �w�i�摜
  //

  // �w�i�e�N�X�`���̔��a�ƒ��S
  GgVector circle;

  // �X�N���[���̕��ƍ���
  std::array<GgVector, camCount> screen;

  // �œ_����
  GLfloat focal;

  // ����
  GLfloat parallax;

  // �X�N���[���̊Ԋu
  GLfloat offset;

  // �w�i�̕`��Ɏg����`����Q�Ƃ���
  friend class Rect;

public:

  //
  // �R���X�g���N�^
  //
  Window(int width = 640, int height = 480,
    const char* title = "GLFW Window",
    GLFWmonitor* monitor = nullptr,
    GLFWwindow* share = nullptr
  );

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
  // �ݒ�l�̏�����
  //
  void reset();

  //
  // �r���[�|�[�g�̏�����
  // 
  void resetViewport()
  {
    resize(window, size[0], size[1]);
  }

  //
  // �������e�ϊ��s����X�V����
  //
  void update();

  //
  // �J�����̉�p�ƒ��S�ʒu���X�V����
  //
  void updateCircle();

  //
  // ���̃E�B���h�E�Ő��䂷��J������ݒ肷��
  //
  void setControlCamera(Camera* cam)
  {
    camera = cam;
  }

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
  // Oculus Rift �̃w�b�h���b�L���O�ɂ��ړ��𓾂�
  //
  const GgVector& getPo(int eye) const
  {
    return po[eye];
  }

  //
  // Oculus Rift �̃w�b�h���b�L���O�ɂ���]�̎l�����𓾂�
  //
  const GgQuaternion& getQo(int eye) const
  {
    return qo[eye];
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
  const auto& getSamples() const
  {
    return samples;
  }

  //
  // ���b�V���̏c���̊i�q�Ԋu�����o��
  //
  const auto& getGap() const
  {
    return gap;
  }

  //
  // �X�N���[���̕��ƍ��������o��
  //
  const auto& getScreen(int eye) const
  {
    return screen[eye];
  }

  //
  // �œ_���������o��
  //
  auto getFocal() const
  {
    return focal;
  }

  //
  // �X�N���[���̊Ԋu�����o��
  //
  auto getOffset(int eye = 0) const
  {
    return static_cast<GLfloat>(1 - (eye & 1) * 2) * offset;
  }

  //
  // �w�i�e�N�X�`���̔��a�ƒ��S�����o��
  //
  const auto& getCircle() const
  {
    return circle;
  }

  //
  // �E�B���h�E�̃T�C�Y�����o��
  //
  const auto& getSize() const
  {
    return size;
  }

  //
  // �E�B���h�E�̃A�X�y�N�g������o��
  //
  auto getAspect() const
  {
    return aspect;
  }

  // �V�[���\��
  bool showScene;

  // �~���[�\��
  bool showMirror;

  // ���j���[�\��
  bool showMenu;

  // Oculus Rift �̃R���e�L�X�g
  friend class Oculus;
  static Oculus* oculus;

  // Oculus Rift ���N������
  bool startOculus();

  // Oculus Rift ���~����
  void stopOculus();

  // �`�悷��ڂ�I������
  void select(int eye);

  // �`����J�n����
  bool start();

  // �`�����������
  static void commit(int eye);
};

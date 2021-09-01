#pragma once

// Windows API �֘A�̐ݒ�
#if defined(_WIN32)
#  define NOMINMAX
#  define GLFW_EXPOSE_NATIVE_WIN32
#  define GLFW_EXPOSE_NATIVE_WGL
#  include <GLFW/glfw3native.h>
#  define OVR_OS_WIN32
#endif

// Oculus Rift SDK ���C�u���� (LibOVR) �̑g�ݍ���
#include <OVR_CAPI_GL.h>
#include <Extras/OVR_Math.h>

// �E�B���h�E�֘A�̏���
class Window;

//
// Oculus Rift
//
class Oculus
{
  // Oculus Rift �Ɋ֘A�t����ꂽ�E�B���h�E
  Window* window;

  // Oculus Rift �̃Z�b�V����
  ovrSession session;

  // Oculus Rift �̏��
  ovrHmdDesc hmdDesc;

  // Oculus Rift �\���p�� FBO
  GLuint oculusFbo[ovrEye_Count];

#if OVR_PRODUCT_VERSION >= 1
  // Oculus Rift �Ƀ����_�����O����t���[���̔ԍ�
  long long frameIndex;

  // Oculus Rift �ւ̕`����
  ovrLayerEyeFov layerData;

  // Oculus Rift �\���p�� FBO �̃f�v�X�e�N�X�`��
  GLuint oculusDepth[ovrEye_Count];

  // �~���[�\���p�� FBO �̃J���[�e�N�X�`��
  ovrMirrorTexture mirrorTexture;

  // �~���[�\���p�� FBO �̃J���[�e�N�X�`���̃T�C�Y
  GLsizei mirrorWidth, mirrorHeight;

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

  // �R���X�g���N�^
  Oculus();

  // �f�X�g���N�^
  ~Oculus() = default;

public:

  // �R�s�[�E���[�u�֎~
  Oculus(const Oculus&) = delete;
  Oculus& operator=(const Oculus&) = delete;
  Oculus(Oculus&&) = delete;
  Oculus& operator=(Oculus&&) = delete;

  ///
  /// @brief Oculus Rift ������������.
  ///
  ///   @param window Oculus Rift �Ɋ֘A�t����E�B���h�E.
  ///   @return Oculus Rift �̏������ɐ��������� true.
  ///
  static bool initialize(Window& window);

  ///
  /// @brief Oculus Rift ���~����.
  ///
  void terminate();

  ///
  /// @brief Oculus Rift �̃Z�b�V�������L�����ǂ������肷��.
  ///
  ///   @return Oculus Rift �̃Z�b�V�������L���Ȃ� true.
  ///
  operator bool()
  {
    return this != nullptr && session != nullptr;
  }

  ///
  /// @brief Oculus Rift �ɕ\������}�`�̕`����J�n����.
  ///
  ///   @param mv ���ꂼ��̖ڂ̈ʒu�ɕ��s�ړ����� GgMatrix �^�̕ϊ��s��.
  ///   @return Oculus Rift �ւ̕`�悪�\�Ȃ� true.
  ///
  bool start(GgMatrix* mv);

  ///
  /// @brief �}�`��`�悷�� Oculus Rift �̖ڂ�I������.
  ///
  ///   @param eye �}�`��`�悷�� Oculus Rift �̖�.
  ///   @param po GgVecotr �^�̕ϐ��� eye �Ɏw�肵���ڂ̈ʒu�̓������W���i�[�����.
  ///   @param qo GgQuaternion �^�̕ϐ��� eye �Ɏw�肵���ڂ̕������l�����Ŋi�[�����.
  ///
  void select(int eye, GgVector& mo, GgQuaternion& qo);

  ///
  /// @brief Oculus Rift �ɕ\������}�`�̕`�����������.
  ///
  ///   @param eye �}�`�̕`����������� Oculus Rift �̖�.
  ///
  void commit(int eye);

  ///
  /// @brief �`�悵���t���[���� Oculus Rift �ɓ]������.
  ///
  ///   @param mirror �~���[�\�����s���Ȃ� true.
  ///   @param width �~���[�\�����s���t���[���o�b�t�@��̗̈�̕�.
  ///   @param height �~���[�\�����s���t���[���o�b�t�@��̗̈�̍���.
  ///   @return Oculus Rift �ւ̓]�������������� true.
  ///
  bool submit(bool mirror, GLsizei width, GLsizei height);
};

#pragma once

//
// ���j���[
//

// �E�B���h�E�֘A�̏���
#include "Window.h"

// �V�[���O���t
#include "Scene.h"

// �p��
#include "Attitude.h"

class Menu
{

  // ���j���[��\������E�B���h�E
  Window& window;

  // ���j���[�ő��삷��V�[��
  Scene& scene;

  // ���j���[�ő��삷��p��
  Attitude& attitude;

  // �T�u�E�B���h�E�̃I���E�I�t
  bool showNodataWindow;
  bool showDisplayWindow;
  bool showAttitudeWindow;
  bool showStartupWindow;

  // �N�����ݒ�̃R�s�[
  int secondary;
  bool fullscreen;
  bool quadbuffer;
  int memorysize[2];

  // ���j���[�o�[
  void menuBar();

  // �f�[�^�ǂݍ��݃G���[�\���E�B���h�E
  void nodataWindow();

  // �\���ݒ�E�B���h�E
  void displayWindow();

  // �p���ݒ�E�B���h�E
  void attitudeWindow();

  // �N�����ݒ�E�B���h�E
  void startupWindow();

public:

  // �R���X�g���N�^
  Menu(Window& window, Scene& scene, Attitude& attitude);

  // �f�X�g���N�^
  virtual ~Menu();

  // ���j���[�̕\��
  void show();
};

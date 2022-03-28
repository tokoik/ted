#pragma once

//
// ���j���[
//

// �E�B���h�E�֘A�̏���
#include "Window.h"

class Menu
{

  // ���j���[��\������E�B���h�E
  Window& window;

  // �T�u�E�B���h�E�̃I���E�I�t
  bool showNodataWindow;
  bool showDisplayWindow;
  bool showCameraWindow;
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

  // �J�����ݒ�E�B���h�E
  void cameraWindow();

  // �N�����ݒ�E�B���h�E
  void startupWindow();

public:

  // �R���X�g���N�^
  Menu(Window& window);

  // �f�X�g���N�^
  virtual ~Menu();

  // ���j���[�̕\��
  void show();
};

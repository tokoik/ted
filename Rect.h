#pragma once

//
// ��`
//

// �E�B���h�E�֘A�̏���
#include "Window.h"

// �W�����C�u����
#include <string>

class Rect
{
  // �`�悷��E�B���h�E
  const Window& window;

  // �`��Ɏg���V�F�[�_
  const GLuint shader;

  // �i�q�Ԋu�� uniform �ϐ��̏ꏊ
  const GLint gapLoc;

  // �X�N���[���̃T�C�Y�ƒ��S�ʒu�� uniform �ϐ��̏ꏊ
  const GLint screenLoc;

  // �œ_������ uniform �ϐ��̏ꏊ
  const GLint focalLoc;

  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu�� uniform �ϐ��̏ꏊ
  const GLint circleLoc;

  // �����̉�]�s��� uniform �ϐ��̏ꏊ
  const GLint rotationLoc;

  // �e�N�X�`���̃T���v���� uniform �ϐ��̏ꏊ
  const GLint imageLoc;

  // ���_�z��I�u�W�F�N�g
  const GLuint vao;

  // �\��t����e�N�X�`��
  GLuint texture[camCount];

public:

  // �R���X�g���N�^
  Rect(const Window& window, const std::string& vert, const std::string& frag);

  // �f�X�g���N�^
  ~Rect();

  // �e�N�X�`����ݒ肷��
  void setTexture(int eye, GLuint eyeTexture)
  {
    texture[eye] = eyeTexture;
  }

  // �V�F�[�_�v���O�������𓾂�
  GLuint get() const;

  // �`��
  void draw(int eye, const GgMatrix& rotation, const GLsizei* samples) const;
};

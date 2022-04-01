#pragma once

//
// ��`
//

// �⏕�v���O����
#include "gg.h"
using namespace gg;

// �W�����C�u����
#include <string>

class Rect
{
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

  // �i�q�Ԋu
  const GLfloat* gap;

  // �X�N���[���̃T�C�Y�ƒ��S�ʒu
  const GLfloat* screen;

  // �œ_����
  GLfloat focal;

  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu
  GgVector circle;

public:

  // �R���X�g���N�^
  Rect(const std::string& vert, const std::string& frag);

  // �f�X�g���N�^
  ~Rect();

  // �V�F�[�_�v���O�������𓾂�
  GLuint get() const;

  // �i�q�Ԋu��ݒ肷��
  void setGap(const GLfloat* gap);

  // �X�N���[���̃T�C�Y�ƒ��S�ʒu��ݒ肷��
  void setScreen(const GgVector& screen);

  // �œ_������ݒ肷��
  void setFocal(GLfloat focal);

  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu��ݒ肷��
  void setCircle(const GgVector& circle, GLfloat offset);

  // �`��
  void draw(GLint texture, const GgMatrix& rotation, const GLsizei* samples) const;
};

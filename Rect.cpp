//
// ��`
//
#include "Rect.h"

// �R���X�g���N�^
Rect::Rect(const std::string &vert, const std::string &frag)
  : shader(ggLoadShader(vert.c_str(), frag.c_str()))
  , gapLoc(glGetUniformLocation(shader, "gap"))
  , screenLoc(glGetUniformLocation(shader, "screen"))
  , focalLoc(glGetUniformLocation(shader, "focal"))
  , circleLoc(glGetUniformLocation(shader, "circle"))
  , rotationLoc(glGetUniformLocation(shader, "rotation"))
  , imageLoc(glGetUniformLocation(shader, "image"))
  , vao([]() { GLuint vao; glGenVertexArrays(1, &vao); return vao; } ())
{
}

// �f�X�g���N�^
Rect::~Rect()
{
  // ���_�z��I�u�W�F�N�g���폜����
  glDeleteVertexArrays(1, &vao);
}

// �V�F�[�_�v���O�������𓾂�
GLuint Rect::get() const
{
  return shader;
}

// �i�q�Ԋu��ݒ肷��
void Rect::setGap(const GLfloat *gap)
{
  this->gap = gap;
}

// �X�N���[���̃T�C�Y�ƒ��S�ʒu��ݒ肷��
void Rect::setScreen(const GgVector& screen)
{
  this->screen = screen.data();
}

// �œ_������ݒ肷��
void Rect::setFocal(GLfloat focal)
{
  this->focal = focal;
}

// �w�i�e�N�X�`���̔��a�ƒ��S�ʒu��ݒ肷��
void Rect::setCircle(const GgVector& circle, GLfloat offset)
{
  this->circle[0] = circle[0];
  this->circle[1] = circle[1];
  this->circle[2] = circle[2] + offset;
  this->circle[3] = circle[3];
}

// �`��
void Rect::draw(GLint texture, const GgMatrix &rotation, const GLsizei* samples) const
{
  // �V�F�[�_�v���O������I������
  glUseProgram(shader);

  // �i�q�Ԋu��ݒ肷��
  glUniform2fv(gapLoc, 1, gap);

  // �X�N���[���̃T�C�Y�ƒ��S�ʒu��ݒ肷��
  glUniform4fv(screenLoc, 1, screen);

  // �œ_������ݒ肷��
  glUniform1f(focalLoc, focal);

  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu��ݒ肷��
  glUniform4fv(circleLoc, 1, circle.data());

  // �����̉�]�s���ݒ肷��
  glUniformMatrix4fv(rotationLoc, 1, GL_TRUE, rotation.get());

  // ���e����e�N�X�`�����w�肷��
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glUniform1i(imageLoc, 0);

  // ���_�z��I�u�W�F�N�g���w�肵�ĕ`�悷��
  glBindVertexArray(vao);
  glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, samples[0] * 2, samples[1] - 1);
}

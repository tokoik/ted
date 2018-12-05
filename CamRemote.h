#pragma once

//
// �����[�g�J��������L���v�`��
//

// �J�����֘A�̏���
#include "Camera.h"

// �����[�g�J��������L���v�`������Ƃ��̃_�~�[�J����
class CamRemote
  : public Camera
{
  // �h�[���摜�ɕό`���邩�ۂ�
  const bool reshape;

  // �w�i�摜�̕ό`�Ɏg���t���[���o�b�t�@�I�u�W�F�N�g
  GLuint fb;

  // �w�i�摜�̕ό`�Ɏg�����b�V���̉𑜓x
  GLsizei slices, stacks;;

  // �w�i�摜�̕ό`�Ɏg�����b�V���̊i�q�Ԋu
  GLfloat gap[2];

  // �w�i�摜���擾���郊���[�g�̃J�����̃X�N���[���̑傫��
  GLfloat screen[2];

  // �����[�g����擾�����t���[��
  cv::Mat remote[camCount];

  // �����[�g����擾�����t���[���̃T���v�����O�Ɏg���e�N�X�`��
  GLuint resample[camCount];

  // �w�i�摜�̃^�C�����O�Ɏg���V�F�[�_
  GLuint shader;

  // �w�i�摜�̃^�C�����O�Ɏg���V�F�[�_�� uniform �ϐ��̏ꏊ
  GLint gapLoc, screenLoc, rotationLoc, imageLoc;

  // �����[�g�̃t���[���Ǝp������M����
  void recv();

  // ���[�J���̎p���𑗐M����
  void send();

public:

  // �R���X�g���N�^
  CamRemote(bool reshape);

  // �f�X�g���N�^
  virtual ~CamRemote();

  // �J����������͂���
  int open(unsigned short port, const char *address, const GLuint *image);

  // �J���������b�N���ĉ摜���e�N�X�`���ɓ]������
  virtual bool transmit(int cam, GLuint texture, const GLsizei *size);
};

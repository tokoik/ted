#pragma once

//
// �Î~��
//

// �J�����֘A�̏���
#include "Camera.h"

// �Î~����g���N���X
class CamImage
  : public Camera
{
  // �t�@�C������ǂݍ��񂾉摜
  cv::Mat image[camCount];

public:

  // �R���X�g���N�^
  CamImage();

  // �f�X�g���N�^
  virtual ~CamImage();

  // �t�@�C��������͂���
  bool open(const std::string &file, int cam);

  // �J�������g�p�\�����肷��
  bool opened(int cam);

  // ���̃J�����ł͉摜�̓]�����s��Ȃ�
  bool CamImage::transmit(int cam, GLuint texture, const GLsizei *size);

  // �ǂݍ��񂾉摜�̃f�[�^�𓾂�
  const GLubyte *getImage(int cam);
};

//
// �Î~��
//
#include "CamImage.h"

// �R���X�g���N�^
CamImage::CamImage()
{
}

// �f�X�g���N�^
CamImage::~CamImage()
{
  // ���M�X���b�h���~�߂�
  stop();
}

// �t�@�C��������͂���
bool CamImage::open(const std::string& file, int cam)
{
  // �摜���t�@�C������ǂݍ���
  image[cam] = cv::imread(file);

  // �ǂݍ��݂ɐ��������� true
  return !image[cam].empty();
}

// �J�������g�p�\�����肷��
bool CamImage::opened(int cam)
{
  return !image[cam].empty();
}

// ���̃J�����ł͉摜�̓]�����s��Ȃ�
bool CamImage::transmit(int cam, GLuint texture, const GLsizei* size)
{
  return true;
}

// �ǂݍ��񂾉摜�̃f�[�^�𓾂�
const GLubyte* CamImage::getImage(int cam)
{
  return image[cam].data;
}

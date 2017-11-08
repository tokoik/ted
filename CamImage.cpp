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
bool CamImage::open(const std::string &file, int cam)
{
  // �摜���t�@�C������ǂݍ���
  image[cam] = cv::imread(defaults.camera_left_image);

  // �ǂݍ��݂Ɏ��s������߂�
  if (image[cam].empty()) return false;

  // �ǂݍ��񂾉摜�̃T�C�Y�����߂�
  size[cam][0] = image[cam].cols;
  size[cam][1] = image[cam].rows;

  return true;
}
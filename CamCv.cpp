//
// OpenCV ���g�����L���v�`��
//
#include "CamCv.h"

// �R���X�g���N�^
CamCv::CamCv()
  : startTime{}
{
}

// �f�X�g���N�^
CamCv::~CamCv()
{
  // �X���b�h���~����
  stop();

  // �J�������~����
  for (int cam = 0; cam < camCount; ++cam)
    if (opened(cam)) camera[cam].release();
}

// �J����������͂���
bool CamCv::open(int device, int cam, const char* codec, double width, double height, double fps)
{
  // �J�������J��
  if (!camera[cam].open(device)) return false;

  // �J������ݒ肷��
  setup(cam, codec, width, height, fps);

  // �L���v�`�����J�n����
  return start(cam);
}

// �t�@�C���^�l�b�g���[�N����L���v�`�����J�n����
bool CamCv::open(const std::string& file, int cam)
{
  // �t�@�C���^�l�b�g���[�N���J��
  return camera[cam].open(file) && start(cam);
}

// �J�������g�p�\�����肷��
bool CamCv::opened(int cam)
{
  return camera[cam].isOpened();
}

// �t���[�����L���v�`������
void CamCv::capture(int cam)
{
  // �X���b�h�����s�̊�
  while (run[cam])
  {
    // �o�b�t�@����̂Ƃ��o�ߎ��Ԃ����̃t���[���̎����ɒB���Ă���
    if (!buffer[cam] && glfwGetTime() - startTime[cam] >= camera[cam].get(cv::CAP_PROP_POS_MSEC) * 0.001)
    {
      // ���̃t���[�������݂����
      if (camera[cam].grab())
      {
        // �L���v�`���f�o�C�X�����b�N����
        captureMutex[cam].lock();

        // ���������t���[����؂�o����
        camera[cam].retrieve(image[cam]);

        // �摜���X�V��
        buffer[cam] = image[cam].data;

        // ��Ǝ҂Ƃ��ē��삵�Ă�����
        if (isWorker())
        {
          // �t���[�������k���ĕۑ���
          cv::imencode(encoderType, image[cam], encoded[cam], param);
        }

        // ���b�N����������
        captureMutex[cam].unlock();

        // ���̃t���[���ɐi��
        continue;
      }

      // �t���[�����擾�ł��Ȃ������烀�[�r�[�t�@�C���������߂�
      if (camera[cam].set(cv::CAP_PROP_POS_FRAMES, 0.0))
      {
        // ���̃t���[���̎��������߂�
        startTime[cam] = glfwGetTime() - camera[cam].get(cv::CAP_PROP_POS_MSEC) * 0.001 + interval[cam];

        // ���̃t���[���ɐi��
        continue;
      }
    }
  }
}

// �L���v�`������������
void CamCv::setup(int cam, const char* codec, double width, double height, double fps)
{
  // �J�����̃R�[�f�b�N�E�𑜓x�E�t���[�����[�g��ݒ肷��
  if (codec[0] != '\0') camera[cam].set(cv::CAP_PROP_FOURCC,
    cv::VideoWriter::fourcc(codec[0], codec[1], codec[2], codec[3]));
  if (width > 0.0) camera[cam].set(cv::CAP_PROP_FRAME_WIDTH, width);
  if (height > 0.0) camera[cam].set(cv::CAP_PROP_FRAME_HEIGHT, height);
  if (fps > 0.0) camera[cam].set(cv::CAP_PROP_FPS, fps);
}

// �L���v�`�����J�n����
bool CamCv::start(int cam)
{
  // ���J�������� 1 �t���[���L���v�`������
  if (camera[cam].read(image[cam]))
  {
    // �L���v�`�������摜�̃t���[�����[�g���擾����
    const double fps(camera[cam].get(cv::CAP_PROP_FPS));
    interval[cam] = fps > 0.0 ? 1.0 / fps : 0.033333333;

    // ���̃t���[���̎��������߂�
    startTime[cam] = glfwGetTime() - camera[cam].get(cv::CAP_PROP_POS_MSEC) * 0.001 + interval[cam];

    // �L���v�`�������摜�̃T�C�Y���擾����
    size[cam][0] = static_cast<GLsizei>(camera[cam].get(cv::CAP_PROP_FRAME_WIDTH));
    size[cam][1] = static_cast<GLsizei>(camera[cam].get(cv::CAP_PROP_FRAME_HEIGHT));

#if defined(DEBUG)
    const int cc(static_cast<int>(camera[cam].get(cv::CAP_PROP_FOURCC)));
    std::cerr << "Camera:" << cam << ", width:" << size[cam][0] << ", height:" << size[cam][1]
      << ", fourcc: " << static_cast<char>(cc & 255)
      << static_cast<char>((cc >> 8) & 255)
      << static_cast<char>((cc >> 16) & 255)
      << static_cast<char>((cc >> 24) & 255) << "\n";
#endif

    // �L���v�`���p�̃��������m�ۂ���
    image[cam].create(size[cam][0], size[cam][1], CV_8UC3);
    buffer[cam] = image[cam].data;

    // ���J�����̗����ƘI�o���擾����
    gain = static_cast<GLsizei>(camera[cam].get(cv::CAP_PROP_GAIN));
    exposure = static_cast<GLsizei>(camera[cam].get(cv::CAP_PROP_EXPOSURE) * 10.0);

    // �L���v�`���X���b�h���N������
    run[cam] = true;
    captureThread[cam] = std::thread([this, cam]() { capture(cam); });

    return true;
  }

  return false;
}

// �I�o���グ��
void CamCv::increaseExposure()
{
  const double e(static_cast<double>(++exposure) * 0.1);
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (camera[cam].isOpened()) camera[cam].set(cv::CAP_PROP_EXPOSURE, e);
  }
}

// �I�o��������
void CamCv::decreaseExposure()
{
  const double e(static_cast<double>(--exposure) * 0.1);
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (camera[cam].isOpened()) camera[cam].set(cv::CAP_PROP_EXPOSURE, e);
  }
}

// �������グ��
void CamCv::increaseGain()
{
  const double g(++gain);
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (camera[cam].isOpened()) camera[cam].set(cv::CAP_PROP_GAIN, g);
  }
}

// ������������
void CamCv::decreaseGain()
{
  const double g(--gain);
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (camera[cam].isOpened()) camera[cam].set(cv::CAP_PROP_GAIN, g);
  }
}

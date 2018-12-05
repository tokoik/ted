//
// OpenCV ���g�����L���v�`��
//
#include "CamCv.h"

// �R���X�g���N�^
CamCv::CamCv()
{
}

// �f�X�g���N�^
CamCv::~CamCv()
{
  // �X���b�h���~����
  stop();
}

// �J����������͂���
bool CamCv::open(int device, int cam)
{
  // �J�����̉𑜓x��ݒ肷��
  camera[cam].set(CV_CAP_PROP_FOURCC, CV_FOURCC('H', '2', '6', '4'));
  if (defaults.capture_width > 0.0) camera[cam].set(CV_CAP_PROP_FRAME_WIDTH, defaults.capture_width);
  if (defaults.capture_height > 0.0) camera[cam].set(CV_CAP_PROP_FRAME_HEIGHT, defaults.capture_height);
  if (defaults.capture_fps > 0.0) camera[cam].set(CV_CAP_PROP_FPS, defaults.capture_fps);

	// �J�������J���ăL���v�`�����J�n����
	return camera[cam].open(device) && start(cam);
}

// �t�@�C���^�l�b�g���[�N����L���v�`�����J�n����
bool CamCv::open(const std::string &file, int cam)
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
  // ���炩���߃L���v�`���f�o�C�X�����b�N����
  captureMutex[cam].lock();

  // �X���b�h�����s�̊�
  while (run[cam])
  {
    // �o�b�t�@����̂Ƃ��o�ߎ��Ԃ����݂̃t���[���̎����ɒB���Ă���
    if (!buffer[cam] && glfwGetTime() >= frameTime[cam])
    {
      // ���̃t���[�������݂����
      if (camera[cam].grab())
      {
        // ���������t���[����؂�o����
        camera[cam].retrieve(image[cam], 3);

        // �摜���X�V��
        buffer[cam] = image[cam].data;

        // ��Ǝ҂Ƃ��ē��삵�Ă�����
        if (isWorker())
        {
          // �t���[�������k���ĕۑ���
          cv::imencode(encoderType, image[cam], encoded[cam], param);
        }

        // ���̃t���[���ɐi��
        continue;
      }

      // �t���[�����擾�ł��Ȃ������烀�[�r�[�t�@�C���������߂�
      if (camera[cam].set(CV_CAP_PROP_POS_FRAMES, 0.0))
      {
        // �o�ߎ��Ԃ����Z�b�g����
        glfwSetTime(0.0);

        // �t���[���̎��������Z�b�g��
        frameTime[cam] = 0.0;

        // ���̃t���[���ɐi��
        continue;
      }
    }

    // �t���[�����؂�o���Ȃ���΃��b�N����������
    captureMutex[cam].unlock();

    // ���̃X���b�h�����\�[�X�ɃA�N�Z�X���邽�߂ɏ����҂��Ă���
    std::this_thread::sleep_for(std::chrono::milliseconds(minDelay));

    // �܂��L���v�`���f�o�C�X�����b�N����
    captureMutex[cam].lock();
  }

  // �I���Ƃ��̓��b�N����������
  captureMutex[cam].unlock();
}

// �L���v�`�����J�n����
bool CamCv::start(int cam)
{
  // ���J�������� 1 �t���[���L���v�`������
  if (camera[cam].grab())
  {
    // �ŏ��̃t���[�����擾������������ɂ���
    glfwSetTime(0.0);

    // �ŏ��̃t���[���̎����� 0 �ɂ���
    frameTime[cam] = 0.0;

    // �L���v�`�������摜�̃T�C�Y���擾����
    size[cam][0] = static_cast<GLsizei>(camera[cam].get(CV_CAP_PROP_FRAME_WIDTH));
    size[cam][1] = static_cast<GLsizei>(camera[cam].get(CV_CAP_PROP_FRAME_HEIGHT));

    // �L���v�`���p�̃��������m�ۂ���
    image[cam].create(size[cam][0], size[cam][1], CV_8UC3);
    buffer[cam] = image[cam].data;

    // ���J�����̗����ƘI�o���擾����
    gain = static_cast<GLsizei>(camera[cam].get(CV_CAP_PROP_GAIN));
    exposure = static_cast<GLsizei>(camera[cam].get(CV_CAP_PROP_EXPOSURE) * 10.0);

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
    if (camera[cam].isOpened()) camera[cam].set(CV_CAP_PROP_EXPOSURE, e);
  }
}

// �I�o��������
void CamCv::decreaseExposure()
{
  const double e(static_cast<double>(--exposure) * 0.1);
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (camera[cam].isOpened()) camera[cam].set(CV_CAP_PROP_EXPOSURE, e);
  }
}

// �������グ��
void CamCv::increaseGain()
{
  const double g(++gain);
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (camera[cam].isOpened()) camera[cam].set(CV_CAP_PROP_GAIN, g);
  }
}

// ������������
void CamCv::decreaseGain()
{
  const double g(--gain);
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (camera[cam].isOpened()) camera[cam].set(CV_CAP_PROP_GAIN, g);
  }
}

//
// Ovrvision Pro ���g�����L���v�`��
//
#include "CamOv.h"

// Ovrvision Pro SDK
#if defined(_WIN64)
#  if defined(_DEBUG)
#    pragma comment(lib, "ovrvision64d.lib")
#  else
#    pragma comment(lib, "ovrvision64.lib")
#  endif
#else
#  if defined(_DEBUG)
#    pragma comment(lib, "ovrvisiond.lib")
#  else
#    pragma comment(lib, "ovrvision.lib")
#  endif
#endif

// �R���X�g���N�^
CamOv::CamOv()
{
  // �L���v�`������摜�̃t�H�[�}�b�g
  format = GL_BGRA;

  // Ovrvision Pro �̐��𐔂���
  device = count++;
}

// �f�X�g���N�^
CamOv::~CamOv()
{
  // �X���b�h���~����
  stop();

  // ���ׂĂ� Ovrvsion Pro ���폜������f�o�C�X�����
  if (ovrvision_pro && --count == 0) ovrvision_pro->Close();
}

// Ovrvision Pro ����L���v�`������
void CamOv::capture()
{
  // ���炩���߃L���v�`���f�o�C�X�����b�N����
  captureMutex[camL].lock();

  // �X���b�h�����s�̊�
  while (run[camL])
  {
    // �����ꂩ�̃o�b�t�@����Ȃ�
    if (!buffer[camL] && !buffer[camR])
    {
      // �t���[����؂�o����
      ovrvision_pro->PreStoreCamData(OVR::Camqt::OV_CAMQT_DMS);

      // ���E�̃t���[���̃|�C���^�����o����
      auto *const bufferL(ovrvision_pro->GetCamImageBGRA(OVR::OV_CAMEYE_LEFT));
      auto *const bufferR(ovrvision_pro->GetCamImageBGRA(OVR::OV_CAMEYE_RIGHT));

      // �����Ƃ��t���[�������o���Ă�����
      if (bufferL && bufferR)
      {
        // �t���[�����X�V��
        buffer[camL] = bufferL;
        buffer[camR] = bufferR;

        // ��Ǝ҂Ƃ��ē��삵�Ă�����
        if (isWorker())
        {
          // �t���[�������k���ĕۑ�����
          cv::Mat frameL(size[camL][1], size[camL][0], CV_8UC4, bufferL);
          cv::imencode(encoderType, frameL, encoded[camL], param);
          cv::Mat frameR(size[camR][1], size[camR][0], CV_8UC4, bufferR);
          cv::imencode(encoderType, frameR, encoded[camR], param);
        }
      }
    }
    else
    {
      // ��łȂ���΃��b�N����������
      captureMutex[camL].unlock();

      // ���̃X���b�h�����\�[�X�ɃA�N�Z�X���邽�߂ɏ����҂��Ă���
      std::this_thread::sleep_for(std::chrono::milliseconds(minDelay));

      // �܂��L���v�`���f�o�C�X�����b�N����
      captureMutex[camL].lock();
    }
  }

  // �I���Ƃ��̓��b�N����������
  captureMutex[camL].unlock();
}

// Ovrvision Pro ���N������
bool CamOv::open(OVR::Camprop ovrvision_property)
{
  // Ovrvision Pro �̃h���C�o�ɐڑ�����
  if (!ovrvision_pro) ovrvision_pro = new OVR::OvrvisionPro;

  if (ovrvision_pro->Open(device, ovrvision_property, 0) != 0)
  {
    // ���J�����̃T�C�Y���擾����
    size[camR][0] = size[camL][0] = ovrvision_pro->GetCamWidth();
    size[camR][1] = size[camL][1] = ovrvision_pro->GetCamHeight();

    // ���J�����̗����ƘI�o���擾����
    gain = ovrvision_pro->GetCameraGain();
    exposure = ovrvision_pro->GetCameraExposure();

    // Ovrvision Pro �̏����ݒ���s��
    ovrvision_pro->SetCameraSyncMode(false);
    ovrvision_pro->SetCameraWhiteBalanceAuto(true);

    // �X���b�h���N������
    run[camL] = run[camR] = true;
    captureThread[camL] = std::thread([this](){ capture(); });

    return true;
  }

  return false;
}

// �I�o���グ��
void CamOv::increaseExposure()
{
  if (ovrvision_pro && exposure < 32767) ovrvision_pro->SetCameraExposure(exposure += 80);
}

// Ovrvision Pro �̘I�o��������
void CamOv::decreaseExposure()
{
  if (ovrvision_pro && exposure > 0) ovrvision_pro->SetCameraExposure(exposure -= 80);
}

// Ovrvision Pro �̗������グ��
void CamOv::increaseGain()
{
  if (ovrvision_pro && gain < 47) ovrvision_pro->SetCameraGain(++gain);
}

// Ovrvision Pro �̗�����������
void CamOv::decreaseGain()
{
  if (ovrvision_pro && gain > 0) ovrvision_pro->SetCameraGain(--gain);
}

// Ovrvision Pro
OVR::OvrvisionPro *CamOv::ovrvision_pro(nullptr);

// �ڑ�����Ă��� Ovrvision Pro �̑䐔
int CamOv::count(0);

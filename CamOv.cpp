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

// Ovrvision Pro
OVR::OvrvisionPro *CamOv::ovrvision_pro(nullptr);

// �ڑ�����Ă��� Ovrvision Pro �̑䐔
int CamOv::count(0);

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
  // �X���b�h�����s�̊�
  while (run[camL])
  {
    // �����̃o�b�t�@����Ȃ�
    if (!captured[camL] && !captured[camR])
    {
      // �L���v�`���f�o�C�X�����b�N���Ă���
      captureMutex[camL].lock();

      // �t���[����؂�o����
      ovrvision_pro->PreStoreCamData(OVR::Camqt::OV_CAMQT_DMS);

      // ��Ǝ҂Ƃ��ē��삵�Ă�����
      if (isWorker())
      {
        // �t���[�������k���ĕۑ���
        cv::imencode(encoderType, image[camL], encoded[camL], param);
        cv::imencode(encoderType, image[camR], encoded[camR], param);
      }

      // �L���v�`���̊������L�^����
      captured[camL] = captured[camR] = true;

      // ���b�N����������
      captureMutex[camL].unlock();
    }
  }
}

// Ovrvision Pro ���N������
bool CamOv::open(OVR::Camprop ovrvision_property)
{
  // Ovrvision Pro �̃h���C�o�ɐڑ�����
  if (!ovrvision_pro) ovrvision_pro = new OVR::OvrvisionPro;

  // Ovrvision Pro ���J��
  if (ovrvision_pro->Open(device, ovrvision_property, 0) == 0) return false;

  // �J�����̃T�C�Y���擾����
  cv::Size size{ ovrvision_pro->GetCamWidth(), ovrvision_pro->GetCamHeight() };

  // �t���[����؂�o��
  ovrvision_pro->PreStoreCamData(OVR::Camqt::OV_CAMQT_DMS);

  // ���E�̃t���[���̃������� cv::Mat �̃w�b�_��t����
  auto* const bufferL(ovrvision_pro->GetCamImageBGRA(OVR::OV_CAMEYE_LEFT));
  image[camL] = cv::Mat(size, CV_8UC4, bufferL);
  auto* const bufferR(ovrvision_pro->GetCamImageBGRA(OVR::OV_CAMEYE_RIGHT));
  image[camR] = cv::Mat(size, CV_8UC4, bufferR);

  // ���J�����̗����ƘI�o���擾����
  gain = ovrvision_pro->GetCameraGain();
  exposure = ovrvision_pro->GetCameraExposure();

  // Ovrvision Pro �̏����ݒ���s��
  ovrvision_pro->SetCameraSyncMode(false);
  ovrvision_pro->SetCameraWhiteBalanceAuto(true);

  // �X���b�h���N������
  run[camL] = run[camR] = true;
  captureThread[camL] = std::thread([this]() { capture(); });

  return true;
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

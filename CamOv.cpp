//
// Ovrvision Pro を使ったキャプチャ
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

// 接続されている Ovrvision Pro の台数
int CamOv::count(0);

// コンストラクタ
CamOv::CamOv()
{
  // キャプチャする画像のフォーマット
  format = GL_BGRA;

  // Ovrvision Pro の数を数える
  device = count++;
}

// デストラクタ
CamOv::~CamOv()
{
  // スレッドを停止する
  stop();

  // すべての Ovrvsion Pro を削除したらデバイスを閉じる
  if (ovrvision_pro && --count == 0) ovrvision_pro->Close();
}

// Ovrvision Pro からキャプチャする
void CamOv::capture()
{
  // スレッドが実行可の間
  while (run[camL])
  {
    // 両方のバッファが空なら
    if (!captured[camL] && !captured[camR])
    {
      // キャプチャデバイスをロックしてから
      captureMutex[camL].lock();

      // フレームを切り出して
      ovrvision_pro->PreStoreCamData(OVR::Camqt::OV_CAMQT_DMS);

      // 作業者として動作していたら
      if (isWorker())
      {
        // フレームを圧縮して保存し
        cv::imencode(encoderType, image[camL], encoded[camL], param);
        cv::imencode(encoderType, image[camR], encoded[camR], param);
      }

      // キャプチャの完了を記録して
      captured[camL] = captured[camR] = true;

      // ロックを解除する
      captureMutex[camL].unlock();
    }
  }
}

// Ovrvision Pro を起動する
bool CamOv::open(OVR::Camprop ovrvision_property)
{
  // Ovrvision Pro のドライバに接続する
  if (!ovrvision_pro) ovrvision_pro = new OVR::OvrvisionPro;

  // Ovrvision Pro を開く
  if (ovrvision_pro->Open(device, ovrvision_property, 0) == 0) return false;

  // カメラのサイズを取得する
  cv::Size size{ ovrvision_pro->GetCamWidth(), ovrvision_pro->GetCamHeight() };

  // フレームを切り出す
  ovrvision_pro->PreStoreCamData(OVR::Camqt::OV_CAMQT_DMS);

  // 左右のフレームのメモリに cv::Mat のヘッダを付ける
  auto* const bufferL(ovrvision_pro->GetCamImageBGRA(OVR::OV_CAMEYE_LEFT));
  image[camL] = cv::Mat(size, CV_8UC4, bufferL);
  auto* const bufferR(ovrvision_pro->GetCamImageBGRA(OVR::OV_CAMEYE_RIGHT));
  image[camR] = cv::Mat(size, CV_8UC4, bufferR);

  // 左カメラの利得と露出を取得する
  gain = ovrvision_pro->GetCameraGain();
  exposure = ovrvision_pro->GetCameraExposure();

  // Ovrvision Pro の初期設定を行う
  ovrvision_pro->SetCameraSyncMode(false);
  ovrvision_pro->SetCameraWhiteBalanceAuto(true);

  // スレッドを起動する
  run[camL] = run[camR] = true;
  captureThread[camL] = std::thread([this]() { capture(); });

  return true;
}

// 露出を上げる
void CamOv::increaseExposure()
{
  if (ovrvision_pro && exposure < 32767) ovrvision_pro->SetCameraExposure(exposure += 80);
}

// Ovrvision Pro の露出を下げる
void CamOv::decreaseExposure()
{
  if (ovrvision_pro && exposure > 0) ovrvision_pro->SetCameraExposure(exposure -= 80);
}

// Ovrvision Pro の利得を上げる
void CamOv::increaseGain()
{
  if (ovrvision_pro && gain < 47) ovrvision_pro->SetCameraGain(++gain);
}

// Ovrvision Pro の利得を下げる
void CamOv::decreaseGain()
{
  if (ovrvision_pro && gain > 0) ovrvision_pro->SetCameraGain(--gain);
}

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
  // あらかじめキャプチャデバイスをロックして
  captureMutex[camL].lock();

  // スレッドが実行可の間
  while (run[camL])
  {
    // いずれかのバッファが空なら
    if (!buffer[camL] && !buffer[camR])
    {
      // フレームを切り出して
      ovrvision_pro->PreStoreCamData(OVR::Camqt::OV_CAMQT_DMS);

      // 左右のフレームのポインタを取り出して
      auto *const bufferL(ovrvision_pro->GetCamImageBGRA(OVR::OV_CAMEYE_LEFT));
      auto *const bufferR(ovrvision_pro->GetCamImageBGRA(OVR::OV_CAMEYE_RIGHT));

      // 両方ともフレームが取り出せていたら
      if (bufferL && bufferR)
      {
        // フレームを更新し
        buffer[camL] = bufferL;
        buffer[camR] = bufferR;

        // 作業者として動作していたら
        if (isWorker())
        {
          // フレームを圧縮して保存する
          cv::Mat frameL(size[camL][1], size[camL][0], CV_8UC4, bufferL);
          cv::imencode(encoderType, frameL, encoded[camL], param);
          cv::Mat frameR(size[camR][1], size[camR][0], CV_8UC4, bufferR);
          cv::imencode(encoderType, frameR, encoded[camR], param);
        }
      }
    }
    else
    {
      // 空でなければロックを解除して
      captureMutex[camL].unlock();

      // 他のスレッドがリソースにアクセスするために少し待ってから
      std::this_thread::sleep_for(std::chrono::milliseconds(minDelay));

      // またキャプチャデバイスをロックする
      captureMutex[camL].lock();
    }
  }

  // 終わるときはロックを解除する
  captureMutex[camL].unlock();
}

// Ovrvision Pro を起動する
bool CamOv::open(OVR::Camprop ovrvision_property)
{
  // Ovrvision Pro のドライバに接続する
  if (!ovrvision_pro) ovrvision_pro = new OVR::OvrvisionPro;

  if (ovrvision_pro->Open(device, ovrvision_property, 0) != 0)
  {
    // 左カメラのサイズを取得する
    size[camR][0] = size[camL][0] = ovrvision_pro->GetCamWidth();
    size[camR][1] = size[camL][1] = ovrvision_pro->GetCamHeight();

    // 左カメラの利得と露出を取得する
    gain = ovrvision_pro->GetCameraGain();
    exposure = ovrvision_pro->GetCameraExposure();

    // Ovrvision Pro の初期設定を行う
    ovrvision_pro->SetCameraSyncMode(false);
    ovrvision_pro->SetCameraWhiteBalanceAuto(true);

    // スレッドを起動する
    run[camL] = run[camR] = true;
    captureThread[camL] = std::thread([this](){ capture(); });

    return true;
  }

  return false;
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

// Ovrvision Pro
OVR::OvrvisionPro *CamOv::ovrvision_pro(nullptr);

// 接続されている Ovrvision Pro の台数
int CamOv::count(0);

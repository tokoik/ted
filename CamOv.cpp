///
/// Ovrvision を使ってキャプチャするクラスの実装
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///
#include "CamOv.h"
#include "gg.h"

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
OVR::OvrvisionPro *CamOv::ovrvision_pro{ nullptr };

// 接続されている Ovrvision Pro の台数
int CamOv::count{ 0 };

//
// コンストラクタ
//
CamOv::CamOv()
{
  // Ovrvision Pro の数を数える
  device = count++;
}

//
// デストラクタ
//
CamOv::~CamOv()
{
  close();

  // すべての Ovrvsion Pro を削除したらデバイスを閉じる
  if (ovrvision_pro && --count == 0)
  {
    ovrvision_pro->Close();
    delete ovrvision_pro;
    ovrvision_pro = nullptr;
  }
}

//
// キャプチャ開始
//
bool CamOv::onStart()
{
  thr = std::thread([this]() { capture(); });
  return true;
}

//
// キャプチャ停止
//
void CamOv::onStop()
{
  // running フラグが false になるのを capture() ループで監視
}

//
// クローズ
//
void CamOv::onClose()
{
  imageR.clear();
  widthR = 0;
  heightR = 0;
  capturedR = false;
}

//
// Ovrvision Pro からキャプチャする
//
void CamOv::capture()
{
  const size_t sz{ static_cast<size_t>(width) * height * 4 };

  while (running)
  {
    if (ovrvision_pro)
    {
      ovrvision_pro->PreStoreCamData(OVR::Camqt::OV_CAMQT_DMS);
      auto* const bufferL{ ovrvision_pro->GetCamImageBGRA(OVR::OV_CAMEYE_LEFT) };
      auto* const bufferR{ ovrvision_pro->GetCamImageBGRA(OVR::OV_CAMEYE_RIGHT) };

      if (bufferL && bufferR)
      {
        std::lock_guard<std::mutex> lock{ mtx };
        if (image.size() >= sz)
        {
          std::memcpy(image.data(), bufferL, sz);
          captured = true;
          notifyFrame(0);
        }
        if (imageR.size() >= sz)
        {
          std::memcpy(imageR.data(), bufferR, sz);
          capturedR = true;
          notifyFrame(1);
        }
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

//
// Ovrvision Pro を起動する
//
bool CamOv::open(OVR::Camprop ovrvision_property)
{
  // Ovrvision Pro のドライバに接続する
  if (!ovrvision_pro) ovrvision_pro = new OVR::OvrvisionPro;

  // Ovrvision Pro を開く
  if (ovrvision_pro->Open(device, ovrvision_property, 0) == 0) return false;

  // カメラのサイズを取得する
  width = widthR = ovrvision_pro->GetCamWidth();
  height = heightR = ovrvision_pro->GetCamHeight();
  channels = 4;

  const size_t sz{ static_cast<size_t>(width) * height * 4 };
  image.resize(sz);
  imageR.resize(sz);

  // フレームを切り出す
  ovrvision_pro->PreStoreCamData(OVR::Camqt::OV_CAMQT_DMS);

  // 左カメラの利得と露出を取得する
  gain = ovrvision_pro->GetCameraGain();
  exposure = ovrvision_pro->GetCameraExposure();

  // Ovrvision Pro の初期設定を行う
  ovrvision_pro->SetCameraSyncMode(false);
  ovrvision_pro->SetCameraWhiteBalanceAuto(true);

  // スレッドを起動する
  start();

  return true;
}

//
// テクスチャへ転送する
//
bool CamOv::transmit(int eye, unsigned int texture, const int* size)
{
  return lockFrame(eye, [texture, size](const std::uint8_t* data, size_t length, int width, int height, int channels) {
    // 画像とテクスチャの大きさが違えば転送しない (バッファの範囲外を読まないようにする)
    if (width != size[0] || height != size[1]) return;

    // キャプチャした画像は 4 チャンネルの BGRA
    glPixelStorei(GL_UNPACK_ALIGNMENT, channels == 4 ? 4 : 1);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size[0], size[1],
      channels == 4 ? GL_BGRA : GL_BGR, GL_UNSIGNED_BYTE, data);
  });
}

//
// Ovrvision Pro の露出を上げる
//
void CamOv::increaseExposure()
{
  if (ovrvision_pro && exposure < 32767) ovrvision_pro->SetCameraExposure(exposure += 80);
}

//
// Ovrvision Pro の露出を下げる
//
void CamOv::decreaseExposure()
{
  if (ovrvision_pro && exposure > 0) ovrvision_pro->SetCameraExposure(exposure -= 80);
}

//
// Ovrvision Pro の利得を上げる
//
void CamOv::increaseGain()
{
  if (ovrvision_pro && gain < 47) ovrvision_pro->SetCameraGain(++gain);
}

//
// Ovrvision Pro の利得を下げる
//
void CamOv::decreaseGain()
{
  if (ovrvision_pro && gain > 0) ovrvision_pro->SetCameraGain(--gain);
}

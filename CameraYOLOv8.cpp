//
// カメラ関連の処理
//
#include "Camera.h"

// シーングラフ
#include "Scene.h"

#include "FaceDetection.h"

#include "yolov8face.h"

// OpenCV のライブラリのリンク
#if defined(_WIN32)
#  define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#  if defined(_DEBUG)
#    define CV_EXT_STR "d.lib"
#  else
#    define CV_EXT_STR ".lib"
#  endif
#  pragma comment(lib, "opencv_world" CV_VERSION_STR CV_EXT_STR)
#endif

// OpenCV の Legacy な定数
#include <opencv2/imgcodecs/legacy/constants_c.h>

//extern void myFaceDetection(Mat frame, Mat frame2);

// コンストラクタ
Camera::Camera()
{
  // 作業用のメモリ領域
  recvbuf = sendbuf = nullptr;

  // キャプチャされる画像のフォーマット
  format = GL_BGR;

  // 圧縮設定
  param.push_back(CV_IMWRITE_JPEG_QUALITY);
  param.push_back(defaults.remote_texture_quality);

  for (int cam = 0; cam < camCount; ++cam)
  {
    // 画像がまだ取得されていないことを記録しておく
    buffer[cam] = nullptr;

    // スレッドが停止状態であることを記録しておく
    run[cam] = false;
    captured[cam] = false;
    unsent[cam] = false;
  }
}

// デストラクタ
Camera::~Camera()
{
  // 作業用のメモリを開放する
  delete[] recvbuf;
  delete[] sendbuf;
  recvbuf = sendbuf = nullptr;
}


// スレッドを停止する
void Camera::stop()
{
  // キャプチャスレッドを止める
  for (int cam = 0; cam < camCount; ++cam)
  {
    if (run[cam])
    {
      // キャプチャスレッドのループを止める
      captureMutex[cam].lock();
      run[cam] = false;
      captureMutex[cam].unlock();

      // キャプチャスレッドが終了するのを待つ
      if (captureThread[cam].joinable()) captureThread[cam].join();
    }
  }

  // ネットワークスレッドを止める
  if (useNetwork())
  {
    // 受信スレッドが終了するのを待つ
    if (recvThread.joinable()) recvThread.join();

    // 送信スレッドが終了するのを待つ
    if (sendThread.joinable()) sendThread.join();
  }
}


/*
// カメラをロックして画像をテクスチャに転送する
bool Camera::transmit(int cam, GLuint texture, const GLsizei *size)
{
	cv::Mat img( size[1], size[0], CV_8UC3);
	cv::Mat imgwork( size[1], size[0], CV_8UC3);

    static int FirstFlag = 1;
    static cv::CascadeClassifier cascade; //カスケード分類器格納場所
    std::vector<cv::Rect> faces; //輪郭情報を格納場所

    if (FirstFlag) {
        cascade.load("C:\libs\etc\haarcascades\haarcascade_frontalface_alt.xml"); //正面顔情報が入っているカスケード
        FirstFlag = 0;
    }

  // カメラのロックを試みる
  if (captureMutex[cam].try_lock())
  {
    // 新しいデータが到着していたら
    if (buffer[cam])
    {
		// カメラの画像データbuffer[cam]を画像データimgにコピー
		for (int v = 0; v < size[1]; v++) {
			for (int u = 0; u < size[0]; u++) {
				for (int i = 0; i < 3; i++)
					img.at<cv::Vec3b>(v, u)[i] = *(buffer[cam] + 3 * (v*size[0] + u) + i);
	//			// B:50 G:100 R:150
			}
		}

		// ガウシアンフィルタ
//		cv::Mat gaussian;
//		cv::GaussianBlur( img, imgwork, cv::Size(10, 10), 0.0);
		// 平均値フィルタ
	    // # Size(x, y)でx方向、y方向のフィルタサイズを指定する
//        if (isWorker())
          cv::blur(img, imgwork, cv::Size(10, 10));
//            myFaceDetection( img);

        cascade.detectMultiScale(imgwork, faces, 1.1, 3, 0, cv::Size(20, 20)); //カスケードファイルに基づいて顔を検知する．検知した顔情報をベクトルfacesに格納

        for (int i = 0; i < faces.size(); i++) //検出した顔の個数"faces.size()"分ループを行う
        {
            cv::rectangle(img, cv::Point(faces[i].x, faces[i].y), cv::Point(faces[i].x + faces[i].width, faces[i].y + faces[i].height), cv:: Scalar(0, 0, 255), 3); //検出した顔を赤色矩形で囲む
        }

        cv::imshow("detect face", imgwork);

//            myFaceDetection(img, imgwork);

      // データをテクスチャに転送する
      glBindTexture(GL_TEXTURE_2D, texture);
	//  if(isWorker())
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size[0], size[1], format, GL_UNSIGNED_BYTE, img.data);
	//  else
	//	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size[0], size[1], format, GL_UNSIGNED_BYTE, buffer[cam]);



      // データの転送完了を記録する
      buffer[cam] = nullptr;
    }

    // 左カメラのロックを解除する
    captureMutex[cam].unlock();

    // テクスチャを転送した
    return true;
  }

  // テクスチャを転送していない
  return false;
}

*/




// カメラをロックして画像をテクスチャに転送する
bool Camera::transmit(int cam, GLuint texture, const GLsizei* size)
{
    static int FirstFlag = 1;
    static std::vector<cv::Rect> faces; //輪郭情報を格納場所
    static YOLOv8_face YOLOv8_face_model("weights/yolov8n-face.onnx", 0.25, 0.5);


    if (FirstFlag) {

        FirstFlag = 0;
    }

  // カメラのロックを試みる
  if (captureMutex[cam].try_lock())
  {
    // 新しいデータが到着していたら
    if (buffer[cam])
    {
      // カメラの画像データ buffer[cam] に cv::Mat のヘッダを付ける
      cv::Mat img(cv::Size(size[0], size[1]), CV_8UC3, const_cast<GLubyte*>(buffer[cam]));

      // ぼかし処理フィルタ
      // # Size(x, y)でx方向、y方向のフィルタサイズを指定する
      cv::Mat imgwork;


      if (1)
      {

          imgwork = img.clone();
//        cv::blur(img, imgwork, cv::Size(20, 20));

      // 他のスレッドがリソースにアクセスするために少し待つ
//          std::this_thread::sleep_for(std::chrono::milliseconds(minDelay));

          YOLOv8_face_model.detect(imgwork);


          // 他のスレッドがリソースにアクセスするために少し待つ
//      std::this_thread::sleep_for(std::chrono::milliseconds(5));
          buffer[cam] = imgwork.data;

          // 他のスレッドがリソースにアクセスするために少し待つ
//      std::this_thread::sleep_for(std::chrono::milliseconds(minDelay));
      }

      // データをテクスチャに転送する
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size[0], size[1], format, GL_UNSIGNED_BYTE, buffer[cam]);

      // 他のスレッドがリソースにアクセスするために少し待つ
  // std::this_thread::sleep_for(std::chrono::milliseconds(minDelay));

      // データの転送完了を記録する
      buffer[cam] = nullptr;
    }

    // 左カメラのロックを解除する
    captureMutex[cam].unlock();

    // テクスチャを転送した
    return true;
  }

  // テクスチャを転送していない
  return false;
}





/*

// カメラをロックして画像をテクスチャに転送する
bool Camera::transmit(int cam, GLuint texture, const GLsizei* size)
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 修正箇所その１（初期設定）
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//　　ここから
    static int FirstFlag = 1;
    static cv::CascadeClassifier cascade; //カスケード分類器格納場所
    static std::vector<cv::Rect> faces; //輪郭情報を格納場所

//    YOLOv8_face YOLOv8_face_model("weights/yolov9_s_wholebody_with_wheelchair_relu_post_0100_1x3x480x640.onnx", 0.45, 0.5);
//    YOLOv8_face YOLOv8_face_model("weights/yolov8n-face.onnx", 0.45, 0.5);

//    YOLOv8_face YOLOv8_face_model("weights/yolov8n-face.onnx", 0.25, 0.5);


//    YOLOv8_face YOLOv8_face_model("weights/yolox_s_body_head_hand_post_0299_0.4983_1x3x256x320.onnx",  0.45, 0.5);

   if (FirstFlag) {
//        cascade.load("C:/development/opencv/build/etc/haarcascades/haarcascade_frontalface_alt.xml"); //正面顔情報が入っているカスケード


        FirstFlag = 0;
    }
//　ここまで
    // カメラのロックを試みる
    if (captureMutex[cam].try_lock())
    {
        // 新しいデータが到着していたら
        if (buffer[cam])
        {
            // カメラの画像データ buffer[cam] に cv::Mat のヘッダを付ける
            cv::Mat img(cv::Size(size[0], size[1]), CV_8UC3, const_cast<GLubyte*>(buffer[cam]));

            // ぼかし処理フィルタ
            // # Size(x, y)でx方向、y方向のフィルタサイズを指定する
//            cv::Mat imgwork;
            cv::Mat imgwork(cv::Size(size[0], size[1]), CV_8UC3, const_cast<GLubyte*>(buffer[cam]));


            if (1)
            {

                imgwork = img.clone();
                //        cv::blur(img, imgwork, cv::Size(20, 20));


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 修正箇所その２（顔検出処理本体）
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//　　ここから

                cascade.detectMultiScale(imgwork, faces, 1.1, 3, 0, cv::Size(20, 20)); //カスケードファイルに基づいて顔を検知する．検知した顔情報をベクトルfacesに格納

                for (int i = 0; i < faces.size(); i++) //検出した顔の個数"faces.size()"分ループを行う
                {
                    cv::rectangle(imgwork, cv::Point(faces[i].x, faces[i].y), cv::Point(faces[i].x + faces[i].width, faces[i].y + faces[i].height), cv::Scalar(0, 0, 255), -1); //検出した顔を赤色矩形で囲む
                }


//                YOLOv8_face_model.detect(imgwork);

  //　ここまで


                buffer[cam] = imgwork.data;
            }

            // データをテクスチャに転送する
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size[0], size[1], format, GL_UNSIGNED_BYTE, buffer[cam]);

            // データの転送完了を記録する
            buffer[cam] = nullptr;
        }

          // 他のスレッドがリソースにアクセスするために少し待つ
//      std::this_thread::sleep_for(std::chrono::milliseconds(minDelay));

        // 左カメラのロックを解除する
        captureMutex[cam].unlock();

        // テクスチャを転送した
        return true;
    }

    // テクスチャを転送していない
    return false;
}

*/



// カメラをロックして画像をテクスチャに転送する
bool Camera::transmittedBuffer(int cam, GLuint texture, const GLsizei *size, cv::Mat &img)
{
//	cv::Mat img(size[0], size[1], CV_8UC3);
//	cv::Mat imgwork(size[0], size[1], CV_8UC3);

	// カメラのロックを試みる
	if (captureMutex[cam].try_lock())
	{
		// 新しいデータが到着していたら
		if (buffer[cam])
		{
			// データをテクスチャに転送する
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size[0], size[1], format, GL_UNSIGNED_BYTE, buffer[cam]);

			for (int v = 0; v < size[1]; v++) {
				for (int u = 0; u < size[0]; u++) {
					for (int i = 0; i < 3; i++)
						img.at<cv::Vec3b>(u, v)[i] = *(buffer[cam] + 3 * (v*size[0] + u) + i);
					// B:50 G:100 R:150
				}
			}
			// データの転送完了を記録する
			buffer[cam] = nullptr;
		}
		int ixsize = img.size().width;
		int iysize = img.size().height;

		std::cout << ixsize << std::endl;
		std::cout << iysize << std::endl;

		cv::imshow("test", img);

		// 左カメラのロックを解除する
		captureMutex[cam].unlock();

		// テクスチャを転送した
		return true;
	}

	// テクスチャを転送していない
	return false;
}





// リモモートの姿勢を受信する
void Camera::recv()
{
  // スレッドが実行可の間
  while (run[camL])
  {
    // 姿勢データを受信する
    const int ret{ network.recvData(recvbuf, maxFrameSize) };

    // サイズが 0 なら終了する
    if (ret == 0) return;

    // エラーがなければデータを読み込む
    if (ret > 0 && network.checkRemote())
    {
      // ヘッダのフォーマット
      unsigned int* const head{ reinterpret_cast<unsigned int*>(recvbuf) };

      // 受信した変換行列の格納場所
      GgMatrix* const body{ reinterpret_cast<GgMatrix*>(head + headLength) };

      // 変換行列を共有メモリに格納する (head[camCount] には変換行列の数が入っている)
      Scene::storeRemoteAttitude(body, head[camCount]);
    }

    // 他のスレッドがリソースにアクセスするために少し待つ
    std::this_thread::sleep_for(std::chrono::milliseconds(minDelay));
  }
}

// ローカルの映像と姿勢を送信する
void Camera::send()
{
  // 直前のフレームの送信時刻
  double last(glfwGetTime());

  // カメラスレッドが実行可の間
  while (run[camL])
  {
    // ヘッダのフォーマット
    unsigned int* const head{ reinterpret_cast<unsigned int*>(sendbuf) };

    // 左右フレームのサイズを 0 にしておく
    head[camL] = head[camR] = 0;

    // 左右のフレームサイズの次に変換行列の数を保存する
    head[camCount] = Scene::getLocalAttitudeSize();

    // 送信する変換行列の格納場所
    GgMatrix* const body{ reinterpret_cast<GgMatrix*>(head + headLength) };

    // 変換行列を共有メモリから取り出す
    Scene::loadLocalAttitude(body, head[camCount]);

    // 左フレームの保存先 (変換行列の最後)
    uchar* data{ reinterpret_cast<uchar*>(body + head[camCount]) };

    // このフレームの遅延時間
    long long delay{ minDelay };

    // 符号化されたデータの一時保存先
    std::vector<GLubyte> encoded;

    // 左画像が未送信なら
    if (unsent[camL])
    {
      // 左画像をロックして
      captureMutex[camL].lock();

      // 左画像を圧縮して保存し
      cv::imencode(encoderType, image[camL], encoded, param);

      // 左画像の送信準備の完了を記録して
      unsent[camL] = false;

      // 左画像のロックを解除してから
      captureMutex[camL].unlock();

      // 左フレームのサイズを保存して
      head[camL] = static_cast<unsigned int>(encoded.size());

      // 左フレームのデータをコピーしたら
      memcpy(data, encoded.data(), head[camL]);

      // 右フレームの保存先 (左フレームの最後)
      data += head[camL];

      // 右キャプチャデバイスが動作していて右画像が未送信なら
      if (run[camR] && unsent[camR])
      {
        // 右画像をロックして
        captureMutex[camR].lock();

        // 右画像を圧縮して保存し
        cv::imencode(encoderType, image[camR], encoded, param);

        // 右画像の送信準備の完了を記録して
        unsent[camR] = false;

        // 右画像のロックを解除してから
        captureMutex[camR].unlock();

        // 右フレームのサイズを保存して
        head[camR] = static_cast<unsigned int>(encoded.size());

        // 右フレームのデータを左フレームのデータの後ろにコピーしたら
        memcpy(data, encoded.data(), head[camR]);

        // 右フレームの最後
        data += head[camR];
      }

      // フレームを送信する
      network.sendData(sendbuf, static_cast<unsigned int>(data - sendbuf));

      // 現在時刻
      const double now{ glfwGetTime() };

      // 次のフレームの送信時刻までの残り時間
      const long long remain{ static_cast<long long>((last + capture_interval - now) * 1000.0) };

#if defined(DEBUG)
      std::cerr << "send remain = " << remain << '\n';
#endif

      // 残り時間分遅延させる
      if (remain > delay) delay = remain;

      // 直前のフレームの送信時刻を更新する
      last = now;
    }

    // 次のフレームの送信時刻まで待つ
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
  }

  // ループを抜けるときに EOF を送信する
  network.sendEof();
}

// 作業者通信スレッド起動
int Camera::startWorker(unsigned short port, const char* address)
{
  // すでに確保されている作業用メモリを破棄する
  delete[] recvbuf;
  delete[] sendbuf;
  recvbuf = sendbuf = nullptr;

  // 作業者として初期化する
  const int ret(network.initialize(2, port, address));
  if (ret > 0) return ret;

  // 作業用のメモリを確保する（これは Camera のデストラクタで delete する）
  recvbuf = new uchar[maxFrameSize];
  sendbuf = new uchar[maxFrameSize];

  // 通信スレッドを開始する
  run[camL] = true;
  recvThread = std::thread([this]() { recv(); });
  sendThread = std::thread([this]() { send(); });

  return 0;
}

//
// キャプチャしたフレームの処理
//

// カメラ関連の処理
#include "Camera.h"

// 例外
#include <exception>

// OpenCV
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

// YOLOv8 による顔検出
#include "yolov8face.h"

//
// 何もしない
//
static void none(cv::Mat& frame)
{
}

//
// フレーム全体をぼかす
//
static void blurFrame(cv::Mat& frame)
{
  cv::blur(frame, frame, cv::Size(20, 20));
}

//
// Haar Like 特徴量で顔を見つける
//
static void detectHaarLike(cv::Mat& frame)
{
  // 最初に実行するときだけ true
  static bool FirstTime{ true };

  // カスケード分類器格納場所
  static cv::CascadeClassifier cascade;

  // 最初に一度だけ
  if (FirstTime)
  {
    // 正面顔情報が入っているカスケードファイルを読み込む
    if (!cascade.load("libs/etc/haarcascades/haarcascade_frontalface_alt.xml"))
    {
      // カスケードファイルが読み込めなければエラー
      throw std::runtime_error("Can't load cascade file.");
    }

    // 実行したことを記録する
    FirstTime = false;
  }

  // カスケードファイルにもとづいて顔を検出して顔領域を faces に格納する
  std::vector<cv::Rect> faces;
  cascade.detectMultiScale(frame, faces);

  // 検出したすべての顔領域に枠を付ける
  for (const auto face : faces) cv::rectangle(frame, face, cv::Scalar(0, 0, 255));
}

//
// Haar Like 特徴量で見つけた顔をぼかす
//
static void blurHaarLike(cv::Mat& frame)
{
  // 最初に実行するときだけ true
  static bool FirstTime{ true };

  // カスケード分類器格納場所
  static cv::CascadeClassifier cascade;

  // 最初に一度だけ
  if (FirstTime)
  {
    // 正面顔情報が入っているカスケードファイルを読み込む
    if (!cascade.load("libs/etc/haarcascades/haarcascade_frontalface_alt.xml"))
    {
      // カスケードファイルが読み込めなければエラー
      throw std::runtime_error("Can't load cascade file.");
    }

    // 実行したことを記録する
    FirstTime = false;
  }

  // カスケードファイルにもとづいて顔を検出して顔領域を faces に格納する
  std::vector<cv::Rect> faces;
  cascade.detectMultiScale(frame, faces);

  // 検出したすべての顔領域に対して
  for (const auto face : faces)
  {
    // ROI
    cv::Mat roi{ frame(face) };

    // ぼかす
    cv::blur(roi, roi, cv::Size(20, 20));
    
    // 枠を付ける
    cv::rectangle(frame, face, cv::Scalar(0, 0, 255));
  }
}

//
// YOLOv8 で見つけた顔をぼかす
//
static void blurYOLOv8(cv::Mat& frame)
{
  // 顔のモデル
  static YOLOv8_face YOLOv8_face_model("weights/yolov8n-face.onnx", 0.25, 0.5);
  //static YOLOv8_face YOLOv8_face_model("weights/yolov9_s_wholebody_with_wheelchair_relu_post_0100_1x3x480x640.onnx", 0.45, 0.5);
  //static YOLOv8_face YOLOv8_face_model("weights/yolov8n-face.onnx", 0.45, 0.5);
  //static YOLOv8_face YOLOv8_face_model("weights/yolov8n-face.onnx", 0.25, 0.5);
  //static YOLOv8_face YOLOv8_face_model("weights/yolox_s_body_head_hand_post_0299_0.4983_1x3x256x320.onnx",  0.45, 0.5);

  // 検出した顔領域をぼかす」
  YOLOv8_face_model.detect(frame);
}

// キャプチャしたフレームを加工する
void (*Camera::processFrame)(cv::Mat& frame)
{
  //none
  //blurFrame
  //detectHaarLike
  //blurHaarLike
  blurYOLOv8
};

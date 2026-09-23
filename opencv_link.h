#pragma once

///
/// MSVC環境用のOpenCV自動リンクおよびインクルード一括管理ヘッダー
///
/// @file
///

// OpenCV
#pragma warning(disable:4819)
#include <opencv2/opencv.hpp>

#if defined(_WIN32)
#  define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#  if defined(_DEBUG)
#    define CV_EXT_STR "d.lib"
#  else
#    define CV_EXT_STR ".lib"
#  endif
#  pragma comment(lib, "opencv_world" CV_VERSION_STR CV_EXT_STR)
#endif

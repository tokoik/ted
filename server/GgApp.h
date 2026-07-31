#pragma once

///
/// Network.cpp が要求する最小限の定義のみを提供するスタブ
///
/// @file
/// @author Kohe Tokoi
/// @date July 31, 2026
///
/// @details
/// ted-server はリポジトリルートの Network.h/.cpp をこのディレクトリに
/// そのまま複製して使う（TED本体との差分が出ないよう維持すること）。
/// 本来の GgApp.h は GLFW/OpenXR/ImGui/OpenCV 等 GUI 一式に依存するため、
/// ヘッドレスの中継サーバである ted-server には持ち込まない。
/// Network.cpp が参照する NOTIFY マクロと maxDropPackets 定数だけを
/// ここで肩代わりする。
///

// windows.h の min/max マクロが std::max 等と衝突するのを防ぐ
// (本家 gg.h も同じ定義をしている。Network.cpp より先に効かせる必要がある)
#if defined(_WIN32)
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#endif

#include <iostream>

// リポジトリルートの Config.h にある既定値と同じ値を保つこと
constexpr int maxDropPackets{ 1000 };

// GUI を持たないためメッセージボックスは出さず、標準エラー出力へ流す
#define NOTIFY(msg) std::cerr << msg << '\n'

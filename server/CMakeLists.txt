cmake_minimum_required(VERSION 3.22)
project(ted_server LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "ted-server は現在 Windows 専用です。")
endif()

add_executable(ted_server
  main.cpp
  Network.cpp
)

target_compile_features(ted_server PRIVATE cxx_std_17)

if(MSVC)
  target_compile_options(ted_server PRIVATE /utf-8)
  target_compile_definitions(ted_server PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# <chrono>/<thread> 等の標準ヘッダが Windows.h を先に引き込んでしまうと
# std::max/std::min が min/max マクロに壊されるため、
# コンパイル単位の先頭で確実に効くよう target 単位で定義する
target_compile_definitions(ted_server PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

target_link_libraries(ted_server PRIVATE ws2_32)

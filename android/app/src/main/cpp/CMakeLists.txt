cmake_minimum_required(VERSION 3.22)
project(ted_android LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include NDK native_app_glue
add_library(native_app_glue STATIC "${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c")
target_include_directories(native_app_glue PUBLIC "${ANDROID_NDK}/sources/android/native_app_glue")

# Configure OpenXR SDK subproject for Android
set(XR_USE_PLATFORM_ANDROID ON CACHE BOOL "" FORCE)
set(XR_USE_GRAPHICS_API_OPENGL_ES ON CACHE BOOL "" FORCE)
set(BUILD_API_LAYERS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_CONFORMANCE_TESTS OFF CACHE BOOL "" FORCE)
set(DYNAMIC_LOADER OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_SYSTEM_JSONCPP OFF CACHE BOOL "" FORCE)

set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../libs")
file(MAKE_DIRECTORY "${LIBS_DIR}")
set(OPENXR_SDK_DIR "${LIBS_DIR}/OpenXR-SDK-release-1.1.61")

if(NOT EXISTS "${OPENXR_SDK_DIR}/CMakeLists.txt")
    message(STATUS "Downloading OpenXR SDK v1.1.61 for Android build...")
    file(DOWNLOAD "https://github.com/KhronosGroup/OpenXR-SDK/archive/refs/tags/release-1.1.61.zip" "${LIBS_DIR}/openxr.zip" SHOW_PROGRESS)
    message(STATUS "Extracting OpenXR SDK...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf openxr.zip WORKING_DIRECTORY "${LIBS_DIR}")
    file(REMOVE "${LIBS_DIR}/openxr.zip")
endif()

add_subdirectory("${OPENXR_SDK_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/openxr_sdk" EXCLUDE_FROM_ALL)

# Main Android OpenXR library
add_library(ted_android SHARED AndroidMain.cpp)

target_compile_definitions(ted_android PRIVATE
    XR_USE_PLATFORM_ANDROID
    XR_USE_GRAPHICS_API_OPENGL_ES
    EGL_EGLEXT_PROTOTYPES
    GL_GLEXT_PROTOTYPES
)

target_include_directories(ted_android PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${OPENXR_SDK_DIR}/include"
)

find_library(log-lib log)
find_library(android-lib android)
find_library(egl-lib EGL)
find_library(gles-lib GLESv3)

target_link_libraries(ted_android PRIVATE
    openxr_loader
    native_app_glue
    ${egl-lib}
    ${gles-lib}
    ${android-lib}
    ${log-lib}
)

target_link_options(ted_android PRIVATE "-Wl,-u,ANativeActivity_onCreate")


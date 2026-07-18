cmake_minimum_required(VERSION 3.22)

# Ignore MSYS2 / MinGW directories when compiling with MSVC to prevent header/library conflicts
if(MSVC)
    list(APPEND CMAKE_IGNORE_PREFIX_PATH "C:/msys64" "C:/msys64/ucrt64")
    list(APPEND CMAKE_IGNORE_PATH "C:/msys64" "C:/msys64/ucrt64")
    set(CMAKE_DISABLE_FIND_PACKAGE_JsonCpp TRUE CACHE BOOL "Disable JsonCpp find" FORCE)
    set(CMAKE_DISABLE_FIND_PACKAGE_jsoncpp TRUE CACHE BOOL "Disable jsoncpp find" FORCE)
endif()

project(ted LANGUAGES C CXX)

# Enforce 64-bit builds on Windows
if(MSVC)
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Only 64-bit (x64) builds are supported on Windows.")
    endif()
endif()

# Configure C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add preprocessor definitions
add_compile_definitions(
    PROJECT_NAME="ted"
    UNICODE
    _UNICODE
    OPENCV_WORLD
)

set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")

# Ensure base directories exist
file(MAKE_DIRECTORY "${LIBS_DIR}")
file(MAKE_DIRECTORY "${LIBS_DIR}/include")
file(MAKE_DIRECTORY "${LIBS_DIR}/lib")
file(MAKE_DIRECTORY "${LIBS_DIR}/bin")

# --- Automated Dependency Downloads at Configure Time ---

# 1. OpenGL & KHR headers
if(NOT EXISTS "${LIBS_DIR}/include/GL/glcorearb.h")
    message(STATUS "Downloading OpenGL & KHR headers...")
    file(MAKE_DIRECTORY "${LIBS_DIR}/include/GL")
    file(MAKE_DIRECTORY "${LIBS_DIR}/include/KHR")
    set(UA_HEADER "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36")
    file(DOWNLOAD "https://registry.khronos.org/OpenGL/api/GL/glcorearb.h" "${LIBS_DIR}/include/GL/glcorearb.h" HTTPHEADER "${UA_HEADER}")
    file(DOWNLOAD "https://registry.khronos.org/OpenGL/api/GL/glext.h" "${LIBS_DIR}/include/GL/glext.h" HTTPHEADER "${UA_HEADER}")
    file(DOWNLOAD "https://registry.khronos.org/OpenGL/api/GL/wgl.h" "${LIBS_DIR}/include/GL/wgl.h" HTTPHEADER "${UA_HEADER}")
    file(DOWNLOAD "https://registry.khronos.org/OpenGL/api/GL/wglext.h" "${LIBS_DIR}/include/GL/wglext.h" HTTPHEADER "${UA_HEADER}")
    file(DOWNLOAD "https://registry.khronos.org/EGL/api/KHR/khrplatform.h" "${LIBS_DIR}/include/KHR/khrplatform.h" HTTPHEADER "${UA_HEADER}")
endif()

# 2. GLFW
if(NOT EXISTS "${LIBS_DIR}/glfw-3.4.bin.WIN64/lib-vc2022/glfw3.lib")
    message(STATUS "Downloading GLFW 3.4 Win64...")
    file(DOWNLOAD "https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.WIN64.zip" "${LIBS_DIR}/glfw.zip" SHOW_PROGRESS)
    message(STATUS "Extracting GLFW...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf glfw.zip WORKING_DIRECTORY "${LIBS_DIR}")
    file(REMOVE "${LIBS_DIR}/glfw.zip")
endif()

# 3. picojson
if(NOT EXISTS "${LIBS_DIR}/include/picojson.h")
    message(STATUS "Downloading picojson...")
    file(DOWNLOAD "https://github.com/kazuho/picojson/raw/refs/heads/master/picojson.h" "${LIBS_DIR}/include/picojson.h" SHOW_PROGRESS)
endif()

# 4. Dear ImGui
if(NOT EXISTS "${LIBS_DIR}/ImGui/imgui.cpp")
    message(STATUS "Downloading Dear ImGui v1.92.8...")
    file(DOWNLOAD "https://github.com/ocornut/imgui/archive/refs/tags/v1.92.8.zip" "${LIBS_DIR}/imgui.zip" SHOW_PROGRESS)
    message(STATUS "Extracting Dear ImGui...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf imgui.zip WORKING_DIRECTORY "${LIBS_DIR}")
    file(RENAME "${LIBS_DIR}/imgui-1.92.8" "${LIBS_DIR}/ImGui")
    file(REMOVE "${LIBS_DIR}/imgui.zip")
endif()

# 5. Native File Dialog Extended
if(NOT EXISTS "${LIBS_DIR}/ImGui/nfd.h")
    message(STATUS "Downloading Native File Dialog Extended v1.3.0...")
    file(DOWNLOAD "https://github.com/btzy/nativefiledialog-extended/archive/refs/tags/v1.3.0.zip" "${LIBS_DIR}/nfd.zip" SHOW_PROGRESS)
    message(STATUS "Extracting Native File Dialog Extended...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf nfd.zip WORKING_DIRECTORY "${LIBS_DIR}")
    set(NFD_SRC "${LIBS_DIR}/nativefiledialog-extended-1.3.0/src")
    file(COPY "${NFD_SRC}/include/nfd.h" DESTINATION "${LIBS_DIR}/ImGui")
    file(COPY "${NFD_SRC}/include/nfd.hpp" DESTINATION "${LIBS_DIR}/ImGui")
    file(COPY "${NFD_SRC}/nfd_win.cpp" DESTINATION "${LIBS_DIR}/ImGui")
    file(REMOVE_RECURSE "${LIBS_DIR}/nativefiledialog-extended-1.3.0")
    file(REMOVE "${LIBS_DIR}/nfd.zip")
endif()

# 6. メニューフォント
if(NOT EXISTS "${LIBS_DIR}/NotoSansCJKjp-Regular.otf")
    message(STATUS "Downloading NotoSansCJKjp-Regular.otf...")
    file(DOWNLOAD "https://github.com/notofonts/noto-cjk/raw/refs/heads/main/Sans/OTF/Japanese/NotoSansCJKjp-Regular.otf" "${LIBS_DIR}/NotoSansCJKjp-Regular.otf" SHOW_PROGRESS)
endif()

# 7. Leap Motion SDK
if(NOT EXISTS "${LIBS_DIR}/include/LeapC.h")
    message(STATUS "Downloading Leap Motion SDK 4.1.0 (Orion)...")
    file(DOWNLOAD "https://s3.eu-west-1.amazonaws.com/downloads.ultraleap.com/software/tracking-software/4.1.0/tracking-software-windows-4.1.0.zip" "${LIBS_DIR}/leap.zip" SHOW_PROGRESS)
    message(STATUS "Extracting Leap Motion SDK...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf leap.zip WORKING_DIRECTORY "${LIBS_DIR}")

    file(GLOB_RECURSE LEAPC_H_FILE "${LIBS_DIR}/*/LeapC.h")
    if(LEAPC_H_FILE)
        get_filename_component(LEAPC_SDK_DIR "${LEAPC_H_FILE}" DIRECTORY)
        get_filename_component(LEAPC_SDK_DIR "${LEAPC_SDK_DIR}" DIRECTORY) # LeapSDK root
        file(COPY "${LEAPC_SDK_DIR}/include/LeapC.h" DESTINATION "${LIBS_DIR}/include")
        file(COPY "${LEAPC_SDK_DIR}/lib/x64/LeapC.lib" DESTINATION "${LIBS_DIR}/lib")
        file(COPY "${LEAPC_SDK_DIR}/lib/x64/LeapC.dll" DESTINATION "${LIBS_DIR}/bin")
    else()
        message(FATAL_ERROR "LeapC.h not found in extracted Leap Motion SDK.")
    endif()
    file(REMOVE "${LIBS_DIR}/leap.zip")
endif()

# 8. OvrvisionPro SDK
if(NOT EXISTS "${LIBS_DIR}/include/ovrvision_pro.h")
    message(STATUS "Downloading OvrvisionPro SDK v2.0...")
    file(DOWNLOAD "https://github.com/Wizapply/OvrvisionPro/releases/download/v2.0/ovrvisionprosdk_windows.zip" "${LIBS_DIR}/ovrvision.zip" SHOW_PROGRESS)
    message(STATUS "Extracting OvrvisionPro SDK...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf ovrvision.zip WORKING_DIRECTORY "${LIBS_DIR}")

    file(GLOB_RECURSE OVR_H_FILE "${LIBS_DIR}/*/ovrvision_pro.h")
    if(OVR_H_FILE)
        get_filename_component(OVR_SDK_DIR "${OVR_H_FILE}" DIRECTORY)
        get_filename_component(OVR_SDK_DIR "${OVR_SDK_DIR}" DIRECTORY) # Ovrvision SDK root
        file(COPY "${OVR_SDK_DIR}/include/ovrvision_pro.h" DESTINATION "${LIBS_DIR}/include")
        file(COPY "${OVR_SDK_DIR}/include/ovrvision_ar.h" DESTINATION "${LIBS_DIR}/include")
        file(COPY "${OVR_SDK_DIR}/include/ovrvision_tracking.h" DESTINATION "${LIBS_DIR}/include")
        file(COPY "${OVR_SDK_DIR}/bin/x64/ovrvision64.lib" DESTINATION "${LIBS_DIR}/lib")
        file(COPY "${OVR_SDK_DIR}/bin/x64/ovrvision64d.lib" DESTINATION "${LIBS_DIR}/lib")
        file(COPY "${OVR_SDK_DIR}/bin/x64/ovrvision.dll" DESTINATION "${LIBS_DIR}/bin")
        file(COPY "${OVR_SDK_DIR}/bin/x64/ovrvisiond.dll" DESTINATION "${LIBS_DIR}/bin")
    else()
        message(FATAL_ERROR "ovrvision_pro.h not found in extracted OvrvisionPro SDK.")
    endif()
    file(REMOVE "${LIBS_DIR}/ovrvision.zip")
endif()

# 9. OpenCV
if(NOT EXISTS "${LIBS_DIR}/opencv/build/include/opencv2/opencv.hpp")
    message(STATUS "Downloading OpenCV 4.13.0 Windows...")
    file(DOWNLOAD "https://github.com/opencv/opencv/releases/download/4.13.0/opencv-4.13.0-windows.exe" "${LIBS_DIR}/opencv.exe" SHOW_PROGRESS)
    message(STATUS "Extracting OpenCV...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf opencv.exe WORKING_DIRECTORY "${LIBS_DIR}")
    file(REMOVE "${LIBS_DIR}/opencv.exe")
endif()

# 10. OpenXR SDK
if(NOT EXISTS "${LIBS_DIR}/OpenXR-SDK-release-1.1.61/CMakeLists.txt")
    message(STATUS "Downloading OpenXR SDK v1.1.61...")
    file(DOWNLOAD "https://github.com/KhronosGroup/OpenXR-SDK/archive/refs/tags/release-1.1.61.zip" "${LIBS_DIR}/openxr.zip" SHOW_PROGRESS)
    message(STATUS "Extracting OpenXR SDK...")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf openxr.zip WORKING_DIRECTORY "${LIBS_DIR}")
    file(REMOVE "${LIBS_DIR}/openxr.zip")
endif()

# --- OpenXR SDK Subproject Configuration ---
set(BUILD_API_LAYERS OFF CACHE BOOL "Build API layers" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(BUILD_CONFORMANCE_TESTS OFF CACHE BOOL "Build conformance tests" FORCE)
set(DYNAMIC_LOADER OFF CACHE BOOL "Build loader as dynamic library" FORCE) # build static loader
set(BUILD_WITH_SYSTEM_JSONCPP OFF CACHE BOOL "Use bundled jsoncpp" FORCE)
add_subdirectory("${LIBS_DIR}/OpenXR-SDK-release-1.1.61" "${CMAKE_CURRENT_BINARY_DIR}/openxr_sdk" EXCLUDE_FROM_ALL)

# --- File Collection & Source Group Setup ---

# Shaders
file(GLOB SHADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
)

# Assets / Resources
file(GLOB ASSET_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.jpg"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.obj"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.mtl"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.lwo"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ico"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.gif"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.otf"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.ttf"
)

# Source and Header Files
file(GLOB SOURCES_CPP "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB SOURCES_H "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

# Exclude unused files
list(REMOVE_ITEM SOURCES_CPP
    "${CMAKE_CURRENT_SOURCE_DIR}/CameraYOLOv8.cpp"
)

# Dear ImGui / Native File Dialog sources
set(IMGUI_SOURCES
    "${LIBS_DIR}/ImGui/imgui.cpp"
    "${LIBS_DIR}/ImGui/imgui.h"
    "${LIBS_DIR}/ImGui/imgui_draw.cpp"
    "${LIBS_DIR}/ImGui/imgui_widgets.cpp"
    "${LIBS_DIR}/ImGui/imgui_tables.cpp"
    "${LIBS_DIR}/ImGui/imgui_demo.cpp"
    "${LIBS_DIR}/ImGui/backends/imgui_impl_glfw.cpp"
    "${LIBS_DIR}/ImGui/backends/imgui_impl_glfw.h"
    "${LIBS_DIR}/ImGui/backends/imgui_impl_opengl3.cpp"
    "${LIBS_DIR}/ImGui/backends/imgui_impl_opengl3.h"
    "${LIBS_DIR}/ImGui/backends/imgui_impl_opengl3_loader.h"
    "${LIBS_DIR}/ImGui/nfd.h"
    "${LIBS_DIR}/ImGui/nfd.hpp"
    "${LIBS_DIR}/ImGui/nfd_win.cpp"
)

# Organize source files into filters for Visual Studio
source_group("Source Files" FILES ${SOURCES_CPP})
source_group("Header Files" FILES ${SOURCES_H})
source_group("Shader Files" FILES ${SHADER_FILES})
source_group("ImGui Files" FILES ${IMGUI_SOURCES})

# Mark config.json and shaders as non-compilable headers so MSBuild does not try to compile or link them
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/config.json" ${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

# Target executable definition
add_executable(ted
    ${SOURCES_CPP}
    ${SOURCES_H}
    ${IMGUI_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/ted.rc"
    "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
    ${SHADER_FILES}
)

# MSVC specific compile settings
if(MSVC)
    target_compile_options(ted PRIVATE /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# Include directories
target_include_directories(ted PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${LIBS_DIR}/ImGui"
    "${LIBS_DIR}/ImGui/backends"
    "${LIBS_DIR}/include"
    "${LIBS_DIR}/glfw-3.4.bin.WIN64/include"
    "${LIBS_DIR}/opencv/build/include"
)

# Link directories
target_link_directories(ted PRIVATE
    "${LIBS_DIR}/lib"
    "${LIBS_DIR}/glfw-3.4.bin.WIN64/lib-vc2022"
    "${LIBS_DIR}/opencv/build/x64/vc16/lib"
)

# Link dependencies
target_link_libraries(ted PRIVATE
    openxr_loader
    glfw3
    debug opencv_world4130d
    optimized opencv_world4130
    LeapC
    debug ovrvision64d
    optimized ovrvision64
    opengl32
    comctl32
    ole32
    uuid
    shell32
    user32
    gdi32
    winmm
    shlwapi
    strmiids
    mf
    mfplat
    mfuuid
    mfreadwrite
)

# --- Post-Build Operations: Copy Shaders, Assets and DLLs ---
add_custom_command(TARGET ted POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SHADER_FILES}
        ${ASSET_FILES}
        $<TARGET_FILE_DIR:ted>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<IF:$<CONFIG:Debug>,${LIBS_DIR}/opencv/build/x64/vc16/bin/opencv_world4130d.dll,${LIBS_DIR}/opencv/build/x64/vc16/bin/opencv_world4130.dll>"
        "${LIBS_DIR}/opencv/build/x64/vc16/bin/opencv_videoio_ffmpeg4130_64.dll"
        "${LIBS_DIR}/bin/LeapC.dll"
        "$<IF:$<CONFIG:Debug>,${LIBS_DIR}/bin/ovrvisiond.dll,${LIBS_DIR}/bin/ovrvision.dll>"
        $<TARGET_FILE_DIR:ted>
    COMMENT "Copying assets, shaders and DLL dependencies to target directory..."
)

# --- Visual Studio Debugger Configuration ---
set_target_properties(ted PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:ted>"
    VS_DEBUGGER_ENVIRONMENT "PATH=$<TARGET_FILE_DIR:ted>;%PATH%"
)

# Set ted as Startup Project in VS solution
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ted)

# Include Visual Studio Installer Project
include_external_msproject(INSTALL_MSI "${CMAKE_CURRENT_SOURCE_DIR}/INSTALL/INSTALL.vdproj"
    TYPE "54435603-DBB4-11D2-8724-00A0C9A8B90C"
    GUID "27EA2BED-E087-3708-B1D1-34290B2E3F34"
)
set_target_properties(INSTALL_MSI PROPERTIES
    EXCLUDE_FROM_ALL TRUE
)
add_dependencies(INSTALL_MSI ted)

# --- Cleanup Temp Directories at the very end of CMake configuration ---
file(GLOB TEMP_DIRS RELATIVE "${LIBS_DIR}" "${LIBS_DIR}/*")
foreach(td ${TEMP_DIRS})
    if(IS_DIRECTORY "${LIBS_DIR}/${td}"
       AND NOT td STREQUAL "include"
       AND NOT td STREQUAL "lib"
       AND NOT td STREQUAL "bin"
       AND NOT td STREQUAL "ImGui"
       AND NOT td STREQUAL "glfw-3.4.bin.WIN64"
       AND NOT td STREQUAL "opencv"
       AND NOT td STREQUAL "OpenXR-SDK-release-1.1.61")
        file(REMOVE_RECURSE "${LIBS_DIR}/${td}")
    endif()
endforeach()


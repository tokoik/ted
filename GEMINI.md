# プロジェクト開発方針と環境定義 (GEMINI.md)

本ドキュメントは、`ted-openxr` プロジェクトにおけるビルド構成、開発・動作環境に関する要件を定義します。

## 1. 開発環境・環境設定

- **統合開発環境 (IDE)**: Visual Studio 2022 以降
- **ビルドシステム**: CMake 3.22 以降
- **開発言語**: C++17 (`/std:c++17`)
- **ターゲットアーキテクチャ**: 64bit (x64)
- **プラットフォーム**: Windows (macOS や Linux は対象外)
- **ファイル文字コード**:
  - **C++ ソースファイル（`.h`, `.cpp`）**: 文字コードは `UTF-8` とし、**BOM を付与**します。
  - **GLSL ソースファイル（`.vert`, `.frag`, `.geom`, `.comp`）**: 文字コードは `UTF-8` とし、**BOM は付与しません**。
  - **Markdown ファイル（`.md`）**: 文字コードは `UTF-8` とし、**BOM は付与しません**。

## 2. Visual Studio フィルタ構成定義

- **Source Files**: C++ のソースファイル (`.cpp`)
- **Header Files**: C++ のヘッダファイル (`.h`)
- **Shader Files**: GLSL のソースファイル (`.vert`, `.frag`, `.geom`, `.comp`)
- **ImGui Files**: Dear ImGui および Native File Dialog Extended のソース・ヘッダファイル
- **フォルダ直下**: `config.json` は CMakeLists.txt と同レベルの `ted` フォルダ直下に配置する。その他の JSON/画像/動画/3Dモデルファイルはソリューションエクスプローラーには表示しない。

## 3. 使用する主要技術・外部ライブラリ

- **グラフィックス**: OpenGL 4.3 以降（GLSL 4.3 以降）
- **ウィンドウ・入力**: GLFW 3.4
- **UI**: Dear ImGui 1.92.8 / Native File Dialog Extended 1.3.0
- **メニューフォント**: NotoSansCJKjp-Regular.otf
- **JSONパース**: picojson
- **画像処理**: OpenCV 4.13.0
- **VR**: OpenXR SDK 1.1.61
- **ハンドトラッキング / 外部デバイス**:
  - Leap Motion SDK 4.1.0 (Orion)
  - OvrvisionPro SDK 2.0
- **その他**: GStreamer は使用しない。

## 4. 外部ライブラリの自動管理化 (libs)

- プロジェクトトップの `libs` ディレクトリ以下に、CMake 実行時に必要なライブラリやヘッダ、フォントを自動ダウンロード・配置する構成とする（ジャンクションの廃止）。
- ビルド先のバイナリディレクトリには、実行に必要な DLL、シェーダーファイル、リソースファイルを自動的にコピーする。

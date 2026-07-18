# TelExistence Display System (TED)

遠隔地の視覚的環境を観測者の周囲に再現する実験システム

    Copyright (c) 2016 Kohe Tokoi. All Rights Reserved.
    
    Permission is hereby granted, free of charge,  to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction,  including without limitation the rights
    to use, copy,  modify, merge,  publish, distribute,  sublicense,  and/or sell
    copies or substantial portions of the Software.
    
    The above  copyright notice  and this permission notice  shall be included in
    all copies or substantial portions of the Software.
    
    THE SOFTWARE  IS PROVIDED "AS IS",  WITHOUT WARRANTY OF ANY KIND,  EXPRESS OR
    IMPLIED,  INCLUDING  BUT  NOT LIMITED  TO THE WARRANTIES  OF MERCHANTABILITY,
    FITNESS  FOR  A PARTICULAR PURPOSE  AND NONINFRINGEMENT.  IN  NO EVENT  SHALL
    KOHE TOKOI  BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER LIABILITY,  WHETHER IN
    AN ACTION  OF CONTRACT,  TORT  OR  OTHERWISE,  ARISING  FROM,  OUT OF  OR  IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


## 概要

このシステムは遠隔地の情景や空間的情報、および光源環境を取得し、観測者の周囲にあるように再現する実験のためのシステムです。
観測者があたかも遠隔地にいるように知覚させることのほか、取得した情報の一部を観測者側のものに置き換えることによって、遠隔地のものが観測者の手元にあるように見せることも目的にしています。
ただし、あくまで実験を目的としたものであり、特定の目的を果たすアプリケーションとして完成したものではありません。


### 応用

本システムをもとに [UZUME Project](http://kazusa.net/uzume/) における「遠隔代理科学者」、すなわちロボットの視覚への応用を目指した実験システムが開発されています。


## 機能

* 本システムを 2 台の PC で動作させ、一方の HMD (作業者 HMD) の視界及びヘッドトラッキング情報を、ネットワークを介して、もう一方の HMD (指示者 HMD) と共有することができます。
* 作業者及び指示者の HMD には、OpenXR に対応する各種ヘッドマウントディスプレイ（Meta Quest, Oculus Rift, HTC Vive, Windows Mixed Reality HMD 等）が使用できます。また、HMD 以外にも通常の平面ディスプレイ (単眼視) のほか、サイドバイサイド、トップアンドボトム、および時分割多重 (フレームシーケンシャル) 方式の3Dディスプレイへの表示も可能です。
* 環境映像の取得には、全方位 (全天球) カメラの RICOH THETA S のほか、魚眼カメラの KODAK SP360 4K、魚眼レンズ (フジノンFE185C046HA-1) を取り付けた USB カメラ (センテックSTC-MCE132U3V)、一般の Web カメラ、および広角ステレオカメラ Ovrvision Pro が使用できます。
* 映像の取得に魚眼カメラや全方位カメラを用いた場合は、それを平面投影像に展開し、環境映像から指示者の視野の映像を切り出す機能を有します。また、パンチルトカメラの映像を合成して全天球画像として用いる機能も持っています。
* 広角ステレオカメラ Ovrvision Pro や 2 台の Web カメラにより取得した立体視映像を表示する機能をもつほか、HMD あるいはパンチルトヘッドに装着した 2 台の全天球カメラ (RICOH THETA S) の映像を安定化する (映像からカメラの方向変化を除去して並進のみにする) 機能を有します。
* 表示映像に Alias OBJ 形式の三次元形状データを半透明で重畳表示する機能を有します。これにはワールド座標に固定するもの、装着する HMD の動きに追従するもの、もう一方の HMD に追従するものを別々に指定できます。ワールド座標に固定したものは静止物体として環境中に配置され、配置はマウスやゲームパッドを用いて調整することができます。装着する HMD に追従するものには、装着者の視線のマーカや、後述する Leap Motion で制御する「手」のモデルを指定します。もう一方 of HMD に追従するものには、指示者の視線や手の動きを作業者に伝えるマーカや「手」のモデル、あるいは作業者の視線を指示者にフィードバックするためのマーカを指定します。
* Alias OBJ 形式の三次元形状データはパーツ間の階層構造や骨格などのロボットの表現に必要となる機能を持たないため、JSON (JavaScript Object Notation) による独自形式の前景グラフで前景の記述を行います。階層化されたパーツ間の相対的な位置関係は前景グラフ内に記述できるほか、共有メモリ (名前付きファイルマッピングオブジェクト) 上に置いた 4 行 4 列の変換行列を指定することもできます。これにより、パーツ間の位置関係を外部プログラムから制御することができます。この記述仕様は [scenegraph.md](scenegraph.md) に示しています。
* 指示者の手の動きをモーションコントローラの Leap Motion で取得し、それを剛体変換の行列として前述の共有メモリに格納します。これにより、任意の形状を Leap Motion で制御する「手」のモデルとして使用することができます。


## 開発環境とビルドシステム

本システムは **CMake 3.22+** および **Visual Studio 2022 (C++17, x64)** をベースとしたモダンなビルドシステムに移行しました。
すべての外部依存ライブラリは、CMakeの構成時（Configure）にインターネットから自動的にダウンロード・展開・配置されるため、事前に手動でライブラリ（OpenCV や Leap Motion SDK など）をシステムへインストールしたり、環境変数を設定したりする必要はありません。

### 動作・開発要件

* **OS**: Windows 10/11 (64-bit)
* **開発ツール**: Visual Studio 2022 以降 (「C++ によるデスクトップ開発」ワークロードがインストールされていること)
* **ビルドツール**: CMake 3.22 以降
* **グラフィックスAPI**: OpenGL 4.3 以降
* **HMD / VR 実行環境**: OpenXR 対応ランタイム (Oculus Link, SteamVR, WMR など)
  * 従来の Oculus SDK (LibOVR) から **OpenXR SDK** へ移行し、業界標準の規格に対応しました。
* **インストーラーのビルド**:
  * [Visual Studio 2022 Installer Projects](https://marketplace.visualstudio.com/items?itemName=VisualStudioProductTeam.MicrosoftVisualStudio2022InstallerProjects) 拡張機能
  * ソリューション内の `INSTALL` プロジェクトをビルドすることで、必要なアセットやDLLを含んだ `.msi` および `setup.exe` インストーラーが作成できます。

### 自動取得される外部ライブラリ

CMakeビルドシステムによって、以下のライブラリが自動的にプロジェクトローカル (`libs` ディレクトリ) に取得され、静的/動的リンクされます。

1. **OpenGL & KHR ヘッダ**: クロノス・グループ公式の OpenGL コアヘッダ
2. **GLFW 3.4**: ウィンドウ作成およびコンテキスト管理
3. **picojson**: JSON 形式の設定ファイル読み出し用ヘッダ
4. **Dear ImGui v1.92.8**: メニューおよび GUI コントロール
5. **Native File Dialog Extended v1.3.0**: ファイルオープンダイアログ
6. **Leap Motion SDK 4.1.0 (Orion)**: モーションコントローラー（Leap Motion）対応
7. **OvrvisionPro SDK v2.0**: 広角ステレオカメラ Ovrvision Pro 対応
8. **OpenCV 4.13.0**: 魚眼レンズ展開、画像処理およびビデオキャプチャ (※ OpenCV World ビルドを適用)
9. **OpenXR SDK v1.1.61**: HMD対応用の標準SDK
10. **Noto Sans CJK JP フォント**: 日本語メニュー表示用フォント

これらのオープンソースおよびSDK開発キットの作成者の方々に感謝いたします。

---

## ビルド・実行方法

### 1. 構成・ビルド手順 (コマンドライン)
Windows 標準のコマンドプロンプトや PowerShell（または Visual Studio 付属の Developer Command Prompt など、他のコンパイラ環境のパスが干渉しないクリーンな環境）で以下を実行します。

```powershell
# プロジェクト構成と依存ライブラリの自動ダウンロード・展開
cmake -S . -B build -G "Visual Studio 17 2022" -A x64

# プロジェクトのビルド (Release 構成)
cmake --build build --config Release
```

### 2. デバッグ・実行手順
1. 生成された `build/ted.sln` を Visual Studio 2022 で開きます。
2. 構成を `Debug` または `Release` に設定します。
3. **`ted`** プロジェクトを「スタートアッププロジェクト」に設定します。
4. **F5キー** を押してデバッグ実行を開始します。
   * ビルド完了時に、シェーダーファイル、3Dモデルアセット、および各種 DLL（`LeapC.dll`, `opencv_world4130.dll`, `ovrvision.dll` 等）が実行ファイルと同レベルの出力フォルダ (`build/Release` または `build/Debug`) に自動的にコピーされます。
   * Visual Studio から実行した際のデバッグ作業ディレクトリや環境変数 (`PATH`) も CMake を通じて自動構成されます。

---

## ドキュメント

* [config.md](config.md)
  + 設定ファイル (`config.json`) の詳細仕様および動作モードの説明
* [scenegraph.md](scenegraph.md)
  + シーングラフ記述ファイル (`scene.json`) の書式および C++ `Scene` クラスの仕様


## 謝辞

本研究は、平成29年度科学研究費助成事業 (基盤研究 (C), 課題番号:17K00271) の助成により実施されました。

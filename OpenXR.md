# TED OpenXR表示

## 概要

TEDのOpenXR対応は、calib-openxr と同じ設計のシングルトン `GgApp::OpenXR` が担当します。OpenXRのinstance、session、参照空間、左右眼のswapchain、フレーム同期、ミラー表示、コントローラーの入力、ハンドトラッキング、頭部中心姿勢を、このクラスが管理します。

GLFWウィンドウとOpenGLコンテキストを所有する `GgApp::Window` は、デスクトップ表示と操作を担当し、OpenXRについては次の2点だけを受け持ちます。

* 表示モードの切り替えに合わせて `GgApp::OpenXR` を開始・停止する
* HMDの視点の姿勢と視野角から、背景とシーンの描画に使う変換行列とスクリーンを求める（`updateHMD()`）

主な公開操作は次のとおりです。

* `GgApp::Window::setDisplayMode(OPENXR)`: OpenXRのsessionを作成し、成功した場合だけ表示モードを確定する
* `GgApp::Window::setDisplayMode(OPENXR以外)`: OpenXR使用中なら終了して通常表示へ戻る
* `GgApp::Window::startHMD()`／`stopHMD()`: `GgApp::OpenXR::initialize()`／`terminate()` を呼ぶ低水準API
* `GgApp::OpenXR::begin()`、`select(eye)`、`commit(eye)`、`submit()`: OpenXR表示のフレーム描画API
* `GgApp::Window::start()`、`select(eye)`、`commit(eye)`: デスクトップ表示のフレーム描画API
* `GgApp::Window::swapBuffers()`: Dear ImGuiのメニューを重ねてPCウィンドウを更新する（両方の表示で共通）

メニューからの切り替えには、開始・停止と設定値の更新を一括して行う `setDisplayMode()` を使用します。

## ビルド

OpenXR SDK 1.1.61は常にビルド構成へ含まれます。CMakeのConfigure時にSDKがなければ `libs/OpenXR-SDK-release-1.1.61` へ取得し、static loaderを構築して `ted` にリンクします。現在、OpenXRだけを無効化するCMakeオプションはありません。

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --target ted -- /m
```

OpenGL graphics bindingはWin32用です。実行には、現在のOpenGLコンテキストを受け入れるOpenXRランタイムとHMDが必要です。

## ウィンドウとOpenGLの準備

`GgApp::Window` のコンストラクタは、最初のウィンドウを開く前にOpenGL 4.3 Core Profileと sRGB 対応のフレームバッファを要求し、Dear ImGuiのコンテキストを作成します（`IMGUI_CHECKVERSION()`、`ImGui::CreateContext()`）。ウィンドウを開いた後は `ggInit()` でOpenGLの関数を読み込み、コールバック関数を登録してから、Dear ImGuiのGLFW／OpenGL3バックエンドを初期化し（`"#version 430"`）、`menu_font` の日本語フォントを読み込みます。デストラクタではOpenXRを停止してからバックエンドを終了し、ImGuiのコンテキストはプログラムの終了時に破棄します。

`ImGui::NewFrame()` と `ImGui::Render()` は `Menu::show()` が呼びます。`GgApp::Window` は、メニュー表示中だけバックエンドの新規フレームを作成し（`operator bool()`）、`swapBuffers()` で描画データを描きます。

## OpenXR表示の開始

起動時に `config.json` の `stereo` が `5`（`OPENXR`）なら、`setDisplayMode(OPENXR)` から `startHMD()` を呼びます。実行中は表示設定メニューの「OpenXR」を選ぶと `setDisplayMode(OPENXR)` が呼ばれます。コマンドラインの `--openxr` オプションはありません。

`GgApp::OpenXR::initialize()` は次の処理を行います。

1. 必須の `XR_KHR_opengl_enable` と、利用可能なら `XR_EXT_hand_tracking` を有効にしてinstanceを作成する
2. HMD systemを取得し、OpenGLのバージョンがランタイムの要件を満たすか確かめる
3. 現在のOpenGLコンテキストを結び付けたWin32 OpenGL bindingでsessionを作成する
4. `STAGE` 基準空間を作り、利用できない場合は `LOCAL` へフォールバックする
5. コントローラーのアクションを作成する
6. 利用可能なら左右のhand trackerを作成する
7. `PRIMARY_STEREO` の各viewについて、ランタイム推奨サイズのcolor swapchainと、FBO、デプスバッファ（`GL_DEPTH24_STENCIL8` のrenderbuffer）を作成する
8. PCウィンドウの垂直同期を無効にする（フレームの速度は `xrWaitFrame()` が制御する）

color形式はランタイムの列挙結果から `GL_SRGB8_ALPHA8`、`GL_SRGB8`、`GL_RGBA8`、`GL_RGB10_A2` の順で選び、どれもなければ列挙の先頭の形式を使います。環境の合成方法はランタイムが最も推奨するものを使います。

途中で失敗した場合は、確保済みの資源を `terminate()` で解放してから `std::runtime_error` を投げます。`startHMD()` はこれを受け止めて `false` を返します。

`initialize()` が成功した時点では、sessionはまだ実行中ではありません。ランタイムが `READY` 状態を通知したとき、`begin()` の中のイベント処理で `xrBeginSession()` を呼びます。そのため、session作成済みかどうかは `isInitialized()`、実行中かどうかは `isRunning()` で区別します。

## フレーム処理

`ted.cpp` の描画ループは、表示モードが `OPENXR` でsessionを作成済みなら次のように描画します。

```cpp
auto& openxr{ GgApp::OpenXR::getInstance() };
if (openxr.begin())
{
  // HMD の姿勢と視野角から左右の目の変換行列とスクリーンを求める
  window.updateHMD();

  // シーングラフの基準モデル変換と頭部中心姿勢、手の姿勢を設定する
  Scene::setup(mm);
  Scene::setLocalAttitude(camL, headPose);
  Scene::setLocalAttitude(camR, headPose);
  openxr.updateOpenXRHands(openxr.getPredictedDisplayTime());

  for (int eye = 0; eye < 2; ++eye)
  {
    openxr.select(eye);
    rect->draw(eye, window.getMo(eye), window.getSamples());
    scene->draw(window.getMp(eye), window.getMo(eye) * window.getMv(eye));
    openxr.commit(eye);
  }

  openxr.submit(window.isMirrorVisible());
}
window.swapBuffers();
```

各APIの役割は次のとおりです。

### `GgApp::OpenXR::begin()`

OpenXRのイベントを処理した後、実行中のsessionについて `xrWaitFrame()`、`xrBeginFrame()` を行い、コントローラーの状態と、予測表示時刻における左右眼の姿勢（`xrLocateViews()`）を取得します。ランタイムが `shouldRender == false` を返したフレームや、視点の位置と向きが得られなかったフレームは、ここでレイヤーを持たない `xrEndFrame()` を送って `false` を返します。このとき `ted.cpp` はPCウィンドウを消去し、メニューだけを表示します。

視点の姿勢が得られたとき、シーン座標の原点がまだ決まっていなければ、左右眼の中点（頭部中心）の位置を原点として保存します。

### `GgApp::Window::updateHMD()`

`begin()` で取得した左右眼ごとに次を更新します。

* `mo[eye]`: 眼の四元数姿勢に対する逆回転 `R^-1`（背景の描画とシーンのview行列に使う）
* `mv[eye]`: シーン座標の原点を差し引いた眼の位置の逆平行移動 `T^-1`
* `mp[eye]`: OpenXRの非対称FOVから作る投影行列。ズーム（`foreAdjust`）と視差の補正を反映する
* `screen[eye]`: 焦点距離（`backAdjust`）を反映した、背景の描画に使うスクリーンの大きさと中心

### `GgApp::OpenXR::select(eye)`

対象眼のswapchain imageを `xrAcquireSwapchainImage()` と `xrWaitSwapchainImage()` で取得し、そのimageとデプスバッファを取り付けたFBOを描画先に設定します。viewportをswapchain推奨サイズへ変更し、sRGBのswapchainなら `GL_FRAMEBUFFER_SRGB` を有効にします。

### `GgApp::OpenXR::commit(eye)`

描画先をPCウィンドウのフレームバッファへ戻します。swapchain imageはミラー表示に使うので、ここでは解放しません。

### `GgApp::OpenXR::submit(mirror)`

ミラー表示が有効なら、ミラー表示するview（既定では左眼）のswapchain imageを、縦横比を保ってPCウィンドウの中央へ転送します。その後、ウィンドウのviewportを戻し、swapchain imageを解放して、左右の `XrCompositionLayerProjectionView` を1個のprojection layerにまとめ、`begin()` で得た予測表示時刻で `xrEndFrame()` を呼びます。

### `GgApp::Window::swapBuffers()`

メニュー表示中なら、`GL_FRAMEBUFFER_SRGB` を有効にしてDear ImGuiのメニューを重ね、PCウィンドウのカラーバッファを入れ替えます。

## 背景ステレオ画像

SBS (`side_by_side`) とTAB (`top_and_bottom`) の入力フレームは、取得直後に左右の片眼画像へ分割され、OpenXRの各眼swapchainへ通常の左右別テクスチャとして描画されます。伝送解像度とリモート展開解像度も分割後の片眼単位です。

機器内のフレーム配置と実際の左右眼が逆の場合は、`camera_layout` はそのままにして `swap_camera_eyes: true` を指定します。入力設定画面の「左右の画像を入れ替える」も同じ設定です。交換は画像コピーではなく、物理入力から論理眼テクスチャへの割り当てで行います。

入力プロファイルを切り替えたときは、テクスチャと同時に `fisheye_fov_*` と `fisheye_center_*` を `updateCircle()` で反映します。これにより、前のカメラの主点・画角が残ることや、姿勢設定の初回操作で表示範囲が急変することを防ぎます。

## 座標とシーン描画

OpenXRの眼姿勢は、基準空間における位置 `T` と向き `R` です。TEDはシーン描画のview行列にその逆変換 `R^-1 * T^-1` を使用します。

```cpp
const GgMatrix sceneView{
  window.getMo(eye) * window.getMv(eye)
};
```

controller 0／1には、左右眼位置の中点と頭部回転から作る同一の頭部中心姿勢 `T_head * R`（`GgApp::OpenXR::getHeadPoseMatrix()`）を保存します。片眼を回転中心にすると眼間距離が回転半径へ混入するため、視界固定ノードが頭部回転時に微小にずれます。頭部中心を共通の親とし、左右眼の差は各眼のview行列で与えます。

ヘッドトラッキングがOFFの場合、sceneのview行列とcontroller 0／1はともに恒等行列として扱います。ONの場合は、ローカルの視界固定ノードだけが頭部中心controllerを参照し、リモートノードはremote controllerだけを参照します。これによりローカルは視界固定、リモートは背景と同じ空間固定になります。

Scene内の合成順は次のとおりです。

```text
親変換 × controller（または remote_controller）× JSONのposition/rotation/scale
```

固定オフセットをcontrollerより前に掛けると、頭部前方へ配置した物体が頭部と一緒に回転せず、奥行き方向のずれとして現れます。詳細は [scenegraph.md](scenegraph.md) を参照してください。

HMD起動時、または「回復」操作（`R` キー、姿勢設定メニュー）の後に最初に取得した頭部中心位置を原点として保持し、以後の眼と手の平行移動から差し引きます。向きは原点設定時に打ち消さず、ランタイムが返す向きをそのままview変換へ反映します。

## ハンドトラッキング

`XR_EXT_hand_tracking` は任意機能です。拡張が利用でき、左右のhand trackerを作成できた場合だけ使用します（`hasHandTracking()`）。表示設定のハンドトラッキング設定で `OpenXR` が選ばれている場合にのみ、各フレームの予測表示時刻で関節を取得し、従来のLeap Motion用テーブルと互換な片手22姿勢へ変換して `Scene::setLocalHandAttitudes()` へ渡します。

手のひらは手首・中指・人差し指・小指の実測位置から左右共通の基底を作ります。手首骨は手のひら行列の固定回転を流用せず、手首から手のひらへの長手軸と手のひら法線から独立した基底を作ります。指骨は各骨の始点から終点への方向を `finger.obj` の長手軸へ対応させ、Leap Motionの `next_joint` と同じく骨の終点に置きます。

シーングラフの左右モデル対応はLeap Motionを基準としているため、OpenXRの `XR_HAND_LEFT_EXT`／`XR_HAND_RIGHT_EXT` は保存時に左右スロットを反転します。関節行列は、視界固定ノードと同じ頭部中心姿勢の逆変換を掛けてから共有テーブルへ保存します。

手全体がトラッキング対象外になった場合はその手の22行列をゼロ行列にして非表示にし、一部の関節が一時的に無効になった場合や基底が縮退した場合は直前の姿勢を維持して点滅を防ぎます。拡張がない、tracker作成に失敗した、または関節姿勢が無効な場合でも、HMD表示は継続します。HMDを停止したときは、OpenXRが格納した手の姿勢を消去します。

## セッション状態と終了

`begin()` の中のイベント処理は、主に次のsession stateを処理します。

* `READY`: `xrBeginSession()` を呼び、フレーム処理を開始する
* `STOPPING`: `xrEndSession()` を呼び、実行中状態を解除する
* `EXITING`／`LOSS_PENDING`: GLFWウィンドウへ終了要求を設定する

`stopHMD()`（`terminate()`）と `Window` のデストラクタは、取得中のswapchain imageを解放し、描画中のフレームがあれば `xrEndFrame()` で閉じてから、FBO／デプスバッファ、hand tracker、アクション、swapchain、参照空間、session、instanceを破棄します。その後、PC側の垂直同期を通常設定へ戻し、デスクトップ表示のために `GL_FRAMEBUFFER_SRGB` を有効に戻します。

## 制約と失敗時の動作

* OpenXR表示は左右2 viewの `PRIMARY_STEREO` を前提とします。背景のテクスチャが左右2枚なので、描画するviewは最大2個です。
* depth swapchainは提出せず、デプスバッファは各眼のOpenGL描画内だけで使用します。
* ミラー表示はviewを1個だけ（既定では左眼）PCウィンドウに表示します。
* `setDisplayMode(OPENXR)` から初期化に失敗した場合、表示モードは変更されません。
* 起動時に設定ファイルから `OPENXR` が選ばれていて `startHMD()` が失敗したときは、表示モードを `MONOCULAR` へ戻し、ビューポートを再初期化してエラー通知を表示します。
* 描画サイクル内の一時的な失敗（swapchain imageの取得失敗、`xrEndFrame()` の失敗など）は標準エラー出力へ警告し、sessionは破棄せずに次のフレームへ進みます。視点の姿勢を取得できないフレームは、レイヤーなしの空フレームを提出して描画をスキップします。
* 通常のクリーンアップでは、任意のセッション状態で呼び出せる `xrDestroySession()` を使用します。ランタイムから `STOPPING` 状態が通知された場合に限り、イベント処理で `xrEndSession()` を呼びます。

## Meta Quest 3 版 (`ted-quest` / Android) の OpenXR 実装

Quest 3 向けネイティブアプリ (`android/app/src/main/cpp/AndroidMain.cpp`) は、`android_native_app_glue` による NativeActivity と OpenXR の OpenGL ES graphics binding (`XR_KHR_opengl_es_enable`) を使用します。PC 版の `GgApp` は使わず、同じ座標の約束事に従う独自の実装です。詳しい仕様は [Quest.md](Quest.md) を参照してください。

1. **初期化**:
   - `xrInitializeLoaderKHR` で Android 用ローダーを初期化し、`XR_KHR_android_create_instance`、`XR_KHR_opengl_es_enable` のほか、利用可能なら `XR_FB_passthrough`、`XR_EXT_hand_tracking`、`XR_KHR_convert_timespec_time` を有効にします。
   - EGL コンテキストを作成した上で、`STAGE`（利用不可時は `LOCAL`）基準空間と、頭部の姿勢を求める `VIEW` 空間を作成します。
   - 各眼のカラー swapchain とデプスバッファを作成し、FBO に接続します。

2. **描画ループ**:
   - パススルー映像のレイヤーを最背面に置き、その上の projection layer に自分の手と指示者の手のモデル（PC 版と同じ OBJ ファイル）を描きます。
   - 送信する変換行列の並び（`0`／`1` が頭部中心姿勢、`2` がモデル変換行列、`3` 以降が手の関節）と、手の関節姿勢の求め方（左右スロットの反転、手のひら・手首・指骨の基底、骨の終点への配置、頭部中心姿勢を親とする表現）は、PC 版の `GgApp::OpenXR::updateOpenXRHands()` と同じです。
   - `0`／`1` の頭部中心姿勢は基準空間での値で、PC 版のようにシーン座標の原点を差し引きません。受信側はこの行列を向きの補正だけに使い、手の関節は頭部中心からの相対値なので、表示には影響しません。

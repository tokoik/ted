# TED OpenXR表示

## 概要

TEDのOpenXR対応は独立クラスではなく、GLFWウィンドウとOpenGLコンテキストを所有する `GgApp::Window` に組み込まれています。OpenXRの初期化・終了、セッション状態、左右眼のswapchain、投影行列、追跡姿勢、ミラー表示を同じウィンドウ層で管理します。

主な公開操作は次のとおりです。

* `GgApp::Window::setDisplayMode(OPENXR)`: OpenXRを開始し、成功した場合だけ表示モードを確定する
* `GgApp::Window::setDisplayMode(OPENXR以外)`: OpenXR使用中なら終了して通常表示へ戻る
* `GgApp::Window::startHMD()`／`stopHMD()`: OpenXR資源を直接初期化／破棄する低水準API
* `start()`、`select(eye)`、`commit(eye)`、`swapBuffers()`: 通常表示とOpenXR表示で共有するフレーム描画API

メニューからの切り替えには、開始・停止と設定値の更新を一括して行う `setDisplayMode()` を使用します。

## ビルド

OpenXR SDK 1.1.61は常にビルド構成へ含まれます。CMakeのConfigure時にSDKがなければ `libs/OpenXR-SDK-release-1.1.61` へ取得し、static loaderを構築して `ted` にリンクします。現在、OpenXRだけを無効化するCMakeオプションはありません。

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --target ted -- /m
```

OpenGL graphics bindingはWin32用です。実行には、現在のOpenGLコンテキストを受け入れるOpenXRランタイムとHMDが必要です。

## OpenXR表示の開始

起動時に `config.json` の `stereo` が `5`（`OPENXR`）なら `startHMD()` を呼びます。実行中は表示設定メニューの「OpenXR」を選ぶと `setDisplayMode(OPENXR)` が呼ばれます。コマンドラインの `--openxr` オプションはありません。

初期化では次の処理を行います。

1. 必須の `XR_KHR_opengl_enable` と、利用可能なら `XR_EXT_hand_tracking` を有効にする
2. instance、HMD system、Win32 OpenGL bindingを使うsessionを作成する
3. `STAGE` 基準空間を作り、利用できない場合は `LOCAL` へフォールバックする
4. `VIEW` 空間を作成する
5. `PRIMARY_STEREO` の左右2 viewについて、ランタイム推奨サイズのcolor swapchainとOpenGL FBO、深度テクスチャを作成する
6. PCウィンドウへ左右眼を表示するためのミラーFBOを作成する

color形式はランタイムの列挙結果から `GL_SRGB8_ALPHA8`、`GL_RGBA8`、`GL_RGBA16F` の順で選びます。対応形式がなければ初期化に失敗します。

## フレーム処理

アプリケーション側の描画ループは通常表示と共通です。

```cpp
if (window.start())
{
  for (int eye = 0; eye < 2; ++eye)
  {
    window.select(eye);

    rect->draw(eye, viewRotation, window.getSamples());
    scene->draw(window.getMp(eye), sceneView);

    window.commit(eye);
  }
}
window.swapBuffers();
```

OpenXR使用時の各APIの役割は次のとおりです。

### `start()`

ウィンドウのイベント処理でOpenXRイベントを処理した後、`start()` は実行中のsessionについて `xrWaitFrame()`、`xrBeginFrame()`、`xrLocateViews()` を行います。ランタイムが `shouldRender == false` を返したフレームは、レイヤーを持たない `xrEndFrame()` を送って `false` を返します。

viewの位置と向きが有効なら、左右眼ごとに次を更新します。

* `mp[eye]`: OpenXRの非対称FOVから作る投影行列
* `mo[eye]`: 眼の四元数姿勢に対する逆回転 `R^-1`
* `mv[eye]`: 起動時または回復時の原点を差し引いた逆平行移動 `T^-1`
* Sceneのcontroller 0／1: 左右眼位置の中点を回転中心とする共通の頭部中心姿勢 `T_head * R`

同じ予測表示時刻を使って、対応ランタイムでは手の関節姿勢も更新します。

### `select(eye)`

対象眼のswapchain imageを `xrAcquireSwapchainImage()` と `xrWaitSwapchainImage()` で取得し、そのimageに対応するFBOを描画先へ設定します。深度テクスチャを接続し、viewportをswapchain推奨サイズへ変更してcolor／depthを消去します。

### `commit(eye)`

ミラー表示が有効なら描画済みの眼画像をPC用ミラーFBOの左右半分へコピーします。その後、OpenGLコマンドをflushして `xrReleaseSwapchainImage()` でimageをランタイムへ返します。

### `swapBuffers()`

左右の `XrCompositionLayerProjectionView` を1個のprojection layerへまとめ、`start()` で得た予測表示時刻を使って `xrEndFrame()` します。ミラー表示が有効なら、ミラーFBOをGLFWのデフォルトframebufferへコピーし、Dear ImGuiを重ねてPCウィンドウも更新します。

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

controller 0／1には、左右眼位置の中点と頭部回転から作る同一の頭部中心姿勢 `T_head * R` を保存します。片眼を回転中心にすると眼間距離が回転半径へ混入するため、視界固定ノードが頭部回転時に微小にずれます。頭部中心を共通の親とし、左右眼の差は各眼のview行列で与えます。

ヘッドトラッキングがOFFの場合、sceneのview行列とcontroller 0／1はともに恒等行列として扱います。ONの場合は、ローカルの視界固定ノードだけが頭部中心controllerを参照し、リモートノードはremote controllerだけを参照します。これによりローカルは視界固定、リモートは背景と同じ空間固定になります。

Scene内の合成順は次のとおりです。

```text
親変換 × controller（または remote_controller）× JSONのposition/rotation/scale
```

固定オフセットをcontrollerより前に掛けると、頭部前方へ配置した物体が頭部と一緒に回転せず、奥行き方向のずれとして現れます。詳細は [scenegraph.md](scenegraph.md) を参照してください。

起動時または「回復」操作時の頭部中心位置を原点として保持し、以後の眼と手の平行移動から差し引きます。向きは原点設定時に打ち消さず、ランタイムが返す向きをそのままview変換へ反映します。

## ハンドトラッキング

`XR_EXT_hand_tracking` は任意機能です。拡張とsystemの両方が対応する場合だけ左右のhand trackerを作成します。表示設定のハンドトラッキング設定で `OpenXR` が選ばれている場合にのみ、各フレームの予測表示時刻で関節を取得し、従来のLeap Motion用テーブルと互換な片手22姿勢へ変換して `Scene::setLocalHandAttitudes()` へ渡します。

手のひらは手首・中指・人差し指・小指の実測位置から左右共通の基底を作ります。手首骨は手のひら行列の固定回転を流用せず、手首から手のひらへの長手軸と手のひら法線から独立した基底を作ります。指骨は各骨の始点から終点への方向を `finger.obj` の長手軸へ対応させます。

シーングラフの左右モデル対応はLeap Motionを基準としているため、OpenXRの `XR_HAND_LEFT_EXT`／`XR_HAND_RIGHT_EXT` は保存時に左右スロットを反転します。関節行列は、視界固定ノードと同じ頭部中心姿勢の逆変換を掛けてから共有テーブルへ保存します。

拡張がない、tracker作成に失敗した、または関節姿勢が無効な場合でも、HMD表示は継続します。

## セッション状態と終了

`pollEvents()` は主に次のsession stateを処理します。

* `READY`: `xrBeginSession()` を呼び、フレーム処理を開始する
* `STOPPING`: `xrEndSession()` を呼び、実行中状態を解除する
* `EXITING`／`LOSS_PENDING`: GLFWウィンドウへ終了要求を設定する

`stopHMD()` と `Window` のデストラクタは、取得中のswapchain image、hand tracker、FBO／texture、swapchain、参照空間、session、instanceを破棄し、PC側の垂直同期を通常設定へ戻します。

## 制約と失敗時の動作

* OpenXR表示は左右2 viewの `PRIMARY_STEREO` を前提とします。
* depth swapchainは提出せず、深度テクスチャは各眼のOpenGL描画内だけで使用します。
* `setDisplayMode(OPENXR)` から初期化に失敗した場合、表示モードは変更されません。
* 起動時に設定ファイルから `OPENXR` が選ばれている場合で `startHMD()` が失敗したときは、自動的に表示モードを `MONOCULAR` へ戻し、ビューポートを再初期化してエラー通知を表示します（不整合状態での起動フリーズを防止します）。
* 描画サイクル内の一時的な失敗（スワップチェーン画像取得失敗や、ビュー位置姿勢のトラッキング喪失など）が発生した場合は、レイヤーなしの空フレームを `xrEndFrame` に提出することで、描画を安全にスキップして動作を継続します。
* `commit()` でのスワップチェーン画像解放失敗時は、空レイヤーで `xrEndFrame` を試みた後に `stopHMD()` を呼び出します。`xrEndFrame` 自体が失敗した場合もセッションを破棄し、`MONOCULAR` 表示へフォールバックします。OpenXR資源を破棄した後は、そのフレームのミラー処理を行いません。
* 通常のクリーンアップでは、任意のセッション状態で呼び出せる `xrDestroySession()` を使用します。ランタイムから `STOPPING` 状態が通知された場合に限り、イベント処理で `xrEndSession()` を呼びます。

## Meta Quest 3 版 (`ted-quest` / Android) の OpenXR 実装

Quest 3 向けネイティブアプリ (`android/app/src/main/cpp/AndroidMain.cpp`) は、`android_native_app_glue` による NativeActivity と OpenXR GLES Graphics Binding (`XR_KHR_opengl_es_enable`) を使用して実装されています。

1. **初期化とリソース管理**:
   - `xrInitializeLoaderKHR` で Android 用ローダーを初期化し、`XR_KHR_android_create_instance` と `XR_KHR_opengl_es_enable` 拡張を有効化します。
   - EGLコンテキストと画面サーフェスを作成した上で、`XR_REFERENCE_SPACE_TYPE_STAGE`（利用不可時は `LOCAL`）基準空間を作成します。
   - 各眼のカラー用 Swapchain 画像のほか、24-bit 深度レンダーバッファ（`GL_DEPTH_COMPONENT24`）を作成し FBO に接続します。
   - 初期化の途中で失敗した場合（`xrCreateInstance` や `xrCreateSwapchain` など）、`terminateOpenXR()` を呼び出して確保済みリソースを完全にロールバックします。

2. **描画ループ**:
   - `ALooper_pollOnce()` で Android イベントと `pollOpenXREvents()` を駆動します。
   - `xrWaitFrame()` / `xrBeginFrame()` を行い、`xrLocateViews()` の戻り値を確認した上で左右眼のカラー／深度 FBO へ描画（左眼: シアン、右眼: マゼンタの単色テスト描画）を実行します。
   - 描画完了後、`xrEndFrame()` に `XrCompositionLayerProjection` を提出します。

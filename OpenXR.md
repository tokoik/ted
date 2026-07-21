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
* Sceneのcontroller 0／1: 眼の完全な姿勢 `T * R`

同じ予測表示時刻を使って、対応ランタイムでは手の関節姿勢も更新します。

### `select(eye)`

対象眼のswapchain imageを `xrAcquireSwapchainImage()` と `xrWaitSwapchainImage()` で取得し、そのimageに対応するFBOを描画先へ設定します。深度テクスチャを接続し、viewportをswapchain推奨サイズへ変更してcolor／depthを消去します。

### `commit(eye)`

ミラー表示が有効なら描画済みの眼画像をPC用ミラーFBOの左右半分へコピーします。その後、OpenGLコマンドをflushして `xrReleaseSwapchainImage()` でimageをランタイムへ返します。

### `swapBuffers()`

左右の `XrCompositionLayerProjectionView` を1個のprojection layerへまとめ、`start()` で得た予測表示時刻を使って `xrEndFrame()` します。ミラー表示が有効なら、ミラーFBOをGLFWのデフォルトframebufferへコピーし、Dear ImGuiを重ねてPCウィンドウも更新します。

## 座標とシーン描画

OpenXRの眼姿勢は、基準空間における位置 `T` と向き `R` です。TEDはシーン描画のview行列にその逆変換 `R^-1 * T^-1` を使用します。

```cpp
const GgMatrix sceneView{
  window.getMo(eye) * window.getMv(eye)
};
```

controller 0／1には逆変換ではなく `T * R` を保存します。その眼に追従するシーンノードではview行列と相殺されるため、頭部の回転と移動に追従しつつ左右眼の相対差を保てます。詳細は [scenegraph.md](scenegraph.md) を参照してください。

起動時または「回復」操作時の頭部中心位置を原点として保持し、以後の眼と手の平行移動から差し引きます。向きは原点設定時に打ち消さず、ランタイムが返す向きをそのままview変換へ反映します。

## ハンドトラッキング

`XR_EXT_hand_tracking` は任意機能です。拡張とsystemの両方が対応する場合だけ左右 of hand tracker を作成します。表示設定のハンドトラッキング設定で `OpenXR` が選ばれている場合にのみ、各フレームの予測表示時刻で関節を取得し、従来のLeap Motion用テーブルと互換な22姿勢へ変換して `Scene::setLocalHandAttitudes()` へ渡します。

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
* 起動時に設定ファイルから `OPENXR` が選ばれている場合は `startHMD()` を直接呼ぶため、失敗時に設定値を通常表示へ書き戻す処理はありません。
* OpenXRの一部API失敗は通知を表示します。session lossやランタイムの終了要求では、ウィンドウを閉じる方向へ遷移します。

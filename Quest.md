# Quest 3 版 (ted-quest) と中継サーバ (ted-server)

Quest 3 版は、Meta Quest 3 を作業者（WORKER）として動かす実験用のアプリケーションです。一般には配布しません。

## 機能

* パススルー映像（`XR_FB_passthrough`）を背景に、ハンドトラッキング（`XR_EXT_hand_tracking`）による自分の手のモデル（`handr.obj` / `handl.obj` / `finger.obj`）を表示します。
* パススルーカメラ（Passthrough Camera API）の左右の映像を、ハードウェアエンコーダで H.264 / HEVC の動画に符号化し（または 1 枚ずつ JPEG に符号化し）、撮影時の頭部姿勢と手の関節姿勢とともに、中継サーバまたは指示者 PC へ送信します。
* 中継サーバまたは指示者 PC から受け取った指示者の手の関節姿勢から、指示者の手のモデル（`handr_remote.obj` / `handl_remote.obj` / `finger.obj`）を重畳表示します。

## 必要な環境

* Meta Quest 3 / 3S、Horizon OS v74 以降（Passthrough Camera API）
* ヘッドセットの設定でパススルーとハンドトラッキングを有効にしておくこと
* Android Studio（Android SDK 34、NDK 27.0.12077973、CMake 3.22.1 以降）

## ビルドとインストール

`android/` を Android Studio で開いてビルドします。コマンドラインでは次のとおりです。

```powershell
cd android
.\gradlew assembleDebug
adb install -r app\build\outputs\apk\debug\app-debug.apk
```

手のモデル（`handr.obj` など 10 ファイル）は、ビルド時にリポジトリのトップから APK の assets へ自動的に取り込まれます。

初回起動時に、ヘッドセット内でカメラの使用許可（`android.permission.CAMERA`、`horizonos.permission.HEADSET_CAMERA`）を求められます。許可するとカメラ画像の送信が始まります。

## 設定ファイル (ted_quest.json)

初回起動時に、既定値の設定ファイルが次の場所に作られます。`adb pull` で取り出して編集し、`adb push` で戻してからアプリを再起動します。

```powershell
adb pull /sdcard/Android/data/com.example.ted_openxr/files/ted_quest.json
adb push ted_quest.json /sdcard/Android/data/com.example.ted_openxr/files/ted_quest.json
```

| キー名 | 型 | 既定値 | 説明 |
| :--- | :--- | :--- | :--- |
| `host` | 文字列 | `"192.168.0.7"` | 通信相手（中継サーバまたは指示者 PC）の IP アドレス |
| `port` | 数値 | `12345` | ポート番号。`port` へ送信し、`port + 1` で受信する |
| `send_images` | 真偽値 | `true` | パススルーカメラの画像を送信するか |
| `camera_width` / `camera_height` | 数値 | `1280` / `960` | パススルーカメラから取得する画像の大きさ（使えない大きさなら、使える最大のものを選ぶ） |
| `codec` | 文字列 | `"h264"` | 画像の送り方。`"h264"` / `"hevc"` はハードウェアエンコーダで動画に符号化し、`"jpeg"` は 1 枚ずつ JPEG に符号化する |
| `bitrate` | 数値 | `6000000` | 動画の片眼あたりのビットレート（bps） |
| `keyframe_interval` | 数値 | `2` | 動画のキーフレームの間隔（秒）。欠落からの復帰は、指示者 PC からのキーフレーム要求でも行う |
| `transmit_quality` | 数値 | `50` | JPEG の品質（0～100）。`codec` が `"jpeg"` のときだけ使う |
| `transmit_fps` | 数値 | `30` | 画像を符号化・送信するフレームレートの上限（`0` なら制限しない） |
| `send_interval` | 数値 | `10` | 姿勢を送信する間隔（ミリ秒） |
| `passthrough` | 真偽値 | `true` | パススルー映像を背景に表示するか |
| `show_local_hands` | 真偽値 | `true` | 自分の手のモデルを表示するか |
| `show_remote_hands` | 真偽値 | `true` | 指示者の手のモデルを表示するか |
| `remote_hand_position` | 配列 | `[0, 0, 0]` | 指示者の手を重畳するときに、頭部座標系で加える平行移動（メートル） |

1 フレームは PC 版の受信バッファ（1 MB）に収まる必要があります。画像が 1 MB を超える場合、画像は送らず姿勢だけを送ります（logcat に警告が出ます）。

### 動画で送る場合

* パススルーカメラの出力先をハードウェアエンコーダ（`AMediaCodec`）の入力 Surface にするので、画素を CPU で扱いません。一定ビットレート、B フレームなし、リアルタイム優先に設定し、キーフレームには SPS / PPS（HEVC では VPS も）を必ず付けます。
* 左右は別々のエンコーダで符号化し、アクセスユニットごとに、撮影時の姿勢と一緒に 1 フレームとして送ります。動画は途中が欠けると復号できないので、JPEG と違って新しいもので置き換えず、すべて順番に送ります。
* 指示者 PC は Media Foundation の同期型デコーダで復号します。アクセスユニットの通し番号が飛んだら（UDP で欠けたら）、次のキーフレームまで復号せず、Quest 3 版へキーフレームを要求します。Quest 3 版は要求が続いても 300 ms に 1 回だけキーフレームを作ります。
* 目安のデータ量は、1280 × 960、30 fps、片眼 6 Mbps のとき合計約 12 Mbps です（JPEG 品質 50 では約 50 Mbps）。画質が足りなければ `bitrate` を上げ、回線が細ければ下げてください。
* HEVC を使う場合、指示者 PC に HEVC のデコーダ（「HEVC ビデオ拡張機能」など）が必要です。見つからなければ、指示者の TED に通知が出ます。

## 接続構成

### 中継サーバを使う場合

```text
Quest 3 (WORKER) <-> ted-server <-> 指示者 PC の TED (OPERATOR)
```

```text
ted-server <questPort> <questAddress> <instructorPort> <instructorAddress>
```

| 機器 | 設定 |
| :--- | :--- |
| Quest 3 (`ted_quest.json`) | `host` = 中継サーバの IP、`port` = `questPort` |
| 中継サーバ | `questAddress` = Quest 3 の IP、`instructorAddress` = 指示者 PC の IP |
| 指示者 PC (`config.json`) | `input_mode` = リモート、`host` = 中継サーバの IP、`port` = `instructorPort` |

中継サーバは `questPort` と `instructorPort + 1` で受信し、Quest 3 は `questPort + 1`、指示者 PC は `instructorPort` で受信します。中継サーバと指示者の TED を同じ PC で動かす場合は、ポート番号が重ならないようにします（例: `questPort` = 12345、`instructorPort` = 12347、IP アドレスは `127.0.0.1`）。

### 中継サーバを使わない場合

Quest 3 の `host` に指示者 PC の IP、指示者 PC の `host` に Quest 3 の IP を指定し、`port` を同じ値にします。

### 起動順序

指示者 PC の TED は、リモート入力を開始するときに、左画像を含むフレームを最大約 15 秒（0.5 秒 × 30 回）待ちます。先に Quest 3 版（と中継サーバ）を起動し、画像の送信が始まってから（ヘッドセット内でカメラの使用を許可してから）指示者の TED を起動してください。

EOF（長さ 0 のデータグラム）は相手の停止通知ですが、PC 版の `CamRemote` と `Worker`、Quest 3 版はいずれもこれを無通信として扱い、受信を続けます。そのため、どちらかを再起動しても、もう一方を再起動せずに通信を再開できます（受信側は 2 秒以上の無通信でフレーム番号を再同期します）。Quest 3 版は終了時に EOF を送りません。

## 指示者 PC 側の推奨設定

Quest 3 版は、起動時に各カメラの内部パラメータから求めた画角を logcat に出力します。

```powershell
adb logcat -s TED
```

```text
camera 50: PC config.json -> "remote_fov_x": 0.9xxx, "remote_fov_y": 0.8xxx (principal point offset ...)
```

指示者 PC の `config.json` では、次のように設定します。

* `remote_fov_x` / `remote_fov_y`: 上の logcat に出た値（カメラの水平・垂直方向の半画角、ラジアン）
* `remote_texture_width` / `remote_texture_height`: 送信する画像と同じ大きさ（既定では `1280` / `960`）
* 表示用のシェーダ（`vertex_shader` / `fragment_shader`）は、これまでのリモート入力と同じ設定を使います。Quest 3 版の画像は、`remote_fov_x` / `remote_fov_y` の画角を持つ透視投影の画像として `CamRemote` が展開します。

主点のずれ（principal point offset）は、現在の `CamRemote` では補正されません。

## 送受信するデータ

PC 版と同じ形式です。1 フレームは次のとおりです。

```text
unsigned int head[3] = { 左画像のバイト数, 右画像のバイト数, 変換行列の数と画像の形式 }
float matrix[変換行列の数][16]   // 列優先 (gg::GgMatrix と同じ)
左画像, 右画像
```

`head[2]` の構成は次のとおりです（PC 版の `Network.h`、Quest 3 版の `TedProtocol.h`）。

| ビット | 内容 |
| :--- | :--- |
| 0～15 | 変換行列の数 |
| 16 | キーフレーム要求（動画を受信する指示者 PC が送信側へ送る） |
| 24～27 | 画像の形式（`0`: JPEG、`1`: H.264、`2`: HEVC） |

動画の場合、左右の画像はそれぞれ 1 アクセスユニット（Annex B）で、先頭に 8 バイトの `VideoUnitHeader`（視点ごとの通し番号、キーフレームのフラグ）が付きます。左右のどちらか一方だけのフレームもあります。

Quest 3 版が送る変換行列（47 個）は、PC 版の `Scene` の共有メモリと同じ並びです。

| 番号 | 内容 |
| :--- | :--- |
| `0`, `1` | 送信する画像を撮影したときの頭部中心姿勢（`XR_KHR_convert_timespec_time` で撮影時刻の姿勢を求める。使えなければ現在の姿勢） |
| `2` | モデル変換行列（単位行列） |
| `3 + joint * 2 + hand` | 手の関節姿勢（`hand` は 0 が右手、1 が左手。`joint` は手のひら、手首、各指 4 本の骨の順の 22 個）。`0`, `1` の頭部座標系で表し、トラッキングしていない手は零行列 |

関節姿勢の求め方は、PC 版の `GgApp::OpenXR::updateOpenXRHands()` と同じです。受信した指示者の変換行列は、Quest 3 の現在の頭部中心姿勢 ×（`remote_hand_position` の平行移動）× 指示者の `2` 番 × 各関節、の順に合成して描画します。これは PC 版の `hand.json` と同じ合成順です。

UDP による分割と再構成は、中継サーバと同じ `server/Network.h/.cpp` を Android（POSIX ソケット）向けにそのままビルドして使っています。

## 未確認事項

* 実機（Quest 3）での動作は未確認です。コードは NDK 27 のヘッダとライブラリでコンパイルとリンク（OpenXR ローダを除く）を確認しています。
* パススルーカメラの左右 2 台を同時に開けない場合は、左だけの単眼画像を送ります（PC 版は右画像が無いとき左画像を右にも使います）。
* パススルーカメラの出力をエンコーダの Surface へ直接つなぐ方法と、動画の実際のビットレート・遅延は、実機で確認していません。PC 側の Media Foundation による復号も、実際の Quest 3 の出力では確認していません。うまくいかない場合は `codec` を `"jpeg"` にすると従来の方式に戻ります。
* パススルーカメラのベンダータグ（`com.meta.extra_metadata.position`）が読めない場合は、カメラ ID `50`（左）と `51`（右）を使います。

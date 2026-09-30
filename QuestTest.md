# Quest 3 版 実機テスト手順書

作成日: 2026-09-29

Quest 3 版 (ted-quest) を実機にインストールし、単体動作、指示者 PC との直結、中継サーバ経由、動画伝送の順に確認します。JPEG 方式は `ted-openxr` ブランチ、H.264 / HEVC 方式は `ted-openxr-video` ブランチ (worktree `worktrees\ted-openxr-video`) でビルドします。

## テストの流れ

前の段階が通ってから次へ進むと、問題の切り分けが簡単になります。

| 段階 | 目的 | ブランチ | codec |
| --- | --- | --- | --- |
| テスト 1 単体動作 | パススルー、自分の手のモデル、カメラの起動を確認 | ted-openxr | jpeg |
| テスト 2 PC 直結 | Quest 3 → PC の映像と手、PC → Quest 3 の手を確認 | ted-openxr | jpeg |
| テスト 3 中継サーバ経由 | ted-server を挟んでテスト 2 と同じことができるか | ted-openxr | jpeg |
| テスト 4 動画 | H.264 で送り、帯域・遅延・欠落からの復帰を確認 | ted-openxr-video | h264 |
| テスト 5 再起動 | 片方だけ再起動して通信が戻るか | 両方 | jpeg / h264 |

`ted-openxr-video` ブランチでも `codec` を `"jpeg"` にすればテスト 1〜3 を行えます。ただし PC 側の受信処理は両ブランチで違うので、問題が出たら `ted-openxr` でも試して比べてください。**Quest 3 版、中継サーバ、指示者の TED は、必ず同じブランチからビルドしたものを組み合わせます**（通信形式が 2026-09-29 以前の版と互換ではないため）。

## 準備するもの

| 区分 | 内容 |
| --- | --- |
| ヘッドセット | Meta Quest 3 または 3S。Horizon OS v74 以降（Passthrough Camera API に必要）。充電済み |
| ケーブル | データ通信できる USB-C ケーブル（充電専用は不可） |
| Meta アカウント | ヘッドセットの持ち主のアカウントと、開発者組織（開発者モードに必要） |
| スマートフォン | Meta Horizon アプリ（開発者モードの切り替えに使う） |
| ビルド用 PC | Android Studio、Android SDK 34、NDK 27.0.12077973、CMake 3.22.1 以降、platform-tools (adb) |
| 指示者 PC | テストするブランチからビルドした ted.exe（中継サーバを使うなら ted_server.exe も）。HEVC をテストする場合は Microsoft Store の「HEVC ビデオ拡張機能」が必要（H.264 は Windows 標準で対応） |
| ネットワーク | Quest 3 と PC が同じ LAN にいること。Wi-Fi は 5 GHz 帯以上を推奨。ゲスト用 Wi-Fi など端末間通信が遮断されるネットワークは不可 |

adb は通常 `%LOCALAPPDATA%\Android\Sdk\platform-tools` にあります。このフォルダを PATH に加えるか、以降のコマンドをそのフォルダで実行してください。

## Quest 3 の準備

初回だけ行います。メニューの名前は Horizon OS の版によって少し違うことがあります。

1. OS の版を確認する。ヘッドセットの「設定」→「一般」→「情報」で v74 以降であることを確かめ、古ければ更新する。
2. 開発者組織を作る。[Meta Horizon 開発者サイト](https://developers.meta.com/horizon/) にログインして組織を作成する（既にあれば不要）。
3. 開発者モードをオンにする。スマートフォンの Meta Horizon アプリで「デバイス」→ 対象の Quest 3 →「ヘッドセットの設定」→「開発者モード」をオンにし、ヘッドセットを再起動する。
4. USB で接続する。ヘッドセットをかぶった状態で PC とつなぎ、「USB デバッグを許可しますか」で「このコンピューターでは常に許可」を選んで許可する。
5. 接続を確かめる。PC で `adb devices` を実行し、シリアル番号の右が `device` になっていればよい（`unauthorized` なら手順 4 をやり直す）。
6. ハンドトラッキングをオンにする。「設定」→「動作とトラッキング」のハンドトラッキングをオンにし、コントローラーを置いて手に切り替わることを確かめる。
7. パススルーを使える状態にする。ガーディアン（境界）を設定し、ホームでパススルーが表示できることを確かめる。

Wi-Fi で adb を使いたい場合は、USB 接続中に `adb tcpip 5555` を実行してからケーブルを外し、`adb connect <Quest 3 の IP>:5555` で接続できます。ログを見ながらうろうろ歩くテストで便利です。

## APK のビルド

テストするブランチの worktree にある `android` フォルダをビルドします（テスト 1〜3 は `worktrees\ted-openxr\android`、テスト 4 は `worktrees\ted-openxr-video\android`）。

### Android Studio でビルドする

1. SDK Manager の「SDK Platforms」で Android 14 (API 34) を、「SDK Tools」で「NDK (Side by side)」27.0.12077973 と「CMake」3.22.1 以降を入れる（「Show Package Details」にチェックすると版を選べる）。
2. 「Open」で `android` フォルダを開き、Gradle の同期が終わるのを待つ。
3. 「Build」→「Make Project」でビルドする。初回は CMake が OpenXR SDK を worktree の `libs` にダウンロードするので、ネットワーク接続が必要。
4. Quest 3 を USB でつないでいれば、ツールバーのデバイス欄に Quest 3 を選び、「Run」（▶）でインストールと起動までまとめて行える。

### コマンドラインでビルドする

PowerShell で次のように実行します。Java は Android Studio に付属するものを使います。

```powershell
$env:JAVA_HOME = "C:\Program Files\Android\Android Studio\jbr"
cd D:\Users\tokoi\Documents\Projects\worktrees\ted-openxr\android
.\gradlew assembleDebug
```

できた APK は `app\build\outputs\apk\debug\app-debug.apk` です。手のモデル（`handr.obj` など 10 ファイル）はビルド時にリポジトリのトップから自動で取り込まれます。ビルドが失敗したら、最初のエラー行を控えてください（この版は Gradle でのビルドをまだ確認していません）。

## インストールと初回起動

1. インストールする。`adb install -r app\build\outputs\apk\debug\app-debug.apk` を実行し、`Success` と出ればよい。署名の違う古い版が入っていて `INSTALL_FAILED_UPDATE_INCOMPATIBLE` になったら、`adb uninstall com.example.ted_openxr` してから入れ直す。
2. ログを見る準備をする。別の PowerShell で `adb logcat -c` のあと `adb logcat -s TED` を実行しておく（保存するなら `adb logcat -s TED > quest_log.txt`）。
3. 起動する。ヘッドセットの「ライブラリ」で「提供元不明」（Unknown Sources）に絞り込むと「TED OpenXR Native」がある。PC からは `adb shell am start -n com.example.ted_openxr/android.app.NativeActivity` でも起動できる。
4. カメラの使用を許可する。初回はヘッドセット内に許可の確認が出るので、すべて許可する。許可すると 3 秒以内にカメラが開く（アプリの再起動は不要）。
5. 終了する。ヘッドセットでアプリを閉じるか、`adb shell am force-stop com.example.ted_openxr` を実行する。

許可の確認が出ない、または誤って拒否したときは、PC から許可を与えられます。

```powershell
adb shell pm grant com.example.ted_openxr android.permission.CAMERA
adb shell pm grant com.example.ted_openxr horizonos.permission.HEADSET_CAMERA
```

起動直後の logcat に次の行が出ていれば、各機能が動いています。

| ログ | 意味 |
| --- | --- |
| `settings loaded from ...` / `default settings written to ...` | 設定ファイルを読んだ / 既定値で作った |
| `TedLink: WORKER send to <IP>:<port>, receive on port <port+1>` | 通信を開始した |
| `Enabled OpenXR extension: XR_EXT_hand_tracking` | ハンドトラッキングが使える |
| `Passthrough enabled` | パススルーを背景に表示している |
| `camera 50: facing=..., meta position=0` | パススルーカメラを見つけた |
| `camera 50: PC config.json -> "remote_fov_x": ..., "remote_fov_y": ...` | PC に設定する画角（控えておく） |
| `passthrough camera: 2 camera(s), 1280x960, JPEG` | 左右のカメラで取得を始めた（動画なら `H.264`） |
| `video/avc encoder: 1280x960, ...` | 動画のエンコーダが始まった（テスト 4 のみ） |
| `instructor data receiving` / `lost` | 指示者の姿勢を受信し始めた / 途切れた |

## 設定ファイル ted_quest.json

初回起動で既定値のファイルが `/sdcard/Android/data/com.example.ted_openxr/files/ted_quest.json` に作られます。設定は起動時にだけ読むので、書き換えたらアプリを起動し直します。

```powershell
adb pull /sdcard/Android/data/com.example.ted_openxr/files/ted_quest.json
# ted_quest.json を編集する
adb push ted_quest.json /sdcard/Android/data/com.example.ted_openxr/files/ted_quest.json
adb shell am force-stop com.example.ted_openxr
adb shell am start -n com.example.ted_openxr/android.app.NativeActivity
```

テスト 2（PC 直結、JPEG）の設定例です。`host` は指示者 PC の IP アドレスにします。

```json
{
  "host": "192.168.0.7",
  "port": 12345,
  "send_images": true,
  "camera_width": 1280,
  "camera_height": 960,
  "codec": "jpeg",
  "bitrate": 6000000,
  "keyframe_interval": 2,
  "transmit_quality": 50,
  "transmit_fps": 30,
  "send_interval": 10,
  "passthrough": true,
  "show_local_hands": true,
  "show_remote_hands": true,
  "remote_hand_position": [ 0, 0, 0 ]
}
```

- `ted-openxr` ブランチの版には `codec`、`bitrate`、`keyframe_interval` がありません（書いても無視され、常に JPEG）。
- `ted-openxr-video` ブランチの版の既定値は `"codec": "h264"` です。テスト 1〜3 をこの版で行うなら `"jpeg"` にします。
- 項目の説明は [Quest.md](Quest.md) にあります。

## PC 側の準備

1. IP アドレスを調べる。PC は `ipconfig` の IPv4 アドレス、Quest 3 はヘッドセットの Wi-Fi の詳細か `adb shell ip -4 addr show wlan0` で分かる。実験中に変わらないよう、ルーターで固定しておくとよい。
2. ネットワークを「プライベート」にする。Windows の「設定」→「ネットワークとインターネット」で、接続中のネットワークの種類をプライベートにする。
3. ファイアウォールで UDP を許可する。ted.exe、ted_server.exe を初めて起動したときの確認で許可するか、管理者の PowerShell でポートを開ける（下の例は 12345〜12348）。
4. 指示者の config.json を設定する（下の表）。起動後に入力設定画面で「リモート」を選び、アドレスとポートを入れて「設定」を押しても同じ。

```powershell
netsh advfirewall firewall add rule name="TED UDP" dir=in action=allow protocol=UDP localport=12345-12348
```

| キー | 設定値 | 説明 |
| --- | --- | --- |
| `input_mode` | `4` | リモート（指示者として受信） |
| `host` | Quest 3 の IP（直結）/ 中継サーバの IP | 送信元の確認と姿勢の送信先 |
| `port` | `12345`（直結）/ `instructorPort`（中継） | `port` で受信し `port + 1` へ送信 |
| `remote_fov_x` / `remote_fov_y` | logcat に出た値 | Quest 3 のカメラの半画角（ラジアン） |
| `remote_texture_width` / `remote_texture_height` | `1280` / `960` | 送られてくる画像と同じ大きさ |
| `hand_tracking` | `2`（OpenXR）または `1`（Leap Motion） | 指示者の手を Quest 3 へ送るのに必要 |
| `scene` | `telexistence.json` など | `hand_remote.json` を含むシーンで、Quest 3 の手が PC に表示される |

表示用のシェーダ (`vertex_shader` / `fragment_shader`) は、これまでリモート入力で使っている設定のままにします。指示者の TED は起動時に Quest 3 の画像を最大約 15 秒待ち、届かなければ「作業者側のデータを受け取れません」で止まるので、必ず Quest 3 版を先に起動します。

## テスト手順

### テスト 1 単体動作

1. `ted-openxr` の APK をインストールし、logcat を表示したまま起動する。
2. 背景にパススルーの周囲が見えることを確かめる。
3. 両手を目の前に出し、青い手のひらと指のモデルが実際の手に重なること、左右が逆でないことを確かめる。手を下げるとモデルが消える。
4. カメラを許可したあと、logcat に `passthrough camera: 2 camera(s)` と `remote_fov_x` の行が出ることを確かめ、`remote_fov_x` / `remote_fov_y` の値を記録する。

### テスト 2 PC と直結 (JPEG)

1. `ted_quest.json` の `host` を指示者 PC の IP、`port` を `12345` にして Quest 3 版を起動する。
2. 指示者の config.json の `host` を Quest 3 の IP、`port` を `12345`、`remote_fov_x` / `remote_fov_y` をテスト 1 の値にして、ted.exe を起動する。
3. PC に Quest 3 のカメラ映像が表示され、Quest 3 を向けた方向に追従することを確かめる。左右の目の画像が逆なら `swap_camera_eyes` を試す。
4. Quest 3 で手を動かし、PC の画面に赤い手のモデル（`hand_remote.json`）が動くことを確かめる。
5. PC 側で手を動かし（OpenXR または Leap Motion）、Quest 3 に赤い指示者の手のモデルが重なること、logcat に `instructor data receiving` が出ることを確かめる。位置がずれるなら `remote_hand_position` で調整する。
6. タスクマネージャーの「パフォーマンス」で、PC の Wi-Fi / Ethernet の受信速度を記録する（テスト 4 と比べる）。

### テスト 3 中継サーバ経由

中継サーバを指示者と同じ PC で動かす例です（`questPort` = 12345、`instructorPort` = 12347）。

1. PC で `ted_server.exe 12345 <Quest 3 の IP> 12347 127.0.0.1` を起動し、`ted-server: relaying ...` と表示されることを確かめる。
2. `ted_quest.json` の `host` を PC の IP、`port` を `12345` にして Quest 3 版を起動する。
3. 指示者の config.json の `host` を `127.0.0.1`、`port` を `12347` にして ted.exe を起動する。
4. テスト 2 の 3〜5 と同じことを確かめる。

### テスト 4 動画 (H.264)

Quest 3 版、ted.exe、ted_server.exe をすべて `ted-openxr-video` からビルドしたものに替えます。

1. `ted_quest.json` の `codec` を `"h264"`、`bitrate` を `6000000` にして起動し、logcat に `video/avc encoder: 1280x960, 6000000 bps` と `passthrough camera: 2 camera(s), 1280x960, H.264` が出ることを確かめる。
2. 指示者の ted.exe を起動し、映像が表示されることを確かめる（最初のキーフレームが届くまでに最大数百ミリ秒かかる）。
3. 受信速度を記録し、テスト 2 の JPEG と比べる（目安は合計約 12 Mbps）。
4. 遅延を測る。ストップウォッチを表示した画面を Quest 3 で見、その画面と指示者 PC の表示を同じ写真に撮って、時刻の差を読む。JPEG（テスト 2）でも同じ方法で測って比べる。
5. 欠落からの復帰を確かめる。ルーターから離れるなどして電波を弱め、映像が一瞬止まっても、ブロックノイズが残らずに数百ミリ秒で戻ることを確かめる。
6. `bitrate` を `3000000`、`10000000` に変えて画質と帯域を比べる。HEVC を試すなら `codec` を `"hevc"` にする（指示者 PC に HEVC デコーダが必要）。

### テスト 5 再起動

1. 指示者の ted.exe を動かしたまま Quest 3 版を終了して起動し直し、PC の映像が数秒で戻ることを確かめる。
2. Quest 3 版を動かしたまま ted.exe を終了して起動し直し、映像と指示者の手が戻ること、logcat が `lost` のあと `receiving` になることを確かめる。
3. ヘッドセットを外してスリープさせ、かぶり直して映像が戻ることを確かめる（カメラが切れたら 3 秒ごとに開き直す）。

## 結果記録表

「結果」に 未実施 / OK / NG / 保留 を記入し、測った値や気づいたことを「メモ」に書きます。NG の項目は、そのときの logcat を保存しておくと原因を調べやすくなります。

| テスト | 確認項目 | 期待される結果 | 結果 | メモ |
| --- | --- | --- | --- | --- |
| 1 | パススルー | 背景に周囲が見える | 未実施 | |
| 1 | 自分の手のモデル | 実際の手に重なり、左右が正しい | 未実施 | |
| 1 | パススルーカメラ | 2 台とも開き、画角が logcat に出る | 未実施 | remote_fov_x / y = |
| 2 | PC への映像 | 表示され、頭の向きに追従する | 未実施 | |
| 2 | PC での Quest 3 の手 | 赤い手のモデルが動く | 未実施 | |
| 2 | Quest 3 での指示者の手 | 赤い手のモデルが重なる | 未実施 | |
| 2 | 帯域 (JPEG) | 記録する | 未実施 | Mbps |
| 3 | 中継サーバ経由 | テスト 2 と同じように動く | 未実施 | |
| 4 | エンコーダの開始 | logcat に `video/avc encoder` が出る | 未実施 | |
| 4 | PC への映像 (H.264) | 表示される | 未実施 | |
| 4 | 帯域 (H.264) | 合計約 12 Mbps（片眼 6 Mbps） | 未実施 | Mbps |
| 4 | 遅延 | JPEG と同等以下 | 未実施 | JPEG: ms / H.264: ms |
| 4 | 欠落からの復帰 | ブロックノイズが残らず数百ミリ秒で戻る | 未実施 | |
| 5 | Quest 3 版の再起動 | PC の映像が数秒で戻る | 未実施 | |
| 5 | ted.exe の再起動 | 映像と指示者の手が戻る | 未実施 | |
| 5 | ヘッドセットのスリープ | かぶり直すと映像が戻る | 未実施 | |

## トラブルシューティング

まず `adb logcat -s TED` のエラー行を見ます。アプリが落ちる場合は、絞り込まない `adb logcat` で `FATAL` や `ted_android` を含む行を探します。

| 症状 | 考えられる原因 | 対処 |
| --- | --- | --- |
| `adb devices` が空、または `unauthorized` | USB デバッグを許可していない、充電専用ケーブル | ヘッドセットをかぶって許可し直す。ケーブルを替える |
| ライブラリにアプリがない | 提供元不明のアプリは別の一覧 | 「提供元不明」で絞り込む。`adb shell am start` でも起動できる |
| 起動してすぐ終わる、何も表示されない | OpenXR の初期化に失敗 | logcat の `xrCreateInstance failed` などを確認。Quest Link 中は切る |
| 背景が黒い | `Passthrough disabled`（パススルーが使えない） | ヘッドセットでパススルーが使えるか確認。`passthrough` が `true` か確認 |
| 手のモデルが出ない | ハンドトラッキングがオフ、コントローラーを持っている | 設定でオンにし、コントローラーを置く。`Hand tracking is not supported` の有無を確認 |
| `passthrough camera is not available; retrying` が続く | カメラの許可がない、OS が v74 より古い | `adb shell pm grant` で許可を与える。OS を更新する |
| `camera 50: ACameraDevice_createCaptureSession failed` など（テスト 4） | カメラの出力をエンコーダに直接出せない | `codec` を `"jpeg"` に戻してテストを続け、そのログを保存する |
| PC が「作業者側のデータを受け取れません」で止まる | 起動順、IP やポートの誤り、ファイアウォール、ブランチの違うビルドの組み合わせ | Quest 3 版を先に起動。`host` / `port` を見直す。全部同じブランチでビルドし直す |
| PC の映像が歪む、大きさが合わない | `remote_fov_x` / `remote_fov_y` が合っていない | logcat に出た値を設定する。`remote_texture_width` / `height` を 1280 / 960 にする |
| Quest 3 に指示者の手が出ない | PC の `hand_tracking` が 0、姿勢が届いていない | PC のハンドトラッキングを有効にする。logcat が `instructor data receiving` か確認 |
| `image data (... bytes) exceeds the frame limit` | 画像が 1 MB を超えた | `transmit_quality` または `bitrate` を下げる |
| 「受信した動画を復号するデコーダが見つかりません」 | 指示者 PC にその形式のデコーダがない（多くは HEVC） | `codec` を `"h264"` にするか、HEVC ビデオ拡張機能を入れる |
| 動画が固まる、ノイズが残る | パケットの欠落が多い | 5 GHz 帯にする。ルーターに近づく。`bitrate` を下げる |
| 遅延が大きい | 帯域が足りない、Wi-Fi の混雑 | `bitrate` や `transmit_fps` を下げる。PC を有線接続にする |

## 参考資料

- [Quest.md](Quest.md)（設定項目、接続構成、送受信するデータ）と [config.md](config.md)（PC 版の設定）
- [Passthrough Camera API（Meta Horizon OS Developers）](https://developers.meta.com/horizon/documentation/native/android/pca-native-overview/): 対応機種、OS の版、解像度、必要な許可
- [Android Native Camera2 API（Meta Horizon OS Developers）](https://developers.meta.com/horizon/documentation/native/android/pca-native-documentation/): カメラの位置を表すベンダータグとマニフェスト

# 作業依頼履歴

この文書は、プログラムの改善に関する依頼と、その依頼に基づいて実施した変更を記録します。

## 2026-07-21: `Menu.cpp` の構造点検

### 依頼

度重なる修正とクラスのスコープ制限によって複雑化した `Menu.cpp` を点検し、問題と改善方法を提案する。

### 点検結果

- Native File Dialog が返すパスの解放漏れ
- 設定再読み込み中に旧描画資源と新設定が混在する可能性
- UI 内に OpenXR、Leap Motion、投影更新などの副作用が分散
- 必須の `GgApp` を nullable なポインタとして保持
- `Window` と `Attitude` の状態を UI が直接編集
- カメラ能力列挙で Media Foundation 固有型が UI 層へ露出
- GUID からコーデック名への変換処理の重複
- 起動時設定保存、文字列入力バッファ、未使用変数などの整理候補

## 2026-07-21: 指摘 1 — ファイルパスの解放

- `getFilePath()` が取得したパスを `std::string` へコピーした後、`NFD_FreePath()` で解放するよう修正した。

## 2026-07-21: 指摘 2 — 設定再読み込みのトランザクション化

- 読み込んだ設定を `defaults` へ即時反映せず、`Menu` が候補設定として保持するよう変更した。
- `Rect` と `Scene` を候補設定から構築し、成功した場合だけ候補設定を確定するよう変更した。
- `Scene` の構築時に、設定ファイルのパスとシーングラフの深さ上限を候補設定から明示的に渡すよう変更した。
- 構築失敗時は候補設定を破棄し、実行中の設定を維持する。

## 2026-07-21: 指摘 3 — 副作用を操作 API へ集約

- 表示モード変更を `GgApp::Window::setDisplayMode()` へ集約した。
- クリップ面の検証、設定、投影更新を `GgApp::Window::setClipPlanes()` へ集約した。
- Leap Motion の開始・停止を `GgApp::setLeapMotionEnabled()` へ集約した。
- ミラー表示とシーン表示をアクセサー経由で変更するようにした。
- `Menu` は UI 入力を取得し、操作 API を呼び出す役割に限定した。

## 2026-07-21: 指摘 4 — クラス境界の整理

- `Menu` が保持する `GgApp*` を `GgApp&` に変更し、null 状態を排除した。
- `Attitude` に姿勢調整値の取得・変更 API を追加し、`Menu` からの直接編集を廃止した。
- `Window` の `showScene`、`showMirror`、`showMenu` を非公開化し、描画ループを含めアクセサー経由へ変更した。

## 2026-07-21: 指摘 5 — カメラ能力 API の分離

- バックエンド非依存の `CaptureCapability` と `CameraCapabilities` API を追加した。
- Media Foundation の `VideoFormat` と `GUID` を `Menu` から排除した。
- GUID からコーデック名への変換を `CamMf.cpp` 側へ集約した。
- `Menu.h` から `CamMf.h` への直接依存を除去した。

## 2026-07-21: `GgApp` と TED 本体の分離検討

### 依頼

`GgApp.h`／`GgApp.cpp` から TED 本体への依存をなくせるか検討する。

### 方針

- GLFW、OpenGL、OpenXR、ImGui を扱う汎用ウィンドウ層と、`Config`、`Attitude`、`Scene`、`Camera` を所有する TED 本体を分離する。
- 現在の外側の `GgApp` は TED 本体として `TedApp` へ移し、`GgApp::Window` は独立した `GgWindow` とする案を基本とする。
- 入力処理は TED の状態を直接変更せず、イベントまたはコールバックとして TED 側へ通知する。
- OpenXR 層は追跡結果を返し、`Scene` への反映は TED 側で行う。
- この分離は影響範囲が大きいため、既存の `Menu` 改善と API 化を先に記録・安定化してから実施する。

## 2026-07-21: 開発方針・履歴・現状文書の更新

- `GEMINI.md` に責務分離、操作 API、設定トランザクション、資源管理、検証の開発方針を追加した。
- 本ファイル `REQUESTS.md` を作成し、`Menu.cpp` の点検から `GgApp` 分離検討までの依頼と対応を記録した。
- `README.md` に現在のクラス構成、設定再読み込み、表示・Leap Motion・姿勢・カメラ能力 API、外部資源の所有権を追記した。

## 検証状況

- 各変更で `git diff --check` を実施した。
- その後、依存ファイルを配置した既存の CMake Debug 構成で `ted` ターゲットのコンパイルとリンクに成功した。
- リンク時には既存の `LNK4098`（`MSVCRT` の競合）警告が残っている。

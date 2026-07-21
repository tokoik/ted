# 設定ファイル仕様 (config.json)

本プロジェクトの設定ファイルは JSON 形式です。このファイル名をコマンドライン引数に指定して起動してください。指定した設定ファイルが作業ディレクトリに存在しない場合は、デフォルト値を設定したファイルが自動的に新規作成されます。ファイル名を指定しない場合は `config.json` というファイル名が使用されます。

---

## 設定項目一覧

設定ファイルで指定可能なキーの一覧です。

### 1. 画面表示・立体視設定

| キー名 | 型 | 説明 |
| :--- | :--- | :--- |
| `stereo` | 数値 | 立体視方式の指定。※1 |
| `quadbuffer` | 数値 / 真偽値 | クワッドバッファステレオ表示を行うか (0: 行わない, 1: 行う) |
| `fullscreen` | 数値 / 真偽値 | フルスクリーン表示を行うか (0: 行わない, 1: 行う) |
| `use_secondary` | 数値 | フルスクリーン表示をプライマリディスプレイに行う場合は `0`、それ以外（サブディスプレイなど）なら `1` 以上を指定 |
| `display_width` | 数値 | 表示ディスプレイの横方向の解像度（画素数） |
| `display_height` | 数値 | 表示ディスプレイの縦方向の解像度（画素数） |
| `display_aspect` | 数値 | ディスプレイの縦横比（アスペクト比）。`0` の場合は自動設定されます。 |
| `display_center` | 数値 | 表示するディスプレイの高さの 1/2（単位: m） |
| `display_distance` | 数値 | 表示するディスプレイまでの距離（単位: m） |
| `depth_near` | 数値 | 視点から前方面（Nearクリップ面）までの距離（焦点距離、単位: m） |
| `depth_far` | 数値 | 視点から後方面（Farクリップ面）までの距離（単位: m） |

#### ※1 立体視の設定 (`stereo`)
* `0`: 単眼視
* `1`: 上下2分割（トップアンドボトム）
* `2`: 左右2分割（サイドバイサイド）
* `3`: 左右の映像を同じ表示領域へ重ねる
* `4`: クワッドバッファステレオ
* `5`: OpenXR (HMD) モード
※ HMDモード使用時は、通常 `5` を指定します。

---

### 2. 映像入力・カメラ設定

| キー名 | 型 | 説明 |
| :--- | :--- | :--- |
| `input_mode` | 数値 | 映像入力ソースのモード。※2 |
| `left_camera` | 数値 | 左目に使用する Media Foundation カメラの列挙番号。負数はカメラを使用しない。※3 |
| `right_camera` | 数値 | 右目に使用する Media Foundation カメラの列挙番号。負数はカメラを使用しない。※3 |
| `left_image` | 文字列 | 左カメラ不使用時（単眼や静止画モード）に表示する静止画ファイル名 |
| `right_image` | 文字列 | 右カメラ不使用時（単眼や静止画モード）に表示する静止画ファイル名 |
| `left_movie` | 文字列 | 左カメラの代わりに使用する動画ファイル名 |
| `right_movie` | 文字列 | 右カメラの代わりに使用する動画ファイル名 |
| `capture_width` | 数値 | カメラからキャプチャする画像の幅。`0` の場合はカメラから自動取得します。 |
| `capture_height` | 数値 | カメラからキャプチャする画像の高さ。`0` の場合はカメラから自動取得します。 |
| `capture_fps` | 数値 | カメラからキャプチャする際のフレームレート。`0` の場合はカメラから自動取得します。 |
| `capture_codec` | 文字列 | カメラのコーデック（例: `"MJPG"`, `"YUY2"` など） |
| `left_capture_codec` | 文字列 | 左カメラ用の個別コーデック（指定がない場合は `capture_codec` または `"MJPG"` にフォールバックします） |
| `right_capture_codec`| 文字列 | 右カメラ用の個別コーデック（指定がない場合は `capture_codec` または `"MJPG"` にフォールバックします） |
| `left_capture_resolution`| 文字列 | 左カメラの個別解像度（例: `"1280 x 720"`） |
| `right_capture_resolution`| 文字列 | 右カメラの個別解像度（例: `"1280 x 720"`） |
| `screen_samples` | 数値 | 背景画像をマッピングする際のメッシュの分割数（数値が大きいほど歪み補正の精度が向上します） |
| `texture_repeat` | 数値 / 真偽値 | 背景画像を繰り返しでマッピングするか (0: 通常, 1: 正距円筒図法などの繰り返し) |
| `tracking` | 数値 / 真偽値 | 背景画像をヘッドトラッキングに追従させるか (0: 固定, 1: 追従) |
| `fisheye_center_x` | 数値 | 魚眼レンズの中心位置の水平オフセット（縦横を `[-1, 1]` とする領域に対する相対値） |
| `fisheye_center_y` | 数値 | 魚眼レンズの中心位置の垂直オフセット（縦横を `[-1, 1]` とする領域に対する相対値） |
| `fisheye_fov_x` | 数値 | 魚眼レンズの水平方向の画角の初期値（単位: ラジアン。通常のカメラや RICOH THETA では `1.0`） |
| `fisheye_fov_y` | 数値 | 魚眼レンズの垂直方向の画角の初期値（単位: ラジアン。通常のカメラや RICOH THETA では `1.0`） |
| `ovrvision_property` | 数値 | Ovrvision Pro を使用する場合の動作モード設定。※4 |
| `controller` | 数値 / 真偽値 | ゲームコントローラを使用するか。保存時には出力されますが、現行の読み込み処理はこのキーを参照しません。下記の注意を参照してください。 |
| `leap_motion` | 数値 / 真偽値 | Leap Motion（ハンドトラッキング）を使用するか (0: 使用しない, 1: 使用する) |
| `vertex_shader` | 文字列 | 入力画像の展開に使用するバーテックスシェーダのファイル名。※5 |
| `fragment_shader` | 文字列 | 入力画像の描画に使用するフラグメントシェーダのファイル名。※6 |

現行の `Config::read()` は `leap_motion` を `use_controller` と `use_leap_motion` の両方へ読み込みます。このため、起動時のゲームコントローラ使用状態は `leap_motion` と同じ値になります。メニュー上では両者を個別に変更できます。

#### ※2 映像入力モード (`input_mode`)
* `0`: 静止画（`left_image` / `right_image`）
* `1`: 動画（`left_movie` / `right_movie`）
* `2`: Web カメラ（`left_camera` / `right_camera` の番号）
* `3`: Ovrvision Pro
* `4`: リモート接続（ネットワーク越しに送信されてきた画像）

#### ※3 映像入力の選択
* `left_camera < 0` かつ `right_camera < 0` : 静止画像ファイルを使用
* `left_camera >= 0` かつ `right_camera < 0` : 左カメラのみを使用（単眼）
* `left_camera < 0` かつ `right_camera >= 0` : Ovrvision Pro を使用
* `left_camera >= 0` かつ `right_camera >= 0` : 左右のカメラを使用（両眼）
動画ファイルはカメラ番号へ文字列として指定せず、`input_mode` を `1` にして `left_movie`／`right_movie` に指定します。

#### ※4 Ovrvision Pro の動作モード設定 (`ovrvision_property`)
* `0`: `OVR::OV_CAM5MP_FULL` (2560x1920, 15fps x2)
* `1`: `OVR::OV_CAM5MP_FHD` (1920x1080, 30fps x2)
* `2`: `OVR::OV_CAMHD_FULL` (1280x960, 45fps x2)
* `3`: `OVR::OV_CAMVR_FULL` (960x950, 60fps x2) - HMD取付時は通常これを選択
* `4`: `OVR::OV_CAMVR_WIDE` (1280x800, 60fps x2)
* `5`: `OVR::OV_CAMVR_VGA` (640x480, 90fps x2)
* `6`: `OVR::OV_CAMVR_QVGA` (320x240, 120fps x2)
* `7`: `OVR::OV_CAM20HD_FULL` (1280x960 (USB2.0のみ), 15fps x2)
* `8`: `OVR::OV_CAM20VR_VGA` (640x480 (USB2.0のみ), 30fps x2)

#### ※5 バーテックスシェーダの選択
* `"rectangle.vert"`: 平面カメラを使用して視線を回転する (Ovrvision Pro)
* `"fisheye.vert"`: 光軸を水平にした魚眼カメラ
* `"pixpro.vert"`: 光軸を垂直にした魚眼カメラ (PIXPRO SP360 4K)
* `"panorama.vert"`: 正距円筒図法 (UVC Blender)
* `"theta.vert"`: RICOH THETA S の二重魚眼ライブビデオ映像用

#### ※6 フラグメントシェーダの選択
* `"normal.frag"`: テクスチャ座標の画素をそのまま使用する (通常)
* `"panorama.frag"`: 正距円筒図法 (UVC Blender)
* `"theta.frag"`: RICOH THETA S のライブビデオ映像用

---

### 3. ネットワーク（役割・通信）設定

| キー名 | 型 | 説明 |
| :--- | :--- | :--- |
| `role` | 数値 | システムの役割指定。※7 |
| `port` | 数値 | 画像のネットワーク転送・通信に使用するポート番号（送信・受信で共通） |
| `host` | 文字列 | 送信先（作業者側など）の IP アドレス |
| `stabilize` | 数値 / 真偽値 | リモートから受信した全方位映像をロボットヘッド取り付け時に安定化（スタビライズ）するか |
| `tracking_delay` | 数値 | 左右両方の画像に対してヘッドトラッキングを遅らせるフレーム数 |
| `tracking_delay_left`| 数値 | 左目の画像に対してヘッドトラッキングを遅らせるフレーム数 |
| `tracking_delay_right`|数値 | 右目の画像に対してヘッドトラッキングを遅らせるフレーム数 |
| `texture_quality` | 数値 | ネットワーク転送する画像の JPEG 圧縮品質 (0〜100 %) |
| `texture_reshape` | 数値 / 真偽値 | ネットワーク転送する画像を送信側でドーム画像に変形するか |
| `texture_samples` | 数値 | 平面画像を全天球画像に変換・変形する際に用いるメッシュの格子点数 |
| `remote_fov_x` | 数値 | ドーム画像に変形するロボット（送信）側の平面カメラの水平方向の画角（単位: ラジアン） |
| `remote_fov_y` | 数値 | ドーム画像に変形するロボット（送信）側の平面カメラの垂直方向の画角（単位: ラジアン） |
| `local_share_size` | 数値 | 送信用に確保する共有メモリのブロック（フレーム）数（標準: `64`） |
| `remote_share_size`| 数値 | 受信用に確保する共有メモリのブロック（フレーム）数（標準: `64`） |

#### ※7 システムの役割 (`role`)
* `0`: スタンドアローン（単独で動作、ネットワーク転送なし）
* `1`: 指示者（操縦者 / 科学者）側
* `2`: 作業者（ロボット）側

---

### 4. その他（シーングラフ・UI）設定

| キー名 | 型 | 説明 |
| :--- | :--- | :--- |
| `max_level` | 数値 | シーンファイルのインクルード（入れ子）の深さの最大値 (標準: `10`) |
| `scene` | 文字列 / オブジェクト | 読み込むシーングラフ定義。JSONオブジェクトを直接記述するか、外部のシーン記述ファイル名（例: `"scene.json"`）を文字列で指定します。 |
| `menu_font` | 文字列 | メニュー表示に使用する TrueType / OpenType フォントのパス（例: `"NotoSansCJKjp-Regular.otf"`） |
| `menu_font_size` | 数値 | メニューのフォントサイズ |

---

## 設定ファイル (config.json) の具体例

```json
{
  "stereo": 0,
  "quadbuffer": 0,
  "fullscreen": 0,
  "use_secondary": 0,
  "display_width": 1280,
  "display_height": 720,
  "display_aspect": 0,
  "display_center": 0.5,
  "display_distance": 1.5,
  "depth_near": 0.1,
  "depth_far": 5,
  "input_mode": 0,
  "left_camera": -1,
  "left_image": "left.jpg",
  "left_movie": "",
  "left_capture_codec": "MJPG",
  "left_capture_resolution": "1280 x 720",
  "right_camera": -1,
  "right_image": "right.jpg",
  "right_movie": "",
  "right_capture_codec": "MJPG",
  "right_capture_resolution": "1280 x 720",
  "screen_samples": 1271,
  "texture_repeat": 0,
  "tracking": 1,
  "stabilize": 1,
  "capture_width": 1280,
  "capture_height": 720,
  "capture_fps": 0,
  "capture_codec": "MJPG",
  "fisheye_center_x": 0,
  "fisheye_center_y": 0,
  "fisheye_fov_x": 1,
  "fisheye_fov_y": 1,
  "ovrvision_property": 3,
  "controller": 0,
  "leap_motion": 0,
  "vertex_shader": "fixed.vert",
  "fragment_shader": "normal.frag",
  "port": 0,
  "role": 0,
  "tracking_delay_left": 0,
  "tracking_delay_right": 0,
  "texture_quality": 50,
  "texture_reshape": 0,
  "texture_samples": 1372,
  "remote_fov_x": 1,
  "remote_fov_y": 1,
  "local_share_size": 64,
  "remote_share_size": 64,
  "max_level": 10,
  "scene": "scene.json"
}
```

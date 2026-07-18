# シーングラフ仕様 (scenegraph.md)

本プロジェクトでは、ロボットや操作デバイス、手の表示モデルなどの3Dオブジェクトの配置や階層構造を「シーングラフ」として管理します。
C++の `Scene` クラスと、それを外部から定義するための JSON ファイル (`scene.json`) の仕様について解説します。

---

## 1. C++ における `Scene` クラス

シーン（ロボットやパーツ、照準）の形状データは、`Scene` クラスで管理しています。

* **形状データ形式**: 三角形分割された Alias OBJ 形式 (`.obj`)。
* **マテリアルとテクスチャ**: マテリアルファイル (`.mtl`) の設定値は参照されますが、テクスチャ画像は利用しません。

### C++ コードによる構築例
例えば、オブジェクト `"upper.obj"` を Z軸中心に `ru` ラジアン回転させ、原点（視点）に対して `(xu, yu, zu)` の位置に配置する場合は以下のように記述します。

```cpp
GgObj upper("upper.obj");
GgMatrix mu(ggTranslate(xu, yu, zu) * ggRotateZ(ru));
Scene scene(&upper, mu);
```

このオブジェクトの下位（子ノード）に `"lower.obj"` を X軸中心に `rl` ラジアン回転させ、`"upper.obj"` の原点に対して `(xl, yl, zl)` の位置に配置する場合は、`addChild()` を用いて階層構造を構築します。

```cpp
// 子ノードの追加
scene.addChild("lower.obj", ggTranslate(xl, yl, zl) * ggRotateX(rl));
```

さらに、その配下に別のオブジェクトを追加するなど、階層を深くする場合は `addChild()` の戻り値（追加した子ノードのポインタ）を保持して操作します。

```cpp
// 階層構造: upper.obj -> lower.obj -> hand.obj
Scene *child(scene.addChild("lower.obj", ggTranslate(xl, yl, zl) * ggRotateX(rl)));
child->addChild("hand.obj", ggTranslate(xh, yh, zh) * ggRotateY(rh));
```

### 動的な変形・回転の更新
オブジェクトの姿勢や回転角を動的に変更する場合は、`loadModelMatrix()` を使用して変換行列を更新します。

```cpp
// upper.obj の回転角を更新
scene.loadModelMatrix(ggTranslate(xu, yu, zu) * ggRotateZ(new_ru));

// lower.obj の回転角を更新
child->loadModelMatrix(ggTranslate(xl, yl, zl) * ggRotateZ(new_rl));
```

### 変換行列の型について
`Scene` のコンストラクタや `addChild()`, `loadModelMatrix()` に指定する行列のデータ型 `GgMatrix` は、`GLfloat` 型の16要素からなる列優先の行列（OpenGLの変換行列の仕様と同一）です。詳細な仕様は以下を参照してください。
* [GgMatrix クラス仕様 (外部リンク)](http://www.wakayama-u.ac.jp/~tokoi/lecture/gg/html/classgg_1_1GgMatrix.html)

---

## 2. シーングラフファイル (scene.json) の仕様

`config.json` の `"scene"` キーに JSON オブジェクトを直接記述するか、または外部のJSONファイル（例: `scene.json`）を指定することで、C++のコードを変更することなく、3Dオブジェクトの配置や動作デバイスとのマッピングを変更できます。

### ノードのプロパティ一覧
シーングラフの各ノードは以下のキーを持つオブジェクトとして記述します（すべてのプロパティは省略可能です）。

| キー名 | 型 | 説明 |
| :--- | :--- | :--- |
| `"position"` | 配列 `[x, y, z]` | 親ノードの原点に対するこのオブジェクトの位置オフセット |
| `"rotation"` | 配列 `[x, y, z, angle]` | 回転軸ベクトル `[x, y, z]` と、その軸まわりの回転角 `angle`（単位: ラジアン） |
| `"scale"` | 配列 `[x, y, z]` | オブジェクトの拡大率（X, Y, Z方向） |
| `"controller"`| 数値 | ローカルのモーションコントローラ（Leap Motion等）の変換行列番号を指定します。※1 |
| `"remote_controller"`| 数値 | リモート（通信相手先）のモーションコントローラの変換行列番号を指定します。※1 |
| `"model"` | 文字列 | 描画する Alias OBJ 形式のファイル名（例: `"handl.obj"`, `"finger.obj"` など） |
| `"children"` | 配列 | このノードの下位（子）に接続するシーングラフノードの配列 |

---

## ※1 モーションコントローラの変換行列番号マッピング

Leap Motion やデバイスのトラッキング情報から取得した各パーツ（関節）の位置姿勢情報は、共有メモリ上の変換行列のテーブルに格納されます。JSONの `"controller"` または `"remote_controller"` でそのインデックス番号を指定することで、対応する関節の動きに3Dモデルを追従させることができます。

### 行列番号の対応表

| 番号 | 対応する部位 / データ源 |
| :--- | :--- |
| **0** | HMD の左目の位置 (camL) |
| **1** | HMD の右目の位置 (camR) |
| **2** | ゲームパッドの2本のスティック入力から算出された座標変換行列 |
| **3** | 左手の手のひら (Palm) |
| **4** | 右手の手のひら (Palm) |
| **5** | 左手首 (Wrist) |
| **6** | 右手首 (Wrist) |
| **7 〜 46** | 左右の手の各指の関節 (骨)。※下記の計算式に基づきます。 |
| **47 〜 63**| システム予約（未使用領域） |

### 指の関節番号 (7 〜 46) の計算式
指の関節位置は、以下の計算式から算出されるインデックスを指定します。

$$\text{インデックス} = 7 + (d \times 8) + (b \times 2) + \text{base}$$

* **$d$ (指の番号)**:
  * `0`: 親指 (Thumb)
  * `1`: 人差し指 (Index)
  * `2`: 中指 (Middle)
  * `3`: 薬指 (Ring)
  * `4`: 小指 (Pinky)
* **$b$ (骨の節番号)**:
  * `0`: 第3関節（中手骨 / 親指の場合は母指球）
  * `1`: 第2関節（基節骨）
  * `2`: 第1関節（中節骨）
  * `3`: 指先（末節骨）
* **$\text{base}$ (左右の手の差分)**:
  * `0`: 左手
  * `1`: 右手

#### 指のインデックス具体例
* **左手の親指の母指球**: `7 + (0 * 8) + (0 * 2) + 0 = 7`
* **右手の親指の母指球**: `7 + (0 * 8) + (0 * 2) + 1 = 8`
* **左手の人差し指の指先**: `7 + (1 * 8) + (3 * 2) + 0 = 21`
* **右手の人差し指の指先**: `7 + (1 * 8) + (3 * 2) + 1 = 22`
* **右手の小指の指先**: `7 + (4 * 8) + (3 * 2) + 1 = 45`

---

## シーングラフファイル (scene.json) の具体例

以下は、Leap Motion からの手のトラッキングデータ（手のひら、手首、各指の骨）に追従する手の3Dモデル（`handr.obj`, `handl.obj`, `finger.obj`）の階層構造を定義した `scene.json` の記述例です。

```json
{
  "position": [ 0, -0.05, 0 ],
  "children": [
    {
      "controller": 0,
      "model": "handr.obj"
    },
    {
      "controller": 1,
      "model": "handl.obj"
    },
    {
      "controller": 2,
      "model": "finger.obj"
    },
    {
      "controller": 3,
      "model": "finger.obj"
    },
    {
      "controller": 4,
      "model": "finger.obj"
    },
    {
      "controller": 5,
      "model": "finger.obj"
    },
    {
      "controller": 6,
      "model": "finger.obj"
    },
    {
      "controller": 7,
      "model": "finger.obj"
    },
    {
      "controller": 8,
      "model": "finger.obj"
    },
    {
      "controller": 9,
      "model": "finger.obj"
    },
    {
      "controller": 10,
      "model": "finger.obj"
    },
    {
      "controller": 11,
      "model": "finger.obj"
    },
    {
      "controller": 12,
      "model": "finger.obj"
    },
    {
      "controller": 13,
      "model": "finger.obj"
    },
    {
      "controller": 14,
      "model": "finger.obj"
    },
    {
      "controller": 15,
      "model": "finger.obj"
    },
    {
      "controller": 16,
      "model": "finger.obj"
    },
    {
      "controller": 17,
      "model": "finger.obj"
    },
    {
      "controller": 18,
      "model": "finger.obj"
    },
    {
      "controller": 19,
      "model": "finger.obj"
    },
    {
      "controller": 20,
      "model": "finger.obj"
    },
    {
      "controller": 21,
      "model": "finger.obj"
    },
    {
      "controller": 22,
      "model": "finger.obj"
    },
    {
      "controller": 23,
      "model": "finger.obj"
    },
    {
      "controller": 24,
      "model": "finger.obj"
    },
    {
      "controller": 25,
      "model": "finger.obj"
    },
    {
      "controller": 26,
      "model": "finger.obj"
    },
    {
      "controller": 27,
      "model": "finger.obj"
    },
    {
      "controller": 28,
      "model": "finger.obj"
    },
    {
      "controller": 29,
      "model": "finger.obj"
    },
    {
      "controller": 30,
      "model": "finger.obj"
    },
    {
      "controller": 31,
      "model": "finger.obj"
    },
    {
      "controller": 32,
      "model": "finger.obj"
    },
    {
      "controller": 33,
      "model": "finger.obj"
    },
    {
      "controller": 34,
      "model": "finger.obj"
    },
    {
      "controller": 35,
      "model": "finger.obj"
    },
    {
      "controller": 36,
      "model": "finger.obj"
    },
    {
      "controller": 37,
      "model": "finger.obj"
    },
    {
      "controller": 38,
      "model": "finger.obj"
    },
    {
      "controller": 39,
      "model": "finger.obj"
    },
    {
      "controller": 40,
      "model": "finger.obj"
    },
    {
      "controller": 41,
      "model": "finger.obj"
    },
    {
      "controller": 42,
      "model": "finger.obj"
    },
    {
      "controller": 43,
      "model": "finger.obj"
    }
  ]
}
```

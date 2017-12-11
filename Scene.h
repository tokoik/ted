#pragma once

//
// シーングラフ
//

// 各種設定
#include "config.h"

// 補助プログラム
#include "gg.h"
using namespace gg;

// Leap Motion
#include "LeapListener.h"

// 共有メモリ
#include "SharedMemory.h"

// シーングラフは JSON で記述する
#include "picojson.h"

// 標準ライブラリ
#include <memory>
#include <map>
#include <queue>

class Scene
{
  // 子供のパーツのリスト
  std::vector<Scene *> children;

  // 描画するパーツ
  const GgObj *obj;

  // 読み込んだパーツを登録するパーツリスト
  static std::map<const std::string, std::unique_ptr<const GgObj>> parts;

  // このパーツのモデル変換行列
  GgMatrix mm;

  // このパーツが参照する外部モデル変換行列
  const GgMatrix *me;

  // 外部モデル変換行列のテーブル
  static SharedMemory *localMatrix, *remoteMatrix;

  // リモートの外部モデル変換行列のテーブルのコピー
  static std::vector<GgMatrix> localJointMatrix, remoteJointMatrix;

  // この骨格を制御する Leap Motion
  static LeapListener *controller;

  // リモートカメラの姿勢のタイミングをフレームに合わせて遅らせるためのキュー
  static std::queue<GgMatrix> fifo[remoteCamCount];

public:

  // コンストラクタ
  Scene(const GgObj *obj = nullptr);

  // シーングラフからシーンのオブジェクトを作成するコンストラクタ
  Scene(const picojson::value &v, const GgSimpleShader *shader, int level = 0);
  
  // シーングラフからシーンのオブジェクトを作成するコンストラクタ
  Scene(const picojson::value &v, const GgSimpleShader &shader, int level = 0);

  // デストラクタ
  virtual ~Scene();

  // モデル変換行列のテーブルを選択する
  static void selectTable(SharedMemory *local, SharedMemory *remote);

  // モデル変換行列を制御するコントローラを選択する
  static void selectController(LeapListener *controller);

  // シーングラフを読み込む
  Scene *load(const picojson::value &v, const GgSimpleShader *shader, int level);

  // 子供にシーンを追加する
  Scene *addChild(Scene *scene);

  // 子供にパーツに追加する
  Scene *addChild(GgObj *obj = nullptr);

  // ローカルとリモートの変換行列を共有メモリから取り出す
  static void setup();

  // リモートのカメラのトラッキング情報を遅延させて取り出す
  static const GgMatrix &getRemoteAttitude(int cam);

  // このパーツ以下のすべてのパーツを描画する
  void draw(
    const GgMatrix &mp = ggIdentity(),
    const GgMatrix &mv = ggIdentity(),
    const GgMatrix &mm = ggIdentity()) const;
};

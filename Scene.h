#pragma once

//
// シーングラフ
//

// 補助プログラム
#include "gg.h"
using namespace gg;

// 共有メモリ
#include "SharedMemory.h"

// Leap Motion
#include "LeapListener.h"

// シーングラフは JSON で記述する
#include "picojson.h"

class Scene
{
  // 子供のパーツのリスト
  std::vector<Scene *> children;

  // 描画するパーツ
  const GgObj *obj;

  // このパーツのモデル変換行列
  GgMatrix mm;

  // このパーツが参照する外部モデル変換行列
  const GgMatrix *me;

  // 外部モデル変換行列のテーブル
  static SharedMemory *localMatrix, *remoteMatrix;

  // この骨格を制御する Leap Motion
  static LeapListener *controller;

  // このパーツ以下のすべてのパーツを描画する
  void drawNode(const GgMatrix &mp, const GgMatrix &mv, const GgMatrix &mm) const;

public:

  // コンストラクタ
  Scene(const GgObj *obj = nullptr)
    : obj(obj)
  {
  }

  // シーングラフからシーンのオブジェクトを作成するコンストラクタ
  Scene(const picojson::value &v, const GgSimpleShader *shader, int level = 0)
    : obj(nullptr)
  {
    load(v, shader, level);
  }
  
  // シーングラフからシーンのオブジェクトを作成するコンストラクタ
  Scene(const picojson::value &v, const GgSimpleShader &shader, int level = 0)
    : Scene(v, &shader, level) {}

  // デストラクタ
  virtual ~Scene();

  // モデル変換行列のテーブルを選択する
  static void selectTable(SharedMemory *local, SharedMemory *remote)
  {
    Scene::localMatrix = local;
    Scene::remoteMatrix = remote;
  }

  // モデル変換行列を制御するコントローラを選択する
  static void selectController(LeapListener *controller)
  {
    // コントローラを登録する
    Scene::controller = controller;
  }

  // シーングラフを読み込む
  Scene *load(const picojson::value &v, const GgSimpleShader *shader, int level);

  // 子供にシーンを追加する
  Scene *addChild(Scene *scene)
  {
    children.push_back(scene);
    return scene;
  }

  // 子供にパーツに追加する
  Scene *addChild(GgObj *obj = nullptr)
  {
    return addChild(new Scene(obj));
  }

  // このパーツ以下のすべてのパーツを描画する
  void draw(const GgMatrix &mp = ggIdentity(),
    const GgMatrix &mv = ggIdentity(),
    const GgMatrix &mm = ggIdentity()) const
  {
    if (localMatrix->lock())
    {
      drawNode(mp, mv, mm);
      localMatrix->unlock();
    }
  }
};

#pragma once

//
// シーングラフ
//

// 各種設定
#include "config.h"

// Leap Motion
#include "LeapListener.h"

// 標準ライブラリ
#include <queue>

class Scene
{
  // 子供のパーツのリスト
  std::vector<Scene *> children;

  // 描画するパーツ
  const GgSimpleObj *obj;

  // 読み込んだパーツを登録するパーツリスト
  static std::map<const std::string, std::unique_ptr<const GgSimpleObj>> parts;

  // このパーツのモデル変換行列
  GgMatrix mm;

  // このパーツが参照する外部モデル変換行列
  const GgMatrix *me;

  // リモートの外部モデル変換行列のテーブルのコピー
  static std::vector<GgMatrix> localJointMatrix, remoteJointMatrix;

  // リモートカメラの姿勢のタイミングをフレームに合わせて遅らせるためのキュー
  static std::queue<GgMatrix> fifo[remoteCamCount];

public:

  // コンストラクタ
  Scene(const GgSimpleObj *obj = nullptr);

  // シーングラフからシーンのオブジェクトを作成するコンストラクタ
  Scene(const picojson::value &v, const GgSimpleShader *shader, int level = 0);
  
  // シーングラフからシーンのオブジェクトを作成するコンストラクタ
  Scene(const picojson::value &v, const GgSimpleShader &shader, int level = 0);

  // デストラクタ
  virtual ~Scene();

  // 変換行列を初期化する
  static void initialize();

  // シーングラフを読み込む
  Scene *load(const picojson::value &v, const GgSimpleShader *shader, int level);

  // 子供にシーンを追加する
  Scene *addChild(Scene *scene);

  // 子供にパーツに追加する
  Scene *addChild(GgSimpleObj *obj = nullptr);

  // ローカルとリモートの変換行列を共有メモリから取り出す
  static void setup();

  // リモートのカメラのトラッキング情報を遅延させて取り出す
  static const GgMatrix &getRemoteAttitude(int cam);

  // このパーツ以下のすべてのパーツを描画する
  void draw(const GgMatrix &mp, const GgMatrix &mv) const;
};

#pragma once

//
// シーングラフ
//

// 各種設定
#include "config.h"

// Leap Motion 関連の処理
#include "LeapListener.h"

// 共有メモリ
#include "SharedMemory.h"

// 標準ライブラリ
#include <map>
#include <queue>

// 共有メモリ上に置く指示者の変換行列
extern std::unique_ptr<SharedMemory> localAttitude;

// 共有メモリ上に置く作業者の変換行列
extern std::unique_ptr<SharedMemory> remoteAttitude;

class Scene
{
  // 子供のパーツのリスト
  std::vector<Scene*> children;

  // 描画するパーツ
  const GgSimpleObj* obj;

  // 使用するシェーダ
  static const GgSimpleShader* shader;

  // 読み込んだパーツを登録するパーツリスト
  static std::map<const std::string, std::unique_ptr<const GgSimpleObj>> parts;

  // このパーツのモデル変換行列
  GgMatrix mm;

  // このパーツが参照する外部モデル変換行列
  const GgMatrix* me;

  // リモートの外部モデル変換行列のテーブルのコピー
  static std::vector<GgMatrix> localMatrixTable, remoteMatrixTable;

  // リモートカメラの姿勢のタイミングをフレームに合わせて遅らせるためのキュー
  static std::queue<GgMatrix> fifo[remoteCamCount];

  // Leap Motion
  static LeapListener listener;

public:

  // コンストラクタ
  Scene(const GgSimpleObj* obj = nullptr);

  // シーングラフからシーンのオブジェクトを作成するコンストラクタ
  Scene(const picojson::value& v, int level = 0);

  // デストラクタ
  virtual ~Scene();

  // 共有メモリを確保して初期化する
  static bool initialize(unsigned int local_size, unsigned int remote_size);

  // シーングラフを読み込む
  picojson::object load(const picojson::value& v);

  // シーングラフを解析する
  Scene* read(const picojson::value& v, int level);

  // 子供にシーンを追加する
  Scene* addChild(Scene* scene);

  // 子供にパーツに追加する
  Scene* addChild(GgSimpleObj* obj = nullptr);

  // ローカルとリモートの変換行列を設定する
  static void setup(const GgMatrix& m);

#if defined(LEAP_INTERPORATE_FRAME)
  // ローカルとリモートの変換行列を更新する
  static void update();
#endif

  // ローカルの変換行列のテーブルに保存する
  static void setLocalAttitude(int cam, const GgMatrix& m);

  // リモートのカメラのトラッキング情報を遅延させて取り出す
  static const GgMatrix& getRemoteAttitude(int cam);

  // Leap Motion を起動する
  static bool startLeapMotion()
  {
    // Leap Motion の listener と controller を作る
    return listener.openConnection() != nullptr;
  }

  // Leap Motion を起動する
  static void stopLeapMotion()
  {
    // Leap Motion の listener と controller を作る
    listener.closeConnection();
  }

  // シェーダを設定する
  static void setShader(const GgSimpleShader& shader)
  {
    Scene::shader = &shader;
  }

  // このパーツ以下のすべてのパーツを描画する
  void draw(const GgMatrix& mp, const GgMatrix& mv) const;
};

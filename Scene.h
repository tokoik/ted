#pragma once

//
// シーングラフ
//

// 各種設定
#include "Config.h"

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

// JSONで定義された階層モデルを保持し、親から子へモデル変換を累積して描画する。
// 共有メモリ上の姿勢行列をノードへ結び付け、Leap Motionや遠隔姿勢も同じ経路で反映する。
class Scene
{
  // このノードが所有し、デストラクタで破棄する子ノード
  std::vector<Scene*> children;

  // parts が所有する共有モデル、または外部所有のモデル。Scene自身は破棄しない。
  const GgSimpleObj* obj{ nullptr };

  // 使用するシェーダ
  static const GgSimpleShader* shader;

  // 同じモデルファイルを重複ロードしないための、モデル名をキーにした所有キャッシュ
  static std::map<const std::string, std::unique_ptr<const GgSimpleObj>> parts;

  // このパーツのモデル変換行列
  GgMatrix mm;

  // controller指定がある場合に参照する共有姿勢行列。テーブルの要素を所有しない。
  const GgMatrix* me{ nullptr };

  // リモート側から受信したモデル変換行列の保持領域
  static std::vector<GgMatrix> localMatrixTable, remoteMatrixTable;

  // リモートカメラの姿勢のタイミングをフレームに合わせて遅らせるためのキュー
  static std::queue<GgMatrix> fifo[remoteCamCount];

  // Leap Motion
  static LeapListener listener;

public:

  // シーンが空（パーツも子供も持たない）かどうか判定する
  bool isEmpty() const { return obj == nullptr && children.empty(); }

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

  // 現在の視点・Leap姿勢を共有領域へ公開し、同時にリモート姿勢を取り込む
  static void setup(const GgMatrix& m);

#if defined(LEAP_INTERPORATE_FRAME)
  // ローカルとリモートの変換行列を更新する
  static void update();
#endif

  // ローカルの変換行列のテーブルに保存する
  static void setLocalAttitude(int cam, const GgMatrix& m);

  // リモートのカメラのトラッキング情報を遅延させて取り出す
  static const GgMatrix& getRemoteAttitude(int cam);

  // Leap Motionサービスへ接続し、関節姿勢を更新するポーリングスレッドを開始する
  static bool startLeapMotion()
  {
    // Leap Motion の listener と controller を作る
    return listener.openConnection() != nullptr;
  }

  // ポーリングスレッドを停止してLeap Motionサービスとの接続を閉じる
  static void stopLeapMotion()
  {
    // Scene破棄後にコールバックが残らないよう、共有listenerを明示的に停止する
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

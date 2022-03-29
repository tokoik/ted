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
#include <queue>

class Scene
{
  // 設定
  static const Config* config;

  // 共有メモリ上に置く操縦者の変換行列
  static std::unique_ptr<SharedMemory> localAttitude;

  // 共有メモリ上に置く作業者の変換行列
  static std::unique_ptr<SharedMemory> remoteAttitude;

  // 子供のパーツのリスト
  std::vector<Scene*> children;

  // 描画するパーツ
  const GgSimpleObj* obj;

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
  static std::unique_ptr<LeapListener> listener;

public:

  // コンストラクタ
  Scene(const GgSimpleObj* obj = nullptr);

  // シーングラフからシーンのオブジェクトを作成するコンストラクタ
  Scene(const picojson::value& v, const GgSimpleShader* shader, int level = 0);

  // シーングラフからシーンのオブジェクトを作成するコンストラクタ
  Scene(const picojson::value& v, const GgSimpleShader& shader, int level = 0);

  // デストラクタ
  virtual ~Scene();

  // 共有メモリを確保して初期化する
  static bool initialize(const Config& config);

  // シーンファイルを読み込む
  picojson::object read(const std::string& file) const;

  // シーングラフを読み込む
  Scene* load(const picojson::value& v, const GgSimpleShader* shader, int level);

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


  // ローカルにコピーされたリモートの変換行列を取り出す
  static void loadLocalAttitude(GgMatrix* dst, unsigned int count)
  {
    localAttitude->load(dst, count);
  }

  // ローカルの変換行列の数を返す
  static unsigned int getLocalAttitudeSize()
  {
    return localAttitude->getSize();
  }

  // ローカルのトラッキング情報を保存する
  static void setLocalAttitude(int cam, const GgMatrix& m);

  // ローカルの姿勢情報をリモートに送る
  static void storeRemoteAttitude(const GgMatrix* src, unsigned int count)
  {
    remoteAttitude->store(src, count);
  }

  // リモートの変換行列の数を返す
  static unsigned int getRemoteAttitudeSize()
  {
    return remoteAttitude->getSize();
  }

  // リモートのトラッキング情報を遅延させて取り出す
  static const GgMatrix& getRemoteAttitude(int cam);

  // このパーツ以下のすべてのパーツを描画する
  void draw(const GgMatrix& mp, const GgMatrix& mv) const;
};

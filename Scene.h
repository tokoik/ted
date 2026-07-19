#pragma once

///
/// シーン定義クラスの定義
///
/// @file
/// @author Kohe Tokoi
/// @date July 19, 2026
///

// 各種設定
#include "Config.h"

// Leap Motion 関連の処理
#include "LeapListener.h"

// 共有メモリ
#include "SharedMemory.h"

// 標準ライブラリ
#include <map>
#include <queue>

/// 共有メモリ上に置く指導者の変換行列
extern std::unique_ptr<SharedMemory> localAttitude;

/// 共有メモリ上に置く作業者の変換行列
extern std::unique_ptr<SharedMemory> remoteAttitude;

///
/// 描画するシーンのクラス
///
/// @details
/// JSON で定義された階層モデルを保持し、親から子へモデル変換を累積して描画する。
/// 共有メモリ上の姿勢行列をノードへ結び付け、Leap Motionや遠隔姿勢も同じ経路で反映する。
///
class Scene
{
  /// このノードが所有し、デストラクタで破棄する子ノード
  std::vector<Scene*> children;

  /// parts が所有する共有モデル、または外部所有のモデル。Scene自身は破棄しない。
  const GgSimpleObj* obj{ nullptr };

  /// 使用するシェーダ
  static const GgSimpleShader* shader;

  /// シーン定義を正常に読み込めた場合は true
  bool valid{ true };

  /// 同じモデルファイルを重複ロードしないための、モデル名をキーにした所有キャッシュ
  static std::map<const std::string, std::unique_ptr<const GgSimpleObj>> parts;

  /// このパーツのモデル変換行列
  GgMatrix mm;

  /// controller指定がある場合に参照する共有姿勢行列。テーブルの要素を所有しない。
  const GgMatrix* me{ nullptr };

  /// ローカルのモデル変換行列の保持領域
  static std::vector<GgMatrix> localMatrixTable;

  /// リモート側から受信したモデル変換行列の保持領域
  static std::vector<GgMatrix> remoteMatrixTable;

  /// リモートカメラの姿勢のタイミングをフレームに合わせて遅らせるためのキュー
  static std::queue<GgMatrix> fifo[remoteCamCount];

  /// Leap Motion
  static LeapListener listener;

public:

  ///
  /// シーンが空（パーツも子供も持たない）かどうか判定する
  ///
  /// @return 空なら true、そうでなければ false
  ///
  bool isEmpty() const { return obj == nullptr && children.empty(); }

  ///
  /// シーン定義を正常に読み込めたかどうか判定する
  ///
  /// @return 正常に読み込めた場合は true、失敗した場合は false
  ///
  bool isValid() const { return valid; }

  ///
  /// コンストラクタ
  ///
  /// @param obj このパーツが参照するモデル、nullptr の場合は空のシーン
  /// 
  /// @details
  /// このコンストラクタは、指定されたモデルを参照するシーンを作成します。
  ///
  Scene(const GgSimpleObj* obj = nullptr);

  ///
  /// シーングラフからシーンのオブジェクトを作成するコンストラクタ
  ///
  /// @param v シーングラフの JSON オブジェクト
  /// @param level シーングラフの階層レベル
  ///
  Scene(const picojson::value& v, int level = 0);

  ///
  /// デストラクタ
  ///
  virtual ~Scene();

  ///
  /// 共有メモリを確保して初期化する
  ///
  /// @param local_size ローカルの共有メモリサイズ
  /// @param remote_size リモートの共有メモリサイズ
  /// @return 初期化に成功した場合は true、失敗した場合は false
  ///
  static bool initialize(unsigned int local_size, unsigned int remote_size);

  ///
  /// シーングラフを読み込む
  ///
  /// @param v シーングラフの JSON オブジェクト
  /// @return 読み込んだシーングラフのオブジェクト
  ///
  picojson::object load(const picojson::value& v);

  ///
  /// シーングラフを解析する
  ///
  /// @param v シーングラフの JSON オブジェクト
  /// @param level シーングラフの階層レベル
  /// @return 解析したシーングラフのオブジェクト
  ///
  Scene* read(const picojson::value& v, int level);

  ///
  /// 子供にシーンを追加する
  ///
  /// @param scene 追加するシーン
  /// @return 追加したシーン
  ///
  Scene* addChild(Scene* scene);

  ///
  /// 子供にパーツを追加する
  ///
  /// @param obj 追加するパーツ
  /// @return 追加したシーン
  ///
  Scene* addChild(GgSimpleObj* obj = nullptr);

  ///
  /// 現在の視点・Leap姿勢を共有領域へ公開し、同時にリモート姿勢を取り込む
  ///
  /// @param m 現在の視点・Leap姿勢の変換行列
  /// 
  static void setup(const GgMatrix& m);

#if defined(LEAP_INTERPORATE_FRAME)
  ///
  /// ローカルとリモートの変換行列を更新する
  ///
  static void update();
#endif

  ///
  /// 左右眼の完全な姿勢 T*R をcontroller 0/1としてテーブルと共有領域へ保存する
  ///
  /// @param cam カメラのインデックス（0 または 1）
  /// @param m 保存する変換行列
  ///
  static void setLocalAttitude(int cam, const GgMatrix& m);

  ///
  /// リモートのカメラのトラッキング情報を遅延させて取り出す
  ///
  /// @param cam カメラのインデックス（0 または 1）
  /// @return 遅延させたリモートの変換行列
  ///
  static const GgMatrix& getRemoteAttitude(int cam);

  ///
  /// Leap Motionサービスへ接続し、関節姿勢を更新するポーリングスレッドを開始する
  ///
  static bool startLeapMotion()
  {
    // Leap Motion の listener と controller を作る
    return listener.openConnection() != nullptr;
  }

  ///
  /// ポーリングスレッドを停止してLeap Motionサービスとの接続を閉じる
  ///
  static void stopLeapMotion()
  {
    // Scene破棄後にコールバックが残らないよう、共有listenerを明示的に停止する
    listener.closeConnection();
  }

  ///
  /// シェーダを設定する
  ///
  /// @param shader 設定するシェーダ
  ///
  static void setShader(const GgSimpleShader& shader)
  {
    Scene::shader = &shader;
  }

  ///
  /// このパーツ以下のすべてのパーツを描画する
  ///
  /// @param mp 投影変換行列
  /// @param mv ビュー変換行列
  ///
  void draw(const GgMatrix& mp, const GgMatrix& mv) const;
};

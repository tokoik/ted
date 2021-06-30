//
// シーングラフ
//
#include "Scene.h"

// 標準ライブラリ
#include <fstream>

// シェーダ
const GgSimpleShader *Scene::shader{ nullptr };

// 読み込んだパーツを登録するパーツリスト
std::map<const std::string, std::unique_ptr<const GgSimpleObj>> Scene::parts;

// 共有メモリ上に置く操縦者の変換行列
std::unique_ptr<SharedMemory> localAttitude;

// 共有メモリ上に置く作業者の変換行列
std::unique_ptr<SharedMemory> remoteAttitude;

// 外部モデル変換行列のテーブルのコピー
std::vector<GgMatrix> Scene::localMatrixTable, Scene::remoteMatrixTable;

// リモートカメラの姿勢のタイミングをフレームに合わせて遅らせるためのキュー
std::queue<GgMatrix> Scene::fifo[remoteCamCount];

// Leap Motion
LeapListener Scene::listener;

// コンストラクタ
Scene::Scene(const GgSimpleObj *obj)
  : obj(obj)
{
}

// シーングラフからシーンのオブジェクトを作成するコンストラクタ
Scene::Scene(const picojson::value &v, int level)
  : obj(nullptr)
{
  read(v, level);
}

// デストラクタ
Scene::~Scene()
{
  // すべての子供のパーツを削除する
  for (const auto o : children) delete o;
}

// 共有メモリを確保して初期化する
bool Scene::initialize(const GgSimpleShader *shader, unsigned int local_size, unsigned int remote_size)
{
  // シェーダを設定する
  Scene::shader = shader;

  // ローカルの変換行列を保持する共有メモリを確保する
  localAttitude.reset(new SharedMemory(localMutexName, localShareName, local_size));

  // ローカルの変換行列を保持する共有メモリが確保できたかチェックする
  if (!localAttitude->get()) return false;

  // ローカルの変換行列を初期化する
  localAttitude->set(0, local_size, ggIdentity());

  // ローカルのモデル変換行列を確保する
  localMatrixTable.resize(local_size, ggIdentity());

  // リモートの変換行列を保持する共有メモリを確保する
  remoteAttitude.reset(new SharedMemory(remoteMutexName, remoteShareName, remote_size));

  // リモートの変換行列を保持する共有メモリが確保できたかチェックする
  if (!remoteAttitude->get()) return false;

  // リモートのの変換行列を初期化する
  remoteAttitude->set(0, remote_size, ggIdentity());

  // リモートのモデル変換行列を確保する
  remoteMatrixTable.resize(remote_size, ggIdentity());

  // 共有メモリの確保に成功した
  return true;
}

// シーングラフを読み込む
inline picojson::object Scene::load(const picojson::value& v)
{
  // v が object ならそれを返す
  if (v.is<picojson::object>()) return v.get<picojson::object>();

  // v が文字列だったら
  if (v.is<std::string>())
  {
    // v をファイル名だとしてシーングラフファイルを開く
    std::ifstream scene(v.get<std::string>());

    // 開けなかったら空のオブジェクトを返す
    if (!scene) return picojson::object{};

    // 設定ファイルを読み込む
    picojson::value f;
    scene >> f;
    scene.close();

    // 読み込んだ設定が object ならそれを返す
    if (f.is<picojson::object>()) return f.get<picojson::object>();
  }
  
  // これら以外なら空の object を返す
  return picojson::object{};
}

// シーングラフを解析する
Scene *Scene::read(const picojson::value &v, int level)
{
  // 引数ををパースする
  const auto &&o(load(v));
  if (o.empty()) return this;

  // シーンオブジェクトの変換行列
  mm = ggIdentity();

  // 共有メモリの変換行列を指定していないとき
  me = nullptr;

  // パーツの位置
  const auto &v_position(o.find("position"));
  if (v_position != o.end() && v_position->second.is<picojson::array>())
  {
    const auto &a(v_position->second.get<picojson::array>());
    GLfloat t[] = { 0.0f, 0.0f, 0.0f };
    const size_t size(sizeof t / sizeof t[0]);
    const size_t count(a.size() < size ? a.size() : size);
    for (size_t i = 0; i < count; ++i) t[i] = static_cast<GLfloat>(a[i].get<double>());
    mm *= ggTranslate(t);
  }

  // パーツの回転
  const auto &v_rotation(o.find("rotation"));
  if (v_rotation != o.end() && v_rotation->second.is<picojson::array>())
  {
    const auto &a(v_rotation->second.get<picojson::array>());
    GLfloat r[] = { 1.0f, 0.0f, 0.0f, 0.0f };
    const size_t size(sizeof r / sizeof r[0]);
    const size_t count(a.size() < size ? a.size() : size);
    for (size_t i = 0; i < count; ++i) r[i] = static_cast<GLfloat>(a[i].get<double>());
    mm *= ggRotate(r);
  }

  // パーツのスケール
  const auto &v_scale(o.find("scale"));
  if (v_scale != o.end() && v_scale->second.is<picojson::array>())
  {
    const auto &a(v_scale->second.get<picojson::array>());
    GLfloat s[] = { 1.0f, 1.0f, 1.0f };
    const size_t size(sizeof s / sizeof s[0]);
    const size_t count(a.size() < size ? a.size() : size);
    for (size_t i = 0; i < count; ++i) s[i] = static_cast<GLfloat>(a[i].get<double>());
    mm *= ggScale(s);
  }

  // 外部コントローラーによる制御
  const auto &v_controller(o.find("controller"));
  if (v_controller != o.end() && v_controller->second.is<double>())
  {
    // 引数に指定されている変換行列の番号を取り出し
    const auto i(static_cast<unsigned int>(v_controller->second.get<double>()));
    if (i >= 0 && i < localMatrixTable.size()) me = localMatrixTable.data() + i;
  }

  // 遠隔コントローラーによる制御
  const auto &v_remote_controller(o.find("remote_controller"));
  if (v_remote_controller != o.end() && v_remote_controller->second.is<double>())
  {
    // 引数に指定されている変換行列の番号を取り出し
    const auto i(static_cast<unsigned int>(v_remote_controller->second.get<double>()));
    if (i >= 0 && i < remoteMatrixTable.size()) me = remoteMatrixTable.data() + i;
  }

  // パーツの図形データ
  const auto &v_model(o.find("model"));
  if (v_model != o.end() && v_model->second.is<std::string>())
  {
    // パーツのファイル名を取り出す
    const auto &model(v_model->second.get<std::string>());

    // パーツリストに登録されていれば
    const auto &p(parts.find(model));
    if (p != parts.end())
    {
      // すでに読み込んだ図形を使う
      obj = p->second.get();
    }
    else
    {
      // ファイルから図形を読み込む
      obj = new GgSimpleObj(model.c_str());

      // パーツリストに登録する
      parts.emplace(std::make_pair(model, std::unique_ptr<const GgSimpleObj>(obj)));
    }
  }

  // シーングラフの入れ子レベルが設定値以下なら
  if (++level <= defaults.max_level)
  {
    // シーングラフに下位ノードを接続する
    const auto &v_children(o.find("children"));
    if (v_children != o.end())
    {
      if (v_children->second.is<picojson::array>())
      {
        for (const auto c : v_children->second.get<picojson::array>())
        {
          if (!c.is<picojson::null>()) addChild(new Scene(c, level));
        }
      }
      else if (v_children->second.is<std::string>())
      {
        addChild(new Scene(v_children->second, level));
      }
    }
  }

  return this;
}

// 子供にシーンを追加する
Scene *Scene::addChild(Scene *scene)
{
  children.push_back(scene);
  return scene;
}

// 子供にパーツに追加する
Scene *Scene::addChild(GgSimpleObj *obj)
{
  return addChild(new Scene(obj));
}

// ローカルとリモートの変換行列を設定する
void Scene::setup(const GgMatrix &m)
{
  // モデル変換行列を変換行列のテーブルに保存する
  localMatrixTable[camCount] = m;

#if defined(LEAP_INTERPORATE_FRAME)
  // Leap Motion と CPU の同期をとる
  listener.synchronize();
#else
  // ローカルの変換行列に Leap Motion の関節の変換行列を取得する
  listener.getHandPose(localMatrixTable.data() + camCount + 1);

  // ローカルの変換行列 (Leap Motion の関節, 視点, モデル変換行列) を共有メモリと同期する
  localAttitude->sync(localMatrixTable.data(), jointCount + camCount + 1);

  // リモートの共有メモリから変換行列を取得する
  remoteAttitude->load(remoteMatrixTable.data(), static_cast<unsigned int>(remoteMatrixTable.size()));
#endif
}

#if defined(LEAP_INTERPORATE_FRAME)
// ローカルとリモートの変換行列を更新する
void Scene::update()
{
  // ローカルの変換行列に Leap Motion の関節の変換行列を取得する
  listener.getHandPose(localMatrixTable.data() + camCount + 1);

  // ローカルの変換行列を共有メモリに保存する
  localAttitude->store(localMatrixTable.data(), static_cast<unsigned int>(localMatrixTable.size()));

  // リモートの共有メモリから変換行列を取得する
  remoteAttitude->load(remoteMatrixTable.data(), static_cast<unsigned int>(remoteMatrixTable.size()));
}
#endif

// ローカルの変換行列のテーブルに保存する
void Scene::setLocalAttitude(int cam, const GgMatrix &m)
{
  localMatrixTable[cam] = m;
  localAttitude->set(cam, m);
}

// リモートのカメラのトラッキング情報を遅延させて取り出す
const GgMatrix &Scene::getRemoteAttitude(int cam)
{
  // 新しいトラッキングデータを追加する
  fifo[cam].push(remoteMatrixTable[cam]);

  // キューの長さが遅延させるフレーム数より長ければキューを進める
  if (fifo[cam].size() > defaults.remote_delay[cam] + 1) fifo[cam].pop();

  // キューの先頭を返す
  return fifo[cam].front();
}

// このパーツ以下のすべてのパーツを描画する
void Scene::draw(const GgMatrix &mp, const GgMatrix &mv) const
{
  // このノードのモデル変換行列を累積する
  GgMatrix mw(mv * this->mm);

  // このノードが参照する共有メモリ上の変換行列を累積する
  if (me) mw *= *me;

  // このパーツが存在するとき
  if (obj)
  {
    // シェーダが設定されていれば変換行列を設定し
    if (shader) shader->loadMatrix(mp, mw);

    // このパーツを描画する
    obj->draw();
  }

  // すべての子供のパーツを描画する
  for (const auto o : children) o->draw(mp, mw);
}

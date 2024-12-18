//
// シーングラフ
//
#include "Scene.h"

// 標準ライブラリ
#include <fstream>

// ファイルマッピングオブジェクト名
constexpr LPCWSTR localMutexName{ L"TED_LOCAL_MUTEX" };
constexpr LPCWSTR localShareName{ L"TED_LOCAL_SHARE" };
constexpr LPCWSTR remoteMutexName{ L"TED_REMOTE_MUTEX" };
constexpr LPCWSTR remoteShareName{ L"TED_REMOTE_SHARE" };

// 設定
const Config* Scene::config{ nullptr };

// 共有メモリ上に置く操縦者の変換行列
std::unique_ptr<SharedMemory> Scene::localAttitude{ nullptr };

// 共有メモリ上に置く作業者の変換行列
std::unique_ptr<SharedMemory> Scene::remoteAttitude{ nullptr };

// コンストラクタ
Scene::Scene(const GgSimpleObj* obj)
  : obj(obj)
{
}

// シーングラフからシーンのオブジェクトを作成するコンストラクタ
Scene::Scene(const picojson::value& v, const GgSimpleShader* shader, int level)
  : obj(nullptr)
{
  load(v, shader, level);
}

// シーングラフからシーンのオブジェクトを作成するコンストラクタ
Scene::Scene(const picojson::value& v, const GgSimpleShader& shader, int level)
  : Scene(v, &shader, level) {}

// デストラクタ
Scene::~Scene()
{
  // すべての子供のパーツを削除する
  for (const auto o : children) delete o;
}

// 共有メモリを確保して初期化する
bool Scene::initialize(const Config& config)
{
  // 設定を取得する
  Scene::config = &config;

  // ローカルの変換行列を保持する共有メモリを確保する
  localAttitude.reset(new SharedMemory(localMutexName, localShareName, config.local_share_size));

  // ローカルの変換行列を保持する共有メモリが確保できたかチェックする
  if (!localAttitude->get()) return false;

  // ローカルの変換行列を初期化する
  localAttitude->set(0, config.local_share_size, ggIdentity());

  // ローカルのモデル変換行列を確保する
  localMatrixTable.resize(config.local_share_size, ggIdentity());

  // リモートの変換行列を保持する共有メモリを確保する
  remoteAttitude.reset(new SharedMemory(remoteMutexName, remoteShareName, config.remote_share_size));

  // リモートの変換行列を保持する共有メモリが確保できたかチェックする
  if (!remoteAttitude->get()) return false;

  // リモートのの変換行列を初期化する
  remoteAttitude->set(0, config.remote_share_size, ggIdentity());

  // リモートのモデル変換行列を確保する
  remoteMatrixTable.resize(config.remote_share_size, ggIdentity());

  // Leap Motion の listener と controller を作る
  listener.reset(new LeapListener);

  // 共有メモリの確保に成功した
  return true;
}

// シーンファイルを読み込む
picojson::object Scene::read(const std::string& file) const
{
  // 設定内容の読み込み
  std::ifstream s{ file };
  if (!s) return picojson::object{};
  picojson::value v;
  s >> v;
  s.close();

  // JSON でなければ空のオブジェクトを返す
  if (!v.is<picojson::object>()) return picojson::object{};

  // オブジェクトとして返す
  return v.get<picojson::object>();
}

// シーングラフを読み込む
Scene* Scene::load(const picojson::value& v, const GgSimpleShader* shader, int level)
{
  // 引数ををパースする
  const auto &&o
  {
    v.is<picojson::object>()      // v がオブジェクトだったら
    ? v.get<picojson::object>()   // v をオブジェクトとして返す
    : v.is<std::string>()         // v が文字列だったら
    ? read(v.get<std::string>())  // v をファイル名として JSON ファイルを読み込む
    : picojson::object{}          // それ以外は空のオブジェクトにする
  };

  // オブジェクトが空だったら何もしない
  if (o.empty()) return this;

  // シーンオブジェクトの変換行列
  mm = ggIdentity();

  // 共有メモリの変換行列を指定していないとき
  me = nullptr;

  // パーツの位置
  const auto& v_position(o.find("position"));
  if (v_position != o.end() && v_position->second.is<picojson::array>())
  {
    const auto& a(v_position->second.get<picojson::array>());
    GLfloat t[] = { 0.0f, 0.0f, 0.0f };
    const size_t size(sizeof t / sizeof t[0]);
    const size_t count(a.size() < size ? a.size() : size);
    for (size_t i = 0; i < count; ++i) t[i] = static_cast<GLfloat>(a[i].get<double>());
    mm *= ggTranslate(t);
  }

  // パーツの回転
  const auto& v_rotation(o.find("rotation"));
  if (v_rotation != o.end() && v_rotation->second.is<picojson::array>())
  {
    const auto& a(v_rotation->second.get<picojson::array>());
    GLfloat r[] = { 1.0f, 0.0f, 0.0f, 0.0f };
    const size_t size(sizeof r / sizeof r[0]);
    const size_t count(a.size() < size ? a.size() : size);
    for (size_t i = 0; i < count; ++i) r[i] = static_cast<GLfloat>(a[i].get<double>());
    mm *= ggRotate(r);
  }

  // パーツのスケール
  const auto& v_scale(o.find("scale"));
  if (v_scale != o.end() && v_scale->second.is<picojson::array>())
  {
    const auto& a(v_scale->second.get<picojson::array>());
    GLfloat s[] = { 1.0f, 1.0f, 1.0f };
    const size_t size(sizeof s / sizeof s[0]);
    const size_t count(a.size() < size ? a.size() : size);
    for (size_t i = 0; i < count; ++i) s[i] = static_cast<GLfloat>(a[i].get<double>());
    mm *= ggScale(s);
  }

  // 外部コントローラーによる制御
  const auto& v_controller(o.find("controller"));
  if (v_controller != o.end() && v_controller->second.is<double>())
  {
    // 引数に指定されている変換行列の番号を取り出し
    const auto i(static_cast<unsigned int>(v_controller->second.get<double>()));
    if (i >= 0 && i < localMatrixTable.size()) me = localMatrixTable.data() + i;
  }

  // 遠隔コントローラーによる制御
  const auto& v_remote_controller(o.find("remote_controller"));
  if (v_remote_controller != o.end() && v_remote_controller->second.is<double>())
  {
    // 引数に指定されている変換行列の番号を取り出し
    const auto i(static_cast<unsigned int>(v_remote_controller->second.get<double>()));
    if (i >= 0 && i < remoteMatrixTable.size()) me = remoteMatrixTable.data() + i;
  }

  // パーツの図形データ
  const auto& v_model(o.find("model"));
  if (v_model != o.end() && v_model->second.is<std::string>())
  {
    // パーツのファイル名を取り出す
    const auto& model(v_model->second.get<std::string>());

    // パーツリストに登録されていれば
    const auto& p(parts.find(model));
    if (p != parts.end())
    {
      // すでに読み込んだ図形を使う
      obj = p->second.get();
    }
    else
    {
      // ファイルから図形を読み込む
      obj = new GgSimpleObj(model.c_str(), shader);

      // パーツリストに登録する
      parts.emplace(std::make_pair(model, std::unique_ptr<const GgSimpleObj>(obj)));
    }
  }

  // シーングラフの入れ子レベルが設定値以下なら
  if (++level <= config->max_level)
  {
    // シーングラフに下位ノードを接続する
    const auto& v_children(o.find("children"));
    if (v_children != o.end())
    {
      if (v_children->second.is<picojson::array>())
      {
        for (const auto c : v_children->second.get<picojson::array>())
        {
          if (!c.is<picojson::null>()) addChild(new Scene(c, shader, level));
        }
      }
      else if (v_children->second.is<std::string>())
      {
        addChild(new Scene(v_children->second, shader, level));
      }
    }
  }

  return this;
}

// 子供にシーンを追加する
Scene* Scene::addChild(Scene* scene)
{
  children.push_back(scene);
  return scene;
}

// 子供にパーツに追加する
Scene* Scene::addChild(GgSimpleObj* obj)
{
  return addChild(new Scene(obj));
}

// ローカルとリモートの変換行列を設定する
void Scene::setup(const GgMatrix& m)
{
  // モデル変換行列を変換行列のテーブルに保存する
  localMatrixTable[camCount] = m;

#if defined(LEAP_INTERPORATE_FRAME)
  // Leap Motion と CPU の同期をとる
  listener->synchronize();
#else
  // ローカルの変換行列に Leap Motion の関節の変換行列を取得する
  listener->getHandPose(localMatrixTable.data() + camCount + 1);

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
  listener->getHandPose(localMatrixTable.data() + camCount + 1);

  // ローカルの変換行列を共有メモリに保存する
  localAttitude->store(localMatrixTable.data(), static_cast<unsigned int>(localMatrixTable.size()));

  // リモートの共有メモリから変換行列を取得する
  remoteAttitude->load(remoteMatrixTable.data(), static_cast<unsigned int>(remoteMatrixTable.size()));
}
#endif

// ローカルの変換行列のテーブルに保存する
void Scene::setLocalAttitude(int cam, const GgMatrix& m)
{
  localMatrixTable[cam] = m;
  localAttitude->set(cam, m);
}

// リモートのカメラのトラッキング情報を遅延させて取り出す
const GgMatrix& Scene::getRemoteAttitude(int cam)
{
  // 新しいトラッキングデータを追加する
  fifo[cam].push(remoteMatrixTable[cam]);

  // キューの長さが遅延させるフレーム数より長ければキューを進める
  if (fifo[cam].size() > config->remote_delay[cam] + 1) fifo[cam].pop();

  // キューの先頭を返す
  return fifo[cam].front();
}

// このパーツ以下のすべてのパーツを描画する
void Scene::draw(const GgMatrix& mp, const GgMatrix& mv) const
{
  // このノードのモデル変換行列を累積する
  GgMatrix mw(mv * this->mm);

  // このノードが参照する共有メモリ上の変換行列を累積する
  if (me) mw *= *me;

  // このパーツが存在するとき
  if (obj)
  {
    // シェーダが設定されていれば変換行列を設定し
    if (obj->getShader()) obj->getShader()->loadMatrix(mp, mw);

    // このパーツを描画する
    obj->draw();
  }

  // すべての子供のパーツを描画する
  for (const auto o : children) o->draw(mp, mw);
}

// 読み込んだパーツを登録するパーツリスト
std::map<const std::string, std::unique_ptr<const GgSimpleObj>> Scene::parts;

// 外部モデル変換行列のテーブルのコピー
std::vector<GgMatrix> Scene::localMatrixTable, Scene::remoteMatrixTable;

// リモートカメラの姿勢のタイミングをフレームに合わせて遅らせるためのキュー
std::queue<GgMatrix> Scene::fifo[remoteCamCount];

// Leap Motion
std::unique_ptr<LeapListener> Scene::listener;

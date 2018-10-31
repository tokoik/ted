//
// シーングラフ
//
#include "Scene.h"

// 共有メモリ
#include "SharedMemory.h"

// コンストラクタ
Scene::Scene(const GgSimpleObj *ob)
  : obj(obj)
{
}

// シーングラフからシーンのオブジェクトを作成するコンストラクタ
Scene::Scene(const picojson::value &v, const GgSimpleShader *shader, int level)
  : obj(nullptr)
{
  load(v, shader, level);
}

// シーングラフからシーンのオブジェクトを作成するコンストラクタ
Scene::Scene(const picojson::value &v, const GgSimpleShader &shader, int level)
  : Scene(v, &shader, level) {}

// デストラクタ
Scene::~Scene()
{
  // すべての子供のパーツを削除する
  for (const auto o : children) delete o;
}

// 変換行列を初期化する
void Scene::initialize()
{
  // ローカルのモデル変換行列のサイズを確保する
  localJointMatrix.resize(localAttitude->getUsed(), ggIdentity());

  // リモートのモデル変換行列のサイズはローカルと同じにしておく
  remoteJointMatrix.resize(localAttitude->getUsed());
  for (auto &m : remoteJointMatrix) remoteAttitude->push(m = ggIdentity());
}

// シーングラフを読み込む
Scene *Scene::load(const picojson::value &v, const GgSimpleShader *shader, int level)
{
  // 引数ををパースする
  const auto &o(
    v.is<std::string>() ? [&v]()
  {
    picojson::value f;
    return config::read(v.get<std::string>(), f)
      && f.is<picojson::object>() ? f.get<picojson::object>() : picojson::object();
  } ()
    : v.is<picojson::object>() ? v.get<picojson::object>() : picojson::object()
    );
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
    if (i < localAttitude->getSize()) me = localJointMatrix.data() + i;
  }

  // 遠隔コントローラーによる制御
  const auto &v_remote_controller(o.find("remote_controller"));
  if (v_remote_controller != o.end() && v_remote_controller->second.is<double>())
  {
    // 引数に指定されている変換行列の番号を取り出し
    const auto i(static_cast<unsigned int>(v_remote_controller->second.get<double>()));
    if (i < remoteAttitude->getSize()) me = remoteJointMatrix.data() + i;
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
      obj = new GgSimpleObj(model.c_str(), shader);

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

// ローカルとリモートの変換行列を共有メモリから取り出す
void Scene::setup()
{
  // ローカルの共有メモリから変換行列を取得する
  localAttitude->load(localJointMatrix.data(), 0, static_cast<unsigned int>(localJointMatrix.size()));

  // リモートの共有メモリから変換行列を取得する
  remoteAttitude->load(remoteJointMatrix.data(), 0, static_cast<unsigned int>(remoteJointMatrix.size()));
}

// リモートのカメラのトラッキング情報を遅延させて取り出す
const GgMatrix &Scene::getRemoteAttitude(int cam)
{
  // 新しいトラッキングデータを追加する
  fifo[cam].push(remoteJointMatrix[cam]);

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
    printf("%f,%f,%f,%f\n", mw.get(0), mw.get(1), mw.get(2), mw.get(3));
    printf("%f,%f,%f,%f\n", mw.get(4), mw.get(5), mw.get(6), mw.get(7));
    printf("%f,%f,%f,%f\n", mw.get(8), mw.get(9), mw.get(10), mw.get(11));
    printf("%f,%f,%f,%f\n\n", mw.get(12), mw.get(13), mw.get(14), mw.get(15));
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
std::vector<GgMatrix> Scene::localJointMatrix, Scene::remoteJointMatrix;

// リモートカメラの姿勢のタイミングをフレームに合わせて遅らせるためのキュー
std::queue<GgMatrix> Scene::fifo[remoteCamCount];

#include "config.h"
#include "Scene.h"

//
// シーングラフ
//

// デストラクタ
Scene::~Scene()
{
  // すべての子供のパーツを削除する
  for (const auto o : children) delete o;

  // このパーツの形状データを削除する
  delete obj;
}

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

  // 共有メモリの変換行列を指定していなければインデックスは ~0
  index = ~0;

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
    // 共有メモリに変換行列を追加
    index = localMatrix->push(ggIdentity());

    // 追加に成功したら
    if (~index != 0 && controller != nullptr)
    {
      // 引数に指定されている関節番号を取り出し
      const auto joint(static_cast<unsigned int>(v_controller->second.get<double>()));

      // その関節番号に追加した共有メモリ上の変換行列のインデックスを割り当てる
      controller->assign(joint, index);
    }
#if defined(_DEBUG)
    else
    {
      std::cerr << "Controller is not selected.\n";
    }
#endif
  }

  // パーツの図形データ
  const auto &v_model(o.find("model"));
  if (v_model != o.end() && v_model->second.is<std::string>())
  {
    const auto &model(v_model->second.get<std::string>());
    obj = new GgObj(model.c_str(), shader);
  }

  // シーングラフの入れ子レベルが設定値以下なら
  if (++level <= defaults.max_level)
  {
    // シーングラフに下位ノードを接続する
    const auto &v_children(o.find("children"));
    if (v_children != o.end() && v_children->second.is<picojson::array>())
    {
      for (const auto c : v_children->second.get<picojson::array>())
      {
        if (!c.is<picojson::null>()) addChild(new Scene(c, shader, level));
      }
    }
  }

  return this;
}

// このパーツ以下のすべてのパーツを描画する
void Scene::drawNode(const GgMatrix &mp, const GgMatrix &mv, const GgMatrix &mm) const
{
  // モデル変換行列を累積する
  GgMatrix mw(mm * this->mm);

  // このパーツが存在するとき
  if (obj)
  {
    // それが参照する共有メモリ上の変換行列を累積して
    if (~index) mw *= localMatrix->get(index)->get();

    // シェーダが設定されていれば変換行列を設定し
    if (obj->getShader()) obj->getShader()->loadMatrix(mp, mv * mw);

    // このパーツを描画する
    obj->draw();
  }

  // すべての子供のパーツを描画する
  for (const auto o : children) o->draw(mp, mv, mw);
}

// モデル変換行列のテーブル
SharedMemory *Scene::localMatrix(nullptr), *Scene::remoteMatrix(nullptr);

// モデル変換行列を制御するコントローラ
LeapListener *Scene::controller(nullptr);

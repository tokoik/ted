#include "config.h"
#include "Scene.h"

//
// �V�[���O���t
//

// �R���X�g���N�^
Scene::Scene(const GgObj *ob)
  : obj(obj)
{
}

// �V�[���O���t����V�[���̃I�u�W�F�N�g���쐬����R���X�g���N�^
Scene::Scene(const picojson::value &v, const GgSimpleShader *shader, int level)
  : obj(nullptr)
{
  load(v, shader, level);
}

// �V�[���O���t����V�[���̃I�u�W�F�N�g���쐬����R���X�g���N�^
Scene::Scene(const picojson::value &v, const GgSimpleShader &shader, int level)
  : Scene(v, &shader, level) {}

// �f�X�g���N�^
Scene::~Scene()
{
  // ���ׂĂ̎q���̃p�[�c���폜����
  for (const auto o : children) delete o;

  // ���̃p�[�c�̌`��f�[�^���폜����
  delete obj;
}

// ���f���ϊ��s��̃e�[�u����I������
void Scene::selectTable(SharedMemory *local, SharedMemory *remote)
{
  Scene::localMatrix = local;
  Scene::remoteMatrix = remote;
}

// ���f���ϊ��s��𐧌䂷��R���g���[����I������
void Scene::selectController(LeapListener *controller)
{
  // �R���g���[����o�^����
  Scene::controller = controller;
}

// �V�[���O���t��ǂݍ���
Scene *Scene::load(const picojson::value &v, const GgSimpleShader *shader, int level)
{
  // ���������p�[�X����
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

  // �V�[���I�u�W�F�N�g�̕ϊ��s��
  mm = ggIdentity();

  // ���L�������̕ϊ��s����w�肵�Ă��Ȃ��Ƃ�
  me = nullptr;

  // �p�[�c�̈ʒu
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

  // �p�[�c�̉�]
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

  // �p�[�c�̃X�P�[��
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

  // �O���R���g���[���[�ɂ�鐧��
  const auto &v_controller(o.find("controller"));
  if (v_controller != o.end() && v_controller->second.is<double>())
  {
    // �����Ɏw�肳��Ă���ϊ��s��̔ԍ������o��
    const auto i(static_cast<unsigned int>(v_controller->second.get<double>()));
    if (i < localMatrix->getSize()) me = localMatrix->get(i);
  }

  // ���u�R���g���[���[�ɂ�鐧��
  const auto &v_remote_controller(o.find("remote_controller"));
  if (v_remote_controller != o.end() && v_remote_controller->second.is<double>())
  {
    // �����Ɏw�肳��Ă���ϊ��s��̔ԍ������o��
    const auto i(static_cast<unsigned int>(v_remote_controller->second.get<double>()));
    if (i < remoteMatrix->getSize()) me = remoteMatrix->get(i);
  }

  // �p�[�c�̐}�`�f�[�^
  const auto &v_model(o.find("model"));
  if (v_model != o.end() && v_model->second.is<std::string>())
  {
    const auto &model(v_model->second.get<std::string>());
    obj = new GgObj(model.c_str(), shader);
  }

  // �V�[���O���t�̓���q���x�����ݒ�l�ȉ��Ȃ�
  if (++level <= defaults.max_level)
  {
    // �V�[���O���t�ɉ��ʃm�[�h��ڑ�����
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

// �q���ɃV�[����ǉ�����
Scene *Scene::addChild(Scene *scene)
{
  children.push_back(scene);
  return scene;
}

// �q���Ƀp�[�c�ɒǉ�����
Scene *Scene::addChild(GgObj *obj)
{
  return addChild(new Scene(obj));
}

// ���̃p�[�c�ȉ��̂��ׂẴp�[�c��`�悷��
void Scene::draw(const GgMatrix &mp, const GgMatrix &mv, const GgMatrix &mm) const
{
  if (localMatrix->lock())
  {
    drawNode(mp, mv, mm);
    localMatrix->unlock();
  }
}

// ���̃p�[�c�ȉ��̂��ׂẴp�[�c��`�悷��
void Scene::drawNode(const GgMatrix &mp, const GgMatrix &mv, const GgMatrix &mm) const
{
  // ���f���ϊ��s���ݐς���
  GgMatrix mw(mm * this->mm);

  // ���̃p�[�c�����݂���Ƃ�
  if (obj)
  {
    // ���ꂪ�Q�Ƃ��鋤�L��������̕ϊ��s���ݐς���
    if (me) mw *= *me;

    // �V�F�[�_���ݒ肳��Ă���Εϊ��s���ݒ肵
    if (obj->getShader()) obj->getShader()->loadMatrix(mp, mv * mw);

    // ���̃p�[�c��`�悷��
    obj->draw();
  }

  // ���ׂĂ̎q���̃p�[�c��`�悷��
  for (const auto o : children) o->draw(mp, mv, mw);
}

// ���f���ϊ��s��̃e�[�u��
SharedMemory *Scene::localMatrix(nullptr), *Scene::remoteMatrix(nullptr);

// ���f���ϊ��s��𐧌䂷��R���g���[��
LeapListener *Scene::controller(nullptr);

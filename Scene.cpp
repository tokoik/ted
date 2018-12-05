#include "config.h"
#include "Scene.h"

//
// �V�[���O���t
//

// �f�X�g���N�^
Scene::~Scene()
{
  // ���ׂĂ̎q���̃p�[�c���폜����
  for (const auto o : children) delete o;

  // ���̃p�[�c�̌`��f�[�^���폜����
  delete obj;
}

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

  // ���L�������̕ϊ��s����w�肵�Ă��Ȃ���΃C���f�b�N�X�� ~0
  index = ~0;

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
    // ���L�������ɕϊ��s���ǉ�
    index = localMatrix->push(ggIdentity());

    // �ǉ��ɐ���������
    if (~index != 0 && controller != nullptr)
    {
      // �����Ɏw�肳��Ă���֐ߔԍ������o��
      const auto joint(static_cast<unsigned int>(v_controller->second.get<double>()));

      // ���̊֐ߔԍ��ɒǉ��������L��������̕ϊ��s��̃C���f�b�N�X�����蓖�Ă�
      controller->assign(joint, index);
    }
#if defined(_DEBUG)
    else
    {
      std::cerr << "Controller is not selected.\n";
    }
#endif
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

// ���̃p�[�c�ȉ��̂��ׂẴp�[�c��`�悷��
void Scene::drawNode(const GgMatrix &mp, const GgMatrix &mv, const GgMatrix &mm) const
{
  // ���f���ϊ��s���ݐς���
  GgMatrix mw(mm * this->mm);

  // ���̃p�[�c�����݂���Ƃ�
  if (obj)
  {
    // ���ꂪ�Q�Ƃ��鋤�L��������̕ϊ��s���ݐς���
    if (~index) mw *= localMatrix->get(index)->get();

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

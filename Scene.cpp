//
// �V�[���O���t
//
#include "Scene.h"

// ���L������
#include "SharedMemory.h"

// �R���X�g���N�^
Scene::Scene(const GgSimpleObj *ob)
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
}

// �ϊ��s�������������
void Scene::initialize()
{
  // ���[�J���̃��f���ϊ��s��̃T�C�Y���m�ۂ���
  localJointMatrix.resize(localAttitude->getUsed(), ggIdentity());

  // �����[�g�̃��f���ϊ��s��̃T�C�Y�̓��[�J���Ɠ����ɂ��Ă���
  remoteJointMatrix.resize(localAttitude->getUsed());
  for (auto &m : remoteJointMatrix) remoteAttitude->push(m = ggIdentity());
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
    if (i < localAttitude->getSize()) me = localJointMatrix.data() + i;
  }

  // ���u�R���g���[���[�ɂ�鐧��
  const auto &v_remote_controller(o.find("remote_controller"));
  if (v_remote_controller != o.end() && v_remote_controller->second.is<double>())
  {
    // �����Ɏw�肳��Ă���ϊ��s��̔ԍ������o��
    const auto i(static_cast<unsigned int>(v_remote_controller->second.get<double>()));
    if (i < remoteAttitude->getSize()) me = remoteJointMatrix.data() + i;
  }

  // �p�[�c�̐}�`�f�[�^
  const auto &v_model(o.find("model"));
  if (v_model != o.end() && v_model->second.is<std::string>())
  {
    // �p�[�c�̃t�@�C���������o��
    const auto &model(v_model->second.get<std::string>());

    // �p�[�c���X�g�ɓo�^����Ă����
    const auto &p(parts.find(model));
    if (p != parts.end())
    {
      // ���łɓǂݍ��񂾐}�`���g��
      obj = p->second.get();
    }
    else
    {
      // �t�@�C������}�`��ǂݍ���
      obj = new GgSimpleObj(model.c_str(), shader);

      // �p�[�c���X�g�ɓo�^����
      parts.emplace(std::make_pair(model, std::unique_ptr<const GgSimpleObj>(obj)));
    }
  }

  // �V�[���O���t�̓���q���x�����ݒ�l�ȉ��Ȃ�
  if (++level <= defaults.max_level)
  {
    // �V�[���O���t�ɉ��ʃm�[�h��ڑ�����
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

// �q���ɃV�[����ǉ�����
Scene *Scene::addChild(Scene *scene)
{
  children.push_back(scene);
  return scene;
}

// �q���Ƀp�[�c�ɒǉ�����
Scene *Scene::addChild(GgSimpleObj *obj)
{
  return addChild(new Scene(obj));
}

// ���[�J���ƃ����[�g�̕ϊ��s������L������������o��
void Scene::setup()
{
  // ���[�J���̋��L����������ϊ��s����擾����
  localAttitude->load(localJointMatrix.data(), 0, static_cast<unsigned int>(localJointMatrix.size()));

  // �����[�g�̋��L����������ϊ��s����擾����
  remoteAttitude->load(remoteJointMatrix.data(), 0, static_cast<unsigned int>(remoteJointMatrix.size()));
}

// �����[�g�̃J�����̃g���b�L���O����x�������Ď��o��
const GgMatrix &Scene::getRemoteAttitude(int cam)
{
  // �V�����g���b�L���O�f�[�^��ǉ�����
  fifo[cam].push(remoteJointMatrix[cam]);

  // �L���[�̒������x��������t���[������蒷����΃L���[��i�߂�
  if (fifo[cam].size() > defaults.remote_delay[cam] + 1) fifo[cam].pop();

  // �L���[�̐擪��Ԃ�
  return fifo[cam].front();
}

// ���̃p�[�c�ȉ��̂��ׂẴp�[�c��`�悷��
void Scene::draw(const GgMatrix &mp, const GgMatrix &mv) const
{
  // ���̃m�[�h�̃��f���ϊ��s���ݐς���
  GgMatrix mw(mv * this->mm);

  // ���̃m�[�h���Q�Ƃ��鋤�L��������̕ϊ��s���ݐς���
  if (me) mw *= *me;

  // ���̃p�[�c�����݂���Ƃ�
  if (obj)
  {
    printf("%f,%f,%f,%f\n", mw.get(0), mw.get(1), mw.get(2), mw.get(3));
    printf("%f,%f,%f,%f\n", mw.get(4), mw.get(5), mw.get(6), mw.get(7));
    printf("%f,%f,%f,%f\n", mw.get(8), mw.get(9), mw.get(10), mw.get(11));
    printf("%f,%f,%f,%f\n\n", mw.get(12), mw.get(13), mw.get(14), mw.get(15));
    // �V�F�[�_���ݒ肳��Ă���Εϊ��s���ݒ肵
    if (obj->getShader()) obj->getShader()->loadMatrix(mp, mw);

    // ���̃p�[�c��`�悷��
    obj->draw();
  }

  // ���ׂĂ̎q���̃p�[�c��`�悷��
  for (const auto o : children) o->draw(mp, mw);
}

// �ǂݍ��񂾃p�[�c��o�^����p�[�c���X�g
std::map<const std::string, std::unique_ptr<const GgSimpleObj>> Scene::parts;

// �O�����f���ϊ��s��̃e�[�u���̃R�s�[
std::vector<GgMatrix> Scene::localJointMatrix, Scene::remoteJointMatrix;

// �����[�g�J�����̎p���̃^�C�~���O���t���[���ɍ��킹�Ēx�点�邽�߂̃L���[
std::queue<GgMatrix> Scene::fifo[remoteCamCount];

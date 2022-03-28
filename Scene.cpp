//
// �V�[���O���t
//
#include "Scene.h"

// �W�����C�u����
#include <fstream>

// �V�F�[�_
const GgSimpleShader *Scene::shader{ nullptr };

// �ǂݍ��񂾃p�[�c��o�^����p�[�c���X�g
std::map<const std::string, std::unique_ptr<const GgSimpleObj>> Scene::parts;

// ���L��������ɒu�����c�҂̕ϊ��s��
std::unique_ptr<SharedMemory> localAttitude;

// ���L��������ɒu����Ǝ҂̕ϊ��s��
std::unique_ptr<SharedMemory> remoteAttitude;

// �O�����f���ϊ��s��̃e�[�u���̃R�s�[
std::vector<GgMatrix> Scene::localMatrixTable, Scene::remoteMatrixTable;

// �����[�g�J�����̎p���̃^�C�~���O���t���[���ɍ��킹�Ēx�点�邽�߂̃L���[
std::queue<GgMatrix> Scene::fifo[remoteCamCount];

// Leap Motion
LeapListener Scene::listener;

// �R���X�g���N�^
Scene::Scene(const GgSimpleObj *obj)
  : obj(obj)
{
}

// �V�[���O���t����V�[���̃I�u�W�F�N�g���쐬����R���X�g���N�^
Scene::Scene(const picojson::value &v, int level)
  : obj(nullptr)
{
  read(v, level);
}

// �f�X�g���N�^
Scene::~Scene()
{
  // ���ׂĂ̎q���̃p�[�c���폜����
  for (const auto o : children) delete o;
}

// ���L���������m�ۂ��ď���������
bool Scene::initialize(const GgSimpleShader *shader, unsigned int local_size, unsigned int remote_size)
{
  // �V�F�[�_��ݒ肷��
  Scene::shader = shader;

  // ���[�J���̕ϊ��s���ێ����鋤�L���������m�ۂ���
  localAttitude.reset(new SharedMemory(localMutexName, localShareName, local_size));

  // ���[�J���̕ϊ��s���ێ����鋤�L���������m�ۂł������`�F�b�N����
  if (!localAttitude->get()) return false;

  // ���[�J���̕ϊ��s�������������
  localAttitude->set(0, local_size, ggIdentity());

  // ���[�J���̃��f���ϊ��s����m�ۂ���
  localMatrixTable.resize(local_size, ggIdentity());

  // �����[�g�̕ϊ��s���ێ����鋤�L���������m�ۂ���
  remoteAttitude.reset(new SharedMemory(remoteMutexName, remoteShareName, remote_size));

  // �����[�g�̕ϊ��s���ێ����鋤�L���������m�ۂł������`�F�b�N����
  if (!remoteAttitude->get()) return false;

  // �����[�g�̂̕ϊ��s�������������
  remoteAttitude->set(0, remote_size, ggIdentity());

  // �����[�g�̃��f���ϊ��s����m�ۂ���
  remoteMatrixTable.resize(remote_size, ggIdentity());

  // ���L�������̊m�ۂɐ�������
  return true;
}

// �V�[���O���t��ǂݍ���
inline picojson::object Scene::load(const picojson::value& v)
{
  // v �� object �Ȃ炻���Ԃ�
  if (v.is<picojson::object>()) return v.get<picojson::object>();

  // v �������񂾂�����
  if (v.is<std::string>())
  {
    // v ���t�@�C�������Ƃ��ăV�[���O���t�t�@�C�����J��
    std::ifstream scene(v.get<std::string>());

    // �J���Ȃ��������̃I�u�W�F�N�g��Ԃ�
    if (!scene) return picojson::object{};

    // �ݒ�t�@�C����ǂݍ���
    picojson::value f;
    scene >> f;
    scene.close();

    // �ǂݍ��񂾐ݒ肪 object �Ȃ炻���Ԃ�
    if (f.is<picojson::object>()) return f.get<picojson::object>();
  }
  
  // �����ȊO�Ȃ��� object ��Ԃ�
  return picojson::object{};
}

// �V�[���O���t����͂���
Scene *Scene::read(const picojson::value &v, int level)
{
  // ���������p�[�X����
  const auto &&o(load(v));
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
    if (i >= 0 && i < localMatrixTable.size()) me = localMatrixTable.data() + i;
  }

  // ���u�R���g���[���[�ɂ�鐧��
  const auto &v_remote_controller(o.find("remote_controller"));
  if (v_remote_controller != o.end() && v_remote_controller->second.is<double>())
  {
    // �����Ɏw�肳��Ă���ϊ��s��̔ԍ������o��
    const auto i(static_cast<unsigned int>(v_remote_controller->second.get<double>()));
    if (i >= 0 && i < remoteMatrixTable.size()) me = remoteMatrixTable.data() + i;
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
      obj = new GgSimpleObj(model.c_str());

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

// ���[�J���ƃ����[�g�̕ϊ��s���ݒ肷��
void Scene::setup(const GgMatrix &m)
{
  // ���f���ϊ��s���ϊ��s��̃e�[�u���ɕۑ�����
  localMatrixTable[camCount] = m;

#if defined(LEAP_INTERPORATE_FRAME)
  // Leap Motion �� CPU �̓������Ƃ�
  listener.synchronize();
#else
  // ���[�J���̕ϊ��s��� Leap Motion �̊֐߂̕ϊ��s����擾����
  listener.getHandPose(localMatrixTable.data() + camCount + 1);

  // ���[�J���̕ϊ��s�� (Leap Motion �̊֐�, ���_, ���f���ϊ��s��) �����L�������Ɠ�������
  localAttitude->sync(localMatrixTable.data(), jointCount + camCount + 1);

  // �����[�g�̋��L����������ϊ��s����擾����
  remoteAttitude->load(remoteMatrixTable.data(), static_cast<unsigned int>(remoteMatrixTable.size()));
#endif
}

#if defined(LEAP_INTERPORATE_FRAME)
// ���[�J���ƃ����[�g�̕ϊ��s����X�V����
void Scene::update()
{
  // ���[�J���̕ϊ��s��� Leap Motion �̊֐߂̕ϊ��s����擾����
  listener.getHandPose(localMatrixTable.data() + camCount + 1);

  // ���[�J���̕ϊ��s������L�������ɕۑ�����
  localAttitude->store(localMatrixTable.data(), static_cast<unsigned int>(localMatrixTable.size()));

  // �����[�g�̋��L����������ϊ��s����擾����
  remoteAttitude->load(remoteMatrixTable.data(), static_cast<unsigned int>(remoteMatrixTable.size()));
}
#endif

// ���[�J���̕ϊ��s��̃e�[�u���ɕۑ�����
void Scene::setLocalAttitude(int cam, const GgMatrix &m)
{
  localMatrixTable[cam] = m;
  localAttitude->set(cam, m);
}

// �����[�g�̃J�����̃g���b�L���O����x�������Ď��o��
const GgMatrix &Scene::getRemoteAttitude(int cam)
{
  // �V�����g���b�L���O�f�[�^��ǉ�����
  fifo[cam].push(remoteMatrixTable[cam]);

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
    // �V�F�[�_���ݒ肳��Ă���Εϊ��s���ݒ肵
    if (shader) shader->loadMatrix(mp, mw);

    // ���̃p�[�c��`�悷��
    obj->draw();
  }

  // ���ׂĂ̎q���̃p�[�c��`�悷��
  for (const auto o : children) o->draw(mp, mw);
}

#pragma once

//
// �V�[���O���t
//

// �⏕�v���O����
#include "gg.h"
using namespace gg;

// ���L������
#include "SharedMemory.h"

// Leap Motion
#include "LeapListener.h"

// �V�[���O���t�� JSON �ŋL�q����
#include "picojson.h"

class Scene
{
  // �q���̃p�[�c�̃��X�g
  std::vector<Scene *> children;

  // �`�悷��p�[�c
  const GgObj *obj;

  // ���̃p�[�c�̃��f���ϊ��s��
  GgMatrix mm;

  // ���̃p�[�c���Q�Ƃ���O�����f���ϊ��s��̃C���f�b�N�X
  unsigned int index;

  // �O�����f���ϊ��s��̃e�[�u��
  static SharedMemory *localMatrix, *remoteMatrix;

  // ���̍��i�𐧌䂷�� Leap Motion
  static LeapListener *controller;

  // ���̃p�[�c�ȉ��̂��ׂẴp�[�c��`�悷��
  void drawNode(const GgMatrix &mp, const GgMatrix &mv, const GgMatrix &mm) const;

public:

  // �����̕ϊ��s����w�肵�ăV�[���̃I�u�W�F�N�g���쐬����R���X�g���N�^
  Scene(unsigned int i, const GgObj *obj = nullptr)
    : obj(obj)
  {
    index = i;
  }

  // �V�K�ɕϊ��s����쐬���ăV�[���̃I�u�W�F�N�g���쐬����R���X�g���N�^
  Scene(const GgMatrix &mat = ggIdentity(), const GgObj *obj = nullptr)
    : obj(obj)
  {
    index = localMatrix->push(mat);
  }

  // �V�K�ɕϊ��s����쐬���ăV�[���̃I�u�W�F�N�g���쐬����R���X�g���N�^
  Scene(const GLfloat *mat, const GgObj *obj = nullptr)
    : Scene(GgMatrix(mat), obj)
  {
  }

  // �V�[���O���t����V�[���̃I�u�W�F�N�g���쐬����R���X�g���N�^
  Scene(const picojson::value &v, const GgSimpleShader *shader, int level = 0)
    : obj(nullptr)
  {
    load(v, shader, level);
  }
  
  // �V�[���O���t����V�[���̃I�u�W�F�N�g���쐬����R���X�g���N�^
  Scene(const picojson::value &v, const GgSimpleShader &shader, int level = 0)
    : Scene(v, &shader, level) {}

  // �f�X�g���N�^
  virtual ~Scene();

  // ���f���ϊ��s��̃e�[�u����I������
  static void selectTable(SharedMemory *local, SharedMemory *remote)
  {
    Scene::localMatrix = local;
    Scene::remoteMatrix = remote;
  }

  // ���f���ϊ��s��𐧌䂷��R���g���[����I������
  static void selectController(LeapListener *controller)
  {
    // �R���g���[����o�^����
    Scene::controller = controller;
  }

  // �V�[���O���t��ǂݍ���
  Scene *load(const picojson::value &v, const GgSimpleShader *shader, int level);

  // �q���̃p�[�c�ɃV�[���I�u�W�F�N�g��ǉ�����
  Scene *addChild(Scene *scene)
  {
    children.push_back(scene);
    return scene;
  }

  // �����̕ϊ��s����w�肵�Ďq���̃p�[�c��ǉ�����
  Scene *addChild(int i, GgObj *obj = nullptr)
  {
    return addChild(new Scene(i, obj));
  }

  // �V�K�ɕϊ��s����쐬���Ďq���̃p�[�c��ǉ�����
  Scene *addChild(const GgMatrix &mm = ggIdentity(), GgObj *obj = nullptr)
  {
    return addChild(new Scene(mm, obj));
  }

  // �V�K�ɕϊ��s����쐬���Ďq���̃p�[�c��ǉ�����
  Scene *addChild(const GLfloat *mm, GgObj *obj = nullptr)
  {
    return addChild(GgMatrix(mm), obj);
  }

  // ���̃p�[�c�ȉ��̂��ׂẴp�[�c��`�悷��
  void draw(const GgMatrix &mp = ggIdentity(),
    const GgMatrix &mv = ggIdentity(),
    const GgMatrix &mm = ggIdentity()) const
  {
    if (localMatrix->lock())
    {
      drawNode(mp, mv, mm);
      localMatrix->unlock();
    }
  }
};

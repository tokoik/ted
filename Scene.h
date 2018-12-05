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

  // ���̃p�[�c���Q�Ƃ���O�����f���ϊ��s��
  const GgMatrix *me;

  // �O�����f���ϊ��s��̃e�[�u��
  static SharedMemory *localMatrix, *remoteMatrix;

  // ���̍��i�𐧌䂷�� Leap Motion
  static LeapListener *controller;

  // ���̃p�[�c�ȉ��̂��ׂẴp�[�c��`�悷��
  void drawNode(const GgMatrix &mp, const GgMatrix &mv, const GgMatrix &mm) const;

public:

  // �R���X�g���N�^
  Scene(const GgObj *obj = nullptr)
    : obj(obj)
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

  // �q���ɃV�[����ǉ�����
  Scene *addChild(Scene *scene)
  {
    children.push_back(scene);
    return scene;
  }

  // �q���Ƀp�[�c�ɒǉ�����
  Scene *addChild(GgObj *obj = nullptr)
  {
    return addChild(new Scene(obj));
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

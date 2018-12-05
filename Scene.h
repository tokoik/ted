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

  // �����[�g�̊O�����f���ϊ��s��̃e�[�u���̃R�s�[
  static std::vector<GgMatrix> localJointMatrix, remoteJointMatrix;

  // ���̍��i�𐧌䂷�� Leap Motion
  static LeapListener *controller;

  // ���̃p�[�c�ȉ��̂��ׂẴp�[�c��`�悷��
  void drawNode(const GgMatrix &mp, const GgMatrix &mv, const GgMatrix &mm) const;

public:

  // �R���X�g���N�^
  Scene(const GgObj *obj = nullptr);

  // �V�[���O���t����V�[���̃I�u�W�F�N�g���쐬����R���X�g���N�^
  Scene(const picojson::value &v, const GgSimpleShader *shader, int level = 0);
  
  // �V�[���O���t����V�[���̃I�u�W�F�N�g���쐬����R���X�g���N�^
  Scene(const picojson::value &v, const GgSimpleShader &shader, int level = 0);

  // �f�X�g���N�^
  virtual ~Scene();

  // ���f���ϊ��s��̃e�[�u����I������
  static void selectTable(SharedMemory *local, SharedMemory *remote);

  // ���f���ϊ��s��𐧌䂷��R���g���[����I������
  static void selectController(LeapListener *controller);

  // �V�[���O���t��ǂݍ���
  Scene *load(const picojson::value &v, const GgSimpleShader *shader, int level);

  // �q���ɃV�[����ǉ�����
  Scene *addChild(Scene *scene);

  // �q���Ƀp�[�c�ɒǉ�����
  Scene *addChild(GgObj *obj = nullptr);

  // ���̃p�[�c�ȉ��̂��ׂẴp�[�c��`�悷��
  void draw(
    const GgMatrix &mp = ggIdentity(),
    const GgMatrix &mv = ggIdentity(),
    const GgMatrix &mm = ggIdentity()) const;
};

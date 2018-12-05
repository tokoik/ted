#pragma once

//
// �V�[���O���t
//

// �e��ݒ�
#include "config.h"

// �⏕�v���O����
#include "gg.h"
using namespace gg;

// Leap Motion
#include "LeapListener.h"

// ���L������
#include "SharedMemory.h"

// �V�[���O���t�� JSON �ŋL�q����
#include "picojson.h"

// �W�����C�u����
#include <memory>
#include <map>
#include <queue>

class Scene
{
  // �q���̃p�[�c�̃��X�g
  std::vector<Scene *> children;

  // �`�悷��p�[�c
  const GgObj *obj;

  // �ǂݍ��񂾃p�[�c��o�^����p�[�c���X�g
  static std::map<const std::string, std::unique_ptr<const GgObj>> parts;

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

  // �����[�g�J�����̎p���̃^�C�~���O���t���[���ɍ��킹�Ēx�点�邽�߂̃L���[
  static std::queue<GgMatrix> fifo[remoteCamCount];

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

  // ���[�J���ƃ����[�g�̕ϊ��s������L������������o��
  static void setup();

  // �����[�g�̃J�����̃g���b�L���O����x�������Ď��o��
  static const GgMatrix &getRemoteAttitude(int cam);

  // ���̃p�[�c�ȉ��̂��ׂẴp�[�c��`�悷��
  void draw(
    const GgMatrix &mp = ggIdentity(),
    const GgMatrix &mv = ggIdentity(),
    const GgMatrix &mm = ggIdentity()) const;
};

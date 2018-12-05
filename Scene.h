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

// �V�[���O���t�� JSON �ŋL�q����
#include "picojson.h"

// �W�����C�u����
#include <map>
#include <queue>
#include <memory>

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

  // �����[�g�̊O�����f���ϊ��s��̃e�[�u���̃R�s�[
  static std::vector<GgMatrix> localJointMatrix, remoteJointMatrix;

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

  // �ϊ��s�������������
  static void initialize();

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
  void draw(const GgMatrix &mp, const GgMatrix &mv) const;
};

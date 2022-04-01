#pragma once

//
// �V�[���O���t
//

// �e��ݒ�
#include "Config.h"

// Leap Motion �֘A�̏���
#include "LeapListener.h"

// ���L������
#include "SharedMemory.h"

// �W�����C�u����
#include <queue>

// ���L��������ɒu�����c�҂̕ϊ��s��
extern std::unique_ptr<SharedMemory> localAttitude;

// ���L��������ɒu����Ǝ҂̕ϊ��s��
extern std::unique_ptr<SharedMemory> remoteAttitude;

class Scene
{
  // �q���̃p�[�c�̃��X�g
  std::vector<Scene *> children;

  // �`�悷��p�[�c
  const GgSimpleObj *obj;

  // �ǂݍ��񂾃p�[�c��o�^����p�[�c���X�g
  static std::map<const std::string, std::unique_ptr<const GgSimpleObj>> parts;

  // ���̃p�[�c�̃��f���ϊ��s��
  GgMatrix mm;

  // ���̃p�[�c���Q�Ƃ���O�����f���ϊ��s��
  const GgMatrix *me;

  // �����[�g�̊O�����f���ϊ��s��̃e�[�u���̃R�s�[
  static std::vector<GgMatrix> localMatrixTable, remoteMatrixTable;

  // �����[�g�J�����̎p���̃^�C�~���O���t���[���ɍ��킹�Ēx�点�邽�߂̃L���[
  static std::queue<GgMatrix> fifo[remoteCamCount];

  // Leap Motion
  static std::unique_ptr<LeapListener> listener;

public:

  // �R���X�g���N�^
  Scene(const GgSimpleObj *obj = nullptr);

  // �V�[���O���t����V�[���̃I�u�W�F�N�g���쐬����R���X�g���N�^
  Scene(const picojson::value &v, const GgSimpleShader *shader, int level = 0);

  // �V�[���O���t����V�[���̃I�u�W�F�N�g���쐬����R���X�g���N�^
  Scene(const picojson::value &v, const GgSimpleShader &shader, int level = 0);

  // �f�X�g���N�^
  virtual ~Scene();

  // ���L���������m�ۂ��ď���������
  static bool initialize(unsigned int local_size, unsigned int remote_size);

  // �V�[���t�@�C����ǂݍ���
  bool read(const std::string& file, picojson::value& v) const;

  // �V�[���O���t��ǂݍ���
  Scene *load(const picojson::value &v, const GgSimpleShader *shader, int level);

  // �q���ɃV�[����ǉ�����
  Scene *addChild(Scene *scene);

  // �q���Ƀp�[�c�ɒǉ�����
  Scene *addChild(GgSimpleObj *obj = nullptr);

  // ���[�J���ƃ����[�g�̕ϊ��s���ݒ肷��
  static void setup(const GgMatrix &m);

#if defined(LEAP_INTERPORATE_FRAME)
  // ���[�J���ƃ����[�g�̕ϊ��s����X�V����
  static void update();
#endif

  // ���[�J���̕ϊ��s��̃e�[�u���ɕۑ�����
  static void setLocalAttitude(int cam, const GgMatrix &m);

  // �����[�g�̃J�����̃g���b�L���O����x�������Ď��o��
  static const GgMatrix &getRemoteAttitude(int cam);

  // ���̃p�[�c�ȉ��̂��ׂẴp�[�c��`�悷��
  void draw(const GgMatrix &mp, const GgMatrix &mv) const;
};

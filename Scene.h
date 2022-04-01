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

class Scene
{
  // �ݒ�
  static const Config* config;

  // ���L��������ɒu�����c�҂̕ϊ��s��
  static std::unique_ptr<SharedMemory> localAttitude;

  // ���L��������ɒu����Ǝ҂̕ϊ��s��
  static std::unique_ptr<SharedMemory> remoteAttitude;

  // �q���̃p�[�c�̃��X�g
  std::vector<Scene*> children;

  // �`�悷��p�[�c
  const GgSimpleObj* obj;

  // �ǂݍ��񂾃p�[�c��o�^����p�[�c���X�g
  static std::map<const std::string, std::unique_ptr<const GgSimpleObj>> parts;

  // ���̃p�[�c�̃��f���ϊ��s��
  GgMatrix mm;

  // ���̃p�[�c���Q�Ƃ���O�����f���ϊ��s��
  const GgMatrix* me;

  // �����[�g�̊O�����f���ϊ��s��̃e�[�u���̃R�s�[
  static std::vector<GgMatrix> localMatrixTable, remoteMatrixTable;

  // �����[�g�J�����̎p���̃^�C�~���O���t���[���ɍ��킹�Ēx�点�邽�߂̃L���[
  static std::queue<GgMatrix> fifo[remoteCamCount];

  // Leap Motion
  static std::unique_ptr<LeapListener> listener;

public:

  // �R���X�g���N�^
  Scene(const GgSimpleObj* obj = nullptr);

  // �V�[���O���t����V�[���̃I�u�W�F�N�g���쐬����R���X�g���N�^
  Scene(const picojson::value& v, const GgSimpleShader* shader, int level = 0);

  // �V�[���O���t����V�[���̃I�u�W�F�N�g���쐬����R���X�g���N�^
  Scene(const picojson::value& v, const GgSimpleShader& shader, int level = 0);

  // �f�X�g���N�^
  virtual ~Scene();

  // ���L���������m�ۂ��ď���������
  static bool initialize(const Config& config);

  // �V�[���t�@�C����ǂݍ���
  picojson::object read(const std::string& file) const;

  // �V�[���O���t��ǂݍ���
  Scene* load(const picojson::value& v, const GgSimpleShader* shader, int level);

  // �q���ɃV�[����ǉ�����
  Scene* addChild(Scene* scene);

  // �q���Ƀp�[�c�ɒǉ�����
  Scene* addChild(GgSimpleObj* obj = nullptr);

  // ���[�J���ƃ����[�g�̕ϊ��s���ݒ肷��
  static void setup(const GgMatrix& m);

#if defined(LEAP_INTERPORATE_FRAME)
  // ���[�J���ƃ����[�g�̕ϊ��s����X�V����
  static void update();
#endif


  // ���[�J���ɃR�s�[���ꂽ�����[�g�̕ϊ��s������o��
  static void loadLocalAttitude(GgMatrix* dst, unsigned int count)
  {
    localAttitude->load(dst, count);
  }

  // ���[�J���̕ϊ��s��̐���Ԃ�
  static unsigned int getLocalAttitudeSize()
  {
    return localAttitude->getSize();
  }

  // ���[�J���̃g���b�L���O����ۑ�����
  static void setLocalAttitude(int cam, const GgMatrix& m);

  // ���[�J���̎p�����������[�g�ɑ���
  static void storeRemoteAttitude(const GgMatrix* src, unsigned int count)
  {
    remoteAttitude->store(src, count);
  }

  // �����[�g�̕ϊ��s��̐���Ԃ�
  static unsigned int getRemoteAttitudeSize()
  {
    return remoteAttitude->getSize();
  }

  // �����[�g�̃g���b�L���O����x�������Ď��o��
  static const GgMatrix& getRemoteAttitude(int cam);

  // ���̃p�[�c�ȉ��̂��ׂẴp�[�c��`�悷��
  void draw(const GgMatrix& mp, const GgMatrix& mv) const;
};

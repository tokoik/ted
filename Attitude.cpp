//
// �p�����
//
#include "Attitude.h"

// �W�����C�u����
#include <fstream>

// �p���f�[�^
Attitude attitude;

// �J���������̕␳�X�e�b�v
GgQuaternion Attitude::eyeOrientationStep[2];

//
// �R���X�g���N�^
//
Attitude::Attitude()
  : position{ 0.0f, 0.0f, 0.0f, 1.0f }
  , initialPosition{ 0.0f, 0.0f, 0.0f, 1.0f }
  , parallax{ 0 }
  , initialParallax{ 0 }
  , foreAdjust{ 0, 0, 0, 0 }
  , initialForeAdjust{ 0, 0, 0, 0 }
  , backAdjust{ 0, 0, 0, 0 }
  , initialBackAdjust{ 0, 0, 0, 0 }
  , circleAdjust{ 0, 0, 0, 0 }
  , initialCircleAdjust{ 0, 0, 0, 0 }
  , offset{ 0 }
  , initialOffset{ 0 }
{
  // �J�������Ƃ̎p���̕␳�l
  for (auto &o : eyeOrientation)
  {
    o[0] = o[1] = o[2] = 0.0f;
    o[3] = 1.0f;
  }

  // �J�������Ƃ̎p���̕␳�l�̏����l
  for (auto &o : initialEyeOrientation)
  {
    o[0] = o[1] = o[2] = 0.0f;
    o[3] = 1.0f;
  }

  // �J���������̕␳�X�e�b�v
  static bool firstTime{ true };
  if (firstTime)
  {
    // �J���������̒����X�e�b�v�����߂�
    eyeOrientationStep[0].loadRotate(0.0f, 1.0f, 0.0f, 0.001f);
    eyeOrientationStep[1].loadRotate(1.0f, 0.0f, 0.0f, 0.001f);
  }
}

//
// �p���� JSON �f�[�^�̓ǂݎ��
//
bool Attitude::read(picojson::value &v)
{
  // �ݒ���e�̃p�[�X
  picojson::object &o(v.get<picojson::object>());
  if (o.empty()) return false;

  // �����ʒu
  getVector(initialPosition, o, "position");

  // �����p��
  getVector(initialOrientation, o, "orientation");

  // �J���������̕␳�l
  const auto &v_parallax_offset{ o.find("parallax_offset") };
  if (v_parallax_offset != o.end() && v_parallax_offset->second.is<picojson::array>())
  {
    picojson::array &a(v_parallax_offset->second.get<picojson::array>());
    for (int eye = 0; eye < camCount; ++eye)
    {
      std::size_t count{std::min(a.size(), initialEyeOrientation[eye].size()) };
      for (std::size_t i = 0; i < count; ++i)
        initialEyeOrientation[eye][i] = static_cast<GLfloat>(a[eye * 4 + i].get<double>());
    }
  }

  // �����̏����l
  getValue(initialParallax, o, "parallax");

  // �O�i�̏œ_�����E�c����E���S�ʒu�̏����l
  getVector(initialForeAdjust, o, "fore_intrinsic");

  // �w�i�̏œ_�����E�c����E���S�ʒu�̏����l
  getVector(initialBackAdjust, o, "back_intrinsic");

  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu�̏����l
  getVector(initialCircleAdjust, o, "circle");

  // �X�N���[���̊Ԋu�̏����l
  getValue(initialOffset, o, "offset");

  return true;
}

//
// �p���̐ݒ�t�@�C���̓ǂݍ���
//
bool Attitude::load(const std::string &file)
{
  // �ǂݍ��񂾐ݒ�t�@�C�������o���Ă���
  attitude_file = file;

  // �ݒ�t�@�C�����J��
  std::ifstream attitude(file);
  if (!attitude) return false;

  // �ݒ�t�@�C����ǂݍ���
  picojson::value v;
  attitude >> v;
  attitude.close();

  // �ݒ����͂���
  return read(v);
}

// �p���̐ݒ�t�@�C���̏�������
bool Attitude::save(const std::string &file) const
{
  // �ݒ�l��ۑ�����
  std::ofstream attitude(file);
  if (!attitude) return false;

  // �I�u�W�F�N�g
  picojson::object o;

  // �ʒu
  setVector(position, o, "position");

  // �p��
  setVector(initialOrientation, o, "orientation");

  // �O�i�ɑ΂���œ_�����ƒ��S�ʒu
  setVector(foreAdjust, o, "fore_intrinsic");

  // �w�i�ɑ΂���œ_�����ƒ��S�ʒu
  setVector(backAdjust, o, "back_intrinsic");

  // ����
  setValue(parallax, o, "parallax");

  // �J���������̕␳�l
  picojson::array e;
  for (int eye = 0; eye < camCount; ++eye)
    for (int i = 0; i < 4; ++i)
      e.push_back(picojson::value(static_cast<double>(initialEyeOrientation[eye].data()[i])));
  o.insert(std::make_pair("parallax_offset", picojson::value(e)));

  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu
  setVector(circleAdjust, o, "circle");

  // �X�N���[���̊Ԋu
  setValue(offset, o, "offset");

  // �ݒ���e���V���A���C�Y���ĕۑ�
  picojson::value v(o);
  attitude << v.serialize(true);
  attitude.close();

  return true;
}

//
// �f�X�g���N�^
//
Attitude::~Attitude()
{
}

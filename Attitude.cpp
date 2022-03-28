//
// �p�����
//
#include "Attitude.h"

// �W�����C�u����
#include <fstream>

// �p���f�[�^
Attitude attitude;

//
// �R���X�g���N�^
//
Attitude::Attitude()
  : position{ 0.0f, 0.0f, 0.0f, 1.0f }
  , initialPosition{ 0.0f, 0.0f, 0.0f, 1.0f }
  , foreIntrinsic{ 1.0f, 1.0f, 0.0f, 0.0f }
  , initialForeIntrinsic{ 1.0f, 1.0f, 0.0f, 0.0f }
  , backIntrinsic{ 1.0f, 1.0f, 0.0f, 0.0f }
  , initialBackIntrinsic{ 1.0f, 1.0f, 0.0f, 0.0f }
  , parallax{ 0.065f }
  , initialParallax{ 0.065f }
{
  // �J�������Ƃ̎p���̕␳�l�̏����l
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
    qrStep[0].loadRotate(0.0f, 1.0f, 0.0f, 0.001f);
    qrStep[1].loadRotate(1.0f, 0.0f, 0.0f, 0.001f);
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
  const auto &v_position(o.find("position"));
  if (v_position != o.end() && v_position->second.is<picojson::array>())
  {
    picojson::array &p(v_position->second.get<picojson::array>());
    for (int i = 0; i < 4; ++i) initialPosition[i] = static_cast<GLfloat>(p[i].get<double>());
  }

  // �����p��
  const auto &v_orientation(o.find("orientation"));
  if (v_orientation != o.end() && v_orientation->second.is<picojson::array>())
  {
    picojson::array &a(v_orientation->second.get<picojson::array>());
    for (int i = 0; i < 4; ++i) initialOrientation[i] = static_cast<GLfloat>(a[i].get<double>());
  }

  // �V�[���̏œ_�����E�c����E���S�ʒu�̏����l
  const auto &v_fore_intrisic(o.find("fore_intrinsic"));
  if (v_fore_intrisic != o.end() && v_fore_intrisic->second.is<picojson::array>())
  {
    picojson::array &c(v_fore_intrisic->second.get<picojson::array>());
    for (int i = 0; i < 4; ++i) initialForeIntrinsic[i] = static_cast<GLfloat>(c[i].get<double>());
  }

  // �w�i�̏œ_�����E�c����E���S�ʒu�̏����l
  const auto &v_back_intrisic(o.find("back_intrinsic"));
  if (v_back_intrisic != o.end() && v_back_intrisic->second.is<picojson::array>())
  {
    picojson::array &c(v_back_intrisic->second.get<picojson::array>());
    for (int i = 0; i < 4; ++i) initialBackIntrinsic[i] = static_cast<GLfloat>(c[i].get<double>());
  }

  // �����̏����l
  const auto &v_parallax(o.find("parallax"));
  if (v_parallax != o.end() && v_parallax->second.is<double>())
    initialParallax = static_cast<GLfloat>(v_parallax->second.get<double>());

  // �J���������̕␳�l
  const auto &v_parallax_offset(o.find("parallax_offset"));
  if (v_parallax_offset != o.end() && v_parallax_offset->second.is<picojson::array>())
  {
    picojson::array &a(v_parallax_offset->second.get<picojson::array>());
    for (int eye = 0; eye < camCount; ++eye)
    {
      GLfloat q[4];
      for (int i = 0; i < 4; ++i) q[i] = static_cast<GLfloat>(a[eye * 4 + i].get<double>());
      initialEyeOrientation[eye] = GgQuaternion(q);
    }
  }

  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu�̏����l
  const auto &v_circle(o.find("circle"));
  if (v_circle != o.end() && v_circle->second.is<picojson::array>())
  {
    picojson::array &c(v_circle->second.get<picojson::array>());
    for (int i = 0; i < 4; ++i) initialCircle[i] = static_cast<GLfloat>(c[i].get<double>());
  }

  // �X�N���[���̊Ԋu�̏����l
  const auto &v_offset(o.find("offset"));
  if (v_offset != o.end() && v_offset->second.is<double>())
    initialOffset = static_cast<GLfloat>(v_offset->second.get<double>());

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
  picojson::array p;
  for (int i = 0; i < 4; ++i) p.push_back(picojson::value(position[i]));
  o.insert(std::make_pair("position", picojson::value(p)));

  // �p��
  picojson::array a;
  for (int i = 0; i < 4; ++i) a.push_back(picojson::value(initialOrientation[i]));
  o.insert(std::make_pair("orientation", picojson::value(a)));

  // �V�[���ɑ΂���œ_�����ƒ��S�ʒu
  picojson::array f;
  for (int i = 0; i < 4; ++i) f.push_back(picojson::value(foreIntrinsic[i]));
  o.insert(std::make_pair("fore_intrinsic", picojson::value(f)));

  // �w�i�ɑ΂���œ_�����ƒ��S�ʒu
  picojson::array b;
  for (int i = 0; i < 4; ++i) f.push_back(picojson::value(backIntrinsic[i]));
  o.insert(std::make_pair("back_intrinsic", picojson::value(b)));

  // ����
  o.insert(std::make_pair("parallax", picojson::value(static_cast<double>(parallax))));

  // �J���������̕␳�l
  picojson::array e;
  for (int eye = 0; eye < camCount; ++eye)
    for (int i = 0; i < 4; ++i)
      e.push_back(picojson::value(static_cast<double>(initialEyeOrientation[eye].get()[i])));
  o.insert(std::make_pair("parallax_offset", picojson::value(e)));

  // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu
  picojson::array c;
  for (int i = 0; i < 4; ++i) c.push_back(picojson::value(static_cast<double>(circle[i])));
  o.insert(std::make_pair("circle", picojson::value(c)));

  // �X�N���[���̊Ԋu
  o.insert(std::make_pair("offset", picojson::value(static_cast<double>(offset))));

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
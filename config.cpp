//
// �����ݒ�֘A�̏���
//

// �e��ݒ�
#include "Config.h"

// Ovrvision Pro
#include "ovrvision_pro.h"

// �W�����C�u����
#include <fstream>

//
// �R���X�g���N�^
//
Config::Config()
  : camera_left{ -1 /* ��2 */ }
  , camera_left_image{ "normal_left.jpg" }
  , camera_left_movie{ "" }
  , camera_right{ -1 /* ��2 */ }
  , camera_right_image{ "normal_right.jpg" }
  , camera_right_movie{ "" }
  , camera_texture_samples{ 1271 }
  , camera_texture_repeat{ false }
  , camera_tracking{ true }
  , capture_width{ 0.0 }
  , capture_height{ 0.0 }
  , capture_fps{ 0.0 }
  , capture_codec{ '\0', '\0', '\0', '\0', '\0' }
  , circle{ 1.0f, 1.0f, 0.0f, 0.0f }
  , ovrvision_property{ OVR::OV_CAM5MP_FHD /* ��3 */ }
  , display_mode{ MONO /* ��1 */ }
  , display_secondary{ 1 }
  , display_fullscreen{ false }
  , display_width{ 960 }
  , display_height{ 540 }
  , display_aspect{ 1.0f }
  , display_center{ 0.5f }
  , display_distance{ 1.5f }
  , display_near{ 0.1f }
  , display_far{ 5.0f }
  , display_offset{ 0.0f }
  , display_zoom{ 1.0f }
  , display_focal{ 1.0f }
  , parallax{ 0.32f }
  , parallax_offset{ ggIdentityQuaternion(), ggIdentityQuaternion() }
  , vertex_shader{ "fixed.vert" }
  , fragment_shader{ "normal.frag" }
  , role{ STANDALONE /* ��4 */ }
  , port{ 0 }
  , address{ "" }
  , remote_delay{ 0, 0 }
  , remote_stabilize{ true }
  , remote_texture_reshape{ false }
  , remote_texture_width{ 640 }
  , remote_texture_height{ 480 }
  , remote_texture_quality{ 50 }
  , remote_texture_samples{ 1372 }
  , remote_fov{ 1.0, 1.0 }
  , local_share_size{ 50 }
  , remote_share_size{ 50 }
  , position{ 0.0f, 0.0f, 0.0f, 1.0f }
  , orientation{ 0.0f, 0.0f, 0.0f, 1.0f }
  , max_level{ 10 }
{
}

//
// �f�X�g���N�^
//
Config::~Config()
{
}

//
// ��1 ���̎��̐ݒ� (StereoMode)
//
//    NONE = 0,                 // �P�ᎋ
//    LINEBYLINE,               // �C���^�[���[�X�i�������j
//    TOPANDBOTTOM,             // �㉺�Q����
//    SIDEBYSIDE,               // ���E�Q����
//    QUADBUFFER,               // �N���b�h�o�b�t�@�X�e���I
//    OCULUS                    // Oculus Rift (HMD)
//

//
// ��2 �J�����ԍ��̐ݒ�
//
//    left_camera <  0 && right_camera <  0 : �摜�t�@�C�����g��
//    left_camera >= 0 && right_camera <  0 : ���J�����������g�� (�P��)
//    left_camera <  0 && right_camera >= 0 : Ovrvision Pro ���g��
//    left_camera >  0 && right_camera >  0 : ���E�̃J�������g��
//
//    ��: (left_camera == right_camera) >= 0 �ɂ��Ȃ��ł�������
//

//
// ��3 Ovrvision Pro �̃v���p�e�B (OVR::Camprop)
//
//    OV_CAM5MP_FULL = 0,	      // 2560x1920 @15fps x2
//    OV_CAM5MP_FHD,			      // 1920x1080 @30fps x2
//    OV_CAMHD_FULL,			      // 1280x960  @45fps x2
//    OV_CAMVR_FULL, 			      // 960x950   @60fps x2
//    OV_CAMVR_WIDE,			      // 1280x800  @60fps x2
//    OV_CAMVR_VGA,			        // 640x480   @90fps x2
//    OV_CAMVR_QVGA,			      // 320x240   @120fps x2
//    OV_CAM20HD_FULL,		      // 1280x960  @15fps x2 Only USB2.0 connection
//    OV_CAM20VR_VGA,           // 640x480   @30fps x2 Only USB2.0 connection
//

//
// ��4 �z�X�g�̖���
//
//    STANDALONE = 0,           // �P��
//    OPERATOR,                 // ���c��
//    WORKER                    // ��Ǝ�
//

//
// �l�̎擾
//
template <typename T>
static bool getValue(T& scalar, const picojson::object& object, const char* name)
{
    const auto& value{ object.find(name) };
    if (value == object.end() || !value->second.is<double>()) return false;
    scalar = static_cast<T>(value->second.get<double>());
    return true;
}

//
// �l�̐ݒ�
//
template <typename T>
static void setValue(const T& scalar, picojson::object& object, const char* name)
{
    object.insert(std::make_pair(name, picojson::value(static_cast<double>(scalar))));
}

//
// �x�N�g���̎擾
//
template <typename T, std::size_t U>
static bool getVector(std::array<T, U>& vector, const picojson::object& object, const char* name)
{
    const auto& value{ object.find(name) };
    if (value == object.end() || !value->second.is<picojson::array>()) return false;

    // �z������o��
    const auto& array{ value->second.get<picojson::array>() };

    // �z��̗v�f��
    const auto n{ std::min(static_cast<decltype(array.size())>(U), array.size()) };

    // �z��̂��ׂĂ̗v�f�ɂ���
    for (decltype(array.size()) i = 0; i < n; ++i)
    {
        // �v�f�����l�Ȃ�ۑ�����
        if (array[i].is<double>()) vector[i] = static_cast<T>(array[i].get<double>());
    }

    return true;
}

//
// �x�N�g���̐ݒ�
//
template <typename T, std::size_t U>
static void setVector(const std::array<T, U>& vector, picojson::object& object, const char* name)
{
    // picojson �̔z��
    picojson::array array;

    // �z��̂��ׂĂ̗v�f�ɂ���
    for (decltype(array.size()) i = 0; i < U; ++i)
    {
        // �v�f�� picojson::array �ɒǉ�����
        array.emplace_back(picojson::value(static_cast<double>(vector[i])));
    }

    // �I�u�W�F�N�g�ɒǉ�����
    object.insert(std::make_pair(name, array));
}

//
// ������̎擾
//
static bool getString(std::string& string, const picojson::object& object, const char* name)
{
    const auto& value{ object.find(name) };
    if (value == object.end() || !value->second.is<std::string>()) return false;
    string = value->second.get<std::string>();
    return true;
}

//
// ������̐ݒ�
//
static void setString(const std::string& string, picojson::object& object, const char* name)
{
    object.insert(std::make_pair(name, picojson::value(string)));
}

//
// ������̃x�N�g���̎擾
//
template <std::size_t U>
static bool getText(std::array<std::string, U>& source, const picojson::object& object,
    const char* name)
{
    const auto& v_shader{ object.find(name) };
    if (v_shader == object.end() || !v_shader->second.is<picojson::array>()) return false;

    // �z������o��
    const auto& array{ v_shader->second.get<picojson::array>() };

    // �z��̗v�f��
    const auto n{ std::min(static_cast<decltype(array.size())>(U), array.size()) };

    // �z��̂��ׂĂ̗v�f�ɂ���
    for (decltype(array.size()) i = 0; i < n; ++i)
    {
        // �v�f��������Ȃ�ۑ�����
        if (array[i].is<std::string>()) source[i] = array[i].get<std::string>();
    }

    return true;
}

//
// ������̃x�N�g���̐ݒ�
//
template <std::size_t U>
static void setText(const std::array<std::string, U>& source, picojson::object& object,
    const char* name)
{
    // picojson �̔z��
    picojson::array array;

    // �z��̂��ׂĂ̗v�f�ɂ���
    for (std::size_t i = 0; i < U; ++i)
    {
        // �v�f�� picojson::array �ɒǉ�����
        array.emplace_back(picojson::value(source[i]));
    }

    // �I�u�W�F�N�g�ɒǉ�����
    object.insert(std::make_pair(name, array));
}

//
// �ݒ�t�@�C���̓ǂݍ���
//
bool Config::load(const std::string &file)
{
  // �ݒ�t�@�C�����J��
  std::ifstream config{ file };
  if (!config) return false;

  // �ݒ�t�@�C����ǂݍ���
  picojson::value v;
  config >> v;
  config.close();

  // �ǂݍ��񂾓��e�� JSON �łȂ���Ζ߂�
  if (!v.is<picojson::object>()) return false;

  // �ݒ���e�̃p�[�X
  const auto& o{ v.get<picojson::object>() };
  if (o.empty()) return false;

  // ���ڂ̃L���v�`���f�o�C�X�̃f�o�C�X�ԍ�
  getValue(camera_left, o, "left_camera");

  // ���ڂ̃��[�r�[�t�@�C��
  getString(camera_left_movie, o, "left_movie");

  // ���ڂ̃L���v�`���f�o�C�X�s�g�p���ɕ\������Î~�摜
  getString(camera_left_image, o, "left_image");

  // �E�ڂ̃L���v�`���f�o�C�X�̃f�o�C�X�ԍ�
  getValue(camera_right, o, "right_camera");

  // �E�ڂ̃��[�r�[�t�@�C��
  getString(camera_right_movie, o, "right_movie");

  // �E�ڂ̃L���v�`���f�o�C�X�s�g�p���ɕ\������Î~�摜
  getString(camera_right_image, o, "right_image");

  // �X�N���[���̃T���v����
  getValue(camera_texture_samples, o, "screen_samples");

  // �����~���}�@�̏ꍇ�̓e�N�X�`�����J��Ԃ�
  getValue(camera_texture_repeat, o, "texture_repeat");

  // �w�b�h�g���b�L���O
  getValue(camera_tracking, o, "tracking");

  // �J�����̉��̉�f��
  getValue(capture_width, o, "capture_width");

  // �J�����̏c�̉�f��
  getValue(capture_height, o, "capture_height");

  // �J�����̃t���[�����[�g
  getValue(capture_fps, o, "capture_fps");

  // �J�����̃R�[�f�b�N
  const auto& v_capture_codec(o.find("capture_codec"));
  if (v_capture_codec != o.end() && v_capture_codec->second.is<std::string>())
  {
    // �R�[�f�b�N�̕�����
    const std::string& codec(v_capture_codec->second.get<std::string>());
    
    // �R�[�f�b�N�̕�����̒����Ɗi�[��̗v�f�� - 1 �̏������ق�
    const auto n{ std::min(codec.length(), capture_codec.size() - 1) };
    
    // �i�[��̐擪����R�[�f�b�N�̕������i�[����
    std::size_t i{ 0 };
    for (; i < n; ++i) capture_codec[i] = toupper(codec[i]);
    
    // �i�[��̎c��̗v�f�� 0 �Ŗ��߂�
    std::fill(capture_codec.begin() + i, capture_codec.end(), '\0');
  }

  // ���჌���Y�̉��̉�p
  getValue(circle[0], o, "fisheye_fov_x");

  // ���჌���Y�̏c�̉�p
  getValue(circle[1], o, "fisheye_fov_y");

  // ���჌���Y�̉��̒��S�ʒu
  getValue(circle[2], o, "fisheye_center_x");

  // ���჌���Y�̏c�̒��S�ʒu
  getValue(circle[3], o, "fisheye_center_y");

  // Ovrvision Pro �̃��[�h
  getValue(ovrvision_property, o, "ovrvision_property");

  // ���̎��̕���
  getValue(display_mode, o, "stereo");

  // �t���X�N���[���\��
  getValue(display_fullscreen, o, "fullscreen");

  // �t���X�N���[���\������f�B�X�v���C�̔ԍ�
  getValue(display_secondary, o, "use_secondary");

  // �f�B�X�v���C�̉��̉�f��
  getValue(display_width, o, "display_width");

  // �f�B�X�v���C�̏c�̉�f��
  getValue(display_height, o, "display_height");

  // �f�B�X�v���C�̏c����
  getValue(display_aspect, o, "display_aspect");

  // �f�B�X�v���C�̒��S�̍���
  getValue(display_center, o, "display_center");

  // ���_����f�B�X�v���C�܂ł̋���
  getValue(display_distance, o, "display_distance");

  // ���_����O���ʂ܂ł̋��� (�œ_����)
  getValue(display_near, o, "depth_near");

  // ���_�������ʂ܂ł̋���
  getValue(display_far, o, "depth_far");

  // ���E�̃f�B�X�v���C�̊Ԋu
  getValue(display_offset, o, "display_offset");

  // �V�[���ɑ΂���Y�[����
  getValue(display_zoom, o, "zoom");

  // �w�i�ɑ΂���œ_����
  getValue(display_focal, o, "focal");

  // ����
  getValue(parallax, o, "parallax");

  // �J���������̕␳�l
  const auto& v_parallax_offset{ o.find("parallax_offset") };
  if (v_parallax_offset != o.end() && v_parallax_offset->second.is<picojson::array>())
  {
    const picojson::array& a{ v_parallax_offset->second.get<picojson::array>() };
    for (int cam = 0; cam < camCount; ++cam)
    {
      GgVector q;
      for (int i = 0; i < 4; ++i) q[i] = static_cast<GLfloat>(a[cam * 4 + i].get<double>());
      parallax_offset[cam] = GgQuaternion(q);
    }
  }

  // �o�[�e�b�N�X�V�F�[�_�̃\�[�X�t�@�C����
  getString(vertex_shader, o, "vertex_shader");

  // �t���O�����g�V�F�[�_�̃\�[�X�t�@�C����
  getString(fragment_shader, o, "fragment_shader");

  // �z�X�g�̖���
  getValue(role, o, "role");

  // �����[�g�\���Ɏg���|�[�g
  getValue(port, o, "port");

  // �����[�g�\���̑��M��� IP �A�h���X
  getString(address, o, "host");

  // �J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  if (getValue(remote_delay[0], o, "tracking_delay")) remote_delay[1] = remote_delay[0];

  // ���J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  getValue(remote_delay[0], o, "tracking_delay_left");

  // �E�J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  getValue(remote_delay[1], o, "tracking_delay_right");

  // ���艻����
  getValue(remote_stabilize, o, "stabilize");

  // ���艻�����p�̃e�N�X�`���̍쐬
  getValue(remote_texture_reshape, o, "texture_reshape");

  // �����[�g�J�����̉��̉�f��
  getValue(remote_texture_width, o, "texture_width");

  // �����[�g�J�����̏c�̉�f��
  getValue(remote_texture_height, o, "texture_height");

  // �`���摜�̕i��
  getValue(remote_texture_quality, o, "texture_quality");

  // �����[�g�J�����̃e�N�X�`���̃T���v����
  getValue(remote_texture_samples, o, "texture_samples");

  // �����[�g�J�����̉��̉�p
  getValue(remote_fov[0], o, "remote_fov_x");

  // �����[�g�J�����̏c�̉�p
  getValue(remote_fov[1], o, "remote_fov_y");

  // ���[�J���̎p���ϊ��s��̍ő吔
  getValue(local_share_size, o, "local_share_size");

  // �����[�g�̎p���ϊ��s��̍ő吔
  getValue(remote_share_size, o, "remote_share_size");

  // �����ʒu
  getVector(position, o, "position");

  // �����p��
  getVector(orientation, o, "orientation");

  // �V�[���O���t�̍ő�̐[��
  getValue(max_level, o, "max_level");

  // ���[���h���W�ɌŒ肷��V�[���O���t
  const auto& v_scene{ o.find("scene") };
  if (v_scene != o.end()) scene = v_scene->second;

  return true;
}

//
// �ݒ�t�@�C���̏�������
//
bool Config::save(const std::string& file) const
{
  // �ݒ�l��ۑ�����
  std::ofstream config(file);
  if (!config) return false;

  // �I�u�W�F�N�g
  picojson::object o;

  // ���ڂ̃L���v�`���f�o�C�X�̃f�o�C�X�ԍ�
  setValue(camera_left, o, "left_camera");

  // ���ڂ̃��[�r�[�t�@�C��
  setString(camera_left_movie, o, "left_movie");

  // ���ڂ̃L���v�`���f�o�C�X�s�g�p���ɕ\������Î~�摜
  setString(camera_left_image, o, "left_image");

  // �E�ڂ̃L���v�`���f�o�C�X�̃f�o�C�X�ԍ�
  setValue(camera_right, o, "right_camera");

  // �E�ڂ̃��[�r�[�t�@�C��
  setString(camera_right_movie, o, "right_movie");

  // �E�ڂ̃L���v�`���f�o�C�X�s�g�p���ɕ\������Î~�摜
  setString(camera_right_image, o, "right_image");

  // �X�N���[���̃T���v����
  setValue(camera_texture_samples, o, "screen_samples");

  // �����~���}�@�̏ꍇ�̓e�N�X�`�����J��Ԃ�
  setValue(camera_texture_repeat, o, "texture_repeat");

  // �w�b�h�g���b�L���O
  setValue(camera_tracking, o, "tracking");

  // �J�����̉��̉�f��
  setValue(capture_width, o, "capture_width");

  // �J�����̏c�̉�f��
  setValue(capture_height, o, "capture_height");

  // �J�����̃t���[�����[�g
  setValue(capture_fps, o, "capture_fps");

  // �J�����̃R�[�f�b�N
  o.insert(std::make_pair("capture_codec", picojson::value(std::string(capture_codec.data(), 4))));

  // ���჌���Y�̉��̉�p
  setValue(circle[0], o, "fisheye_fov_x");

  // ���჌���Y�̏c�̉�p
  setValue(circle[1], o, "fisheye_fov_y");

  // ���჌���Y�̉��̒��S�ʒu
  setValue(circle[2], o, "fisheye_center_x");

  // ���჌���Y�̏c�̒��S�ʒu
  setValue(circle[3], o, "fisheye_center_y");

  // Ovrvision Pro �̃��[�h
  setValue(ovrvision_property, o, "ovrvision_property");

  // ���̎��̕���
  setValue(display_mode, o, "stereo");

  // �t���X�N���[���\��
  setValue(display_fullscreen, o, "fullscreen");

  // �t���X�N���[���\������f�B�X�v���C�̔ԍ�
  setValue(display_secondary, o, "use_secondary");

  // �f�B�X�v���C�̉��̉�f��
  setValue(display_width, o, "display_width");

  // �f�B�X�v���C�̏c�̉�f��
  setValue(display_height, o, "display_height");

  // �f�B�X�v���C�̏c����
  setValue(display_aspect, o, "display_aspect");

  // �f�B�X�v���C�̒��S�̍���
  setValue(display_center, o, "display_center");

  // ���_����f�B�X�v���C�܂ł̋���
  setValue(display_distance, o, "display_distance");

  // ���_����O���ʂ܂ł̋��� (�œ_����)
  setValue(display_near, o, "depth_near");

  // ���_�������ʂ܂ł̋���
  setValue(display_far, o, "depth_far");

  // ���E�̃f�B�X�v���C�̊Ԋu
  setValue(display_offset, o, "display_offset");

  // �V�[���ɑ΂���Y�[����
  setValue(display_zoom, o, "zoom");

  // �w�i�ɂɑ΂���œ_����
  setValue(display_focal, o, "focal");

  // ����
  setValue(parallax, o, "parallax");

  // �J���������̕␳�l
  picojson::array q;
  for (int cam = 0; cam < camCount; ++cam)
    for (int i = 0; i < 4; ++i) q.push_back(picojson::value(static_cast<double>(parallax_offset[cam][i])));
  o.insert(std::make_pair("parallax_offset", picojson::value(q)));

  // �o�[�e�b�N�X�V�F�[�_�̃\�[�X�t�@�C����
  setString(vertex_shader, o, "vertex_shader");

  // �t���O�����g�V�F�[�_�̃\�[�X�t�@�C����
  setString(fragment_shader, o, "fragment_shader");

  // �z�X�g�̖���
  setValue(role, o, "role");

  // �����[�g�\���Ɏg���|�[�g
  setValue(port, o, "port");

  // �����[�g�\���̑��M��� IP �A�h���X
  setString(address, o, "host");

  // ���J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  setValue(remote_delay[0], o, "tracking_delay_left");

  // �E�J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  setValue(remote_delay[1], o, "tracking_delay_right");

  // ���艻����
  setValue(remote_stabilize, o, "stabilize");

  // ���艻�����p�̃e�N�X�`���̍쐬
  setValue(remote_texture_reshape, o, "texture_reshape");

  // �����[�g�J�����̉��̉�f��
  setValue(remote_texture_width, o, "texture_width");

  // �����[�g�J�����̏c�̉�f��
  setValue(remote_texture_height, o, "texture_height");

  // �`���摜�̕i��
  setValue(remote_texture_quality, o, "texture_quality");

  // �����[�g�J�����̃e�N�X�`���̃T���v����
  setValue(remote_texture_samples, o, "texture_samples");

  // �����[�g�J�����̉��̉�p
  setValue(remote_fov[0], o, "remote_fov_x");

  // �����[�g�J�����̏c�̉�p
  setValue(remote_fov[1], o, "remote_fov_y");

  // ���[�J���̎p���ϊ��s��̍ő吔
  setValue(local_share_size, o, "local_share_size");

  // �����[�g�̎p���ϊ��s��̍ő吔
  setValue(remote_share_size, o, "remote_share_size");

  // �ʒu
  setVector(position, o, "position");

  // �p��
  setVector(orientation, o, "orientation");

  // �V�[���O���t�̍ő�̐[��
  o.insert(std::make_pair("max_level", picojson::value(static_cast<double>(max_level))));

  // ���[���h���W�ɌŒ肷��V�[���O���t
  o.insert(std::make_pair("scene", scene));

  // �ݒ���e���V���A���C�Y���ĕۑ�
  picojson::value v{ o };
  config << v.serialize(true);
  config.close();

  return true;
}

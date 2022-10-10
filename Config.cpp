//
// �����ݒ�֘A�̏���
//

// �e��ݒ�
#include "Config.h"

// Ovrvision Pro
#include "ovrvision_pro.h"

// �W�����C�u����
#include <fstream>

// �����ݒ�
Config defaults;

//
// ��1 �\���̐ݒ� (DisplayMode)
//
//    MONOCULAR = 0,            // �P�ᎋ
//    INTERLACE,                // �C���^�[���[�X�i�������j
//    TOP_AND_BOTTOM,           // �㉺�Q����
//    SIDE_BY_SIDE,             // ���E�Q����
//    QUADBUFFER,               // �N���b�h�o�b�t�@�X�e���I
//    OCULUS                    // Oculus Rift (HMD)
//

//
// ��2 ���͂̐ݒ� (InputMode)
//
//    IMAGE = 0,                // �Î~��
//    MOVIE,                    // ����
//    CAMERA,                   // Web �J����
//    OVRVISON,                 // Ovrvision Pro
//    REMOTE                    // �����[�g�� TED
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
//    INSTRUCTOR,               // �w����
//    WORKER                    // ��Ǝ�
//

//
// �R���X�g���N�^
//
Config::Config()
  : display_mode{ MONOCULAR }                 // ��ʕ\���̃��[�h (��1)
  , display_quadbuffer{ false }               // �N���b�h�o�b�t�@�X�e���I�\�����s���Ƃ� true
  , display_fullscreen{ false }               // �t���X�N���[���\�����s���Ƃ� true
  , display_secondary{ 0 }                    // �t���X�N���[���\������f�B�X�v���C�̔ԍ�
  , display_size{ 960, 540 }                  // ��ʁi�~���[�\���j�̉�f��
  , display_aspect{ 0.0f }                    // ��ʂ̏c����
  , display_center{ 0.5f }                    // ��ʂ̒��S�̍���
  , display_distance{ 1.5f }                  // ��ʂ܂ł̋���
  , display_near{ 0.1f }                      // ���_����O���ʂ܂ł̋���
  , display_far{ 5.0f }                       // ���_�������ʂ܂ł̋���
  , input_mode{ IMAGE }                      // ���̓��[�h (��2)
  , camera_id{ -1, -1 }                       // �J�����̔ԍ�
  , camera_image{ "left.jpg", "right.jpg" }   // �J�����̑���Ɏg���Î~��
  , camera_movie{ "", "" }                    // �J�����̑���Ɏg������
  , camera_texture_samples{ 1271 }            // �w�i�摜���}�b�s���O����Ƃ��̃��b�V���̕�����
  , camera_texture_repeat{ false }            // �w�i�摜���J��Ԃ��Ń}�b�s���O����Ƃ� true
  , camera_tracking{ true }                   // �w�i�摜���w�b�h�g���b�L���O�ɒǏ]������Ƃ� true
  , camera_size{ 0, 0 }                       // �J�����̉��̉�f��
  , camera_fps{ 0.0 }                         // �J�����̃t���[�����[�g
  , camera_fourcc{ '\0', '\0', '\0', '\0' }   // �J�����̂S�����R�[�f�b�N
  , camera_center_x{ 0.0 }                    // ����J�����̉��̒��S�ʒu
  , camera_center_y{ 0.0 }                    // ����J�����̉��̒��S�ʒu
  , camera_fov_x{ 1.0 }                       // ����J�����̉��̉�p
  , camera_fov_y{ 1.0 }                       // ����J�����̏c�̉�p
  , ovrvision_property{ OVR::OV_CAMVR_FULL }  // Ovrvision Pro �̐ݒ� (��3)
  , use_controller{ false }                   // �Q�[���R���g���[���̎g�p
  , use_leap_motion{ false }                  // Leap Motion �̎g�p
  , vertex_shader{ "fixed.vert" }             // �o�[�e�b�N�X�V�F�[�_�̃\�[�X�v���O����
  , fragment_shader{ "normal.frag" }          // �t���O�����g�V�F�[�_�̃\�[�X�v���O����
  , role{ STANDALONE }                        // �z�X�g�̖��� (��4)
  , port{ 0 }                                 // �ʐM�Ɏg���|�[�g�ԍ�
  , address{ "" }                             // ������ IP �A�h���X
  , remote_stabilize{ true }                  // �����̉f�������艻����Ƃ� true
  , remote_texture_reshape{ false }           // �����̉f����ό`����Ƃ� true
  , remote_delay{ 0, 0 }                      // �����̕\���ɉ�����x��
  , remote_texture_quality{ 50 }              // ���M����摜�̕i��
  , remote_texture_samples{ 1372 }            // ��M�����摜���}�b�s���O����Ƃ��̃��b�V���̕�����
  , remote_fov_x{ 1.0 }                       // �����̃����Y�̉��̉�p
  , remote_fov_y{ 1.0 }                       // �����̃����Y�̏c�̉�p
  , local_share_size{ localShareSize }        // ���M�ɗp���鋤�L�������̃u���b�N��
  , remote_share_size{ remoteShareSize }      // ��M�ɗp���鋤�L�������̃u���b�N��
  , max_level{ 10 }                           // �V�[���t�@�C���̓���q�̐[���̏��
  , scene{}                                   // �V�[���O���t
  , config_file{ "" }                         // �ݒ�t�@�C���̃t�@�C����
{
}

//
// �f�X�g���N�^
//
Config::~Config()
{
}

//
// JSON �̓ǂݎ��
//
bool Config::read(picojson::value& v)
{
  // �ݒ���e�̃p�[�X
  const auto& o{ v.get<picojson::object>() };
  if (o.empty()) return false;

  // ���̎��̕���
  getValue(display_mode, o, "stereo");

  // �N�A�b�h�o�b�t�@�X�e���I�\��
  getValue(display_quadbuffer, o, "quadbuffer");

  // �t���X�N���[���\��
  getValue(display_fullscreen, o, "fullscreen");

  // �t���X�N���[���\������f�B�X�v���C�̔ԍ�
  getValue(display_secondary, o, "use_secondary");

  // �f�B�X�v���C�̉��̉�f��
  getValue(display_size[0], o, "display_width");

  // �f�B�X�v���C�̏c�̉�f��
  getValue(display_size[1], o, "display_height");

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

  // �j���[�̓\�[�X
  getValue(input_mode, o, "input_mode");

  // ���ڂ̃L���v�`���f�o�C�X�̃f�o�C�X�ԍ�
  getValue(camera_id[camL], o, "left_camera");

  // ���ڂ̃��[�r�[�t�@�C��
  getString(camera_movie[camL], o, "left_movie");

  // ���ڂ̃L���v�`���f�o�C�X�s�g�p���ɕ\������Î~�摜
  getString(camera_image[camL], o, "left_image");

  // �E�ڂ̃L���v�`���f�o�C�X�̃f�o�C�X�ԍ�
  getValue(camera_id[camR], o, "right_camera");

  // �E�ڂ̃��[�r�[�t�@�C��
  getString(camera_movie[camR], o, "right_movie");

  // �E�ڂ̃L���v�`���f�o�C�X�s�g�p���ɕ\������Î~�摜
  getString(camera_image[camR], o, "right_image");

  // �X�N���[���̃T���v����
  getValue(camera_texture_samples, o, "screen_samples");

  // �����~���}�@�̏ꍇ�̓e�N�X�`�����J��Ԃ�
  getValue(camera_texture_repeat, o, "texture_repeat");

  // �w�b�h�g���b�L���O
  getValue(camera_tracking, o, "tracking");

  // ���艻����
  getValue(remote_stabilize, o, "stabilize");

  // �J�����̉��̉�f��
  getValue(camera_size[0], o, "capture_width");

  // �J�����̏c�̉�f��
  getValue(camera_size[1], o, "capture_height");

  // �J�����̃t���[�����[�g
  getValue(camera_fps, o, "capture_fps");

  // �J�����̃R�[�f�b�N
  const auto& v_camera_fourcc{ o.find("capture_codec") };
  if (v_camera_fourcc != o.end() && v_camera_fourcc->second.is<std::string>())
  {
    // �R�[�f�b�N�̕�����
    const auto& codec{ v_camera_fourcc->second.get<std::string>() };

    // �R�[�f�b�N�̕�����̒����Ɗi�[��̗v�f�� - 1 �̏������ق�
    const auto limit{ std::min(codec.length(), camera_fourcc.size() - 1) };

    // �i�[��̐擪����R�[�f�b�N�̕������i�[����
    std::size_t i{ 0 };
    for (; i < limit; ++i) camera_fourcc[i] = toupper(codec[i]);

    // �i�[��̎c��̗v�f�� 0 �Ŗ��߂�
    std::fill(camera_fourcc.begin() + i, camera_fourcc.end(), '\0');
  }

  // ���჌���Y�̉��̒��S�ʒu
  getValue(camera_center_x, o, "fisheye_center_x");

  // ���჌���Y�̏c�̒��S�ʒu
  getValue(camera_center_y, o, "fisheye_center_y");

  // ���჌���Y�̉��̉�p
  getValue(camera_fov_x, o, "fisheye_fov_x");

  // ���჌���Y�̏c�̉�p
  getValue(camera_fov_y, o, "fisheye_fov_y");

  // Ovrvision Pro �̃��[�h
  getValue(ovrvision_property, o, "ovrvision_property");

  // �Q�[���R���g���[���̎g�p
  getValue(use_controller, o, "leap_motion");

  // Leap Motion �̎g�p
  getValue(use_leap_motion, o, "leap_motion");

  // �o�[�e�b�N�X�V�F�[�_�̃\�[�X�t�@�C����
  getString(vertex_shader, o, "vertex_shader");

  // �t���O�����g�V�F�[�_�̃\�[�X�t�@�C����
  getString(fragment_shader, o, "fragment_shader");

  // �����[�g�\���Ɏg���|�[�g
  getValue(port, o, "port");

  // �����[�g�\���̑��M��� IP �A�h���X
  getString(address, o, "host");

  // �z�X�g�̖���
  getValue(role, o, "role");

  // �J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  if (getValue(remote_delay[0], o, "role")) remote_delay[1] = remote_delay[0];

  // ���J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  getValue(remote_delay[0], o, "tracking_delay_left");

  // �E�J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  getValue(remote_delay[1], o, "tracking_delay_right");

  // �`���摜�̕i��
  getValue(remote_texture_quality, o, "texture_quality");

  // ���艻�����i�h�[���摜�ւ̕ό`�j���s��
  getValue(remote_texture_reshape, o, "texture_reshape");

  // ���艻�����i�h�[���摜�ւ̕ό`�j�ɗp����e�N�X�`���̃T���v����
  getValue(remote_texture_samples, o, "texture_samples");

  // �����[�g�J�����̉��̉�p
  getValue(remote_fov_x, o, "remote_fov_x");

  // �����[�g�J�����̏c�̉�p
  getValue(remote_fov_y, o, "remote_fov_y");

  // ���[�J���̎p���ϊ��s��̍ő吔
  getValue(local_share_size, o, "local_share_size");

  // �����[�g�̎p���ϊ��s��̍ő吔
  getValue(remote_share_size, o, "remote_share_size");

  // �V�[���O���t�̍ő�̐[��
  getValue(max_level, o, "max_level");

  // ���[���h���W�ɌŒ肷��V�[���O���t
  const auto& v_scene{ o.find("scene") };
  if (v_scene != o.end())
    scene = v_scene->second;

  return true;
}

//
// �ݒ�t�@�C���̓ǂݍ���
//
bool Config::load(const std::string& file)
{
  // �ǂݍ��񂾐ݒ�t�@�C�������o���Ă���
  config_file = file;

  // �ݒ�t�@�C�����J��
  std::ifstream config{ file };
  if (!config) return false;

  // �ݒ�t�@�C����ǂݍ���
  picojson::value v;
  config >> v;
  config.close();

  // �ݒ����͂���
  return read(v);
}

//
// �ݒ�t�@�C���̏�������
//
bool Config::save(const std::string& file) const
{
  // �ݒ�l��ۑ�����
  std::ofstream config{ file };
  if (!config) return false;

  // �I�u�W�F�N�g
  picojson::object o;

  // ���̎��̕���
  setValue(display_mode, o, "stereo");

  // �N�A�b�h�o�b�t�@�X�e���I�\��
  setValue(display_quadbuffer, o, "quadbuffer");

  // �t���X�N���[���\��
  setValue(display_fullscreen, o, "fullscreen");

  // �t���X�N���[���\������f�B�X�v���C�̔ԍ�
  setValue(display_secondary, o, "use_secondary");

  // �f�B�X�v���C�̉��̉�f��
  setValue(display_size[0], o, "display_width");

  // �f�B�X�v���C�̏c�̉�f��
  setValue(display_size[1], o, "display_height");

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

  // ���ڂ̃L���v�`���f�o�C�X�̃f�o�C�X�ԍ�
  setValue(camera_id[camL], o, "left_camera");

  // ���ڂ̃��[�r�[�t�@�C��
  setString(camera_movie[camL], o, "left_movie");

  // ���ڂ̃L���v�`���f�o�C�X�s�g�p���ɕ\������Î~�摜
  setString(camera_image[camL], o, "left_image");

  // �E�ڂ̃L���v�`���f�o�C�X�̃f�o�C�X�ԍ�
  setValue(camera_id[camR], o, "right_camera");

  // �E�ڂ̃��[�r�[�t�@�C��
  setString(camera_movie[camL], o, "right_movie");

  // �E�ڂ̃L���v�`���f�o�C�X�s�g�p���ɕ\������Î~�摜
  setString(camera_image[camR], o, "right_image");

  // �X�N���[���̃T���v����
  setValue(camera_texture_samples, o, "screen_samples");

  // �����~���}�@�̏ꍇ�̓e�N�X�`�����J��Ԃ�
  setValue(camera_texture_repeat, o, "texture_repeat");

  // �w�b�h�g���b�L���O
  setValue(camera_tracking, o, "tracking");

  // ���艻����
  setValue(remote_stabilize, o, "stabilize");

  // �J�����̉��̉�f��
  setValue(camera_size[0], o, "capture_width");

  // �J�����̏c�̉�f��
  setValue(camera_size[1], o, "capture_height");

  // �J�����̃t���[�����[�g
  setValue(camera_fps, o, "capture_fps");

  // �J�����̃R�[�f�b�N
  setString(std::string(camera_fourcc.data(), camera_fourcc.size()), o, "capture_codec");

  // ���჌���Y�̉��̒��S�ʒu
  setValue(camera_center_x, o, "fisheye_center_x");

  // ���჌���Y�̏c�̒��S�ʒu
  setValue(camera_center_y, o, "fisheye_center_y");

  // ���჌���Y�̉��̉�p
  setValue(camera_fov_x, o, "fisheye_fov_x");

  // ���჌���Y�̏c�̉�p
  setValue(camera_fov_y, o, "fisheye_fov_y");

  // Ovrvision Pro �̃��[�h
  setValue(ovrvision_property, o, "ovrvision_property");

  // �R���g���[���̎g�p
  setValue(use_controller, o, "controller");

  // Leap Motion �̎g�p
  setValue(use_leap_motion, o, "leap_motion");

  // �o�[�e�b�N�X�V�F�[�_�̃\�[�X�t�@�C����
  setString(vertex_shader, o, "vertex_shader");

  // �t���O�����g�V�F�[�_�̃\�[�X�t�@�C����
  setString(fragment_shader, o, "fragment_shader");

  // �����[�g�\���Ɏg���|�[�g
  setValue(port, o, "port");

  // �����[�g�\���̑��M��� IP �A�h���X
  setString(address, o, "host");

  // �z�X�g�̖���
  setValue(role, o, "role");

  // ���J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  setValue(remote_delay[0], o, "tracking_delay_left");

  // �E�J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  setValue(remote_delay[1], o, "tracking_delay_right");

  // �`���摜�̕i��
  setValue(remote_texture_quality, o, "texture_quality");

  // ���艻�����i�h�[���摜�ւ̕ό`�j���s��
  setValue(remote_texture_reshape, o, "texture_reshape");

  // ���艻�����i�h�[���摜�ւ̕ό`�j�ɗp����e�N�X�`���̃T���v����
  setValue(remote_texture_samples, o, "texture_samples");

  // �����[�g�J�����̉��̉�p
  setValue(remote_fov_x, o, "remote_fov_x");

  // �����[�g�J�����̏c�̉�p
  setValue(remote_fov_y, o, "remote_fov_y");

  // ���[�J���̎p���ϊ��s��̍ő吔
  setValue(local_share_size, o, "local_share_size");

  // �����[�g�̎p���ϊ��s��̍ő吔
  setValue(remote_share_size, o, "remote_share_size");

  // �V�[���O���t�̍ő�̐[��
  setValue(max_level, o, "max_level");

  // ���[���h���W�ɌŒ肷��V�[���O���t
  o.insert(std::make_pair("scene", scene));

  // �ݒ���e���V���A���C�Y���ĕۑ�
  picojson::value v(o);
  config << v.serialize(true);
  config.close();

  return true;
}

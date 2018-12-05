//
// �����ݒ�֘A�̏���
//

// �e��ݒ�
#include "config.h"

// �W�����C�u����
#include <fstream>

// Ovrvision Pro
#if defined(APIENTRY)
#  undef APIENTRY
#endif
#include "ovrvision_pro.h"

config defaults =
{
  -1,                           // int camera_left; (��2)
  "normal_left.jpg",            // std::string camera_left_image;
  "",                           // std::string camera_left_movie;
  -1,                           // int camera_right; (��2)
  "normal_right.jpg",           // std::string camera_right_image;
  "",                           // std::string camera_right_movie;
  1271,                         // int camera_texture_samples;
  true,                         // bool camera_texture_repeat;
  true,                         // bool camera_tracking;
  true,                         // bool remote_stabilize;
  0.0,                          // double capture_width; (0 �Ȃ�J��������擾)
  0.0,                          // double capture_height; (0 �Ȃ�J��������擾)
  0.0,                          // double capture_fps; (0 �Ȃ�J��������擾)
  0.0,                          // GLfloat fisheye_center_x;
  0.0,                          // GLfloat fisheye_center_y;
  1.0,                          // GLfloat fisheye_fov_x;
  1.0,                          // GLfloat fisheye_fov_y;
  OVR::OV_CAM5MP_FHD,           // int ovrvision_propaty; (��3)
  MONO,                         // StereoMode display_mode; (��1)
  1,                            // int display_secondary;
  0.5f,                         // GLfloat display_center;
  1.5f,                         // GLfloat display_distance;
  0.1f,                         // GLfloat display_near;
  5.0f,                         // GLfloat display_far;
  1.0f,                         // GLfloat display_zoom;
  "fixed.vert",                 // std::string vertex_shader;
  "normal.frag",                // std::string fragment_shader;
  0,                            // int port;
  "",                           // std::string address;
  STANDALONE,                   // int role (��4)
  { 0, 0 },                     // int tracking_delay_left, tracking_delay_right;
  50,                           // int texture_quality;
  false,                        // bool texture_reshape;
  1271,                         // int texture_samples;
  1.0,                          // GLfloat remote_fov_x;
  1.0,                          // GLfloat remote_fov_y;
  50,                           // int local_share_size;
  50,                           // int remote_share_size;
  10                            // int max_level;
};

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
// JSON �t�@�C���̓ǂݍ���
//
bool config::read(const std::string &file, picojson::value &v)
{
  std::ifstream config(file);
  if (!config) return false;
  config >> v;
  config.close();
  return true;
}

//
// �ݒ�t�@�C���̓ǂݍ���
//
bool config::load(const std::string &file)
{
  // �ݒ���e�̓ǂݍ���
  picojson::value v;
  if (!read(file, v)) return false;

  // �ݒ���e�̃p�[�X
  const auto &o(v.get<picojson::object>());
  if (o.empty()) return false;

  // ���ڂ̃L���v�`���f�o�C�X�̃f�o�C�X�ԍ��������̓��[�r�[�t�@�C��
  const auto &v_left_camera(o.find("left_camera"));
  if (v_left_camera != o.end())
  {
    if (v_left_camera->second.is<std::string>())
      camera_left_movie = v_left_camera->second.get<std::string>();
    else if (v_left_camera->second.is<double>())
      camera_left = static_cast<int>(v_left_camera->second.get<double>());
  }

  // ���ڂ̃L���v�`���f�o�C�X�s�g�p���ɕ\������Î~�摜
  const auto &v_left_image(o.find("left_image"));
  if (v_left_image != o.end() && v_left_image->second.is<std::string>())
    camera_left_image = v_left_image->second.get<std::string>();

  // �E�ڂ̃L���v�`���f�o�C�X�̃f�o�C�X�ԍ��������̓��[�r�[�t�@�C��
  const auto &v_right_camera(o.find("right_camera"));
  if (v_right_camera != o.end())
  {
    if (v_right_camera->second.is<std::string>())
      camera_right_movie = v_right_camera->second.get<std::string>();
    else if (v_right_camera->second.is<double>())
      camera_right = static_cast<int>(v_right_camera->second.get<double>());
  }

  // �E�ڂ̃L���v�`���f�o�C�X�s�g�p���ɕ\������Î~�摜
  const auto &v_right_image(o.find("right_image"));
  if (v_right_image != o.end() && v_right_image->second.is<std::string>())
    camera_right_image = v_right_image->second.get<std::string>();

  // �X�N���[���̃T���v����
  const auto &v_screen_samples(o.find("screen_samples"));
  if (v_screen_samples != o.end() && v_screen_samples->second.is<double>())
    camera_texture_samples = static_cast<int>(v_screen_samples->second.get<double>());

  // �����~���}�@�̏ꍇ�̓e�N�X�`�����J��Ԃ�
  const auto &v_texture_repeat(o.find("texture_repeat"));
  if (v_texture_repeat != o.end() && v_texture_repeat->second.is<bool>())
    camera_texture_repeat = v_texture_repeat->second.get<bool>();

  // �w�b�h�g���b�L���O
  const auto &v_tracking(o.find("tracking"));
  if (v_tracking != o.end() && v_tracking->second.is<bool>())
    camera_tracking = v_tracking->second.get<bool>();

  // ���艻����
  const auto &v_stabilize(o.find("stabilize"));
  if (v_stabilize != o.end() && v_stabilize->second.is<bool>())
    remote_stabilize = v_stabilize->second.get<bool>();

  // �J�����̉��̉�f��
  const auto &v_capture_width(o.find("capture_width"));
  if (v_capture_width != o.end() && v_capture_width->second.is<double>())
    capture_width = v_capture_width->second.get<double>();

  // �J�����̏c�̉�f��
  const auto &v_capture_height(o.find("capture_height"));
  if (v_capture_height != o.end() && v_capture_height->second.is<double>())
    capture_height = v_capture_height->second.get<double>();

  // �J�����̃t���[�����[�g
  const auto &v_capture_fps(o.find("capture_fps"));
  if (v_capture_fps != o.end() && v_capture_fps->second.is<double>())
    capture_fps = v_capture_fps->second.get<double>();

  // ���჌���Y�̉��̒��S�ʒu
  const auto &v_fisheye_center_x(o.find("fisheye_center_x"));
  if (v_fisheye_center_x != o.end() && v_fisheye_center_x->second.is<double>())
    fisheye_center_x = static_cast<GLfloat>(v_fisheye_center_x->second.get<double>());

  // ���჌���Y�̏c�̒��S�ʒu
  const auto &v_fisheye_center_y(o.find("fisheye_center_y"));
  if (v_fisheye_center_y != o.end() && v_fisheye_center_y->second.is<double>())
    fisheye_center_y = static_cast<GLfloat>(v_fisheye_center_y->second.get<double>());

  // ���჌���Y�̉��̉�p
  const auto &v_fisheye_fov_x(o.find("fisheye_fov_x"));
  if (v_fisheye_fov_x != o.end() && v_fisheye_fov_x->second.is<double>())
    fisheye_fov_x = static_cast<GLfloat>(v_fisheye_fov_x->second.get<double>());

  // ���჌���Y�̏c�̉�p
  const auto &v_fisheye_fov_y(o.find("fisheye_fov_y"));
  if (v_fisheye_fov_y != o.end() && v_fisheye_fov_y->second.is<double>())
    fisheye_fov_y = static_cast<GLfloat>(v_fisheye_fov_y->second.get<double>());

  // Ovrvision Pro �̃��[�h
  const auto &v_ovrvision_property(o.find("ovrvision_property"));
  if (v_ovrvision_property != o.end() && v_ovrvision_property->second.is<double>())
    ovrvision_property = static_cast<int>(v_ovrvision_property->second.get<double>());

  // ���̎��̕���
  const auto &v_stereo(o.find("stereo"));
  if (v_stereo != o.end() && v_stereo->second.is<double>())
    display_mode = static_cast<int>(v_stereo->second.get<double>());

  // �Z�J���_���f�B�X�v���C�̎g�p
  const auto &v_use_secondary(o.find("use_secondary"));
  if (v_use_secondary != o.end() && v_use_secondary->second.is<double>())
    display_secondary = static_cast<int>(v_use_secondary->second.get<double>());

  // �f�B�X�v���C�̒��S�̍���
  const auto &v_display_center(o.find("display_center"));
  if (v_display_center != o.end() && v_display_center->second.is<double>())
    display_center = static_cast<GLfloat>(v_display_center->second.get<double>());

  // ���_����f�B�X�v���C�܂ł̋���
  const auto &v_display_distance(o.find("display_distance"));
  if (v_display_distance != o.end() && v_display_distance->second.is<double>())
    display_distance = static_cast<GLfloat>(v_display_distance->second.get<double>());

  // ���_����O���ʂ܂ł̋��� (�œ_����)
  const auto &v_depth_near(o.find("depth_near"));
  if (v_depth_near != o.end() && v_depth_near->second.is<double>())
    display_near = static_cast<GLfloat>(v_depth_near->second.get<double>());

  // ���_�������ʂ܂ł̋���
  const auto &v_depth_far(o.find("depth_far"));
  if (v_depth_far != o.end() && v_depth_far->second.is<double>())
    display_far = static_cast<GLfloat>(v_depth_far->second.get<double>());

  // �V�[���ɑ΂���Y�[����
  const auto &v_zoom(o.find("zoom"));
  if (v_zoom != o.end() && v_zoom->second.is<double>())
    display_zoom = static_cast<GLfloat>(v_zoom->second.get<double>());

  // �o�[�e�b�N�X�V�F�[�_�̃\�[�X�t�@�C����
  const auto &v_vertex_shader(o.find("vertex_shader"));
  if (v_vertex_shader != o.end() && v_vertex_shader->second.is<std::string>())
    vertex_shader = v_vertex_shader->second.get<std::string>();

  // �t���O�����g�V�F�[�_�̃\�[�X�t�@�C����
  const auto &v_fragment_shader(o.find("fragment_shader"));
  if (v_fragment_shader != o.end() && v_fragment_shader->second.is<std::string>())
    fragment_shader = v_fragment_shader->second.get<std::string>();

  // �����[�g�\���Ɏg���|�[�g
  const auto &v_port(o.find("port"));
  if (v_port != o.end())
    port = static_cast<int>(v_port->second.get<double>());

  // �����[�g�\���̑��M��� IP �A�h���X
  const auto &v_host(o.find("host"));
  if (v_host != o.end())
    address = v_host->second.get<std::string>();

  // �z�X�g�̖���
  const auto &v_role(o.find("role"));
  if (v_role != o.end())
    role = static_cast<int>(v_role->second.get<double>());

  // �J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  const auto &v_tracking_delay(o.find("tracking_delay"));
  if (v_tracking_delay != o.end())
    tracking_delay[0] = tracking_delay[1] = static_cast<int>(v_tracking_delay->second.get<double>());

  // ���J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  const auto &v_tracking_delay_left(o.find("tracking_delay_left"));
  if (v_tracking_delay_left != o.end())
    tracking_delay[0] = static_cast<int>(v_tracking_delay_left->second.get<double>());

  // �E�J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  const auto &v_tracking_delay_right(o.find("tracking_delay_right"));
  if (v_tracking_delay_right != o.end())
    tracking_delay[1] = static_cast<int>(v_tracking_delay_right->second.get<double>());

  // �`���摜�̕i��
  const auto &v_texture_quality(o.find("texture_quality"));
  if (v_texture_quality != o.end())
    remote_texture_quality = static_cast<int>(v_texture_quality->second.get<double>());

  // ���艻�����i�h�[���摜�ւ̕ό`�j���s��
  const auto &v_texture_reshape(o.find("texture_reshape"));
  if (v_texture_reshape != o.end() && v_texture_reshape->second.is<bool>())
    remote_texture_reshape = v_texture_reshape->second.get<bool>();

  // ���艻�����i�h�[���摜�ւ̕ό`�j�ɗp����e�N�X�`���̃T���v����
  const auto &v_texture_samples(o.find("texture_samples"));
  if (v_texture_samples != o.end())
    remote_texture_samples = static_cast<int>(v_texture_samples->second.get<double>());

  // �����[�g�J�����̉��̉�p
  const auto &v_remote_fov_x(o.find("remote_fov_x"));
  if (v_remote_fov_x != o.end() && v_remote_fov_x->second.is<double>())
    remote_fov_x = static_cast<GLfloat>(v_remote_fov_x->second.get<double>());

  // �����[�g�J�����̏c�̉�p
  const auto &v_remote_fov_y(o.find("remote_fov_y"));
  if (v_remote_fov_y != o.end() && v_remote_fov_y->second.is<double>())
    remote_fov_y = static_cast<GLfloat>(v_remote_fov_y->second.get<double>());

  // ���[�J���̎p���ϊ��s��̍ő吔
  const auto &v_local_share_size(o.find("local_share_size"));
  if (v_local_share_size != o.end())
    local_share_size = static_cast<int>(v_local_share_size->second.get<double>());

  // �����[�g�̎p���ϊ��s��̍ő吔
  const auto &v_remote_share_size(o.find("remote_share_size"));
  if (v_remote_share_size != o.end())
    remote_share_size = static_cast<int>(v_remote_share_size->second.get<double>());

  // �V�[���O���t�̍ő�̐[��
  const auto &v_max_level(o.find("max_level"));
  if (v_max_level != o.end())
    max_level = static_cast<int>(v_max_level->second.get<double>());

  // ���[���h���W�ɌŒ肷��V�[���O���t
  const auto &v_scene(o.find("scene"));
  if (v_scene != o.end())
    scene = v_scene->second;

  return true;
}

//
// �ݒ�t�@�C���̏�������
//
bool config::save(const std::string &file) const
{
  // �ݒ�l��ۑ�����
  std::ofstream config(file);
  if (!config) return false;

  // �I�u�W�F�N�g
  picojson::object o;

  // ���ڂ̃L���v�`���f�o�C�X�̃f�o�C�X�ԍ��������̓��[�r�[�t�@�C��
  o.insert(std::make_pair("left_camera", camera_left_movie.empty()
    ? picojson::value(static_cast<double>(camera_left))
    : picojson::value(camera_left_movie)));

  // ���ڂ̃L���v�`���f�o�C�X�s�g�p���ɕ\������Î~�摜
  o.insert(std::make_pair("left_image", picojson::value(camera_left_image)));

  // �E�ڂ̃L���v�`���f�o�C�X�̃f�o�C�X�ԍ��������̓��[�r�[�t�@�C��
  o.insert(std::make_pair("right_camera", camera_right_movie.empty()
    ? picojson::value(static_cast<double>(camera_right))
    : picojson::value(camera_left_movie)));

  // �E�ڂ̃L���v�`���f�o�C�X�s�g�p���ɕ\������Î~�摜
  o.insert(std::make_pair("right_image", picojson::value(camera_right_image)));

  // �X�N���[���̃T���v����
  o.insert(std::make_pair("screen_samples", picojson::value(static_cast<double>(camera_texture_samples))));

  // �����~���}�@�̏ꍇ�̓e�N�X�`�����J��Ԃ�
  o.insert(std::make_pair("texture_repeat", picojson::value(camera_texture_repeat)));

  // �w�b�h�g���b�L���O
  o.insert(std::make_pair("tracking", picojson::value(camera_tracking)));

  // ���艻����
  o.insert(std::make_pair("stabilize", picojson::value(remote_stabilize)));

  // �J�����̉��̉�f��
  o.insert(std::make_pair("capture_width", picojson::value(capture_width)));

  // �J�����̏c�̉�f��
  o.insert(std::make_pair("capture_height", picojson::value(capture_height)));

  // �J�����̃t���[�����[�g
  o.insert(std::make_pair("capture_fps", picojson::value(capture_fps)));

  // ���჌���Y�̉��̒��S�ʒu
  o.insert(std::make_pair("fisheye_center_x", picojson::value(static_cast<double>(fisheye_center_x))));

  // ���჌���Y�̏c�̒��S�ʒu
  o.insert(std::make_pair("fisheye_center_y", picojson::value(static_cast<double>(fisheye_center_y))));

  // ���჌���Y�̉��̉�p
  o.insert(std::make_pair("fisheye_fov_x", picojson::value(static_cast<double>(fisheye_fov_x))));

  // ���჌���Y�̏c�̉�p
  o.insert(std::make_pair("fisheye_fov_y", picojson::value(static_cast<double>(fisheye_fov_y))));

  // Ovrvision Pro �̃��[�h
  o.insert(std::make_pair("ovrvision_property", picojson::value(static_cast<double>(ovrvision_property))));

  // ���̎��̕���
  o.insert(std::make_pair("stereo", picojson::value(static_cast<double>(display_mode))));

  // �Z�J���_���f�B�X�v���C�̎g�p
  o.insert(std::make_pair("use_secondary", picojson::value(static_cast<double>(display_secondary))));

  // �f�B�X�v���C�̒��S�̍���
  o.insert(std::make_pair("display_center", picojson::value(static_cast<double>(display_center))));

  // ���_����f�B�X�v���C�܂ł̋���
  o.insert(std::make_pair("display_distance", picojson::value(static_cast<double>(display_distance))));

  // ���_����O���ʂ܂ł̋��� (�œ_����)
  o.insert(std::make_pair("depth_near", picojson::value(static_cast<double>(display_near))));

  // ���_�������ʂ܂ł̋���
  o.insert(std::make_pair("depth_far", picojson::value(static_cast<double>(display_far))));

  // �V�[���ɑ΂���Y�[����
  o.insert(std::make_pair("zoom", picojson::value(static_cast<double>(display_zoom))));

  // �o�[�e�b�N�X�V�F�[�_�̃\�[�X�t�@�C����
  o.insert(std::make_pair("vertex_shader", picojson::value(vertex_shader)));

  // �t���O�����g�V�F�[�_�̃\�[�X�t�@�C����
  o.insert(std::make_pair("fragment_shader", picojson::value(fragment_shader)));

  // �����[�g�\���Ɏg���|�[�g
  o.insert(std::make_pair("port", picojson::value(static_cast<double>(port))));

  // �����[�g�\���̑��M��� IP �A�h���X
  o.insert(std::make_pair("host", picojson::value(address)));

  // �z�X�g�̖���
  o.insert(std::make_pair("role", picojson::value(static_cast<double>(role))));

  // ���J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  o.insert(std::make_pair("tracking_delay_left", picojson::value(static_cast<double>(tracking_delay[0]))));

  // �E�J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  o.insert(std::make_pair("tracking_delay_right", picojson::value(static_cast<double>(tracking_delay[1]))));

  // �`���摜�̕i��
  o.insert(std::make_pair("texture_quality", picojson::value(static_cast<double>(remote_texture_quality))));

  // ���艻�����i�h�[���摜�ւ̕ό`�j���s��
  o.insert(std::make_pair("texture_reshape", picojson::value(remote_texture_reshape)));

  // ���艻�����i�h�[���摜�ւ̕ό`�j�ɗp����e�N�X�`���̃T���v����
  o.insert(std::make_pair("texture_samples", picojson::value(static_cast<double>(remote_texture_samples))));

  // �����[�g�J�����̉��̉�p
  o.insert(std::make_pair("remote_fov_x", picojson::value(static_cast<double>(remote_fov_x))));

  // �����[�g�J�����̏c�̉�p
  o.insert(std::make_pair("remote_fov_y", picojson::value(static_cast<double>(remote_fov_y))));

  // ���[�J���̎p���ϊ��s��̍ő吔
  o.insert(std::make_pair("local_share_size", picojson::value(static_cast<double>(local_share_size))));

  // �����[�g�̎p���ϊ��s��̍ő吔
  o.insert(std::make_pair("remote_share_size", picojson::value(static_cast<double>(remote_share_size))));

  // �V�[���O���t�̍ő�̐[��
  o.insert(std::make_pair("max_level", picojson::value(static_cast<double>(max_level))));

  // ���[���h���W�ɌŒ肷��V�[���O���t
  o.insert(std::make_pair("scene", scene));

  // �ݒ���e���V���A���C�Y���ĕۑ�
  picojson::value v(o);
  config << v.serialize(true);
  config.close();

  return true;
}

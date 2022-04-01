//
// �����ݒ�֘A�̏���
//

// �e��ݒ�
#include "Config.h"

// �W�����C�u����
#include <fstream>

// Ovrvision Pro
#include "ovrvision_pro.h"

// �R���X�g���N�^
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
  , ovrvision_property{ OVR::OV_CAM5MP_FHD /* ��3 */ }
  , display_mode{ MONO /* ��1 */}
  , display_secondary{ 1 }
  , display_fullscreen{ false }
  , display_width{ 960 }
  , display_height{ 540 }
  , display_aspect{ 1.0f }
  , display_center{ 0.5f }
  , display_distance{ 1.5f }
  , display_offset{ 0.0f }
  , display_near{ 0.1f }
  , display_far{ 5.0f }
  , display_zoom{ 1.0f }
  , display_focal{ 1.0f }
  , display_parallax{ 0.32f }
  , vertex_shader{ "fixed.vert" }
  , fragment_shader{ "normal.frag" }
  , role{ STANDALONE /* ��4 */}
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

// �f�X�g���N�^
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
// JSON �t�@�C���̓ǂݍ���
//
bool Config::load(const std::string& file)
{
  // �ݒ���e�̓ǂݍ���
  picojson::value v;
  std::ifstream config(file);
  if (!config) return false;
  config >> v;
  config.close();

  // �ݒ���e�̃p�[�X
  const auto& o(v.get<picojson::object>());
  if (o.empty()) return false;

  // ���ڂ̃L���v�`���f�o�C�X�̃f�o�C�X�ԍ��������̓��[�r�[�t�@�C��
  const auto& v_left_camera(o.find("left_camera"));
  if (v_left_camera != o.end())
  {
    if (v_left_camera->second.is<std::string>())
      camera_left_movie = v_left_camera->second.get<std::string>();
    else if (v_left_camera->second.is<double>())
      camera_left = static_cast<int>(v_left_camera->second.get<double>());
  }

  // ���ڂ̃L���v�`���f�o�C�X�s�g�p���ɕ\������Î~�摜
  const auto& v_left_image(o.find("left_image"));
  if (v_left_image != o.end() && v_left_image->second.is<std::string>())
    camera_left_image = v_left_image->second.get<std::string>();

  // �E�ڂ̃L���v�`���f�o�C�X�̃f�o�C�X�ԍ��������̓��[�r�[�t�@�C��
  const auto& v_right_camera(o.find("right_camera"));
  if (v_right_camera != o.end())
  {
    if (v_right_camera->second.is<std::string>())
      camera_right_movie = v_right_camera->second.get<std::string>();
    else if (v_right_camera->second.is<double>())
      camera_right = static_cast<int>(v_right_camera->second.get<double>());
  }

  // �E�ڂ̃L���v�`���f�o�C�X�s�g�p���ɕ\������Î~�摜
  const auto& v_right_image(o.find("right_image"));
  if (v_right_image != o.end() && v_right_image->second.is<std::string>())
    camera_right_image = v_right_image->second.get<std::string>();

  // �X�N���[���̃T���v����
  const auto& v_screen_samples(o.find("screen_samples"));
  if (v_screen_samples != o.end() && v_screen_samples->second.is<double>())
    camera_texture_samples = static_cast<int>(v_screen_samples->second.get<double>());

  // �����~���}�@�̏ꍇ�̓e�N�X�`�����J��Ԃ�
  const auto& v_texture_repeat(o.find("texture_repeat"));
  if (v_texture_repeat != o.end() && v_texture_repeat->second.is<bool>())
    camera_texture_repeat = v_texture_repeat->second.get<bool>();

  // �w�b�h�g���b�L���O
  const auto& v_tracking(o.find("tracking"));
  if (v_tracking != o.end() && v_tracking->second.is<bool>())
    camera_tracking = v_tracking->second.get<bool>();

  // �J�����̉��̉�f��
  const auto& v_capture_width(o.find("capture_width"));
  if (v_capture_width != o.end() && v_capture_width->second.is<double>())
    capture_width = v_capture_width->second.get<double>();

  // �J�����̏c�̉�f��
  const auto& v_capture_height(o.find("capture_height"));
  if (v_capture_height != o.end() && v_capture_height->second.is<double>())
    capture_height = v_capture_height->second.get<double>();

  // �J�����̃t���[�����[�g
  const auto& v_capture_fps(o.find("capture_fps"));
  if (v_capture_fps != o.end() && v_capture_fps->second.is<double>())
    capture_fps = v_capture_fps->second.get<double>();

  // �J�����̃R�[�f�b�N
  const auto& v_capture_codec(o.find("capture_codec"));
  if (v_capture_codec != o.end() && v_capture_codec->second.is<std::string>())
  {
    const std::string& codec(v_capture_codec->second.get<std::string>());
    if (codec.length() == 4)
    {
      capture_codec[0] = toupper(codec[0]);
      capture_codec[1] = toupper(codec[1]);
      capture_codec[2] = toupper(codec[2]);
      capture_codec[3] = toupper(codec[3]);
      capture_codec[4] = '\0';
    }
    else
    {
      std::fill(capture_codec.begin(), capture_codec.end(), '\0');
    }
  }

  // ���჌���Y�̉��̉�p
  const auto& v_fisheye_fov_x(o.find("fisheye_fov_x"));
  if (v_fisheye_fov_x != o.end() && v_fisheye_fov_x->second.is<double>())
    circle[0] = static_cast<GLfloat>(v_fisheye_fov_x->second.get<double>());

  // ���჌���Y�̏c�̉�p
  const auto& v_fisheye_fov_y(o.find("fisheye_fov_y"));
  if (v_fisheye_fov_y != o.end() && v_fisheye_fov_y->second.is<double>())
    circle[1] = static_cast<GLfloat>(v_fisheye_fov_y->second.get<double>());

  // ���჌���Y�̉��̒��S�ʒu
  const auto& v_fisheye_center_x(o.find("fisheye_center_x"));
  if (v_fisheye_center_x != o.end() && v_fisheye_center_x->second.is<double>())
    circle[2] = static_cast<GLfloat>(v_fisheye_center_x->second.get<double>());

  // ���჌���Y�̏c�̒��S�ʒu
  const auto& v_fisheye_center_y(o.find("fisheye_center_y"));
  if (v_fisheye_center_y != o.end() && v_fisheye_center_y->second.is<double>())
    circle[3] = static_cast<GLfloat>(v_fisheye_center_y->second.get<double>());

  // Ovrvision Pro �̃��[�h
  const auto& v_ovrvision_property(o.find("ovrvision_property"));
  if (v_ovrvision_property != o.end() && v_ovrvision_property->second.is<double>())
    ovrvision_property = static_cast<int>(v_ovrvision_property->second.get<double>());

  // ���̎��̕���
  const auto& v_stereo(o.find("stereo"));
  if (v_stereo != o.end() && v_stereo->second.is<double>())
    display_mode = static_cast<int>(v_stereo->second.get<double>());

  // �Z�J���_���f�B�X�v���C�̎g�p
  const auto& v_use_secondary(o.find("use_secondary"));
  if (v_use_secondary != o.end() && v_use_secondary->second.is<double>())
    display_secondary = static_cast<int>(v_use_secondary->second.get<double>());

  // �t���X�N���[���\��
  const auto& v_fullscreen(o.find("fullscreen"));
  if (v_fullscreen != o.end() && v_fullscreen->second.is<bool>())
    display_fullscreen = v_fullscreen->second.get<bool>();

  // �f�B�X�v���C�̉��̉�f��
  const auto& v_display_width(o.find("display_width"));
  if (v_display_width != o.end() && v_display_width->second.is<double>())
    display_width = static_cast<int>(v_display_width->second.get<double>());

  // �f�B�X�v���C�̏c�̉�f��
  const auto& v_display_height(o.find("display_height"));
  if (v_display_height != o.end() && v_display_height->second.is<double>())
    display_height = static_cast<int>(v_display_height->second.get<double>());

  // �f�B�X�v���C�̏c����
  const auto& v_display_aspect(o.find("display_aspect"));
  if (v_display_aspect != o.end() && v_display_aspect->second.is<double>())
    display_aspect = static_cast<GLfloat>(v_display_aspect->second.get<double>());

  // �f�B�X�v���C�̒��S�̍���
  const auto& v_display_center(o.find("display_center"));
  if (v_display_center != o.end() && v_display_center->second.is<double>())
    display_center = static_cast<GLfloat>(v_display_center->second.get<double>());

  // ���_����f�B�X�v���C�܂ł̋���
  const auto& v_display_distance(o.find("display_distance"));
  if (v_display_distance != o.end() && v_display_distance->second.is<double>())
    display_distance = static_cast<GLfloat>(v_display_distance->second.get<double>());

  // ���E�̃f�B�X�v���C�̊Ԋu
  const auto& v_display_offset(o.find("display_offset"));
  if (v_display_offset != o.end() && v_display_offset->second.is<double>())
    display_offset = static_cast<GLfloat>(v_display_offset->second.get<double>());

  // ���_����O���ʂ܂ł̋��� (�œ_����)
  const auto& v_depth_near(o.find("depth_near"));
  if (v_depth_near != o.end() && v_depth_near->second.is<double>())
    display_near = static_cast<GLfloat>(v_depth_near->second.get<double>());

  // ���_�������ʂ܂ł̋���
  const auto& v_depth_far(o.find("depth_far"));
  if (v_depth_far != o.end() && v_depth_far->second.is<double>())
    display_far = static_cast<GLfloat>(v_depth_far->second.get<double>());

  // �V�[���ɑ΂���Y�[����
  const auto& v_zoom(o.find("zoom"));
  if (v_zoom != o.end() && v_zoom->second.is<double>())
    display_zoom = static_cast<GLfloat>(v_zoom->second.get<double>());

  // �w�i�ɑ΂���œ_����
  const auto& v_focal(o.find("focal"));
  if (v_focal != o.end() && v_focal->second.is<double>())
    display_focal = static_cast<GLfloat>(v_focal->second.get<double>());

  // ����
  const auto& v_display_parallax(o.find("display_parallax"));
  if (v_display_parallax != o.end() && v_display_parallax->second.is<double>())
    display_parallax = static_cast<GLfloat>(v_display_parallax->second.get<double>());

  // �o�[�e�b�N�X�V�F�[�_�̃\�[�X�t�@�C����
  const auto& v_vertex_shader(o.find("vertex_shader"));
  if (v_vertex_shader != o.end() && v_vertex_shader->second.is<std::string>())
    vertex_shader = v_vertex_shader->second.get<std::string>();

  // �t���O�����g�V�F�[�_�̃\�[�X�t�@�C����
  const auto& v_fragment_shader(o.find("fragment_shader"));
  if (v_fragment_shader != o.end() && v_fragment_shader->second.is<std::string>())
    fragment_shader = v_fragment_shader->second.get<std::string>();

  // �z�X�g�̖���
  const auto& v_role(o.find("role"));
  if (v_role != o.end())
    role = static_cast<int>(v_role->second.get<double>());

  // �����[�g�\���Ɏg���|�[�g
  const auto& v_port(o.find("port"));
  if (v_port != o.end())
    port = static_cast<int>(v_port->second.get<double>());

  // �����[�g�\���̑��M��� IP �A�h���X
  const auto& v_host(o.find("host"));
  if (v_host != o.end())
    address = v_host->second.get<std::string>();

  // �J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  const auto& v_tracking_delay(o.find("tracking_delay"));
  if (v_tracking_delay != o.end())
    remote_delay[0] = remote_delay[1] = static_cast<int>(v_tracking_delay->second.get<double>());

  // ���J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  const auto& v_tracking_delay_left(o.find("tracking_delay_left"));
  if (v_tracking_delay_left != o.end())
    remote_delay[0] = static_cast<int>(v_tracking_delay_left->second.get<double>());

  // �E�J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  const auto& v_tracking_delay_right(o.find("tracking_delay_right"));
  if (v_tracking_delay_right != o.end())
    remote_delay[1] = static_cast<int>(v_tracking_delay_right->second.get<double>());

  // ���艻����
  const auto& v_stabilize(o.find("stabilize"));
  if (v_stabilize != o.end() && v_stabilize->second.is<bool>())
    remote_stabilize = v_stabilize->second.get<bool>();

  // ���艻�����p�̃e�N�X�`���̍쐬
  const auto& v_texture_reshape(o.find("texture_reshape"));
  if (v_texture_reshape != o.end() && v_texture_reshape->second.is<bool>())
    remote_texture_reshape = v_texture_reshape->second.get<bool>();

  // �����[�g�J�����̉��̉�f��
  const auto& v_texture_width(o.find("texture_width"));
  if (v_texture_width != o.end() && v_texture_width->second.is<double>())
    remote_texture_width = static_cast<int>(v_texture_width->second.get<double>());

  // �����[�g�J�����̏c�̉�f��
  const auto& v_texture_height(o.find("texture_height"));
  if (v_texture_height != o.end() && v_texture_height->second.is<double>())
    remote_texture_height = static_cast<int>(v_texture_height->second.get<double>());

  // �`���摜�̕i��
  const auto& v_texture_quality(o.find("texture_quality"));
  if (v_texture_quality != o.end())
    remote_texture_quality = static_cast<int>(v_texture_quality->second.get<double>());

  // �����[�g�J�����̃e�N�X�`���̃T���v����
  const auto& v_texture_samples(o.find("texture_samples"));
  if (v_texture_samples != o.end())
    remote_texture_samples = static_cast<int>(v_texture_samples->second.get<double>());

  // �����[�g�J�����̉��̉�p
  const auto& v_remote_fov_x(o.find("remote_fov_x"));
  if (v_remote_fov_x != o.end() && v_remote_fov_x->second.is<double>())
    remote_fov[0] = static_cast<GLfloat>(v_remote_fov_x->second.get<double>());

  // �����[�g�J�����̏c�̉�p
  const auto& v_remote_fov_y(o.find("remote_fov_y"));
  if (v_remote_fov_y != o.end() && v_remote_fov_y->second.is<double>())
    remote_fov[1] = static_cast<GLfloat>(v_remote_fov_y->second.get<double>());

  // ���[�J���̎p���ϊ��s��̍ő吔
  const auto& v_local_share_size(o.find("local_share_size"));
  if (v_local_share_size != o.end())
    local_share_size = static_cast<int>(v_local_share_size->second.get<double>());

  // �����[�g�̎p���ϊ��s��̍ő吔
  const auto& v_remote_share_size(o.find("remote_share_size"));
  if (v_remote_share_size != o.end())
    remote_share_size = static_cast<int>(v_remote_share_size->second.get<double>());

  // �����ʒu
  const auto& v_position(o.find("position"));
  if (v_position != o.end() && v_position->second.is<picojson::array>())
  {
    const picojson::array& p(v_position->second.get<picojson::array>());
    for (int i = 0; i < 4; ++i) position[i] = static_cast<GLfloat>(p[i].get<double>());
  }

  // �����p��
  const auto& v_orientation(o.find("orientation"));
  if (v_orientation != o.end() && v_orientation->second.is<picojson::array>())
  {
    const picojson::array& a(v_orientation->second.get<picojson::array>());
    for (int i = 0; i < 4; ++i) orientation[i] = static_cast<GLfloat>(a[i].get<double>());
  }

  // �V�[���O���t�̍ő�̐[��
  const auto& v_max_level(o.find("max_level"));
  if (v_max_level != o.end())
    max_level = static_cast<int>(v_max_level->second.get<double>());

  // ���[���h���W�ɌŒ肷��V�[���O���t
  const auto& v_scene(o.find("scene"));
  if (v_scene != o.end())
    scene = v_scene->second;

  return true;
}

//
// �ݒ�t�@�C���̏�������
//
bool Config::save(const std::string &file) const
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

  // �J�����̉��̉�f��
  o.insert(std::make_pair("capture_width", picojson::value(capture_width)));

  // �J�����̏c�̉�f��
  o.insert(std::make_pair("capture_height", picojson::value(capture_height)));

  // �J�����̃t���[�����[�g
  o.insert(std::make_pair("capture_fps", picojson::value(capture_fps)));

  // �J�����̃R�[�f�b�N
  o.insert(std::make_pair("capture_codec", picojson::value(std::string(capture_codec.data(), 4))));

  // ���჌���Y�̉��̉�p
  o.insert(std::make_pair("fisheye_fov_x", picojson::value(static_cast<double>(circle[0]))));

  // ���჌���Y�̏c�̉�p
  o.insert(std::make_pair("fisheye_fov_y", picojson::value(static_cast<double>(circle[1]))));

  // ���჌���Y�̉��̒��S�ʒu
  o.insert(std::make_pair("fisheye_center_x", picojson::value(static_cast<double>(circle[2]))));

  // ���჌���Y�̏c�̒��S�ʒu
  o.insert(std::make_pair("fisheye_center_y", picojson::value(static_cast<double>(circle[3]))));

  // Ovrvision Pro �̃��[�h
  o.insert(std::make_pair("ovrvision_property", picojson::value(static_cast<double>(ovrvision_property))));

  // ���̎��̕���
  o.insert(std::make_pair("stereo", picojson::value(static_cast<double>(display_mode))));

  // �Z�J���_���f�B�X�v���C�̎g�p
  o.insert(std::make_pair("use_secondary", picojson::value(static_cast<double>(display_secondary))));

  // �t���X�N���[���\��
  o.insert(std::make_pair("fullscreen", picojson::value(display_fullscreen)));

  // �f�B�X�v���C�̉��̉�f��
  o.insert(std::make_pair("display_width", picojson::value(static_cast<double>(display_width))));

  // �f�B�X�v���C�̏c�̉�f��
  o.insert(std::make_pair("display_height", picojson::value(static_cast<double>(display_height))));

  // �f�B�X�v���C�̏c����
  o.insert(std::make_pair("display_aspect", picojson::value(static_cast<double>(display_aspect))));

  // �f�B�X�v���C�̒��S�̍���
  o.insert(std::make_pair("display_center", picojson::value(static_cast<double>(display_center))));

  // ���_����f�B�X�v���C�܂ł̋���
  o.insert(std::make_pair("display_distance", picojson::value(static_cast<double>(display_distance))));

  // ���E�̃f�B�X�v���C�̊Ԋu
  o.insert(std::make_pair("display_offset", picojson::value(static_cast<double>(display_offset))));

  // ���_����O���ʂ܂ł̋��� (�œ_����)
  o.insert(std::make_pair("depth_near", picojson::value(static_cast<double>(display_near))));

  // ���_�������ʂ܂ł̋���
  o.insert(std::make_pair("depth_far", picojson::value(static_cast<double>(display_far))));

  // �V�[���ɑ΂���Y�[����
  o.insert(std::make_pair("zoom", picojson::value(static_cast<double>(display_zoom))));

  // �w�i�ɂɑ΂���œ_����
  o.insert(std::make_pair("focal", picojson::value(static_cast<double>(display_focal))));

  // ����
  o.insert(std::make_pair("display_parallax", picojson::value(static_cast<double>(display_parallax))));

  // �o�[�e�b�N�X�V�F�[�_�̃\�[�X�t�@�C����
  o.insert(std::make_pair("vertex_shader", picojson::value(vertex_shader)));

  // �t���O�����g�V�F�[�_�̃\�[�X�t�@�C����
  o.insert(std::make_pair("fragment_shader", picojson::value(fragment_shader)));

  // �z�X�g�̖���
  o.insert(std::make_pair("role", picojson::value(static_cast<double>(role))));

  // �����[�g�\���Ɏg���|�[�g
  o.insert(std::make_pair("port", picojson::value(static_cast<double>(port))));

  // �����[�g�\���̑��M��� IP �A�h���X
  o.insert(std::make_pair("host", picojson::value(address)));

  // ���J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  o.insert(std::make_pair("tracking_delay_left", picojson::value(static_cast<double>(remote_delay[0]))));

  // �E�J�����̃t���[���ɑ΂��ăg���b�L���O����x�点��t���[���̐�
  o.insert(std::make_pair("tracking_delay_right", picojson::value(static_cast<double>(remote_delay[1]))));

  // ���艻����
  o.insert(std::make_pair("stabilize", picojson::value(remote_stabilize)));

  // ���艻�����p�̃e�N�X�`���̍쐬
  o.insert(std::make_pair("texture_reshape", picojson::value(remote_texture_reshape)));

  // �����[�g�J�����̉��̉�f��
  o.insert(std::make_pair("texture_width", picojson::value(static_cast<double>(remote_texture_width))));

  // �����[�g�J�����̏c�̉�f��
  o.insert(std::make_pair("texture_height", picojson::value(static_cast<double>(remote_texture_height))));

  // �`���摜�̕i��
  o.insert(std::make_pair("texture_quality", picojson::value(static_cast<double>(remote_texture_quality))));

  // �����[�g�J�����̃e�N�X�`���̃T���v����
  o.insert(std::make_pair("texture_samples", picojson::value(static_cast<double>(remote_texture_samples))));

  // �����[�g�J�����̉��̉�p
  o.insert(std::make_pair("remote_fov_x", picojson::value(static_cast<double>(remote_fov[0]))));

  // �����[�g�J�����̏c�̉�p
  o.insert(std::make_pair("remote_fov_y", picojson::value(static_cast<double>(remote_fov[1]))));

  // ���[�J���̎p���ϊ��s��̍ő吔
  o.insert(std::make_pair("local_share_size", picojson::value(static_cast<double>(local_share_size))));

  // �����[�g�̎p���ϊ��s��̍ő吔
  o.insert(std::make_pair("remote_share_size", picojson::value(static_cast<double>(remote_share_size))));

  // �ʒu
  picojson::array p;
  for (int i = 0; i < 4; ++i) p.push_back(picojson::value(position[i]));
  o.insert(std::make_pair("position", picojson::value(p)));

  // �p��
  picojson::array a;
  for (int i = 0; i < 4; ++i) a.push_back(picojson::value(orientation.get()[i]));
  o.insert(std::make_pair("orientation", picojson::value(a)));

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

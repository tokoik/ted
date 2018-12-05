#pragma once

//
// �e��ݒ�
//

// �⏕�v���O����
#include "gg.h"
using namespace gg;

// JSON
#include "picojson.h"

// ���̎��̐ݒ�
enum StereoMode
{
  MONO = 0,                                             // �P�ᎋ
  LINEBYLINE,                                           // �C���^�[���[�X�i�������j
  TOPANDBOTTOM,                                         // �㉺�Q����
  SIDEBYSIDE,                                           // ���E�Q����
  QUADBUFFER,                                           // �N���b�h�o�b�t�@�X�e���I
  OCULUS                                                // Oculus Rift (HMD)
};

// ����
enum HostRole
{
  STANDALONE = 0,                                       // �P��
  OPERATOR,                                             // ���c��
  WORKER                                                // ��Ǝ�
};

// �ݒ�l
struct config
{
  int camera_left;
  std::string camera_left_image;
  std::string camera_left_movie;
  int camera_right;
  std::string camera_right_image;
  std::string camera_right_movie;
  int camera_texture_samples;
  bool camera_texture_repeat;
  bool camera_tracking;
  double capture_width;
  double capture_height;
  double capture_fps;
  GLfloat fisheye_center_x;
  GLfloat fisheye_center_y;
  GLfloat fisheye_fov_x;
  GLfloat fisheye_fov_y;
  int ovrvision_property;
  int display_mode;
  int display_secondary;
  bool display_fullscreen;
  int display_width;
  int display_height;
  GLfloat display_aspect;
  GLfloat display_center;
  GLfloat display_distance;
  GLfloat display_near;
  GLfloat display_far;
  GLfloat display_zoom;
  std::string vertex_shader;
  std::string fragment_shader;
  int role;
  int port;
  std::string address;
  bool remote_stabilize;
  bool remote_texture_reshape;
  unsigned int remote_delay[2];
  int remote_texture_quality;
  int remote_texture_samples;
  GLfloat remote_fov_x;
  GLfloat remote_fov_y;
  int local_share_size;
  int remote_share_size;
  int max_level;
  picojson::value scene;
  static bool read(const std::string &file, picojson::value &v);
  bool load(const std::string &file);
  bool save(const std::string &file) const;
};

// �f�t�H���g�l
extern config defaults;

// �i�r�Q�[�V�����̑��x����
constexpr GLfloat zoomStep(0.01f);                          // ���̂̃Y�[���������̃X�e�b�v
constexpr GLfloat shiftStep(0.001f);                        // �w�i�e�N�X�`���̃V�t�g�ʒ����̃X�e�b�v
constexpr GLfloat focalStep(50.0f);                         // �w�i�e�N�X�`���̃X�P�[�������̃X�e�b�v
constexpr GLfloat speedScale(0.005f);                       // �t���[��������̈ړ����x�W��
constexpr GLfloat angleScale(-0.05f);                       // �t���[��������̉�]���x�W��
constexpr GLfloat wheelXStep(0.005f);                       // �}�E�X�z�C�[���� X �����̌W��
constexpr GLfloat wheelYStep(0.005f);                       // �}�E�X�z�C�[���� Y �����̌W��
constexpr GLfloat axesSpeedScale(0.01f);                    // �Q�[���p�b�h�̃X�e�B�b�N�̑��x�̌W��
constexpr GLfloat axesAngleScale(0.01f);                    // �Q�[���p�b�h�̃X�e�B�b�N�̊p���x�̌W��
constexpr GLfloat btnsScale(0.02f);                         // �Q�[���p�b�h�̃{�^���̌W��

// �����̕ύX�X�e�b�v (�P�� m)
constexpr GLfloat parallaxStep(0.001f);

// �}���`�T���v�����O�̃T���v���� (Oculus Rift)
constexpr int backBufferMultisample(0);

// ����
constexpr GgSimpleShader::Light lightData =
{
  { 0.2f, 0.2f, 0.2f, 0.4f },                           // ��������
  { 1.0f, 1.0f, 1.0f, 0.0f },                           // �g�U���ˌ�����
  { 1.0f, 1.0f, 1.0f, 0.0f },                           // ���ʌ�����
  { 0.0f, 0.0f, 1.0f, 1.0f }                            // �ʒu
};

// �ގ�
constexpr GgSimpleShader::Material materialData =
{
  { 0.8f, 0.8f, 0.8f, 1.0f },                           // �����̔��ˌW��
  { 0.8f, 0.8f, 0.8f, 1.0f },                           // �g�U���ˌW��
  { 0.2f, 0.2f, 0.2f, 1.0f },                           // ���ʔ��ˌW��
  50.0f                                                 // �P���W��
};

// �e�N�X�`���̋��E�F
constexpr GLfloat borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

// �G���R�[�h���@
constexpr char encoderType[] = ".jpg";

// ��M���g���C��
constexpr int receiveRetry(30);

// �ǂݔ�΂��p�P�b�g�̍ő吔
constexpr int maxDropPackets(1000);

// �t���[�����M�̍ŏ��Ԋu
constexpr long long minDelay(10);

// �J�����̐��Ǝ��ʎq
constexpr int camCount(2), camL(0), camR(1);

// �����[�g�J�����̐�
constexpr int remoteCamCount(2);

// �w�b�_�̒���
constexpr int headLength(camCount + 1);

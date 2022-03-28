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
  MONOCULAR = 0,                            // �P�ᎋ
  INTERLACE,                                // �C���^�[���[�X�i�������j
  TOP_AND_BOTTOM,                           // �㉺�Q����
  SIDE_BY_SIDE,                             // ���E�Q����
  QUADBUFFER,                               // �N���b�h�o�b�t�@�X�e���I
  OCULUS                                    // Oculus Rift (HMD)
};

// ����
enum HostRole
{
  STANDALONE = 0,                           // �P��
  OPERATOR,                                 // ���c��
  WORKER                                    // ��Ǝ�
};

// �ݒ�l
struct config
{
  // ��ʕ\���̃��[�h
  int display_mode;

  // �N���b�h�o�b�t�@�X�e���I�\�����s���Ƃ� true
  bool display_quadbuffer;

  // �t���X�N���[���\�����s���Ƃ� true
  bool display_fullscreen;

  // �t���X�N���[���\������f�B�X�v���C�̔ԍ�
  int display_secondary;

  // ��ʂ̉𑜓x
  int display_width;
  int display_height;

  // ��ʂ̏c����
  GLfloat display_aspect;

  // ��ʂ̒��S�̍���
  GLfloat display_center;

  // ��ʂ܂ł̋���
  GLfloat display_distance;

  // ���_����O���ʂ܂ł̋���
  GLfloat display_near;

  // ���_�������ʂ܂ł̋���
  GLfloat display_far;

  // ���J�����̔ԍ�
  int camera_left;

  // ���J�����̑���Ɏg���Î~��
  std::string camera_left_image;

  // ���J�����̑���Ɏg������
  std::string camera_left_movie;

  // �E�J�����̔ԍ�
  int camera_right;

  // �E�J�����̑���Ɏg���Î~��
  std::string camera_right_image;

  // �E�J�����̑���Ɏg������
  std::string camera_right_movie;

  // �w�i�摜���}�b�s���O����Ƃ��̃��b�V���̕�����
  int camera_texture_samples;

  // �w�i�摜���J��Ԃ��Ń}�b�s���O����Ƃ� true
  bool camera_texture_repeat;

  // �w�i�摜���w�b�h�g���b�L���O�ɒǏ]������Ƃ� true
  bool camera_tracking;

  // �J�����̉𑜓x
  double camera_width;
  double camera_height;

  // �J�����̃t���[�����[�g
  double camera_fps;

  // �J�����̂S�����R�[�f�b�N
  char camera_fourcc[4];

  // ����J�����̒��S�ʒu
  GLfloat camera_center_x;
  GLfloat camera_center_y;

  // ����J�����̉�p
  GLfloat camera_fov_x;
  GLfloat camera_fov_y;

  // Ovrvision Pro �̐ݒ�
  int ovrvision_property;

  // �o�[�e�b�N�X�V�F�[�_�̃\�[�X�v���O����
  std::string vertex_shader;

  // �t���O�����g�V�F�[�_�̃\�[�X�v���O����
  std::string fragment_shader;

  // ����
  int role;

  // �ʐM�Ɏg���|�[�g�ԍ�
  int port;

  // ������ IP �A�h���X
  std::string address;

  // �����̉f�������艻����Ƃ� true
  bool remote_stabilize;

  // �����̉f����ό`����Ƃ� true
  bool remote_texture_reshape;

  // �����̕\���ɉ�����x��
  unsigned int remote_delay[2];

  // ���M����摜�̕i��
  int remote_texture_quality;

  // ��M�����摜���}�b�s���O����Ƃ��̃��b�V���̕�����
  int remote_texture_samples;

  // �����̃����Y�̉�p
  GLfloat remote_fov_x;
  GLfloat remote_fov_y;

  // ���M�ɗp���鋤�L�������̃u���b�N��
  int local_share_size;

  // ��M�ɗp���鋤�L�������̃u���b�N��
  int remote_share_size;

  // �V�[���t�@�C���̓���q�̐[���̏��
  int max_level;

  // �V�[���O���t
  picojson::value scene;

  // �ݒ�t�@�C���̃t�@�C����
  std::string config_file;

  // �R���X�g���N�^
  config();

  // �f�X�g���N�^
  virtual ~config();

  // �ݒ�̉��
  bool read(picojson::value &v);

  // �ݒ�̓ǂݍ���
  bool load(const std::string &file);

  // �ݒ�̏�������
  bool save(const std::string &file) const;
};

// �f�t�H���g�l
extern config defaults;

// �E�B���h�E�̃^�C�g��
constexpr char windowTitle[]{ "TED" };

// �E�B���h�E���[�h���̃E�B���h�E�T�C�Y�̏����l
constexpr int defaultWindowWidth{ 960 };
constexpr int defaultWindowHeight{ 540 };

// �i�r�Q�[�V�����̑��x����
constexpr GLfloat zoomStep{ 0.01f };        // ���̂̃Y�[���������̃X�e�b�v
constexpr GLfloat shiftStep{ 0.001f };      // �w�i�e�N�X�`���̃V�t�g�ʒ����̃X�e�b�v
constexpr GLfloat focalStep{ 100.0f };      // �w�i�e�N�X�`���̃X�P�[�������̃X�e�b�v
constexpr GLfloat speedScale{ 0.005f };     // �t���[��������̈ړ����x�W��
constexpr GLfloat angleScale{ -0.05f };     // �t���[��������̉�]���x�W��
constexpr GLfloat wheelXStep{ 0.005f };     // �}�E�X�z�C�[���� X �����̌W��
constexpr GLfloat wheelYStep{ 0.005f };     // �}�E�X�z�C�[���� Y �����̌W��
constexpr GLfloat axesSpeedScale{ 0.01f };  // �Q�[���p�b�h�̃X�e�B�b�N�̑��x�̌W��
constexpr GLfloat axesAngleScale{ 0.01f };  // �Q�[���p�b�h�̃X�e�B�b�N�̊p���x�̌W��
constexpr GLfloat btnsScale{ 0.02f };       // �Q�[���p�b�h�̃{�^���̌W��

// �X�N���[���̊Ԋu�̃f�t�H���g�l
constexpr GLfloat defaultOffset{ 0.0f };

// �X�N���[���̊Ԋu�̕ύX�X�e�b�v
constexpr GLfloat offsetStep{ 0.001f };

// �����̃f�t�H���g�l
constexpr GLfloat defaultParallax{ 0.032f };

// �����̕ύX�X�e�b�v (�P�� m)
constexpr GLfloat parallaxStep{ 0.001f };

// �}���`�T���v�����O�̃T���v���� (Oculus Rift)
constexpr int backBufferMultisample{ 0 };

// ����
constexpr GgSimpleShader::Light lightData
{
  { 0.2f, 0.2f, 0.2f, 1.0f },               // ��������
  { 1.0f, 1.0f, 1.0f, 0.0f },               // �g�U���ˌ�����
  { 1.0f, 1.0f, 1.0f, 0.0f },               // ���ʌ�����
  { 0.0f, 0.0f, 1.0f, 1.0f }                // �ʒu
};

// �ގ�
constexpr GgSimpleShader::Material materialData
{
  { 0.8f, 0.8f, 0.8f, 1.0f },               // �����̔��ˌW��
  { 0.8f, 0.8f, 0.8f, 1.0f },               // �g�U���ˌW��
  { 0.2f, 0.2f, 0.2f, 1.0f },               // ���ʔ��ˌW��
  50.0f                                     // �P���W��
};

// �e�N�X�`���̋��E�F
constexpr GLfloat borderColor[]{ 0.0f, 0.0f, 0.0f, 1.0f };

// �G���R�[�h���@
constexpr char encoderType[]{ ".jpg" };

// ��M���g���C��
constexpr int receiveRetry{ 30 };

// �ǂݔ�΂��p�P�b�g�̍ő吔
constexpr int maxDropPackets{ 1000 };

// �t���[�����M�̍ŏ��Ԋu
constexpr long long minDelay{ 10 };

// �J�����̎��ʎq�Ɛ�
enum CameraId{ camL = 0, camR, camCount };

// �����[�g�J�����̐�
constexpr int remoteCamCount{ camCount };

// �w�b�_�̒���
constexpr int headLength{ camCount + 1 };

// ���[�J���̋��L�������̃T�C�Y
constexpr int localShareSize{ 64 };

// �����[�g�̋��L�������̃T�C�Y
constexpr int remoteShareSize{ 64 };

// �t�@�C���}�b�s���O�I�u�W�F�N�g��
constexpr wchar_t* localMutexName{ L"TED_LOCAL_MUTEX" };
constexpr wchar_t* localShareName{ L"TED_LOCAL_SHARE" };
constexpr wchar_t* remoteMutexName{ L"TED_REMOTE_MUTEX" };
constexpr wchar_t* remoteShareName{ L"TED_REMOTE_SHARE" };

// �ݒ�t�@�C����
constexpr char defaultConfig[]{ "config.json" };

// �p���t�@�C����
constexpr char defaultAttitude[]{ "attitude.json" };

//
// TelExistence Display System
//

// �E�B���h�E�֘A�̏���
#include "GgApp.h"

// �l�b�g���[�N�֘A�̏���
#include "Network.h"

// �J�����֘A�̏���
#include "CamCv.h"
#include "CamOv.h"
#include "CamImage.h"
#include "CamRemote.h"

// �V�[���O���t
#include "Scene.h"

// �p��
#include "Attitude.h"

// ��`
#include "Rect.h"

// ���j���[
#include "Menu.h"

//
// �R���X�g���N�^
//
GgApp::GgApp()
  : image{ nullptr, nullptr }
  , size{ { 0, 0 }, { 0, 0 } }
  , aspect{ 1.0f, 1.0f }
  , texture{ 0, 0 }
  , stereo{ false }
{
}

//
// �f�X�g���N�^
//
GgApp::~GgApp()
{
}

//
// �Î~�摜�t�@�C�����g��
//
bool GgApp::useImage()
{
  // ���̉摜���w�肳��Ă��Ȃ���Ζ߂�
  if (defaults.camera_image[camL].empty())
  {
    NOTIFY("���̉摜�t�@�C�����w�肳��Ă��܂���B");
    return false;
  }

  // ���J�����ɉ摜�t�@�C�����g��
  CamImage *const cam{ new CamImage };

  // ���������J�������L�^���Ă���
  camera.reset(cam);

  // ���̉摜���g�p�ł��Ȃ���Ζ߂�
  if (!cam->open(defaults.camera_image[camL], camL))
  {
    NOTIFY("���̉摜�t�@�C�����g�p�ł��܂���B");
    return false;
  }

  // ���̉摜��ۑ����Ă���
  image[camL] = cam->getImage(camL);

  // �E�̉摜���w�肳��Ă��ĉE�̉摜�Ɠ����łȂ����
  if (!defaults.camera_image[camR].empty() && defaults.camera_image[camR] != defaults.camera_image[camL])
  {
    // �E�̉摜���g�p�ł��Ȃ���Όx������
    if (!cam->open(defaults.camera_image[camR], camR))
    {
      NOTIFY("�E�̉摜�t�@�C�����g�p�ł��܂���B");
    }
    else
    {
      // �E�̉摜��ۑ����Ă���
      image[camR] = cam->getImage(camR);

      // �X�e���I����
      stereo = true;
    }
  }

  return true;
}

//
// ���摜�t�@�C�����g�p����
//
bool GgApp::useMovie()
{
  // ���̓��摜�t�@�C�����w�肳��Ă��Ȃ���Ζ߂�
  if (defaults.camera_movie[camL].empty())
  {
    NOTIFY("���̓��摜�t�@�C�����w�肳��Ă��܂���B");
    return false;
  }

  // ���J������ OpenCV �̃L���v�`���f�o�C�X���g��
  CamCv *const cam{ new CamCv };

  // ���������J�������L�^���Ă���
  camera.reset(cam);

  // ���̓��摜�t�@�C�����J���Ȃ���Ζ߂�
  if (!cam->open(defaults.camera_movie[camL], camL))
  {
    NOTIFY("���̓��摜�t�@�C�����g�p�ł��܂���B");
    return false;
  }

  // �E�J�����ɍ��ƈقȂ郀�[�r�[�t�@�C�����w�肳��Ă����
  if (!defaults.camera_movie[camR].empty() && defaults.camera_movie[camR] != defaults.camera_movie[camL])
  {
    // �E�̓��摜�t�@�C�����g�p�ł��Ȃ���Όx������
    if (!cam->open(defaults.camera_movie[camR], camR))
    {
      NOTIFY("�E�̓��摜�t�@�C�����g�p�ł��܂���B");
    }
    else
    {
      // �X�e���I����
      stereo = true;
    }
  }

  return true;
}

//
// Web �J�������g�p����
//
bool GgApp::useCamera()
{
  // ���J�������w�肳��Ă��Ȃ���Ζ߂�
  if (defaults.camera_id[camL] < 0)
  {
    NOTIFY("���̃J�������w�肳��Ă��܂���B");
    return false;
  }

  // ���J������ OpenCV �̃L���v�`���f�o�C�X���g��
  CamCv *const cam{ new CamCv };

  // ���������J�������L�^���Ă���
  camera.reset(cam);

  // ���J�����̃f�o�C�X���J���Ȃ���Ζ߂�
  if (!cam->open(defaults.camera_id[camL], camL))
  {
    NOTIFY("���̃J�������g�p�ł��܂���B");
    return false;
  }

  // �E�J�������w�肳��Ă��č��ƈقȂ�J�������w�肳��Ă����
  if (defaults.camera_id[camR] >= 0 && defaults.camera_id[camR] != defaults.camera_id[camL])
  {
    // �E�J�����̃f�o�C�X���g�p�ł��Ȃ���Όx������
    if (!cam->open(defaults.camera_id[camR], camR))
    {
      NOTIFY("�E�̃J�������g�p�ł��܂���B");
    }
    else
    {
      // �X�e���I����
      stereo = true;
    }
  }

  return true;
}

//
// Ovrvision Pro ���g��
//
bool GgApp::useOvervision()
{
  // Ovrvision Pro ���g��
  CamOv *const cam{ new CamOv };

  // ���������J�������L�^���Ă���
  camera.reset(cam);

  // Ovrvision Pro ���J���Ȃ���Ζ߂�
  if (!cam->open(static_cast<OVR::Camprop>(defaults.ovrvision_property)))
  {
    // Ovrvision Pro ���g���Ȃ�����
    NOTIFY("Ovrvision Pro ���g���܂���B");
    return false;
  }

  // �X�e���I����
  stereo = true;

  return true;
}

//
// RealSense ���g��
//
bool GgApp::useRealsense()
{
  return false;
}

//
// �����[�g�� TED ����擾����
//
bool GgApp::useRemote()
{
  // �����[�g�J��������L���v�`�����邽�߂̃_�~�[�J�������g��
  CamRemote *const cam(new CamRemote(defaults.remote_texture_reshape));

  // ���������J�������L�^���Ă���
  camera.reset(cam);

  // �w���ґ����N������
  if (cam->open(defaults.port, defaults.address.c_str()) < 0)
  {
    NOTIFY("��Ǝґ��̃f�[�^���󂯎��܂���B");
    return false;
  }

  // �X�e���I����
  stereo = true;

  return true;
}

//
// ���̓\�[�X��I������
//
bool GgApp::selectInput()
{
  std::fill(image, image + camCount, nullptr);

  switch (defaults.input_mode)
  {
  case InputMode::IMAGE:
    if (!useImage()) return false;
    break;
  case InputMode::MOVIE:
    if (!useMovie()) return false;
    break;
  case InputMode::CAMERA:
    if (!useCamera()) return false;
    break;
  case InputMode::OVRVISION:
    if (!useOvervision()) return false;
    break;
  case InputMode::REALSENSE:
    if (!useRealsense()) return false;
    break;
  case InputMode::REMOTE:
    if (!useRemote()) return false;
    break;
  default:
    break;
  }

  // ���J�����̃T�C�Y�𓾂�
  size[camL][0] = camera->getWidth(camL);
  size[camL][1] = camera->getHeight(camL);

  // �E�J�������g�p�ł����
  if (stereo)
  {
    // �E�J�����̃T�C�Y�𓾂�
    size[camR][0] = camera->getWidth(camR);
    size[camR][1] = camera->getHeight(camR);
  }
  else
  {
    // �E�J�����̃T�C�Y�͍��Ɠ����ɂ��Ă���
    size[camR][0] = size[camL][0];
    size[camR][1] = size[camL][1];
  }

  // ����܂Ŏg���Ă����e�N�X�`�����폜����
  glDeleteTextures(camCount, texture);

  // �w�i�摜��ۑ�����e�N�X�`�����쐬����
  glGenTextures(camCount, texture);

  // �e�N�X�`���̋��E�̏���
  const GLenum border{ static_cast<GLenum>(defaults.camera_texture_repeat ? GL_REPEAT : GL_CLAMP_TO_BORDER) };

  // �e�N�X�`������������
  for (int cam = 0; cam < camCount; ++cam)
  {
    // �e�N�X�`�����������m�ۂ���
    glBindTexture(GL_TEXTURE_2D, texture[cam]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[cam][0], size[cam][1], 0,
      GL_BGR, GL_UNSIGNED_BYTE, image[cam]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, border);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // �e�N�X�`���̃A�X�y�N�g������߂�
    aspect[cam] = static_cast<GLfloat>(size[camL][0]) / static_cast<GLfloat>(size[camL][1]);
  }

  return true;
}

//
// ���C���v���O����
//
int GgApp::main(int argc, const char *const *const argv)
{
  // ������ݒ�t�@�C�����Ɏg���i�w�肳��Ă��Ȃ���� defaultConfig �ɂ���j
  const char *config_file{ argc > 1 ? argv[1] : defaultConfig };

  // �ݒ�t�@�C����ǂݍ��� (������Ȃ���������)
  if (!defaults.load(config_file)) defaults.save(config_file);

  // �p���t�@�C����ǂݍ��ށi������Ȃ���������j
  if (!attitude.load(defaultAttitude)) attitude.save(defaultAttitude);

  // GLFW ������������
  if (glfwInit() == GLFW_FALSE)
  {
    // GLFW �̏������Ɏ��s����
    NOTIFY("GLFW �̏������Ɏ��s���܂����B");
    return EXIT_FAILURE;
  }

  // �v���O�����I�����ɂ� GLFW ���I������
  atexit(glfwTerminate);

  // �f�B�X�v���C�̏��
  GLFWmonitor *monitor;
  int windowWidth, windowHeight;

  // �t���X�N���[���\��
  if (defaults.display_fullscreen)
  {
    // �ڑ�����Ă��郂�j�^�̐��𐔂���
    int monitorCount;
    GLFWmonitor **const monitors(glfwGetMonitors(&monitorCount));

    // ���j�^�̑��݃`�F�b�N
    if (monitorCount == 0)
    {
      NOTIFY("�\���\�ȃf�B�X�v���C��������܂���B");
      return EXIT_FAILURE;
    }

    // �Z�J���_�����j�^������΂�����g��
    monitor = monitors[monitorCount > defaults.display_secondary ? defaults.display_secondary : 0];

    // ���j�^�̃��[�h�𒲂ׂ�
    const GLFWvidmode *mode{ glfwGetVideoMode(monitor) };

    // �E�B���h�E�̃T�C�Y���f�B�X�v���C�̃T�C�Y�ɂ���
    windowWidth = mode->width;
    windowHeight = mode->height;
  }
  else
  {
    // �v���C�}�����j�^���E�B���h�E���[�h�Ŏg��
    monitor = nullptr;

    // �E�B���h�E�̃T�C�Y�Ƀf�t�H���g�l��ݒ肷��
    windowWidth = defaults.display_size[0] ? defaults.display_size[0] : defaultWindowWidth;
    windowHeight = defaults.display_size[1] ? defaults.display_size[1] : defaultWindowHeight;
  }

  // �E�B���h�E���J��
  Window window(windowWidth, windowHeight, windowTitle, monitor);

  // �E�B���h�E�I�u�W�F�N�g����������Ȃ���ΏI������
  if (!window.get())
  {
    // �E�C���h�E�̍쐬�����s�������Ǝv����ݒ��߂�
    defaults.display_secondary = 0;
    defaults.display_fullscreen = false;
    defaults.display_quadbuffer = false;

    // �ݒ�t�@�C����ۑ�����
    defaults.save(config_file);

    // �E�B���h�E���J���Ȃ������̂ŏI������
    NOTIFY("�\���p�̃E�B���h�E���쐬�ł��܂���ł����B");
    return EXIT_FAILURE;
  }

  // ���L���������m�ۂ���
  if (!Scene::initialize(defaults.local_share_size, defaults.remote_share_size))
  {
    // ���L�������̊m�ۂ����s�������Ǝv����ݒ��߂�
    defaults.local_share_size = localShareSize;
    defaults.remote_share_size = remoteShareSize;

    // �ݒ�t�@�C����ۑ�����
    defaults.save(config_file);

    // ���L�������̊m�ۂɎ��s�����̂ŏI������
    NOTIFY("���L���������m�ۂł��܂���ł����B");
    return EXIT_FAILURE;
  }

  // �w�i�̕`��ɗp�����`���쐬����
  Rect rect{ window, defaults.vertex_shader, defaults.fragment_shader };
  if (!rect.get())
  {
    // �V�F�[�_���ǂݍ��߂Ȃ�����
    NOTIFY("�w�i�`��p�̃V�F�[�_�t�@�C���̓ǂݍ��݂Ɏ��s���܂����B");
    return EXIT_FAILURE;
  }

  // �O�i�̕`��ɗp����V�F�[�_�v���O������ǂݍ���
  GgSimpleShader simple{ "simple.vert", "simple.frag" };
  if (!simple.get())
  {
    // �V�F�[�_���ǂݍ��߂Ȃ�����
    NOTIFY("�}�`�`��p�̃V�F�[�_�t�@�C���̓ǂݍ��݂Ɏ��s���܂����B");
    return EXIT_FAILURE;
  }

  // winsock2 ������������
  WSAData wsaData;
  if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
  {
    NOTIFY("Windows Sockets 2 �̏������Ɏ��s���܂����B");
    return EXIT_FAILURE;
  }

  // winsock2 �̏I��������o�^����
  atexit(reinterpret_cast<void(*)()>(WSACleanup));

  // �V�[���O���t
  Scene scene{ defaults.scene };

  // �V�[���ɃV�F�[�_��ݒ肷��
  scene.setShader(simple);

  // ����
  const GgSimpleShader::LightBuffer light{ lightData };

  // ���͂�I������
  if (!selectInput()) return EXIT_FAILURE;

  // �E�B���h�E�ɂ��̃J���������ѕt����
  window.setControlCamera(camera.get());

  // ���ڗp�̔w�i�摜��\��t�����`�Ƀe�N�X�`����ݒ肷��
  rect.setTexture(0, texture[0]);

  // �E�ڗp�̔w�i�摜��\��t�����`�Ƀe�N�X�`����ݒ肷��
  rect.setTexture(1, texture[stereo ? 1 : 0]);

  // �ʏ�̃t���[���o�b�t�@�ɕ`��
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // �J�����O����
  glCullFace(GL_BACK);

  // �A���t�@�u�����f�B���O����
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // �`��񐔂͒P�ᎋ�Ȃ� 1�A����ȊO�Ȃ�J�����̐�
  int drawCount{ defaults.display_mode == MONOCULAR ? 1 : camCount };

  // �f�t�H���g�� OCULUS �Ȃ� Oculus Rift ���N������
  if (defaults.display_mode == OCULUS) window.startOculus();

  // ���j���[
  Menu menu{ this, window, scene, attitude };

  // �E�B���h�E���J���Ă���Ԃ���Ԃ��`�悷��
  while (window)
  {
    // ���j���[��\������
    if (window.showMenu) menu.show();

    // �L���ȃJ�����̐�
    int cam_count{ 1 };

    // �L���ȃJ�����ɂ���
    for (int cam = 0; cam < cam_count; ++cam)
    {
      // �J���������b�N���ĉ摜��]������
      camera->transmit(cam, texture[cam], size[cam]);
    }

    // �`��J�n
    if (window.start())
    {
      // �L���Ȗڂɂ���
      for (int eye = 0; eye < eyeCount; ++eye)
      {
        // �}�`��������ڂ�I������
        window.select(eye);

        // �w�i�̕`��ݒ�
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        // ���[�J���̃w�b�h�g���b�L���O�̕ϊ��s��
        const GgMatrix &mo(defaults.camera_tracking
          ? window.getMo(eye) : attitude.eyeOrientation[eye].getMatrix());

        // �����[�g�̃w�b�h�g���b�L���O�̕ϊ��s��
        const GgMatrix &&mr(mo * Scene::getRemoteAttitude(eye));

        // �w�i��`��
        rect.draw(eye, defaults.remote_stabilize ? mr : mo, window.getSamples());

        // �}�`�ƏƏ��̕`��ݒ�
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);

        // �`��p�̃V�F�[�_�v���O�����̎g�p�J�n
        simple.use(light);

        // �}�`��`�悷��
        if (window.showScene) scene.draw(window.getMp(eye), window.getMv(eye) * window.getMo(eye));

        // �Жڂ̏�������������
        window.commit(eye);

        // �P�ᎋ�Ȃ�I��
        if (defaults.display_mode == MONOCULAR) break;
      }
    }

    // �o�b�t�@�����ւ���
    window.swapBuffers();
  }

  // �w�i�摜�p�̃e�N�X�`�����폜����
  glDeleteBuffers(camCount, texture);

  return EXIT_SUCCESS;
}

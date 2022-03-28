//
// TelExistence Display System
//

// �E�B���h�E�֘A�̏���
#include "Window.h"

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
// ���C���v���O����
//
int main(int argc, const char *const *const argv)
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
    int mcount;
    GLFWmonitor **const monitors(glfwGetMonitors(&mcount));

    // ���j�^�̑��݃`�F�b�N
    if (mcount == 0)
    {
      NOTIFY("�\���\�ȃf�B�X�v���C��������܂���B");
      return EXIT_FAILURE;
    }

    // �Z�J���_�����j�^������΂�����g��
    monitor = monitors[mcount > defaults.display_secondary ? defaults.display_secondary : 0];

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
    windowWidth = defaults.display_width ? defaults.display_width : defaultWindowWidth;
    windowHeight = defaults.display_height ? defaults.display_height : defaultWindowHeight;
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

  // �V�[���O���t
  Scene scene{ defaults.scene };

  // �V�[���ɃV�F�[�_��ݒ肷��
  scene.setShader(simple);

  // ����
  const GgSimpleShader::LightBuffer light{ lightData };

  // �w�i�摜�̃f�[�^
  const GLubyte *image[camCount]{ nullptr };

  // �w�i�摜�̃T�C�Y
  GLsizei size[camCount][2];

  // �w�i�摜���擾����J����
  std::shared_ptr<Camera> camera;

  // �l�b�g���[�N���g�p����ꍇ
  if (defaults.role != STANDALONE)
  {
    // winsock2 ������������
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
    {
      NOTIFY("Windows Sockets 2 �̏������Ɏ��s���܂����B");
      return EXIT_FAILURE;
    }

    // winsock2 �̏I��������o�^����
    atexit(reinterpret_cast<void(*)()>(WSACleanup));

    // ���c�҂Ƃ��ē��삷��ꍇ
    if (defaults.role == OPERATOR)
    {
      // �����[�g�J��������L���v�`�����邽�߂̃_�~�[�J�������g��
      CamRemote *const cam(new CamRemote(defaults.remote_texture_reshape));

      // ���������J�������L�^���Ă���
      camera.reset(cam);

      // ���c�ґ����N������
      if (cam->open(defaults.port, defaults.address.c_str()) < 0)
      {
        NOTIFY("��Ǝґ��̃f�[�^���󂯎��܂���B");
        return EXIT_FAILURE;
      }

      // �摜�T�C�Y��ݒ肷��
      size[camL][0] = cam->getWidth(camL);
      size[camL][1] = cam->getHeight(camL);
      size[camR][0] = cam->getWidth(camR);
      size[camR][1] = cam->getHeight(camR);
    }
  }

  // �_�~�[�J�������ݒ肳��Ă��Ȃ����
  if (!camera)
  {
    // ���J�������瓮�����͂���Ƃ�
    if (defaults.camera_left >= 0 || !defaults.camera_left_movie.empty())
    {
      // ���J������ OpenCV �̃L���v�`���f�o�C�X���g��
      CamCv *const cam{ new CamCv };

      // ���������J�������L�^���Ă���
      camera.reset(cam);

      // ���[�r�[�t�@�C�����w�肳��Ă����
      if (!defaults.camera_left_movie.empty())
      {
        // ���[�r�[�t�@�C�����J��
        if (!cam->open(defaults.camera_left_movie, camL))
        {
          NOTIFY("���̓���t�@�C�����g�p�ł��܂���B");
          return EXIT_FAILURE;
        }
      }
      else
      {
        // �J�����f�o�C�X���J��
        if (!cam->open(defaults.camera_left, camL))
        {
          NOTIFY("���̃J�������g�p�ł��܂���B");
          return EXIT_FAILURE;
        }
      }

      // ���J�����̃T�C�Y�𓾂�
      size[camL][0] = cam->getWidth(camL);
      size[camL][1] = cam->getHeight(camL);

      // �E�J�����̃T�C�Y�͍��Ɠ����ɂ��Ă���
      size[camR][0] = size[camL][0];
      size[camR][1] = size[camL][1];

      // ���̎��\�����s���Ƃ�
      if (defaults.display_mode != MONOCULAR)
      {
        // �E�J�����ɍ��ƈقȂ郀�[�r�[�t�@�C�����w�肳��Ă����
        if (!defaults.camera_right_movie.empty() && defaults.camera_right_movie != defaults.camera_left_movie)
        {
          // ���[�r�[�t�@�C�����J��
          if (!cam->open(defaults.camera_right_movie, camR))
          {
            NOTIFY("�E�̓���t�@�C�����g�p�ł��܂���B");
            return EXIT_FAILURE;
          }

          // �E�J�����̃T�C�Y�𓾂�
          size[camR][0] = cam->getWidth(camR);
          size[camR][1] = cam->getHeight(camR);
        }
        else if (defaults.camera_right >= 0 && defaults.camera_right != defaults.camera_left)
        {
          // �J�����f�o�C�X���J��
          if (!cam->open(defaults.camera_right, camR))
          {
            NOTIFY("�E�̃J�������g�p�ł��܂���B");
            return EXIT_FAILURE;
          }

          // �E�J�����̃T�C�Y�𓾂�
          size[camR][0] = cam->getWidth(camR);
          size[camR][1] = cam->getHeight(camR);
        }
      }
    }
    else
    {
      // �E�J�����������g���Ȃ�
      if (defaults.camera_right >= 0)
      {
        // Ovrvision Pro ���g��
        CamOv *const cam{ new CamOv };

        // ���������J�������L�^���Ă���
        camera.reset(cam);

        // Ovrvision Pro ���J��
        if (cam->open(static_cast<OVR::Camprop>(defaults.ovrvision_property)))
        {
          // Ovrvision Pro �̉摜�T�C�Y�𓾂�
          size[camL][0] = cam->getWidth(camL);
          size[camL][1] = cam->getHeight(camL);
          size[camR][0] = cam->getWidth(camR);
          size[camR][1] = cam->getHeight(camR);
        }
        else
        {
          // Ovrvision Pro ���g���Ȃ�����
          NOTIFY("Ovrvision Pro ���g���܂���B");
          return EXIT_FAILURE;
        }
      }
      else
      {
        // ���J�����ɉ摜�t�@�C�����g��
        CamImage *const cam{ new CamImage };

        // ���������J�������L�^���Ă���
        camera.reset(cam);

        // ���̉摜���g�p�\�Ȃ�
        if (!cam->open(defaults.camera_left_image, camL))
        {
          // ���̉摜�t�@�C�����ǂݍ��߂Ȃ�����
          NOTIFY("���̉摜�t�@�C�����g�p�ł��܂���B");
          return EXIT_FAILURE;
        }

        // ���̉摜���g�p����
        image[camL] = cam->getImage(camL);

        // ���J�����̃T�C�Y�𓾂�
        size[camL][0] = camera->getWidth(camL);
        size[camL][1] = camera->getHeight(camL);

        // �E�J�����̃T�C�Y�͍��Ɠ����ɂ��Ă���
        size[camR][0] = size[camL][0];
        size[camR][1] = size[camL][1];

        // ���̎��\�����s���Ƃ�
        if (defaults.display_mode != MONOCULAR)
        {
          // �E�̉摜���w�肳��Ă����
          if (!defaults.camera_right_image.empty())
          {
            // �E�̉摜���g�p�\�Ȃ�
            if (!cam->open(defaults.camera_right_image, camR))
            {
              // �E�̉摜�t�@�C�����ǂݍ��߂Ȃ�����
              NOTIFY("�E�̉摜�t�@�C�����g�p�ł��܂���B");
              return EXIT_FAILURE;
            }

            // �E�̉摜���g�p����
            image[camR] = cam->getImage(camR);

            // �E�̉摜�̃T�C�Y�𓾂�
            size[camR][0] = cam->getWidth(camR);
            size[camR][1] = cam->getHeight(camR);
          }
        }
      }
    }

    // ��Ǝ҂Ƃ��ē��삷��ꍇ
    if (defaults.role == WORKER && defaults.port > 0 && !defaults.address.empty())
    {
      // �ʐM�X���b�h���N������
      camera->startWorker(defaults.port, defaults.address.c_str());
    }
  }

  // �E�B���h�E�ɂ��̃J���������ѕt����
  window.setControlCamera(camera.get());

  // �e�N�X�`���̃A�X�y�N�g��
  const GLfloat texture_aspect[]
  {
    (GLfloat(size[camL][0]) / GLfloat(size[camL][1])),
    (GLfloat(size[camR][0]) / GLfloat(size[camR][1]))
  };

  // �e�N�X�`���̋��E�̏���
  const GLenum border(defaults.camera_texture_repeat ? GL_REPEAT : GL_CLAMP_TO_BORDER);

  // �w�i�摜��ۑ�����e�N�X�`�����쐬����
  GLuint texture[camCount];
  glGenTextures(camCount, texture);

  // �w�i�摜��ۑ�����e�N�X�`������������
  for (int cam = 0; cam < camCount; ++cam)
  {
    glBindTexture(GL_TEXTURE_2D, texture[cam]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[cam][0], size[cam][1], 0,
      GL_BGR, GL_UNSIGNED_BYTE, image[cam]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, border);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
  }

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
  Menu menu{ window, scene, attitude };

  // �E�B���h�E���J���Ă���Ԃ���Ԃ��`�悷��
  while (window)
  {
    // ���j���[��\������
    if (window.showMenu) menu.show();

    // ���J���������b�N���ĉ摜��]������
    camera->transmit(camL, texture[camL], size[camL]);

    // �E�̃e�N�X�`�����L���ɂȂ��Ă����
    if (texture[camR] != texture[camL])
    {
      // �E�J���������b�N���ĉ摜��]������
      camera->transmit(camR, texture[camR], size[camR]);
    }

    // �`��J�n
    if (window.start())
    {
      // ���ׂĂ̖ڂɂ���
      for (int eye = 0; eye < drawCount; ++eye)
      {
        // �}�`��������ڂ�I������
        window.select(eye);

        // �w�i�̕`��ݒ�
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        // ���[�J���̃w�b�h�g���b�L���O�̕ϊ��s��
        const GgMatrix &&mo(defaults.camera_tracking
          ? window.getMo(eye) * attitude.eyeOrientation[eye].getMatrix()
          : attitude.eyeOrientation[eye].getMatrix());

        // �����[�g�̃w�b�h�g���b�L���O�̕ϊ��s��
        const GgMatrix &&mr(mo * Scene::getRemoteAttitude(eye));

        // �w�i��`��
        rect.draw(eye, texture, defaults.remote_stabilize
          ? mr : mo, window.getSamples());

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
      }
    }

    // �o�b�t�@�����ւ���
    window.swapBuffers();
  }

  // �w�i�摜�p�̃e�N�X�`�����폜����
  glDeleteBuffers(camCount, texture);
}

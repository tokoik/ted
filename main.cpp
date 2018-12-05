//
// TelExistence Display System
//

// �E�B���h�E�֘A�̏���
#include "Window.h"

// �l�b�g���[�N�֘A�̏���
#include "Network.h"

// ���L������
#include "SharedMemory.h"

// �J�����֘A�̏���
#include "CamCv.h"
#include "CamOv.h"
#include "CamImage.h"
#include "CamRemote.h"

// �V�[���O���t
#include "Scene.h"

// ��`
#include "Rect.h"

// �E�B���h�E���[�h���̃E�B���h�E�T�C�Y�̏����l
const int defaultWindowWidth(960);
const int defaultWindowHeight(540);

// �E�B���h�E�̃^�C�g��
const char windowTitle[] = "TED";

//
// ���C���v���O����
//
int main(int argc, const char *const *const argv)
{
  // ������ݒ�t�@�C�����Ɏg���i�w�肳��Ă��Ȃ���� config.json �ɂ���j
  const char *config_file(argc > 1 ? argv[1] : "config.json");

  // �ݒ�t�@�C����ǂݍ��� (������Ȃ���������)
  if (!defaults.load(config_file)) defaults.save(config_file);

  // ���L���������m�ۂ���
  if (!SharedMemory::initialize(defaults.local_share_size, defaults.remote_share_size, camCount + 1))
  {
    // ���L�������̊m�ۂɎ��s����
    NOTIFY("�ϊ��s���ێ����鋤�L���������m�ۂł��܂���ł����B");
    return EXIT_FAILURE;
  }

  // GLFW ������������
  if (glfwInit() == GL_FALSE)
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
      NOTIFY("GLFW �̏����������Ă��Ȃ����\���\�ȃf�B�X�v���C��������܂���B");
      return EXIT_FAILURE;
    }

    // �Z�J���_�����j�^������΂�����g��
    monitor = monitors[mcount > defaults.display_secondary ? defaults.display_secondary : 0];

    // ���j�^�̃��[�h�𒲂ׂ�
    const GLFWvidmode *mode(glfwGetVideoMode(monitor));

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
  if (!window.get()) return EXIT_FAILURE;

  // �w�i�摜�̃f�[�^
  const GLubyte *image[camCount] = { nullptr };

  // �w�i�摜�̃T�C�Y
  GLsizei size[camCount][2];

  // �w�i�摜���擾����J����
  std::shared_ptr<Camera> camera(nullptr);

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
    // ���J�������g��Ȃ��Ƃ�
    if (defaults.camera_left < 0)
    {
      // �E�J�����������g���Ȃ�
      if (defaults.camera_right >= 0)
      {
        // Ovrvision Pro ���g��
        CamOv *const cam(new CamOv);

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
        CamImage *const cam(new CamImage);

        // ���������J�������L�^���Ă���
        camera.reset(cam);

        // ���̉摜���g�p�\�Ȃ�
        if (cam->open(defaults.camera_left_image, camL))
        {
          // ���̉摜���g�p����
          image[camL] = cam->getImage(camL);

          // ���̉摜�̃T�C�Y�𓾂�
          size[camL][0] = cam->getWidth(camL);
          size[camL][1] = cam->getHeight(camL);
        }
        else
        {
          // ���̉摜�t�@�C�����ǂݍ��߂Ȃ�����
          NOTIFY("���̉摜�t�@�C�����g�p�ł��܂���B");
          return EXIT_FAILURE;
        }

        // �E�̉摜�T�C�Y�͍��Ɠ����ɂ��Ă���
        size[camR][0] = size[camL][0];
        size[camR][1] = size[camL][1];

        // ���̎��\�����s���Ƃ�
        if (defaults.display_mode != MONO)
        {
          // �E�̉摜���w�肳��Ă����
          if (!defaults.camera_right_image.empty())
          {
            // �E�̉摜���g�p�\�Ȃ�
            if (cam->open(defaults.camera_right_image, camR))
            {
              // �E�̉摜���g�p����
              image[camR] = cam->getImage(camR);

              // �E�̉摜�̃T�C�Y�𓾂�
              size[camR][0] = cam->getWidth(camR);
              size[camR][1] = cam->getHeight(camR);
            }
            else
            {
              // �E�̉摜�t�@�C�����ǂݍ��߂Ȃ�����
              NOTIFY("�E�̉摜�t�@�C�����g�p�ł��܂���B");
              return EXIT_FAILURE;
            }
          }
        }
      }
    }
    else
    {
      // ���J������ OpenCV �̃L���v�`���f�o�C�X���g��
      CamCv *const cam(new CamCv);

      // ���������J�������L�^���Ă���
      camera.reset(cam);

      // ���J�������g�p�\�Ȃ�
      if (defaults.camera_left >= 0
        ? cam->open(defaults.camera_left, camL)
        : !defaults.camera_left_movie.empty()
        ? cam->open(defaults.camera_left_movie, camL)
        : false)
      {
        // ���J�����̉摜�T�C�Y�𓾂�
        size[camL][0] = cam->getWidth(camL);
        size[camL][1] = cam->getHeight(camL);
      }
      else
      {
        // ���J�������g���Ȃ�����
        NOTIFY("���̃J�������g���܂���B");
        return EXIT_FAILURE;
      }

      // ���̎��\�����s���Ƃ��E�J�������g�p����Ȃ�
      if (defaults.display_mode != MONO && defaults.camera_right >= 0)
      {
        if (defaults.camera_right != defaults.camera_left
          ? cam->open(defaults.camera_right, camR)
          : !defaults.camera_right_movie.empty()
          ? cam->open(defaults.camera_right_movie, camR)
          : false)
        {
          // �E�J�����̉摜�T�C�Y�𓾂�
          size[camR][0] = cam->getWidth(camR);
          size[camR][1] = cam->getHeight(camR);
        }
        else
        {
          // �E�J�������g���Ȃ�����
          NOTIFY("�E�̃J�������g���܂���B");
          return EXIT_FAILURE;
        }
      }
      else
      {
        // �E�̉摜�T�C�Y�͍��Ɠ����ɂ��Ă���
        size[camR][0] = size[camL][0];
        size[camR][1] = size[camL][1];
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
  const GLfloat texture_aspect[] =
  {
    (GLfloat(size[camL][0]) / GLfloat(size[camL][1])),
    (GLfloat(size[camR][0]) / GLfloat(size[camR][1]))
  };

  // �e�N�X�`���̋��E�̏���
  const GLenum border(defaults.camera_texture_repeat ? GL_REPEAT : GL_CLAMP_TO_BORDER);

  // �w�i�摜��ۑ�����e�N�X�`��
  GLuint texture[camCount];

  // ���̃e�N�X�`�����쐬����
  glGenTextures(1, texture + camL);

  // ���̔w�i�摜��ۑ�����e�N�X�`������������
  glBindTexture(GL_TEXTURE_2D, texture[camL]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[camL][0], size[camL][1], 0,
    GL_BGR, GL_UNSIGNED_BYTE, image[camL]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, border);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, border);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  // �E�̃J�����ɓ��͂���Ȃ�
  if (defaults.camera_right >= 0 || !defaults.camera_right_movie.empty() || defaults.role == OPERATOR)
  {
    // �E�̃e�N�X�`�����쐬����
    glGenTextures(1, texture + camR);

    // �E�̔w�i�摜��ۑ�����e�N�X�`������������
    glBindTexture(GL_TEXTURE_2D, texture[camR]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[camR][0], size[camR][1], 0,
      GL_BGR, GL_UNSIGNED_BYTE, image[camR]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, border);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
  }
  else
  {
    // �E�̃e�N�X�`���͍��̃e�N�X�`���Ɠ����ɂ���
    texture[camR] = texture[camL];
  }

  // �ʏ�̃t���[���o�b�t�@�ɖ߂�
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // �}�`�`��p�̃V�F�[�_�v���O������ǂݍ���
  GgSimpleShader simple("simple.vert", "simple.frag");
  if (!simple.get())
  {
    // �V�F�[�_���ǂݍ��߂Ȃ�����
    NOTIFY("�}�`�`��p�̃V�F�[�_�t�@�C���̓ǂݍ��݂Ɏ��s���܂����B");
    return EXIT_FAILURE;
  }

  // Leap Motion �� listener �� controller �����
  LeapListener listener;

  // Leap Motion �� listener �� controller ����C�x���g���󂯎��悤�ɂ���
  Leap::Controller controller;
  controller.addListener(listener);
  controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);
  controller.setPolicy(Leap::Controller::POLICY_ALLOW_PAUSE_RESUME);

  // �V�[���̕ϊ��s�������������
  Scene::initialize();

  // �V�[���O���t
  Scene scene(defaults.scene, simple);

  // �w�i�`��p�̋�`���쐬����
  Rect rect(defaults.vertex_shader, defaults.fragment_shader);

  // �V�F�[�_�v���O�������𒲂ׂ�
  if (!rect.get())
  {
    // �V�F�[�_���ǂݍ��߂Ȃ�����
    NOTIFY("�w�i�`��p�̃V�F�[�_�t�@�C���̓ǂݍ��݂Ɏ��s���܂����B");
    return EXIT_FAILURE;
  }

  // ����
  const GgSimpleShader::LightBuffer light(lightData);

  // �ގ�
  const GgSimpleShader::MaterialBuffer material(materialData);

  // �J�����O
  glCullFace(GL_BACK);

  // �A���t�@�u�����f�B���O
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // �`���
  const int drawCount(defaults.display_mode == MONO ? 1 : camCount);

  // �E�B���h�E���J���Ă���Ԃ���Ԃ��`�悷��
  while (!window.shouldClose())
  {
    // ���[�J���ƃ����[�g�̕ϊ��s������L������������o��
    Scene::setup();

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
      for (int eye = 0; eye < drawCount; ++eye)
      {
        // �}�`��������ڂ�I������
        window.select(eye);

        // �w�i�̕`��ݒ�
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        // �X�N���[���̊i�q�Ԋu
        rect.setGap(window.getGap());

        // �X�N���[���̃T�C�Y�ƒ��S�ʒu
        rect.setScreen(window.getScreen(eye));

        // �œ_����
        rect.setFocal(window.getFocal());

        // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu
        rect.setCircle(window.getCircle());

        // ���[�J���̃w�b�h�g���b�L���O�̕ϊ��s��
        const GgMatrix mo(window.getMo(eye));
        const GgMatrix ml(defaults.camera_tracking ? mo : window.getQr(eye).getMatrix());

        // �����[�g�̃w�b�h�g���b�L���O�̕ϊ��s��
        const GgMatrix mr(ml * Scene::getRemoteAttitude(eye));

        // �w�i��`��
        rect.draw(texture[eye], camera->isOperator() && defaults.remote_stabilize ? mr : ml, window.getSamples());

        // �}�`�ƏƏ��̕`��ݒ�
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);

        // �`��p�̃V�F�[�_�v���O�����̎g�p�J�n
        simple.use();
        simple.selectLight(light);
        simple.selectMaterial(material);

        // �}�`��`�悷��
        printf("%d\n", eye);
        if (window.showScene) scene.draw(window.getMp(eye), window.getMv(eye) * mo);

        // �Жڂ̏�������������
        window.commit(eye);
      }
    }

    // �o�b�t�@�����ւ���
    window.swapBuffers();
  }

  // �w�i�摜�p�̃e�N�X�`�����폜����
  glDeleteBuffers(1, &texture[camL]);
  if (texture[camR] != texture[camL]) glDeleteBuffers(1, &texture[camR]);
}

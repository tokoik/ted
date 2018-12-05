//
// �Y���� 3D �r���[�A
//

// �W�����C�u����
#include <iostream>
#include <typeinfo>
#include <memory>

// �l�b�g���[�N�֘A�̏���
#include "Network.h"

// �E�B���h�E�֘A�̏���
#include "Window.h"

// �J�����֘A�̏���
#include "Camera.h"
#include "CamCv.h"
#include "CamOv.h"
#include "CamImage.h"
#include "CamRemote.h"

// ��`
#include "Rect.h"

// �V�[���O���t
#include "Scene.h"

// Leap Motion
#include "LeapListener.h"

// �E�B���h�E���[�h���̃E�B���h�E�T�C�Y�̏����l
const int defaultWindowWidth(960);
const int defaultWindowHeight(540);

// �E�B���h�E�̃^�C�g��
const char windowTitle[] = "TED";

// ���b�Z�[�W�{�b�N�X�̃^�C�g��
const LPCWSTR messageTitle = L"TED";

// �t�@�C���}�b�s���O�I�u�W�F�N�g��
const LPCWSTR localMutexName = L"TED_LOCAL_MUTEX";
const LPCWSTR localShareName = L"TED_LOCAL_SHARE";
const LPCWSTR remoteMutexName = L"TED_REMOTE_MUTEX";
const LPCWSTR remoteShareName = L"TED_REMOTE_SHARE";

//
// ���C���v���O����
//
int main(int argc, const char *const *const argv)
{
  // ������ݒ�t�@�C�����Ɏg���i�w�肳��Ă��Ȃ���� config.json �ɂ���j
  const char *config_file(argc > 1 ? argv[1] : "config.json");

  // �ݒ�t�@�C����ǂݍ��� (������Ȃ���������)
  if (!defaults.load(config_file)) defaults.save(config_file);

  // ���[�J���̕ϊ��s���ێ����鋤�L���������m�ۂ���
  std::unique_ptr<SharedMemory> localAttitude(new SharedMemory(localMutexName, localShareName, defaults.local_share_size));

  // ���[�J���̕ϊ��s���ێ����鋤�L���������m�ۂł������`�F�b�N����
  if (!localAttitude->get())
  {
    // ���L���������m�ۂł��Ȃ�����
    NOTIFY("���[�J���̕ϊ��s���ێ����鋤�L���������m�ۂł��܂���ł����B");
    return EXIT_FAILURE;
  }

  // �����[�g�̕ϊ��s���ێ����鋤�L���������m�ۂ���
  std::unique_ptr<SharedMemory> remoteAttitude(new SharedMemory(remoteMutexName, remoteShareName, defaults.remote_share_size));

  // �����[�g�̕ϊ��s���ێ����鋤�L���������m�ۂł������`�F�b�N����
  if (!remoteAttitude->get())
  {
    // ���L���������m�ۂł��Ȃ�����
    NOTIFY("�����[�g�̕ϊ��s���ێ����鋤�L���������m�ۂł��܂���ł����B");
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
  if (defaults.display_mode != MONO && defaults.display_mode != OCULUS && !debug)
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
    windowWidth = defaultWindowWidth;
    windowHeight = defaultWindowHeight;
  }

  // �E�B���h�E���J��
  Window window(windowWidth, windowHeight, windowTitle, monitor);

  // �E�B���h�E�I�u�W�F�N�g����������Ȃ���ΏI������
  if (!window.get()) return EXIT_FAILURE;

  // �w�i�摜��ۑ�����e�N�X�`��
  GLuint texture[eyeCount];

  // �w�i�摜�̃f�[�^
  const GLubyte *image[eyeCount] = { nullptr };

  // �w�i�摜�̃T�C�Y
  GLsizei size[eyeCount][2];

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

      // �J�����ŎQ�Ƃ���ϊ��s��̕[��I������
      camera->selectTable(localAttitude.get(), remoteAttitude.get());

      // ���c�ґ����N������
      if (cam->open(defaults.port, defaults.address.c_str(), texture) < 0)
      {
        NOTIFY("��Ǝґ�����f�[�^�������Ă��܂���B");
        return EXIT_FAILURE;
      }

      // ���E�̃e�N�X�`�����쐬����
      glGenTextures(2, texture);

      // �摜�T�C�Y��ݒ肷��
      size[eyeL][0] = cam->getWidth(eyeL);
      size[eyeL][1] = cam->getHeight(eyeL);
      size[eyeR][0] = cam->getWidth(eyeR);
      size[eyeR][1] = cam->getHeight(eyeR);
    }
  }

  // �_�~�[�J�������ݒ肳��Ă��Ȃ����
  if (!camera)
  {
    // ���̃e�N�X�`�����쐬����
    glGenTextures(1, texture + eyeL);

    // �E�̃J�����ɓ��͂���Ȃ�
    if (defaults.camera_right >= 0 || !defaults.camera_right_movie.empty())
    {
      // �E�̃e�N�X�`�����쐬����
      glGenTextures(1, texture + eyeR);
    }
    else
    {
      // �E�̃e�N�X�`���͍��̃e�N�X�`���Ɠ����ɂ���
      texture[camR] = texture[camL];
    }

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
          size[eyeL][0] = cam->getWidth(eyeL);
          size[eyeL][1] = cam->getHeight(eyeL);
          size[eyeR][0] = cam->getWidth(eyeR);
          size[eyeR][1] = cam->getHeight(eyeR);
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
        if (cam->open(defaults.camera_left_image, eyeL))
        {
          // ���̉摜���g�p����
          image[eyeL] = cam->getImage(eyeL);

          // ���̉摜�̃T�C�Y�𓾂�
          size[eyeL][0] = cam->getWidth(eyeL);
          size[eyeL][1] = cam->getHeight(eyeL);
        }
        else
        {
          // ���̉摜�t�@�C�����ǂݍ��߂Ȃ�����
          NOTIFY("���̉摜�t�@�C�����g�p�ł��܂���B");
          return EXIT_FAILURE;
        }

        // �E�̉摜�T�C�Y�͍��Ɠ����ɂ��Ă���
        size[eyeR][0] = size[eyeL][0];
        size[eyeR][1] = size[eyeL][1];

        // ���̎��\�����s���Ƃ�
        if (defaults.display_mode != MONO)
        {
          // �E�̉摜���w�肳��Ă����
          if (!defaults.camera_right_image.empty())
          {
            // �E�̉摜���g�p�\�Ȃ�
            if (cam->open(defaults.camera_right_image, eyeR))
            {
              // �E�̉摜���g�p����
              image[eyeR] = cam->getImage(eyeR);

              // �E�̉摜�̃T�C�Y�𓾂�
              size[eyeR][0] = cam->getWidth(eyeR);
              size[eyeR][1] = cam->getHeight(eyeR);
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
        ? cam->open(defaults.camera_left, eyeL)
        : !defaults.camera_left_movie.empty()
        ? cam->open(defaults.camera_left_movie, eyeL)
        : false)
      {
        // ���J�����̉摜�T�C�Y�𓾂�
        size[eyeL][0] = cam->getWidth(eyeL);
        size[eyeL][1] = cam->getHeight(eyeL);
      }
      else
      {
        // ���J�������g���Ȃ�����
        NOTIFY("���̃J�������g���܂���B");
        return EXIT_FAILURE;
      }

      // ���̎��\�����s���Ƃ�
      if (defaults.display_mode != MONO)
      {
        // �E�J�������g�p����Ȃ�
        if (defaults.camera_right >= 0)
        {
          if (defaults.camera_right != defaults.camera_left
            ? cam->open(defaults.camera_right, eyeR)
            : !defaults.camera_right_movie.empty()
            ? cam->open(defaults.camera_right_movie, eyeR)
            : false)
          {
            // �E�J�����̉摜�T�C�Y�𓾂�
            size[eyeR][0] = cam->getWidth(eyeR);
            size[eyeR][1] = cam->getHeight(eyeR);
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
          size[eyeR][0] = size[eyeL][0];
          size[eyeR][1] = size[eyeL][1];
        }
      }
    }

    // �J�����ŎQ�Ƃ���ϊ��s��̕[��I������
    camera->selectTable(localAttitude.get(), remoteAttitude.get());

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
    (GLfloat(size[eyeL][0]) / GLfloat(size[eyeL][1])),
    (GLfloat(size[eyeR][0]) / GLfloat(size[eyeR][1]))
  };

  // �e�N�X�`���̋��E�̏���
  const GLenum border(defaults.camera_texture_repeat ? GL_REPEAT : GL_CLAMP_TO_BORDER);

  // ���̔w�i�摜��ۑ�����e�N�X�`������������
  glBindTexture(GL_TEXTURE_2D, texture[eyeL]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[eyeL][0], size[eyeL][1], 0,
    GL_BGR, GL_UNSIGNED_BYTE, image[eyeL]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, border);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, border);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  // �E�̃e�N�X�`�������̃e�N�X�`���ƈقȂ�Ȃ�
  if (texture[eyeR] != texture[eyeL])
  {
    // �E�̔w�i�摜��ۑ�����e�N�X�`������������
    glBindTexture(GL_TEXTURE_2D, texture[eyeR]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[eyeR][0], size[eyeR][1], 0,
      GL_BGR, GL_UNSIGNED_BYTE, image[eyeR]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, border);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
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

  // ���[�J���̎p���̎p���̃C���f�b�N�X�𓾂�
  int localAttitudeIndex[eyeCount];
  for (int eye = 0; eye < eyeCount; ++eye) localAttitudeIndex[eye] = localAttitude->push(ggIdentity());

  // Leap Motion �� listener �� controller �����
  LeapListener listener(localAttitude.get());

  // Leap Motion �� listener �� controller ����C�x���g���󂯎��悤�ɂ���
  Leap::Controller controller;
  controller.addListener(listener);
  controller.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);
  controller.setPolicy(Leap::Controller::POLICY_ALLOW_PAUSE_RESUME);

  // �V�[���̕ϊ��s��� Leap Motion �Ő���ł���悤�ɂ���
  Scene::selectController(&listener);

  // �V�[���ŎQ�Ƃ���ϊ��s��̕\��I������
  Scene::selectTable(localAttitude.get(), remoteAttitude.get());

  // �V�[���O���t
  Scene scene(defaults.scene, simple);
  Scene target(defaults.target, simple);
  Scene remote(defaults.remote, simple);

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
  const GgSimpleLightBuffer light(lightData);

  // �ގ�
  const GgSimpleMaterialBuffer material(materialData);

  // �J�����O
  glCullFace(GL_BACK);

  // �A���t�@�u�����f�B���O
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // �`���
  const int drawCount(defaults.display_mode == MONO ? 1 : 2);

  // �E�B���h�E���J���Ă���Ԃ���Ԃ��`�悷��
  while (!window.shouldClose())
  {
    // ���J���������b�N���ĉ摜��]������
    camera->transmit(eyeL, texture[eyeL], size[eyeL]);

    // �E�̃e�N�X�`�����L���ɂȂ��Ă����
    if (texture[eyeR] != texture[eyeL])
    {
      // �E�J���������b�N���ĉ摜��]������
      camera->transmit(eyeR, texture[eyeR], size[eyeR]);
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
        const GgQuaternion qo(window.getQo(eye));
        const GgMatrix mo(qo.getMatrix());

        // ���[�J���̃w�b�h�g���b�L���O�������L�������ɕۑ�����
        localAttitude->set(localAttitudeIndex[eye], mo);

        // �����[�g�̃w�b�h�g���b�L���O�̕ϊ��s��
        GgMatrix mr;

        // �w�i��`��
        rect.draw(texture[eye], defaults.camera_tracking
          ? (camera->isOperator() ? mr = mo * camera->getRemoteAttitude(eye).transpose().get() : mo)
          : ggIdentity(), window.getSamples());

        // �}�`�ƏƏ��̕`��ݒ�
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);

        // �`��p�̃V�F�[�_�v���O�����̎g�p�J�n
        simple.use();
        simple.selectLight(light);
        simple.selectMaterial(material);

        // �}�`��`�悷��
        if (window.showScene) scene.draw(window.getMp(eye), mo * window.getMv(eye), window.getMm());

        // ���[�J���̏Ə���`�悷��
        if (window.showTarget) target.draw(window.getMp(eye), window.getMv(eye));

        // �l�b�g���[�N���L���̎�
        if (window.showTarget && camera->useNetwork())
        {
          // �����[�g�̉�]�ϊ��s����g���ă����[�g�̏Ə���`�悷��
          remote.draw(window.getMp(eye), mr * window.getMv(eye));
        }

        // �Жڂ̏�������������
        window.commit(eye);
      }
    }

    // �o�b�t�@�����ւ���
    window.swapBuffers();
  }

  // �w�i�摜�p�̃e�N�X�`�����폜����
  glDeleteBuffers(1, &texture[eyeL]);
  if (texture[eyeR] != texture[eyeL]) glDeleteBuffers(1, &texture[eyeR]);
}

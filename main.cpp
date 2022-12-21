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

// ��`
#include "Rect.h"

// �t�@�C���_�C�A���O
#include "nfd.h"

//
// ���C���v���O����
//
int main(int argc, const char* const* const argv)
{
  // ������ݒ�t�@�C�����Ɏg���i�w�肳��Ă��Ȃ���� config.json �ɂ���j
  const char* config_file(argc > 1 ? argv[1] : "config.json");

  // �ݒ�f�[�^
  Config defaults;

  // �ݒ�t�@�C����ǂݍ��� (������Ȃ���������)
  if (!defaults.load(config_file)) defaults.save(config_file);

  // GLFW ������������
  if (glfwInit() == GLFW_FALSE)
  {
    // GLFW �̏������Ɏ��s����
    NOTIFY("GLFW �̏������Ɏ��s���܂����B");
    return EXIT_FAILURE;
  }

  // �v���O�����I�����ɂ� GLFW ���I������
  atexit(glfwTerminate);

  // �E�B���h�E���J��
  Window window(defaults);

  // �E�B���h�E�I�u�W�F�N�g����������Ȃ���ΏI������
  if (!window.get()) return EXIT_FAILURE;

  // ���L���������m�ۂ���
  if (!Scene::initialize(defaults))
  {
    // ���L�������̊m�ۂɎ��s����
    NOTIFY("�ϊ��s���ێ����鋤�L���������m�ۂł��܂���ł����B");
    return EXIT_FAILURE;
  }

  // �w�i�摜�̃f�[�^
  const GLubyte* image[camCount]{ nullptr };

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
      CamRemote* const cam(new CamRemote(defaults.remote_texture_width,
        defaults.remote_texture_height, defaults.remote_texture_reshape));

      // ���������J�������L�^���Ă���
      camera.reset(cam);

      // ���c�ґ����N������
      if (cam->open(defaults.port, defaults.address.c_str(), defaults.remote_fov.data(),
        defaults.remote_texture_samples) < 0)
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
      CamCv* const cam{ new CamCv };

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
        if (!cam->open(defaults.camera_left, camL, defaults.capture_codec.data(),
          defaults.capture_width, defaults.capture_height, defaults.capture_fps))
        {
          NOTIFY("���̃J�������g�p�ł��܂���B");
          return EXIT_FAILURE;
        }
      }

      // ���J�����̃T�C�Y�𓾂�
      size[camL][0] = cam->getWidth(camL);
      size[camL][1] = cam->getHeight(camL);

      // �E�J�����̃T�C�Y�� 0 �ɂ��Ă���
      size[camR][0] = size[camR][1] = 0;

      // ���̎��\�����s���Ƃ�
      if (defaults.display_mode != MONO)
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
          if (!cam->open(defaults.camera_right, camR, defaults.capture_codec.data(),
            defaults.capture_width, defaults.capture_height, defaults.capture_fps))
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
        CamOv* const cam(new CamOv);

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
        CamImage* const cam{ new CamImage };

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

        // �E�J�����̃T�C�Y�� 0 �ɂ��Ă���
        size[camR][0] = size[camR][1] = 0;

        // ���̎��\�����s���Ƃ�
        if (defaults.display_mode != MONO)
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

  // ���k�ݒ�
  camera->setQuality(defaults.remote_texture_quality);

  // �L���v�`���Ԋu
  camera->setInterval(defaults.capture_fps);

  // �E�B���h�E�ɂ��̃J���������ѕt����
  window.setControlCamera(camera.get());

  // �e�N�X�`���̋��E�̏���
  const GLenum border{ static_cast<GLenum>(defaults.camera_texture_repeat ? GL_REPEAT : GL_CLAMP_TO_BORDER) };

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
  if (size[camR][0] > 0)
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

#if defined(IMGUI_VERSION)
  //
  // ImGui �̏����ݒ�
  //

  //ImGuiIO& io = ImGui::GetIO();
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  //ImGui::StyleColorsDark();
  //ImGui::StyleColorsClassic();

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
  // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
  // - Read 'docs/FONTS.txt' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
  //io.Fonts->AddFontDefault();
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
  //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
  //IM_ASSERT(font != NULL);

  // �m�F�E�C���h�E���o�����R
  enum class ConfirmReason : int
  {
    NONE = 0,
    RESET,
    QUIT
  } confirmReason{ ConfirmReason::NONE };

  // �t�@�C���G���[�̗��R
  enum class ErrorReason : int
  {
    NONE = 0,
    READ,
    WRITE
  } errorReason{ ErrorReason::NONE };
#endif

  // �E�B���h�E���J���Ă���Ԃ���Ԃ��`�悷��
  while (window)
  {
#if defined(IMGUI_VERSION)
    //
    // ���[�U�C���^�t�F�[�X
    //
    ImGui::NewFrame();

    // �R���g���[���p�l��
    ImGui::SetNextWindowPos(ImVec2(4, 4), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(276, 54), ImGuiCond_Once);
    ImGui::Begin("Control panel");

    // �ݒ�t�@�C�����̃t�B���^
    constexpr nfdfilteritem_t configFilter[]{ "JSON", "json" };

    if (ImGui::Button("Load"))
    {
      nfdchar_t* filepath{ NULL };

      // �t�@�C���_�C�A���O���J��
      if (NFD_OpenDialog(&filepath, configFilter, 1, NULL) == NFD_OKAY)
      {
        if (defaults.load(filepath))
          window.reset();
        else
          errorReason = ErrorReason::READ;
        NFD_FreePath(filepath);
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
      // �t�@�C���_�C�A���O���瓾��p�X
      nfdchar_t* filepath{ NULL };

      // �t�@�C���_�C�A���O���J��
      if (NFD_SaveDialog(&filepath, configFilter, 1, NULL, "*.json") == NFD_OKAY)
      {
        if (!window.save(filepath)) errorReason = ErrorReason::WRITE;
        NFD_FreePath(filepath);
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) confirmReason = ConfirmReason::RESET;
    ImGui::SameLine();
    if (ImGui::Button("Quit"))  confirmReason = ConfirmReason::QUIT;
    ImGui::SameLine();
    ImGui::Text("%7.2f fps", ImGui::GetIO().Framerate);
    ImGui::End();

    // �m�F����Ƃ�
    if (static_cast<int>(confirmReason))
    {
      // �m�F�E�B���h�E
      ImGui::SetNextWindowPos(ImVec2(96, 40), ImGuiCond_Once);
      ImGui::SetNextWindowSize(ImVec2(96, 54), ImGuiCond_Once);
      bool confirm{ true };
      ImGui::Begin("Confirm", &confirm);
      if (!confirm)
        confirmReason = ConfirmReason::NONE;
      else
      {
        if (ImGui::Button("OK"))
        {
          switch (confirmReason)
          {
          case ConfirmReason::RESET:
            window.reset();
            confirmReason = ConfirmReason::NONE;
            break;
          case ConfirmReason::QUIT:
            window.setClose(GLFW_TRUE);
            confirmReason = ConfirmReason::NONE;
            break;
          default:
            confirmReason = ConfirmReason::NONE;
            break;
          }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) confirmReason = ConfirmReason::NONE;
      }
      ImGui::End();
    }

    // �G���[���o���Ƃ�
    if (static_cast<int>(errorReason))
    {
      // �G���[�E�B���h�E
      ImGui::SetNextWindowPos(ImVec2(8, 40), ImGuiCond_Once);
      ImGui::SetNextWindowSize(ImVec2(136, 72), ImGuiCond_Once);
      bool confirm{ true };
      ImGui::Begin("Error", &confirm);
      if (!confirm)
        errorReason = ErrorReason::NONE;
      else
      {
        switch (errorReason)
        {
        case ErrorReason::READ:
          ImGui::Text("File read error.");
          break;
        case ErrorReason::WRITE:
          ImGui::Text("File write error.");
          break;
        default:
          errorReason = ErrorReason::NONE;
          break;
        }
        if (ImGui::Button("OK")) errorReason = ErrorReason::NONE;
      }
      ImGui::End();
    }

    ImGui::Render();
#endif

    // ���J���������b�N���ĉ摜��]������
    camera->transmit(camL, texture[camL], size[camL]);

    // �E�̃e�N�X�`�����L���ɂȂ��Ă����
    if (size[camR] > 0)
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

        // �X�N���[���̊i�q�Ԋu
        rect.setGap(window.getGap());

        // �X�N���[���̃T�C�Y�ƒ��S�ʒu
        rect.setScreen(window.getScreen(eye));

        // �œ_����
        rect.setFocal(window.getFocal());

        // �w�i�e�N�X�`���̔��a�ƒ��S�ʒu
        rect.setCircle(window.getCircle(), window.getOffset(eye));

        // ���[�J���̃w�b�h�g���b�L���O�̕ϊ��s��
        const GgMatrix&& mo(defaults.camera_tracking ? window.getMo(eye) * window.getQa(eye).getMatrix() : window.getQa(eye).getMatrix());

        // �����[�g�̃w�b�h�g���b�L���O�̕ϊ��s��
        const GgMatrix&& mr(mo * Scene::getRemoteAttitude(eye));

        // �w�i��`��
        rect.draw(texture[eye], defaults.remote_stabilize ? mr : mo, window.getSamples());

        // �}�`�ƏƏ��̕`��ݒ�
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);

        // �`��p�̃V�F�[�_�v���O�����̎g�p�J�n
        simple.use();
        simple.selectLight(light);
        simple.selectMaterial(material);

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
  glDeleteBuffers(1, &texture[camL]);
  if (texture[camR] != texture[camL]) glDeleteBuffers(1, &texture[camR]);
}

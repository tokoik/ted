//
// TelExistence Display System
//

// ウィンドウ関連の処理
#include "Window.h"

// ネットワーク関連の処理
#include "Network.h"

// カメラ関連の処理
#include "CamCv.h"
#include "CamOv.h"
#include "CamImage.h"
#include "CamRemote.h"

// シーングラフ
#include "Scene.h"

// 矩形
#include "Rect.h"

// ファイルダイアログ
#include "nfd.h"

//
// メインプログラム
//
int main(int argc, const char* const* const argv)
{
  // 引数を設定ファイル名に使う（指定されていなければ config.json にする）
  const char* config_file(argc > 1 ? argv[1] : "config.json");

  // 設定データ
  Config defaults;

  // 設定ファイルを読み込む (見つからなかったら作る)
  if (!defaults.load(config_file)) defaults.save(config_file);

  // GLFW を初期化する
  if (glfwInit() == GLFW_FALSE)
  {
    // GLFW の初期化に失敗した
    NOTIFY("GLFW の初期化に失敗しました。");
    return EXIT_FAILURE;
  }

  // プログラム終了時には GLFW を終了する
  atexit(glfwTerminate);

  // ウィンドウを開く
  Window window(defaults);

  // ウィンドウオブジェクトが生成されなければ終了する
  if (!window.get()) return EXIT_FAILURE;

  // 共有メモリを確保する
  if (!Scene::initialize(defaults))
  {
    // 共有メモリの確保に失敗した
    NOTIFY("変換行列を保持する共有メモリが確保できませんでした。");
    return EXIT_FAILURE;
  }

  // 背景画像のデータ
  const GLubyte* image[camCount]{ nullptr };

  // 背景画像のサイズ
  GLsizei size[camCount][2];

  // 背景画像を取得するカメラ
  std::shared_ptr<Camera> camera(nullptr);

  // ネットワークを使用する場合
  if (defaults.role != STANDALONE)
  {
    // winsock2 を初期化する
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
    {
      NOTIFY("Windows Sockets 2 の初期化に失敗しました。");
      return EXIT_FAILURE;
    }

    // winsock2 の終了処理を登録する
    atexit(reinterpret_cast<void(*)()>(WSACleanup));

    // 操縦者として動作する場合
    if (defaults.role == OPERATOR)
    {
      // リモートカメラからキャプチャするためのダミーカメラを使う
      CamRemote* const cam(new CamRemote(defaults.remote_texture_width,
        defaults.remote_texture_height, defaults.remote_texture_reshape));

      // 生成したカメラを記録しておく
      camera.reset(cam);

      // 操縦者側を起動する
      if (cam->open(defaults.port, defaults.address.c_str(), defaults.remote_fov.data(),
        defaults.remote_texture_samples) < 0)
      {
        NOTIFY("作業者側のデータを受け取れません。");
        return EXIT_FAILURE;
      }

      // 画像サイズを設定する
      size[camL][0] = cam->getWidth(camL);
      size[camL][1] = cam->getHeight(camL);
      size[camR][0] = cam->getWidth(camR);
      size[camR][1] = cam->getHeight(camR);
    }
  }

  // ダミーカメラが設定されていなければ
  if (!camera)
  {
    // 左カメラから動画を入力するとき
    if (defaults.camera_left >= 0 || !defaults.camera_left_movie.empty())
    {
      // 左カメラに OpenCV のキャプチャデバイスを使う
      CamCv* const cam{ new CamCv };

      // 生成したカメラを記録しておく
      camera.reset(cam);

      // ムービーファイルが指定されていれば
      if (!defaults.camera_left_movie.empty())
      {
        // ムービーファイルを開く
        if (!cam->open(defaults.camera_left_movie, camL))
        {
          NOTIFY("左の動画ファイルが使用できません。");
          return EXIT_FAILURE;
        }
      }
      else
      {
        // カメラデバイスを開く
        if (!cam->open(defaults.camera_left, camL, defaults.capture_codec.data(),
          defaults.capture_width, defaults.capture_height, defaults.capture_fps))
        {
          NOTIFY("左のカメラが使用できません。");
          return EXIT_FAILURE;
        }
      }

      // 左カメラのサイズを得る
      size[camL][0] = cam->getWidth(camL);
      size[camL][1] = cam->getHeight(camL);

      // 右カメラのサイズは 0 にしておく
      size[camR][0] = size[camR][1] = 0;

      // 立体視表示を行うとき
      if (defaults.display_mode != MONO)
      {
        // 右カメラに左と異なるムービーファイルが指定されていれば
        if (!defaults.camera_right_movie.empty() && defaults.camera_right_movie != defaults.camera_left_movie)
        {
          // ムービーファイルを開く
          if (!cam->open(defaults.camera_right_movie, camR))
          {
            NOTIFY("右の動画ファイルが使用できません。");
            return EXIT_FAILURE;
          }

          // 右カメラのサイズを得る
          size[camR][0] = cam->getWidth(camR);
          size[camR][1] = cam->getHeight(camR);
        }
        else if (defaults.camera_right >= 0 && defaults.camera_right != defaults.camera_left)
        {
          // カメラデバイスを開く
          if (!cam->open(defaults.camera_right, camR, defaults.capture_codec.data(),
            defaults.capture_width, defaults.capture_height, defaults.capture_fps))
          {
            NOTIFY("右のカメラが使用できません。");
            return EXIT_FAILURE;
          }

          // 右カメラのサイズを得る
          size[camR][0] = cam->getWidth(camR);
          size[camR][1] = cam->getHeight(camR);
        }
      }
    }
    else
    {
      // 右カメラだけを使うなら
      if (defaults.camera_right >= 0)
      {
        // Ovrvision Pro を使う
        CamOv* const cam(new CamOv);

        // 生成したカメラを記録しておく
        camera.reset(cam);

        // Ovrvision Pro を開く
        if (cam->open(static_cast<OVR::Camprop>(defaults.ovrvision_property)))
        {
          // Ovrvision Pro の画像サイズを得る
          size[camL][0] = cam->getWidth(camL);
          size[camL][1] = cam->getHeight(camL);
          size[camR][0] = cam->getWidth(camR);
          size[camR][1] = cam->getHeight(camR);
        }
        else
        {
          // Ovrvision Pro が使えなかった
          NOTIFY("Ovrvision Pro が使えません。");
          return EXIT_FAILURE;
        }
      }
      else
      {
        // 左カメラに画像ファイルを使う
        CamImage* const cam{ new CamImage };

        // 生成したカメラを記録しておく
        camera.reset(cam);

        // 左の画像が使用可能なら
        if (!cam->open(defaults.camera_left_image, camL))
        {
          // 左の画像ファイルが読み込めなかった
          NOTIFY("左の画像ファイルが使用できません。");
          return EXIT_FAILURE;
        }

        // 左の画像を使用する
        image[camL] = cam->getImage(camL);

        // 左カメラのサイズを得る
        size[camL][0] = camera->getWidth(camL);
        size[camL][1] = camera->getHeight(camL);

        // 右カメラのサイズは 0 にしておく
        size[camR][0] = size[camR][1] = 0;

        // 立体視表示を行うとき
        if (defaults.display_mode != MONO)
        {
          // 右の画像が指定されていれば
          if (!defaults.camera_right_image.empty())
          {
            // 右の画像が使用可能なら
            if (!cam->open(defaults.camera_right_image, camR))
            {
              // 右の画像ファイルが読み込めなかった
              NOTIFY("右の画像ファイルが使用できません。");
              return EXIT_FAILURE;
            }

            // 右の画像を使用する
            image[camR] = cam->getImage(camR);

            // 右の画像のサイズを得る
            size[camR][0] = cam->getWidth(camR);
            size[camR][1] = cam->getHeight(camR);
          }
        }
      }
    }

    // 作業者として動作する場合
    if (defaults.role == WORKER && defaults.port > 0 && !defaults.address.empty())
    {
      // 通信スレッドを起動する
      camera->startWorker(defaults.port, defaults.address.c_str());
    }
  }

  // 圧縮設定
  camera->setQuality(defaults.remote_texture_quality);

  // キャプチャ間隔
  camera->setInterval(defaults.capture_fps);

  // ウィンドウにそのカメラを結び付ける
  window.setControlCamera(camera.get());

  // テクスチャの境界の処理
  const GLenum border{ static_cast<GLenum>(defaults.camera_texture_repeat ? GL_REPEAT : GL_CLAMP_TO_BORDER) };

  // 背景画像を保存するテクスチャ
  GLuint texture[camCount];

  // 左のテクスチャを作成する
  glGenTextures(1, texture + camL);

  // 左の背景画像を保存するテクスチャを準備する
  glBindTexture(GL_TEXTURE_2D, texture[camL]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size[camL][0], size[camL][1], 0,
    GL_BGR, GL_UNSIGNED_BYTE, image[camL]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, border);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, border);
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  // 右のカメラに入力するなら
  if (size[camR][0] > 0)
  {
    // 右のテクスチャを作成する
    glGenTextures(1, texture + camR);

    // 右の背景画像を保存するテクスチャを準備する
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
    // 右のテクスチャは左のテクスチャと同じにする
    texture[camR] = texture[camL];
  }

  // 通常のフレームバッファに戻す
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // 図形描画用のシェーダプログラムを読み込む
  GgSimpleShader simple("simple.vert", "simple.frag");
  if (!simple.get())
  {
    // シェーダが読み込めなかった
    NOTIFY("図形描画用のシェーダファイルの読み込みに失敗しました。");
    return EXIT_FAILURE;
  }

  // シーングラフ
  Scene scene(defaults.scene, simple);

  // 背景描画用の矩形を作成する
  Rect rect(defaults.vertex_shader, defaults.fragment_shader);

  // シェーダプログラム名を調べる
  if (!rect.get())
  {
    // シェーダが読み込めなかった
    NOTIFY("背景描画用のシェーダファイルの読み込みに失敗しました。");
    return EXIT_FAILURE;
  }

  // 光源
  const GgSimpleShader::LightBuffer light(lightData);

  // 材質
  const GgSimpleShader::MaterialBuffer material(materialData);

  // カリング
  glCullFace(GL_BACK);

  // アルファブレンディング
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // 描画回数
  const int drawCount(defaults.display_mode == MONO ? 1 : camCount);

#if defined(IMGUI_VERSION)
  //
  // ImGui の初期設定
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

  // 確認ウインドウを出す理由
  enum class ConfirmReason : int
  {
    NONE = 0,
    RESET,
    QUIT
  } confirmReason{ ConfirmReason::NONE };

  // ファイルエラーの理由
  enum class ErrorReason : int
  {
    NONE = 0,
    READ,
    WRITE
  } errorReason{ ErrorReason::NONE };
#endif

  // ウィンドウが開いている間くり返し描画する
  while (window)
  {
#if defined(IMGUI_VERSION)
    //
    // ユーザインタフェース
    //
    ImGui::NewFrame();

    // コントロールパネル
    ImGui::SetNextWindowPos(ImVec2(4, 4), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(276, 54), ImGuiCond_Once);
    ImGui::Begin("Control panel");

    // 設定ファイル名のフィルタ
    constexpr nfdfilteritem_t configFilter[]{ "JSON", "json" };

    if (ImGui::Button("Load"))
    {
      nfdchar_t* filepath{ NULL };

      // ファイルダイアログを開く
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
      // ファイルダイアログから得るパス
      nfdchar_t* filepath{ NULL };

      // ファイルダイアログを開く
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

    // 確認するとき
    if (static_cast<int>(confirmReason))
    {
      // 確認ウィンドウ
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

    // エラーが出たとき
    if (static_cast<int>(errorReason))
    {
      // エラーウィンドウ
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

    // 左カメラをロックして画像を転送する
    camera->transmit(camL, texture[camL], size[camL]);

    // 右のテクスチャが有効になっていれば
    if (size[camR] > 0)
    {
      // 右カメラをロックして画像を転送する
      camera->transmit(camR, texture[camR], size[camR]);
    }

    // 描画開始
    if (window.start())
    {
      // すべての目について
      for (int eye = 0; eye < drawCount; ++eye)
      {
        // 図形を見せる目を選択する
        window.select(eye);

        // 背景の描画設定
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        // スクリーンの格子間隔
        rect.setGap(window.getGap());

        // スクリーンのサイズと中心位置
        rect.setScreen(window.getScreen(eye));

        // 焦点距離
        rect.setFocal(window.getFocal());

        // 背景テクスチャの半径と中心位置
        rect.setCircle(window.getCircle(), window.getOffset(eye));

        // ローカルのヘッドトラッキングの変換行列
        const GgMatrix&& mo(defaults.camera_tracking ? window.getMo(eye) * window.getQa(eye).getMatrix() : window.getQa(eye).getMatrix());

        // リモートのヘッドトラッキングの変換行列
        const GgMatrix&& mr(mo * Scene::getRemoteAttitude(eye));

        // 背景を描く
        rect.draw(texture[eye], defaults.remote_stabilize ? mr : mo, window.getSamples());

        // 図形と照準の描画設定
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);

        // 描画用のシェーダプログラムの使用開始
        simple.use();
        simple.selectLight(light);
        simple.selectMaterial(material);

        // 図形を描画する
        if (window.showScene) scene.draw(window.getMp(eye), window.getMv(eye) * window.getMo(eye));

        // 片目の処理を完了する
        window.commit(eye);
      }
    }

    // バッファを入れ替える
    window.swapBuffers();
  }

  // 背景画像用のテクスチャを削除する
  glDeleteBuffers(1, &texture[camL]);
  if (texture[camR] != texture[camL]) glDeleteBuffers(1, &texture[camR]);
}

///
/// Quest 3 版で手のモデルを描画するための Wavefront OBJ 形状の実装
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
#include "ObjModel.h"

#include <android/log.h>

#include <cmath>
#include <cstdlib>
#include <map>
#include <sstream>

#define LOG_TAG "TED"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace
{
  ///
  /// assets のテキストファイルを読み込む
  ///
  bool readAsset(AAssetManager* assets, const std::string& name, std::string& text)
  {
    AAsset* const asset{ AAssetManager_open(assets, name.c_str(), AASSET_MODE_BUFFER) };
    if (!asset) return false;
    const auto* const data{ static_cast<const char*>(AAsset_getBuffer(asset)) };
    const off_t length{ AAsset_getLength(asset) };
    if (data) text.assign(data, data + length);
    AAsset_close(asset);
    return data != nullptr;
  }

  ///
  /// シェーダのコンパイル
  ///
  GLuint compile(GLenum type, const char* source)
  {
    const GLuint shader{ glCreateShader(type) };
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint status{ GL_FALSE };
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
      char log[1024]{};
      glGetShaderInfoLog(shader, sizeof log, nullptr, log);
      LOGE("shader compile error: %s", log);
      glDeleteShader(shader);
      return 0;
    }
    return shader;
  }

  /// バーテックスシェーダ
  const char* const vertexSource{ R"(#version 300 es
uniform mat4 mvp;
uniform mat4 mv;
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
out vec3 n;
out vec3 p;
void main()
{
  n = mat3(mv) * normal;
  p = (mv * vec4(position, 1.0)).xyz;
  gl_Position = mvp * vec4(position, 1.0);
}
)" };

  /// フラグメントシェーダ (視点に置いた光源による拡散反射と環境光)
  const char* const fragmentSource{ R"(#version 300 es
precision mediump float;
uniform vec3 color;
uniform float gamma;
in vec3 n;
in vec3 p;
out vec4 fragment;
void main()
{
  vec3 nn = normalize(n);
  vec3 l = normalize(-p);
  float diffuse = abs(dot(nn, l));
  vec3 linear = pow(color, vec3(gamma));
  fragment = vec4(linear * (0.25 + 0.75 * diffuse), 1.0);
}
)" };
}

//
// デストラクタ
//
ObjShader::~ObjShader()
{
  destroy();
}

//
// シェーダを作成する
//
bool ObjShader::create()
{
  destroy();

  const GLuint vert{ compile(GL_VERTEX_SHADER, vertexSource) };
  const GLuint frag{ compile(GL_FRAGMENT_SHADER, fragmentSource) };
  if (vert == 0 || frag == 0)
  {
    if (vert) glDeleteShader(vert);
    if (frag) glDeleteShader(frag);
    return false;
  }

  program = glCreateProgram();
  glAttachShader(program, vert);
  glAttachShader(program, frag);
  glLinkProgram(program);
  glDeleteShader(vert);
  glDeleteShader(frag);

  GLint status{ GL_FALSE };
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE)
  {
    LOGE("shader link error");
    destroy();
    return false;
  }

  mvpLoc = glGetUniformLocation(program, "mvp");
  mvLoc = glGetUniformLocation(program, "mv");
  colorLoc = glGetUniformLocation(program, "color");
  gammaLoc = glGetUniformLocation(program, "gamma");
  return true;
}

//
// シェーダを破棄する
//
void ObjShader::destroy()
{
  if (program) glDeleteProgram(program);
  program = 0;
}

//
// シェーダの使用を開始する
//
void ObjShader::use() const
{
  glUseProgram(program);
}

//
// 材質の色に掛けるガンマ値を設定する
//
void ObjShader::setGamma(float gamma) const
{
  glUniform1f(gammaLoc, gamma);
}

//
// 変換行列を設定する
//
void ObjShader::setMatrix(const qm::Mat4& projection, const qm::Mat4& modelview) const
{
  const qm::Mat4 mvp{ projection * modelview };
  glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp.data());
  glUniformMatrix4fv(mvLoc, 1, GL_FALSE, modelview.data());
}

//
// 材質の色を設定する
//
void ObjShader::setColor(const std::array<float, 3>& color) const
{
  glUniform3fv(colorLoc, 1, color.data());
}

//
// デストラクタ
//
ObjModel::~ObjModel()
{
  destroy();
}

//
// assets から OBJ ファイルを読み込む
//
bool ObjModel::load(AAssetManager* assets, const std::string& name)
{
  destroy();

  std::string text;
  if (!readAsset(assets, name, text))
  {
    LOGE("cannot read asset %s", name.c_str());
    return false;
  }

  // assets 内のディレクトリ (材質ファイルの場所)
  const auto slash{ name.find_last_of('/') };
  const std::string directory{ slash == std::string::npos ? "" : name.substr(0, slash + 1) };

  std::vector<std::array<float, 3>> positions, normals;
  std::map<std::string, std::array<float, 3>> materials;

  // 位置 3 要素と法線 3 要素を交互に並べた三角形の頂点
  std::vector<float> vertices;

  const auto startGroup = [this, &vertices](const std::array<float, 3>& color)
  {
    const auto first{ static_cast<GLint>(vertices.size() / 6) };
    if (!groups.empty() && groups.back().count == 0) groups.pop_back();
    groups.push_back(Group{ first, 0, color });
  };
  startGroup(Group{}.color);

  // OBJ の頂点番号 (1 始まり, 負なら末尾から) を配列の添字に直す
  const auto resolve = [](int index, std::size_t size)
  {
    if (index > 0) return index - 1;
    if (index < 0) return static_cast<int>(size) + index;
    return -1;
  };

  std::istringstream lines{ text };
  std::string line;
  while (std::getline(lines, line))
  {
    std::istringstream in{ line };
    std::string op;
    in >> op;

    if (op == "v")
    {
      std::array<float, 3> v{};
      in >> v[0] >> v[1] >> v[2];
      positions.push_back(v);
    }
    else if (op == "vn")
    {
      std::array<float, 3> v{};
      in >> v[0] >> v[1] >> v[2];
      normals.push_back(v);
    }
    else if (op == "mtllib")
    {
      std::string file;
      in >> file;
      std::string mtl;
      if (readAsset(assets, directory + file, mtl))
      {
        std::istringstream mtlLines{ mtl };
        std::string current;
        while (std::getline(mtlLines, line))
        {
          std::istringstream m{ line };
          std::string key;
          m >> key;
          if (key == "newmtl") m >> current;
          else if (key == "Kd" && !current.empty())
          {
            std::array<float, 3> kd{};
            m >> kd[0] >> kd[1] >> kd[2];
            materials[current] = kd;
          }
        }
      }
    }
    else if (op == "usemtl")
    {
      std::string material;
      in >> material;
      const auto it{ materials.find(material) };
      startGroup(it != materials.end() ? it->second : Group{}.color);
    }
    else if (op == "f")
    {
      // 多角形は扇状に三角形分割する
      std::vector<std::pair<int, int>> face;
      std::string token;
      while (in >> token)
      {
        int v{ 0 }, n{ 0 };
        const auto first{ token.find('/') };
        v = std::atoi(token.c_str());
        if (first != std::string::npos)
        {
          const auto second{ token.find('/', first + 1) };
          if (second != std::string::npos) n = std::atoi(token.c_str() + second + 1);
        }
        face.emplace_back(resolve(v, positions.size()), resolve(n, normals.size()));
      }

      for (std::size_t i = 2; i < face.size(); ++i)
      {
        const std::pair<int, int> tri[]{ face[0], face[i - 1], face[i] };
        bool ok{ true };
        for (const auto& c : tri)
          ok = ok && c.first >= 0 && c.first < static_cast<int>(positions.size());
        if (!ok) continue;

        // 法線が無ければ面の法線を使う
        const auto& a{ positions[tri[0].first] };
        const auto& b{ positions[tri[1].first] };
        const auto& c{ positions[tri[2].first] };
        const float e1[]{ b[0] - a[0], b[1] - a[1], b[2] - a[2] };
        const float e2[]{ c[0] - a[0], c[1] - a[1], c[2] - a[2] };
        std::array<float, 3> faceNormal{
          e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0] };
        const float length{ std::sqrt(faceNormal[0] * faceNormal[0] + faceNormal[1] * faceNormal[1]
          + faceNormal[2] * faceNormal[2]) };
        if (length > 0.0f) for (auto& x : faceNormal) x /= length;

        for (const auto& corner : tri)
        {
          const auto& p{ positions[corner.first] };
          const auto& n{ corner.second >= 0 && corner.second < static_cast<int>(normals.size())
            ? normals[corner.second] : faceNormal };
          vertices.insert(vertices.end(), { p[0], p[1], p[2], n[0], n[1], n[2] });
        }
        groups.back().count += 3;
      }
    }
  }

  if (!groups.empty() && groups.back().count == 0) groups.pop_back();
  if (vertices.empty())
  {
    LOGE("no faces in %s", name.c_str());
    groups.clear();
    return false;
  }

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
    vertices.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
    reinterpret_cast<const void*>(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glBindVertexArray(0);

  return true;
}

//
// 描画資源を破棄する
//
void ObjModel::destroy()
{
  if (vbo) glDeleteBuffers(1, &vbo);
  if (vao) glDeleteVertexArrays(1, &vao);
  vbo = vao = 0;
  groups.clear();
}

//
// 描画する
//
void ObjModel::draw(const ObjShader& shader, const qm::Mat4& projection, const qm::Mat4& modelview) const
{
  if (!vao) return;
  shader.setMatrix(projection, modelview);
  glBindVertexArray(vao);
  for (const auto& group : groups)
  {
    shader.setColor(group.color);
    glDrawArrays(GL_TRIANGLES, group.first, group.count);
  }
  glBindVertexArray(0);
}

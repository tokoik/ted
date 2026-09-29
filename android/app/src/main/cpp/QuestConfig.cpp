///
/// Quest 3 版の設定の実装
///
/// @file
/// @author Kohe Tokoi
/// @date September 29, 2026
///
#include "QuestConfig.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

namespace
{
  ///
  /// 設定ファイルの値 (平坦なオブジェクトの値だけを扱う)
  ///
  struct Value
  {
    enum class Type { NUMBER, BOOLEAN, STRING, ARRAY } type{ Type::NUMBER };
    double number{ 0.0 };
    bool boolean{ false };
    std::string string;
    std::vector<double> array;
  };

  ///
  /// 数値・真偽値・文字列・数値配列を値に持つ JSON オブジェクトの最小限の解析器
  ///
  class Parser
  {
    const std::string& text;
    std::size_t pos{ 0 };

    void skip()
    {
      while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
    }

    bool expect(char c)
    {
      skip();
      if (pos < text.size() && text[pos] == c)
      {
        ++pos;
        return true;
      }
      return false;
    }

    bool parseString(std::string& out)
    {
      if (!expect('"')) return false;
      out.clear();
      while (pos < text.size() && text[pos] != '"')
      {
        if (text[pos] == '\\' && pos + 1 < text.size()) ++pos;
        out.push_back(text[pos++]);
      }
      return expect('"');
    }

    bool parseNumber(double& out)
    {
      skip();
      const char* begin{ text.c_str() + pos };
      char* end{ nullptr };
      out = std::strtod(begin, &end);
      if (end == begin) return false;
      pos += static_cast<std::size_t>(end - begin);
      return true;
    }

    bool parseValue(Value& value)
    {
      skip();
      if (pos >= text.size()) return false;

      const char c{ text[pos] };
      if (c == '"')
      {
        value.type = Value::Type::STRING;
        return parseString(value.string);
      }
      if (c == '[')
      {
        ++pos;
        value.type = Value::Type::ARRAY;
        if (expect(']')) return true;
        do
        {
          double number;
          if (!parseNumber(number)) return false;
          value.array.push_back(number);
        } while (expect(','));
        return expect(']');
      }
      if (text.compare(pos, 4, "true") == 0)
      {
        pos += 4;
        value.type = Value::Type::BOOLEAN;
        value.boolean = true;
        return true;
      }
      if (text.compare(pos, 5, "false") == 0)
      {
        pos += 5;
        value.type = Value::Type::BOOLEAN;
        value.boolean = false;
        return true;
      }
      value.type = Value::Type::NUMBER;
      return parseNumber(value.number);
    }

  public:

    explicit Parser(const std::string& text)
      : text{ text }
    {
    }

    bool parse(std::map<std::string, Value>& object)
    {
      if (!expect('{')) return false;
      if (expect('}')) return true;
      do
      {
        std::string key;
        Value value;
        if (!parseString(key) || !expect(':') || !parseValue(value)) return false;
        object[key] = value;
      } while (expect(','));
      return expect('}');
    }
  };

  void getValue(const std::map<std::string, Value>& o, const char* key, int& out)
  {
    const auto it{ o.find(key) };
    if (it == o.end()) return;
    if (it->second.type == Value::Type::NUMBER) out = static_cast<int>(it->second.number);
    else if (it->second.type == Value::Type::BOOLEAN) out = it->second.boolean ? 1 : 0;
  }

  void getValue(const std::map<std::string, Value>& o, const char* key, double& out)
  {
    const auto it{ o.find(key) };
    if (it != o.end() && it->second.type == Value::Type::NUMBER) out = it->second.number;
  }

  void getValue(const std::map<std::string, Value>& o, const char* key, bool& out)
  {
    const auto it{ o.find(key) };
    if (it == o.end()) return;
    if (it->second.type == Value::Type::BOOLEAN) out = it->second.boolean;
    else if (it->second.type == Value::Type::NUMBER) out = it->second.number != 0.0;
  }

  void getValue(const std::map<std::string, Value>& o, const char* key, std::string& out)
  {
    const auto it{ o.find(key) };
    if (it != o.end() && it->second.type == Value::Type::STRING) out = it->second.string;
  }

  void getValue(const std::map<std::string, Value>& o, const char* key, std::array<float, 3>& out)
  {
    const auto it{ o.find(key) };
    if (it == o.end() || it->second.type != Value::Type::ARRAY) return;
    const auto& a{ it->second.array };
    for (std::size_t i = 0; i < out.size() && i < a.size(); ++i) out[i] = static_cast<float>(a[i]);
  }
}

//
// 設定ファイルを読み込む
//
bool QuestConfig::load(const std::string& path)
{
  std::ifstream file(path);
  if (!file) return false;

  std::stringstream buffer;
  buffer << file.rdbuf();
  const std::string text{ buffer.str() };

  std::map<std::string, Value> o;
  if (!Parser(text).parse(o)) return false;

  getValue(o, "host", host);
  getValue(o, "port", port);
  getValue(o, "send_images", send_images);
  getValue(o, "camera_width", camera_width);
  getValue(o, "camera_height", camera_height);
  getValue(o, "transmit_quality", transmit_quality);
  getValue(o, "transmit_fps", transmit_fps);
  getValue(o, "send_interval", send_interval);
  getValue(o, "passthrough", passthrough);
  getValue(o, "show_local_hands", show_local_hands);
  getValue(o, "show_remote_hands", show_remote_hands);
  getValue(o, "remote_hand_position", remote_hand_position);

  // 不正な値は使える範囲に丸める
  port = std::clamp(port, 1, 65534);
  camera_width = std::max(camera_width, 1);
  camera_height = std::max(camera_height, 1);
  transmit_quality = std::clamp(transmit_quality, 0, 100);
  transmit_fps = std::max(transmit_fps, 0.0);
  send_interval = std::max(send_interval, 1);

  return true;
}

//
// 設定ファイルを保存する
//
bool QuestConfig::save(const std::string& path) const
{
  std::ofstream file(path);
  if (!file) return false;

  const auto boolean = [](bool b) { return b ? "true" : "false"; };

  file << "{\n"
    << "  \"host\": \"" << host << "\",\n"
    << "  \"port\": " << port << ",\n"
    << "  \"send_images\": " << boolean(send_images) << ",\n"
    << "  \"camera_width\": " << camera_width << ",\n"
    << "  \"camera_height\": " << camera_height << ",\n"
    << "  \"transmit_quality\": " << transmit_quality << ",\n"
    << "  \"transmit_fps\": " << transmit_fps << ",\n"
    << "  \"send_interval\": " << send_interval << ",\n"
    << "  \"passthrough\": " << boolean(passthrough) << ",\n"
    << "  \"show_local_hands\": " << boolean(show_local_hands) << ",\n"
    << "  \"show_remote_hands\": " << boolean(show_remote_hands) << ",\n"
    << "  \"remote_hand_position\": [ " << remote_hand_position[0] << ", "
    << remote_hand_position[1] << ", " << remote_hand_position[2] << " ]\n"
    << "}\n";

  return static_cast<bool>(file);
}

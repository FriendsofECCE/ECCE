// A small JSON reader for the output of GENSUB_EXPLAIN and similar tool
// output: full JSON syntax (strings with \u escapes, numbers, nested
// arrays and objects), no dependency.  Numbers are kept as their text.
#ifndef MINIJSON_HH
#define MINIJSON_HH

#include <map>
#include <string>
#include <vector>

class MiniJson
{
public:
  enum Type { Null, Bool, Number, String, Array, Object };

  MiniJson() : p_type(Null), p_bool(false) {}

  // False and err set on malformed text or trailing garbage.
  static bool parse(const std::string& text, MiniJson& out, std::string& err);

  Type type() const { return p_type; }
  bool isNull() const { return p_type == Null; }
  bool isString() const { return p_type == String; }
  // String contents (UTF-8), or the number's text; "" for anything else.
  const std::string& str() const { return p_text; }
  bool boolean() const { return p_bool; }
  const std::vector<MiniJson>& items() const { return p_items; }
  const std::map<std::string, MiniJson>& members() const { return p_members; }
  // The member, or a Null value when absent or not an object.
  const MiniJson& get(const std::string& key) const;

  // Compact, key-sorted, ASCII-only text (what python's json.dumps gives
  // with sort_keys and ensure_ascii, apart from spacing).
  std::string dump() const;

private:
  friend class MiniJsonParser;
  Type p_type;
  bool p_bool;
  std::string p_text;
  std::vector<MiniJson> p_items;
  std::map<std::string, MiniJson> p_members;
};

#endif

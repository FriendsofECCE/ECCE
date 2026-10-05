#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "util/MiniJson.H"

using std::string;

class MiniJsonParser
{
public:
  MiniJsonParser(const string& t) : s(t), i(0), depth(0) {}

  bool run(MiniJson& out, string& err)
  {
    ws();
    if (!value(out)) { err = msg; return false; }
    ws();
    if (i != s.size()) { bad("trailing text"); err = msg; return false; }
    return true;
  }

private:
  const string& s;
  size_t i;
  int depth;
  string msg;

  bool bad(const char* what)
  {
    char buf[64];
    snprintf(buf, sizeof buf, " at offset %lu", (unsigned long)i);
    msg = string(what) + buf;
    return false;
  }

  void ws()
  {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' ||
                            s[i] == '\r'))
      i++;
  }

  bool lit(const char* word)
  {
    size_t n = strlen(word);
    if (s.compare(i, n, word) != 0)
      return bad("bad literal");
    i += n;
    return true;
  }

  bool value(MiniJson& v)
  {
    if (i >= s.size())
      return bad("unexpected end");
    char c = s[i];
    if (c == '{') return object(v);
    if (c == '[') return array(v);
    if (c == '"') { v.p_type = MiniJson::String; return string_(v.p_text); }
    if (c == 't') { v.p_type = MiniJson::Bool; v.p_bool = true;
                    return lit("true"); }
    if (c == 'f') { v.p_type = MiniJson::Bool; v.p_bool = false;
                    return lit("false"); }
    if (c == 'n') { v.p_type = MiniJson::Null; return lit("null"); }
    return number(v);
  }

  bool number(MiniJson& v)
  {
    size_t b = i;
    if (i < s.size() && s[i] == '-') i++;
    size_t d = i;
    while (i < s.size() && isdigit((unsigned char)s[i])) i++;
    if (i == d) { i = b; return bad("bad value"); }
    if (i < s.size() && s[i] == '.') {
      i++;
      size_t f = i;
      while (i < s.size() && isdigit((unsigned char)s[i])) i++;
      if (i == f) return bad("bad number");
    }
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
      i++;
      if (i < s.size() && (s[i] == '+' || s[i] == '-')) i++;
      size_t e = i;
      while (i < s.size() && isdigit((unsigned char)s[i])) i++;
      if (i == e) return bad("bad number");
    }
    v.p_type = MiniJson::Number;
    v.p_text = s.substr(b, i - b);
    return true;
  }

  static void utf8(string& out, unsigned long cp)
  {
    if (cp < 0x80) out += (char)cp;
    else if (cp < 0x800) {
      out += (char)(0xC0 | (cp >> 6));
      out += (char)(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
      out += (char)(0xE0 | (cp >> 12));
      out += (char)(0x80 | ((cp >> 6) & 0x3F));
      out += (char)(0x80 | (cp & 0x3F));
    } else {
      out += (char)(0xF0 | (cp >> 18));
      out += (char)(0x80 | ((cp >> 12) & 0x3F));
      out += (char)(0x80 | ((cp >> 6) & 0x3F));
      out += (char)(0x80 | (cp & 0x3F));
    }
  }

  bool hex4(unsigned long& cp)
  {
    if (i + 4 > s.size()) return bad("short \\u escape");
    cp = 0;
    for (int k = 0; k < 4; k++) {
      char c = s[i++];
      cp <<= 4;
      if (c >= '0' && c <= '9') cp |= c - '0';
      else if (c >= 'a' && c <= 'f') cp |= c - 'a' + 10;
      else if (c >= 'A' && c <= 'F') cp |= c - 'A' + 10;
      else return bad("bad \\u escape");
    }
    return true;
  }

  bool string_(string& out)
  {
    out.clear();
    i++;   // opening quote
    while (i < s.size()) {
      unsigned char c = s[i++];
      if (c == '"')
        return true;
      if (c < 0x20) { i--; return bad("control character in string"); }
      if (c != '\\') { out += (char)c; continue; }
      if (i >= s.size()) break;
      char e = s[i++];
      switch (e) {
        case '"': out += '"'; break;
        case '\\': out += '\\'; break;
        case '/': out += '/'; break;
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case 'n': out += '\n'; break;
        case 'r': out += '\r'; break;
        case 't': out += '\t'; break;
        case 'u': {
          unsigned long cp;
          if (!hex4(cp)) return false;
          if (cp >= 0xD800 && cp < 0xDC00) {
            unsigned long lo;
            if (s.compare(i, 2, "\\u") != 0)
              return bad("lone surrogate");
            i += 2;
            if (!hex4(lo)) return false;
            if (lo < 0xDC00 || lo > 0xDFFF)
              return bad("bad surrogate pair");
            cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
          } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
            return bad("lone surrogate");
          }
          utf8(out, cp);
          break;
        }
        default: i--; return bad("bad escape");
      }
    }
    return bad("unterminated string");
  }

  bool array(MiniJson& v)
  {
    if (++depth > 64) return bad("nested too deeply");
    v.p_type = MiniJson::Array;
    i++;
    ws();
    if (i < s.size() && s[i] == ']') { i++; depth--; return true; }
    for (;;) {
      MiniJson item;
      ws();
      if (!value(item)) return false;
      v.p_items.push_back(item);
      ws();
      if (i < s.size() && s[i] == ',') { i++; continue; }
      if (i < s.size() && s[i] == ']') { i++; depth--; return true; }
      return bad("expected , or ]");
    }
  }

  bool object(MiniJson& v)
  {
    if (++depth > 64) return bad("nested too deeply");
    v.p_type = MiniJson::Object;
    i++;
    ws();
    if (i < s.size() && s[i] == '}') { i++; depth--; return true; }
    for (;;) {
      string key;
      MiniJson item;
      ws();
      if (i >= s.size() || s[i] != '"') return bad("expected a key");
      if (!string_(key)) return false;
      ws();
      if (i >= s.size() || s[i] != ':') return bad("expected :");
      i++;
      ws();
      if (!value(item)) return false;
      v.p_members[key] = item;
      ws();
      if (i < s.size() && s[i] == ',') { i++; continue; }
      if (i < s.size() && s[i] == '}') { i++; depth--; return true; }
      return bad("expected , or }");
    }
  }
};

bool MiniJson::parse(const string& text, MiniJson& out, string& err)
{
  out = MiniJson();
  MiniJsonParser p(text);
  return p.run(out, err);
}

const MiniJson& MiniJson::get(const string& key) const
{
  static const MiniJson none;
  if (p_type != Object)
    return none;
  std::map<string, MiniJson>::const_iterator it = p_members.find(key);
  return it == p_members.end() ? none : it->second;
}

static void dumpString(const string& s, string& out)
{
  out += '"';
  for (size_t k = 0; k < s.size(); k++) {
    unsigned char c = s[k];
    switch (c) {
      case '"': out += "\\\""; continue;
      case '\\': out += "\\\\"; continue;
      case '\n': out += "\\n"; continue;
      case '\r': out += "\\r"; continue;
      case '\t': out += "\\t"; continue;
      case '\b': out += "\\b"; continue;
      case '\f': out += "\\f"; continue;
    }
    if (c >= 0x20 && c < 0x7F) { out += (char)c; continue; }
    unsigned long cp = c;
    int extra = 0;
    if (c >= 0xF0) { cp = c & 0x07; extra = 3; }
    else if (c >= 0xE0) { cp = c & 0x0F; extra = 2; }
    else if (c >= 0xC0) { cp = c & 0x1F; extra = 1; }
    for (int n = 0; n < extra && k + 1 < s.size(); n++)
      cp = (cp << 6) | ((unsigned char)s[++k] & 0x3F);
    char buf[16];
    if (cp >= 0x10000) {
      cp -= 0x10000;
      snprintf(buf, sizeof buf, "\\u%04lx\\u%04lx", 0xD800 + (cp >> 10),
               0xDC00 + (cp & 0x3FF));
    } else {
      snprintf(buf, sizeof buf, "\\u%04lx", cp);
    }
    out += buf;
  }
  out += '"';
}

string MiniJson::dump() const
{
  string out;
  switch (p_type) {
    case Null: out = "null"; break;
    case Bool: out = p_bool ? "true" : "false"; break;
    case Number: out = p_text; break;
    case String: dumpString(p_text, out); break;
    case Array:
      out = "[";
      for (size_t k = 0; k < p_items.size(); k++)
        out += (k ? "," : "") + p_items[k].dump();
      out += "]";
      break;
    case Object: {
      out = "{";
      bool first = true;
      for (std::map<string, MiniJson>::const_iterator it = p_members.begin();
           it != p_members.end(); ++it) {
        if (!first) out += ",";
        first = false;
        dumpString(it->first, out);
        out += ":" + it->second.dump();
      }
      out += "}";
      break;
    }
  }
  return out;
}

#include "DetailsRules.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

namespace details {

using std::string;

string trim(const string& s)
{
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == string::npos) return "";
  return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

Strings split(const string& s, char sep)
{
  Strings out;
  string cur;
  for (char c : s) {
    if (c == sep) { out.push_back(cur); cur.clear(); }
    else cur += c;
  }
  out.push_back(cur);
  return out;
}

bool toNumber(const string& s, double& v)
{
  string t = trim(s);
  if (t.empty()) return false;
  char *end = 0;
  v = strtod(t.c_str(), &end);
  return end && *end == 0;
}

string pyFloatStr(double v)
{
  if (std::isnan(v)) return "nan";
  if (std::isinf(v)) return v < 0 ? "-inf" : "inf";
  char buf[64];
  for (int p = 1; p <= 17; p++) {
    snprintf(buf, sizeof buf, "%.*e", p - 1, v);
    if (strtod(buf, 0) == v) break;
  }
  // buf is d.ddde[+-]XX holding the shortest round-trip digits.
  string m(buf);
  size_t e = m.find('e');
  string mant = m.substr(0, e);
  int exp10 = atoi(m.c_str() + e + 1);
  bool neg = mant[0] == '-';
  if (neg) mant.erase(0, 1);
  string digits;
  for (char c : mant) if (c != '.') digits += c;
  while (digits.size() > 1 && digits.back() == '0') digits.pop_back();
  string r;
  if (exp10 < -4 || exp10 >= 16) {
    r = digits.substr(0, 1);
    if (digits.size() > 1) r += "." + digits.substr(1);
    char eb[16];
    snprintf(eb, sizeof eb, "e%c%02d", exp10 < 0 ? '-' : '+', std::abs(exp10));
    r += eb;
  } else if (exp10 >= 0) {
    if ((int)digits.size() <= exp10 + 1)
      r = digits + string(exp10 + 1 - digits.size(), '0') + ".0";
    else
      r = digits.substr(0, exp10 + 1) + "." + digits.substr(exp10 + 1);
  } else {
    r = "0." + string(-exp10 - 1, '0') + digits;
  }
  return (neg ? "-" : "") + r;
}

static string stripComment(const string& raw)
{
  string s = raw;
  bool inq = false;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '"') inq = !inq;
    if (s[i] == '#' && !inq) { s.erase(i); break; }
  }
  return trim(s);
}

bool parseLine(const string& raw, Line& out)
{
  string s = stripComment(raw);
  if (s.empty()) return false;
  Strings tok;
  string cur;
  bool inq = false;
  for (char c : s) {
    if (c == '"') { inq = !inq; cur += c; continue; }
    if ((c == ' ' || c == '\t') && !inq) {
      if (!cur.empty()) tok.push_back(cur);
      cur.clear();
    } else cur += c;
  }
  if (!cur.empty()) tok.push_back(cur);
  out = Line();
  out.cmd = tok[0];
  for (size_t i = 1; i < tok.size(); i++) {
    size_t eq = tok[i].find('=');
    if (eq == string::npos) continue;
    string v = tok[i].substr(eq + 1);
    v.erase(std::remove(v.begin(), v.end(), '"'), v.end());
    out.kv[tok[i].substr(0, eq)] = v;
  }
  return true;
}

bool Description::parse(std::istream& in)
{
  string raw;
  int n = 0;
  while (std::getline(in, raw)) {
    n++;
    string s = stripComment(raw);
    if (s.empty()) continue;
    if (s.compare(0, 5, "list ") == 0 || s.compare(0, 5, "list\t") == 0) {
      size_t eq = s.find('=');
      string name = eq == string::npos ? "" : trim(s.substr(4, eq - 4));
      if (name.empty()) {
        error = "line " + std::to_string(n) + ": list needs NAME = a|b|c";
        return false;
      }
      Strings items = split(s.substr(eq + 1), '|');
      for (string& i : items) i = trim(i);
      lists[name] = items;
      continue;
    }
    Line L;
    if (parseLine(raw, L)) lines.push_back(L);
  }
  return true;
}

bool Description::load(const string& path)
{
  std::ifstream in(path);
  if (!in) { error = "cannot read " + path; return false; }
  return parse(in);
}

// ---- expressions ---------------------------------------------------------

namespace {

struct Tok { string t; bool word; };

bool isOpChar(char c) { return strchr("=!<>&|()", c) != 0; }

bool lex(const string& e, std::vector<Tok>& out)
{
  size_t i = 0;
  while (i < e.size()) {
    char c = e[i];
    if (c == ' ' || c == '\t') { i++; continue; }
    if (isOpChar(c)) {
      string two = e.substr(i, 2);
      if (two == "==" || two == "!=" || two == "<=" || two == ">=" ||
          two == "&&" || two == "||") {
        out.push_back({two, false});
        i += 2;
      } else if (c == '=' || c == '&' || c == '|') {
        return false;
      } else {
        out.push_back({string(1, c), false});
        i++;
      }
      continue;
    }
    size_t j = i;
    while (j < e.size() && e[j] != ' ' && e[j] != '\t' && !isOpChar(e[j])) j++;
    out.push_back({e.substr(i, j - i), true});
    i = j;
  }
  return true;
}

struct Parser {
  std::vector<Tok> t;
  size_t p = 0;
  const Lookup& lookup;
  const Lists& lists;
  string err;
  Parser(const Lookup& lk, const Lists& ls) : lookup(lk), lists(ls) {}

  bool at(const char *s) { return p < t.size() && !t[p].word && t[p].t == s; }
  bool atWord(const char *s) { return p < t.size() && t[p].word && t[p].t == s; }

  bool orExpr()
  {
    bool r = andExpr();
    while (at("||")) { p++; bool b = andExpr(); r = r || b; }
    return r;
  }
  bool andExpr()
  {
    bool r = notExpr();
    while (at("&&")) { p++; bool b = notExpr(); r = r && b; }
    return r;
  }
  bool notExpr()
  {
    if (at("!")) { p++; return !notExpr(); }
    if (at("(")) {
      p++;
      bool r = orExpr();
      if (at(")")) p++; else err = "missing )";
      return r;
    }
    return cmp();
  }
  string value(const string& w)
  {
    string v;
    if (!w.empty() && w[0] == '$') {
      if (lookup(w.substr(1), v)) return v;
      return "";
    }
    if (lookup(w, v)) return v;
    return "";
  }
  string literal(const string& w)
  {
    string v;
    if (!w.empty() && w[0] == '$' && lookup(w.substr(1), v)) return v;
    if (w.size() > 1 && w[0] == '@' && isdigit((unsigned char)w[1]) &&
        lookup(w, v)) return v;
    return w;
  }
  bool cmp()
  {
    if (p >= t.size() || !t[p].word) {
      err = "operand expected";
      p = t.size();
      return false;
    }
    string lv = value(t[p++].t);
    bool neg = false;
    if (atWord("not") && p + 1 < t.size() && t[p + 1].t == "in") {
      neg = true;
      p++;
    }
    if (atWord("in")) {
      p++;
      if (p >= t.size() || t[p].t.size() < 2 || t[p].t[0] != '@') {
        err = "in needs @LIST";
        return false;
      }
      string name = t[p++].t.substr(1);
      Lists::const_iterator it = lists.find(name);
      if (it == lists.end()) { err = "unknown list @" + name; return false; }
      bool in = std::find(it->second.begin(), it->second.end(), lv) !=
                it->second.end();
      return neg ? !in : in;
    }
    static const char *ops[] = {"==", "!=", "<=", ">=", "<", ">"};
    for (const char *op : ops) {
      if (!at(op)) continue;
      p++;
      if (p >= t.size() || !t[p].word) { err = "operand expected"; return false; }
      string rv = literal(t[p++].t);
      double a, b;
      int c;
      if (toNumber(lv, a) && toNumber(rv, b)) c = a < b ? -1 : a > b ? 1 : 0;
      else c = lv.compare(rv);
      string o = op;
      return o == "==" ? c == 0 : o == "!=" ? c != 0 : o == "<=" ? c <= 0 :
             o == ">=" ? c >= 0 : o == "<" ? c < 0 : c > 0;
    }
    double a;
    return toNumber(lv, a) ? a != 0 : !lv.empty();
  }
};

}  // namespace

bool evalExpr(const string& expr, const Lookup& lookup, const Lists& lists,
              string *err)
{
  if (trim(expr).empty()) return true;
  Parser ps(lookup, lists);
  if (!lex(expr, ps.t)) {
    if (err) *err = "bad operator in: " + expr;
    return false;
  }
  bool r = ps.orExpr();
  if (ps.err.empty() && ps.p < ps.t.size()) ps.err = "unexpected " + ps.t[ps.p].t;
  if (!ps.err.empty()) {
    if (err) *err = ps.err + " in: " + expr;
    return false;
  }
  return r;
}

string substitute(const string& s, const Lookup& lookup)
{
  string out;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] != '$') { out += s[i]; continue; }
    size_t j = i + 1;
    while (j < s.size() && (isalnum((unsigned char)s[j]) || s[j] == '_')) j++;
    string v;
    if (j > i + 1 && lookup(s.substr(i + 1, j - i - 1), v)) {
      out += v;
      i = j - 1;
    } else out += s[i];
  }
  return out;
}

// ---- ranges --------------------------------------------------------------

bool rangeBounds(const string& range, bool& hasLo, double& lo, bool& hasHi,
                 double& hi)
{
  hasLo = hasHi = false;
  if (range.empty()) return true;
  size_t dots = range.find("..");
  if (dots == string::npos || range.size() < 2) return false;
  hasLo = toNumber(range.substr(1, dots - 1), lo);
  hasHi = toNumber(range.substr(dots + 2, range.size() - dots - 3), hi);
  return true;
}

// templates.py takes a bound as int() when it parses as one, else float(),
// and prints it with str(): "[1..)" says 1, "[1.0..)" says 1.0.
static string boundStr(const string& text, double v)
{
  size_t b = text.find_first_not_of(" \t");
  size_t e = text.find_last_not_of(" \t");
  string t = b == string::npos ? "" : text.substr(b, e - b + 1);
  size_t d = (!t.empty() && (t[0] == '-' || t[0] == '+')) ? 1 : 0;
  bool isInt = t.size() > d &&
               t.find_first_not_of("0123456789", d) == string::npos;
  return isInt ? std::to_string((long long)v) : pyFloatStr(v);
}

bool inRange(const string& range, double v, string& msg)
{
  msg.clear();
  bool hasLo, hasHi;
  double lo = 0, hi = 0;
  if (!rangeBounds(range, hasLo, lo, hasHi, hi)) return true;
  if (!hasLo && !hasHi) return true;
  size_t dots = range.find("..");
  string loText = range.substr(1, dots - 1);
  string hiText = range.substr(dots + 2, range.size() - dots - 3);
  bool ok = true;
  if (hasLo) {
    if (range[0] == '(') {
      if (v <= lo) { msg = "greater than " + boundStr(loText, lo) + "."; ok = false; }
    } else if (v < lo) {
      msg = "greater than or equal to " + boundStr(loText, lo) + ".";
      ok = false;
    }
  }
  if (hasHi) {
    if (range.back() == ')') {
      if (v >= hi) { msg = "less than " + boundStr(hiText, hi) + "."; ok = false; }
    } else if (v > hi) {
      msg = "less than or equal to " + boundStr(hiText, hi) + ".";
      ok = false;
    }
  }
  return ok;
}

}  // namespace details

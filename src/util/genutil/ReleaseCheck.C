#include "util/ReleaseCheck.H"

#include "util/Ecce.H"

#include <cstdlib>
#include <cctype>
#include <vector>

namespace {

  // A just-big-enough JSON reader: GitHub's list is an array of objects and
  // only top-level string/boolean members of each are needed.
  struct Reader
  {
    const string& s;
    size_t i;
    bool ok;
    Reader(const string& t) : s(t), i(0), ok(true) {}

    void ws() { while (i < s.size() && isspace((unsigned char)s[i])) i++; }

    string str()
    {
      string out;
      if (i >= s.size() || s[i] != '"') { ok = false; return out; }
      i++;
      while (i < s.size() && s[i] != '"') {
        if (s[i] == '\\' && i + 1 < s.size()) {
          i++;
          switch (s[i]) {
            case 'n': out += '\n'; break;
            case 't': out += '\t'; break;
            case 'u': out += '?'; i += 4; break;   // never in tags or URLs
            default: out += s[i];
          }
        } else {
          out += s[i];
        }
        i++;
      }
      if (i >= s.size()) ok = false; else i++;
      return out;
    }

    // Skips any value, returning a scalar's text (strings unquoted).
    string value()
    {
      ws();
      if (i >= s.size()) { ok = false; return ""; }
      char c = s[i];
      if (c == '"') return str();
      if (c == '{' || c == '[') {
        char close = c == '{' ? '}' : ']';
        i++;
        ws();
        if (i < s.size() && s[i] == close) { i++; return ""; }
        while (ok) {
          if (c == '{') {
            ws();
            str();
            ws();
            if (i < s.size() && s[i] == ':') i++; else ok = false;
          }
          value();
          ws();
          if (i < s.size() && s[i] == ',') { i++; continue; }
          if (i < s.size() && s[i] == close) { i++; break; }
          ok = false;
        }
        return "";
      }
      size_t b = i;
      while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ']' &&
             !isspace((unsigned char)s[i])) i++;
      return s.substr(b, i - b);
    }
  };

  struct Parsed
  {
    long core[3];
    size_t ncore;
    std::vector<string> pre;
  };

  bool allDigits(const string& s)
  {
    if (s.empty()) return false;
    for (size_t k = 0; k < s.size(); k++)
      if (!isdigit((unsigned char)s[k])) return false;
    return true;
  }

  string stripV(const string& v)
  {
    return (!v.empty() && (v[0] == 'v' || v[0] == 'V')) ? v.substr(1) : v;
  }

  // 9.0.0-beta.1 and the Debian spelling 9.0.0~beta.1 both parse.
  bool parse(const string& in, Parsed& p)
  {
    string v = stripV(in);
    size_t dash = v.find_first_of("-~");
    string core = v.substr(0, dash);
    p.pre.clear();
    p.ncore = 0;
    if (dash != string::npos) {
      string pre = v.substr(dash + 1);
      size_t b = 0;
      while (true) {
        size_t d = pre.find('.', b);
        p.pre.push_back(pre.substr(b, d == string::npos ? d : d - b));
        if (d == string::npos) break;
        b = d + 1;
      }
    }
    size_t b = 0;
    while (true) {
      size_t d = core.find('.', b);
      string part = core.substr(b, d == string::npos ? d : d - b);
      if (!allDigits(part) || p.ncore == 3) return false;
      p.core[p.ncore++] = atol(part.c_str());
      if (d == string::npos) break;
      b = d + 1;
    }
    return true;
  }
}


int ReleaseCheck::compareVersions(const string& a, const string& b)
{
  Parsed pa, pb;
  bool oka = parse(a, pa), okb = parse(b, pb);
  if (!oka || !okb) return oka == okb ? 0 : (oka ? 1 : -1);

  for (size_t k = 0; k < 3; k++) {
    long x = k < pa.ncore ? pa.core[k] : 0;
    long y = k < pb.ncore ? pb.core[k] : 0;
    if (x != y) return x < y ? -1 : 1;
  }
  // A release outranks any of its pre-releases.
  if (pa.pre.empty() || pb.pre.empty()) {
    if (pa.pre.empty() && pb.pre.empty()) return 0;
    return pa.pre.empty() ? 1 : -1;
  }
  for (size_t k = 0; k < pa.pre.size() && k < pb.pre.size(); k++) {
    const string& x = pa.pre[k];
    const string& y = pb.pre[k];
    bool nx = allDigits(x), ny = allDigits(y);
    int c;
    if (nx && ny) {
      long lx = atol(x.c_str()), ly = atol(y.c_str());
      c = lx < ly ? -1 : (lx > ly ? 1 : 0);
    } else if (nx != ny) {
      c = nx ? -1 : 1;          // numbers sort before words
    } else {
      c = x < y ? -1 : (x > y ? 1 : 0);
    }
    if (c != 0) return c;
  }
  if (pa.pre.size() == pb.pre.size()) return 0;
  return pa.pre.size() < pb.pre.size() ? -1 : 1;
}


bool ReleaseCheck::isPreview(const string& version)
{
  Parsed p;
  return parse(version, p) && !p.pre.empty();
}


bool ReleaseCheck::newerRelease(const string& current, const string& json,
                                Release& found, bool includePreviews)
{
  Parsed cur;
  if (!parse(current, cur)) return false;
  bool previewUser = !cur.pre.empty() || includePreviews;

  Reader r(json);
  r.ws();
  if (r.i >= json.size() || json[r.i] != '[') return false;
  r.i++;
  r.ws();

  bool have = false;
  while (r.ok && r.i < json.size() && json[r.i] == '{') {
    r.i++;
    string tag, url;
    bool draft = false, prerelease = false;
    while (r.ok) {
      r.ws();
      if (r.i < json.size() && json[r.i] == '}') { r.i++; break; }
      string key = r.str();
      r.ws();
      if (r.i < json.size() && json[r.i] == ':') r.i++; else r.ok = false;
      string val = r.value();
      if (key == "tag_name") tag = val;
      else if (key == "html_url") url = val;
      else if (key == "draft") draft = (val == "true");
      else if (key == "prerelease") prerelease = (val == "true");
      r.ws();
      if (r.i < json.size() && json[r.i] == ',') r.i++;
    }
    r.ws();
    if (r.i < json.size() && json[r.i] == ',') { r.i++; r.ws(); }

    Parsed p;
    if (draft || !parse(tag, p) || url.empty()) continue;
    if (p.core[0] != cur.core[0]) continue;
    // 8.x is bug-fix only, so its line is the minor version.
    if (cur.core[0] == 8 && (p.ncore < 2 || cur.ncore < 2 ||
                             p.core[1] != cur.core[1])) continue;
    if ((prerelease || !p.pre.empty()) && !previewUser) continue;
    string ver = stripV(tag);
    if (compareVersions(ver, current) <= 0) continue;
    if (!have || compareVersions(ver, found.version) > 0) {
      found.version = ver;
      found.url = url;
      have = true;
    }
  }
  return have;
}


string ReleaseCheck::releasesUrl()
{
  const char* v = getenv("ECCE_UPDATE_CHECK_URL");
  if (v != 0 && *v != '\0') return v;
  return "https://api.github.com/repos/FriendsofECCE/ECCE/releases";
}


bool ReleaseCheck::disabledByEnvironment()
{
  const char* v = getenv("ECCE_NO_UPDATE_CHECK");
  return v != 0 && *v != '\0' && string(v) != "0";
}


bool ReleaseCheck::disabledBySite()
{
  return Ecce::ecceNoUpdateCheck();
}


bool ReleaseCheck::checkDue(long lastCheck, long now)
{
  return lastCheck > now || now - lastCheck >= 24L * 3600L;
}

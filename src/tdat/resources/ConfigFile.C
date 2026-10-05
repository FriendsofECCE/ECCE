#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <regex>
#include <sstream>

#include "tdat/ConfigFile.H"
#include "util/StringConverter.H"

using std::string;
using std::vector;
using std::map;

static void trimBoth(string& s)
{
  size_t b = s.find_first_not_of(" \t\r\n");
  size_t e = s.find_last_not_of(" \t\r\n");
  s = (b == string::npos) ? string() : s.substr(b, e - b + 1);
}

static string lower(const string& s)
{
  string out;
  StringConverter::toLower(s, out);
  return out;
}

ConfigFile::ConfigFile()
  : p_exists(false), p_modified(false), p_site(false), p_finalNewline(false)
{
}

bool ConfigFile::load(const string& path)
{
  p_path = path;
  p_exists = false;
  std::ifstream in(path.c_str(), std::ios::binary);
  if (!in) {
    bool missing = access(path.c_str(), F_OK) != 0;
    parse("");
    return missing;
  }
  std::ostringstream buf;
  buf << in.rdbuf();
  parse(buf.str());
  p_exists = true;
  return true;
}

void ConfigFile::parse(const string& text)
{
  p_lines.clear();
  p_modified = false;
  p_finalNewline = !text.empty() && text[text.size() - 1] == '\n';
  size_t pos = 0;
  while (pos < text.size()) {
    size_t nl = text.find('\n', pos);
    if (nl == string::npos) {
      p_lines.push_back(text.substr(pos));
      break;
    }
    p_lines.push_back(text.substr(pos, nl - pos));
    pos = nl + 1;
  }
  index();
}

// Same line rules as readConfig() in scripts/gensub.
void ConfigFile::index()
{
  static const std::regex oneBlock("^\\s*(.*)\\s*\\{(.*)\\}");
  static const std::regex openBlock("^\\s*(.*)\\s*\\{");
  static const std::regex colonForm("^\\s*([^\\s:]+)\\s*:\\s*(.*)");
  static const std::regex spaceForm("^\\s*(\\S+)\\s+(.*)");
  static const std::regex blank("^\\s*(#.*|//.*)?$");

  p_entries.clear();
  p_warnings.clear();
  for (size_t i = 0; i < p_lines.size(); i++) {
    std::smatch m;
    const string& line = p_lines[i];
    Entry e;
    e.first = e.last = i;
    e.block = false;
    char where[48];
    snprintf(where, sizeof where, "line %lu: ", (unsigned long)(i + 1));
    if (std::regex_search(line, blank))
      continue;
    if (std::regex_search(line, m, oneBlock)) {
      e.key = m[1]; e.value = m[2]; e.block = true;
    } else if (std::regex_search(line, m, openBlock)) {
      e.key = m[1];
      e.block = true;
      string body;
      bool closed = false;
      while (++i < p_lines.size()) {
        const string& b = p_lines[i];
        if (!b.empty() && b[0] == '}') { closed = true; break; }
        string t = b;
        trimBoth(t);
        if (t == "}")
          p_warnings.push_back(string(where) + "a closing brace must be in "
                               "column 0 to end a block");
        body += b + "\n";
      }
      if (!closed) {
        i = p_lines.size() - 1;
        p_warnings.push_back(string(where) + "block is not closed");
      }
      e.value = body;
      e.last = i;
    } else if (std::regex_search(line, m, colonForm)) {
      e.key = m[1]; e.value = m[2];
    } else if (std::regex_search(line, m, spaceForm)) {
      e.key = m[1]; e.value = m[2];
    } else {
      string k = line;
      trimBoth(k);
      p_warnings.push_back(string(where) + "'" + k + "' has no value and "
                           "is ignored");
      continue;
    }
    trimBoth(e.key);
    trimBoth(e.value);
    e.lkey = lower(e.key);
    if (e.lkey.empty())
      continue;
    if (e.value.empty())
      p_warnings.push_back(string(where) + "'" + e.key + "' has no value "
                           "and does not override anything");
    p_entries.push_back(e);
  }
}

void ConfigFile::mergeInto(map<string,string>& out) const
{
  for (size_t i = 0; i < p_entries.size(); i++) {
    const Entry& e = p_entries[i];
    if (e.value == "-")
      out.erase(e.lkey);
    else if (!e.value.empty())
      out[e.lkey] = e.value;
  }
}

void ConfigFile::mergeFile(const string& path, map<string,string>& out)
{
  ConfigFile f;
  if (f.load(path))
    f.mergeInto(out);
}

bool ConfigFile::get(const string& key, string& value, bool* cleared) const
{
  string lk = lower(key);
  bool have = false;
  if (cleared)
    *cleared = false;
  for (size_t i = 0; i < p_entries.size(); i++) {
    const Entry& e = p_entries[i];
    if (e.lkey != lk)
      continue;
    if (e.value == "-") {
      have = false;
      if (cleared)
        *cleared = true;
    } else if (!e.value.empty()) {
      have = true;
      value = e.value;
      if (cleared)
        *cleared = false;
    }
  }
  return have;
}

static vector<string> splitLines(const string& v)
{
  vector<string> out;
  size_t pos = 0;
  for (;;) {
    size_t nl = v.find('\n', pos);
    if (nl == string::npos) { out.push_back(v.substr(pos)); break; }
    out.push_back(v.substr(pos, nl - pos));
    pos = nl + 1;
  }
  return out;
}

// Replace the first occurrence's span with the lines, drop the others; a
// key not in the file goes after the leading comment block.
static void putLines(vector<string>& lines, const vector<ConfigFile::Entry>&
                     entries, const string& lkey,
                     const vector<string>& repl)
{
  vector<size_t> hit;
  for (size_t i = 0; i < entries.size(); i++)
    if (entries[i].lkey == lkey)
      hit.push_back(i);
  if (hit.empty()) {
    size_t at = 0;
    static const std::regex comment("^\\s*#");
    while (at < lines.size() && std::regex_search(lines[at], comment))
      at++;
    lines.insert(lines.begin() + at, repl.begin(), repl.end());
    return;
  }
  for (size_t k = hit.size(); k-- > 1; )
    lines.erase(lines.begin() + entries[hit[k]].first,
                lines.begin() + entries[hit[k]].last + 1);
  const ConfigFile::Entry& f = entries[hit[0]];
  lines.erase(lines.begin() + f.first, lines.begin() + f.last + 1);
  lines.insert(lines.begin() + f.first, repl.begin(), repl.end());
}

bool ConfigFile::set(const string& key, const string& value, string* err)
{
  string why;
  string lk = lower(key);
  string k = key;
  trimBoth(k);
  if (k.empty() || k.find_first_of(" \t\r\n:{}") != string::npos)
    why = "'" + key + "' is not a usable key";
  else if (value.empty() || value == "-")
    why = "a value of '" + value + "' is not writable; use remove() or "
          "clear()";
  else {
    string t = value;
    trimBoth(t);
    if (t != value)
      why = "leading or trailing whitespace would be lost";
    else if (value.find('\n') != string::npos ||
             value.find('{') != string::npos) {
      vector<string> vl = splitLines(value);
      for (size_t i = 0; i < vl.size(); i++)
        if (!vl[i].empty() && vl[i][0] == '}')
          why = "a line starting with '}' would end the block early";
    }
  }
  if (!why.empty()) {
    if (err) *err = why;
    return false;
  }

  string spelling = k;
  for (size_t i = 0; i < p_entries.size(); i++)
    if (p_entries[i].lkey == lk) { spelling = p_entries[i].key; break; }

  vector<string> repl;
  if (value.find('\n') == string::npos && value.find('{') == string::npos) {
    repl.push_back(spelling + ": " + value);
  } else {
    repl.push_back(spelling + " {");
    vector<string> vl = splitLines(value);
    repl.insert(repl.end(), vl.begin(), vl.end());
    repl.push_back("}");
  }

  vector<string> before = p_lines;
  putLines(p_lines, p_entries, lk, repl);
  index();
  string got;
  if (!get(key, got) || got != value) {
    p_lines = before;
    index();
    if (err) *err = "value does not survive the file grammar";
    return false;
  }
  p_modified = true;
  return true;
}

void ConfigFile::remove(const string& key)
{
  string lk = lower(key);
  vector<size_t> hit;
  for (size_t i = 0; i < p_entries.size(); i++)
    if (p_entries[i].lkey == lk)
      hit.push_back(i);
  for (size_t k = hit.size(); k-- > 0; )
    p_lines.erase(p_lines.begin() + p_entries[hit[k]].first,
                  p_lines.begin() + p_entries[hit[k]].last + 1);
  if (!hit.empty()) {
    index();
    p_modified = true;
  }
}

void ConfigFile::clear(const string& key)
{
  string lk = lower(key);
  string spelling = key;
  trimBoth(spelling);
  bool block = false;
  for (size_t i = 0; i < p_entries.size(); i++)
    if (p_entries[i].lkey == lk) {
      spelling = p_entries[i].key;
      block = p_entries[i].block;
      break;
    }
  vector<string> repl(1, block ? spelling + " { - }" : spelling + ": -");
  putLines(p_lines, p_entries, lk, repl);
  index();
  p_modified = true;
}

string ConfigFile::text() const
{
  string out;
  for (size_t i = 0; i < p_lines.size(); i++) {
    out += p_lines[i];
    if (i + 1 < p_lines.size() || p_finalNewline)
      out += "\n";
  }
  return out;
}

bool ConfigFile::save(string* err)
{
  if (p_path.empty()) {
    if (err) *err = "no file name";
    return false;
  }
  if (!p_modified && !p_exists)
    return true;   // nothing to write, and no file made from nothing
  if (p_modified && !p_lines.empty())
    p_finalNewline = true;

  bool onlyComments = true;
  {
    static const std::regex blank("^\\s*(#.*|//.*)?$");
    for (size_t i = 0; i < p_lines.size() && onlyComments; i++)
      if (!std::regex_search(p_lines[i], blank))
        onlyComments = false;
  }
  if (p_modified && onlyComments) {
    if (p_exists && unlink(p_path.c_str()) != 0 && errno != ENOENT) {
      if (err) *err = p_path + ": " + strerror(errno);
      return false;
    }
    p_exists = false;
    p_modified = false;
    return true;
  }

  // Write through a symlink's target rather than replacing the link.
  string target = p_path;
  char real[PATH_MAX];
  if (realpath(p_path.c_str(), real))
    target = real;
  mode_t mode = 0644;
  struct stat st;
  if (stat(target.c_str(), &st) == 0) {
    mode = st.st_mode & 07777;
    if (!(p_site && !(mode & 0200)))
      mode |= 0200;
  }

  char pid[24];
  snprintf(pid, sizeof pid, ".tmp.%d", (int)getpid());
  string tmp = target + pid;
  int fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
  if (fd < 0) {
    if (err) *err = tmp + ": " + strerror(errno);
    return false;
  }
  string data = text();
  size_t done = 0;
  bool ok = true;
  while (done < data.size()) {
    ssize_t n = write(fd, data.data() + done, data.size() - done);
    if (n < 0) {
      if (errno == EINTR) continue;
      ok = false;
      break;
    }
    done += n;
  }
  ok = ok && fchmod(fd, mode) == 0 && fsync(fd) == 0;
  int saved = errno;
  ok = (close(fd) == 0) && ok;
  if (ok)
    ok = rename(tmp.c_str(), target.c_str()) == 0;
  if (!ok) {
    if (errno == 0) errno = saved;
    if (err) *err = target + ": " + strerror(errno ? errno : saved);
    unlink(tmp.c_str());
    return false;
  }
  p_exists = true;
  p_modified = false;
  return true;
}

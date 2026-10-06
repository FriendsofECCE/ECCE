#include <cerrno>
#include <cstring>
#include <sstream>

#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "tdat/SiteRequest.H"
#include "util/ProcessMachine.H"

using std::string;
using std::vector;

static const char* const MAGIC = "ecce-site-request 1";

static bool unhex(char c, int& v)
{
  if (c >= '0' && c <= '9') v = c - '0';
  else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
  else if (c >= 'A' && c <= 'F') v = c - 'A' + 10;
  else return false;
  return true;
}

// The inverse of ProcessMachine::encode, strictly: anything but %XX and the
// unreserved characters is an error, so a damaged file is refused.
static bool decodeToken(const string& in, string& out)
{
  out.clear();
  for (size_t i = 0; i < in.size(); i++) {
    unsigned char c = in[i];
    if (c == '%') {
      int hi, lo;
      if (i + 2 >= in.size() || !unhex(in[i+1], hi) || !unhex(in[i+2], lo))
        return false;
      out += (char)(hi * 16 + lo);
      i += 2;
    } else if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += (char)c;
    } else {
      return false;
    }
  }
  return true;
}

// processmachine's own decoding of a form value: '+' is a space.
static string formDecode(const string& in)
{
  string out;
  for (size_t i = 0; i < in.size(); i++) {
    int hi, lo;
    if (in[i] == '+') out += ' ';
    else if (in[i] == '%' && i + 2 < in.size() && unhex(in[i+1], hi) &&
             unhex(in[i+2], lo)) {
      out += (char)(hi * 16 + lo);
      i += 2;
    } else out += in[i];
  }
  return out;
}

string SiteRequest::formField(const string& form, const string& name)
{
  string value;
  size_t pos = 0;
  while (pos <= form.size()) {
    size_t amp = form.find('&', pos);
    if (amp == string::npos) amp = form.size();
    string pair = form.substr(pos, amp - pos);
    size_t eq = pair.find('=');
    if (eq != string::npos && formDecode(pair.substr(0, eq)) == name)
      value = formDecode(pair.substr(eq + 1));   // last wins, as in Perl
    pos = amp + 1;
  }
  return value;
}

bool SiteRequest::validMachineName(const string& name)
{
  if (name.empty() || name[0] == '.' || name.size() > 255) return false;
  for (unsigned char c : name)
    if (c == '/' || c < 0x20 || c == 0x7f) return false;
  return true;
}

static bool validKey(const string& key)
{
  if (key.empty() || key.size() > 128) return false;
  for (unsigned char c : key)
    if (!(isalnum(c) || c == '_' || c == '-' || c == '.' || c == '+'))
      return false;
  return true;
}

string SiteRequest::encode() const
{
  typedef ProcessMachine PM;
  string out = string(MAGIC) + "\n";
  out += "machine " + PM::encode(machine) + "\n";
  if (!form.empty()) out += "form " + PM::encode(form) + "\n";
  for (size_t i = 0; i < edits.size(); i++) {
    const ConfigEdit& e = edits[i];
    if (e.op == ConfigEdit::Set)
      out += "set " + PM::encode(e.key) + " " + PM::encode(e.value) + "\n";
    else
      out += string(e.op == ConfigEdit::Clear ? "clear " : "remove ") +
             PM::encode(e.key) + "\n";
  }
  if (replaceText) {
    out += "text " + PM::encode(text) + "\n";
    out += "base " + PM::encode(baseText) + "\n";
  }
  out += "end\n";
  return out;
}

bool SiteRequest::decode(const string& data, string& err)
{
  *this = SiteRequest();
  std::istringstream in(data);
  string line;
  if (!std::getline(in, line) || line != MAGIC) {
    err = "not a site settings request";
    return false;
  }
  bool ended = false, haveMachine = false, haveText = false, haveBase = false;
  int n = 1;
  while (std::getline(in, line)) {
    n++;
    if (ended) { err = "text after the end"; return false; }
    vector<string> tok;
    std::istringstream ls(line);
    string t;
    while (ls >> t) tok.push_back(t);
    vector<string> val(tok.size());
    for (size_t i = 1; i < tok.size(); i++)
      if (!decodeToken(tok[i], val[i])) {
        err = "line " + std::to_string(n) + " is damaged";
        return false;
      }
    const string what = tok.empty() ? "" : tok[0];
    if (what == "end" && tok.size() == 1) ended = true;
    else if (what == "machine" && tok.size() == 2) { machine = val[1]; haveMachine = true; }
    else if (what == "form" && tok.size() == 2) form = val[1];
    else if (what == "set" && tok.size() == 3) {
      ConfigEdit e; e.op = ConfigEdit::Set; e.key = val[1]; e.value = val[2];
      edits.push_back(e);
    } else if ((what == "clear" || what == "remove") && tok.size() == 2) {
      ConfigEdit e;
      e.op = what == "clear" ? ConfigEdit::Clear : ConfigEdit::Remove;
      e.key = val[1];
      edits.push_back(e);
    } else if (what == "text" && tok.size() <= 2) {
      text = tok.size() == 2 ? val[1] : ""; haveText = true;
    } else if (what == "base" && tok.size() <= 2) {
      baseText = tok.size() == 2 ? val[1] : ""; haveBase = true;
    } else {
      err = "line " + std::to_string(n) + " is not understood";
      return false;
    }
  }
  if (!ended) { err = "the request is incomplete"; return false; }
  if (!haveMachine || !validMachineName(machine)) {
    err = "the machine name is missing or not usable as a file name";
    return false;
  }
  if (haveText != haveBase) { err = "a raw edit needs its base text"; return false; }
  replaceText = haveText;
  for (size_t i = 0; i < edits.size(); i++)
    if (!validKey(edits[i].key)) {
      err = "'" + edits[i].key + "' is not a setting name";
      return false;
    }
  if (!form.empty()) {
    string type = formField(form, "type");
    if (type != "accept" && type != "delete") {
      err = "unknown registration action '" + type + "'";
      return false;
    }
    if (formField(form, "siteconfig") != "true") {
      err = "the registration is not for the site settings";
      return false;
    }
    if (formField(form, "name") != machine) {
      err = "the registration names a different machine";
      return false;
    }
  }
  return true;
}

// Runs argv with no shell; its output goes where ours does.
static int runProgram(const vector<string>& args)
{
  vector<char*> argv;
  for (const string& a : args) argv.push_back(const_cast<char*>(a.c_str()));
  argv.push_back(nullptr);
  fflush(stdout);
  fflush(stderr);
  pid_t pid = fork();
  if (pid < 0) return -1;
  if (pid == 0) {
    execv(argv[0], argv.data());
    _exit(127);
  }
  int status = 0;
  while (waitpid(pid, &status, 0) < 0)
    if (errno != EINTR) return -1;
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static string whoAmI()
{
  struct passwd* pw = getpwuid(geteuid());
  return pw ? pw->pw_name : std::to_string(geteuid());
}

static string hostName()
{
  char buf[256] = "";
  gethostname(buf, sizeof(buf) - 1);
  return buf;
}

static string groupName(gid_t gid)
{
  struct group* g = getgrgid(gid);
  return g ? g->gr_name : std::to_string(gid);
}

int SiteRequest::applyOnServer(const string& ecceHome, string& report) const
{
  const string dir = ecceHome + "/siteconfig";
  const string setup = " The one-time setup is described in GETTING_STARTED.md, "
                       "\"Changing the site settings from a client\".";
  struct stat st;
  if (stat(dir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) {
    report = "There is no " + dir + " on " + hostName() + ".";
    return 2;
  }
  if (access(dir.c_str(), W_OK) != 0) {
    string g = groupName(st.st_gid);
    report = "You (" + whoAmI() + ") cannot change the site settings on " +
             hostName() + ": " + dir +
             (st.st_mode & S_IWGRP
              ? " is writable only by the group " + g + ", and you are not "
                "in it (log in again after being added)."
              : " is not writable by its group (" + g + ").") +
             " Nothing was changed." + setup;
    return 3;
  }

  // processmachine rewrites these in place, so each must be writable, not
  // just the directory.
  vector<string> files;
  if (!form.empty()) {
    files.push_back(dir + "/Machines");
    files.push_back(dir + "/Queues");
    files.push_back(dir + "/" + machine + ".Q");
  }
  for (const string& f : files)
    if (access(f.c_str(), F_OK) == 0 && access(f.c_str(), W_OK) != 0) {
      report = "You (" + whoAmI() + ") cannot write " + f + " on " +
               hostName() + "; the files in siteconfig must be writable by "
               "its group too. Nothing was changed." + setup;
      return 3;
    }

  const string cfgPath = dir + "/CONFIG." + machine;
  ConfigFile cfg;
  cfg.setSiteFile(true);
  bool writeCfg = replaceText || !edits.empty();
  if (writeCfg) {
    if (!cfg.load(cfgPath)) {
      report = "Cannot read " + cfgPath + ". Nothing was changed.";
      return 4;
    }
    if (replaceText && cfg.text() != baseText) {
      report = cfgPath + " was changed on the server after this computer's "
               "copy was taken. Nothing was changed; refresh the copy "
               "(ecce-remote-setup --refresh) and edit again.";
      return 5;
    }
    string why;
    if (replaceText)
      cfg.setText(text);
    else
      for (size_t i = 0; i < edits.size(); i++)
        if (!cfg.apply(edits[i], &why)) {
          report = edits[i].key + ": " + why + ". Nothing was changed.";
          return 4;
        }
  }

  string done;
  if (!form.empty()) {
    int status = ProcessMachine::run(form);
    if (status != 0) {
      report = "processmachine failed on " + hostName() + " (status " +
               std::to_string(status) + "). Nothing else was changed.";
      return 6;
    }
    done = dir + "/Machines";
  }
  if (writeCfg && cfg.modified()) {
    string why;
    if (!cfg.save(&why)) {
      report = (done.empty() ? string("") : "The registration was saved, but ") +
               cfgPath + " was not written: " + why;
      return 7;
    }
  }

  vector<string> pub;
  pub.push_back(ecceHome + "/bin/ecce-site-publish");
  if (access(pub[0].c_str(), X_OK) != 0) {
    report = "The site settings were saved on " + hostName() + ", but this "
             "installation has no " + pub[0] + " (the ecce-server package) to "
             "publish them to clients.";
    return 8;
  }
  int status = runProgram(pub);
  if (status != 0) {
    report = "The site settings were saved on " + hostName() + ", but "
             "publishing them to clients failed (status " +
             std::to_string(status) + ").";
    return 8;
  }
  report = "Saved and published on " + hostName() + ".";
  return 0;
}

#include "comm/SecretStore.H"

#include <cstdlib>
#include <fstream>
#include <sstream>

#include <unistd.h>

#include "comm/DirectTransport.H"

#ifdef _WIN32
#include <windows.h>
#include <wincred.h>
#endif

namespace SecretStore {

namespace {
const int kTimeoutSec = 5;

std::string target(const Login& l)
{
  return "ecce-ssh:" + l.user + "@" + l.host + ":" + std::to_string(l.port) +
         ":" + l.jump;
}

#ifndef _WIN32
std::string toolName()
{
  const char* t = getenv("ECCE_SECRET_TOOL");
  return t && *t ? t : "secret-tool";
}

bool onPath(const std::string& prog)
{
  if (prog.find('/') != std::string::npos) return access(prog.c_str(), X_OK) == 0;
  const char* p = getenv("PATH");
  std::stringstream ss(p ? p : "");
  std::string d;
  while (std::getline(ss, d, ':'))
    if (!d.empty() && access((d + "/" + prog).c_str(), X_OK) == 0) return true;
  return false;
}

#ifndef __APPLE__
std::vector<std::string> attrs(const std::string& verb, const Login& l)
{
  std::vector<std::string> a = {toolName(), verb};
  if (verb == "store") a.push_back("--label=ECCE ssh " + l.user + "@" + l.host);
  for (const std::string& s : {std::string("service"), std::string("ecce-ssh"),
                               std::string("user"), l.user, std::string("host"), l.host,
                               std::string("port"), std::to_string(l.port),
                               std::string("jump"), l.jump})
    a.push_back(s);
  return a;
}
#endif
#endif

std::string indexFile()
{
  const char* h = getenv("ECCE_REALUSERHOME");
  if (!h || !*h) h = getenv("HOME");
  return std::string(h ? h : "") + "/.ECCE/remembered-logins";
}

std::string line(const Login& l)
{
  return l.user + "@" + l.host + ":" + std::to_string(l.port) +
         (l.jump.empty() ? "" : " via " + l.jump);
}
}  // namespace

bool available()
{
#if defined(_WIN32) || defined(__APPLE__)
  return true;
#else
  const char* bus = getenv("DBUS_SESSION_BUS_ADDRESS");
  return bus && *bus && onPath(toolName());
#endif
}

bool lookup(const Login& l, std::string& password)
{
#ifdef _WIN32
  PCREDENTIALA c = 0;
  if (!CredReadA(target(l).c_str(), CRED_TYPE_GENERIC, 0, &c)) return false;
  password.assign((const char*)c->CredentialBlob, c->CredentialBlobSize);
  CredFree(c);
  return !password.empty();
#elif defined(__APPLE__)
  TransportResult r = DirectTransport::runProcess(
      {"/usr/bin/security", "find-generic-password", "-s", target(l), "-a", l.user, "-w"},
      "", -1, -1, kTimeoutSec);
#else
  if (!available()) return false;
  TransportResult r = DirectTransport::runProcess(attrs("lookup", l), "", -1, -1, kTimeoutSec);
#endif
#ifndef _WIN32
  if (r.status != 0 || r.timedOut || r.out.empty()) return false;
  password = r.out;
  while (!password.empty() && (password[password.size()-1] == '\n')) password.erase(password.size()-1);
  return !password.empty();
#endif
}

bool save(const Login& l, const std::string& password)
{
  if (password.empty()) return false;
#ifdef _WIN32
  CREDENTIALA c = {};
  std::string t = target(l), u = l.user;
  c.Type = CRED_TYPE_GENERIC;
  c.TargetName = (LPSTR)t.c_str();
  c.UserName = (LPSTR)u.c_str();
  c.CredentialBlobSize = (DWORD)password.size();
  c.CredentialBlob = (LPBYTE)password.data();
  c.Persist = CRED_PERSIST_LOCAL_MACHINE;
  return CredWriteA(&c, 0) != 0;
#elif defined(__APPLE__)
  // `security -i` reads its commands from stdin, which keeps the password out
  // of ps; its quoting has no portable escape for these two characters.
  if (password.find_first_of("\"\\\n") != std::string::npos) return false;
  TransportResult r = DirectTransport::runProcess(
      {"/usr/bin/security", "-i"},
      "add-generic-password -U -s \"" + target(l) + "\" -a \"" + l.user +
      "\" -w \"" + password + "\"\n", -1, -1, kTimeoutSec);
  return r.status == 0 && !r.timedOut;
#else
  if (!available()) return false;
  TransportResult r = DirectTransport::runProcess(attrs("store", l), password, -1, -1, kTimeoutSec);
  return r.status == 0 && !r.timedOut;
#endif
}

bool forget(const Login& l)
{
#ifdef _WIN32
  return CredDeleteA(target(l).c_str(), CRED_TYPE_GENERIC, 0) != 0;
#elif defined(__APPLE__)
  TransportResult r = DirectTransport::runProcess(
      {"/usr/bin/security", "delete-generic-password", "-s", target(l), "-a", l.user},
      "", -1, -1, kTimeoutSec);
  return r.status == 0;
#else
  if (!available()) return false;
  TransportResult r = DirectTransport::runProcess(attrs("clear", l), "", -1, -1, kTimeoutSec);
  return r.status == 0 && !r.timedOut;
#endif
}

std::vector<Login> remembered()
{
  std::vector<Login> out;
  std::ifstream in(indexFile());
  std::string s;
  while (std::getline(in, s)) {
    Login l;
    size_t via = s.find(" via ");
    if (via != std::string::npos) { l.jump = s.substr(via + 5); s.erase(via); }
    size_t at = s.find('@'), colon = s.rfind(':');
    if (at == std::string::npos || colon == std::string::npos || colon < at) continue;
    l.user = s.substr(0, at);
    l.host = s.substr(at + 1, colon - at - 1);
    l.port = atoi(s.c_str() + colon + 1);
    if (l.port <= 0) l.port = 22;
    out.push_back(l);
  }
  return out;
}

static void rewrite(const std::vector<Login>& all)
{
  std::ofstream o(indexFile(), std::ios::trunc);
  for (const Login& l : all) o << line(l) << "\n";
}

void noteRemembered(const Login& l)
{
  std::vector<Login> all = remembered();
  for (const Login& x : all) if (line(x) == line(l)) return;
  all.push_back(l);
  rewrite(all);
}

void noteForgotten(const Login& l)
{
  std::vector<Login> all, old = remembered();
  for (const Login& x : old) if (line(x) != line(l)) all.push_back(x);
  if (all.size() != old.size()) rewrite(all);
}

void forgetRemembered(const Login& l)
{
  forget(l);
  noteForgotten(l);
}

}  // namespace SecretStore

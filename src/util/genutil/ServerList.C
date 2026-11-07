#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <map>
#include <set>

#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/PosixCompat.H"
#include "util/ServerList.H"

namespace {

bool clean(const string& s)
{
  return s.find_first_of("\t\r\n") == string::npos;
}

bool blank(const string& s)
{
  return s.find_first_not_of(" ") == string::npos;
}

}


string ServerList::file()
{
  return string(Ecce::realUserPrefPath()) + "AddedServers";
}


vector<ServerEntry> ServerList::load()
{
  vector<ServerEntry> out;
  std::ifstream in(file().c_str());
  string line;
  while (std::getline(in, line)) {
    if (!line.empty() && line[line.size()-1] == '\r') line.erase(line.size()-1);
    size_t a = line.find('\t');
    size_t b = a == string::npos ? a : line.find('\t', a + 1);
    if (line.empty() || line[0] == '#' || b == string::npos) continue;
    ServerEntry e;
    e.type = line.substr(0, a);
    e.name = line.substr(a + 1, b - a - 1);
    size_t c = line.find('\t', b + 1);
    e.address = line.substr(b + 1, c == string::npos ? c : c - b - 1);
    // Three-column entries predate the option and retain their startup behavior.
    e.connectAtStart = c == string::npos || line.substr(c + 1) != "0";
    if (check(e).empty()) out.push_back(e);
  }
  return out;
}


bool ServerList::save(const vector<ServerEntry>& list)
{
  string path = file();
  string tmp = path + ".tmp";
  {
    std::ofstream out(tmp.c_str(), std::ios::binary | std::ios::trunc);
    if (!out) return false;
    for (size_t i = 0; i < list.size(); i++)
      out << list[i].type << '\t' << list[i].name << '\t' << list[i].address
          << '\t' << (list[i].connectAtStart ? 1 : 0) << '\n';
    if (!out) return false;
  }
  remove(path.c_str());   // rename() does not replace on Windows
  return rename(tmp.c_str(), path.c_str()) == 0;
}


string ServerList::check(const ServerEntry& e)
{
  if (e.type != "ssh" && e.type != "ecce") return "Choose the type of the server.";
  if (blank(e.name)) return "Give the server a name.";
  if (blank(e.address)) return "Type the address of the server.";
  if (!clean(e.name) || !clean(e.address))
    return "The name and the address cannot contain tabs or line breaks.";
  if (e.type == "ecce" && e.address.compare(0, 8, "https://") != 0 &&
      !getenv("ECCE_TEST_ALLOW_HTTP_SERVER"))
    return "The address of an ECCE server starts with https://";
  if (e.type == "ssh") {
    string u, h, f;
    if (!parseSsh(e.address, u, h, f))
      return "An SSH folder needs a user, a host and a folder.";
    string bare;
    int port;
    if (!splitHostPort(h, bare, port))
      return "The port of an SSH folder is a number from 1 to 65535.";
    // EcceURL cannot parse a bracketed IPv6 authority back.
    if (bare.find(':') != string::npos)
      return "Give the host of an SSH folder by name (or a Host alias in ~/.ssh/config), not as an IPv6 address.";
  }
  return "";
}


string ServerList::add(const ServerEntry& e)
{
  return replace("", e);
}


string ServerList::replace(const string& oldName, const ServerEntry& e)
{
  string bad = check(e);
  if (!bad.empty()) return bad;
  vector<ServerEntry> list = load();
  size_t at = list.size();
  for (size_t i = 0; i < list.size(); i++) {
    if (!oldName.empty() && list[i].name == oldName) at = i;
    else if (list[i].name == e.name)
      return "There is already a server called \"" + e.name + "\".";
  }
  if (!oldName.empty() && at == list.size()) return "No such server.";
  if (at == list.size()) list.push_back(e);
  else list[at] = e;
  return save(list) ? "" : "The list of servers could not be saved.";
}


bool ServerList::remove(const string& name)
{
  vector<ServerEntry> list = load();
  for (size_t i = 0; i < list.size(); i++)
    if (list[i].name == name) {
      list.erase(list.begin() + i);
      return save(list);
    }
  return false;
}


string ServerList::typeLabel(const string& type)
{
  return type == "ssh" ? "SSH folder" : type == "ecce" ? "ECCE server" : type;
}


string ServerList::sshAddress(const string& user, const string& host,
                              const string& folder, int port)
{
  string f = folder;
  if (f.empty() || f[0] != '/') f = "/" + f;
  return "sftp://" + user + "@" + hostPort(host, port) + f;
}


string ServerList::hostPort(const string& host, int port)
{
  // Port 22 is left out, so the addresses (and the login and password keys
  // derived from them) of folders added before ports existed stay the same.
  if (port == 22 || port == 0) return host;
  const bool v6 = host.find(':') != string::npos && host[0] != '[';
  return (v6 ? "[" + host + "]" : host) + ":" + std::to_string(port);
}


bool ServerList::splitHostPort(const string& authority, string& host, int& port)
{
  host = authority;
  port = 22;
  size_t colon = authority.rfind(':');
  if (colon == string::npos) return !host.empty();
  if (authority[0] == '[') {
    size_t close = authority.find(']');
    if (close == string::npos) return false;
    host = authority.substr(1, close - 1);
    if (close + 1 == authority.size()) return !host.empty();
    if (authority[close + 1] != ':') return false;
    colon = close + 1;
  } else if (authority.find(':') != colon) {
    return true;   // a bare IPv6 address: no port
  } else {
    host = authority.substr(0, colon);
  }
  const string p = authority.substr(colon + 1);
  if (p.empty() || p.size() > 5 ||
      p.find_first_not_of("0123456789") != string::npos) return false;
  port = atoi(p.c_str());
  return !host.empty() && port >= 1 && port <= 65535;
}


bool ServerList::parseSsh(const string& address, string& user, string& host,
                          string& folder)
{
  if (address.compare(0, 7, "sftp://") != 0) return false;
  string rest = address.substr(7);
  size_t at = rest.find('@');
  size_t slash = rest.find('/');
  if (at == string::npos || slash == string::npos || at > slash) return false;
  user = rest.substr(0, at);
  host = rest.substr(at + 1, slash - at - 1);
  if (host.size() > 3 && host.compare(host.size() - 3, 3, ":22") == 0)
    host.erase(host.size() - 3);   // the default port, as typed
  folder = rest.substr(slash);
  return !user.empty() && !host.empty();
}


namespace {
std::set<string>& addedRefs() { static std::set<string> s; return s; }
std::map<string, string>& logins() { static std::map<string, string> m; return m; }
ServerList::LoginProvider& provider() { static ServerList::LoginProvider p = 0; return p; }
// True once this process keeps the set itself (the Organizer).
bool& managed() { static bool m = false; return m; }

// The session store's login for an ECCE server listed in AddedServers.
string sessionLogin(const string& ref)
{
  if (managed() || !provider() || ref.empty()) return "";
  vector<ServerEntry> list = ServerList::load();
  for (size_t i = 0; i < list.size(); i++)
    if (list[i].type == "ecce" && ServerList::refOf(list[i].address) == ref)
      return provider()(ref);
  return "";
}
}

string ServerList::refOf(const string& address)
{
  return EcceURL(address).getRef();
}

void ServerList::setLoginProvider(LoginProvider p) { provider() = p; }

void ServerList::markAdded(const string& ref) { managed() = true; addedRefs().insert(ref); }
void ServerList::clearAdded() { managed() = true; addedRefs().clear(); }
bool ServerList::isAdded(const string& ref)
{
  return addedRefs().count(ref) != 0 || !sessionLogin(ref).empty();
}

void ServerList::setLogin(const string& ref, const string& user)
{
  if (user.empty()) logins().erase(ref); else logins()[ref] = user;
}

string ServerList::loginFor(const string& ref)
{
  std::map<string, string>::const_iterator it = logins().find(ref);
  return it == logins().end() ? sessionLogin(ref) : it->second;
}

string ServerList::sshRootOf(const string& url)
{
  vector<ServerEntry> list = load();
  for (size_t i = 0; i < list.size(); i++) {
    if (list[i].type != "ssh") continue;
    const string& a = list[i].address;
    if (url == a || (url.size() > a.size() && url.compare(0, a.size(), a) == 0 &&
                     url[a.size()] == '/'))
      return a;
  }
  return "";
}

string ServerList::userFor(const string& url)
{
  if (url.compare(0, 7, "sftp://") == 0) {
    string u, h, f;
    return parseSsh(url, u, h, f) ? u : Ecce::serverUser();
  }
  const string ref = EcceURL(url).getRef();
  if (isAdded(ref)) return loginFor(ref);
  return Ecce::serverUser();
}

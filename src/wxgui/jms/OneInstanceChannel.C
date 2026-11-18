/**
 * @file
 *
 * See include/wxgui/OneInstanceChannel.H.  No GUI here: ecce-one-instance
 * links it without the wx GUI libraries' windows.
 */
#include <iostream>

#include "wx/file.h"
#include "wx/filename.h"
#include "wx/ipc.h"
#include "wx/log.h"
#include "wx/stdpaths.h"
#include "wx/utils.h"
#ifndef __WXMSW__
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "wxgui/OneInstanceChannel.H"

const char *OneInstanceChannel::TOPIC = "ecce";

namespace {

unsigned long fnv(const wxString& s)
{
  unsigned long h = 2166136261UL;
  const wxScopedCharBuffer b = s.utf8_str();
  for (size_t i = 0; i < b.length(); i++)
    h = ((h ^ (unsigned char)b[i]) * 16777619UL) & 0xffffffffUL;
  return h;
}

// Per user: a private folder, short because a Unix socket path is limited
// to about 100 characters.  Empty when no folder can be trusted: /tmp is
// shared, and a folder someone else made there (or a symlink) must not be
// used for the token or the socket.
wxString runDir()
{
#ifdef __WXMSW__
  wxString dir = wxStandardPaths::Get().GetTempDir() + "\\ecce-one";
  wxFileName::Mkdir(dir, 0700, wxPATH_MKDIR_FULL);
  return dir;
#else
  wxString dir;
#ifdef __APPLE__
  const char *base = getenv("TMPDIR");    // per user on macOS
#else
  const char *base = getenv("XDG_RUNTIME_DIR");
#endif
  if (base && *base && base[0] == '/')
    dir = wxString::FromUTF8(base) + "/ecce-one";
  else
    dir = wxString::Format("/tmp/ecce-one-%d", (int)getuid());
  mkdir(dir.fn_str(), 0700);
  struct stat st;
  if (lstat(dir.fn_str(), &st) != 0 || !S_ISDIR(st.st_mode) ||
      st.st_uid != getuid() || (st.st_mode & 077) != 0) {
    std::cerr << "ECCE: " << dir.utf8_str()
              << " is not a private folder of yours; single-instance "
                 "checking is off." << std::endl;
    return wxString();
  }
  return dir;
#endif
}

}  // namespace


wxString OneInstanceChannel::context()
{
  wxString context = wxGetUserId();
  for (const char *name : {"ECCE_REALUSERHOME", "ECCE_LOCAL_DATA",
                           "ECCE_SERVER_SESSION", "ECCE_REMOTE_SERVER",
                           "ECCE_REMOTE_DIR"}) {
    wxString value;
    wxGetEnv(name, &value);
    context += "|" + value;
  }
  return context;
}


OneInstanceChannel::Place OneInstanceChannel::place(const wxString& context)
{
  Place p;
  p.dir = runDir();
  if (p.dir.empty()) return p;
  p.key = wxString::Format("%08lx", fnv(context));
#ifdef __WXMSW__
  // wxServer is DDE here (wxUSE_DDE_FOR_IPC): a service name, not a port.
  // The whole key, so two contexts of one user never share a name.
  p.service = "ecce-" + p.key;
#else
  p.service = p.dir + "/" + p.key + ".sock";
#endif
  p.tokenFile = p.dir + "/" + p.key + ".tok";
  return p;
}


bool OneInstanceChannel::tell(const Place& where,
                              const std::vector<wxString>& files)
{
  wxLogNull quiet;
  if (!wxFileExists(where.tokenFile)) return false;
  wxFile f(where.tokenFile);
  wxString token;
  if (!f.IsOpened() || !f.ReadAll(&token)) return false;
  wxClient client;
  wxConnectionBase *conn =
      client.MakeConnection("localhost", where.service, TOPIC);
  if (!conn) return false;
  wxString msg = token;
  for (const wxString& p : files) msg += "\n" + p;
  bool ok = conn->Execute(msg);
  delete conn;
  return ok;
}


std::vector<wxString> OneInstanceChannel::filesIn(
    const std::vector<wxString>& words)
{
  static const char *kinds[] = {"xyz", "pdb", "cif", "mvm", "car", "gro"};
  std::vector<wxString> out;
  for (size_t i = 0; i < words.size(); i++) {
    if (words[i] == "-pipe") { i++; continue; }
    if (words[i].StartsWith("-")) continue;
    wxFileName fn(words[i]);
    fn.MakeAbsolute();
    const wxString ext = fn.GetExt().Lower();
    bool known = false;
    for (const char *k : kinds) known = known || ext == k;
    if (known && fn.FileExists()) out.push_back(fn.GetFullPath());
  }
  return out;
}

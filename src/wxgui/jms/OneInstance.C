/**
 * @file
 *
 * See include/wxgui/OneInstance.H and docs/claude/services/second-start-1069.md.
 */
#include <random>

#include "wx/app.h"
#include "wx/file.h"
#include "wx/filename.h"
#include "wx/ipc.h"
#include "wx/log.h"
#include "wx/snglinst.h"
#include "wx/stdpaths.h"
#include "wx/utils.h"
#ifndef __WXMSW__
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "wxgui/MainFrame.H"
#include "wxgui/OneInstance.H"

namespace {

wxSingleInstanceChecker *s_lock = 0;
wxServer *s_server = 0;
wxString s_token;

const char *TOPIC = "ecce";

unsigned long fnv(const wxString& s)
{
  unsigned long h = 2166136261UL;
  const wxScopedCharBuffer b = s.utf8_str();
  for (size_t i = 0; i < b.length(); i++)
    h = ((h ^ (unsigned char)b[i]) * 16777619UL) & 0xffffffffUL;
  return h;
}

// Per user: a private folder in the system's temp area (short, because a
// Unix socket path is limited to about 100 characters).
wxString runDir()
{
#ifdef __WXMSW__
  wxString dir = wxStandardPaths::Get().GetTempDir() + "\\ecce-one";
  wxFileName::Mkdir(dir, 0700, wxPATH_MKDIR_FULL);
#else
  wxString dir = wxString::Format("/tmp/ecce-one-%d", (int)getuid());
  mkdir(dir.fn_str(), 0700);
  chmod(dir.fn_str(), 0700);
#endif
  return dir;
}

// Anything on the socket without the token is dropped: the Windows port is
// not private to the user.
class Connection : public wxConnection
{
public:
  bool OnExecute(const wxString&, const void *data, size_t size,
                 wxIPCFormat fmt) override
  {
    wxArrayString lines = wxSplit(GetTextFromData(data, size, fmt), '\n');
    if (lines.empty() || lines[0] != s_token) return false;
    std::vector<wxString> files(lines.begin() + 1, lines.end());
    if (wxTheApp)
      wxTheApp->CallAfter([files]() {
        if (MainFrame *host = MainFrame::instance()) {
          host->bringToFront();
          if (!files.empty()) host->openFiles(files);
        }
      });
    return true;
  }
};

class Server : public wxServer
{
public:
  wxConnectionBase *OnAcceptConnection(const wxString& topic) override
  {
    return topic == TOPIC ? new Connection : 0;
  }
};

wxString serviceFor(const wxString& dir, const wxString& key)
{
#ifdef __WXMSW__
  // A port per context; the lock tells whose it is.
  return wxString::Format("%lu", 49152UL + fnv(key) % 16000UL);
#else
  return dir + "/" + key + ".sock";
#endif
}

bool tellRunningCopy(const wxString& service, const wxString& tokenFile,
                     const std::vector<wxString>& files)
{
  wxFile f(tokenFile);
  wxString token;
  if (!f.IsOpened() || !f.ReadAll(&token)) return false;
  wxLogNull quiet;
  wxClient client;
  wxConnectionBase *conn = client.MakeConnection("localhost", service, TOPIC);
  if (!conn) return false;
  wxString msg = token;
  for (const wxString& p : files) msg += "\n" + p;
  bool ok = conn->Execute(msg);
  delete conn;
  return ok;
}

}  // namespace


bool OneInstance::claim(const wxString& context,
                        const std::vector<wxString>& files)
{
  const wxString key = wxString::Format("%08lx", fnv(context));
  const wxString dir = runDir();
  const wxString service = serviceFor(dir, key);
  const wxString tokenFile = dir + "/" + key + ".tok";

  s_lock = new wxSingleInstanceChecker;
  if (!s_lock->Create("ecce-" + key + ".lock", dir + wxFILE_SEP_PATH)) return true;
  if (s_lock->IsAnotherRunning()) {
    // The copy that has the lock may still be starting: give it a few
    // seconds to open its socket.
    for (int i = 0; i < 20; i++) {
      if (tellRunningCopy(service, tokenFile, files)) return false;
      wxMilliSleep(250);
    }
    return true;
  }

  std::random_device rd;
  s_token = wxString::Format("%08x%08x", (unsigned)rd(), (unsigned)rd());
  wxFile tf;
  if (tf.Create(tokenFile, true, wxS_IRUSR | wxS_IWUSR)) {
    tf.Write(s_token);
    tf.Close();
  }
#ifndef __WXMSW__
  wxRemoveFile(service);    // left by a copy that did not end cleanly
#endif
  s_server = new Server;
  if (!s_server->Create(service)) {
    delete s_server;
    s_server = 0;
  }
  return true;
}


std::vector<wxString> OneInstance::filesIn(const std::vector<wxString>& words)
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

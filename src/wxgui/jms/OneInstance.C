/**
 * @file
 *
 * See include/wxgui/OneInstance.H and docs/claude/services/second-start-1069.md.
 */
#include <iostream>
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
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "wxgui/MainFrame.H"
#include "wxgui/OneInstance.H"
#include "wxgui/OneInstanceChannel.H"

namespace {

wxSingleInstanceChecker *s_lock = 0;
wxServer *s_server = 0;
wxString s_token;
wxString s_tokenFile, s_service;
std::vector<wxString> s_pending;   // arrived before the window existed
bool s_released = false;

// Anything without the token is dropped: the Windows DDE service is not
// private to the user.
class Connection : public wxConnection
{
public:
  bool OnExecute(const wxString&, const void *data, size_t size,
                 wxIPCFormat fmt) override
  {
    wxArrayString lines = wxSplit(GetTextFromData(data, size, fmt), '\n');
    if (s_released || lines.empty() || lines[0] != s_token) return false;
    std::vector<wxString> files(lines.begin() + 1, lines.end());
    MainFrame *host = MainFrame::instance();
    if (host && host->isShuttingDown()) return false;
    // Still starting: keep the files for the window to open.
    if (!host) {
      s_pending.insert(s_pending.end(), files.begin(), files.end());
      return true;
    }
    if (wxTheApp)
      wxTheApp->CallAfter([files]() {
        if (MainFrame *h = MainFrame::instance()) {
          h->bringToFront();
          if (!files.empty()) h->openFiles(files);
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
    return topic == OneInstanceChannel::TOPIC ? new Connection : 0;
  }
};

// Never through an existing file or link: remove, then create exclusively.
void writeToken(const wxString& path)
{
#ifdef __WXMSW__
  wxFile tf;
  if (tf.Create(path, true, wxS_IRUSR | wxS_IWUSR)) {
    tf.Write(s_token);
    tf.Close();
  }
#else
  unlink(path.fn_str());
  int fd = open(path.fn_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
  if (fd < 0) return;
  const wxScopedCharBuffer b = s_token.utf8_str();
  if (write(fd, b.data(), b.length()) < 0) { /* no token: second start starts anyway */ }
  close(fd);
#endif
}

}  // namespace


bool OneInstance::claim(const wxString& context,
                        const std::vector<wxString>& files)
{
  const OneInstanceChannel::Place where = OneInstanceChannel::place(context);
  if (where.dir.empty()) return true;
  const wxString& key = where.key;
  const wxString& service = where.service;
  const wxString& tokenFile = where.tokenFile;

  s_lock = new wxSingleInstanceChecker;
  if (!s_lock->Create("ecce-" + key + ".lock", where.dir + wxFILE_SEP_PATH))
    return true;
  if (s_lock->IsAnotherRunning()) {
    // The copy that has the lock may still be starting: give it a few
    // seconds to open its socket.
    for (int i = 0; i < 20; i++) {
      if (OneInstanceChannel::tell(where, files)) return false;
      wxMilliSleep(250);
    }
    return true;
  }

  std::random_device rd;
  s_token = wxString::Format("%08x%08x", (unsigned)rd(), (unsigned)rd());
  s_tokenFile = tokenFile;
  s_service = service;
  writeToken(tokenFile);
#ifndef __WXMSW__
  // unlink, not wxRemoveFile: that one logs a missing file as an error,
  // and the log window is modal over the Organizer (it blocked the Quit).
  unlink(service.fn_str());   // left by a copy that did not end cleanly
#endif
  s_server = new Server;
  wxLogNull quiet;    // no server only means a second start cannot raise us
  if (!s_server->Create(service)) {
    delete s_server;
    s_server = 0;
  }
  return true;
}


std::vector<wxString> OneInstance::filesIn(const std::vector<wxString>& words)
{
  return OneInstanceChannel::filesIn(words);
}


std::vector<wxString> OneInstance::takePending()
{
  std::vector<wxString> out;
  out.swap(s_pending);
  return out;
}


// The listener must not outlive the window: a start during a slow exit
// would be told "already running" and then see nothing.
void OneInstance::release()
{
  s_released = true;
  wxLogNull quiet;
  delete s_server;
  s_server = 0;
  delete s_lock;       // removes the lock file
  s_lock = 0;
#ifndef __WXMSW__
  if (!s_service.empty()) unlink(s_service.fn_str());
  if (!s_tokenFile.empty()) unlink(s_tokenFile.fn_str());
#endif
}

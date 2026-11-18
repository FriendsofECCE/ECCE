/**
 * @file
 *
 * ecce-one-instance [file ...]: hand this start to the user's running ECCE
 * (#1069).  The `ecce` launcher runs it before it starts a gateway, which
 * would otherwise ask for the central server's password only for the
 * Organizer to hand over afterwards.  Same channel and context as the
 * Organizer (OneInstanceChannel); ECCE_OPEN_FILES is read as the Organizer
 * reads it.
 *
 * Exit 0: the running copy was told to come to the front and open the
 * files.  Exit 1: none is running (or it cannot be reached, or the run
 * folder is not private); start as usual.
 */
#include <iostream>

#include "wx/app.h"
#include "wx/init.h"
#include "wx/snglinst.h"
#include "wx/socket.h"
#include "wx/utils.h"

#include "wxgui/OneInstanceChannel.H"

int main(int argc, char **argv)
{
  wxInitializer init(argc, argv);
  if (!init.IsOk()) return 1;
  wxSocketBase::Initialize();

  const OneInstanceChannel::Place where =
      OneInstanceChannel::place(OneInstanceChannel::context());
  // No token: no copy has started since the last clean exit.
  if (where.dir.empty() || !wxFileExists(where.tokenFile)) return 1;

  std::vector<wxString> words;
  for (int i = 1; i < argc; i++) words.push_back(wxString::FromUTF8(argv[i]));
  wxString env;
  if (wxGetEnv("ECCE_OPEN_FILES", &env))
    for (const wxString& f : wxSplit(env, '\n'))
      if (!f.empty()) words.push_back(f);
  const std::vector<wxString> files = OneInstanceChannel::filesIn(words);

  // A token without the lock is left by a copy that crashed: start anew.
  // While the lock is held, the copy may still be opening its socket.
  bool held = true;
  {
    wxSingleInstanceChecker lock;
    held = lock.Create("ecce-" + where.key + ".lock",
                       where.dir + wxFILE_SEP_PATH) && lock.IsAnotherRunning();
  }
  if (!held) return 1;
  for (int i = 0; i < 20; i++) {
    if (OneInstanceChannel::tell(where, files)) {
      std::cerr << "ECCE is already running for this user; "
                   "the running window was brought to the front." << std::endl;
      return 0;
    }
    wxMilliSleep(250);
  }
  return 1;
}

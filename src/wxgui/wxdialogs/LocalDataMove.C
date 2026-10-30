#include "wx/wx.h"

#include <cstdlib>
#include <string>
using std::string;

#include "util/LocalData.H"
#include "util/PosixCompat.H"      // setenv
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/LocalDataMove.H"


// A move of the local data folder asked for in Preferences (#216) happens
// here, at the start of a session and before any of its processes opens
// the folder; never while ECCE is using it.  Only for a folder that came
// from the preference: an explicit ECCE_LOCAL_DATA is left alone.
void offerLocalDataMove()
{
  const char *env = getenv("ECCE_LOCAL_DATA");
  if (!getenv("ECCE_LOCAL_DATA_FROM_PREF") || !env || !*env) return;
  string from = env;
  string to = LocalData::prefMoveTo();
  if (to.empty()) return;
  string use = from, pending, note;
  if (to == from) {
    pending = "";
  } else if (LocalData::isEmptyOrMissing(to)) {
    ewxMessageDialog dlg(0,
        "Move your calculations from\n    " + from + "\nto\n    " + to + " ?\n\n"
        "Start empty: use the new folder with no calculations in it; "
        "they stay in the old one.\n"
        "Cancel: keep using the old folder.",
        "ECCE data folder", wxICON_QUESTION);
    dlg.AddButton(wxID_CANCEL, "Cancel");
    dlg.AddButton(wxID_NO, "Start empty");
    dlg.AddButton(wxID_YES, "Move")->SetDefault();
    int answer = dlg.ShowModal();
    if (answer == wxID_YES) {
      string msg;
      LocalData::MoveResult r = LocalData::move(from, to, msg);
      if (r == LocalData::MOVED || r == LocalData::SAME) {
        use = to;
        if (!msg.empty()) note = msg;
      } else if (r == LocalData::IN_USE) {
        pending = to;
        note = msg + "\n\nECCE uses " + from + " for this session and "
               "will offer the move again at the next start.";
      } else {
        note = msg + "\n\nECCE keeps using " + from + ".";
      }
    } else if (answer == wxID_NO) {
      use = to;
      note = "Your calculations stay in " + from + ".";
    } else {
      note = "ECCE keeps using " + from + ".";
    }
  } else {
    ewxMessageDialog dlg(0,
        to + " is not empty, and ECCE does not merge two data folders.\n\n"
        "Use it as it is: work with what is already in " + to +
        "; your calculations in " + from + " stay there.\n"
        "Cancel: keep using " + from + ".",
        "ECCE data folder", wxICON_QUESTION);
    dlg.AddButton(wxID_CANCEL, "Cancel");
    dlg.AddButton(wxID_OK, "Use it as it is");
    if (dlg.ShowModal() == wxID_OK) {
      use = to;
      note = "Your calculations in " + from + " stay there.";
    } else {
      note = "ECCE keeps using " + from + ".";
    }
  }
  LocalData::setPref(true, use, pending);
  setenv("ECCE_LOCAL_DATA", use.c_str(), 1);
  if (!note.empty()) {
    ewxMessageDialog info(0, note, "ECCE data folder", wxOK | wxICON_INFORMATION);
    info.ShowModal();
  }
}

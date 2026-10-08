//  ECCE_CALCED_SCRIPT=<file>: the steps of the Quantum ESPRESSO tutorial in
//  the real Electronic Structure Editor, one per timer tick, each through
//  the handler a click in the same control reaches.  Inert unless the
//  variable is set.  '#' starts a comment.  Commands:
//    wait MS                  pause
//    ready                    wait until both details dialogs have started
//    theory NAME | runtype NAME   Theory: / Runtype: choices
//    basis NAME               the quick-pick basis set menu entry NAME
//    gui KEY VALUE            what the Details dialog sends when that field
//                             changes (the same message over the same path)
//    details theory|runtype   the "Theory Details..." / "Runtype Details..."
//                             button (opens the real dialog)
//    close-details            close the dialogs, as OK does
//    button save|verify|finaledit|launch
//    dismiss                  close a modal dialog (OK)
//    info                     the current choices, GUI values and the
//                             Verify lamp ("verify="), on stderr
//    shot NAME                writes NAME.ready next to the script and waits
//                             for NAME.go while the test photographs
//    quit
//  Each command is answered on stderr: "ECCE_CALCED_SCRIPT: <line>: ok".

#include <cstdio>
#include <fstream>
#include <sstream>
#include <strings.h>
#include <sys/stat.h>

#include <wx/app.h>
#include <wx/dialog.h>
#include <wx/menu.h>
#include <wx/timer.h>
#include <wx/utils.h>

#include "tdat/GUIValues.H"
#include "wxgui/ewxChoice.H"

#include "CalcEd.H"

namespace {

struct CalcScript {
  std::vector<std::string> lines;
  size_t next;
  wxLongLong notBefore, waitUntil;
  std::string waitingFor, dir;
  CalcScript() : next(0), notBefore(0), waitUntil(0) {}
};

bool exists(const std::string& path)
{
  struct stat st;
  return stat(path.c_str(), &st) == 0;
}

}

void CalcEd::runCalcEdScript(const string& file)
{
  CalcScript *st = new CalcScript();
  std::ifstream in(file.c_str());
  std::string line;
  while (std::getline(in, line)) {
    size_t hash = line.find('#');
    if (hash != std::string::npos) line.erase(hash);
    if (line.find_first_not_of(" \t\r") != std::string::npos)
      st->lines.push_back(line);
  }
  size_t slash = file.rfind('/');
  st->dir = slash == std::string::npos ? "." : file.substr(0, slash);

  wxTimer *timer = new wxTimer();   // lives until the process exits
  timer->Bind(wxEVT_TIMER, [this, st, timer](wxTimerEvent&) {
    if (!st->waitingFor.empty()) {
      if (exists(st->waitingFor) || wxGetLocalTimeMillis() > st->waitUntil)
        st->waitingFor.clear();
      else
        return;
    }
    if (wxGetLocalTimeMillis() < st->notBefore) return;
    if (st->next >= st->lines.size()) { timer->Stop(); return; }

    std::string line = st->lines[st->next];
    std::istringstream words(line);
    std::vector<std::string> w;
    for (std::string t; words >> t; ) w.push_back(t);
    std::string outcome = "ok";

    if (w[0] == "ready") {
      if (!(p_theoryInitFlag && p_runtypeInitFlag)) return;  // try again
    }
    st->next++;
    try {
      auto clickButton = [this](wxWindowID id) {
        wxWindow *b = FindWindow(id);
        if (!b || !b->IsEnabled()) return false;
        wxCommandEvent ev(wxEVT_BUTTON, id);
        ev.SetEventObject(b);
        GetEventHandler()->ProcessEvent(ev);
        return true;
      };
      auto choose = [this](wxWindowID id, const std::string& name) {
        ewxChoice *c = (ewxChoice*)FindWindow(id);
        if (!c || !c->SetStringSelection(name)) return false;
        wxCommandEvent ev(wxEVT_CHOICE, id);
        ev.SetEventObject(c);
        ev.SetInt(c->GetSelection());
        GetEventHandler()->ProcessEvent(ev);
        return true;
      };
      auto rest = [&line, &w](size_t skip) {
        std::string r = line;
        for (size_t i = 0; i < skip; i++) {
          r.erase(0, r.find_first_not_of(" \t"));
          r.erase(0, r.find_first_of(" \t"));
        }
        r.erase(0, r.find_first_not_of(" \t"));
        return r;
      };
      if (w[0] == "wait" && w.size() == 2) {
        st->notBefore = wxGetLocalTimeMillis() + atoi(w[1].c_str());
      } else if (w[0] == "ready") {
        // reached: both dialogs are up
      } else if (w[0] == "theory" && w.size() >= 2) {
        if (!choose(ID_CHOICE_CALCED_THEORY, rest(1)))
          outcome = "FAIL: no such theory";
      } else if (w[0] == "runtype" && w.size() >= 2) {
        if (!choose(ID_CHOICE_CALCED_RUNTYPE, rest(1)))
          outcome = "FAIL: no such runtype";
      } else if (w[0] == "gui" && w.size() >= 3) {
        GUIValue *old = p_GUIValues->get(w[1]);
        if (!old) {
          outcome = "FAIL: no GUI value " + w[1];
        } else {
          std::string msg = w[1] + "|" + rest(2) + "|" + old->m_units + "|" +
              (old->m_sensitive ? "1" : "0") + "|" +
              (old->m_write ? "1" : "0") + "|" + old->m_type + "\n";
          if (w[1].find("ES.Runtype.") == 0)
            processRuntypeInput(msg.c_str());
          else
            processTheoryInput(msg.c_str());
        }
      } else if (w[0] == "basis" && w.size() >= 2) {
        std::vector<std::string> picks = quickPicks();
        std::string want = rest(1);
        size_t at = picks.size();
        for (size_t i = 0; i < picks.size(); i++)
          if (strcasecmp(picks[i].c_str(), want.c_str()) == 0) at = i;
        if (at == picks.size()) {
          outcome = "FAIL: no such basis set";
        } else {
          wxCommandEvent ev(wxEVT_MENU, 100000 + (int)at);
          OnMenuCalcedBasisSetSelected(ev);
        }
      } else if (w[0] == "details" && w.size() == 2) {
        if (!clickButton(w[1] == "theory" ? ID_BUTTON_CALCED_THEORY
                                          : ID_BUTTON_CALCED_RUNTYPE))
          outcome = "FAIL: button disabled";
      } else if (w[0] == "close-details") {
        closeTheoryApp(true);
        closeRuntypeApp(true);
      } else if (w[0] == "button" && w.size() == 2) {
        bool ok;
        if (w[1] == "save") {
          wxCommandEvent ev(wxEVT_MENU, wxID_SAVE);
          GetEventHandler()->ProcessEvent(ev);   // Save skips the event
          ok = true;
        } else {
          ok = clickButton(w[1] == "verify" ? ID_BUTTON_CALCED_VERIFY
                           : w[1] == "launch" ? ID_BUTTON_CALCED_LAUNCH
                           : ID_BUTTON_CALCED_FINAL_EDIT);
        }
        if (!ok) outcome = "FAIL: " + w[1] + " not available";
      } else if (w[0] == "dismiss") {
        bool closed = false;
        for (wxWindowList::compatibility_iterator n =
                 wxTopLevelWindows.GetFirst(); n; n = n->GetNext()) {
          wxDialog *d = dynamic_cast<wxDialog*>(n->GetData());
          if (d && d->IsModal()) { d->EndModal(wxID_OK); closed = true; break; }
        }
        if (!closed) outcome = "no modal dialog";
      } else if (w[0] == "info") {
        wxWindow *launch = FindWindow(ID_BUTTON_CALCED_LAUNCH);
        fprintf(stderr, "CALCED: theory=%s runtype=%s launch=%s\n",
                getTheoryName().ToStdString().c_str(),
                getRuntypeName().ToStdString().c_str(),
                launch && launch->IsEnabled() ? "enabled" : "disabled");
        //  The Verify lamp: empty (not checked), else its tooltip.
        wxWindow *lamp = FindWindow(ID_STATICTEXT_CALCED_VERIFY_LIGHT);
        wxString lampText = !lamp || lamp->GetLabel().IsEmpty()
            ? wxString("unchecked")
            : lamp->GetLabel() + " " + lamp->GetToolTipText();
        lampText.Replace("\n", " | ");
        fprintf(stderr, "CALCED: verify=%s\n", lampText.utf8_str().data());
        for (GUIValues::const_iterator it = p_GUIValues->begin();
             it != p_GUIValues->end(); ++it)
          fprintf(stderr, "CALCED: %s=%s\n", it->first.c_str(),
                  it->second->getValueAsString().c_str());
      } else if (w[0] == "shot" && w.size() == 2) {
        std::string base = st->dir + "/" + w[1];
        std::remove((base + ".go").c_str());
        std::ofstream(base + ".ready") << "ready\n";
        st->waitingFor = base + ".go";
        st->waitUntil = wxGetLocalTimeMillis() + 90000;
      } else if (w[0] == "quit") {
        fprintf(stderr, "ECCE_CALCED_SCRIPT: quit\n");
        closeTheoryApp(true);
        closeRuntypeApp(true);
        Close(true);
        timer->Stop();
        return;
      } else {
        outcome = "unknown command";
      }
    } catch (EcceException& ex) {
      outcome = std::string("exception: ") + ex.what();
    }
    fprintf(stderr, "ECCE_CALCED_SCRIPT: %s: %s\n", line.c_str(),
            outcome.c_str());
    fflush(stderr);
  });
  timer->Start(500);
}

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
//    guiw KEY VALUE          as gui, marked to be written, which is what the
//                             dialog sends for a value that is not the default
//    details theory|runtype   the "Theory Details..." / "Runtype Details..."
//                             button (opens the real dialog)
//    close-details            close the dialogs, as OK does
//    button save|verify|finaledit|launch
//    dismiss                  close a modal dialog (OK)
//    code NAME                a click on that code's button in the Code row
//    wmcommand ID             (Windows) the message a menu item click sends
//    ts-source other          click the TS Search button (FAIL if hidden or off)
//    ts-pick reactant|product|guess NAME / ts-state   pick a calculation; print what is shown
//    enabled                  which main controls are enabled, and the state
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

#include "wxgui/ewxCheckBox.H"
#include <wx/app.h>
#include <wx/dialog.h>
#include <wx/menu.h>
#include <wx/timer.h>
#include <wx/utils.h>

#include "tdat/GUIValues.H"
#include "util/ResourceUtils.H"
#include "dsm/ICalculation.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxThemeColours.H"

#include "CalcEd.H"
#include "wxgui/EcceTool.H"

namespace {

struct CalcScript {
  std::vector<std::string> lines;
  size_t next;
  wxLongLong notBefore, waitUntil;
  std::string waitingFor, dir;
  CalcScript() : next(0), notBefore(0), waitUntil(0) {}
};

// A multi-line note on one log line, so a test can read it with one regex.
std::string oneLine(std::string n)
{
  for (size_t k; (k = n.find('\n')) != std::string::npos; )
    n.replace(k, 1, " | ");
  return n;
}

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
      } else if ((w[0] == "gui" || w[0] == "guiw") && w.size() >= 3) {
        GUIValue *old = p_GUIValues->get(w[1]);
        if (!old) {
          outcome = "FAIL: no GUI value " + w[1];
        } else {
          std::string msg = w[1] + "|" + rest(2) + "|" + old->m_units + "|" +
              (old->m_sensitive ? "1" : "0") + "|" +
              (old->m_write || w[0] == "guiw" ? "1" : "0") + "|" + old->m_type + "\n";
          if (w[1].find("ES.Runtype.") == 0)
            processRuntypeInput(msg.c_str());
          else
            processTheoryInput(msg.c_str());
        }
      } else if (w[0] == "guivalue" && w.size() == 2) {
        GUIValue *v = p_GUIValues ? p_GUIValues->get(w[1]) : 0;
        fprintf(stderr, "CALCED: guivalue %s=%s\n", w[1].c_str(),
                v ? v->getValueAsString().c_str() : "<none>");
      } else if (w[0] == "basis" && w.size() >= 2) {
        std::vector<std::string> picks = quickPicks();
        std::string want = rest(1);
        size_t at = picks.size();
        for (size_t i = 0; i < picks.size(); i++)
          if (strcasecmp(picks[i].c_str(), want.c_str()) == 0) at = i;
        if (at == picks.size()) {
          outcome = "FAIL: no such basis set; the menu has:";
          for (size_t i = 0; i < picks.size(); i++) outcome += " " + picks[i];
        } else {
          wxCommandEvent ev(wxEVT_MENU, ID_BASIS_PICK0 + (int)at);
          OnMenuCalcedBasisSetSelected(ev);
        }
      } else if (w[0] == "code" && w.size() >= 2) {
        // a click on the code's button in the Code row
        std::string want = rest(1);
        wxWindow *hit = 0;
        wxSizerItemList kids = p_codeSizer->GetChildren();
        for (wxSizerItemList::compatibility_iterator n = kids.GetFirst();
             n && !hit; n = n->GetNext()) {
          wxWindow *b = n->GetData()->GetWindow();
          if (b && b->GetToolTipText() == want) hit = b;
        }
        if (!hit) {
          outcome = "FAIL: no such code";
        } else {
          wxCommandEvent ev(wxEVT_BUTTON, ID_BUTTON_CALCED_CODE);
          ev.SetEventObject(hit);
          OnButtonCalcedCodeClick(ev);
        }
      } else if (w[0] == "ts-source" && w.size() == 2) {
        // the TS Search button, as clicked; fails when it is not clickable
        if (w[1] != "other" || !p_tsSearchBtn || !p_tsSearchBtn->IsShown()
            || !p_tsSearchBtn->IsEnabled()) {
          outcome = "FAIL: TS Search button not available";
        } else {
          wxCommandEvent ev(wxEVT_BUTTON, p_tsSearchBtn->GetId());
          ev.SetEventObject(p_tsSearchBtn);
          p_tsSearchBtn->GetEventHandler()->ProcessEvent(ev);
        }
      } else if (w[0] == "ts-pick" && w.size() >= 3) {
        const int role = w[1] == "reactant" ? 0 : w[1] == "product" ? 1 : 2;
        const std::string want = rest(2);
        int sel = want == "(none)" ? 0 : -1;
        for (size_t i = 0; i < p_tsNames.size(); i++)
          if (p_tsNames[i] == want) sel = (int)i + 1;
        if (sel < 0 || !p_tsChoice[role]) {
          outcome = "FAIL: no such calculation to pick";
        } else {
          p_tsChoice[role]->SetSelection(sel);
          tsPicked(role, sel);
        }
      } else if (w[0] == "ts-state") {
        // what a user sees of the structure source
        std::string names;
        for (size_t i = 0; i < p_tsNames.size(); i++)
          names += (i ? "," : "") + p_tsNames[i];
        fprintf(stderr, "CALCED: ts-state source=%s builder=%s search=%s panel=%s symmetry=%s "
                "atoms=%d candidates=%s runtypes=",
                p_tsSource ? "other" : "builder",
                p_builderTool->IsEnabled() ? "on" : "off",
                p_tsSearchBtn && p_tsSearchBtn->IsShown() ? "shown" : "hidden",
                p_tsPanel && p_tsPanel->IsShown() ? "shown" : "hidden",
                !FindWindow(ID_CHECKBOX_CALCED_USE_SYMMETRY) ? "none" :
                FindWindow(ID_CHECKBOX_CALCED_USE_SYMMETRY)->IsEnabled() ? "on" : "off",
                p_frag ? (int)p_frag->numAtoms() : -1, names.c_str());
        ewxChoice *rt = (ewxChoice*)FindWindow(ID_CHOICE_CALCED_RUNTYPE);
        for (unsigned i = 0; rt && i < rt->GetCount(); i++)
          fprintf(stderr, "%s%s", i ? "|" : "", rt->GetString(i).ToStdString().c_str());
        wxString ln = p_tsLine ? p_tsLine->GetLabel() : wxString();
        ln.Replace("\n", " ");
        fprintf(stderr, " line=%s reason=%s\n", ln.ToStdString().c_str(),
                notReadyReason().c_str());
      } else if (w[0] == "ts-warnings") {
        // the warnings under the TS Search pickers, joined with '|'
        outcome = "warnings:";
        for (const std::string& t : tsWarnings()) outcome += "|" + t;
        outcome += "|levels-differ=" + std::string(tsLevelsDiffer() ? "yes" : "no");
        for (char& c : outcome) if (c == '\n') c = '|';
      } else if (w[0] == "ts-mapping") {
        // the text 'Show mapping' shows, on one line
        outcome = tsMappingText();
        for (char& c : outcome) if (c == '\n') c = '|';
      } else if (w[0] == "enabled") {
        // which of the main controls are enabled, as a user sees them
        const struct { const char *name; wxWindowID id; } ctl[] = {
          {"charge-label", ID_STATIC_CALCED_CHARGE},
          {"charge", ID_COMBOBOX_CALCED_CHARGE},
          {"spin-label", ID_STATIC_CALCED_SPIN_MULT},
          {"theory", ID_CHOICE_CALCED_THEORY},
          {"theory-details", ID_BUTTON_CALCED_THEORY},
          {"runtype-details", ID_BUTTON_CALCED_RUNTYPE},
          {"basis-quick", ID_BUTTON_CALCED_BASIS_QUICK},
          {"verify", ID_BUTTON_CALCED_VERIFY},
          {"finaledit", ID_BUTTON_CALCED_FINAL_EDIT},
          {"launch", ID_BUTTON_CALCED_LAUNCH}};
        outcome = "ok";
        for (auto& c : ctl) {
          wxWindow *x = FindWindow(c.id);
          outcome += std::string(" ") + c.name + "=" +
                     (!x ? "none" : x->IsEnabled() ? "on" : "off");
        }
        outcome += std::string(" state=") + (p_iCalc ?
            ResourceUtils::stateToString(p_iCalc->getState()) : "none");
#ifdef __WXMSW__
      } else if (w[0] == "wmcommand" && w.size() == 2) {
        // the WM_COMMAND a click on a popup menu item with that id sends
        int id = atoi(w[1].c_str());
        ::SendMessage((HWND)GetHWND(), WM_COMMAND, MAKEWPARAM(id, 0), 0);
#endif
      } else if (w[0] == "details" && w.size() == 2) {
        if (!clickButton(w[1] == "theory" ? ID_BUTTON_CALCED_THEORY
                                          : ID_BUTTON_CALCED_RUNTYPE))
          outcome = "FAIL: button disabled";
      } else if (w[0] == "close-details") {
        closeTheoryApp(true);
        closeRuntypeApp(true);
      } else if (w[0] == "button" && w.size() == 2) {
        bool ok;
        if (w[1] == "save-keep") {
          // as Save with Shift held: the generator's files stay for a test
          p_testKeepParams = true;
          wxCommandEvent ev(wxEVT_MENU, wxID_SAVE);
          GetEventHandler()->ProcessEvent(ev);
          p_testKeepParams = false;
          ok = true;
        } else if (w[1] == "save") {
          wxCommandEvent ev(wxEVT_MENU, wxID_SAVE);
          GetEventHandler()->ProcessEvent(ev);   // Save skips the event
          ok = true;
        } else {
          ok = clickButton(w[1] == "verify" ? ID_BUTTON_CALCED_VERIFY
                           : w[1] == "launch" ? ID_BUTTON_CALCED_LAUNCH
                           : ID_BUTTON_CALCED_FINAL_EDIT);
        }
        if (!ok) outcome = "FAIL: " + w[1] + " not available";
      } else if (w[0] == "symmetry") {
        //  "symmetry" reports the box; "symmetry 0|1" clicks it
        if (w.size() > 1) {
          ((ewxCheckBox*)FindWindow(ID_CHECKBOX_CALCED_USE_SYMMETRY))
              ->SetValue(w[1] == "1");
          wxCommandEvent ev(wxEVT_COMMAND_CHECKBOX_CLICKED,
                            ID_CHECKBOX_CALCED_USE_SYMMETRY);
          OnCheckboxCalcedUseSymmetryClick(ev);
        }
        fprintf(stderr, "CALCED: symmetry box=%d stored=%d note=%s\n",
                useSymmetryBox() ? 1 : 0, getUseSymmetry() ? 1 : 0,
                oneLine(std::string(p_notes->GetLabel().ToUTF8().data())).c_str());
      } else if (w[0] == "constraints") {
        //  The constraint count and the warning in the Settings box (#306)
        fprintf(stderr, "CALCED: constraints count=%d note=%s\n",
                p_constraintCount,
                oneLine(constraintNote()).c_str());
        //  what the Settings box line itself shows (#502)
        fprintf(stderr, "CALCED: constraintsline shown=%d text=%s\n",
                p_notes->IsShown() ? 1 : 0,
                oneLine(std::string(p_notes->GetLabel().ToUTF8().data())).c_str());
        {
          //  and what Verify says about them
          std::vector<VerifyFinding> findings;
          if (verifyInput(&findings))
            for (size_t i = 0; i < findings.size(); i++)
              if (findings[i].check == "constraints")
                fprintf(stderr, "CALCED: verify %s: %s\n",
                        findings[i].level == VerifyFinding::UNSURE ? "UNSURE" : "other",
                        findings[i].message.c_str());
        }
      } else if (w[0] == "dismiss") {
        bool closed = false;
        for (wxWindowList::compatibility_iterator n =
                 wxTopLevelWindows.GetFirst(); n; n = n->GetNext()) {
          wxDialog *d = dynamic_cast<wxDialog*>(n->GetData());
          if (d && d->IsModal()) { d->EndModal(wxID_OK); closed = true; break; }
        }
        if (!closed) outcome = "no modal dialog";
      } else if (w[0] == "rects") {
        // where the button row, the summary and the frame are, for layout tests
        auto r = [this](wxWindowID id) {
          wxWindow *x = FindWindow(id);
          wxRect q = x ? x->GetRect() : wxRect();
          return wxString::Format("%d,%d,%d,%d", q.x, q.y, q.width, q.height);
        };
        wxWindow *log = FindWindow(WxFeedbackGUI::ID_TEXTCTRL_FEEDBACK_MSG);
        fprintf(stderr, "CALCED: rects verify=%s launch=%s frame=%d,%d "
                "log=%d\n",
                r(ID_BUTTON_CALCED_VERIFY).utf8_str().data(),
                r(ID_BUTTON_CALCED_LAUNCH).utf8_str().data(),
                GetClientSize().x, GetClientSize().y,
                log ? log->GetSize().y : -1);
      } else if (w[0] == "grow" && w.size() == 2) {
        //  Make the frame taller by N pixels, as a drag of its bottom edge.
        wxSize c = GetClientSize();
        SetClientSize(c.x, c.y + atoi(w[1].c_str()));
        Layout();
      } else if (w[0] == "info") {
        wxWindow *launch = FindWindow(ID_BUTTON_CALCED_LAUNCH);
        fprintf(stderr, "CALCED: theory=%s runtype=%s launch=%s\n",
                getTheoryName().ToStdString().c_str(),
                getRuntypeName().ToStdString().c_str(),
                launch && launch->IsEnabled() ? "enabled" : "disabled");
        //  The standing not-ready line and the box outlined in red.
        const wxColour red = ewxThemeColours::statusText(ewxThemeColours::BAD);
        fprintf(stderr, "CALCED: notready=%s outline=%s,%s\n",
                p_notReadyWhy.c_str(),
                p_chemSysMark->GetBackgroundColour() == red ? "chemsys" : "none",
                p_basisMark->GetBackgroundColour() == red ? "basis" : "none");
        fprintf(stderr, "CALCED: basisbox=%s\n",
                p_basisMark && p_basisMark->IsShown() ? "shown" : "hidden");
        //  The Verify lamp: empty (not checked), else its tooltip.
        wxWindow *lamp = FindWindow(ID_STATICTEXT_CALCED_VERIFY_LIGHT);
        wxString lampText = !lamp || lamp->GetLabel().IsEmpty()
            ? wxString("unchecked")
            : lamp->GetLabel() + " " + lamp->GetToolTipText();
        lampText.Replace("\n", " | ");
        fprintf(stderr, "CALCED: verify=%s\n", lampText.utf8_str().data());
        //  The chemical system as the page shows it.
        auto label = [this](wxWindowID id) {
          wxWindow *w = FindWindow(id);
          return w ? w->GetLabel() : wxString("?");
        };
        fprintf(stderr, "CALCED: formula=%s atoms=%s electrons=%s "
                "symmetry=%s\n",
                label(ID_STATICTEXT_CALCED_FORMULA).utf8_str().data(),
                label(ID_STATICTEXT_CALCED_ATOMS).utf8_str().data(),
                label(ID_STATICTEXT_CALCED_ELECTRONS).utf8_str().data(),
                label(ID_STATICTEXT_CALCED_SYMMETRY).utf8_str().data());
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

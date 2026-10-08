//  ECCE_BUILDER_SCRIPT=<file>: steps of the Quantum ESPRESSO tutorial in the
//  real Builder window, one per timer tick, each through the handler a click
//  or Enter in the same control reaches.  Inert unless the variable is set.
//  '#' starts a comment.  Commands:
//    wait MS                     pause
//    add ELEM SHAPE X Y Z        a click in the 3D view in Atom mode
//    panel NAME                  Tools > NAME (shows the panel)
//    cmd TEXT                    a Builder command line (addh, center, ...)
//    pbc ...                     Periodic Builder, see PBC::scriptCommand
//    library PATH [ENTRY]        the Structure Library pane: open the folder
//                                PATH (Teaching/Diatomics), select ENTRY
//    save                        File > Save
//    info                        atoms and lattice, on stderr
//    expect atoms N | expect lattice yes|no
//    shot NAME                   writes NAME.ready next to the script and
//                                waits for NAME.go: the test photographs the
//                                window meanwhile
//    symmetry                    Symmetry > Find, then the point group
//    progress N                  the MO Compute progress dialog, N seconds
//    property NAME               a click on Properties > NAME (toggles it)
//    list NAME                   a click on NAME in the Properties list
//    viewer NAME                 a click on NAME's "Show in viewer" box
//    overlay                     the column's tab (1 = Properties), and for
//                                each viewer panel: shown, viz focus
//    quit
//  Each command is answered on stderr: "ECCE_BUILDER_SCRIPT: <line>: ok".

#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include <wx/aui/aui.h>
#include <wx/app.h>
#include <wx/checkbox.h>
#include <wx/menu.h>
#include <wx/timer.h>
#include <wx/utils.h>

#include "tdat/LatticeDef.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "viz/FindSymmetryCmd.H"
#include "wxgui/ewxProgressDialog.H"
#include "Builder.H"
#include "PBC.H"
#include "dsm/IPropCalculation.H"
#include "PropertyIndexPanel.H"
#include "StructLib.H"
#include "VizPropertyPanel.H"

namespace {

struct ScriptState {
  std::vector<std::string> lines;
  size_t next;
  wxLongLong notBefore;
  std::string waitingFor;      // a .go file
  wxLongLong waitUntil;
  std::string dir;
  ScriptState() : next(0), notBefore(0), waitUntil(0) {}
};

bool exists(const std::string& path)
{
  struct stat st;
  return stat(path.c_str(), &st) == 0;
}

}

void Builder::runBuilderScript(const std::string& file)
{
  ScriptState *st = new ScriptState();
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

    std::string line = st->lines[st->next++];
    std::istringstream words(line);
    std::vector<std::string> w;
    for (std::string t; words >> t; ) w.push_back(t);
    std::string outcome = "ok";
    try {
      SGFragment *frag = getSG()->getFragment();
      PBC *pbc = dynamic_cast<PBC*>(p_mgr.GetPane(NAME_TOOL_PBC).window);
      if (w[0] == "wait" && w.size() == 2) {
        st->notBefore = wxGetLocalTimeMillis() + atoi(w[1].c_str());
      } else if (w[0] == "add" && w.size() == 6) {
        interpretCommand("add elem " + w[1] + " geom " + w[2] + " x " + w[3] +
                         " y " + w[4] + " z " + w[5]);
        getViewer().viewAll();
      } else if (w[0] == "cmd" && w.size() >= 2) {
        interpretCommand(line.substr(line.find("cmd") + 4));
        getViewer().viewAll();
      } else if (w[0] == "panel" && w.size() >= 2) {
        std::string name = line.substr(line.find("panel") + 6);
        while (!name.empty() && isspace((unsigned char)name[0]))
          name.erase(0, 1);
        int id = p_toolMenu->FindItem(name);
        if (id == wxNOT_FOUND) {
          outcome = "no Tools menu item " + name;
        } else {
          p_toolMenu->Check(id, true);
          wxCommandEvent ev(wxEVT_MENU, id);
          ev.SetInt(1);
          GetEventHandler()->ProcessEvent(ev);
        }
      } else if (w[0] == "pbc" && w.size() >= 2 && pbc) {
        std::vector<std::string> rest(w.begin() + 1, w.end());
        std::string err = pbc->scriptCommand(rest);
        if (!err.empty()) outcome = err;
      } else if (w[0] == "library" && w.size() >= 2) {
        //  Mode > Add Structure shows the library pane.
        p_modeMenu->Check(ID_MODE_STRUCTLIB, true);
        wxCommandEvent modeEv(wxEVT_MENU, ID_MODE_STRUCTLIB);
        GetEventHandler()->ProcessEvent(modeEv);
        if (!p_structLib->openFolder(w[1], w.size() > 2 ? w[2] : ""))
          outcome = "FAIL: not found in the Structure Library";
      } else if (w[0] == "save") {
        doSave();
      } else if (w[0] == "info") {
        LatticeDef *lat = frag ? frag->getLattice() : 0;
        fprintf(stderr, "BUILDER: atoms=%d lattice=%s\n",
                frag ? (int)frag->numAtoms() : -1, lat ? "yes" : "no");
      } else if (w[0] == "expect" && w.size() == 3) {
        LatticeDef *lat = frag ? frag->getLattice() : 0;
        if (w[1] == "atoms" && (!frag || (int)frag->numAtoms() != atoi(w[2].c_str())))
          outcome = "FAIL: atoms = " + std::to_string(frag ? frag->numAtoms() : -1);
        else if (w[1] == "lattice" && (lat != 0) != (w[2] == "yes"))
          outcome = "FAIL: lattice is " + std::string(lat ? "yes" : "no");
      } else if (w[0] == "shot" && w.size() == 2) {
        std::string base = st->dir + "/" + w[1];
        std::remove((base + ".go").c_str());
        std::ofstream(base + ".ready") << "ready\n";
        st->waitingFor = base + ".go";
        st->waitUntil = wxGetLocalTimeMillis() + 90000;
      } else if (w[0] == "symmetry") {
        //  The Symmetry panel's Find (autosym), then the point group found.
        Command *cmd = new FindSymmetryCmd("Find Symmetry", getSG());
        cmd->getParameter("threshold")->setDouble(0.01);
        execute(cmd);
        fprintf(stderr, "BUILDER: pointgroup=%s\n",
                frag ? frag->pointGroup().c_str() : "?");
      } else if (w[0] == "progress" && w.size() == 2) {
        //  The dialog MO/density/ESP Compute shows, made as MoPanel makes
        //  it, held up for N seconds at 30% so the test can photograph it.
        ewxProgressDialog *dlg = new ewxProgressDialog("ECCE Compute MOs",
            "Initializing...", 100, 0,
            wxPD_AUTO_HIDE|wxPD_CAN_ABORT|wxPD_ELAPSED_TIME|wxPD_SMOOTH);
        wxPoint pos = GetScreenPosition() + wxPoint(60, 60);
        dlg->SetSize(pos.x, pos.y, -1, -1);
        dlg->Show();
        wxLongLong end = wxGetLocalTimeMillis() + 1000 * atoi(w[1].c_str());
        while (wxGetLocalTimeMillis() < end) {
          dlg->isInterrupted("Computing grid points", 30);
          wxMilliSleep(50);
        }
        dlg->Destroy();
      } else if ((w[0] == "property" || w[0] == "list" || w[0] == "viewer")
                 && w.size() >= 2) {
        std::string name = line.substr(line.find(w[0]) + w[0].size() + 1);
        while (!name.empty() && isspace((unsigned char)name[0]))
          name.erase(0, 1);
        if (w[0] == "property") {
          //  As GTK and MSW deliver a click on a check item: the item is
          //  toggled first, the event carries its new state.
          int id = p_propertyMenu->FindItem(name);
          if (id == wxNOT_FOUND) {
            outcome = "no Properties menu item " + name;
          } else {
            bool on = !p_propertyMenu->IsChecked(id);
            p_propertyMenu->Check(id, on);
            wxCommandEvent ev(wxEVT_MENU, id);
            ev.SetInt(on ? 1 : 0);
            GetEventHandler()->ProcessEvent(ev);
          }
        } else if (w[0] == "list") {
          if (!p_index || !p_index->click(name))
            outcome = "not in the Properties list: " + name;
        } else {
          VizPropertyPanel *panel = dynamic_cast<VizPropertyPanel*>(
              p_mgr.GetPane(wxString(name)).window);
          wxCheckBox *box = panel ? panel->viewerToggle() : 0;
          if (!box) {
            outcome = "no Show in viewer box on " + name;
          } else {
            box->SetFocus();
            box->SetValue(!box->GetValue());
            wxCommandEvent ev(wxEVT_CHECKBOX, box->GetId());
            ev.SetEventObject(box);
            ev.SetInt(box->GetValue() ? 1 : 0);
            box->GetEventHandler()->ProcessEvent(ev);
          }
        }
      } else if (w[0] == "overlay") {
        std::string list;
        set<VizPropertyPanel*> panels =
            VizPropertyPanel::getPanels(p_calculation->getURL().toString());
        for (VizPropertyPanel *panel : panels) {
          wxAuiPaneInfo &pane = p_mgr.GetPane(panel);
          wxCheckBox *box = panel->viewerToggle();
          list += " [" + panel->getName() + " shown=" +
                  (pane.IsOk() && pane.IsShown() ? "1" : "0") + " focus=" +
                  (panel->hasFocus() ? "1" : "0") + " box=" +
                  (box ? (box->GetValue() ? "1" : "0") : "-") + "]";
        }
        fprintf(stderr, "BUILDER: overlay tab=%d%s\n", p_columnTab,
                list.c_str());
      } else if (w[0] == "quit") {
        fprintf(stderr, "ECCE_BUILDER_SCRIPT: quit\n");
        Close(true);
        timer->Stop();
        return;
      } else {
        outcome = "unknown command";
      }
    } catch (EcceException& ex) {
      outcome = std::string("exception: ") + ex.what();
    }
    fprintf(stderr, "ECCE_BUILDER_SCRIPT: %s: %s\n", line.c_str(),
            outcome.c_str());
    fflush(stderr);
  });
  timer->Start(500);
}

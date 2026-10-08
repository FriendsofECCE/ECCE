//  ECCE_BUILDER_SCRIPT=<file>: steps of the Quantum ESPRESSO tutorial in the
//  real Builder window, one per timer tick, each through the handler a click
//  or Enter in the same control reaches.  Inert unless the variable is set.
//  '#' starts a comment.  Commands:
//    wait MS                     pause
//    add ELEM SHAPE X Y Z        a click in the 3D view in Atom mode
//    panel NAME                  Tools > NAME (shows the panel)
//    cmd TEXT                    a Builder command line (addh, center, ...)
//    pbc ...                     Periodic Builder, see PBC::scriptCommand
//    save                        File > Save
//    info                        atoms and lattice, on stderr
//    expect atoms N | expect lattice yes|no
//    shot NAME                   writes NAME.ready next to the script and
//                                waits for NAME.go: the test photographs the
//                                window meanwhile
//    quit
//  Each command is answered on stderr: "ECCE_BUILDER_SCRIPT: <line>: ok".

#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include <wx/aui/aui.h>
#include <wx/app.h>
#include <wx/menu.h>
#include <wx/timer.h>
#include <wx/utils.h>

#include "tdat/LatticeDef.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "Builder.H"
#include "PBC.H"

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

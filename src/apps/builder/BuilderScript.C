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
//    mode atom|select|structlib  Mode menu
//    popup NAME                  the element chooser: its place and size
//    click                       a click in the 3D view in the current mode
//    save                        File > Save
//    savemenu                    the File > Save menu event, if enabled
//    savestate                   dirty flag and Save menu/toolbar enabled state
//    info                       atoms and lattice, on stderr
//    libatom I                   select atom (a Nub) I of the library structure
//    select I                    select atom (or Nub) I, as a click does; -1 none
//    selectnub A                 select the first Nub of the last atom of element A
//    bonds A B                   count the bonds from the last atom of element A
//                                to atoms of element B
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
//    rows PANEL                  the rows of a key/value panel (Energies)
//    fold PANEL GROUP            a click on that group's heading
//    vector PROP                 each arrow under PROP's property switch, as
//                                drawn: start, unit direction, farthest
//                                vertex from the start (length) and from the
//                                axis (thickness), Angstrom; the property
//                                vector and the longest bond for comparison
//    ellipsoid PROP              each sphere under PROP's property switch, as
//                                drawn: centre, and its three axes as semi-axis
//                                length and unit direction, Angstrom
//    quit
//  Each command is answered on stderr: "ECCE_BUILDER_SCRIPT: <line>: ok".

#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include <wx/aui/aui.h>
#include <wx/app.h>
#include <wx/checkbox.h>
#include <wx/dcmemory.h>
#include <wx/dcscreen.h>
#include <wx/image.h>
#include <wx/menu.h>
#include <wx/timer.h>
#include <wx/utils.h>

#include <cmath>

#include "inv/actions/SoCallbackAction.H"
#include "inv/actions/SoGetMatrixAction.H"
#include "inv/nodes/SoSphere.H"
#include "inv/actions/SoSearchAction.H"
#include "inv/nodes/SoShape.H"
#include "inv/nodes/SoSwitch.H"
#include "inv/SoPrimitiveVertex.H"
#include "inv/SoPath.H"

#include "tdat/LatticeDef.H"
#include "tdat/PropVector.H"
#include "tdat/TBond.H"
#include "viz/VRVector.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "viz/FindSymmetryCmd.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/PerTabPanel.H"
#include "wxgui/TearableContent.H"
#include "Builder.H"
#include "PBC.H"
#include "dsm/IPropCalculation.H"
#include "KeyValuePropertyPanel.H"
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

struct ArrowExtent { SbVec3f start, axis; float along, across; };

void arrowVertex(void *d, SoCallbackAction *a, const SoPrimitiveVertex *v1,
                 const SoPrimitiveVertex *v2, const SoPrimitiveVertex *v3)
{
  ArrowExtent *e = (ArrowExtent *)d;
  const SoPrimitiveVertex *v[3] = {v1, v2, v3};
  for (int i = 0; i < 3; i++) {
    SbVec3f p;
    a->getModelMatrix().multVecMatrix(v[i]->getPoint(), p);
    SbVec3f r = p - e->start;
    float t = r.dot(e->axis);
    float off = (r - e->axis * t).length();
    if (t > e->along) e->along = t;
    if (off > e->across) e->across = off;
  }
}

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
      } else if (w[0] == "popup" && w.size() == 2) {
        //  The element chooser, as the arrow beside the element tool opens
        //  it, and where it is, straight after it is shown and half a
        //  second later, with the mode toolbar's own rectangle.
        TearableContent *tc = showElementChooser();
        const wxRect bar(p_modeToolbar->GetScreenPosition(),
                         p_modeToolbar->GetSize());
        for (int shot = 0; shot < 2; shot++) {
          if (shot) {
            for (int i = 0; i < 10; i++) { wxTheApp->Yield(true); wxMilliSleep(50); }
          }
          const wxRect r = tc->GetScreenRect();
          fprintf(stderr, "BUILDER popup %s after %d ms: at %d,%d size %dx%d, "
                  "toolbar %d,%d %dx%d, mouse %d,%d\n", w[1].c_str(),
                  shot * 500, r.x, r.y, r.width, r.height, bar.x, bar.y,
                  bar.width, bar.height, wxGetMousePosition().x,
                  wxGetMousePosition().y);
        }
        tc->Dismiss();
      } else if (w[0] == "click") {
        //  A click in the viewer in the current mode (Atom adds the
        //  element, Add Structure the library selection).
        try {
          Command *cmd = createModeCommand(p_currentMode);
          if (cmd) execute(cmd);
          else outcome = "FAIL: nothing to do in this mode";
        } catch (EcceException& ex) {
          outcome = std::string("FAIL: ") + ex.what();
        }
      } else if (w[0] == "mode" && w.size() == 2) {
        const int id = w[1] == "atom" ? ID_MODE_ATOM
                     : w[1] == "select" ? ID_MODE_SELECT : ID_MODE_STRUCTLIB;
        p_modeMenu->Check(id, true);
        wxCommandEvent modeEv(wxEVT_MENU, id);
        GetEventHandler()->ProcessEvent(modeEv);
      } else if (w[0] == "key" && w.size() == 2 && w[1] == "delete") {
        //  What the Delete accelerator sends.
        wxCommandEvent delEv(wxEVT_MENU, wxID_DELETE);
        GetEventHandler()->ProcessEvent(delEv);
      } else if (w[0] == "save") {
        doSave();
      } else if (w[0] == "savemenu") {
        //  File > Save as the menu or Ctrl+S sends it; ignored when disabled
        wxCommandEvent saveEv(wxEVT_MENU, wxID_SAVE);
        if (p_fileMenu->IsEnabled(wxID_SAVE)) GetEventHandler()->ProcessEvent(saveEv);
      } else if (w[0] == "savestate") {
        fprintf(stderr, "BUILDER: dirty=%d save_enabled=%d toolbar_save_enabled=%d\n",
                isDirty() ? 1 : 0, p_fileMenu->IsEnabled(wxID_SAVE) ? 1 : 0,
                p_fileToolbar->GetToolEnabled(wxID_SAVE) ? 1 : 0);
      } else if (w[0] == "info") {
        LatticeDef *lat = frag ? frag->getLattice() : 0;
        fprintf(stderr, "BUILDER: atoms=%d lattice=%s\n",
                frag ? (int)frag->numAtoms() : -1, lat ? "yes" : "no");
      } else if (w[0] == "libatom" && w.size() == 2) {
        p_structLib->setTestAtom(atoi(w[1].c_str()));
      } else if (w[0] == "select" && w.size() == 2 && frag) {
        //  The atom (or Nub) a click on it would select, by index.
        frag->m_atomHighLight.clear();
        if (atoi(w[1].c_str()) >= 0)
          frag->m_atomHighLight.assign(1, atoi(w[1].c_str()));
      } else if (w[0] == "selectnub" && w.size() == 2 && frag) {
        //  The first Nub on the last atom of element A.
        TAtm *last = 0;
        for (size_t i = 0; i < frag->numAtoms(); i++)
          if (frag->atomRef(i)->atomicSymbol() == w[1]) last = frag->atomRef(i);
        outcome = "FAIL: no such atom or no nub";
        for (TBond *b : last ? last->bondList() : vector<TBond*>()) {
          TAtm *o = b->atom1() == last ? b->atom2() : b->atom1();
          if (o->atomicSymbol() == "Nub") {
            frag->m_atomHighLight.assign(1, o->index());
            outcome = "ok";
            break;
          }
        }
      } else if (w[0] == "bonds" && w.size() == 3 && frag) {
        //  Bonds from the last atom of element A to atoms of element B.
        TAtm *last = 0;
        for (size_t i = 0; i < frag->numAtoms(); i++)
          if (frag->atomRef(i)->atomicSymbol() == w[1]) last = frag->atomRef(i);
        int n = 0;
        for (TBond *b : last ? last->bondList() : vector<TBond*>()) {
          TAtm *o = b->atom1() == last ? b->atom2() : b->atom1();
          if (o->atomicSymbol() == w[2]) n++;
        }
        fprintf(stderr, "BUILDER: bonds %s-%s=%d\n", w[1].c_str(),
                w[2].c_str(), n);
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
      } else if ((w[0] == "rows" || w[0] == "fold") && w.size() >= 2) {
        //  rows <panel>: the rows a KeyValuePropertyPanel shows;
        //  fold <panel> <group>: what a click on that group's heading does.
        KeyValuePropertyPanel *kv = dynamic_cast<KeyValuePropertyPanel*>(
            p_mgr.GetPane(wxString(w[1])).window);
        if (!kv) {
          outcome = "no key/value panel " + w[1];
        } else if (w[0] == "fold") {
          if (w.size() < 3) outcome = "fold needs a group";
          else kv->toggleGroup(w[2]);
        } else {
          fprintf(stderr, "BUILDER: rows %s\n", w[1].c_str());
          for (const std::string& row : kv->rowDump())
            fprintf(stderr, "BUILDER: row %s\n", row.c_str());
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
      } else if (w[0] == "vector" && w.size() == 2) {
        SoSearchAction sa;
        sa.setType(VRVector::getClassTypeId(), FALSE);
        sa.setInterest(SoSearchAction::ALL);
        sa.apply(getSG()->getPropertySwitch(w[1]));
        for (int k = 0; k < sa.getPaths().getLength(); k++) {
          VRVector *vec = (VRVector *)sa.getPaths()[k]->getTail();
          const double *p = vec->getPosition(), *d = vec->getDirection();
          SbVec3f u((float)d[0], (float)d[1], (float)d[2]);
          if (u.length() > 0.f) u.normalize();
          ArrowExtent ex = {SbVec3f((float)p[0], (float)p[1], (float)p[2]),
                            u, 0.f, 0.f};
          SoCallbackAction ca;
          ca.addTriangleCallback(SoShape::getClassTypeId(), arrowVertex, &ex);
          ca.apply(vec);
          fprintf(stderr, "BUILDER: vector %s %d start=%.4f,%.4f,%.4f "
                  "dir=%.4f,%.4f,%.4f length=%.4f radius=%.4f\n",
                  w[1].c_str(), k, p[0], p[1], p[2], u[0], u[1], u[2],
                  ex.along, ex.across);
        }
        PropVector *pv = dynamic_cast<PropVector*>(
            p_calculation ? p_calculation->getProperty(w[1]) : 0);
        if (pv && pv->rows() >= 3)
          fprintf(stderr, "BUILDER: vector %s value=%.4f,%.4f,%.4f |v|=%.4f "
                  "%s\n", w[1].c_str(), pv->value(0), pv->value(1),
                  pv->value(2), sqrt(pv->value(0)*pv->value(0) +
                  pv->value(1)*pv->value(1) + pv->value(2)*pv->value(2)),
                  pv->units().c_str());
        double longest = 0.0;
        vector<TBond*> *bonds = frag ? frag->bonds() : 0;
        for (size_t b = 0; bonds && b < bonds->size(); b++)
          longest = std::max(longest, (double)(*bonds)[b]->length());
        delete bonds;
        fprintf(stderr, "BUILDER: vector %s arrows=%d longest_bond=%.4f\n",
                w[1].c_str(), sa.getPaths().getLength(), longest);
      } else if (w[0] == "ellipsoid" && w.size() == 2) {
        SoSearchAction sa;
        sa.setType(SoSphere::getClassTypeId(), FALSE);
        sa.setInterest(SoSearchAction::ALL);
        sa.apply(getSG()->getPropertySwitch(w[1]));
        // Read back through the transform the renderer applies to the
        // unit sphere: its rows are the images of the unit axes.
        for (int k = 0; k < sa.getPaths().getLength(); k++) {
          SoGetMatrixAction ma(SbViewportRegion(100, 100));
          ma.apply(sa.getPaths()[k]);
          const SbMatrix m = ma.getMatrix();
          fprintf(stderr, "BUILDER: ellipsoid %s %d centre=%.4f,%.4f,%.4f",
                  w[1].c_str(), k, m[3][0], m[3][1], m[3][2]);
          for (int i = 0; i < 3; i++) {
            SbVec3f a(m[i][0], m[i][1], m[i][2]);
            float len = a.length();
            if (len > 0.f) a /= len;
            fprintf(stderr, " axis%d=%.4f,%.4f,%.4f,%.4f", i, len, a[0], a[1],
                    a[2]);
          }
          fprintf(stderr, "\n");
        }
        fprintf(stderr, "BUILDER: ellipsoid %s count=%d\n", w[1].c_str(),
                sa.getPaths().getLength());
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

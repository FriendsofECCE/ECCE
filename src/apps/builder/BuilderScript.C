//  ECCE_BUILDER_SCRIPT=<file>: steps of the Quantum ESPRESSO tutorial in the
//  real Builder window, one per timer tick, each through the handler a click
//  or Enter in the same control reaches.  Inert unless the variable is set.
//  '#' starts a comment.  Commands:
//    wait MS                     pause
//    add ELEM SHAPE X Y Z        a click in the 3D view in Atom mode
//    panel NAME                  Tools > NAME (shows the panel)
//    panesize NAME               the docked pane's rectangle and height floor
//    cmd TEXT                 a Builder command line (addh, center, ...)
//    pbc ...                     Periodic Builder, see PBC::scriptCommand
//    library PATH [ENTRY]        the Structure Library pane: open the folder
//                                PATH (Teaching/Diatomics), select ENTRY
//    mode atom|select|bond|structlib  Mode menu
//    popup NAME                  the element chooser: its place and size
//    key viewer|text cut|copy|paste|undo|selectall
//                                Ctrl+X/C/V/Z with the focus in the 3-D view
//                                or in a text field (holding "abc")
//    click                       a click in the 3D view in the current mode
//    save                        File > Save
//    savemenu                    the File > Save menu event, if enabled
//    savestate                   dirty flag and Save menu/toolbar enabled state
//    info                       atoms and lattice, on stderr
//    libatom I                   select atom (a Nub) I of the library structure
//    select I                    select atom (or Nub) I, as a click does; -1 none
//    selectelem A                select the first atom of element A
//    addelem A                   add the last atom of element A to the selection
//    bondorder single|metal      the order Bond mode makes
//    clean                       the viewer's Clean command
//    key escape                  what the Esc key does with the view focused
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
//    pertab A                    press the Build panel's periodic-table button,
//                                pick element A in it, report the Build element
//    quit
//  Each command is answered on stderr: "ECCE_BUILDER_SCRIPT: <line>: ok".

#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>
#include <sys/stat.h>

#include <wx/aui/aui.h>
#include <wx/app.h>
#include <wx/checkbox.h>
#include <wx/dcmemory.h>
#include <wx/dcscreen.h>
#include <wx/image.h>
#include <wx/menu.h>
#include <wx/textctrl.h>
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
#include "tdat/TPerTab.H"
#include "MiniPerTab.H"
#include "viz/VRVector.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "viz/FindSymmetryCmd.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/PerTabPanel.H"
#include "wxgui/TearableContent.H"
#include "Builder.H"
#include "ContextPanel.H"
#include "GeomConstraintsDialog.H"
#include "inv/SoWx/SoWxRenderArea.H"
#include "inv/nodes/SoCamera.H"
#include "wxviz/SGViewer.H"
#include "dsm/ChemistryTask.H"
#include "tdat/GeomConstraintModel.H"
#include "tdat/GeomConstraintRules.H"
#include "tdat/GeomConstraintPick.H"
#include "util/EventDispatcher.H"
#include "viz/CleanCmd.H"
#include "PBC.H"
#include "dsm/IPropCalculation.H"
#include "KeyValuePropertyPanel.H"
#include "PropertyIndexPanel.H"
#include "StructLib.H"
#include "VizPropertyPanel.H"
#include "wxgui/ewxModalGuard.H"

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

SoWxRenderArea *findViewerArea(wxWindow *w)
{
  if (SoWxRenderArea *a = dynamic_cast<SoWxRenderArea *>(w)) return a;
  for (wxWindowList::iterator it = w->GetChildren().begin();
       it != w->GetChildren().end(); ++it)
    if (SoWxRenderArea *a = findViewerArea(*it)) return a;
  return 0;
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
      } else if (w[0] == "panesize" && w.size() >= 2) {
        wxString nm = line.substr(line.find("panesize") + 9);
        nm.Trim(false).Trim(true);
        wxAuiPaneInfo &pn = p_mgr.GetPane(nm);
        fprintf(stderr, "PANESIZE %s: %s y=%d height=%d min_height=%d "
                "resizable=%d\n",
                (const char*)nm.mb_str(),
                pn.IsOk() && pn.IsShown() ? "shown" : "hidden", pn.rect.y,
                pn.rect.height, pn.min_size.y, pn.IsResizable() ? 1 : 0);
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
      } else if (w[0] == "key" && w.size() == 3 &&
                 (w[1] == "viewer" || w[1] == "text")) {
        //  Ctrl+X/C/V/Z as the frame's accelerator delivers it, with the
        //  focus in the 3-D view or in a text field holding "abc", all of
        //  it selected; then the field's text.
        const int id = w[2] == "cut" ? ID_ACCEL_CUT : w[2] == "copy"
                     ? ID_ACCEL_COPY : w[2] == "paste" ? ID_ACCEL_PASTE
                     : w[2] == "selectall" ? ID_ACCEL_SELECTALL : ID_ACCEL_UNDO;
        wxTextCtrl *field = 0;
        std::function<void(wxWindow*)> find = [&](wxWindow *p) {
          for (wxWindow *c : p->GetChildren()) {
            if (field) return;
            wxTextCtrl *t = dynamic_cast<wxTextCtrl*>(c);
            if (t && t->IsShownOnScreen() && t->IsEditable() &&
                !t->IsMultiLine())
              field = t;
            else
              find(c);
          }
        };
        if (w[1] == "text") {
          find(this);
          if (field) {
            field->SetValue("abc");
            field->SetFocus();
            field->SelectAll();
          }
        } else if (SoWxRenderArea *area = findViewerArea(p_viewer)) {
          area->SetFocus();
        }
        for (int i = 0; i < 5; i++) wxTheApp->Yield(true);
        wxWindow *focus = wxWindow::FindFocus();
        wxCommandEvent ev(wxEVT_MENU, id);
        GetEventHandler()->ProcessEvent(ev);
        outcome = std::string("ok focus=") +
                  (focus ? wxString(focus->GetClassInfo()->GetClassName()) : wxString("none"))
                      .ToStdString();
        if (field) outcome += " text=[" + field->GetValue().ToStdString() + "]";
        else if (w[1] == "text") outcome = "FAIL: no text field";
        if (frag)
          outcome += " selected=" + std::to_string(frag->m_atomHighLight.size());
      } else if (w[0] == "pertab" && w.size() == 2) {
        //  The Build panel's periodic-table button, then a pick in the
        //  table it opens: the Build element must follow.
        wxWindow *button = p_pertab->FindWindow(ID_SHOW_TABLE);
        if (!button) {
          outcome = "FAIL: no periodic-table button";
        } else {
          wxCommandEvent press(wxEVT_BUTTON, ID_SHOW_TABLE);
          press.SetEventObject(button);
          button->GetEventHandler()->ProcessEvent(press);
          for (int i = 0; i < 3; i++) wxTheApp->Yield(true);
          PerTabPanel *table =
              dynamic_cast<PerTabPanel*>(p_pertab->tablePanel());
          TPerTab tpt;
          const int z = tpt.atomicNumber(w[1]);
          if (!table) {
            outcome = "FAIL: the button opened no table";
          } else {
            table->setCurrentElement(z);
            table->fireSelectionChanged(z);
            fprintf(stderr, "BUILDER: pertab element=%s\n",
                    p_currentElement.c_str());
          }
        }
      } else if (w[0] == "popup" && w.size() == 2) {
        //  The element chooser, as the arrow beside the element tool opens
        //  it, and where it is, straight after it is shown and half a
        //  second later, with the mode toolbar's own rectangle.
        TearableContent *tc = showElementChooser();
        const wxRect bar(p_modeToolbar->GetScreenPosition(),
                         p_modeToolbar->GetSize());
        wxRect tool = ewxToolRect(p_modeToolbar, ID_MODE_ELEMENT_LIST);
        tool.SetPosition(p_modeToolbar->ClientToScreen(tool.GetPosition()));
        fprintf(stderr, "BUILDER popup %s tool: %d,%d %dx%d\n", w[1].c_str(),
                tool.x, tool.y, tool.width, tool.height);
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
                     : w[1] == "select" ? ID_MODE_SELECT
                     : w[1] == "bond" ? ID_MODE_BOND : ID_MODE_STRUCTLIB;
        p_modeMenu->Check(id, true);
        wxCommandEvent modeEv(wxEVT_MENU, id);
        GetEventHandler()->ProcessEvent(modeEv);
      } else if (w[0] == "key" && w.size() == 2 && w[1] == "escape") {
        //  What the Esc key hook does with the 3-D view focused.
        if (SoWxRenderArea *area = findViewerArea(p_viewer)) area->SetFocus();
        for (int i = 0; i < 3; i++) wxTheApp->Yield(true);
        outcome = std::string("ok acted=") + (escapeKey() ? "1" : "0");
      } else if (w[0] == "key" && w.size() == 2 && w[1] == "delete") {
        //  What the Delete accelerator sends.
        wxCommandEvent delEv(wxEVT_MENU, ID_ACCEL_DELETE);
        GetEventHandler()->ProcessEvent(delEv);
      } else if (w[0] == "delete" && w.size() == 3 &&
                 (w[1] == "menu" || w[1] == "key") &&
                 (w[2] == "viewer" || w[2] == "log" || w[2] == "text")) {
        //  Edit > Delete Selection (menu) or the Delete accelerator (key)
        //  with the focus in the 3-D view, the Log, or an editable field
        //  holding "abc"; reports the atoms left and the field's text.
        wxTextCtrl *field = 0;
        if (w[2] == "log") {
          field = dynamic_cast<wxTextCtrl*>(p_mgr.GetPane(NAME_TOOL_LOG).window);
          if (field) field->SetFocus();
        } else if (w[2] == "text") {
          std::function<void(wxWindow*)> find = [&](wxWindow *p) {
            for (wxWindow *c : p->GetChildren()) {
              if (field) return;
              wxTextCtrl *t = dynamic_cast<wxTextCtrl*>(c);
              if (t && t->IsShownOnScreen() && t->IsEditable() &&
                  !t->IsMultiLine())
                field = t;
              else
                find(c);
            }
          };
          find(this);
          if (field) {
            field->SetValue("abc");
            field->SetFocus();
            field->SelectAll();
          }
        } else if (SoWxRenderArea *area = findViewerArea(p_viewer)) {
          area->SetFocus();
        }
        for (int i = 0; i < 5; i++) wxTheApp->Yield(true);
        wxWindow *focus = wxWindow::FindFocus();
        if (w[2] != "viewer" && !field) {
          outcome = "FAIL: no field";
        } else {
          wxCommandEvent ev(wxEVT_MENU, w[1] == "menu" ? wxID_DELETE
                                                        : ID_ACCEL_DELETE);
          GetEventHandler()->ProcessEvent(ev);
          outcome = std::string("ok focus=") +
                    (focus ? wxString(focus->GetClassInfo()->GetClassName())
                           : wxString("none")).ToStdString() +
                    " atoms=" + std::to_string(frag ? (int)frag->numAtoms() : -1);
          if (w[2] == "text")
            outcome += " text=[" + field->GetValue().ToStdString() + "]";
        }
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
        fprintf(stderr, "CONTEXTS: %d\n", (int)p_calculations.size());
        {
          wxAuiPaneInfo &cp = p_mgr.GetPane(static_cast<wxWindow*>(p_contextPanel));
          fprintf(stderr, "CONTEXTPANE: min_height=%d rows=%d\n",
                  cp.min_size.y, (int)p_calculations.size());
        }
        LatticeDef *lat = frag ? frag->getLattice() : 0;
        int nubs = 0;
        for (size_t i = 0; frag && i < frag->numAtoms(); i++)
          if (frag->atomRef(i)->atomicSymbol() == "Nub") nubs++;
        fprintf(stderr, "BUILDER: atoms=%d lattice=%s nubs=%d\n",
                frag ? (int)frag->numAtoms() : -1, lat ? "yes" : "no", nubs);
        fprintf(stderr, "BUILDER: frame=%lld\n", ewxModalGuard::frameCounter(this));
        fprintf(stderr, "BUILDER: periodic_builder=%s\n",
                p_mgr.GetPane(NAME_TOOL_PBC).IsShown() ? "shown" : "hidden");
        //  The mode in effect, the mode the toolbar shows and whether the
        //  viewer picks (a click selects) must name the same thing.
        auto modeName = [](int id) {
          return id == ID_MODE_ATOM ? "atom" : id == ID_MODE_SELECT ? "select"
               : id == ID_MODE_STRUCTLIB ? "structlib" : "other";
        };
        int shown = 0, nShown = 0;
        for (int id = ID_MODE_SELECT; id <= ID_MODE_STRUCTLIB; id++)
          if (p_modeToolbar->FindById(id) && p_modeToolbar->GetToolState(id)) {
            shown = id;
            nShown++;
          }
        fprintf(stderr, "BUILDER: mode=%s toolbar=%s checked=%d viewer=%s\n",
                modeName(p_currentMode), modeName(shown), nShown,
                p_viewer->isViewing() ? "viewing" : "picking");
      } else if (w[0] == "libatom" && w.size() == 2) {
        p_structLib->setTestAtom(atoi(w[1].c_str()));
      } else if (w[0] == "select" && w.size() == 2 && frag) {
        //  The atom (or Nub) a click on it would select, by index.
        frag->m_atomHighLight.clear();
        if (atoi(w[1].c_str()) >= 0)
          frag->m_atomHighLight.assign(1, atoi(w[1].c_str()));
      } else if (w[0] == "selectelem" && w.size() == 2 && frag) {
        //  The first atom of element A, as a click on it selects it.
        outcome = "FAIL: no such atom";
        for (size_t i = 0; i < frag->numAtoms(); i++)
          if (frag->atomRef(i)->atomicSymbol() == w[1]) {
            frag->m_atomHighLight.assign(1, (int)i);
            outcome = "ok";
            break;
          }
      } else if (w[0] == "addelem" && w.size() == 2 && frag) {
        //  Add the last atom of element A to the selection (a shift-click).
        outcome = "FAIL: no such atom";
        for (size_t i = frag->numAtoms(); i-- > 0; )
          if (frag->atomRef(i)->atomicSymbol() == w[1]) {
            frag->m_atomHighLight.push_back((int)i);
            outcome = "ok";
            break;
          }
      } else if (w[0] == "bondorder" && w.size() == 2) {
        //  The order the Bond mode makes (the Bond tool's choice).
        p_currentBond = w[1] == "metal" ? TBond::Metal : TBond::Single;
      } else if (w[0] == "clean") {
        //  The Clean command of the viewer's menu, with its default settings.
        execute(new CleanCmd("Force Field Clean", getSG(), 0));
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
      } else if (w[0] == "atomclick" && w.size() == 3 && frag) {
        //  A left click on atom N (1-based) in the viewer, as wx delivers it,
        //  with no key, Shift or Ctrl down.  Prints the selection after it.
        SoWxRenderArea *area = findViewerArea(p_viewer);
        if (!area) {
          outcome = "FAIL: no render area";
        } else {
          SbVec3f p = frag->getAtomCoordinates(atoi(w[2].c_str()) - 1), d;
          const SbViewportRegion& vpr = p_viewer->getViewportRegion();
          SbViewVolume vv =
              p_viewer->getCamera()->getViewVolume(vpr.getViewportAspectRatio());
          vv.projectToScreen(p, d);
          SbVec2s sz = vpr.getViewportSizePixels();
          const double sc = area->GetContentScaleFactor();
          const int lx = (int)std::lround(d[0] * sz[0] / sc);
          const int ly = (int)std::lround((sz[1] - 1 - d[1] * sz[1]) / sc);
          static long stamp = 100000;
          for (int k = 0; k < 2; k++) {
            wxMouseEvent e(k == 0 ? wxEVT_LEFT_DOWN : wxEVT_LEFT_UP);
            e.m_x = lx;
            e.m_y = ly;
            e.m_leftDown = (k == 0);
            e.SetShiftDown(w[1] == "shift");
            e.SetControlDown(w[1] == "ctrl");
            e.SetEventObject(area);
            e.SetTimestamp(stamp += 50);
            area->GetEventHandler()->ProcessEvent(e);
          }
          wxTheApp->Yield(true);
          std::string sel;
          for (size_t i = 0; i < frag->m_atomHighLight.size(); i++)
            sel += " " + std::to_string(frag->m_atomHighLight[i] + 1);
          fprintf(stderr, "BUILDER: selected after %s click on %s:%s\n",
                  w[1].c_str(), w[2].c_str(), sel.c_str());
        }
      } else if ((w[0] == "atompos" && w.size() == 2 && frag) ||
                 (w[0] == "viewpos" && w.size() == 3)) {
        //  Where atom N (1-based), or the point at fractions fx fy of the
        //  view from its top left, is on the screen, for a test that clicks
        //  there through the X server.
        SoWxRenderArea *area = findViewerArea(p_viewer);
        if (!area) {
          outcome = "FAIL: no render area";
        } else {
          wxPoint at;
          if (w[0] == "atompos") {
            SbVec3f p = frag->getAtomCoordinates(atoi(w[1].c_str()) - 1), d;
            const SbViewportRegion& vpr = p_viewer->getViewportRegion();
            SbViewVolume vv = p_viewer->getCamera()->getViewVolume(
                vpr.getViewportAspectRatio());
            vv.projectToScreen(p, d);
            SbVec2s sz = vpr.getViewportSizePixels();
            const double sc = area->GetContentScaleFactor();
            at = wxPoint((int)std::lround(d[0] * sz[0] / sc),
                         (int)std::lround((sz[1] - 1 - d[1] * sz[1]) / sc));
          } else {
            const wxSize sz = area->GetClientSize();
            at = wxPoint((int)(atof(w[1].c_str()) * sz.x),
                         (int)(atof(w[2].c_str()) * sz.y));
          }
          at = area->ClientToScreen(at);
          fprintf(stderr, "BUILDER: %s %s at %d %d\n", w[0].c_str(),
                  w[1].c_str(), at.x, at.y);
        }
      } else if (w[0] == "element" && w.size() == 2) {
        //  The element chosen in the Build panel.
        EventDispatcher::getDispatcher().publish(Event("ElementChanged", w[1]));
      } else if (w[0] == "coords" && frag) {
        for (size_t i = 0; i < frag->numAtoms(); i++) {
          const double *c = frag->atomRef(i)->coordinates();
          fprintf(stderr, "BUILDER: atom %d %s %.4f %.4f %.4f\n", (int)i + 1,
                  frag->atomRef(i)->atomicSymbol().c_str(), c[0], c[1], c[2]);
        }
        fprintf(stderr, "BUILDER: formula=%s\n", frag->formula().c_str());
      } else if (w[0] == "constraints" && w.size() >= 2) {
        //  Geometry Constraints (#306): "show", "select I J ..", "press
        //  constrain|restrain|fix yes|no", "list", "stored".
        if (!p_constraintsDialog) showConstraints();
        GeomConstraintsDialog *dlg = p_constraintsDialog;
        if (w[1] == "show") {
          showConstraints();
          fprintf(stderr, "BUILDER: constraints shown=%d\n", dlg->IsShown() ? 1 : 0);
        } else if (w[1] == "select" && frag) {
          frag->m_atomHighLight.clear();
          for (size_t i = 2; i < w.size(); i++)
            frag->m_atomHighLight.push_back(atoi(w[i].c_str()));
          EventDispatcher::getDispatcher().publish(Event("SelectionChanged"));
          fprintf(stderr, "BUILDER: constraints pick: %s\n", dlg->selectionText().c_str());
        } else if (w[1] == "press" && w.size() == 4) {
          fprintf(stderr, "BUILDER: constraints %s: %s\n", w[2].c_str(),
                  dlg->press(w[2], w[3] == "yes").c_str());
        } else if (w[1] == "list") {
          std::vector<std::string> rows = dlg->rowTexts();
          for (size_t i = 0; i < rows.size(); i++)
            fprintf(stderr, "BUILDER: constraint row: %s\n", rows[i].c_str());
          fprintf(stderr, "BUILDER: constraints rows=%d note: %s\n",
                  (int)rows.size(), dlg->noteText().c_str());
        } else if (w[1] == "stored") {
          //  What the calculation itself holds, read back as the input
          //  writers read it.
          ChemistryTask *task = dynamic_cast<ChemistryTask*>(p_calculation);
          int n = -1;
          if (task && frag) {
            GeomConstraintModel m(*frag, GeomConstraintRules());
            task->getGeomConstraintModel(m);
            n = m.size();
          }
          fprintf(stderr, "BUILDER: constraints stored=%d\n", n);
        }
      } else if (w[0] == "dummy" && w.size() >= 2) {
        //  Add Dummy Atom (#306): "dummy centre|beyond|between [D]" on the
        //  selection.
        int how = w[1] == "centre" ? DUMMY_CENTRE :
                  (w[1] == "between" ? DUMMY_BETWEEN : DUMMY_BEYOND);
        std::string err = addDummyAtom(how, w.size() > 2 ? atof(w[2].c_str()) : 1.0);
        fprintf(stderr, "BUILDER: dummy %s: %s\n", w[1].c_str(),
                err.empty() ? "added" : ("refused: " + err).c_str());
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

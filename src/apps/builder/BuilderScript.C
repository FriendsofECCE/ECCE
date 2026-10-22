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
//    cell residue|atom ROW COL TEXT [enter|esc|none]
//                                type TEXT into that cell of the shown table
//                                and press the key; prints the cell and
//                                whether its editor is still open
//    residues                    the residue names of the structure, on stderr
//    menustate NAME              whether Tools > NAME is enabled, on stderr
//    save                        File > Save
//    savemenu                    the File > Save menu event, if enabled
//    savestate                   dirty flag and Save menu/toolbar enabled state
//    context URL                 switch to (or back to) calculation URL
//    info                       atoms and lattice, on stderr
//    libatom I                   select atom (a Nub) I of the library structure
//    select I                    select atom (or Nub) I, as a click does; -1 none
//    selectelem A                select the first atom of element A
//    addelem A                   add the last atom of element A to the selection
//    bondorder single|metal      the order Bond mode makes
//    clean                       the viewer's Clean command
//    key escape                  what the Esc key does with the view focused
//    dclick A                    a double-click on the last atom of element A
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
//    tablecopy PANEL             Copy all of the first table in PANEL
//    inview                      whether an atom is outside the 3-D view
//    zoom F                      camera view angle/height times F (<1: in)
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
//    chooser kind|pick SYM|search TEXT|enter TEXT|shot NAME|close
//                                the element chooser popup: pick an element,
//                                type a search, press Enter in it, save a PNG
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
#include <wx/weakref.h>
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
#include "tdat/Residue.H"
#include "wxviz/MoleculeDataTable.H"
#include "tdat/PropVector.H"
#include "tdat/TBond.H"
#include "util/EventDispatcher.H"
#include "util/Event.H"
#include "wxviz/ViewerEvtHandler.H"
#include "wxviz/SGSelection.H"
#include "tdat/TPerTab.H"
#include "MiniPerTab.H"
#include "ElementChooser.H"
#include "viz/VRVector.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "viz/FindSymmetryCmd.H"
#include "wxgui/ewxGrid.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/PerTabPanel.H"
#include "wxgui/TearableContent.H"
#include "Builder.H"
#include "ContextPanel.H"
#include "GeomConstraintsDialog.H"
#include "inv/SoWx/SoWxRenderArea.H"
#include "inv/nodes/SoCamera.H"
#include "inv/nodes/SoOrthographicCamera.H"
#include "inv/nodes/SoPerspectiveCamera.H"
#include "wxviz/SGViewer.H"
#include "dsm/ChemistryTask.H"
#include "wxgui/SliderCombo.H"
#include "NModePanel.H"
#include "ScanPanel.H"
#include "IRCPanel.H"
#include "ReactionPanel.H"
#include "PullPanel.H"
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
          wxWindow *panel = p_pertab->tablePanel();
          ElementChooser *table = dynamic_cast<ElementChooser*>(panel);
          PerTabPanel *old = dynamic_cast<PerTabPanel*>(panel);
          if (!table && !old) {
            outcome = "FAIL: the button opened no table";
          } else {
            if (table) {
              table->pick(w[1]);
            } else {
              TPerTab tpt;
              const int z = tpt.atomicNumber(w[1]);
              old->setCurrentElement(z);
              old->fireSelectionChanged(z);
            }
            fprintf(stderr, "BUILDER: pertab element=%s\n",
                    p_currentElement.c_str());
          }
        }
      } else if (w[0] == "chooser" && w.size() >= 2) {
        //  The element chooser popup: "chooser pick Fe", "chooser search
        //  iron" (then "chooser enter iron" to press Enter), "chooser
        //  shot NAME" for a screenshot of the open popup, "chooser close".
        static wxWeakRef<TearableContent> popup;
        if (!popup) popup = showElementChooser();
        wxWindow *found = popup->FindWindow("Elements");
        ElementChooser *ch = dynamic_cast<ElementChooser*>(found);
        for (int i = 0; i < 5; i++) wxTheApp->Yield(true);
        if (w[1] == "kind") {
          std::string recent;
          if (ch) for (const std::string& r : ch->recent()) recent += r + ",";
          fprintf(stderr, "BUILDER: chooser kind=%s recent=%s\n",
                  ch ? "new" : dynamic_cast<PerTabPanel*>(found) ? "old" : "none",
                  recent.c_str());
        } else if (w[1] == "screen" && w.size() == 3) {
          //  The whole display: the Builder window with the popup over it.
          wxScreenDC cdc;
          const wxSize sz = wxGetDisplaySize();
          wxBitmap bmp(sz.x, sz.y);
          wxMemoryDC mdc(bmp);
          mdc.Blit(0, 0, sz.x, sz.y, &cdc, 0, 0);
          mdc.SelectObject(wxNullBitmap);
          bmp.SaveFile(st->dir + "/" + w[2] + ".png", wxBITMAP_TYPE_PNG);
        } else if (!ch) {
          outcome = "FAIL: no chooser";
        } else if (w[1] == "pick" && w.size() == 3) {
          fprintf(stderr, "BUILDER: chooser pick %s -> %d\n", w[2].c_str(),
                  ch->pick(w[2]) ? 1 : 0);
          fprintf(stderr, "BUILDER: chooser element=%s\n", p_currentElement.c_str());
        } else if ((w[1] == "search" || w[1] == "enter") && w.size() >= 3) {
          std::string text = w[2];
          for (size_t i = 3; i < w.size(); i++) text += " " + w[i];
          const int n = ch->search(text, w[1] == "enter");
          fprintf(stderr, "BUILDER: chooser matches=%d element=%s\n", n,
                  p_currentElement.c_str());
        } else if (w[1] == "shot" && w.size() == 3) {
          wxScreenDC cdc;
          const wxPoint org = ch->GetScreenPosition();
          wxSize sz = ch->GetClientSize();
          wxBitmap bmp(sz.x, sz.y);
          wxMemoryDC mdc(bmp);
          mdc.Blit(0, 0, sz.x, sz.y, &cdc, org.x, org.y);
          mdc.SelectObject(wxNullBitmap);
          bmp.SaveFile(st->dir + "/" + w[2] + ".png", wxBITMAP_TYPE_PNG);
        }
        if (w[1] == "close" && popup) popup->Dismiss();
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
      } else if (w[0] == "cell" && w.size() >= 5) {
        wxWindow *win = p_mgr.GetPane(w[1] == "atom" ? NAME_TOOL_ATOM_TABLE
                                                     : NAME_TOOL_RESIDUE_TABLE).window;
        MoleculeDataTable *tbl = dynamic_cast<MoleculeDataTable*>(win);
        if (!tbl) {
          outcome = "no table";
        } else {
          std::string key = w.size() > 5 ? w[5] : "enter";
          outcome = tbl->scriptEdit(atoi(w[2].c_str()), atoi(w[3].c_str()), w[4], key);
        }
      } else if (w[0] == "menustate" && w.size() >= 2) {
        std::string name = line.substr(line.find("menustate") + 10);
        int id = p_toolMenu->FindItem(name);
        fprintf(stderr, "BUILDER: menu %s enabled=%d\n", name.c_str(),
                id == wxNOT_FOUND ? -1 : (p_toolMenu->IsEnabled(id) ? 1 : 0));
      } else if (w[0] == "residues") {
        std::string names;
        for (int i = 0; i < frag->numResidues(); i++)
          names += (i ? "," : "") + frag->residue(i)->name();
        fprintf(stderr, "BUILDER: residues=%s\n", names.c_str());
      } else if (w[0] == "save") {
        doSave();
      } else if (w[0] == "context" && w.size() == 2) {
        //  Switch to (or back to) calculation URL, as the Organizer does.
        setContext(w[1]);
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
        //  The order the Bond mode makes, sent as the Bond tool and the
        //  Build panel's list send it.
        const std::string order = w[1] == "metal" ? "Metal-Pi"
            : w[1] == "double" ? "Double" : w[1] == "triple" ? "Triple"
            : w[1] == "aromatic" ? "Aromatic" : "Single";
        ecce::EventDispatcher::getDispatcher().publish(
            ecce::Event("BondChanged", order));
        fprintf(stderr, "BUILDER: bondorder current=%g\n", p_currentBond);
      } else if (w[0] == "selectaction" && w.size() == 2) {
        //  View > Select Mode Action: Atom Drag or Lasso Select.
        const int id = w[1] == "lasso" ? ViewerEvtHandler::ID_SELECT_LASSO
                                       : ViewerEvtHandler::ID_SELECT_DRAG;
        GetMenuBar()->Check(id, true);
        wxCommandEvent ev(wxEVT_MENU, id);
        ev.SetInt(1);
        GetEventHandler()->ProcessEvent(ev);
      } else if (w[0] == "selectbonds" && frag) {
        //  The bonds between selected atoms too, as a drag box selects.
        vector<TBond*> *all = frag->bonds();
        const vector<int>& hl = frag->m_atomHighLight;
        for (size_t i = 0; i < all->size(); i++) {
          TBond *b = (*all)[i];
          if (find(hl.begin(), hl.end(), b->atom1()->index()) != hl.end() &&
              find(hl.begin(), hl.end(), b->atom2()->index()) != hl.end())
            frag->m_bondHighLight.push_back((int)i);
        }
        delete all;
      } else if (w[0] == "selection" && frag) {
        //  What is selected, and whether a drag in the view draws a box.
        fprintf(stderr, "BUILDER: selection atoms=%d bonds=%d drag=%d "
                "lasso=%d\n", (int)frag->m_atomHighLight.size(),
                (int)frag->m_bondHighLight.size(),
                p_viewer->getSelectModeDrag() ? 1 : 0,
                (int)p_viewer->getSel()->lassoType.getValue());
      } else if (w[0] == "bondorders" && frag) {
        //  Each bond between real atoms: its stored order and the type
        //  the display draws it as.
        vector<TBond*> *all = frag->bonds();
        for (size_t i = 0; i < all->size(); i++) {
          TBond *b = (*all)[i];
          if (b->atom1()->atomicSymbol() == "Nub" ||
              b->atom2()->atomicSymbol() == "Nub") continue;
          fprintf(stderr, "BUILDER: bond %d-%d %s-%s order=%g drawn=%d\n",
                  b->atom1()->index(), b->atom2()->index(),
                  b->atom1()->atomicSymbol().c_str(),
                  b->atom2()->atomicSymbol().c_str(), b->order(),
                  frag->getBondType((int32_t)i));
        }
        delete all;
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
      } else if (w[0] == "dclick" && w.size() == 2 && frag) {
        //  A double-click on the last atom of element A, as the viewer
        //  reports it: two presses on that atom, the second flagged.
        int idx = -1;
        for (size_t i = 0; i < frag->numAtoms(); i++)
          if (frag->atomRef(i)->atomicSymbol() == w[1]) idx = (int)i;
        if (idx < 0) {
          outcome = "FAIL: no such atom";
        } else {
          frag->m_atomHighLight.assign(1, idx);
          pickedInMode(false, -1);
          frag->m_atomHighLight.assign(1, idx);
          pickedInMode(true, idx);
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
        //  "rows" takes the rest of the line, so a panel name may hold
        //  spaces ("MD Summary").
        string pane = w[1];
        if (w[0] == "rows")
          for (size_t i = 2; i < w.size(); i++) pane += " " + w[i];
        KeyValuePropertyPanel *kv = dynamic_cast<KeyValuePropertyPanel*>(
            p_mgr.GetPane(wxString(pane)).window);
        if (!kv) {
          outcome = "no key/value panel " + pane;
        } else if (w[0] == "fold") {
          if (w.size() < 3) outcome = "fold needs a group";
          else kv->toggleGroup(w[2]);
        } else {
          fprintf(stderr, "BUILDER: rows %s\n", pane.c_str());
          for (const std::string& row : kv->rowDump())
            fprintf(stderr, "BUILDER: row %s\n", row.c_str());
          for (const std::string& fit : kv->rowFit())
            fprintf(stderr, "BUILDER: fit %s\n", fit.c_str());
          for (const std::string& sel : kv->rowSelectable())
            fprintf(stderr, "BUILDER: selectable %s\n", sel.c_str());
          //  what Copy all (#408) puts on the clipboard
          wxString all = kv->copyAllText();
          all.Replace("\t", " \\t ");
          for (const wxString& line : wxSplit(all, '\n'))
            if (!line.empty())
              fprintf(stderr, "BUILDER: copy %s\n", line.ToUTF8().data());
        }
      } else if (w[0] == "zoom" && w.size() == 2) {
        //  zoom F: the camera's view angle or height times F (F < 1 zooms
        //  in), as the mouse wheel would.
        SoCamera *cam = p_viewer->getCamera();
        const float f = (float)atof(w[1].c_str());
        if (SoPerspectiveCamera *pc = dynamic_cast<SoPerspectiveCamera*>(cam))
          pc->heightAngle = pc->heightAngle.getValue() * f;
        else if (SoOrthographicCamera *oc = dynamic_cast<SoOrthographicCamera*>(cam))
          oc->height = oc->height.getValue() * f;
        p_viewer->refreshRenderArea();
      } else if (w[0] == "inview") {
        //  inview: whether any atom is outside the 3-D view (#409).
        fprintf(stderr, "BUILDER: inview outside=%d\n",
                atomsOutsideView() ? 1 : 0);
      } else if (w[0] == "tablecopy" && w.size() >= 2) {
        //  tablecopy <panel>: what Copy all (#408) puts on the clipboard
        //  from the first table in that panel.
        string pane = w[1];
        for (size_t i = 2; i < w.size(); i++) pane += " " + w[i];
        std::function<ewxGrid*(wxWindow*)> findGrid =
            [&findGrid](wxWindow *win) -> ewxGrid* {
          if (ewxGrid *g = dynamic_cast<ewxGrid*>(win)) return g;
          for (wxWindow *child : win->GetChildren())
            if (ewxGrid *g = findGrid(child)) return g;
          return 0;
        };
        wxWindow *win = p_mgr.GetPane(wxString(pane)).window;
        ewxGrid *grid = win ? findGrid(win) : 0;
        if (!grid) {
          outcome = "no table in panel " + pane;
        } else {
          //  what a double-click on the first cell shows: selectable text
          wxTextCtrl *cell = grid->showCellText(0, 0);
          if (cell)
            fprintf(stderr, "BUILDER: tablecell editable=%d selected='%s'\n",
                    cell->IsEditable() ? 1 : 0,
                    cell->GetStringSelection().ToUTF8().data());
          wxString all = grid->copyText(true);
          all.Replace("\t", " \\t ");
          for (const wxString& line : wxSplit(all, '\n'))
            if (!line.empty())
              fprintf(stderr, "BUILDER: tablecopy %s\n", line.ToUTF8().data());
        }
      } else if (w[0] == "spectrumcsv" && w.size() >= 2) {
        //  spectrumcsv <path> [ir|raman]: the Vibrational Frequencies
        //  panel's Export .csv (#392) without the file dialog.
        NModePanel *nm = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString()))
            if ((nm = dynamic_cast<NModePanel*>(p)) != 0) break;
        if (!nm) {
          outcome = "no Vibrational Frequencies panel";
        } else {
          const VibKind kind = w.size() > 2 && w[2] == "raman" ? VIB_RAMAN
                                                              : VIB_IR;
          fprintf(stderr, "BUILDER: spectrumcsv %s\n",
                  nm->exportSpectrumCsv(wxString::FromUTF8(w[1].c_str()), kind)
                      ? "ok" : "failed");
        }
      } else if (w[0] == "scancsv" && w.size() >= 2) {
        //  scancsv <path> [kcal]: the Relaxed Scan panel's Export .csv
        //  (#393) without the file dialog.
        ScanPanel *sp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString()))
            if ((sp = dynamic_cast<ScanPanel*>(p)) != 0) break;
        if (!sp) {
          outcome = "no Relaxed Scan panel";
        } else {
          fprintf(stderr, "BUILDER: scancsv points=%d %s\n", sp->points(),
                  sp->exportCsv(wxString::FromUTF8(w[1].c_str()),
                                w.size() > 2 && w[2] == "kcal")
                      ? "ok" : "failed");
        }
      } else if (w[0] == "pullcsv" && w.size() >= 2) {
        //  pullcsv <path>: the Pull Force panel's Export .csv without the
        //  file dialog.
        PullPanel *pp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString()))
            if ((pp = dynamic_cast<PullPanel*>(p)) != 0) break;
        if (!pp) {
          outcome = "no Pull Force panel";
        } else {
          fprintf(stderr, "BUILDER: pullcsv points=%d %s\n", pp->points(),
                  pp->exportCsv(wxString::FromUTF8(w[1].c_str())) ? "ok" : "failed");
        }
      } else if (w[0] == "rxnenergies" && w.size() >= 2) {
        //  rxnenergies <path> [kcal]: the Reaction Energies panel's Export
        //  .csv (#419) without the file dialog.
        ReactionPanel *rp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString()))
            if ((rp = dynamic_cast<ReactionPanel*>(p)) != 0) break;
        if (!rp) {
          outcome = "no Reaction Energies panel";
        } else {
          fprintf(stderr, "BUILDER: rxnenergies levels=%d problems=%d warnings=%d %s\n",
                  (int)rp->result().levels.size(), (int)rp->result().problems.size(),
                  (int)rp->result().warnings.size(),
                  rp->exportCsv(wxString::FromUTF8(w[1].c_str()),
                                w.size() > 2 && w[2] == "kcal")
                      ? "ok" : "failed");
        }
      } else if (w[0] == "ircpath" && w.size() >= 2) {
        //  ircpath <path> [kcal]: the IRC Path panel's Export .csv (#417)
        //  without the file dialog.
        IRCPanel *ip = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString()))
            if ((ip = dynamic_cast<IRCPanel*>(p)) != 0) break;
        if (!ip) {
          outcome = "no IRC Path panel";
        } else {
          fprintf(stderr, "BUILDER: ircpath points=%d %s\n", ip->points(),
                  ip->exportCsv(wxString::FromUTF8(w[1].c_str()),
                                w.size() > 2 && w[2] == "kcal")
                      ? "ok" : "failed");
        }
      } else if ((w[0] == "scanclick" || w[0] == "scancalc" ||
                  w[0] == "irccalc") && w.size() >= 2) {
        //  scanclick <i>: show scan point i; scancalc/irccalc <i>: the
        //  "New calculation from this point" action (#410).
        ScanPanel *sp = 0;
        IRCPanel *ip = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString())) {
            if (!sp) sp = dynamic_cast<ScanPanel*>(p);
            if (!ip) ip = dynamic_cast<IRCPanel*>(p);
          }
        const int idx = atoi(w[1].c_str());
        if (w[0] == "irccalc" ? !ip : !sp) {
          outcome = "no such panel";
        } else if (w[0] == "scanclick") {
          const int frame = sp->clickPoint(idx);
          std::string where;
          SGFragment *frag = getSceneGraph().getFragment();
          for (int a = 0; frag && a < 2 && a < frag->numAtoms(); a++) {
            const double *c = frag->atomRef(a)->coordinates();
            char buf[96];
            snprintf(buf, sizeof buf, " %.5f %.5f %.5f", c[0], c[1], c[2]);
            where += buf;
          }
          fprintf(stderr, "BUILDER: scanclick %d frame=%d atoms:%s\n", idx, frame,
                  where.c_str());
        } else {
          const std::string url = w[0] == "irccalc" ? ip->newCalcFromPoint(idx)
                                                    : sp->newCalcFromPoint(idx);
          fprintf(stderr, "BUILDER: %s %d url=%s\n", w[0].c_str(), idx,
                  url.empty() ? "none" : url.c_str());
        }
      } else if ((w[0] == "scanrow" || w[0] == "scanstep") && w.size() >= 2) {
        //  scanrow <i>: a click on table row i; scanstep first|prev|next|last:
        //  the playback buttons.  Reports the selection and the viewer's
        //  first two atoms.
        ScanPanel *sp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString()))
            if ((sp = dynamic_cast<ScanPanel*>(p)) != 0) break;
        if (!sp) {
          outcome = "no Relaxed Scan panel";
        } else {
          if (w[0] == "scanrow") sp->selectRow(atoi(w[1].c_str()));
          else sp->step(w[1]);
          std::string where;
          SGFragment *frag = getSceneGraph().getFragment();
          for (int a = 0; frag && a < 2 && a < frag->numAtoms(); a++) {
            const double *c = frag->atomRef(a)->coordinates();
            char buf[96];
            snprintf(buf, sizeof buf, " %.5f %.5f %.5f", c[0], c[1], c[2]);
            where += buf;
          }
          fprintf(stderr, "BUILDER: %s %s point=%d row=%d marker=%d atoms:%s\n",
                  w[0].c_str(), w[1].c_str(), sp->selectedPoint(),
                  sp->selectedRow(), sp->markerPoint(), where.c_str());
        }
      } else if (w[0] == "scanstatus") {
        //  scanstatus: the Relaxed Scan panel's status line
        ScanPanel *sp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString()))
            if ((sp = dynamic_cast<ScanPanel*>(p)) != 0) break;
        fprintf(stderr, "BUILDER: scanstatus [%s]\n",
                sp ? sp->status().utf8_str().data() : "no panel");
      } else if (w[0] == "ircclick" && w.size() >= 2) {
        //  ircclick <index>: what a click on IRC Path plot point <index>
        //  does (#417); reports the frame shown and the first two atoms.
        IRCPanel *ip = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString()))
            if ((ip = dynamic_cast<IRCPanel*>(p)) != 0) break;
        if (!ip) {
          outcome = "no IRC Path panel";
        } else {
          const int frame = ip->clickPoint(atoi(w[1].c_str()));
          std::string where;
          SGFragment *frag = getSceneGraph().getFragment();
          for (int a = 0; frag && a < 2 && a < frag->numAtoms(); a++) {
            const double *c = frag->atomRef(a)->coordinates();
            char buf[96];
            snprintf(buf, sizeof buf, " %.5f %.5f %.5f", c[0], c[1], c[2]);
            where += buf;
          }
          if (frame < 0)
            fprintf(stderr, "BUILDER: ircclick %s frame=none atoms:%s\n",
                    w[1].c_str(), where.c_str());
          else
            fprintf(stderr, "BUILDER: ircclick %s frame=%d atoms:%s\n",
                    w[1].c_str(), frame, where.c_str());
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

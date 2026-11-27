//  ECCE_BUILDER_SCRIPT=<file>: steps of the Quantum ESPRESSO tutorial in the
//  real Builder window, one per timer tick, each through the handler a click
//  or Enter in the same control reaches.  Inert unless the variable is set.
//  '#' starts a comment.  Commands:
//    wait MS                     pause
//    add ELEM SHAPE X Y Z        a click in the 3D view in Atom mode
//    panel NAME                  Tools > NAME (shows the panel)
//    panesize NAME               the docked pane's rectangle and height floor
//    plist                       print the Properties list as GROUP/NAME lines (#1079)
//    vibnote                     print the Vibrations panel's small-imaginary-mode note (#1042)
//    specfactor TEXT             type TEXT into the spectrum's Freq. factor (#604)
//    specclick MODE              left click on a mode in the spectrum graph (#445)
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
//    undo                        Edit > Undo (the menu event)
//    ctrlz                       the Ctrl+Z key press, through the frame's key hook (#1247)
//    focus                       the class of the window with the keyboard focus
//    resatoms                    atoms per residue name (NAME=N,...), on stderr
//    import FILE                 File > Import of FILE, as the dialog does (#1244)
//    menustate NAME              whether Tools > NAME is enabled, on stderr
//    save                        File > Save
//    charge INTEGER | mult INTEGER   total charge / spin multiplicity
//    savemenu                    the File > Save menu event, if enabled
//    savestate                   dirty flag and Save menu/toolbar enabled state
//    context URL                 switch to (or back to) calculation URL
//    hbonds [DIST ANGLE [noions]] Show Hydrogen Bonds on, logs count and ms
//    info                       atoms and lattice, on stderr
//    libatom I                   select atom (a Nub) I of the library structure
//    select N                    select atom (or Nub) number N (from 1), as a click
//                                does; 0 or less none
//    select_within R [molecules] [replace]  extend by distance in Angstrom
//    cutcluster R [center N] [whole none|molecules|units] [cap none|H|OH]
//                                Build > Cut Cluster (#1033) around the selected
//                                atoms, or atom N (from 1); prints each atom
//                                of the result as `BUILDER: clusteratom EL X Y Z`
//    select_protein              Edit > Select Protein (replaces the selection)
//    select_ligand               Edit > Select Ligand: the largest ligand (replaces the selection)
//    ligands                     the Select Ligand submenu entries, likely ligand first (#1244)
//    ligand N                    Select Ligand > Nth entry (from 1)
//    select_hetero               Edit > Select Heteroatoms (not protein): all but protein and the ligand (#1244)
//    selectelem A                select the first atom of element A
//    addelem A                   add the last atom of element A to the selection
//    bondkind covalent|metal     the kind of bond Bond mode makes
//    toolbar bond                press the Bond button of the mode toolbar
//    clean                       the viewer's Clean command
//    bonddipoles on [KEY] | off  Dipole Moment panel: bond dipoles, logs the table
//    key escape                  what the Esc key does with the view focused
//    dclick A                    a double-click on the last atom of element A
//    selectnub A                 select the first Nub of the last atom of element A
//    bonds A B                   count the bonds from the last atom of element A
//                                to atoms of element B
//    expect atoms N | expect lattice yes|no
//    modal ok                    press OK in the open modal dialog (the XYZ
//                                units prompt of a file opened as context)
//    shot NAME                   writes NAME.ready next to the script and
//                                waits for NAME.go: the test photographs the
//                                window meanwhile
//    symmetry                    Symmetry > Find, then the point group
//    coordsraw                   every atom's coordinates at %.17g
//    fragments [auto]            report fragment assignments, or reset to molecules
//    fragment N atoms LIST       assign 1-based atoms (spaces, commas or ranges)
//    fragment N charge Q mult M  set fragment charge and spin multiplicity
//    counterpoise orca DIR [MULT [ROUTE...]]  export complex and ghost monomers
//    atomtable                   the shown text of each Atom Table row
//    progress N                  the MO Compute progress dialog, N seconds
//    property NAME               a click on Properties > NAME (toggles it)
//    columnwidth N               restore a saved side column width of N pixels
//    panesize NAME               report pane width, height and virtual width
//    minsizes NAME               each control's and sizer row's minimum width
//    list NAME                   a click on NAME in the Properties list
//    viewer NAME                 a click on NAME's "Show in viewer" box
//    overlay                     the column's tab (1 = Properties), and for
//                                each viewer panel: shown, viz focus
//    rows PANEL                  the rows of a key/value panel (Energies)
//    tablecopy PANEL             Copy all of the first table in PANEL
//    thermolayout                Thermochemistry label widths and clip audit
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
//    surface PROP                each angular surface (multipole moment) under
//                                PROP's property switch: vertices, triangles,
//                                centre, farthest vertex (Angstrom)
//    pertab A                   press the Build panel's periodic-table button,
//                                pick element A in it, report the Build element
//    chooser kind|pick SYM|search TEXT|enter TEXT|shot NAME|close
//                                the element chooser popup: pick an element,
//                                type a search, press Enter in it, save a PNG
//    findpockets                 one line per binding pocket of the structure
//    vinabox [pocket=N] [margin] the Vina search box around the selection or pocket N
//    quit
//  Each command is answered on stderr: "ECCE_BUILDER_SCRIPT: <line>: ok".

#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include "wxviz/VizRender.H"
#include <sys/stat.h>

#include <wx/aui/aui.h>
#include <wx/app.h>
#include <wx/checkbox.h>
#include <wx/dcmemory.h>
#include <wx/dcscreen.h>
#include <wx/dialog.h>
#include <wx/grid.h>
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
#include "inv/nodes/SoIndexedFaceSet.H"
#include "inv/nodes/SoCoordinate3.H"
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
#include "tdat/Counterpoise.H"
#include "dsm/ICalculation.H"
#include "util/EventDispatcher.H"
#include "util/Event.H"
#include "wxviz/ViewerEvtHandler.H"
#include "viz/HydrogenBondsCmd.H"
#include "wxviz/SGSelection.H"
#include "tdat/TPerTab.H"
#include "MiniPerTab.H"
#include "ElementChooser.H"
#include "viz/VRVector.H"
#include "inv/nodes/SoText2.H"
#include "viz/ForegroundCmd.H"
#include "wxgui/SliderCombo.H"
#include "viz/SGContainer.H"
#include "viz/SGLattice.H"
#include "viz/SGFragment.H"
#include "viz/SelectCmd.H"
#include "viz/UnselectCmd.H"
#include "viz/AtomMeasureDist.H"
#include "viz/AtomMeasureAngle.H"
#include "viz/AtomMeasureTorsion.H"

#include "viz/FindSymmetryCmd.H"
#include "viz/InsertResidueCmd.H"
#include "viz/GenerateDNACmd.H"
#include "wxgui/DiffractionDialog.H"
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
#include "tdat/PropVecString.H"
#include "MoPanel.H"
#include "ScanPanel.H"
#include <wx/stattext.h>
#include "IRCPanel.H"
#include "ReactionPanel.H"
#include "ThermoPanel.H"
#include "PullPanel.H"
#include "MdAnalysisPanels.H"
#include "GmxAnalysisPanel.H"
#include "XYPlotCanvas.H"
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
#include "MomentsPanel.H"
#include "StressPanel.H"
#include "NmrPanel.H"
#include "DistortBondsDialog.H"
#include "wxgui/ewxModalGuard.H"

namespace {

// "view=NAME" anywhere in a diffraction hook's words: taken out of the words,
// so the positional arguments keep their meaning, and returned as the pattern
// mode onPattern uses.  -1 if it was not given, -2 for an unknown name.
int takeViewWord(std::vector<std::string>& w)
{
  static const std::map<std::string, int> modes = {
    {"fibre", 0}, {"fiber", 0}, {"powder", 1}, {"single", 2},
    {"frame", 3}, {"debye", 4}};
  int mode = -1;
  for (size_t i = 1; i < w.size(); ) {
    if (w[i].compare(0, 5, "view=") == 0) {
      const std::map<std::string, int>::const_iterator it = modes.find(w[i].substr(5));
      mode = it == modes.end() ? -2 : it->second;
      w.erase(w.begin() + i);
    } else ++i;
  }
  return mode;
}

struct ScriptState {
  std::vector<std::string> lines;
  size_t next;
  wxLongLong notBefore;
  std::string waitingFor;      // a .go file
  wxLongLong waitUntil;
  std::function<bool()> busy;  // a background job the script waits for
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

bool fragmentInteger(const std::string& text, int& value)
{
  std::istringstream in(text);
  char extra;
  return (in >> value) && !(in >> extra);
}

bool fragmentAtomList(const std::vector<std::string>& words, int atomCount,
                      std::vector<int>& atoms)
{
  for (size_t i = 3; i < words.size(); ++i) {
    std::string token = words[i];
    for (char& c : token) if (c == ',') c = ' ';
    std::istringstream parts(token);
    for (std::string part; parts >> part;) {
      size_t dash = part.find('-');
      int first, last;
      if (dash == std::string::npos) {
        if (!fragmentInteger(part, first)) return false;
        last = first;
      } else if (!fragmentInteger(part.substr(0, dash), first) ||
                 !fragmentInteger(part.substr(dash + 1), last)) return false;
      if (first < 1 || last < first || last > atomCount) return false;
      for (int a = first; a <= last; ++a) atoms.push_back(a - 1);
    }
  }
  return !atoms.empty();
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

  wxTimer *timer = ewxWindowTimer(this);   // deleted with the window (#983)
  timer->Bind(wxEVT_TIMER, [this, st, timer](wxTimerEvent&) {
    if (!st->waitingFor.empty()) {
      if (exists(st->waitingFor) || wxGetLocalTimeMillis() > st->waitUntil)
        st->waitingFor.clear();
      else
        return;
    }
    if (st->busy) {
      if (st->busy() && wxGetLocalTimeMillis() < st->waitUntil) return;
      st->busy = nullptr;
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
      } else if (w[0] == "charge" || w[0] == "mult") {
        ICalculation *calc = dynamic_cast<ICalculation*>(p_calculation);
        int value;
        if (w.size() != 2 || !fragmentInteger(w[1], value)) {
          outcome = "FAIL: expected an integer";
        } else if (!frag || !calc || isReadOnly()) {
          outcome = "FAIL: no editable calculation";
        } else {
          const bool charge = w[0] == "charge";
          const int oldCharge = frag->charge();
          if (charge) frag->charge(value);
          int mult = charge ? (int)calc->spinMultiplicity() : value;
          const int electrons = frag->numElectrons();
          const bool allowed = mult > 0 && mult - 1 <= electrons &&
                               electrons % 2 == (mult - 1) % 2;
          // CalcEd retains an allowed spin after a charge edit, otherwise
          // it selects singlet/doublet from the new electron parity.
          if (charge && !allowed) mult = electrons % 2 ? 2 : 1;
          if (!charge && !allowed) {
            outcome = "FAIL: multiplicity incompatible with electron count";
          } else if (!calc->spinMultiplicity((SpinMult::SpinMultEnum)mult) ||
                     !calc->openShells(mult - 1)) {
            frag->charge(oldCharge);
            outcome = "FAIL: unable to store spin";
          } else {
            setDirty(true);
            updateSave();
            fprintf(stderr, "BUILDER: charge=%d mult=%d\n", frag->charge(), mult);
          }
        }
      } else if (w[0] == "fragments" || w[0] == "fragment" ||
                 w[0] == "counterpoise") {
        std::string error;
        bool changed = false;
        bool validSyntax = false;
        if (!frag || frag->numAtoms() == 0) {
          error = "No structure atoms";
        } else {
          bool unassigned = true;
          for (int i = 0; i < frag->numAtoms(); ++i)
            if (frag->atomRef(i)->fragmentId() != 0) unassigned = false;
          if (w[0] == "fragments" &&
              (w.size() == 1 || (w.size() == 2 && w[1] == "auto"))) {
            validSyntax = true;
            if (unassigned || w.size() == 2)
              changed = Counterpoise::autoAssign(*frag, error);
          } else if (w[0] == "fragment") {
            int id = 0, charge = 0, mult = 0;
            std::vector<int> atoms;
            const bool atomCommand = w.size() >= 4 && w[2] == "atoms" &&
                fragmentAtomList(w, frag->numAtoms(), atoms);
            const bool stateCommand = w.size() == 6 && w[2] == "charge" &&
                w[4] == "mult" && fragmentInteger(w[3], charge) &&
                fragmentInteger(w[5], mult);
            if (w.size() >= 3 && fragmentInteger(w[1], id) && id > 0 &&
                (atomCommand || stateCommand)) {
              validSyntax = true;
              if (unassigned) changed = Counterpoise::autoAssign(*frag, error);
              if (error.empty()) {
                bool ok = atomCommand
                    ? Counterpoise::assignAtoms(*frag, id, atoms, error)
                    : Counterpoise::setState(*frag, id, charge, mult, error);
                changed = changed || ok;
              }
            }
          } else if (w[0] == "counterpoise" && w.size() >= 3 && w[1] == "orca") {
            int mult = (frag->numElectrons() % 2) ? 2 : 1;
            if (ICalculation *calc = dynamic_cast<ICalculation*>(p_calculation)) {
              int stored = static_cast<int>(calc->spinMultiplicity());
              if (stored > 0) mult = stored;
            }
            validSyntax = w.size() == 3 || (fragmentInteger(w[3], mult) && mult > 0);
            if (validSyntax) {
              std::string route = "HF def2-SVP";
              if (w.size() > 4) {
                route.clear();
                for (size_t i = 4; i < w.size(); ++i)
                  route += (i == 4 ? "" : " ") + w[i];
              }
              if (unassigned) changed = Counterpoise::autoAssign(*frag, error);
              if (error.empty()) Counterpoise::writeOrcaInputs(*frag, w[2], route, mult, error);
              if (error.empty()) fprintf(stderr, "BUILDER: counterpoise orca: %s\n", w[2].c_str());
            }
          }
          if (!validSyntax && error.empty())
            error = "Usage: fragments [auto]; fragment N atoms LIST; fragment N charge Q mult M; counterpoise orca DIR [MULT [ROUTE...]]";
        }
        if (changed) { setDirty(true); updateSave(); }
        if (!error.empty()) {
          outcome = "FAIL: " + error;
          fprintf(stderr, "BUILDER: %s: refused: %s\n", w[0].c_str(), error.c_str());
        } else if (w[0] != "counterpoise") {
          fprintf(stderr, "BUILDER: fragments:\n%s", Counterpoise::query(*frag).c_str());
        }
      } else if (w[0] == "add" && w.size() == 6) {
        //  A free-space atom in a structure with residues goes into a new
        //  residue at the end, as Insert in the GUI's residue prompt does.
        SGFragment *rf = getSG()->getFragment();
        if (rf->numResidues() > 0 && rf->editResidueIndex() < 0 &&
            rf->getAtomHighlight()->empty()) {
          const int at = rf->numResidues();
          Command *ins = new InsertResidueCmd("Insert residue", getSG());
          ins->getParameter("name")->setString("UNK");
          ins->getParameter("index")->setInteger(at);
          execute(ins);
          getSG()->getFragment()->editResidueMode(true, at);
        }
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
      } else if (w[0] == "columnwidth" && w.size() == 2) {
        wxAuiPaneInfo &pane = p_mgr.GetPane(wxString(NAME_PROPERTY_INDEX));
        if (!pane.IsOk() || !pane.IsShown()) {
          outcome = "no Properties pane";
        } else {
          // Restore the dock size as a saved user layout does.
          std::string perspective = p_mgr.SavePerspective().ToStdString();
          const std::string key = wxString::Format("dock_size(%d,%d,%d)=",
              pane.dock_direction, pane.dock_layer, pane.dock_row).ToStdString();
          const size_t at = perspective.find(key);
          if (at == std::string::npos) {
            outcome = "no Properties dock size";
          } else {
            const size_t value = at + key.size();
            perspective.replace(value, perspective.find('|', value) - value, w[1]);
            p_mgr.LoadPerspective(wxString(perspective), false);
            updatePanes();
          }
        }
      } else if (w[0] == "minsizes" && w.size() >= 2) {
        //  Which control or sizer row sets a pane's minimum width.
        wxString nm = line.substr(line.find("minsizes") + 9);
        nm.Trim(false).Trim(true);
        wxAuiPaneInfo &pn = p_mgr.GetPane(nm);
        std::function<void(wxSizer*, int)> dump = [&](wxSizer *s, int depth) {
          for (wxSizerItem *it : s->GetChildren()) {
            wxString what = it->IsShown() ? "" : "(hidden) ";
            if (it->IsWindow()) {
              wxWindow *win = it->GetWindow();
              what += wxString(win->GetClassInfo()->GetClassName()) + " '" +
                     win->GetLabel().Left(30) + "'";
            } else if (it->IsSizer()) {
              what += wxString(it->GetSizer()->GetClassInfo()->GetClassName());
            } else {
              what += "spacer";
            }
            fprintf(stderr, "MINSIZE %*s%s min=%d border=%d\n", depth * 2, "",
                    (const char*)what.mb_str(), it->CalcMin().x,
                    it->GetBorder());
            if (it->IsSizer()) dump(it->GetSizer(), depth + 1);
            else if (it->IsWindow() && it->GetWindow()->GetSizer() &&
                     depth < 3)
              dump(it->GetWindow()->GetSizer(), depth + 1);
          }
        };
        if (pn.IsOk() && pn.window && pn.window->GetSizer()) {
          fprintf(stderr, "MINSIZE %s window_min=%d sizer_min=%d\n",
                  (const char*)nm.mb_str(), pn.window->GetMinSize().x,
                  pn.window->GetSizer()->GetMinSize().x);
          dump(pn.window->GetSizer(), 1);
        } else {
          outcome = "no such pane";
        }
      } else if (w[0] == "panesize" && w.size() >= 2) {
        wxString nm = line.substr(line.find("panesize") + 9);
        nm.Trim(false).Trim(true);
        wxAuiPaneInfo &pn = p_mgr.GetPane(nm);
        fprintf(stderr, "PANESIZE %s: %s y=%d height=%d min_height=%d "
                "resizable=%d width=%d client_width=%d virtual_width=%d "
                "pane_min_width=%d window_min_width=%d content_width=%d\n",
                (const char*)nm.mb_str(),
                pn.IsOk() && pn.IsShown() ? "shown" : "hidden", pn.rect.y,
                pn.rect.height, pn.min_size.y, pn.IsResizable() ? 1 : 0,
                pn.rect.width, pn.window ? pn.window->GetClientSize().x : 0,
                pn.window ? pn.window->GetVirtualSize().x : 0, pn.min_size.x,
                pn.window ? pn.window->GetMinSize().x : 0,
                pn.window && pn.window->GetSizer()
                    ? pn.window->GetSizer()->GetMinSize().x : 0);
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
        auto editableField = [](wxWindow *win) -> wxTextCtrl* {
          wxTextCtrl *t = dynamic_cast<wxTextCtrl*>(win);
          return t && t->IsShownOnScreen() && t->IsEnabled() &&
                 t->IsEditable() && !t->IsMultiLine() ? t : nullptr;
        };
        std::function<void(wxWindow*)> find = [&](wxWindow *p) {
          for (wxWindow *c : p->GetChildren()) {
            if (field) return;
            if (wxTextCtrl *t = editableField(c))
              field = t;
            else
              find(c);
          }
        };
        if (w[1] == "text") {
          wxWindow *focused = wxWindow::FindFocus();
          if (focused && IsDescendant(focused)) field = editableField(focused);
          if (!field) find(this);
          if (!field) {
            // The default layout has no editable text field. The Symmetry
            // panel provides one without applying an edit to the structure.
            const int tool = p_toolMenu->FindItem(NAME_TOOL_SYMMETRY);
            p_toolMenu->Check(tool, true);
            wxCommandEvent show(wxEVT_MENU, tool);
            show.SetInt(1);
            GetEventHandler()->ProcessEvent(show);
            for (int i = 0; i < 5; i++) wxTheApp->Yield(true);
            find(p_mgr.GetPane(NAME_TOOL_SYMMETRY).window);
          }
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
        if (w[1] != "text" || field) GetEventHandler()->ProcessEvent(ev);
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
      } else if (w[0] == "undo" && w.size() == 1) {
        wxCommandEvent ev(wxEVT_MENU, wxID_UNDO);
        ev.SetEventObject(this);
        GetEventHandler()->ProcessEvent(ev);
      } else if (w[0] == "redo" && w.size() == 1) {
        wxCommandEvent ev(wxEVT_MENU, wxID_REDO);
        ev.SetEventObject(this);
        GetEventHandler()->ProcessEvent(ev);
      } else if (w[0] == "menuenabled" && w.size() >= 2) {
        //  "menuenabled <Menu>|<item>": whether a click could reach the item
        //  (the "menu" command sends it even when it is greyed out).
        std::string rest = line.substr(line.find("menuenabled") + 12);
        size_t bar = rest.find('|');
        wxMenuBar *mb = GetMenuBar();
        int top = bar == std::string::npos ? wxNOT_FOUND
                : mb->FindMenu(wxString(rest.substr(0, bar)));
        int id = top == wxNOT_FOUND ? wxNOT_FOUND
               : mb->FindMenuItem(wxString(rest.substr(0, bar)),
                                  wxString(rest.substr(bar + 1)));
        if (id == wxNOT_FOUND) {
          outcome = "FAIL: no menu item " + rest;
        } else {
          fprintf(stderr, "BUILDER: menuenabled %s top=%d item=%d\n",
                  rest.c_str(), mb->IsEnabledTop(top) ? 1 : 0,
                  mb->IsEnabled(id) ? 1 : 0);
        }
      } else if (w[0] == "ctrlz" && w.size() == 1) {
        wxKeyEvent ev(wxEVT_CHAR_HOOK);
        ev.m_keyCode = 'Z';
        ev.SetControlDown(true);
        ev.SetEventObject(this);
        GetEventHandler()->ProcessEvent(ev);
      } else if (w[0] == "delkey" && w.size() == 1) {
        //  A Delete key press as GTK delivers it: the char hook of the
        //  frame, with the 3-D view focused, not the accelerator's menu id.
        if (SoWxRenderArea *area = findViewerArea(p_viewer)) area->SetFocus();
        for (int i = 0; i < 3; i++) wxTheApp->Yield(true);
        wxKeyEvent ev(wxEVT_CHAR_HOOK);
        ev.m_keyCode = WXK_DELETE;
        ev.SetEventObject(this);
        GetEventHandler()->ProcessEvent(ev);
      } else if (w[0] == "undolabel" && w.size() == 1) {
        wxMenuItem *item = GetMenuBar() ? GetMenuBar()->FindItem(wxID_UNDO) : 0;
        fprintf(stderr, "BUILDER: undolabel [%s] enabled=%d\n",
                item ? (const char*)item->GetItemLabelText().utf8_str() : "none",
                item && item->IsEnabled() ? 1 : 0);
      } else if (w[0] == "drawnstyles") {
        //  Atoms whose display style has a node in the scene graph; an atom
        //  with a style the graph no longer has exists but is not drawn.
        int drawn = 0;
        for (int i = 0; i < (int)frag->numAtoms(); i++) {
          const DisplayStyle &ds = frag->atomRef(i)->displayStyle();
          if (ds.isDisplayed() && getSG()->styleExists(ds.getName())) drawn++;
        }
        fprintf(stderr, "BUILDER: drawn %d of %d\n", drawn, (int)frag->numAtoms());
      } else if (w[0] == "focus" && w.size() == 1) {
        wxWindow *f = wxWindow::FindFocus();
        fprintf(stderr, "BUILDER: focus %s\n",
                f ? (const char*)wxString(f->GetClassInfo()->GetClassName()).utf8_str() : "none");
      } else if (w[0] == "resatoms") {
        std::map<std::string,int> counts;
        for (int i = 0; i < (int)frag->numAtoms(); i++) {
          const Residue *r = frag->atomRef(i)->getResidue();
          std::string n = r ? r->name() : std::string("(none)");
          //  PDB residue names are padded (" CL", "BNZ "); print them bare.
          n.erase(0, n.find_first_not_of(' '));
          n.erase(n.find_last_not_of(' ') + 1);
          counts[n]++;
        }
        std::string out;
        for (auto &kv : counts)
          out += (out.empty() ? "" : ",") + kv.first + "=" + std::to_string(kv.second);
        fprintf(stderr, "BUILDER: resatoms %s\n", out.c_str());
      } else if (w[0] == "import" && w.size() == 2) {
        SFile file(w[1]);
        wxString ext = wxString(file.extension()).Upper();
        if (!readFragmentFromFile(&file, ext, ext)) outcome = "import cancelled";
        getViewer().viewAll();
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
        fprintf(stderr, "BUILDER: cell_images=%d\n", getSG()->numCellImages());
        if (frag && frag->numAtoms() > 0) {
          double alo[3], ahi[3];
          for (size_t i = 0; i < frag->numAtoms(); i++) {
            const double *xyz = frag->atomRef(i)->coordinates();
            for (int k = 0; k < 3; k++) {
              if (i == 0 || xyz[k] < alo[k]) alo[k] = xyz[k];
              if (i == 0 || xyz[k] > ahi[k]) ahi[k] = xyz[k];
            }
          }
          fprintf(stderr, "BUILDER: atom_bounds=%.3f %.3f %.3f %.3f %.3f %.3f\n",
                  alo[0], alo[1], alo[2], ahi[0], ahi[1], ahi[2]);
        }
        if (lat) {
          MPoint cc = lat->getLatticeCorner();
          fprintf(stderr, "BUILDER: lattice_centered=%d corner=%.3f %.3f %.3f\n",
                  lat->getCentering() ? 1 : 0, cc.x(), cc.y(), cc.z());
          SGLattice *sgl = dynamic_cast<SGLattice*>(getSG()->findNode("lattice"));
          double lo[3], hi[3];
          if (sgl && sgl->drawnBounds(lo, hi))
            fprintf(stderr, "BUILDER: lattice_drawn=%.3f %.3f %.3f %.3f %.3f %.3f\n",
                    lo[0], lo[1], lo[2], hi[0], hi[1], hi[2]);
          vector<MPoint> *basis = lat->toVectors();
          for (MPoint& v : *basis)
            fprintf(stderr, "BUILDER: lattice_vector=%.3f %.3f %.3f\n", v.x(), v.y(), v.z());
          delete basis;
        }
        fprintf(stderr, "BUILDER: frame=%lld\n", ewxModalGuard::frameCounter(this));
        fprintf(stderr, "BUILDER: periodic_builder=%s\n",
                p_mgr.GetPane(NAME_TOOL_PBC).IsShown() ? "shown" : "hidden");
        //  The mode in effect, the mode the toolbar shows and whether the
        //  viewer picks (a click selects) must name the same thing.
        auto modeName = [](int id) {
          return id == ID_MODE_ATOM ? "atom" : id == ID_MODE_SELECT ? "select"
               : id == ID_MODE_BOND ? "bond" : id == ID_MODE_STRUCTLIB ? "structlib" : "other";
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
        //  The atom (or Nub) a click on it would select, numbered from 1 as
        //  everywhere the user sees an atom number; 0 or less clears.
        frag->m_atomHighLight.clear();
        if (atoi(w[1].c_str()) >= 1)
          frag->m_atomHighLight.assign(1, atoi(w[1].c_str()) - 1);
      } else if (w[0] == "select_within" && w.size() >= 2 && frag) {
        bool molecules = false, keep = true, valid = w.size() <= 4;
        for (size_t i = 2; i < w.size(); ++i) {
          if (w[i] == "molecules") molecules = true;
          else if (w[i] == "replace") keep = false;
          else valid = false;
        }
        double radius = 0.0;
        std::istringstream input(w[1]);
        input >> radius;
        if (!input || !input.eof() || !std::isfinite(radius) ||
            radius < 0.1 || radius > 50.0 || !valid || frag->m_atomHighLight.empty())
          outcome = "FAIL: select_within needs a selection and distance 0.1-50";
        else selectWithin(radius, molecules, keep);
      } else if (w[0] == "cutcluster" && w.size() >= 2 && frag) {
        double radius = 0.0;
        int whole = 0, cap = 0, center = 0;
        bool valid = true;
        std::istringstream input(w[1]);
        input >> radius;
        if (!input || !input.eof() || !std::isfinite(radius) ||
            radius < 0.5 || radius > 50.0) valid = false;
        for (size_t i = 2; valid && i < w.size(); i += 2) {
          if (i + 1 >= w.size()) { valid = false; break; }
          const std::string &key = w[i], &val = w[i + 1];
          if (key == "center") center = atoi(val.c_str());
          else if (key == "whole" && val == "none") whole = 0;
          else if (key == "whole" && val == "molecules") whole = 1;
          else if (key == "whole" && val == "units") whole = 2;
          else if (key == "cap" && val == "none") cap = 0;
          else if (key == "cap" && val == "H") cap = 1;
          else if (key == "cap" && val == "OH") cap = 2;
          else valid = false;
        }
        if (!valid)
          outcome = "FAIL: cutcluster R(0.5-50) [center N] [whole none|molecules|units] [cap none|H|OH]";
        else {
          if (center >= 1) frag->m_atomHighLight.assign(1, center - 1);
          std::string message;
          if (!cutCluster(radius, whole, cap, message))
            outcome = "FAIL: cutcluster: " + message;
          else {
            SGFragment *now = getSG()->getFragment();
            for (size_t i = 0; i < now->numAtoms(); ++i) {
              const double *c = now->atomRef(i)->coordinates();
              fprintf(stderr, "BUILDER: clusteratom %s %.6f %.6f %.6f\n",
                      now->atomRef(i)->atomicSymbol().c_str(), c[0], c[1], c[2]);
            }
          }
        }
      } else if (w[0] == "solvate" && frag) {
        //  solvate [solvent NAME] [margin A] [scale S]: Build > Solvate.
        std::string solvent = "water216";
        double margin = 10.0, scale = 1.0;
        bool valid = (w.size() % 2) == 1;
        for (size_t i = 1; valid && i + 1 < w.size(); i += 2) {
          if (w[i] == "solvent") solvent = w[i + 1];
          else if (w[i] == "margin") margin = atof(w[i + 1].c_str());
          else if (w[i] == "scale") scale = atof(w[i + 1].c_str());
          else valid = false;
        }
        if (!valid || !(margin >= 1.0 && margin <= 50.0) || !(scale >= 0.3 && scale <= 2.0))
          outcome = "FAIL: solvate [solvent NAME] [margin 1-50] [scale 0.3-2]";
        else {
          std::string message;
          if (!solvateStructure(solvent, margin, scale, message))
            outcome = "FAIL: solvate: " + message;
        }
      } else if ((w[0] == "select_protein" || w[0] == "select_ligand") &&
                 w.size() == 1 && frag) {
        if (!selectResidueClass(w[0] == "select_ligand"))
          outcome = "FAIL: nothing selected";
      } else if (w[0] == "ligands" && w.size() == 1) {
        //  The Select Ligand submenu as Edit would show it.
        fillLigandMenu();
        std::string out;
        for (size_t k = 0; k < p_ligandMenu->GetMenuItemCount(); k++)
          out += (k ? "; " : "") + p_ligandMenu->FindItemByPosition(k)->GetItemLabelText().ToStdString();
        fprintf(stderr, "BUILDER: ligands %s\n", out.c_str());
      } else if (w[0] == "ligand" && w.size() == 2 && frag) {
        //  Select Ligand > Nth entry (from 1).
        fillLigandMenu();
        if (!selectLigandCandidate(atoi(w[1].c_str()) - 1))
          outcome = "FAIL: no such ligand candidate";
        else
          fprintf(stderr, "BUILDER: selection atoms=%d bonds=%d\n",
                  (int)frag->m_atomHighLight.size(),
                  (int)frag->m_bondHighLight.size());
      } else if (w[0] == "select_hetero" && w.size() == 1 && frag) {
        if (!selectHeteroatoms()) outcome = "FAIL: nothing selected";
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
      } else if (w[0] == "toolbar" && w.size() == 2 && w[1] == "bond") {
        //  What a click on the Bond button does: toggle, then the tool event.
        p_modeToolbar->ToggleTool(ID_MODE_BOND, true);
        wxCommandEvent toolEv(wxEVT_TOOL, ID_MODE_BOND);
        toolEv.SetInt(1);
        GetEventHandler()->ProcessEvent(toolEv);
      } else if (w[0] == "bondkind" && w.size() == 2) {
        //  The kind of bond Bond mode makes, sent as the Bond tool sends it:
        //  covalent or metal (a bond order is not settable).
        ecce::EventDispatcher::getDispatcher().publish(
            ecce::Event("BondChanged", w[1] == "metal" ? "Metal-Pi"
                                                      : "Single"));
        fprintf(stderr, "BUILDER: bondkind current=%g\n", p_currentBond);
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
      } else if (w[0] == "menu" && w.size() >= 2) {
        //  "menu <Menu>|<item>": the item chosen, as a click on it sends it
        //  (Edit|Select Solvent, Render|Wireframe from the Style submenu).
        std::string rest = line.substr(line.find("menu") + 5);
        size_t bar = rest.find('|');
        int id = bar == std::string::npos ? wxNOT_FOUND
               : GetMenuBar()->FindMenuItem(wxString(rest.substr(0, bar)),
                                            wxString(rest.substr(bar + 1)));
        if (id == wxNOT_FOUND) {
          outcome = "FAIL: no menu item " + rest;
        } else {
          wxCommandEvent ev(wxEVT_MENU, id);
          ev.SetEventObject(this);
          GetEventHandler()->ProcessEvent(ev);
        }
      } else if (w[0] == "hbonds" && frag) {
        //  "hbonds [DIST ANGLE]": switch Show Hydrogen Bonds on (with those
        //  criteria), redraw, log the number of bonds drawn.
        if (w.size() >= 3)
          HydrogenBondsCmd::setCriteria(atof(w[1].c_str()), atof(w[2].c_str()));
        HydrogenBondsCmd::setHalideAcceptors(!(w.size() == 4 && w[3] == "noions"));
        const wxLongLong t0 = wxGetLocalTimeMillis();
        GetMenuBar()->Check(ViewerEvtHandler::ID_SHOW_HYDROGEN_BOND, true);
        Command *hb = new HydrogenBondsCmd("Show Hydrogen Bond", &getFW().getSceneGraph());
        hb->getParameter("switch")->setBoolean(true);
        const wxLongLong t1 = wxGetLocalTimeMillis();
        getFW().execute(hb);
        const wxLongLong t2 = wxGetLocalTimeMillis();
        fprintf(stderr, "BUILDER: hbonds_split menu_ms=%lld execute_ms=%lld\n",
                (long long)(t1 - t0).GetValue(), (long long)(t2 - t1).GetValue());
        fprintf(stderr, "BUILDER: hbonds=%d ms=%lld atoms=%d search_ms=%.0f draw_ms=%.0f\n",
                HydrogenBondsCmd::lastCount(),
                (long long)(wxGetLocalTimeMillis() - t0).GetValue(),
                (int)frag->numAtoms(), HydrogenBondsCmd::lastSearchMs(),
                HydrogenBondsCmd::lastDrawMs());
      } else if (w[0] == "atomstyles" && frag) {
        //  Each run of atoms with one style: "first-last name:style".
        std::string out;
        int n = frag->numAtoms(), start = 0;
        auto key = [frag](int i) {
          DisplayStyle ds = frag->atomRef(i)->displayStyle();
          return ds.getName() + ":" + DisplayStyle::fromStyle(ds.getStyle());
        };
        for (int i = 1; i <= n; i++)
          if (i == n || key(i) != key(start)) {
            out += " " + std::to_string(start) + "-" + std::to_string(i - 1) +
                   " " + key(start);
            start = i;
          }
        fprintf(stderr, "BUILDER: atomstyles%s\n", out.c_str());
      } else if (w[0] == "selection" && frag) {
        fprintf(stderr, "BUILDER: selection atoms=%d bonds=%d\n",
                (int)frag->m_atomHighLight.size(),
                (int)frag->m_bondHighLight.size());
      } else if (w[0] == "bondorders" && frag) {
        //  Each bond between real atoms: its stored order and the type
        //  the display draws it as.
        vector<TBond*> *all = frag->bonds();
        for (size_t i = 0; i < all->size(); i++) {
          TBond *b = (*all)[i];
          if (b->atom1()->atomicSymbol() == "Nub" ||
              b->atom2()->atomicSymbol() == "Nub") continue;
          const double *p = b->atom1()->coordinates();
          const double *q = b->atom2()->coordinates();
          fprintf(stderr, "BUILDER: bond %d-%d %s-%s order=%g drawn=%d "
                  "len=%.3f\n",
                  b->atom1()->index(), b->atom2()->index(),
                  b->atom1()->atomicSymbol().c_str(),
                  b->atom2()->atomicSymbol().c_str(), b->order(),
                  frag->getBondType((int32_t)i),
                  sqrt((p[0]-q[0])*(p[0]-q[0]) + (p[1]-q[1])*(p[1]-q[1]) +
                       (p[2]-q[2])*(p[2]-q[2])));
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
      } else if (w[0] == "saveimage" && (w.size() == 3 || w.size() == 4)) {
        //  "saveimage <path.png> <factor> [white]": as in the scene script.
        wxSize sz = p_viewer->GetSize();
        if (w.size() == 4 && w[3] != "white")
          outcome = "FAIL: saveimage: unknown option " + w[3];
        else if (!imageOnFineSurface(w[1], sz.GetWidth(), sz.GetHeight(),
                                     w.size() == 4, atoi(w[2].c_str())))
          outcome = "FAIL: saveimage: " + VizRender::msg();
      } else if (w[0] == "saveimagemenu" && w.size() == 4) {
        //  "saveimagemenu <factor> <white|keep> <path>": File > Save Image...
        //  with its three dialogs answered.
        setenv("ECCE_SAVE_IMAGE_ANSWERS", (w[1] + " " + w[2] + " " + w[3]).c_str(), 1);
        wxCommandEvent ev(wxEVT_MENU, ID_SAVE_IMAGE);
        OnSaveImageClick(ev);
        unsetenv("ECCE_SAVE_IMAGE_ANSWERS");
      } else if (w[0] == "modal" && w.size() == 2 && w[1] == "ok") {
        //  Runs from inside the dialog's own modal loop (timers still fire).
        wxDialog *open = 0;
        for (wxWindow *t : wxTopLevelWindows) {
          wxDialog *dlg = dynamic_cast<wxDialog*>(t);
          if (dlg && dlg->IsModal()) open = dlg;
        }
        if (!open) {
          outcome = "FAIL: no modal dialog";
        } else {
          outcome = "ok \"" + std::string(open->GetTitle().ToUTF8()) + "\"";
          open->EndModal(wxID_OK);
        }
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
      } else if (w[0] == "measure" && w.size() >= 4 && frag) {
        //  "measure dist|angle|torsion I J [K [L]]": the Measure toolbar's
        //  button on those atoms (1-based).
        frag->m_atomHighLight.clear();
        for (size_t k = 2; k < w.size(); k++)
          frag->m_atomHighLight.push_back(atoi(w[k].c_str()) - 1);
        wxCommandEvent ev(wxEVT_MENU,
            w[1] == "dist" ? ViewerEvtHandler::ID_MEASURE_DIST
          : w[1] == "angle" ? ViewerEvtHandler::ID_MEASURE_ANGLE
                            : ViewerEvtHandler::ID_MEASURE_TORSION);
        GetEventHandler()->ProcessEvent(ev);
        wxTheApp->Yield(true);
      } else if (w[0] == "labelclick" && w.size() == 2 && getSG()) {
        //  "labelclick dist|angle|torsion": a left click in the middle of the
        //  first such measure's number.  The popup logs instead of opening.
        SoWxRenderArea *area = findViewerArea(p_viewer);
        SbVec3f lp;
        int len = 0;
        SoGroup *top = getSG()->getTopLabel();
        for (int i = 0; top && i < top->getNumChildren() && !len; i++) {
          SoNode *m = ((SoSeparator*)top->getChild(i))->getChild(0);
          if (w[1] == "dist" && m->isOfType(AtomMeasureDist::getClassTypeId())) {
            lp = ((AtomMeasureDist*)m)->getLabelPosition();
            len = ((AtomMeasureDist*)m)->getLabelLength();
          } else if (w[1] == "angle" &&
                     m->isOfType(AtomMeasureAngle::getClassTypeId())) {
            lp = ((AtomMeasureAngle*)m)->getLabelPosition();
            len = ((AtomMeasureAngle*)m)->getLabelLength();
          } else if (w[1] == "torsion" &&
                     m->isOfType(AtomMeasureTorsion::getClassTypeId())) {
            lp = ((AtomMeasureTorsion*)m)->getLabelPosition();
            len = ((AtomMeasureTorsion*)m)->getLabelLength();
          }
        }
        if (!area || !len) {
          outcome = "FAIL: no drawn " + w[1] + " label";
        } else {
          SbVec3f d;
          const SbViewportRegion& vpr = p_viewer->getViewportRegion();
          SbViewVolume vv =
              p_viewer->getCamera()->getViewVolume(vpr.getViewportAspectRatio());
          vv.projectToScreen(lp, d);
          SbVec2s sz = vpr.getViewportSizePixels();
          const double sc = area->GetContentScaleFactor();
          const int lx = (int)std::lround((d[0] * sz[0] + 4.5 * len) / sc);
          const int ly = (int)std::lround((sz[1] - 1 - d[1] * sz[1] - 7) / sc);
          static long stamp = 200000;
          for (int k = 0; k < 2; k++) {
            wxMouseEvent e(k == 0 ? wxEVT_LEFT_DOWN : wxEVT_LEFT_UP);
            e.m_x = lx;
            e.m_y = ly;
            e.m_leftDown = (k == 0);
            e.SetEventObject(area);
            e.SetTimestamp(stamp += 50);
            area->GetEventHandler()->ProcessEvent(e);
          }
          wxTheApp->Yield(true);
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
      } else if (w[0] == "dna" && w.size() == 2 && frag) {
        //  dna SEQUENCE: Dna panel's Generate, for the sample structures (#971).
        std::string seq = w[1];
        GenerateDNACmd *cmd = new GenerateDNACmd("Generate DNA", getSG());
        cmd->getParameter("sequence")->setString(seq);
        try { execute(cmd); } catch (const std::exception& e) {
          fprintf(stderr, "BUILDER: dna refused: %s\n", e.what());
        }
        fprintf(stderr, "BUILDER: dna: %d atoms\n", (int)frag->numAtoms());
      } else if ((w[0] == "savegro" || w[0] == "savepdb") && w.size() == 2) {
        //  savegro|savepdb PATH: File > Export Structure... to that file.
        const std::string err = exportStructure(w[1]);
        fprintf(stderr, "BUILDER: %s: %s\n", w[0].c_str(), err.empty() ? "done" : err.c_str());
      } else if (w[0] == "savexyz" && w.size() == 2) {
        std::vector<int> none;
        const std::string err = writeStructureXyz(w[1], none);
        fprintf(stderr, "BUILDER: savexyz: %s\n", err.empty() ? "done" : err.c_str());
      } else if (w[0] == "xrd") {
        fprintf(stderr, "BUILDER: xrd: %s\n", diffractionScript(w).ToUTF8().data());
      } else if (w[0] == "diffraction" && w.size() >= 3) {
        //  diffraction OUT.png OUT.csv [XYZ] [view=NAME]: Tools > Diffraction...
        //  with the default fields; the structure it was given is kept as XYZ
        //  (#971).  view=fibre|powder|single|frame|debye picks the view instead
        //  of the default for the structure (#1230).
        std::vector<std::string> dw = w;
        const int view = takeViewWord(dw);
        std::vector<int> sel;
        const std::string xyz = dw.size() > 3 ? dw[3] : dw[1] + ".xyz";
        std::string err = view == -2 ? "unknown view" : writeStructureXyz(xyz, sel);
        std::string why;
        DiffractionDialog *dd = 0;
        if (err.empty()) {
          dd = new DiffractionDialog(this, xyz, sel);
          if (view >= 0 && !dd->setPattern(view, why)) err = why;
          else if (!dd->calculate(why)) err = why;
          else if (!dd->savePng(dw[1], why) || !dd->saveCsv(dw[2], why)) err = why;
        }
        fprintf(stderr, "BUILDER: diffraction: %s\n", err.empty() ? "done" : ("refused: " + err).c_str());
        if (dd) dd->Destroy();
      } else if (w[0] == "debye" && w.size() >= 4) {
        std::vector<int> sel;
        std::string err = writeStructureXyz(w[3], sel), why;
        DiffractionDialog *dd = 0;
        if (err.empty()) {
          dd = new DiffractionDialog(this, w[3], sel);
          // Compute the initial view first so switching must clear old reflections.
          if (!dd->calculate(why)) err = why;
          else {
            dd->setDebye();
            dd->setSource(w.size() > 4 ? atoi(w[4].c_str()) : 0,
                          w.size() > 5 ? atoi(w[5].c_str()) : -1);
            if (!dd->calculate(why)) err = why;
            else if (!dd->savePng(w[1], why) || !dd->saveCsv(w[2], why)) err = why;
            if (err.empty()) {
              fprintf(stderr, "BUILDER: debye state: debye=%d powder=%d reflection=%s\n",
                      dd->debye(), dd->powder(), dd->reflectionText(31.7, 180).ToUTF8().data());
              fprintf(stderr, "BUILDER: debye cursor: %s\n", dd->cursorText(31.7, 1).ToUTF8().data());
            }
          }
        }
        fprintf(stderr, "BUILDER: debye: %s\n", err.empty() ? "done" : ("refused: " + err).c_str());
        if (dd) dd->Destroy();
      } else if (w[0] == "calcdiffraction" && w.size() >= 3) {
        //  calcdiffraction OUT.png OUT.csv [XYZ [EXPOSURE SCHEME]] [view=NAME]: the
        //  Diffraction entry of the open calculation's results, default fields (#587).
        //  view=fibre|powder|single|frame|debye picks the view instead of the
        //  default for the structure (#1230).
        std::vector<std::string> dw = w;
        const int view = takeViewWord(dw);
        std::string err = view == -2 ? "unknown view" : std::string(), why;
        DiffractionDialog *dd = err.empty() && calcHasStructure() ?
            openResultsDiffraction(dw.size() > 3 ? dw[3] : dw[1] + ".xyz") : 0;
        if (!err.empty()) {}
        else if (!dd) err = "no Diffraction entry for this calculation";
        else if (view >= 0 && !dd->setPattern(view, why)) err = why;
        else if (dw.size() > 5 && (dd->setDisplay(atoi(dw[4].c_str()), atoi(dw[5].c_str())), false)) {}
        else if (!dd->calculate(why)) err = why;
        else if (!dd->savePng(dw[1], why) || !dd->saveCsv(dw[2], why)) err = why;
        if (dd && err.empty() && dw.size() > 6 && dw[6] == "viewtest") {
          if (!dd->testImageView(dw[1], why)) err = why;
          fprintf(stderr, "BUILDER: image view: %s\n", err.empty() ? "PASS" : err.c_str());
        }
        if (dd && err.empty())
          fprintf(stderr, "BUILDER: calcdiffraction pattern: %s\n",
                  // Every view has to be named here: powder() is false in
                  // Debye mode, so without it a Debye view reported "fibre".
                  dd->singleCrystal() ? "single" :
                  dd->detectorFrame() ? "frame" :
                  dd->debye() ? "debye" :
                  dd->powder() ? "powder" : "fibre");
        fprintf(stderr, "BUILDER: calcdiffraction: %s\n", err.empty() ? "done" : ("refused: " + err).c_str());
        if (dd) dd->Destroy();
      } else if (w[0] == "calcpowder" && w.size() >= 5) {
        //  calcpowder OUT.png OUT.csv XYZ SOURCE [DOUBLET [MEASURED]]: as calcdiffraction
        //  with the powder source (0 Cu, 1 Mo, 2 Co, 3 other) and the Ka1/Ka2
        //  box set; reports the box's state (#587).
        std::string err, why;
        DiffractionDialog *dd = calcHasStructure() ? openResultsDiffraction(w[3]) : 0;
        if (!dd) err = "no Diffraction entry for this calculation";
        else {
          fprintf(stderr, "BUILDER: calcpowder state: %s\n",
                  dd->setSource(atoi(w[4].c_str()), w.size() > 5 ? atoi(w[5].c_str()) : -1).ToUTF8().data());
          if (w.size() > 6 && !dd->setMeasured(w[6], why)) err = why;
          else if (!dd->calculate(why)) err = why;
          else if (!dd->savePng(w[1], why) || !dd->saveCsv(w[2], why)) err = why;
        }
        fprintf(stderr, "BUILDER: calcpowder: %s\n", err.empty() ? "done" : ("refused: " + err).c_str());
        if (dd) dd->Destroy();
      } else if (w[0] == "calclayer" && w.size() >= 3) {
        //  calclayer OUT.png OUT.csv [ZONE [LAYER [DMIN [h,k,l]]]]: the
        //  Diffraction entry set to Single crystal, with absent spots shown;
        //  the spot h,k,l is reported as hovering would (#972).
        std::string err, why;
        DiffractionDialog *dd = calcIsPeriodic() ? openResultsDiffraction(w[1] + ".xyz") : 0;
        if (!dd) err = "no Diffraction entry for this calculation";
        else if (!dd->setLayer(w.size() > 3 ? w[3] : "l", w.size() > 4 ? atoi(w[4].c_str()) : 0,
                               w.size() > 5 ? atof(w[5].c_str()) : 1.0, true, why)) err = why;
        else if (!dd->calculate(why)) err = why;
        else if (!dd->savePng(w[1], why) || !dd->saveCsv(w[2], why)) err = why;
        int h = 0, k = 0, l = 0;
        if (err.empty() && w.size() > 6 && sscanf(w[6].c_str(), "%d,%d,%d", &h, &k, &l) == 3)
          fprintf(stderr, "BUILDER: calclayer spot: %s\n", dd->spotText(h, k, l).ToUTF8().data());
        fprintf(stderr, "BUILDER: calclayer: %s\n", err.empty() ? "done" : ("refused: " + err).c_str());
        if (dd) dd->Destroy();
      } else if (w[0] == "calcframe" && w.size() >= 3) {
        //  calcframe OUT.png OUT.csv [PHI [OSC [labels [rings]]]]: the
        //  Diffraction entry set to Detector frame (#985).
        std::string err, why;
        DiffractionDialog *dd = calcIsPeriodic() ? openResultsDiffraction(w[1] + ".xyz") : 0;
        if (!dd) err = "no Diffraction entry for this calculation";
        else if (!dd->setFrame(w.size() > 3 ? atof(w[3].c_str()) : 0.0,
                               w.size() > 4 ? atof(w[4].c_str()) : 10.0,
                               w.size() > 5 && w[5] == "1", w.size() > 6 && w[6] == "1", why)) err = why;
        else if (!dd->calculate(why)) err = why;
        else if (!dd->savePng(w[1], why) || !dd->saveCsv(w[2], why)) err = why;
        fprintf(stderr, "BUILDER: calcframe: %s\n", err.empty() ? "done" : ("refused: " + err).c_str());
        if (dd) dd->Destroy();
      } else if (w[0] == "calcmeasure" && w.size() >= 6) {
        //  calcmeasure KIND OUT.png OUT.csv EXPOSURE SCHEME [h,k,l h,k,l | strongest | 2THETA]:
        //  the Diffraction entry set to KIND (layerN = hkN, frame = phi 0 to 30,
        //  powder), drawn at the exposure (tenths of a stop) and scheme (0 Film,
        //  1 Dark), then the readout of two picked spots or of a powder
        //  cursor position (#985).
        std::string err, why;
        const std::string kind = w[1];
        DiffractionDialog *dd = calcIsPeriodic() ? openResultsDiffraction(w[2] + ".xyz") : 0;
        if (!dd) err = "no Diffraction entry for this calculation";
        else if (kind.compare(0, 5, "layer") == 0 &&
                 !dd->setLayer("l", atoi(kind.c_str() + 5), 1.0, false, why)) err = why;
        else if (kind == "frame" && !dd->setFrame(0.0, w.size() > 6 && w[6] == "viewtest" ? 10.0 : 30.0,
                     w.size() > 6 && w[6] == "viewtest", w.size() > 6 && w[6] == "viewtest", why)) err = why;
        else if (kind.compare(0, 5, "layer") != 0 && kind != "frame" && kind != "powder") err = "kind is layer, frame or powder";
        if (err.empty()) {
          dd->setDisplay(atoi(w[4].c_str()), atoi(w[5].c_str()));
          if (!dd->calculate(why)) err = why;
        }
        if (err.empty()) {
          dd->setDisplay(atoi(w[4].c_str()), atoi(w[5].c_str()));
          if (!dd->savePng(w[2], why) || !dd->saveCsv(w[3], why)) err = why;
        }
        if (dd && err.empty() && w.size() > 6 && w[6] == "viewtest") {
          if (!dd->testImageView(w[2], why)) err = why;
          fprintf(stderr, "BUILDER: image view: %s\n", err.empty() ? "PASS" : err.c_str());
        }
        int h1 = 0, k1 = 0, l1 = 0, h2 = 0, k2 = 0, l2 = 0;
        if (err.empty() && kind == "powder" && w.size() > 6)
          fprintf(stderr, "BUILDER: calcmeasure readout: %s\n",
                  dd->cursorText(atof(w[6].c_str()), 0.2).ToUTF8().data());
        else if (err.empty() && w.size() > 6 && w[6] == "strongest")
          fprintf(stderr, "BUILDER: calcmeasure readout: %s\n",
                  dd->measureStrongest().ToUTF8().data());
        else if (err.empty() && w.size() > 7 && sscanf(w[6].c_str(), "%d,%d,%d", &h1, &k1, &l1) == 3 &&
                 sscanf(w[7].c_str(), "%d,%d,%d", &h2, &k2, &l2) == 3)
          fprintf(stderr, "BUILDER: calcmeasure readout: %s\n",
                  dd->measure(h1, k1, l1, h2, k2, l2).ToUTF8().data());
        if (dd) fprintf(stderr, "BUILDER: calcmeasure size: %s\n", dd->sizeText().ToUTF8().data());
        fprintf(stderr, "BUILDER: calcmeasure: %s\n", err.empty() ? "done" : ("refused: " + err).c_str());
        if (dd) dd->Destroy();
      } else if (w[0] == "takefrom" && w.size() >= 3) {
        //  takefrom PATH.xyz SET|selected [align]: Take Coordinates From...
        //  with the other structure read from an .xyz file (#620).
        std::ifstream in(w[1]);
        int n = 0;
        std::string title, el, xyz;
        in >> n;
        std::getline(in, title);
        std::getline(in, title);
        for (int i = 0; i < n && in; i++) {
          std::string s;
          double x, y, z;
          if (in >> s >> x >> y >> z) {
            char buf[128];
            snprintf(buf, sizeof buf, "%.17g %.17g %.17g ", x, y, z);
            el += s + " ";
            xyz += buf;
          }
        }
        double rmsd = 0;
        bool ok = takeCoordinates(w[2] == "selected" ? "" : w[2], el, xyz,
                                  w.size() > 3 && w[3] == "align", &rmsd);
        fprintf(stderr, "BUILDER: takefrom %s rmsd=%.6g\n", ok ? "ok" : "refused", rmsd);
      } else if (w[0] == "distortdlg" && w.size() >= 2) {
        //  distortdlg open|fill <dr>|apply|state|close: the modeless dialog.
        if (w[1] == "open") {
          wxCommandEvent ev(wxEVT_MENU, ID_DISTORT_BONDS);
          OnDistortBondsClick(ev);
        } else if (p_distortDlg && w[1] == "fill" && w.size() == 3) {
          p_distortDlg->fillAll(w[2]);
        } else if (p_distortDlg && w[1] == "apply") {
          wxCommandEvent ev(wxEVT_BUTTON, wxID_APPLY);
          ev.SetEventObject(p_distortDlg);
          p_distortDlg->GetEventHandler()->ProcessEvent(ev);
        } else if (p_distortDlg && w[1] == "close") {
          p_distortDlg->Close();
          wxTheApp->Yield(true);
        }
        refreshDistortDialog();
        fprintf(stderr, "BUILDER: distortdlg open=%d apply=%d note=[%s]\n",
                p_distortDlg ? 1 : 0,
                p_distortDlg && p_distortDlg->applyEnabled() ? 1 : 0,
                p_distortDlg ? p_distortDlg->note().c_str() : "");
      } else if (w[0] == "coordsraw" && frag) {
        for (size_t i = 0; i < frag->numAtoms(); i++) {
          const double *c = frag->atomRef(i)->coordinates();
          fprintf(stderr, "BUILDER: rawatom %d %.17g %.17g %.17g\n",
                  (int)i + 1, c[0], c[1], c[2]);
        }
      } else if (w[0] == "atomtable") {
        //  atomtable: the text of every cell of the Atom Table, as shown.
        std::function<ewxGrid*(wxWindow*)> findGrid =
            [&findGrid](wxWindow *win) -> ewxGrid* {
          if (ewxGrid *gr = dynamic_cast<ewxGrid*>(win)) return gr;
          for (wxWindow *child : win->GetChildren())
            if (ewxGrid *gr = findGrid(child)) return gr;
          return 0;
        };
        wxWindow *win = p_mgr.GetPane(NAME_TOOL_ATOM_TABLE).window;
        ewxGrid *g = win ? findGrid(win) : 0;
        if (!g) {
          outcome = "no table";
        } else {
          for (int r = 0; r < g->GetNumberRows(); r++) {
            wxString row;
            for (int c = 0; c < g->GetNumberCols(); c++)
              row += g->GetCellValue(r, c) + "|";
            fprintf(stderr, "BUILDER: tablerow %d %s\n", r,
                    row.ToUTF8().data());
          }
        }
      } else if (w[0] == "constraints" && w.size() >= 2) {
        //  Geometry Constraints (#306): "show", "select I J ..", "press
        //  constrain|restrain|fix yes|no", "scan N STEP [yes]", "list", "stored".
        if (!p_constraintsDialog) showConstraints();
        GeomConstraintsDialog *dlg = p_constraintsDialog;
        if (w[1] == "show") {
          showConstraints();
          fprintf(stderr, "BUILDER: constraints shown=%d\n", dlg->IsShown() ? 1 : 0);
        } else if (w[1] == "select" && frag) {
          frag->m_atomHighLight.clear();
          for (size_t i = 2; i < w.size(); i++)
            frag->m_atomHighLight.push_back(atoi(w[i].c_str()) - 1);
          EventDispatcher::getDispatcher().publish(Event("SelectionChanged"));
          fprintf(stderr, "BUILDER: constraints pick: %s\n", dlg->selectionText().c_str());
        } else if (w[1] == "press" && w.size() == 4) {
          fprintf(stderr, "BUILDER: constraints %s: %s\n", w[2].c_str(),
                  dlg->press(w[2], w[3] == "yes").c_str());
        } else if (w[1] == "scan" && (w.size() == 4 || (w.size() == 5 && w[4] == "yes"))) {
          //  "scan <steps> <step> [yes]": Scan... on the selection (Å or
          //  degrees); "yes" answers the unbonded-atoms question.
          std::string msg = dlg->scanSelection(atoi(w[2].c_str()), atof(w[3].c_str()),
                                               w.size() == 5);
          fprintf(stderr, "BUILDER: constraints scan: %s\n", msg.c_str());
          if (msg.compare(0, 5, "Added") != 0) outcome = "FAIL: " + msg;
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
      } else if ((w[0] == "smiles" || w[0] == "replacegroup") && w.size() >= 2) {
        //  "smiles <SMILES> [extended]": Tools > Add from SMILES... (#833).
        //  "replacegroup <SMILES>": Tools > Replace Group... on the
        //  selection (a bond, or the group's first atom).
        std::string err = w[0] == "smiles"
            ? addFromSmiles(w[1], w.size() > 2 && w[2] == "extended")
            : replaceGroupFromSmiles(w[1]);
        fprintf(stderr, "BUILDER: %s: %s charge=%g\n", w[0].c_str(),
                err.empty() ? "done" : ("refused: " + err).c_str(),
                frag ? frag->getTotalCharge() : 0.0);
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
      } else if (w[0] == "camerastate") {
        SoCamera *cam = p_viewer->getCamera();
        const SbVec3f p = cam->position.getValue();
        float h = 0.f;
        if (SoPerspectiveCamera *pc = dynamic_cast<SoPerspectiveCamera*>(cam))
          h = pc->heightAngle.getValue();
        else if (SoOrthographicCamera *oc = dynamic_cast<SoOrthographicCamera*>(cam))
          h = oc->height.getValue();
        fprintf(stderr, "BUILDER: camerastate %.5g %.5g %.5g focal=%.5g "
                "height=%.5g\n", p[0], p[1], p[2],
                cam->focalDistance.getValue(), h);
      } else if (w[0] == "emptyclick") {
        //  What a click on empty space in Select mode runs.
        execute(new UnselectCmd("UnselectCmd", getSG()));
      } else if (w[0] == "pickatom" && w.size() == 2 && frag) {
        //  What a click on atom N (from 1) in the current mode runs.
        frag->m_atomHighLight.assign(1, atoi(w[1].c_str()) - 1);
        pickedInMode(false, -1);
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
      } else if (w[0] == "mostate") {
        //  mostate: the MOs panel's note and the table it lists now (header
        //  and one "row" line per orbital), as the panel shows them.
        MoPanel *mp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((mp = dynamic_cast<MoPanel*>(p)) != 0) break;
        if (!mp) {
          outcome = "no MOs panel";
        } else {
          wxStaticText *note = dynamic_cast<wxStaticText*>(
              wxWindow::FindWindowByName("orbitalPointNote", mp));
          fprintf(stderr, "BUILDER: monote %s\nBUILDER: mostate\n%s",
                  note ? note->GetLabel().utf8_str().data() : "",
                  mp->tableState().c_str());
        }
      } else if (w[0] == "spectrumcsv" && w.size() >= 2) {
        //  spectrumcsv <path> [ir|raman]: the Vibrational Frequencies
        //  panel's Export .csv (#392) without the file dialog.
        NModePanel *nm = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
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
      } else if (w[0] == "specclick" && w.size() == 2) {
        //  specclick <mode, 1-based>: a left click on that mode's stick in
        //  the Vibrational Frequencies graph (#445), and how much of the
        //  canvas on screen differs from the layout the click is tested on.
        NModePanel *nm = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((nm = dynamic_cast<NModePanel*>(p)) != 0) break;
        double mismatch = 1.0;
        int selected = -1;
        const int mode = atoi(w[1].c_str());
        if (!nm) {
          outcome = "no Vibrational Frequencies panel";
        } else if (!nm->scriptSpectrumClick(mode - 1, &mismatch, &selected)) {
          outcome = "no graph or no stick for that mode";
        } else {
          fprintf(stderr, "BUILDER: specclick mode %d: screen mismatch "
                  "%.4f, selected mode %d\n", mode, mismatch, selected + 1);
        }
      } else if (w[0] == "plist" && w.size() == 1) {
        //  One "BUILDER: entry GROUP/NAME" line per row of the Properties list.
        if (p_index)
          for (const auto& e : p_index->listing())
            fprintf(stderr, "BUILDER: entry %s\n", e.c_str());
      } else if (w[0] == "vibnote" && w.size() == 1) {
        //  vibnote: what the Vibrational Frequencies panel's note line shows (#1042).
        NModePanel *nm = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((nm = dynamic_cast<NModePanel*>(p)) != 0) break;
        if (!nm) outcome = "no Vibrational Frequencies panel";
        else {
          //  Wrap() put line breaks into the label; report it on one line.
          wxString t = nm->softNoteText();
          t.Replace("\n", " ");
          fprintf(stderr, "BUILDER: vibnote shown=%d text=%s\n",
                  nm->softNoteShown() ? 1 : 0, (const char*)t.utf8_str());
        }
      } else if (w[0] == "specfactor" && w.size() == 2) {
        //  specfactor <text>: type text into the Freq. factor field (#604).
        NModePanel *nm = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((nm = dynamic_cast<NModePanel*>(p)) != 0) break;
        if (!nm) outcome = "no Vibrational Frequencies panel";
        else fprintf(stderr, "BUILDER: specfactor %s\n",
                     (const char*)nm->scriptSpectrumFactor(
                         wxString::FromUTF8(w[1].c_str())).utf8_str());
      } else if (w[0] == "scancsv" && w.size() >= 2) {
        //  scancsv <path> [kcal]: the Relaxed Scan panel's Export .csv
        //  (#393) without the file dialog.
        ScanPanel *sp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
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
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((pp = dynamic_cast<PullPanel*>(p)) != 0) break;
        if (!pp) {
          outcome = "no Pull Force panel";
        } else {
          fprintf(stderr, "BUILDER: pullcsv points=%d %s\n", pp->points(),
                  pp->exportCsv(wxString::FromUTF8(w[1].c_str())) ? "ok" : "failed");
        }
      } else if (w[0].compare(0, 8, "umbrella") == 0) {
        //  umbrellafind <spacing> [from to]; umbrellacreate <eq> <prod>;
        //  umbrellawham [jacobian] [noise=<kJ/mol>]; umbrellaexport <path>; umbrellashot
        //  <png> [w h]; umbrellastate; umbrelladetails [close]; umbrellaclip (Pull Force panel > Umbrella sampling).
        PullPanel *pp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((pp = dynamic_cast<PullPanel*>(p)) != 0) break;
        if (!pp) {
          outcome = "no Pull Force panel";
        } else if (w[0] == "umbrellafind" && w.size() >= 2) {
          const bool ok = pp->umbrellaFind(atof(w[1].c_str()),
                                           w.size() > 3 ? wxString(w[2]) : wxString(),
                                           w.size() > 3 ? wxString(w[3]) : wxString());
          fprintf(stderr, "BUILDER: umbrellafind %s %s\n", ok ? "ok" : "failed",
                  pp->umbrellaState().utf8_str().data());
        } else if (w[0] == "umbrellacreate" && w.size() >= 3) {
          fprintf(stderr, "BUILDER: umbrellacreate %s\n",
                  pp->umbrellaCreate(atoi(w[1].c_str()), atoi(w[2].c_str()))
                      ? "ok" : "failed");
        } else if (w[0] == "umbrellawham") {
          bool jac = false;
          for (size_t k = 1; k < w.size(); k++) {
            if (w[k] == "jacobian") jac = true;
            else if (w[k].compare(0, 6, "noise=") == 0) pp->setNoise(w[k].substr(6));
          }
          const bool ok = pp->umbrellaWham(jac);
          fprintf(stderr, "BUILDER: umbrellawham %s %s\n", ok ? "ok" : "failed",
                  pp->umbrellaState().utf8_str().data());
        } else if (w[0] == "umbrellaexport" && w.size() >= 2) {
          fprintf(stderr, "BUILDER: umbrellaexport %s\n",
                  pp->umbrellaExport(wxString::FromUTF8(w[1].c_str())) ? "ok" : "failed");
        } else if (w[0] == "umbrellashot" && w.size() >= 2) {
          fprintf(stderr, "BUILDER: umbrellashot %s\n",
                  pp->umbrellaShot(wxString::FromUTF8(w[1].c_str()),
                                   w.size() > 3 ? atoi(w[2].c_str()) : 900,
                                   w.size() > 3 ? atoi(w[3].c_str()) : 600)
                      ? "ok" : "failed");
        } else if (w[0] == "umbrellapage") {
          pp->showUmbrella();
        } else if (w[0] == "umbrelladetails") {
          //  umbrelladetails [close]: the Details... window
          const bool open = !(w.size() > 1 && w[1] == "close");
          const bool ok = pp->umbrellaDetails(open);
          fprintf(stderr, "BUILDER: umbrelladetails %s lines=%d\n", ok ? "ok" : "failed",
                  (int)pp->umbrellaDetailsText().Freq('\n') + 1);
        } else if (w[0] == "umbrellaclip") {
          pp->clipAudit();
          fprintf(stderr, "BUILDER: umbrellaclip done\n");
        } else if (w[0] == "umbrellaanalysis" && w.size() >= 2) {
          //  umbrellaanalysis rms|gyrate: the pull page's MD Analysis links
          fprintf(stderr, "BUILDER: umbrellaanalysis %s\n",
                  pp->openAnalysis(w[1]) ? "ok" : "failed");
        } else if (w[0] == "umbrellastate") {
          fprintf(stderr, "BUILDER: umbrellastate %s\n", pp->umbrellaState().utf8_str().data());
        } else {
          outcome = "unknown umbrella command";
        }
      } else if (w[0] == "gmxa" && w.size() >= 2) {
        //  The MD Analysis panel (#877): gmxa analysis <rdf|energy|...>;
        //  gmxa set <field> <value...>; gmxa run (the script waits for it,
        //  at most 10 min); gmxa wait (for the group/term listing);
        //  gmxa state; gmxa dump <path> (the plotted curves); gmxa csv <path>.
        GmxAnalysisPanel *gp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((gp = dynamic_cast<GmxAnalysisPanel*>(p)) != 0) break;
        if (!gp) {
          outcome = "no MD Analysis panel";
        } else if (w[1] == "analysis" && w.size() >= 3) {
          if (!gp->setAnalysis(w[2])) outcome = "no analysis " + w[2];
        } else if (w[1] == "set" && w.size() >= 3) {
          const size_t at = line.find(w[2], line.find("set") + 3) + w[2].size();
          std::string value = at < line.size() ? line.substr(at) : std::string();
          while (!value.empty() && isspace((unsigned char)value[0])) value.erase(0, 1);
          outcome = gp->setField(w[2], value);
        } else if (w[1] == "run" || w[1] == "wait") {
          if (w[1] == "run" && !gp->run())
            outcome = "not started: " + std::string(gp->status().ToUTF8());
          st->busy = [gp]() { return gp->busy(); };
          st->waitUntil = wxGetLocalTimeMillis() + 600000;
        } else if (w[1] == "state") {
          std::string res = std::string(gp->resultsText().ToUTF8());
          for (char& c : res) if (c == '\n') c = '|';
          fprintf(stderr, "BUILDER: gmxa analysis=%s busy=%d curves=%d status=[%s] "
                  "groups=[%s] terms=[%s] results=[%s]\n", gp->analysis().c_str(),
                  gp->busy() ? 1 : 0, gp->plot()->curveCount(),
                  std::string(gp->status().ToUTF8()).c_str(), gp->groupChoices().c_str(),
                  gp->energyTerms().c_str(), res.c_str());
        } else if (w[1] == "dump" && w.size() >= 3) {
          std::ofstream out(w[2].c_str());
          out << gp->plotDump();
          out << "#json " << gp->lastJson() << "\n";
          if (!out) outcome = "cannot write " + w[2];
        } else if (w[1] == "csv" && w.size() >= 3) {
          if (!gp->exportCsv(wxString::FromUTF8(w[2].c_str()))) outcome = "csv failed";
        } else {
          outcome = "unknown gmxa command";
        }
      } else if ((w[0] == "mdcsv" || w[0] == "mdcompare" || w[0] == "comparewith" || w[0] == "mdfit" ||
                  w[0] == "mdloglog") && w.size() >= 2) {
        //  mdcsv speed|msd <path>; mdcompare speed|msd <calculation...|none|?>;
        //  comparewith [speed|msd] <calculation...> defaults to speed;
        //  mdfit <from ps> <to ps>; mdloglog on|off: the Speed Distribution
        //  and Displacement panels' Export, Compare, Fit and Log-log without
        //  dialogs or clicks.
        const bool bareCompare = w[0] == "comparewith" && w[1] != "speed" && w[1] != "msd";
        const bool speed = bareCompare || w[1] == "speed";
        MdAnalysisPanel *mp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this)) {
            if (w[0] == "mdfit" || w[0] == "mdloglog") mp = dynamic_cast<MsdPanel*>(p);
            else if (speed) mp = dynamic_cast<SpeedPanel*>(p);
            else mp = dynamic_cast<MsdPanel*>(p);
            if (mp) break;
          }
        MsdPanel *msd = dynamic_cast<MsdPanel*>(mp);
        if (!mp) {
          outcome = "no Speed Distribution or Displacement panel";
        } else if (w[0] == "mdcsv" && w.size() >= 3) {
          fprintf(stderr, "BUILDER: mdcsv curves=%d %s\n", mp->curves(),
                  mp->exportCsv(wxString::FromUTF8(w[2].c_str())) ? "ok" : "failed");
        } else if (w[0] == "mdcompare" && w.size() >= 3 && w[2] == "?") {
          //  mdcompare speed|msd ?: what Compare would offer.
          std::vector<std::string> labels;
          wxStopWatch took;
          mp->comparable(&labels);
          std::string all;
          for (const std::string& l : labels) all += (all.empty() ? "" : "|") + l;
          fprintf(stderr, "BUILDER: mdcompare candidates=%d ms=%ld %s\n",
                  (int)labels.size(), took.Time(), all.c_str());
        } else if ((w[0] == "mdcompare" || w[0] == "comparewith") &&
                   w.size() >= (bareCompare ? 2u : 3u)) {
          const size_t firstName = bareCompare ? 1 : 2;
          std::vector<std::string> names(w.begin() + firstName, w.end());
          if (names.size() == 1 && names[0] == "none") names.clear();
          const bool ok = mp->compareWith(names);
          fprintf(stderr, "BUILDER: mdcompare %s curves=%d %s\n", w[firstName].c_str(), mp->curves(),
                  ok ? "ok" : "failed");
        } else if (w[0] == "mdfit" && w.size() >= 3 && msd) {
          msd->setFit(atof(w[1].c_str()), atof(w[2].c_str()));
          fprintf(stderr, "BUILDER: mdfit D=%.4g\n", msd->diffusion(0));
        } else if (w[0] == "mdloglog" && msd) {
          msd->setLogLog(w[1] == "on");
          fprintf(stderr, "BUILDER: mdloglog %s\n", w[1].c_str());
        }
      } else if (w[0] == "rxnenergies" && w.size() >= 2) {
        //  rxnenergies <path> [kcal]: the Reaction Energies panel's Export
        //  .csv (#419) without the file dialog.
        ReactionPanel *rp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
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
      } else if (w[0] == "thermolayout" || (w[0] == "thermo" && w.size() >= 2)) {
        //  thermo <treatment> [T] [1atm|1M]: set the Thermochemistry panel's
        //  choices (#692) and print the recomputed H, S, G.
        ThermoPanel *tp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((tp = dynamic_cast<ThermoPanel*>(p)) != 0) break;
        if (!tp) {
          outcome = "no Thermochemistry panel";
        } else if (w[0] == "thermolayout") {
          // The grids sit in the panel's scrolled body (#1044).
          wxScrolledWindow *body = tp->body();
          fprintf(stderr, "BUILDER: thermolayout body client=%d virtual=%d scrollrange=%d\n",
                  body->GetClientSize().y, body->GetVirtualSize().y,
                  body->GetScrollRange(wxVERTICAL));
          for (wxWindow *child : body->GetChildren()) {
            wxGrid *grid = wxDynamicCast(child, wxGrid);
            if (!grid) continue;
            wxClientDC dc(grid);
            dc.SetFont(grid->GetLabelFont());
            for (int row = 0; row < grid->GetNumberRows(); ++row)
              fprintf(stderr, "BUILDER: thermolayout row=%d width=%d text=%d\n",
                      row, grid->GetRowLabelSize(),
                      dc.GetTextExtent(grid->GetRowLabelValue(row)).x);
            for (int col = 0; col < grid->GetNumberCols(); ++col)
              fprintf(stderr, "BUILDER: thermolayout col=%d width=%d text=%d\n",
                      col, grid->GetColSize(col),
                      dc.GetTextExtent(grid->GetColLabelValue(col)).x);
            fprintf(stderr, "BUILDER: thermolayout viewport=%d content=%d scroll=%d thumb=%d\n",
                    grid->GetGridWindow()->GetClientSize().x,
                    grid->GetColSize(0) + grid->GetColSize(1),
                    grid->GetScrollRange(wxHORIZONTAL),
                    grid->GetScrollThumb(wxHORIZONTAL));
            int rowsHeight = 0;
            for (int row = 0; row < grid->GetNumberRows(); ++row)
              rowsHeight += grid->GetRowSize(row);
            fprintf(stderr, "BUILDER: thermolayout height=%d rows=%d\n",
                    grid->GetGridWindow()->GetClientSize().y, rowsHeight);
          }
          ewxWindowUtils::clipAuditReport(tp, "Thermochemistry");
        } else {
          ThermoValue v;
          const bool set = tp->setChoice(w[1], w.size() > 2 ? atof(w[2].c_str()) : 298.15,
                                         w.size() > 3 && w[3] == "1M");
          const bool have = set && tp->shown(&v);
          fprintf(stderr, "BUILDER: thermo %s H=%.6f S=%.6f G=%.6f %s\n", w[1].c_str(),
                  v.H, v.S, v.G, have ? "ok" : ("failed: " + tp->errorText()).c_str());
          // The contributions table (#1044), row by row as shown.
          std::istringstream rows(tp->contribText());
          for (std::string l; std::getline(rows, l); )
            fprintf(stderr, "BUILDER: thermocontrib %s\n", l.c_str());
        }
      } else if (w[0] == "ircpath" && w.size() >= 2) {
        //  ircpath <path> [kcal]: the IRC Path panel's Export .csv (#417)
        //  without the file dialog.
        IRCPanel *ip = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
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
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this)) {
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
      } else if (w[0] == "scanview") {
        //  scanview [graph|table]: the Relaxed Scan panel's Show choice (#584);
        //  logs which of plot and table is shown, and their heights
        ScanPanel *sp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((sp = dynamic_cast<ScanPanel*>(p)) != 0) break;
        if (!sp) outcome = "no Relaxed Scan panel";
        else {
          if (w.size() >= 2) sp->setTableView(w[1] == "table");
          fprintf(stderr, "BUILDER: scanview %s\n", sp->viewState().c_str());
        }
      } else if ((w[0] == "scanrow" || w[0] == "scanstep") && w.size() >= 2) {
        //  scanrow <i>: a click on table row i; scanstep first|prev|next|last:
        //  the playback buttons.  Reports the selection and the viewer's
        //  first two atoms.
        ScanPanel *sp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
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
      } else if (w[0] == "propchange" && w.size() >= 2 && p_calculation) {
        //  propchange <KEY>: what a running job's property message does.
        //  The context key, which property messages carry as their url.
        std::string url = p_calculation->getURL().toString();
        for (const auto& c : p_calculations)
          if (c.second == p_calculation) url = c.first;
        propertyChanged(url, w[1], "");
        fprintf(stderr, "BUILDER: propchange %s\n", w[1].c_str());
      } else if (w[0] == "scan" && w.size() >= 2) {
        ScanPanel *sp = nullptr;
        if (p_calculation)
          for (auto *panel : PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((sp = dynamic_cast<ScanPanel*>(panel))) break;
        if (!sp) outcome = "no Relaxed Scan panel";
        else if (w[1] == "cpsingle" && (w.size() == 2 || (w.size() == 3 && w[2] == "launch"))) {
          std::string why;
          const auto url = sp->counterpoiseSinglePoints(w.size() == 3, why);
          if (url.empty()) outcome = why;
          fprintf(stderr, "BUILDER: scan cpsingle url=%s %s\n", url.c_str(), why.c_str());
        } else if (w[1] == "cpcolumn") {
          fprintf(stderr, "BUILDER: scan cpcolumn\n%s", sp->counterpoiseColumn().c_str());
        } else outcome = "use scan cpsingle [launch] or scan cpcolumn";
      } else if (w[0] == "scanmorse" || w[0] == "scancompare" || w[0] == "scanpng" ||
                 w[0] == "scanzero" || w[0] == "scanjoin" || w[0] == "scanlayout") {
        //  scanmorse on|off [window]: the Fit Morse check and window (the
        //  panel's unit); logs the fit and the text box (#858).
        //  scancompare [none|NAME...]: lists the scans beside this one, or
        //  draws the named ones.  scanpng PATH [W H]: the plot on white.
        ScanPanel *sp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((sp = dynamic_cast<ScanPanel*>(p)) != 0) break;
        if (!sp) {
          outcome = "no Relaxed Scan panel";
        } else if (w[0] == "scanmorse") {
          const std::string state = sp->setMorse(
              w.size() >= 2 && w[1] == "on", w.size() >= 3 ? atof(w[2].c_str()) : 0);
          wxString text = sp->morseText();
          text.Replace("\n", " | ");
          fprintf(stderr, "BUILDER: scanmorse %s\nBUILDER: scanmorsetext [%s]\n",
                  state.c_str(), text.utf8_str().data());
        } else if (w[0] == "scanzero") {
          if (w.size() != 2 || !sp->setZero(w[1])) outcome = "use scanzero lowest|largest|first";
        } else if (w[0] == "scanjoin") {
          if (w.size() != 2 || (w[1] != "on" && w[1] != "off")) outcome = "use scanjoin on|off";
          else sp->setJoin(w[1] == "on");
        } else if (w[0] == "scanlayout") {
          if (w.size() != 2 || (w[1] != "overlay" && w[1] != "multiples"))
            outcome = "use scanlayout overlay|multiples";
          else sp->setSmallMultiples(w[1] == "multiples");
        } else if (w[0] == "scancompare") {
          std::string why, list;
          if (w.size() >= 2) {
            std::vector<std::string> names;
            if (w[1] != "none") names.assign(w.begin() + 1, w.end());
            if (!sp->setCompared(names, why)) outcome = why;
          }
          for (const std::string& n : sp->compareCandidates()) list += " " + n;
          fprintf(stderr, "BUILDER: scancompare candidates:%s\n", list.c_str());
        } else if (w.size() >= 2) {
          const int wd = w.size() >= 4 ? atoi(w[2].c_str()) : 900;
          const int ht = w.size() >= 4 ? atoi(w[3].c_str()) : 560;
          fprintf(stderr, "BUILDER: scanpng %s\n",
                  sp->savePng(wxString::FromUTF8(w[1].c_str()), wd, ht) ? "saved" : "failed");
        }
      } else if (w[0] == "scanstatus") {
        //  scanstatus: the Relaxed Scan panel's status line
        ScanPanel *sp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((sp = dynamic_cast<ScanPanel*>(p)) != 0) break;
        fprintf(stderr, "BUILDER: scanstatus [%s]\n",
                sp ? sp->status().utf8_str().data() : "no panel");
      } else if ((w[0] == "ircplayopt" || w[0] == "scanplayopt") && w.size() >= 2) {
        //  ircplayopt|scanplayopt speed <ms per step> |
        //  repeat <off|start|backforth> | loop <on|off> | order <ticks>: the
        //  playback Delay field and Repeat choice (loop on = backforth); order
        //  logs the points <ticks> ticks of play would show (no timers).
        IRCPanel *ip = 0;
        ScanPanel *sp = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this)) {
            if (w[0] == "ircplayopt" && (ip = dynamic_cast<IRCPanel*>(p)) != 0) break;
            if (w[0] == "scanplayopt" && (sp = dynamic_cast<ScanPanel*>(p)) != 0) break;
          }
        if (!ip && !sp) {
          outcome = "no such panel";
        } else {
          const std::string arg = w.size() >= 3 ? w[2] : "";
          if (w[1] == "speed") { if (ip) ip->setPlaybackSpeed(arg); else sp->setPlaybackSpeed(arg); }
          if (w[1] == "mo" && ip) { ip->setShowMo(arg == "on"); ip->flushFollow(); }
          if (w[1] == "flush" && ip) ip->flushFollow();
          playbackloop::Repeat rep;
          if ((w[1] == "loop" || w[1] == "repeat") && playbackloop::parseRepeat(arg, rep)) {
            if (ip) ip->setPlaybackRepeat(rep); else sp->setPlaybackRepeat(rep);
          }
          std::string order;
          if (w[1] == "order")
            for (int i : ip ? ip->playOrder(atoi(arg.c_str())) : sp->playOrder(atoi(arg.c_str())))
              order += " " + std::to_string(i + 1);
          const playbackloop::Repeat now = ip ? ip->playbackRepeat() : sp->playbackRepeat();
          fprintf(stderr, "BUILDER: %s %s speed=%s loop=%d repeat=%s order:%s\n", w[0].c_str(),
                  w[1].c_str(), (ip ? ip->playbackSpeed() : sp->playbackSpeed()).c_str(),
                  now == playbackloop::BACK_AND_FORTH ? 1 : 0,
                  playbackloop::repeatWord(now), order.c_str());
          if (ip)
            fprintf(stderr, "BUILDER: ircplayopt mobox enabled=%d on=%d\n",
                    ip->showMoAvailable() ? 1 : 0, ip->showMo() ? 1 : 0);
        }
      } else if (w[0] == "ircaxes" && w.size() >= 2) {
        //  ircaxes <i>: the IRC Path axis labels, table headers and hover text
        IRCPanel *ip = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((ip = dynamic_cast<IRCPanel*>(p)) != 0) break;
        if (!ip) outcome = "no IRC Path panel";
        else fprintf(stderr, "BUILDER: ircaxes %s\n", ip->axisText(atoi(w[1].c_str())).c_str());
      } else if (w[0] == "ircview" && w.size() >= 2) {
        //  ircview graph|table: the IRC Path panel's Show choice (#452)
        IRCPanel *ip = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((ip = dynamic_cast<IRCPanel*>(p)) != 0) break;
        if (!ip) outcome = "no IRC Path panel";
        else ip->setTableView(w[1] == "table");
      } else if ((w[0] == "ircstep" || w[0] == "ircrow") && w.size() >= 2) {
        //  ircrow <i>: a click on table row i.
        //  ircstep first|prev|next|last|play: the IRC Path playback buttons;
        //  play steps through the whole path (reverse end, TS, forward end)
        //  and logs the points in the order shown.
        IRCPanel *ip = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if ((ip = dynamic_cast<IRCPanel*>(p)) != 0) break;
        if (!ip) {
          outcome = "no IRC Path panel";
        } else {
          std::string order;
          if (w[1] == "play") {
            for (int i = ip->step("first"); i >= 0; ) {
              order += " " + std::to_string(i + 1);
              if (i >= ip->points() - 1) break;
              i = ip->step("next");
            }
          } else if (w[0] == "ircrow") {
            ip->selectRow(atoi(w[1].c_str()));
          } else {
            ip->step(w[1]);
          }
          fprintf(stderr, "BUILDER: %s %s point=%d row=%d marker=%d mo=%d order:%s\n",
                  w[0].c_str(), w[1].c_str(), ip->selectedPoint(), ip->selectedRow(),
                  ip->markerPoint(), ip->followedMo() + 1, order.c_str());
        }
      } else if (w[0] == "ircclick" && w.size() >= 2) {
        //  ircclick <index>: what a click on IRC Path plot point <index>
        //  does (#417); reports the frame shown and the first two atoms.
        IRCPanel *ip = 0;
        if (p_calculation)
          for (PropertyPanel *p :
               PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
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
          fprintf(stderr, "BUILDER: ircstatus %s\n", ip->statusText().c_str());
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
            VizPropertyPanel::getPanels(p_calculation->getURL().toString(), "", this);
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
      } else if (w[0] == "selectoverlay") {
        //  "selectoverlay [N [M]]": select atoms N (and M), from 1, and
        //  report the bottom-left overlay text (#1236).
        std::string err;
        outcome = selectOverlay(w, err) ? "ok" : "FAIL: " + err;
      } else if (w[0] == "stamp") {
        //  Wall-clock seconds, to time what a script line saw.
        fprintf(stderr, "BUILDER: stamp %.3f\n",
                std::chrono::duration<double>(
                    std::chrono::system_clock::now().time_since_epoch()).count());
      } else if (w[0] == "drawn") {
        //  Atoms handed to the display nodes, by element, against the
        //  fragment's own count.
        std::string line;
        int total = 0;
        std::map<std::string,int> drawn = getSG()->drawnAtomsByElement();
        for (std::map<std::string,int>::const_iterator it = drawn.begin();
             it != drawn.end(); ++it) {
          line += " " + it->first + "=" + std::to_string(it->second);
          total += it->second;
        }
        fprintf(stderr, "BUILDER: drawn%s total=%d of %d\n", line.c_str(), total,
                frag ? (int)frag->numAtoms() : -1);
      } else if (w[0] == "bondlen" && frag) {
        //  The longest bond and the bonds longer than 2.5 A: a molecule
        //  broken over a cell face is bonded across the cell.
        int bonds = 0, longer = 0;
        double longest = 0;
        for (size_t i = 0; i < frag->numAtoms(); i++) {
          const std::vector<TBond*>& bl = frag->atomRef(i)->bondList();
          for (size_t b = 0; b < bl.size(); b++) {
            if (bl[b]->atom1() != frag->atomRef(i)) continue;
            const double *p = bl[b]->atom1()->coordinates();
            const double *q = bl[b]->atom2()->coordinates();
            const double d = sqrt((p[0]-q[0])*(p[0]-q[0]) + (p[1]-q[1])*(p[1]-q[1]) +
                                  (p[2]-q[2])*(p[2]-q[2]));
            bonds++;
            if (d > 2.5) longer++;
            if (d > longest) longest = d;
          }
        }
        fprintf(stderr, "BUILDER: bondlen bonds=%d longest=%.2f longer_than_2.5=%d\n",
                bonds, longest, longer);
      } else if (w[0] == "background" && w.size() == 4) {
        getViewer().setBackgroundColor(SbColor(atof(w[1].c_str()),
            atof(w[2].c_str()), atof(w[3].c_str())));
        getSG()->setDepthCueColor(atof(w[1].c_str()), atof(w[2].c_str()), atof(w[3].c_str()));
        SbColor fg = getViewer().calculateContrastingColor();
        ForegroundCmd fgCmd("Foreground", getSG());
        fgCmd.getParameter("red")->setDouble(fg[0]);
        fgCmd.getParameter("green")->setDouble(fg[1]);
        fgCmd.getParameter("blue")->setDouble(fg[2]);
        fgCmd.execute();
      } else if (w[0] == "style" && w.size() >= 2) {
        // "style Ball And Stick", as Render > <style>.
        string name = w[1];
        for (size_t i = 2; i < w.size(); i++) name += " " + w[i];
        p_viewerEvtHandler->doStyleChange(wxString(name));
      } else if (w[0] == "viewall") {
        getViewer().viewAll();
      } else if ((w[0] == "rotate" || w[0] == "rotatex") && w.size() == 2) {
        // Orbit the camera about the world y (rotatex: x) axis, in degrees.
        SoCamera *cam = getViewer().getCamera();
        SbRotation r(w[0] == "rotate" ? SbVec3f(0, 1, 0) : SbVec3f(1, 0, 0),
                     (float)(atof(w[1].c_str()) * M_PI / 180.0));
        SbVec3f np;
        r.multVec(cam->position.getValue(), np);
        cam->position.setValue(np);
        cam->orientation.setValue(r * cam->orientation.getValue());
      } else if (w[0] == "stressinfo") {
        bool found = false;
        for (PropertyPanel *panel : PropertyPanel::getPanels(
                 p_calculation->getURL().toString(), "", this)) {
          if (auto *stress = dynamic_cast<StressPanel*>(panel)) {
            fprintf(stderr, "BUILDER: stressinfo\n%s\n",
                    (stress->summaryText()+"\n"+stress->labelAnchorText()).c_str());
            found = true;
          }
        }
        if (!found) outcome = "FAIL: no Stress Tensor panel";
      } else if (w[0] == "vector" && w.size() == 2) {
        {
          int sliders = 0;
          std::function<void(wxWindow*)> count = [&](wxWindow *win) {
            if (dynamic_cast<SliderCombo*>(win)) sliders++;
            for (wxWindow *c : win->GetChildren()) count(c);
          };
          for (VizPropertyPanel *panel : VizPropertyPanel::getPanels(
                   p_calculation->getURL().toString(), "", this))
            if (panel->getName() == w[1] || panel->hasFocus()) count(panel);
          fprintf(stderr, "BUILDER: vector %s slider=%d\n", w[1].c_str(),
                  sliders);
        }
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
        fprintf(stderr, "BUILDER: vector %s label=%s\n", w[1].c_str(),
                getViewer().getURightText().c_str());
        double longest = 0.0;
        vector<TBond*> *bonds = frag ? frag->bonds() : 0;
        for (size_t b = 0; bonds && b < bonds->size(); b++)
          longest = std::max(longest, (double)(*bonds)[b]->length());
        delete bonds;
        fprintf(stderr, "BUILDER: vector %s arrows=%d longest_bond=%.4f\n",
                w[1].c_str(), sa.getPaths().getLength(), longest);
      } else if (w[0] == "bonddipoles" && w.size() >= 2) {
        //  "bonddipoles on [KEY] | off": the Dipole Moment panel's Bond
        //  dipoles box and charge choice (KEY: MULLIKEN, ESPFIT, ...); the
        //  table rows and the arrow count are logged.
        MomentsPanel *mp = 0;
        for (VizPropertyPanel *panel : VizPropertyPanel::getPanels(
                 p_calculation->getURL().toString(), "", this))
          if (MomentsPanel *m = dynamic_cast<MomentsPanel*>(panel))
            if (m->hasBondDipoles()) mp = m;
        if (!mp) {
          outcome = "FAIL: no Dipole Moment panel with bond dipoles";
        } else {
          mp->setBondDipoles(w[1] == "on", w.size() > 2 ? w[2] : "");
          wxTheApp->Yield(true);
          vector<vector<string> > rows = mp->bondDipoleRows();
          for (size_t i = 0; i < rows.size(); i++)
            fprintf(stderr, "BUILDER: bonddipoles row %zu | %s | %s | %s\n", i,
                    rows[i][0].c_str(), rows[i][1].c_str(),
                    rows[i][2].c_str());
          SoSearchAction sa;
          sa.setType(VRVector::getClassTypeId(), FALSE);
          sa.setInterest(SoSearchAction::ALL);
          sa.apply(getSG()->getPropertySwitch("DIPOLE"));
          fprintf(stderr, "BUILDER: bonddipoles rows=%zu arrows=%d\n",
                  rows.size(), sa.getPaths().getLength());
        }
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
      } else if (w[0] == "surface" && w.size() == 2) {
        SoSearchAction sa;
        sa.setType(SoIndexedFaceSet::getClassTypeId(), FALSE);
        sa.setInterest(SoSearchAction::ALL);
        sa.apply(getSG()->getPropertySwitch(w[1]));
        for (int k = 0; k < sa.getPaths().getLength(); k++) {
          SoGetMatrixAction ma(SbViewportRegion(100, 100));
          ma.apply(sa.getPaths()[k]);
          const SbMatrix m = ma.getMatrix();
          SoIndexedFaceSet *fs =
              (SoIndexedFaceSet*) sa.getPaths()[k]->getTail();
          SoSearchAction ca;
          ca.setType(SoCoordinate3::getClassTypeId(), FALSE);
          ca.apply(sa.getPaths()[k]->getNodeFromTail(1));
          SoCoordinate3 *cs = ca.getPath() ?
              (SoCoordinate3*) ca.getPath()->getTail() : NULL;
          float reach = 0.f;
          const int nv = cs ? cs->point.getNum() : 0;
          for (int i = 0; i < nv; i++)
            reach = std::max(reach, cs->point[i].length());
          fprintf(stderr, "BUILDER: surface %s %d vertices=%d triangles=%d "
                  "centre=%.4f,%.4f,%.4f farthest=%.4f\n", w[1].c_str(), k,
                  nv, fs->coordIndex.getNum()/4, m[3][0], m[3][1], m[3][2],
                  reach);
        }
        fprintf(stderr, "BUILDER: surface %s count=%d\n", w[1].c_str(),
                sa.getPaths().getLength());
      } else if (w[0] == "vinabox" || w[0] == "vinaboxinfo" ||
                 w[0] == "selectatoms" || w[0] == "poses" ||
                 w[0] == "findpockets") {
        outcome = vinaScriptCommand(w);
      } else if (w[0].compare(0, 4, "gten") == 0 && w[0] != "gtenkind") {
        outcome = ensembleCommand(w);
        if (outcome.empty()) outcome = "ok";
      } else if (w[0] == "nmr" && w.size() >= 2) {
        NmrPanel *nmr = nullptr;
        if (p_calculation)
          for (PropertyPanel *p : PropertyPanel::getPanels(p_calculation->getURL().toString(), "", this))
            if (auto *panel = dynamic_cast<NmrPanel*>(p)) nmr = panel;
        bool ok = nmr != nullptr;
        if (nmr) {
          nmr->ensureInitialized();
          const std::string arg = w.size() > 2 ? w[2] : "";
          if (w[1] == "element") ok = nmr->chooseElement(arg);
          else if (w[1] == "reference") ok = nmr->setReference(arg == "empty" ? "" : arg);
          else if (w[1] == "calculation") ok = nmr->referenceCalculation(arg);
          else if (w[1] == "width") {
            double v;
            ok = wxString::FromUTF8(arg.c_str()).ToDouble(&v) && nmr->setWidth(v);
          } else if (w[1] == "csv") ok = nmr->exportCsv(wxString::FromUTF8(arg.c_str()));
          else if (w[1] == "png") ok = nmr->exportPng(wxString::FromUTF8(arg.c_str()));
          else if (w[1] == "click") ok = nmr->clickGroup(atoi(arg.c_str()) - 1);
          else if (w[1] != "status") ok = false;
          fprintf(stderr, "NMR: ok=%d %s\n", ok, nmr->summary().c_str());
          if (w[1] == "click") {
            fprintf(stderr, "NMR selected:");
            SGFragment *frag = getSceneGraph().getFragment();
            if (frag) for (int a : frag->m_atomHighLight) fprintf(stderr, " %d", a + 1);
            fprintf(stderr, "\n");
          }
        }
        outcome = ok ? "ok" : "FAIL: NMR command";
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


//  Edit > Select Protein / Select Ligand (#967).  Without residue
//  information, or with nothing of the kind, it warns and selects nothing.
//  Both replace the selection, so Select Ligand then Select Protein leaves
//  only the protein selected.
bool Builder::selectResidueClass(bool ligand)
{
  SGFragment *frag = getSG()->getFragment();
  if (!frag || frag->numResidues() == 0) {
    showMessage(ligand ? "Select Ligand: this structure has no residue "
                         "information, so a ligand cannot be told from the "
                         "rest."
                       : "Select Protein: this structure has no residue "
                         "information.");
    return false;
  }
  SelectCmd *cmd = new SelectCmd(ligand ? "Select Ligand" : "Select Protein",
                                 getSG());
  cmd->getParameter("clear")->setBoolean(true);
  cmd->getParameter(ligand ? "ligand" : "protein")->setBoolean(true);
  execute(cmd);
  if (ligand) {
    showMessage(cmd->message().empty()
                ? "Select Ligand: no ligand found; every residue is polymer, "
                  "water, an ion or a common additive."
                : cmd->message());
    return !cmd->message().empty();
  }
  if (frag->m_atomHighLight.empty()) {
    showMessage("Select Protein: no amino acid or nucleic acid residues found.");
    return false;
  }
  return true;
}

void Builder::focusView()
{
  CallAfter([this]() {
    if (SoWxRenderArea *area = findViewerArea(p_viewer)) area->SetFocus();
  });
  //  After opening a structure the panels are still being built and one of
  //  their buttons takes the focus; ask once more when that has settled.
  wxTimer *later = new wxTimer();
  wxWeakRef<Builder> self(this);
  later->Bind(wxEVT_TIMER, [self, later](wxTimerEvent&) {
    SoWxRenderArea *area = self ? findViewerArea(self->p_viewer) : 0;
    if (area) {
      wxWindow *f = wxWindow::FindFocus();
      if (!dynamic_cast<wxTextEntry*>(f)) area->SetFocus();
    }
    later->Stop();
    wxTheApp->CallAfter([later]() { delete later; });
  });
  later->StartOnce(400);
}


void Builder::OnSelectProteinClick(wxCommandEvent&)
{
  selectResidueClass(false);
}

//  Select Ligand lists the candidates, the likely ligand first.  The menu
//  is refilled only as Edit opens: changing a menu while it is open breaks
//  the item under the pointer (#1239).  Clicking the entry itself (script)
//  picks the first.
void Builder::OnSelectLigandClick(wxCommandEvent&)
{
  selectResidueClass(true);
}

void Builder::fillLigandMenu()
{
  if (!p_ligandMenu) return;
  SGFragment *frag = p_sgMgr ? getSG()->getFragment() : 0;
  vector<SGFragment::LigandCandidate> cands;
  if (frag) cands = frag->ligandCandidates();
  while (p_ligandMenu->GetMenuItemCount() > 0)
    p_ligandMenu->Destroy(p_ligandMenu->FindItemByPosition(0));
  p_ligandFirstAtoms.clear();
  for (size_t k = 0; k < cands.size() && k < 16; k++) {
    const SGFragment::LigandCandidate& c = cands[k];
    wxString label = wxString::Format("%s %d (%d atom%s)", c.name, c.number,
                                      c.atoms, c.atoms == 1 ? "" : "s");
    p_ligandMenu->Append(ID_LIGAND_FIRST + (int)k, label);
    p_ligandFirstAtoms.push_back(c.firstAtom);
  }
  if (cands.empty()) {
    p_ligandMenu->Append(ID_LIGAND_FIRST, _("(no ligand candidates)"));
    p_ligandMenu->Enable(ID_LIGAND_FIRST, false);
  }
}

bool Builder::selectLigandCandidate(int n)
{
  SGFragment *frag = getSG()->getFragment();
  if (!frag || n < 0 || n >= (int)p_ligandFirstAtoms.size()) return false;
  const int first = p_ligandFirstAtoms[n];
  SelectCmd *cmd = new SelectCmd("Select Ligand", getSG());
  cmd->getParameter("clear")->setBoolean(true);
  cmd->getParameter("full_residue")->setInteger(first);
  execute(cmd);
  const Residue *res = frag->atomRef(first)->getResidue();
  showMessage("Selected " + std::to_string(frag->m_atomHighLight.size()) +
              " atoms of " + (res ? res->name() : string("the residue")) + ".");
  return !frag->m_atomHighLight.empty();
}

void Builder::OnSelectLigandItem(wxCommandEvent& event)
{
  selectLigandCandidate(event.GetId() - ID_LIGAND_FIRST);
}

void Builder::OnMenuOpen(wxMenuEvent& event)
{
  if (event.GetMenu() == p_editMenu) fillLigandMenu();
  event.Skip();
}

//  Edit > Select Heteroatoms (not protein) (#1244): then Delete leaves the
//  protein and the ligand.
bool Builder::selectHeteroatoms()
{
  SGFragment *frag = getSG()->getFragment();
  if (!frag || frag->numResidues() == 0) {
    showMessage("Select Heteroatoms: this structure has no residue "
                "information.");
    return false;
  }
  SelectCmd *cmd = new SelectCmd("Select Heteroatoms", getSG());
  cmd->getParameter("clear")->setBoolean(true);
  cmd->getParameter("heteroatoms")->setBoolean(true);
  execute(cmd);
  showMessage(cmd->message());
  return !frag->m_atomHighLight.empty();
}

void Builder::OnSelectHeteroClick(wxCommandEvent&)
{
  selectHeteroatoms();
}

//  Greyed, with the reason in the label, while there are no residues; the
//  handler still explains itself if it is reached anyway.
void Builder::OnUpdateSelectResidueItem(wxUpdateUIEvent& event)
{
  SGContainer *sg = p_sgMgr ? getSG() : NULL;
  SGFragment *frag = sg ? sg->getFragment() : NULL;
  const bool has = frag && frag->numResidues() > 0;
  event.Enable(has);
  const bool ligand = event.GetId() == ID_SELECT_LIGAND;
  event.SetText(has ? (ligand ? _("Select Ligand") : _("Select Protein"))
                    : (ligand ? _("Select Ligand (no residue information)")
                              : _("Select Protein (no residue information)")));
}

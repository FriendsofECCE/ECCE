#include "wx/wrapsizer.h"
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <ios>
#include <iomanip>
#include <memory>
#include <set>
#include <sstream>
#include <utility>

#include <wx/app.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/dcbuffer.h>
#include <wx/link.h>
#include <wx/sizer.h>
#include <wx/stopwatch.h>
#include <wx/timer.h>
#include <wx/toplevel.h>

#include "util/Ecce.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"
#include <wx/checklst.h>

#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/ewxStaticText.H"

#include "tdat/MoAoBasis.H"
#include "tdat/MoComposition.H"
#include "tdat/MoFragments.H"
#include "tdat/FoDiagram.H"
#include "tdat/ModelDiagram.H"
#include "tdat/MoLigandField.H"
#include "tdat/PropTable.H"
#include "tdat/TGBSAngFunc.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TGaussianBasisSet.H"
#include "dsm/ICalculation.H"
#include "dsm/TaskJob.H"
#include "dsm/ICalcUtils.H"
#include "dsm/JCode.H"
#include "tdat/CharacterTable.H"
#include "tdat/SymmetryOps.H"
#include "tdat/SymmetryAnalysis.H"
#include "tdat/RadialDist.H"
#include "tdat/ShellRotation.H"
#include "tdat/BasisFlatten.H"
#include "tdat/MoAoOrder.H"
#include "tdat/EspField.H"
#include "tdat/TAtm.H"

#include "dsm/IPropCalculation.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "wxviz/WxVizToolFW.H"

#include "MoPanel.H"
#include "wxgui/ewxConfig.H"
#include "MoDiagramPanel.H"

using std::max;
using std::min;
using std::ostringstream;

wxFORCE_LINK_THIS_MODULE(MoDiagramPanel)


#include "MoDiagramCanvas.H"



BEGIN_EVENT_TABLE( MoDiagramPanel, VizPropertyPanel )
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(MoDiagramPanel, VizPropertyPanel)


MoDiagramPanel::MoDiagramPanel()
  : p_fragmentation(MoFragments::NOT_BUILT),
    p_canvas(0), p_fragPanel(0), p_fragToggle(0), p_optToggle(0), p_optSizer(0), p_optRow(0), p_optViews(0), p_optOpen(true), p_fragWasOpen(false), p_fragments(0), p_autoFragments(0), p_construction(0), p_piOnly(0), p_showAll(0), p_ligandField(0),
    p_about(0), p_pickFirst(0), p_pickSecond(0), p_compute(0),
    p_pickA(-1), p_pickB(-1), p_pickEnvDone(false),
    p_building(false), p_rebuildPending(false)
{
}


MoDiagramPanel::MoDiagramPanel(IPropCalculation *calculation,
      wxWindow *parent, wxWindowID id, const wxPoint& pos,
      const wxSize& size, long style, const wxString& name)
  : p_fragmentation(MoFragments::NOT_BUILT),
    p_canvas(0), p_fragPanel(0), p_fragToggle(0), p_optToggle(0), p_optSizer(0), p_optRow(0), p_optViews(0), p_optOpen(true), p_fragWasOpen(false), p_fragments(0), p_autoFragments(0), p_construction(0), p_piOnly(0), p_showAll(0), p_ligandField(0),
    p_about(0), p_pickFirst(0), p_pickSecond(0), p_compute(0),
    p_pickA(-1), p_pickB(-1), p_pickEnvDone(false),
    p_building(false), p_rebuildPending(false)
{
  Create(calculation, parent, id, pos, size, style, name);
}


MoDiagramPanel::~MoDiagramPanel()
{
}


/////////////////////////////////////////////////////////////////////////////
// Description
//   Show a clicked level's orbital in the viewer.
//
//   Handed to the MOs panel rather than reimplemented here: that panel
//   owns the grid, the basis set, the isovalue and the whole compute
//   path, and a second way of putting an orbital on screen would be a
//   second thing to keep working.
//
//   The MOs panel may not be open.  That is not an error -- the diagram
//   is available for a calculation whose orbitals cannot be rendered at
//   all, which is why it claims ORBENG and not MO -- so it says so
//   instead of failing silently.
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Description
//   How many basis functions each atom carries.
//
//   The molecular orbital coefficients are stored atom by atom in this
//   order, so this is what says which coefficients belong to which
//   fragment -- and therefore how much of an orbital sits on each side
//   of the diagram.
//
//   The count is checked against the coefficient table's own width,
//   which is what says whether the recorded coordinate system
//   (Cartesian/spherical) is the one the coefficients were actually
//   printed in -- a spherical-only code such as ORCA ran a nominally
//   Cartesian Pople basis with five d functions, not six, which is
//   the same width mismatch computeFullGroupLabels() guards against.
//   A mapping that is off by one atom produces a perfectly plausible
//   population rather than an error, and would quietly connect the
//   wrong levels, so both are tried and the caller is told when
//   neither matches.
/////////////////////////////////////////////////////////////////////////////
//  The walk itself lives in MoAoBasis (tdat), shared with the MO
//  composition display; these only unpack the scene-graph fragment.
static bool fragmentSymbols(SGFragment *sgfrag, vector<string>& symbols,
                            vector<double>& coords)
{
  return MoAoBasis::fragmentGeometry(sgfrag, symbols, coords);
}

static bool functionsPerAtom(IPropCalculation *expt, SGFragment *sgfrag,
                             int coefficientWidth,
                             vector<int>& counts, vector<int>& shellOf,
                             bool *semiempirical = 0)
{
  vector<string> symbols;
  vector<double> coords;
  if (!fragmentSymbols(sgfrag, symbols, coords)) {
    counts.clear(); shellOf.clear();
    if (semiempirical != 0) *semiempirical = false;
    return false;
  }
  return MoAoBasis::functionCounts(expt, symbols, coefficientWidth, counts,
                                   shellOf, semiempirical);
}


/**
 * The real overlap matrix and per-atom function counts, for the
 * localisation and overlap-population checks classify() runs before
 * any construction-specific bonding rule (#132, 2026-09-27), in the
 * STORED frame the coefficients are in.
 */
static bool buildBasisOverlap(IPropCalculation *expt, SGFragment *sgfrag,
                              int coefficientWidth,
                              vector<int>& perAtom, vector<double>& Sflat,
                              std::function<void(double)> onRow = nullptr,
                              vector<EspBasisFunction> *basisOut = 0)
{
  vector<string> symbols;
  vector<double> coords;
  if (!fragmentSymbols(sgfrag, symbols, coords)) {
    perAtom.clear(); Sflat.clear();
    return false;
  }
  return MoAoBasis::overlapMatrix(expt, symbols, coords, coefficientWidth,
                                  perAtom, Sflat, onRow, basisOut);
}


/**
 * Drop a fragment column's levels that carry no correlation line into
 * the (already pi-filtered) molecular column, remapping the surviving
 * links' indices to match -- pi-only mode otherwise kept every sigma
 * TASO in the fragment columns and the whole H column even though
 * nothing in the molecular column pointed at any of them any more,
 * which is what dragged the energy axis out to the sigma system's
 * range and left two columns of unconnected rows on screen.
 */
static void trimUnconnectedFragment(MoColumn& column,
                                    vector<MoConnection>& links, bool isLeft)
{
  const size_t n = column.levels.size();
  vector<bool> used(n, false);
  for (size_t i = 0; i < links.size(); i++) {
    const int idx = isLeft ? links[i].leftLevel : links[i].rightLevel;
    if (idx >= 0 && (size_t)idx < n) used[idx] = true;
  }

  vector<int> remap(n, -1);
  vector<MoLevel> kept;
  for (size_t i = 0; i < n; i++) {
    if (used[i]) {
      remap[i] = (int)kept.size();
      kept.push_back(column.levels[i]);
    }
  }
  column.levels = kept;

  for (size_t i = 0; i < links.size(); i++) {
    int& idx = isLeft ? links[i].leftLevel : links[i].rightLevel;
    idx = (idx >= 0 && (size_t)idx < n) ? remap[idx] : -1;
  }
}


string MoDiagramPanel::levelText(int level)
{
  return p_canvas ? p_canvas->describeLevel(level) : string();
}


string MoDiagramPanel::calcLevelText(int n)
{
  if (!p_canvas) return string();
  return p_canvas->describeLevel(p_canvas->calcKey(n));
}


string MoDiagramPanel::orbitalComposition(const vector<int>& orbengIndices)
{
  IPropCalculation *calc = getCalculation();
  if (calc == 0) return "";
  set<PropertyPanel*> panels =
      PropertyPanel::getPanels(calc->getURL().toString(), "MOs");
  for (set<PropertyPanel*>::iterator it = panels.begin();
       it != panels.end(); ++it) {
    MoPanel *mo = dynamic_cast<MoPanel*>(*it);
    if (mo != 0) return mo->compositionText(orbengIndices);
  }
  return "";
}


void MoDiagramPanel::orbitalClicked(int orbengIndex)
{
  IPropCalculation *calc = getCalculation();
  if (calc == 0) return;

  set<PropertyPanel*> panels =
      PropertyPanel::getPanels(calc->getURL().toString(), "MOs");

  for (set<PropertyPanel*>::iterator it = panels.begin();
       it != panels.end(); ++it) {
    MoPanel *mo = dynamic_cast<MoPanel*>(*it);
    if (mo == 0) continue;

    if (mo->showOrbital(orbengIndex)) return;

    getFW().showMessage("That orbital cannot be computed: the MOs panel "
                        "has no basis set or no coefficients for it.",
                        false/*warning*/);
    return;
  }

  getFW().showMessage("Open the MOs panel (Properties menu) to show an "
                      "orbital from the diagram.", false/*warning*/);
}


//  The basis as the code writes it ("def2-SVP", not ECCE's key "def2-svp"),
//  from the same table the input writer translates with.
static string basisAsCodeWrites(const string& code, const string& name)
{
  string stem;
  for (size_t i = 0; i < code.size(); i++)
    if (isalnum((unsigned char)code[i])) stem += code[i];
  if (stem.empty()) return name;
  std::ifstream in((string(Ecce::ecceHome()) + "/scripts/parsers/wr" + stem +
                    "GBS.pm").c_str());
  //  A basis of several groups is named "def2-svp..." (the heaviest atom's, and more).
  string shown = name;
  if (shown.size() > 3 && shown.compare(shown.size() - 3, 3, "...") == 0) shown.erase(shown.size() - 3);
  string want = shown;
  for (size_t i = 0; i < want.size(); i++) want[i] = (char)tolower((unsigned char)want[i]);
  string line;
  while (std::getline(in, line)) {
    const size_t k = line.find("$NameToBasis{\"");
    if (k == string::npos) continue;
    const size_t a = k + 14, b = line.find('"', a);
    if (b == string::npos || line.compare(a, b - a, want) != 0) continue;
    const size_t c = line.find('"', line.find('=', b));
    const size_t d = c == string::npos ? c : line.find('"', c + 1);
    if (d != string::npos) return line.substr(c + 1, d - c - 1);
  }
  return shown;
}


//  Where the molecular levels come from, for the summary line: the fragment
//  levels say they are extended Hückel, so the middle column says what it is.
//  A DFT run is named by its functional, not by the theory ("BP86", not
//  "RDFT"); an unrestricted one is told by UKS.
static string levelSource(IPropCalculation *calc, bool slash = false)
{
  ICalculation *esCalc = dynamic_cast<ICalculation*>(calc);
  if (esCalc == 0) return string();
  string what;
  TTheory *theory = esCalc->theory();
  if (theory) {
    what = theory->toConciseString();
    if (theory->category() == "DFT") {
      GUIValues *params = esCalc->guiparams();
      GUIValue *value = params ? params->get("ES.Theory.DFT.XCFunctionals") : 0;
      string functional = value ? value->getValueAsString() : string();
      const size_t paren = functional.find(" (");
      if (paren != string::npos) functional.erase(paren);
      if (!functional.empty())
        what = (what.size() && what[0] == 'U' ? "UKS " : "") + functional;
      delete params;
    }
    delete theory;
  }
  TGBSConfig *config = esCalc->gbsConfig();
  if (config && !config->empty() && !config->name().empty()) {
    string code;
    if (TaskJob *tj = dynamic_cast<TaskJob*>(calc))
      if (const JCode *jc = tj->application()) code = jc->name();
    what += (what.empty() ? "" : slash ? "/" : ", ") + basisAsCodeWrites(code, config->name());
  }
  if (slash) return what;
  return what.empty() ? string() : "MO levels: " + what + " \xC2\xB7 ";
}

bool MoDiagramPanel::Create(IPropCalculation *calculation,
      wxWindow *parent, wxWindowID id, const wxPoint& pos,
      const wxSize& size, long style, const wxString& name)
{
  VizPropertyPanel::Create(calculation, parent, id, pos, size, style, name);

  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  p_canvas = new MoDiagramCanvas(this);
  p_canvas->setClickHandler(this);
  sizer->Add(p_canvas, 1, wxEXPAND|wxALL, 2);

  buildFragmentChooser(sizer);
  SetSizer(sizer);

  return true;
}


/////////////////////////////////////////////////////////////////////////////
// Description
//   The control that says which atoms are which fragment.
//
//   A LIST OF ORBITS, NOT OF ATOMS.  A fragment has to be a union of
//   symmetry-equivalent sets, because the group must map it onto
//   itself or it has no symmetry orbitals at all.  Offering the choice
//   this way means an impossible fragmentation cannot be expressed --
//   there is no way to tick half of an orbit.
//
//   Ferrocene comes out as three rows, {Fe} {10 C} {10 H}, so metal
//   against both rings is one tick.  Ethene as two CH2 units does not
//   appear at all, and cannot: each CH2 holds one carbon and two
//   hydrogens and the group maps one onto the other, so it is no orbit
//   of D2h.  That diagram needs a subgroup, which is a different
//   feature and not a different grouping.
/////////////////////////////////////////////////////////////////////////////
void MoDiagramPanel::buildFragmentChooser(wxSizer *sizer)
{
  //  Three rows that wrap rather than run off a docked window: what is
  //  drawn (pick, construction, About), how it is shown (check boxes),
  //  and the free choice of fragments.
  wxWrapSizer *row = new wxWrapSizer(wxHORIZONTAL);
  wxWrapSizer *views = new wxWrapSizer(wxHORIZONTAL);
  wxBoxSizer *fragRow = new wxBoxSizer(wxHORIZONTAL);
  //  Closed by default: choosing fragments is rare.  Not a
  //  wxCollapsiblePane (it resizes the docked frame), as in the machine
  //  register.
  p_fragToggle = new wxButton(this, wxID_ANY,
      wxString::FromUTF8("\xe2\x96\xb8 Fragments..."), wxDefaultPosition,
      wxDefaultSize, wxBU_EXACTFIT|wxBORDER_NONE);
  p_fragPanel = new wxPanel(this);
  p_fragPanel->Hide();
  p_fragPanel->SetSizer(fragRow);

  //  THE CLASS IS WHAT A CHEMIST WOULD WANT TO OVERRIDE.
  //
  //  There is no universal way to split a molecule: a complex is
  //  drawn one way, an AXn molecule another, ethene a third, and each
  //  is a CONVENTION that suits a class. The diagram detects the
  //  class and takes the construction that suits it -- which is
  //  right nearly always and is a judgement, not a fact, so it has to
  //  be possible to say otherwise. Overriding the CLASS is the useful
  //  form of that; regrouping individual atom sets, below, is the
  //  last resort beneath it.
  //
  //  An entry the molecule cannot support is refused with a reason
  //  rather than quietly replaced: someone asking for a ligand field
  //  diagram of ethene needs to be told ethene has no metal.
  //  THE ONE SEPARATION THAT IS NOT A CONVENTION.
  //
  //  For a planar molecule, pi orbitals are exactly those that change
  //  sign in the molecular plane -- a fact about the group, not a
  //  choice about how to draw it. Benzene's diagram IS the pi set:
  //  nobody draws its thirty levels, they draw the four-rung ladder,
  //  and this is what produces it.
  //
  //  Shown only where the point group names a distinguished plane,
  //  which is the Cnh and Dnh groups -- the aromatic rings. D2h calls
  //  all three of its mirrors "s" and C2v has two equivalent ones, so
  //  for ethene or water it would take the geometry to say which
  //  plane holds the atoms. Better to offer nothing there than to
  //  guess which plane was meant.
  p_piOnly = new ewxCheckBox(this, wxID_ANY, _("pi system only"));
  p_piOnly->SetValue(getenv("ECCE_MODIAGRAM_PIONLY") != 0);
  p_piOnly->Show(false);
  p_piOnly->SetToolTip("Show only the orbitals that change sign in the "
                       "molecular plane");
  views->Add(p_piOnly, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);

  //  FILLED PER MOLECULE, in build(), with the constructions that
  //  molecule can actually be drawn by.
  //
  //  Offering all of them and refusing most is not a choice, it is a
  //  list of disappointments: ethene has no metal, benzene has no two
  //  halves, water has no second chemical group. build() still
  //  refuses a construction that does not suit -- it must never hand
  //  back a different diagram than the one asked for -- but the user
  //  should not be able to ask.
  wxArrayString kinds;
  kinds.Add(_("Automatic"));
  p_construction = new ewxChoice(this, wxID_ANY, wxDefaultPosition,
                                 wxDefaultSize, kinds);
  p_construction->SetSelection(0);
  p_construction->SetToolTip("Which construction the diagram is built "
                             "by. Left automatic it takes the one that "
                             "suits this molecule.");
  //  PICK AN ATOM OR A BOND INSTEAD OF A CONSTRUCTION.  Filled per
  //  molecule in build(); both empty is Automatic.
  {
    wxArrayString none;
    none.Add(_("(automatic)"));
    p_pickFirst = new ewxChoice(this, wxID_ANY, wxDefaultPosition,
                                wxDefaultSize, none);
    p_pickFirst->SetSelection(0);
    p_pickFirst->SetToolTip("Pick the centre atom, or the first atom of a "
                            "bond, and the diagram is built around it. "
                            "Left automatic it takes the construction that "
                            "suits the molecule. A pick overrides Built as.");
    wxArrayString nothing;
    nothing.Add(wxString::FromUTF8("\xE2\x80\x94"));
    p_pickSecond = new ewxChoice(this, wxID_ANY, wxDefaultPosition,
                                 wxDefaultSize, nothing);
    p_pickSecond->SetSelection(0);
    p_pickSecond->Enable(false);
    p_pickSecond->SetToolTip("An atom bonded to the first: the bond is cut "
                             "and its two sides are drawn against each other. "
                             "\xE2\x80\x94 leaves the first atom as the centre.");
    row->Add(new ewxStaticText(this, wxID_ANY, _("Atom:")),
             0, wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    row->Add(p_pickFirst, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);
    row->Add(new ewxStaticText(this, wxID_ANY, _("Bond to:")),
             0, wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
    row->Add(p_pickSecond, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);
    p_compute = new wxButton(this, wxID_ANY, _("Compute"), wxDefaultPosition,
                             wxDefaultSize);
    p_compute->SetToolTip("Draw the diagram for the atom, or the bond, chosen "
                          "here. Nothing is drawn until this is pressed, so a "
                          "pick can be made in two steps.");
    row->Add(p_compute, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);
  }

  row->Add(new ewxStaticText(this, wxID_ANY, _("Built as:")),
           0, wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
  row->Add(p_construction, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);

  //  Next to the construction, where a narrow window does not push it off.
  p_about = new wxButton(this, wxID_ANY, _("About this diagram..."),
                         wxDefaultPosition, wxDefaultSize);
  p_about->SetToolTip("About this diagram: how it is made, how far to trust it, and "
                      "what is left out");
  row->Add(p_about, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);

  p_autoFragments = new ewxCheckBox(p_fragPanel, wxID_ANY, _("Choose fragments"));
  p_autoFragments->SetValue(false);
  //  AN OVERRIDE, NOT A MODE.
  //
  //  Left alone, the diagram works through the constructions in
  //  order -- a metal and its donors, a central atom and its
  //  neighbours, two equivalent halves -- and takes the first that
  //  suits the molecule, which is what a chemist would have drawn.
  //  This is for saying otherwise. It can only express groupings of
  //  equivalent atoms, so it cannot produce the two-halves diagram
  //  (the group swaps the halves, so neither is a set of equivalent
  //  atoms); untick it to get that one back.
  p_autoFragments->SetToolTip("Group the sets of equivalent atoms "
                              "manually, instead of the construction "
                              "the diagram chose");
  fragRow->Add(p_autoFragments, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);

  //  TWELVE LABELS AND TWELVE NUMBERS IS A LOT OF TEXT.
  //
  //  The numbers are the part a reader of a qualitative diagram needs
  //  least -- what it is for is which orbital lies above which, not by
  //  how much -- so they can be put away.  On by default, because the
  //  levels are nudged apart a few pixels for legibility and the
  //  printed value is what keeps the energy exact.
  p_showEnergies = new ewxCheckBox(this, wxID_ANY, _("Show energies"));
  p_showEnergies->SetValue(true);
  p_showEnergies->SetToolTip("Write each level's energy beside its label");
  views->Add(p_showEnergies, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);
  p_showDescription = new ewxCheckBox(this, wxID_ANY, _("Describe clicked level"));
  p_showDescription->SetValue(true);
  p_showDescription->SetToolTip("Write the clicked level's description under the diagram; "
                                "hovering a level still shows it");
  views->Add(p_showDescription, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);

  p_constructLabels = new ewxCheckBox(this, wxID_ANY, _("Construction labels"));
  p_constructLabels->SetValue(true);
  p_constructLabels->SetToolTip("Name each model level by how it is built: sigma, pi, * for out of phase, nb for non-bonding");
  views->Add(p_constructLabels, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);
  p_symmetryLabels = new ewxCheckBox(this, wxID_ANY, _("Symmetry labels"));
  p_symmetryLabels->SetValue(true);
  p_symmetryLabels->SetToolTip("Name each model level by its irreducible representation");
  views->Add(p_symmetryLabels, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);

  p_ligandField = new ewxCheckBox(this, wxID_ANY, _("Ligand-field view"));
  p_ligandField->SetValue(getenv("ECCE_MODIAGRAM_LFVIEW") != 0);
  p_ligandField->SetToolTip("Complexes: hide the empty ligand-centred levels "
                            "(such as CO pi*) above the d levels, leaving the "
                            "textbook picture. Transitions into the hidden "
                            "levels are charge transfer, not d-d");
  p_ligandField->Enable(false);

  //  The fragment-orbital diagram draws only what interacts with the
  //  metal; this puts back the ligand-internal and low-metal-share levels.
  p_showAll = new ewxCheckBox(this, wxID_ANY, _("Show all levels"));
  p_showAll->SetValue(getenv("ECCE_MODIAGRAM_SHOWALL") != 0);
  p_showAll->SetToolTip("Fragment-orbital diagram: also draw the "
                        "ligand-internal levels and the levels with little "
                        "metal character, and their lines. Central atom: "
                        "draw every level and line, including the non-bonding "
                        "ones the simplified view merges or leaves out");
  p_showAll->Enable(false);
  views->Add(p_showAll, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);

  //  SAY WHAT A FRAGMENT IS, ON THE PANEL.
  //
  //  "Choose fragments" with a list of atom counts beside it does not
  //  explain itself, and the explanation was in a tooltip, which is
  //  invisible until you already suspect there is something to learn.
  //  A diagram correlates a molecule against TWO groups of atoms, and
  //  the only thing being chosen here is which group each set goes in.
  p_fragmentHint = new ewxStaticText(p_fragPanel, wxID_ANY,
      _("Ticked sets become the left column, the rest the right"));
  fragRow->Add(p_fragmentHint, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);

  p_fragments = new wxCheckListBox(p_fragPanel, wxID_ANY, wxDefaultPosition,
                                   wxSize(220, 70));
  p_fragments->SetToolTip("Each row is a set of atoms the symmetry makes "
                          "equivalent. Ticked sets form the left column of "
                          "the diagram; the rest form the right.");
  fragRow->Add(p_fragments, 1, wxEXPAND|wxALL, 4);

  views->Add(p_ligandField, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);
  p_optToggle = new wxButton(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                             wxDefaultSize, wxBU_EXACTFIT);
  p_optToggle->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    setOptionsOpen(!p_optOpen);
  });
  p_optSizer = sizer;  p_optRow = row;  p_optViews = views;
  sizer->Add(p_optToggle, 0, wxALL, 2);
  sizer->Add(row, 0, wxEXPAND);
  sizer->Add(views, 0, wxEXPAND);
  sizer->Add(p_fragToggle, 0, wxALL, 2);
  sizer->Add(p_fragPanel, 0, wxEXPAND);
  p_fragToggle->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    setFragmentsOpen(!p_fragPanel->IsShown());
  });

  //  Bound on the controls themselves.  A static table does not reach
  //  this panel for a control with a pushed handler chain, which is
  //  what the ewx classes install -- the same reason #81's radio box
  //  never received its event.
  p_autoFragments->Bind(wxEVT_CHECKBOX, &MoDiagramPanel::onFragmentChanged,
                        this);
  p_construction->Bind(wxEVT_CHOICE, &MoDiagramPanel::onFragmentChanged,
                       this);
  p_pickFirst->Bind(wxEVT_CHOICE, &MoDiagramPanel::onPickFirstChanged, this);
  p_compute->Bind(wxEVT_BUTTON, &MoDiagramPanel::onCompute, this);
  p_piOnly->Bind(wxEVT_CHECKBOX, &MoDiagramPanel::onFragmentChanged, this);
  p_fragments->Bind(wxEVT_CHECKLISTBOX, &MoDiagramPanel::onFragmentChanged,
                    this);
  p_showEnergies->Bind(wxEVT_CHECKBOX, &MoDiagramPanel::onShowEnergies, this);
  p_showDescription->Bind(wxEVT_CHECKBOX, &MoDiagramPanel::onShowEnergies, this);
  p_constructLabels->Bind(wxEVT_CHECKBOX, &MoDiagramPanel::onShowEnergies, this);
  p_symmetryLabels->Bind(wxEVT_CHECKBOX, &MoDiagramPanel::onShowEnergies, this);
  p_showAll->Bind(wxEVT_CHECKBOX, &MoDiagramPanel::onFragmentChanged, this);
  p_ligandField->Bind(wxEVT_CHECKBOX, &MoDiagramPanel::onFragmentChanged, this);
  p_about->Bind(wxEVT_BUTTON, &MoDiagramPanel::onAbout, this);

  p_fragments->Enable(false);

  bool open = true;
  ewxConfig *cfg = ewxConfig::getConfig("wxbuilder.ini");
  if (cfg) cfg->Read("/MoDiagram/OptionsOpen", &open, true);
  setOptionsOpen(open);
}


//  The diagram gets the height of the controls when they are folded.
void MoDiagramPanel::setOptionsOpen(bool open)
{
  if (p_optSizer == 0) return;
  if (!open && p_optOpen) p_fragWasOpen = p_fragPanel->IsShown();
  p_optOpen = open;
  p_optSizer->Show(p_optRow, open, true);
  p_optSizer->Show(p_optViews, open, true);
  p_optSizer->Show(p_fragToggle, open);
  p_fragPanel->Show(open && p_fragWasOpen);
  p_optToggle->SetLabel(wxString::FromUTF8(
      open ? "\xe2\x96\xbe Options" : "\xe2\x96\xb8 Options"));
  ewxConfig *cfg = ewxConfig::getConfig("wxbuilder.ini");
  if (cfg) {
    cfg->Write("/MoDiagram/OptionsOpen", open);
    cfg->Flush();
  }
  Layout();
  if (GetParent() != 0) GetParent()->Layout();
}


void MoDiagramPanel::setFragmentsOpen(bool open)
{
  if (p_fragPanel == 0) return;
  p_fragPanel->Show(open && p_optOpen);
  p_fragWasOpen = open;
  p_fragToggle->SetLabel(wxString::FromUTF8(
      open ? "\xe2\x96\xbe Fragments..." : "\xe2\x96\xb8 Fragments..."));
  Layout();
  if (GetParent() != 0) GetParent()->Layout();
}


/** List the orbits, one row each, and show which side they are on. */
void MoDiagramPanel::fillFragmentChooser(const vector< vector<int> >& orbits,
                                         const vector<string>& elements)
{
  if (p_fragments == 0) return;

  const bool choosing = (p_autoFragments != 0 && p_autoFragments->GetValue());
  p_fragments->Enable(choosing);

  //  Only rebuild the rows when the molecule changed, or a rebuild
  //  triggered by a tick would throw the ticks away.
  if ((int)p_fragments->GetCount() != (int)orbits.size()) {
    p_fragments->Clear();
    for (size_t i = 0; i < orbits.size(); i++) {
      //  "10 C", "1 Fe" -- what the set is, and how many of it.
      ostringstream label;
      label << orbits[i].size() << " " << elements[orbits[i][0]];
      if (orbits[i].size() > 1) label << " (equivalent)";
      p_fragments->Append(wxString(label.str().c_str(), wxConvUTF8));
    }
    p_sideOfOrbit.clear();
  }

  if (!choosing) { p_sideOfOrbit.clear(); return; }

  p_sideOfOrbit.assign(orbits.size(), 1);
  for (size_t i = 0; i < orbits.size(); i++) {
    if (p_fragments->IsChecked((unsigned int)i)) p_sideOfOrbit[i] = 0;
  }
}


void MoDiagramPanel::onShowEnergies(wxCommandEvent& WXUNUSED(event))
{
  if (p_canvas == 0) return;
  p_canvas->setShowEnergies(p_showEnergies->IsChecked());
  p_canvas->setShowDescription(p_showDescription->IsChecked());
  p_canvas->setLabelSides(p_constructLabels->IsChecked(), p_symmetryLabels->IsChecked());
  //  The energies change how much room each row needs, so the whole
  //  layout is recomputed, not just repainted.
  p_canvas->Refresh();
}


/**
 * The pop-up: the canvas's about text, read-only and wrapped.  The text
 * carries **bold** headings and <sub> tags; the dialog shows the headings
 * bold and the subscripts as plain letters.
 */
void MoDiagramPanel::onAbout(wxCommandEvent& WXUNUSED(event))
{
  if (p_canvas == 0) return;
  string raw = p_canvas->aboutText();
  if (raw.empty()) raw = "There is nothing more to say about this diagram.";

  string plain;
  vector< std::pair<size_t,size_t> > bold;
  size_t open = string::npos;
  for (size_t i = 0; i < raw.size(); ) {
    if (raw.compare(i, 2, "**") == 0) {
      if (open == string::npos) open = plain.size();
      else { bold.push_back(std::make_pair(open, plain.size())); open = string::npos; }
      i += 2;
    } else if (raw.compare(i, 5, "<sub>") == 0) {
      i += 5;
    } else if (raw.compare(i, 6, "</sub>") == 0) {
      i += 6;
    } else {
      plain += raw[i++];
    }
  }

  wxDialog dlg(this, wxID_ANY, _("About this diagram"), wxDefaultPosition,
               wxSize(560, 520), wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxTextCtrl *text = new wxTextCtrl(&dlg, wxID_ANY, wxEmptyString,
      wxDefaultPosition, wxDefaultSize,
      wxTE_MULTILINE|wxTE_READONLY|wxTE_WORDWRAP|wxTE_RICH2|wxBORDER_NONE);
  text->SetValue(wxString::FromUTF8(plain.c_str()));
  //  Positions are in characters, not bytes.
  wxFont boldFont = text->GetFont();
  boldFont.SetWeight(wxFONTWEIGHT_BOLD);
  for (size_t k = 0; k < bold.size(); k++) {
    const long from = (long)wxString::FromUTF8(plain.substr(0, bold[k].first).c_str()).length();
    const long to = (long)wxString::FromUTF8(plain.substr(0, bold[k].second).c_str()).length();
    text->SetStyle(from, to, wxTextAttr(wxNullColour, wxNullColour, boldFont));
  }
  text->SetInsertionPoint(0);
  top->Add(text, 1, wxEXPAND|wxALL, 10);
  top->Add(dlg.CreateButtonSizer(wxOK), 0, wxEXPAND|wxALL, 8);
  dlg.SetSizer(top);
  dlg.ShowModal();
}


void MoDiagramPanel::onPickFirstChanged(wxCommandEvent& event)
{
  //  Only the second chooser follows; the diagram waits for Compute.
  refreshSecondChooser(p_pickFirst->GetSelection() - 1);
  event.Skip();
}


void MoDiagramPanel::onCompute(wxCommandEvent& event)
{
  const int first = p_pickFirst->GetSelection() - 1;
  const int sel = p_pickSecond->GetSelection();
  p_pickA = first >= 0 ? first : -1;
  p_pickB = (first >= 0 && sel > 0 && sel <= (int)p_secondAtoms.size())
            ? p_secondAtoms[sel - 1] : -1;
  onFragmentChanged(event);
}


int MoDiagramPanel::centreOverlaps() const
{
  return p_canvas != 0 ? p_canvas->centreOverlaps() : -1;
}


bool MoDiagramPanel::computePick(const string& first, const string& second)
{
  if (p_pickFirst == 0) return false;
  ensureInitialized();
  struct Find {
    static int label(const vector<string>& labels, const string& want) {
      for (size_t i = 0; i < labels.size(); i++)
        if (strcasecmp(labels[i].c_str(), want.c_str()) == 0) return (int)i;
      return -1;
    }
  };
  const int a = first.empty() ? -1 : Find::label(p_atomLabels, first);
  if (!first.empty() && a < 0) return false;
  p_pickFirst->SetSelection(a + 1);
  refreshSecondChooser(a);
  if (!second.empty()) {
    const int b = Find::label(p_atomLabels, second);
    int at = -1;
    for (size_t i = 0; i < p_secondAtoms.size(); i++)
      if (p_secondAtoms[i] == b) at = (int)i + 1;
    if (at < 0) return false;
    p_pickSecond->SetSelection(at);
  }
  wxCommandEvent ev(wxEVT_BUTTON, p_compute->GetId());
  ev.SetEventObject(p_compute);
  p_compute->GetEventHandler()->ProcessEvent(ev);
  return true;
}


void MoDiagramPanel::refreshSecondChooser(int first)
{
  vector<int> bonded;
  if (first >= 0 && first < (int)p_pickElements.size()) {
    vector< std::pair<int,int> > bonds;
    MoFragments::covalentBonds(p_pickCoords, p_pickElements, bonds);
    for (size_t k = 0; k < bonds.size(); k++) {
      if (bonds[k].first == first) bonded.push_back(bonds[k].second);
      else if (bonds[k].second == first) bonded.push_back(bonds[k].first);
    }
    std::sort(bonded.begin(), bonded.end());
  }
  int wasSecond = -1;
  {
    const int sel = p_pickSecond->GetSelection();
    if (sel > 0 && sel <= (int)p_secondAtoms.size())
      wasSecond = p_secondAtoms[sel - 1];
  }
  if (bonded != p_secondAtoms) {
    p_secondAtoms = bonded;
    p_pickSecond->Clear();
    p_pickSecond->Append(wxString::FromUTF8("\xE2\x80\x94"));
    int restore = 0;
    for (size_t i = 0; i < bonded.size(); i++) {
      p_pickSecond->Append(wxString(p_atomLabels[bonded[i]].c_str(), wxConvUTF8));
      if (bonded[i] == wasSecond) restore = (int)i + 1;
    }
    p_pickSecond->SetSelection(restore);
  }
  p_pickSecond->Enable(first >= 0 &&
      !(p_autoFragments != 0 && p_autoFragments->GetValue()));
}


/** Labels are the Builder's: element symbol and the atom's number from 1. */
void MoDiagramPanel::fillPickChoosers(const vector<double>& coords,
                                      const vector<string>& elements,
                                      int& first, int& second)
{
  first = second = -1;
  if (p_pickFirst == 0 || p_pickSecond == 0) return;

  vector<string> labels;
  for (size_t i = 0; i < elements.size(); i++) {
    ostringstream l;
    l << elements[i] << (i + 1);
    labels.push_back(l.str());
  }
  if (labels != p_atomLabels) {
    p_atomLabels = labels;
    p_pickFirst->Clear();
    p_pickFirst->Append(_("(automatic)"));
    for (size_t i = 0; i < labels.size(); i++)
      p_pickFirst->Append(wxString(labels[i].c_str(), wxConvUTF8));
    p_pickFirst->SetSelection(0);
    p_secondAtoms.clear();
    p_pickSecond->Clear();
    p_pickSecond->Append(wxString::FromUTF8("\xE2\x80\x94"));
    p_pickSecond->SetSelection(0);
    p_pickA = p_pickB = -1;
  }
  p_pickCoords = coords;
  p_pickElements = elements;

  //  ECCE_MODIAGRAM_PICK="O1" or "C1,C2" drives a pick in tests, once.
  if (!p_pickEnvDone) {
    p_pickEnvDone = true;
    if (const char *v = getenv("ECCE_MODIAGRAM_PICK")) {
      const string w(v);
      const size_t comma = w.find(',');
      const string envFirst = w.substr(0, comma);
      const string envSecond = comma != string::npos ? w.substr(comma + 1) : string();
      auto find = [&](const string& want) {
        for (size_t i = 0; i < labels.size(); i++)
          if (strcasecmp(labels[i].c_str(), want.c_str()) == 0) return (int)i;
        return -1;
      };
      const int a = envFirst.empty() ? -1 : find(envFirst);
      if (a >= 0) {
        p_pickFirst->SetSelection(a + 1);
        refreshSecondChooser(a);
        p_pickA = a;
        const int b = envSecond.empty() ? -1 : find(envSecond);
        for (size_t i = 0; i < p_secondAtoms.size(); i++)
          if (p_secondAtoms[i] == b) { p_pickSecond->SetSelection((int)i + 1); p_pickB = b; }
      }
    }
  }

  const bool chosen = p_autoFragments != 0 && p_autoFragments->GetValue();
  p_pickFirst->Enable(!chosen);
  p_pickSecond->Enable(!chosen && p_pickFirst->GetSelection() > 0 && !p_secondAtoms.empty());
  p_compute->Enable(!chosen);

  //  Choose fragments is the more explicit request, and wins.
  if (chosen) return;
  if (p_pickA >= 0 && p_pickA < (int)labels.size()) {
    first = p_pickA;
    if (p_pickB >= 0 && p_pickB < (int)labels.size()) second = p_pickB;
  }
}


void MoDiagramPanel::onFragmentChanged(wxCommandEvent& event)
{
  event.Skip();
  if (p_canvas == 0) return;

  //  Enable the list here, not only in fillFragmentChooser(): that
  //  runs inside build() and only once the orbit analysis has
  //  succeeded, so on a molecule where build() returned earlier the
  //  list stayed greyed out however the box was set.
  if (p_fragments != 0 && p_autoFragments != 0)
    p_fragments->Enable(p_autoFragments->GetValue());

  //  A DIAGRAM NEEDS TWO SIDES, SO DO NOT OFFER ONE.
  //
  //  Ticking every set, or none, leaves one column empty and the
  //  diagram cannot be built -- it came back as a bare spectrum with
  //  "Both sides of the diagram need at least one set of atoms", which
  //  reads as a failure rather than as a choice that was never
  //  available.  The tick is undone instead, so the control cannot be
  //  put into a state that has no answer.
  if (p_fragments != 0 && p_autoFragments != 0 &&
      p_autoFragments->GetValue()) {
    const unsigned int count = p_fragments->GetCount();
    unsigned int ticked = 0;
    for (unsigned int i = 0; i < count; i++) {
      if (p_fragments->IsChecked(i)) ticked++;
    }
    //  A side cannot be empty -- but REFUSING the click is not the way
    //  to prevent it, and refusing it froze the control completely.
    //
    //  With two sets, which is the common case ("2 C" and "4 H"),
    //  every single click from a working state passes through all-
    //  ticked or none-ticked. The old guard undid each one, so once
    //  the user had a valid split they could never change which set
    //  was on the left; the only way through was to untick "Choose
    //  fragments" and tick it again. Reported live 2026-09-25: "if 4H
    //  is picked I can't select 2C ... it also means that you can't
    //  CHANGE".
    //
    //  So move the set instead of rejecting the click. Ticking the
    //  last unticked row means "I want this one on the LEFT", and
    //  unticking the last ticked row means "I want this one on the
    //  RIGHT" -- both are what the user was reaching for, both keep
    //  each side occupied, and neither needs a warning.
    if (count > 1 && (ticked == 0 || ticked == count)) {
      const int changed = event.GetInt();
      if (changed < 0 || changed >= (int)count) return;

      const bool alone = (ticked == count);   // it was just ticked
      for (unsigned int i = 0; i < count; i++) {
        p_fragments->Check(i, alone ? (i == (unsigned int)changed)
                                    : (i != (unsigned int)changed));
      }
    }
  }

  build();
  p_canvas->Refresh();
}


bool MoDiagramPanel::prefersFloating() const
{
  return true;
}


wxSize MoDiagramPanel::preferredFloatingSize() const
{
  //  Three columns, each with a label, an annotation and a phase
  //  sketch beside its levels, and correlation lines between them.
  //  At 860 CCl4's chlorine set had its labels on top of one another
  //  and its sketches on top of those.  A window can always be made
  //  smaller; one that opens too small looks broken.
  return wxSize(1400, 950);
}


bool MoDiagramPanel::isRelevant(IPropCalculation *propCalc) const
{
  if (propCalc == 0) return false;
  //  The site setting wins over every user preference.
  if (Ecce::ecceHideMoDiagram()) return false;
  //  A code can opt out in its .edml (SupportsMODiagram false), e.g.
  //  ECCE-QM for labs where students draw the diagram; the user can opt
  //  back in under Preferences.  Orbital list, energies and pictures are
  //  unaffected.
  ICalculation *esCalc = dynamic_cast<ICalculation*>(propCalc);
  if (esCalc != 0) {
    const JCode *app = esCalc->application();
    //  Not for MOPAC for now (Andy, 2026-10-17): its orthonormal basis gives
    //  the fragment-orbital diagram nothing to project on, and the symmetry
    //  route alone is not worth offering for it yet.
    if (app != 0 && app->name() == "MOPAC") return false;
    if (app != 0 && !app->supportsMODiagram()) {
      Preferences prefs(PrefLabels::GLOBALPREFFILE);
      //  Must match GlobalPrefs' default for an unset preference.
      bool show = true;
      prefs.getBool(PrefLabels::ECCEQMMODIAGRAM, show);
      if (!show) return false;
    }
  }
  //  Energies are the one thing without which there is nothing to draw.
  //  Symmetry labels and fragment analysis are both optional: without
  //  them this degrades to a labelled level diagram, which is still
  //  worth having.
  return propCalc->getProperty("ORBENG") != 0;
}


void MoDiagramPanel::initialize()
{
  build();
}


namespace {

  /**
   * The subgroup name whose irreps cover a set of reported labels --
   * the SAME "smallest group whose irreps cover them" search the
   * existing #147 downgrade further down in build() runs, but wanted
   * here, BEFORE groupByIrrep(), to drive the independent cross-check.
   * Kept as its own small copy rather than factored out: the two
   * versions differ in exactly one respect (this one returns just the
   * name; the other also rewrites `group`/`why` for the whole diagram)
   * and sharing risked coupling changes to one into the other.
   */
  string smallestCoveringGroup(const vector<string>& reported)
  {
    set<string> canon;
    for (size_t i = 0; i < reported.size(); i++) {
      const string c = MoDiagram::canonicalIrrep(reported[i]);
      if (!c.empty()) canon.insert(c);
    }
    if (canon.empty()) return string();

    vector<string> all = CharacterTable::names();
    string best;
    int bestOrder = 0;
    for (size_t g = 0; g < all.size(); g++) {
      const CharacterTable *cand = CharacterTable::lookup(all[g]);
      if (cand == 0) continue;
      const vector<string>& theirs = cand->irreps();
      bool covers = true;
      for (set<string>::const_iterator it = canon.begin();
           covers && it != canon.end(); ++it) {
        bool found = false;
        for (size_t j = 0; j < theirs.size(); j++) {
          if (MoDiagram::canonicalIrrep(theirs[j]) == *it) found = true;
        }
        if (!found) covers = false;
      }
      if (!covers) continue;
      const int order = cand->order();
      if (best.empty() || order < bestOrder) { best = all[g]; bestOrder = order; }
    }
    return best;
  }

  /**
   * Try to replace a code's own ORBSYM with labels computed from the
   * orbitals themselves, in the FULL point group (#147/#132).
   *
   * Deliberately all-or-nothing and heavily defensive: anything this
   * needs that is missing, mismatched, or fails its own internal
   * checks (a shell the angular table cannot describe, a bad frame
   * fit, a subgroup cross-check that does not fully agree) makes this
   * return false and leave `computed`/`note` untouched -- the caller
   * then keeps the code's own labels and the existing #147 subgroup-
   * downgrade path exactly as it already runs.
   *
   * The independent evidence this insists on before replacing anything
   * is the SAME one tests/symmetry/testSubgroupCrossCheck.C uses: the
   * code's own abelian-subgroup labels (built from diagonal +/-1
   * operations in the STORED frame, no gensym/autosym involved) must
   * agree with what this computes for that subgroup, AND the full-
   * group label this assigns must subduce to it. Only both together
   * are treated as confirmation; the full-group computation is never
   * trusted on its own say-so. If the code reported no labels at all
   * there is nothing to cross-check against, so nothing is replaced --
   * the existing s/p-only orbitalIrrep() path already exists for that
   * more common case and is untouched by any of this.
   */
  //  ECCE_DEBUG_MOSYM=1 names the step at which the full-group labelling
  //  gave up.  Every exit below is otherwise silent by design -- the
  //  diagram falls back to the code's own labels -- which left a live
  //  failure with nothing to go on.
  //
  //  Written to stderr, but Builder is spawned by Organizer as a
  //  detached child process, so that stderr never reaches whatever
  //  terminal started `ecce` -- same gotcha the #83 material probes
  //  already ran into. ECCE_DEBUG_MOSYM_LOG=<path>, if set, additionally
  //  (or instead) writes to that file so this can actually be read --
  //  and setting the log path alone is enough to turn debug output on,
  //  with no separate ECCE_DEBUG_MOSYM needed.
  static FILE *mosymLog()
  {
    static FILE *log = NULL;
    static bool tried = false;
    if (!tried) {
      tried = true;
      const char *where = getenv("ECCE_DEBUG_MOSYM_LOG");
      if (where != 0) log = fopen(where, "a");
    }
    return log;
  }
  static bool mosymDebug()
  {
    return getenv("ECCE_DEBUG_MOSYM") != 0 || getenv("ECCE_DEBUG_MOSYM_LOG") != 0;
  }
  //  Every [MOSYM]/[MOLOC] print goes through here so stderr and the
  //  log file (when there is one) never drift apart.
  static void mosymPrintf(const char *fmt, ...)
  {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);
    FILE *log = mosymLog();
    if (log != 0) {
      va_start(ap, fmt);
      vfprintf(log, fmt, ap);
      va_end(ap);
      fflush(log);
    }
  }
  static string& lastMosymFail() { static string why; return why; }
  static bool mosymFail(int where, const char *why)
  {
    lastMosymFail() = why;
    if (mosymDebug()) {
      mosymPrintf("[MOSYM] full-group labels not computed (step %d): %s\n",
                  where, why);
    }
    return false;
  }



  //  LINEAR MOLECULES.  autosym has no infinite groups and stands C4v in
  //  for C-infinity-v and D4h for D-infinity-h.  Gaussian labels in the
  //  true groups (SG, PI, DLTA; SGG, PIU, DLTG ...), everything else in
  //  the stand-in, so the two could never be matched.  Everything is
  //  put in Gaussian's spelling, which the canvas draws as sigma, pi,
  //  delta.  a1 is sigma+, a2 sigma-, e pi, and b1 and b2 are the two
  //  halves of delta.
  bool isCollinear(const vector<double>& xyz)
  {
    const size_t n = xyz.size() / 3;
    if (n < 2) return false;
    double d[3] = { xyz[3]-xyz[0], xyz[4]-xyz[1], xyz[5]-xyz[2] };
    const double len = sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
    if (len < 1.0e-6) return false;
    for (int k = 0; k < 3; k++) d[k] /= len;
    for (size_t i = 2; i < n; i++) {
      double v[3] = { xyz[i*3]-xyz[0], xyz[i*3+1]-xyz[1], xyz[i*3+2]-xyz[2] };
      double c[3] = { v[1]*d[2]-v[2]*d[1], v[2]*d[0]-v[0]*d[2],
                      v[0]*d[1]-v[1]*d[0] };
      if (sqrt(c[0]*c[0] + c[1]*c[1] + c[2]*c[2]) > 0.05) return false;
    }
    return true;
  }

  //  The title's group: a linear molecule is C-inf-v or D-inf-h whatever
  //  finite stand-in the symmetry code worked in.
  string shownGroup(const string& group, const vector<double>& xyz)
  {
    string g = group;
    for (size_t i = 0; i < g.size(); i++) g[i] = toupper((unsigned char)g[i]);
    if (!isCollinear(xyz)) return group;
    if (g == "C4V" || g == "C2V") return "CINFV";
    if (g == "D4H" || g == "D2H") return "DINFH";
    return group;
  }

  //  A finite stand-in irrep (canonical spelling) in the linear group's
  //  spelling; anything already linear, or unknown, is returned as is.
  string linearName(const string& irrep)
  {
    static const char* const map[][2] = {
      {"A1","SG"}, {"A2","SG-"}, {"E","PI"}, {"B1","DLTA"}, {"B2","DLTA"},
      {"A1G","SGG"}, {"A2U","SGU"}, {"A2G","SGG-"}, {"A1U","SGU-"},
      {"EG","PIG"}, {"EU","PIU"},
      {"B1G","DLTG"}, {"B2G","DLTG"}, {"B1U","DLTU"}, {"B2U","DLTU"},
      {0,0} };
    for (int k = 0; map[k][0] != 0; k++) {
      if (irrep == map[k][0]) return map[k][1];
    }
    return irrep;
  }

  /**
   * Match each alpha orbital to the beta orbital it corresponds to, for
   * a merged open-shell level (#132: unrestricted calculations, e.g.
   * O2's triplet ground state).
   *
   * BY IRREP FIRST -- the k-th alpha orbital of a given irrep is matched
   * to the k-th beta orbital reporting the SAME irrep, both taken in
   * their own energy order (a PropVector's rows already run that way).
   * Falls back to matching by plain energy-ordered POSITION when either
   * side carries no usable labels of matching length, since that is the
   * best evidence left.
   */
  void matchAlphaBeta(const vector<string>& sAlphaIn, const vector<string>& sBetaIn,
                      int nAlpha, int nBeta, vector<int>& betaOfAlpha)
  {
    betaOfAlpha.assign(nAlpha, -1);
    const bool haveLabels = (int)sAlphaIn.size() == nAlpha &&
        (int)sBetaIn.size() == nBeta && !sAlphaIn.empty() && !sBetaIn.empty();
    //  An orbital the labeller gave up on in one spin only (CoCl4's deep
    //  Cl a1+t2 accident) would shift every later k-th match of that
    //  irrep, so it is left out of both.
    vector<string> sAlpha = sAlphaIn, sBeta = sBetaIn;
    if (haveLabels && nAlpha == nBeta) {
      for (int i = 0; i < nAlpha; i++) {
        if (sAlpha[i].empty() != sBeta[i].empty()) {
          sAlpha[i].clear();
          sBeta[i].clear();
        }
      }
    }
    if (haveLabels) {
      map<string, vector<int> > byIrrep;
      for (int i = 0; i < nBeta; i++) {
        const string c = MoDiagram::canonicalIrrep(sBeta[i]);
        if (!c.empty()) byIrrep[c].push_back(i);
      }
      map<string, size_t> next;
      for (int i = 0; i < nAlpha; i++) {
        const string c = MoDiagram::canonicalIrrep(sAlpha[i]);
        if (c.empty()) continue;
        map<string, vector<int> >::iterator it = byIrrep.find(c);
        if (it == byIrrep.end()) continue;
        size_t& n = next[c];
        if (n < it->second.size()) betaOfAlpha[i] = it->second[n++];
      }
      //  Labels in two different spellings share no irrep: by energy order.
      bool any = false;
      for (int i = 0; i < nAlpha; i++) if (betaOfAlpha[i] >= 0) any = true;
      if (!any)
        for (int i = 0; i < nAlpha && i < nBeta; i++) betaOfAlpha[i] = i;
    } else {
      for (int i = 0; i < nAlpha && i < nBeta; i++) betaOfAlpha[i] = i;
    }
  }

  /**
   * The energy window the diagram will actually draw for this
   * spectrum, computed the SAME WAY MoDiagramPanel::build() itself
   * does further down (MoDiagram::suggestCoreCutoff()/
   * suggestVirtualCutoff()) -- but on a plain energy grouping, with no
   * labels at all, so this can run BEFORE any labelling is attempted.
   * Both cutoff functions only look at each level's energy and
   * occupancy, never its label, so grouping by energy alone gives the
   * same cutoffs the real, labelled grouping will.
   */
  void drawnWindow(const vector<double>& energies, const vector<double>& occs,
                   double& low, double& high)
  {
    low = -1.0e30; high = 1.0e30;
    if (energies.empty()) return;
    vector<MoLevel> tmp;
    if (!MoDiagram::group(energies, occs, vector<string>(), 1.0e-4, tmp) ||
        tmp.empty()) return;
    low  = MoDiagram::suggestCoreCutoff(tmp);
    high = MoDiagram::suggestVirtualCutoff(tmp);
  }

  bool computeFullGroupLabels(IPropCalculation *calc,
                              const string& group,
                              const CharacterTable *table,
                              SGFragment *sgfrag,
                              const vector<double>& probeCoords,
                              const vector<string>& elements,
                              const vector<double>& e,
                              const vector<string>& reported,
                              vector<string>& computed,
                              string& note,
                              const char *moPropertyName = "MO",
                              double windowLow = -1.0e30,
                              double windowHigh = 1.0e30,
                              std::function<void(double)> onRow = nullptr,
                              bool trustComputed = false,
                              vector< vector<double> > *rotatedOut = 0,
                              const vector<string> *parentLabels = 0,
                              const string *parentGroup = 0,
                              const vector<double> *parentCoords = 0)
  {
    if (mosymDebug()) {
      mosymPrintf("[MOSYM] group %s, %d orbitals, %d reported labels\n",
                  group.c_str(), (int)e.size(), (int)reported.size());
    }
    if (calc == 0 || table == 0 || sgfrag == 0) return mosymFail(11, "calc == 0 || table == 0 || sgfrag == 0");
    //  NOTE: `reported.empty()` used to return here unconditionally.  A
    //  code that reports no labels at all now still gets computed ones
    //  (#132), just with a weaker acceptance test further down -- see
    //  `haveReported` -- since there is nothing of the code's own left
    //  to cross-check a computed label against.
    if (probeCoords.size() != elements.size()*3) return mosymFail(13, "probeCoords.size() != elements.size()*3");

    ICalculation *escalc = dynamic_cast<ICalculation*>(calc);
    if (escalc == 0) return mosymFail(16, "escalc == 0");

    TGBSConfig *config = escalc->gbsConfig();
    if (config == 0 || config->empty()) return mosymFail(19, "config == 0 || config->empty()");

    const JCode *code = escalc->application();
    if (code == 0) return mosymFail(22, "code == 0");


    PropTable *moCoefs = (PropTable*)calc->getProperty(moPropertyName);
    if (moCoefs == 0) return mosymFail(29, "moCoefs == 0");

    //  The STORED frame -- sgfrag's OWN coordinates, not the reoriented
    //  probe copy -- because the MO coefficients are in whatever frame
    //  the code's SCF actually ran in (see SymmetryAnalysis::alignFrames()'s
    //  header comment).
    vector<TAtm*> *atoms = sgfrag->atoms();
    double *xyz = sgfrag->coordinates();
    const unsigned long natoms = sgfrag->numAtoms();
    if (atoms == 0 || xyz == 0 || natoms == 0 || atoms->size() != natoms ||
        elements.size() != natoms) {
      delete atoms;
      return mosymFail(41, "if (atoms == 0 || xyz == 0 || natoms == 0 || atoms->size() != natoms ||");
    }
    vector<string> storedElements(natoms);
    vector<double> storedCoords(natoms*3);
    for (unsigned long a = 0; a < natoms; a++) {
      storedElements[a] = (*atoms)[a]->atomicSymbol();
      for (int k = 0; k < 3; k++) storedCoords[a*3+k] = xyz[a*3+k];
    }
    delete atoms;

    //  MOAOORDER-marked calcs (currently ORCA) store MO/MOBETA with
    //  columns in the PARSER's canonical order, not TGBSConfig's own --
    //  see MoAoOrder.H. Without this, #163's full-group labelling would
    //  be computed from scrambled coefficients for any such calc.
    //  unique_ptr frees the possible new copy on every return out of
    //  this function; never mutates the cached property itself.
    std::unique_ptr<PropTable> moCoefsOwned;
    {
      PropTable *reordered = MoAoOrder::reorderToNative(
          moCoefs, calc, storedElements, config);
      if (reordered != moCoefs) {
        moCoefsOwned.reset(reordered);
        moCoefs = reordered;
      }
    }

    //  THE COEFFICIENT TABLE'S WIDTH DECIDES CARTESIAN VS SPHERICAL.
    //  The calculation records the basis's own convention (Pople sets
    //  are Cartesian in ECCE's library), but a spherical-only code such
    //  as ORCA ran 6-31G* with five d functions: 22 columns for CH4
    //  against 23 from the recorded Cartesian.  Try the recorded system
    //  first and the other only when it does not reproduce the width.
    vector<EspBasisFunction> basis;
    TGBSAngFunc *angfunc = 0;
    {
      int lengthShellCart[7] = { 1, 3, 6, 10, 15, 21, 28 };
      int lengthShellSph[7]  = { 1, 3, 5, 7, 9, 11, 13 };
      const TGaussianBasisSet::CoordinateSystem recorded = config->coordsys();
      const TGaussianBasisSet::CoordinateSystem other =
          (recorded == TGaussianBasisSet::Spherical)
            ? TGaussianBasisSet::Cartesian : TGaussianBasisSet::Spherical;
      const TGaussianBasisSet::CoordinateSystem tries[2] = { recorded, other };
      for (int t = 0; t < 2 && angfunc == 0; t++) {
        TGBSAngFunc *candidate = code->getAngFunc(tries[t]);
        if (candidate == 0) continue;
        const bool sph = (tries[t] == TGaussianBasisSet::Spherical);
        vector<EspBasisFunction> trial;
        if (BasisFlatten::flatten(storedElements, storedCoords, config, code,
                                  candidate, candidate->maxShells(),
                                  sph ? lengthShellSph : lengthShellCart,
                                  trial) &&
            (int)trial.size() == moCoefs->columns()) {
          basis.swap(trial);
          angfunc = candidate;
          if (t == 1 && mosymDebug()) {
            mosymPrintf("[MOSYM] coefficient width matches the %s "
                        "basis, not the recorded one\n",
                        sph ? "spherical" : "Cartesian");
          }
        } else {
          delete candidate;
        }
      }
    }
    if (angfunc == 0)
      return mosymFail(58, "neither Cartesian nor spherical flattening "
                           "matches the MO coefficient width");

    //  A shell the angular table cannot describe leaves an EMPTY
    //  placeholder with no centre -- decline outright rather than risk
    //  misreading the atom boundaries below from it.
    for (size_t i = 0; i < basis.size(); i++) if (basis[i].empty()) return mosymFail(63, "for (size_t i = 0; i < basis.size(); i++) if (basis[i].empty())");

    //  Per-atom function counts, read back off the flattened basis's
    //  OWN atom-major walk (grouped by contiguous matching centre)
    //  rather than re-derived from the config -- one walk, trusted,
    //  not two that could disagree.
    vector<int> perAtom;
    {
      size_t i = 0;
      while (i < basis.size()) {
        size_t j = i;
        while (j < basis.size() &&
               fabs(basis[j].center[0]-basis[i].center[0]) < 1.0e-9 &&
               fabs(basis[j].center[1]-basis[i].center[1]) < 1.0e-9 &&
               fabs(basis[j].center[2]-basis[i].center[2]) < 1.0e-9) j++;
        perAtom.push_back((int)(j - i));
        i = j;
      }
    }
    if (perAtom.size() != natoms) return mosymFail(82, "perAtom.size() != natoms");

    const int nbasis = (int)basis.size();
    vector< vector<double> > S(nbasis, vector<double>(nbasis, 0.0));
    for (int i = 0; i < nbasis; i++) {
      for (int j = 0; j < nbasis; j++)
        S[i][j] = EspField::overlapOf(basis[i], basis[j]);
      if (onRow && nbasis > 0) onRow((i+1.0)/nbasis);
    }

    vector<int> shellTypeOf(nbasis);
    for (int i = 0; i < nbasis; i++) {
      shellTypeOf[i] = basis[i].powerX[0]+basis[i].powerY[0]+basis[i].powerZ[0];
    }

    //  The full group's own operations (gensym's standard frame,
    //  matching `probeCoords`) -- shelled out to symops exactly as
    //  every other structural tool in ECCE does.
    vector<SymOp> ops;
    if (!MoFragments::symmetryOperations(group, ops)) return mosymFail(99, "!MoFragments::symmetryOperations(group, ops)");

    vector< vector<int> > images;
    if (!SymmetryAnalysis::atomImages(probeCoords, elements, ops, 0.05, images))
      return mosymFail(103, "if (!SymmetryAnalysis::atomImages(probeCoords, elements, ops, 0.05, images))");

    vector< vector<int> > classes;
    SymmetryAnalysis::conjugacyClasses(ops, classes);
    vector<int> classOfOp;
    if (!SymmetryAnalysis::matchClasses(ops, classes, *table, classOfOp))
      return mosymFail(109, "if (!SymmetryAnalysis::matchClasses(ops, classes, *table, classOfOp))");

    SymOp Q; double rmsd;
    if (!SymmetryAnalysis::alignFrames(storedCoords, probeCoords, Q, rmsd))
      return mosymFail(113, "if (!SymmetryAnalysis::alignFrames(storedCoords, probeCoords, Q, rmsd))");
    if (rmsd > 0.1) return mosymFail(114, "rmsd > 0.1");   // frames do not actually agree

    vector<SymOp> opsInCoeffFrame(ops.size());
    for (size_t i = 0; i < ops.size(); i++)
      opsInCoeffFrame[i] = SymmetryAnalysis::conjugate(ops[i], Q);

    const int norb = moCoefs->rows();
    if (norb <= 0 || (int)e.size() != norb) return mosymFail(121, "norb <= 0 || (int)e.size() != norb");
    vector< vector<double> > orbitals(norb, vector<double>(nbasis));
    for (int m = 0; m < norb; m++)
      for (int mu = 0; mu < nbasis; mu++)
        orbitals[m][mu] = moCoefs->value(m, mu);

    vector<string> fullDerived;
    vector< vector<double> > rotated;
    const int fullLabelled = SymmetryAnalysis::fullLabelSpectrum(
        orbitals, e, perAtom, shellTypeOf, S, images, classOfOp,
        opsInCoeffFrame, angfunc, *table, 1.0e-4, fullDerived,
        trustComputed ? &rotated : 0, trustComputed);
    if (fullLabelled <= 0) return mosymFail(131, "fullLabelled <= 0");

    //  NO CODE LABELS TO CROSS-CHECK AGAINST (#132).
    //
    //  The subgroup cross-check below is the only independent evidence
    //  this function otherwise insists on, and it needs the code's own
    //  labels to exist at all.  Where they do not, accept the computed
    //  full-group labels on a weaker but still real condition instead:
    //  every orbital the diagram will actually DRAW must have been
    //  labelled -- not every orbital the calculation reports.
    //
    //  It used to insist on ALL of them (fullLabelled == norb), which
    //  refused O2's diagram outright: the high virtuals well above the
    //  drawn window decline (correctly -- they are basis-set artefacts
    //  the projection has no reason to resolve cleanly), and that one
    //  partial failure, far outside anything shown, sank the whole
    //  labelling. What actually has to hold is the same guarantee this
    //  comment always meant: nothing the reader can SEE is left
    //  unexplained. windowLow/windowHigh are the caller's own
    //  suggestCoreCutoff()/suggestVirtualCutoff() cutoffs -- the drawn
    //  valence window -- and default to "everything" for a caller that
    //  has not computed them, which keeps this exactly as strict as it
    //  used to be for anyone that does not pass them.
    //  A pick's subgroup that the code's own labels (in its abelian subgroup of
    //  the full group) cannot be a subgroup of: nothing to cross-check against.
    if (reported.empty() || trustComputed) {
      bool windowLabelled = true;
      for (int i = 0; i < norb && (windowLabelled || mosymDebug()); i++) {
        if (e[i] < windowLow || e[i] > windowHigh) continue;   // not drawn
        if (fullDerived[i].empty()) {
          windowLabelled = false;
          if (mosymDebug()) mosymPrintf("[MOSYM]   orbital %d (%.5f) not labelled\n", i + 1, e[i]);
        }
      }
      if (!windowLabelled)
        return mosymFail(132, "reported.empty() && a DRAWN orbital was not labelled");

      //  A pick's subgroup: each degenerate level of the validated parent group
      //  must have been split into exactly the irreps it subduces to.
      if (trustComputed && parentLabels != 0 && parentGroup != 0 && parentCoords != 0) {
        const CharacterTable *pt = CharacterTable::lookup(*parentGroup);
        vector<SymOp> pops;
        vector< vector<int> > pcls;
        vector<int> pClassOfOp;
        SymOp Qs; double r2;
        if (pt == 0 || !MoFragments::symmetryOperations(*parentGroup, pops))
          return mosymFail(133, "the parent group's operations are not available");
        SymmetryAnalysis::conjugacyClasses(pops, pcls);
        if (!SymmetryAnalysis::matchClasses(pops, pcls, *pt, pClassOfOp) ||
            !SymmetryAnalysis::alignFrames(*parentCoords, probeCoords, Qs, r2) || r2 > 0.1)
          return mosymFail(134, "the subgroup's frame cannot be tied to the parent group's");
        vector<int> toParent(ops.size(), -1);
        for (size_t k = 0; k < ops.size(); k++) {
          const SymOp w = SymmetryAnalysis::conjugate(ops[k], Qs);
          for (size_t o = 0; o < pops.size() && toParent[k] < 0; o++) {
            bool same = true;
            for (int a = 0; a < 3 && same; a++)
              for (int b = 0; b < 3 && same; b++)
                if (fabs(w.m[a][b] - pops[o].m[a][b]) > 2.0e-2) same = false;
            if (same) toParent[k] = pClassOfOp[o];
          }
          if (toParent[k] < 0) return mosymFail(135, "a subgroup operation is not one of the parent group's");
        }
        const vector<string>& subNames = table->irreps();
        for (int i = 0; i < norb; ) {
          int j = i;
          const string& P = (*parentLabels)[i];
          while (j + 1 < norb && (*parentLabels)[j + 1] == P && fabs(e[j + 1] - e[j]) < 1.5e-5) j++;
          const bool inWindow = e[i] >= windowLow && e[i] <= windowHigh;
          if (!P.empty() && inWindow) {
            const vector<double> *chiP = pt->characters(P);
            if (chiP == 0) return mosymFail(136, "the parent label has no characters");
            for (size_t x = 0; x < subNames.size(); x++) {
              const vector<double> *chiX = table->characters(subNames[x]);
              if (chiX == 0) return mosymFail(137, "a subgroup irrep has no characters");
              double sum = 0.0;
              for (size_t k = 0; k < ops.size(); k++) sum += (*chiP)[toParent[k]]*(*chiX)[classOfOp[k]];
              const int want = (int)floor(sum/ops.size() + 0.5)*table->dimension(subNames[x]);
              int have = 0;
              for (int m = i; m <= j; m++)
                if (fullDerived[m] == subNames[x]) have++;
              if (want != have) {
                ostringstream why;
                why << "the " << P << " level at " << e[i] << " Hartree gives " << have << " orbital(s) of "
                    << subNames[x] << " in " << group << " but subduces to " << want;
                lastMosymFail() = why.str();
                if (mosymDebug()) mosymPrintf("[MOSYM] %s\n", why.str().c_str());
                return false;
              }
            }
          }
          i = j + 1;
        }
      }

      computed.assign(e.size(), string());
      for (int i = 0; i < norb && i < (int)computed.size(); i++) {
        if (!fullDerived[i].empty()) computed[i] = fullDerived[i];
      }
      if (trustComputed && rotatedOut != 0) *rotatedOut = rotated;
      ostringstream text;
      text << "Symmetry labels computed from the orbitals in " << group
           << (trustComputed ? (parentLabels != 0
                   ? "; every level is the subduction of its label in the parent group, which the code's own labels validated."
                   : "; the code's own labels belong to a different subgroup.")
                             : "; the code reported none.");
      note = text.str();
      return true;
    }

    //  THE INDEPENDENT CHECK: the code's own reported labels, verified
    //  against operations built directly in the stored frame (no
    //  gensym/autosym on this side at all -- see
    //  SymmetryAnalysis::subgroupLabelSpectrum()'s header comment).
    const string subgroupName = smallestCoveringGroup(reported);
    if (subgroupName.empty() || subgroupName == group) return mosymFail(138, "subgroupName.empty() || subgroupName == group");
    const CharacterTable *subgroupTable = CharacterTable::lookup(subgroupName);
    if (subgroupTable == 0) return mosymFail(140, "subgroupTable == 0");

    vector<string> subDerived;
    string axisNote;
    const int subLabelled = SymmetryAnalysis::subgroupLabelSpectrum(
        orbitals, reported, perAtom, shellTypeOf, S, storedElements,
        storedCoords, angfunc, *subgroupTable, 1.0e-4, 1.0e-4, subDerived,
        axisNote);
    if (subLabelled <= 0) return mosymFail(148, "subLabelled <= 0");

    int agree = 0, checked = 0;
    for (int i = 0; i < norb; i++) {
      if (subDerived[i].empty() || i >= (int)reported.size() ||
          reported[i].empty()) continue;
      checked++;
      //  canonicalIrrep(), not toupper(): the codes write a double prime
      //  as A" and the tables as A'', so every A'' orbital of a Cs-
      //  labelled run used to count as a disagreement.
      if (MoDiagram::canonicalIrrep(subDerived[i]) ==
          MoDiagram::canonicalIrrep(reported[i])) agree++;
      else if (mosymDebug())
        mosymPrintf("[MOSYM]   orbital %d (%.4f): code %s, computed %s\n",
                    i + 1, e[i], reported[i].c_str(), subDerived[i].c_str());
    }
    //  ALL OR NOTHING: a single disagreement means the axis convention
    //  or the frame premise is wrong somewhere, and a diagram is not
    //  the place to find out which -- keep the code's own labels.
    if (mosymDebug()) {
      mosymPrintf("[MOSYM] subgroup %s cross-check: %d of %d agree\n",
                  subgroupName.c_str(), agree, checked);
    }
    if (checked == 0 || agree != checked) return mosymFail(163, "checked == 0 || agree != checked");

    //  SUBDUCTION, independent of the cross-check just done: every
    //  full-group label fullLabelSpectrum() assigned must itself
    //  restrict, via its own characters, to include the (now verified)
    //  subgroup label for that same orbital -- otherwise the full-
    //  group computation and the subgroup one agree with the code by
    //  coincidence, not because the full-group frame/operations are
    //  actually right for this run.  Only D2/D2H subgroups (what
    //  subgroupLabelSpectrum() builds operations for) are checked this
    //  way; the axis permutation it settled on is recovered from
    //  axisNote rather than threaded through another parameter.
    {
      static const int PERMS[6][3] = { {0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0} };
      int perm[3] = { 0, 1, 2 };
      bool permFound = false;
      for (int p = 0; p < 6 && !permFound; p++) {
        const char *ax[3] = {
          PERMS[p][0]==0?"z":PERMS[p][0]==1?"y":"x",
          PERMS[p][1]==0?"z":PERMS[p][1]==1?"y":"x",
          PERMS[p][2]==0?"z":PERMS[p][2]==1?"y":"x" };
        if (axisNote.find(string("(1st table C2)=") + ax[0]) != string::npos &&
            axisNote.find(string("(2nd)=") + ax[1]) != string::npos) {
          perm[0] = PERMS[p][0]; perm[1] = PERMS[p][1]; perm[2] = PERMS[p][2];
          permFound = true;
        }
      }

      //  The subgroup's operations are the ones its labels were
      //  computed with (SymmetryAnalysis::subgroupOperations()), each
      //  classified by the same matchClasses() call, so an op is tied
      //  to its table column by class rather than by position.  This
      //  used to be a hand-built copy that knew only D2 and D2h: for a
      //  C2v subgroup (ORCA's for every C-infinity-v molecule, CO) it
      //  built D2's three C2 axes, none of which C4v contains, and
      //  failed every time.
      const int subOrder = subgroupTable->order();
      vector<SymOp> subPhys;
      vector<int> subClassOfOp;
      bool subductionOk = permFound &&
          SymmetryAnalysis::subgroupOperations(subgroupName, perm, subPhys) &&
          (int)subPhys.size() == subOrder;
      if (subductionOk) {
        vector< vector<int> > subClasses;
        SymmetryAnalysis::conjugacyClasses(subPhys, subClasses);
        subductionOk = SymmetryAnalysis::matchClasses(subPhys, subClasses,
                                                      *subgroupTable,
                                                      subClassOfOp) &&
                       subgroupTable->counts().size() == (size_t)subOrder;
      }
      vector<int> subClassToFullClass(subOrder, -1);
      if (subductionOk) {
        for (int k = 0; k < subOrder; k++) {
          const SymOp& want = subPhys[k];
          int foundClass = -1;
          for (size_t o = 0; o < opsInCoeffFrame.size() && foundClass < 0; o++) {
            bool same = true;
            for (int a = 0; a < 3 && same; a++)
              for (int b = 0; b < 3 && same; b++)
                if (fabs(opsInCoeffFrame[o].m[a][b]-want.m[a][b]) > 1.0e-3) same = false;
            if (same) foundClass = classOfOp[o];
          }
          const int sc = subClassOfOp[k];
          if (foundClass < 0 || sc < 0 || sc >= subOrder) { subductionOk = false; break; }
          subClassToFullClass[sc] = foundClass;
        }
      }

      if (subductionOk) {
        const vector<string>& subNames = subgroupTable->irreps();
        for (int i = 0; i < norb && subductionOk; i++) {
          if (fullDerived[i].empty() || subDerived[i].empty()) continue;
          const vector<double> *chiFull = table->characters(fullDerived[i]);
          if (chiFull == 0) { subductionOk = false; break; }

          double bestMult = -1;
          for (size_t x = 0; x < subNames.size(); x++) {
            const vector<double> *chiSub = subgroupTable->characters(subNames[x]);
            if (chiSub == 0 || (int)chiSub->size() != subOrder) continue;
            double sum = 0.0;
            for (int k = 0; k < subOrder; k++)
              sum += (*chiFull)[subClassToFullClass[k]] * (*chiSub)[k];
            string a = subNames[x], b = subDerived[i];
            for (size_t c=0;c<a.size();c++) a[c]=toupper((unsigned char)a[c]);
            for (size_t c=0;c<b.size();c++) b[c]=toupper((unsigned char)b[c]);
            if (a == b) bestMult = sum/subOrder;
          }
          if (bestMult <= 0.5) subductionOk = false;
        }
      }
      if (!subductionOk) return mosymFail(252, "!subductionOk");
    }

    computed.assign(e.size(), string());
    for (int i = 0; i < norb && i < (int)computed.size(); i++) {
      if (!fullDerived[i].empty()) computed[i] = fullDerived[i];
    }

    ostringstream text;
    text << "Symmetry labels computed from the orbitals in " << group
         << " (cross-checked against the code's own " << subgroupName
         << " labels, which agreed on all " << checked << " orbitals "
            "both sides could label, and every computed label subduces "
            "to match).";
    note = text.str();
    return true;
  }

}


void MoDiagramPanel::refresh()
{
  build();
}


/**
 * RAII lifetime for the "building..." progress dialog. Created empty;
 * progress() only actually pops up a dialog once 300ms have elapsed,
 * so a fast build (water, a small organic) never flashes one. Every
 * return from buildOnce() destroys this stack-local, which Destroy()s
 * the dialog if one was ever created -- no explicit cleanup needed at
 * each of buildOnce()'s many early returns.
 */
namespace {
class MoDiagramProgress
{
  public:
    explicit MoDiagramProgress(wxWindow *parent)
      : p_parent(parent), p_dlg(0)
    {
    }

    ~MoDiagramProgress()
    {
      if (p_dlg != 0) p_dlg->Destroy();
    }

    void progress(int percent, const wxString& stage)
    {
      if (p_dlg == 0) {
        if (p_watch.Time() < 300) return;
        p_dlg = new ewxProgressDialog(_("Building MO Diagram"), stage, 100,
            p_parent, wxPD_AUTO_HIDE|wxPD_APP_MODAL|wxPD_SMOOTH);
        p_dlg->Show();
      }
      p_dlg->Update(percent, stage);
    }

  private:
    wxWindow *p_parent;
    ewxProgressDialog *p_dlg;
    wxStopWatch p_watch;
};
}  // namespace


/**
 * ECCE_MODIAGRAM_DUMP=<path> oracle for #171: a deterministic text
 * record of buildOnce()'s finished model, written at every return point
 * that calls setDiagram().  Plain types only (no wx) -- this is meant
 * to move verbatim into the wx-free library Stage 1 extracts.  Kept
 * free-standing rather than a member so it can't reach into panel
 * state by accident.
 */
namespace {

//  The "MO levels: ..." text of the summary line, for the dump (test oracle).
string dumpLevelSource;

string escapeDumpText(const string& s)
{
  string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); i++) {
    const char c = s[i];
    if (c == '\n') out += "\\n";
    else if (c == '\t') out += "\\t";
    else out += c;
  }
  return out;
}

/** The text under the diagram in its three tiers (see FoDiagram::Result). */
struct NoteTiers
{
  string summary;
  vector<string> warnings;
  string about;
  string verdict;          // model against calculation (model-first diagram)
  bool verdictOk;
  NoteTiers() : verdictOk(true) {}
};

/** What is shown under the diagram, as one text: summary then warnings. */
string shownNotes(const NoteTiers& t)
{
  string out = t.summary;
  for (size_t i = 0; i < t.warnings.size(); i++) out += "\n" + t.warnings[i];
  return out;
}

/**
 * The About text of a diagram that is not the fragment-orbital one:
 * one paragraph per subject, each present only when there is something
 * to say.  `lead` is how the construction drew the molecule.
 */
string aboutText(const string& lead, const string& fragments,
                 const string& beta, const string& unlabelled,
                 const string& symmetry, const string& symmetrised,
                 const MoColumn& centre, const MoColumn& left,
                 const MoColumn& right)
{
  string out = "**What is drawn.** ";
  if (!lead.empty()) out += lead + " ";
  out += "The molecular levels in the middle are the calculation's own "
         "orbital energies, in Hartree.";
  if (!beta.empty()) out += " " + beta;
  if (!fragments.empty()) out += "\n\n**Fragment levels.** " + fragments;

  string labels;
  const string parts[3] = { unlabelled, symmetry, symmetrised };
  for (int k = 0; k < 3; k++) {
    if (parts[k].empty()) continue;
    labels += (labels.empty() ? "" : " ") + parts[k];
  }
  if (!labels.empty()) out += "\n\n**Symmetry.** " + labels;

  bool sketch = false;
  for (size_t i = 0; i < left.levels.size(); i++)
    if (!left.levels[i].phases.empty()) sketch = true;
  for (size_t i = 0; i < right.levels.size(); i++)
    if (!right.levels[i].phases.empty()) sketch = true;
  if (sketch)
    out += "\n\n**The circles beside each symmetry orbital** show the atoms as "
           "seen from one side, filled and open for opposite sign and sized by "
           "weight; a bar is a p lobe in the page.";

  const string core = FoDiagram::coreSentence(centre);
  if (!core.empty())
    out += "\n\n**Not shown.** " + core.substr(string("Also not shown: ").size());
  return out;
}

const char* levelCharacterName(MoLevel::Character c)
{
  switch (c) {
    case MoLevel::BONDING:     return "BONDING";
    case MoLevel::NONBONDING:  return "NONBONDING";
    case MoLevel::ANTIBONDING: return "ANTIBONDING";
    case MoLevel::MIXED:       return "MIXED";
    default:                   return "UNKNOWN";
  }
}

void dumpColumn(std::ofstream& out, const string& name, const MoColumn& col,
    const vector<double> *localisedShare, const vector<double> *centreOP,
    const vector<double> *mullikenMax, const vector<double> *normVals,
    const vector<double> *metalLigandOP = 0)
{
  out << "column " << name << "\n";
  out << "  title: " << escapeDumpText(col.title) << "\n";
  out << "  fromHalves: " << (col.fromHalves ? 1 : 0) << "\n";
  out << "  hiddenCount: " << col.hiddenCount << "\n";
  out << "  hiddenMaxEnergy: " << std::fixed << std::setprecision(6)
      << col.hiddenMaxEnergy << "\n";
  out << "  hiddenSemicoreCount: " << col.hiddenSemicoreCount << "\n";
  out << "  hiddenAboveCount: " << col.hiddenAboveCount << "\n";
  out << "  shellKeys:";
  for (size_t i = 0; i < col.shellKeys.size(); i++) out << " " << col.shellKeys[i];
  out << "\n";
  for (size_t i = 0; i < col.bands.size(); i++)
    out << "  band " << i << " low=" << std::fixed << std::setprecision(6)
        << col.bands[i].low << " high=" << col.bands[i].high
        << " levels=" << col.bands[i].levels
        << " label=" << escapeDumpText(col.bands[i].label) << "\n";
  out << "  levels: " << col.levels.size() << "\n";
  out << std::fixed;
  for (size_t i = 0; i < col.calcOnly.size(); i++)
    out << "  calconly " << i << " energy=" << std::setprecision(6) << col.calcOnly[i].calcEnergy
        << " label=" << escapeDumpText(col.calcOnly[i].label)
        << " occupancy=" << std::setprecision(4) << col.calcOnly[i].calcOccupancy << "\n";
  for (size_t i = 0; i < col.levels.size(); i++) {
    const MoLevel& lv = col.levels[i];
    out << "  level " << i
        << " energy=" << std::setprecision(6) << lv.energy
        << " tabulated=" << lv.tabulated
        << " irrep=" << (lv.irrep.empty() ? "-" : lv.irrep)
        << " label=" << escapeDumpText(lv.label.empty() ? "-" : lv.label)
        << (lv.tag.empty() ? string() : " tag=" + escapeDumpText(lv.tag))
        << (lv.symLabel.empty() ? string() : " sym=" + escapeDumpText(lv.symLabel))
        << (lv.model ? " construct=" + escapeDumpText(lv.construct.empty() ? "-" : lv.construct) : string())
        << " character=" << levelCharacterName(lv.character)
        << " pairing=" << lv.pairing
        << " occupancy=" << std::setprecision(4) << lv.occupancy
        << " occupancyBeta=" << lv.occupancyBeta
        << " degeneracy=" << lv.degeneracy
        << " shell=" << lv.shell
        << " slot=" << lv.slot
        << " shareLeft=" << lv.shareLeft
        << " shareRight=" << lv.shareRight;
    if (localisedShare != 0 && i < localisedShare->size())
      out << " lowdinMax=" << (*localisedShare)[i];
    if (centreOP != 0 && i < centreOP->size())
      out << " OP=" << (*centreOP)[i];
    if (mullikenMax != 0 && i < mullikenMax->size())
      out << " mullikenMax=" << (*mullikenMax)[i];
    if (normVals != 0 && i < normVals->size())
      out << " norm=" << (*normVals)[i];
    if (lv.hasCalc)
      out << " calcEnergy=" << std::setprecision(6) << lv.calcEnergy
          << " calcOverlap=" << std::setprecision(4) << lv.calcOverlap
          << " calcLabel=" << escapeDumpText(lv.calcLabel.empty() ? "-" : lv.calcLabel)
          << " calcOcc=" << std::setprecision(4) << lv.calcOccupancy
          << " calcOccBeta=" << lv.calcOccupancyBeta
          << " calcDeg=" << lv.calcDegeneracy;
    //  SKELETON only (#183): metal-ligand overlap population, empty
    //  and so absent for every other construction/fixture.
    if (metalLigandOP != 0 && i < metalLigandOP->size())
      out << " OPml=" << (*metalLigandOP)[i];
    out << " orbitals=[";
    for (size_t k = 0; k < lv.orbitals.size(); k++) {
      if (k > 0) out << ",";
      out << lv.orbitals[k];
    }
    out << "]";
    out << " annotation=\"" << escapeDumpText(lv.annotation) << "\"";
    if (!lv.detail.empty())
      out << " detail=\"" << escapeDumpText(lv.detail) << "\"";
    if (!lv.phases.empty()) {
      out << " phases=[";
      for (size_t k = 0; k < lv.phases.size(); k++)
        out << (k ? "," : "") << std::setprecision(4) << lv.phases[k];
      out << "] phaseAtoms=[";
      for (size_t k = 0; k < lv.phaseAtoms.size(); k++)
        out << (k ? "," : "") << lv.phaseAtoms[k];
      out << "]";
    }
    out << "\n";
  }
  if (!col.sketchX.empty()) {
    out << "  sketch view=" << col.sketchView << " normal=" << col.sketchNormal << " points=[";
    for (size_t k = 0; k < col.sketchX.size(); k++)
      out << (k ? ";" : "") << std::setprecision(4) << col.sketchX[k] << ","
          << col.sketchY[k] << "," << (k < col.sketchZ.size() ? col.sketchZ[k] : 0.0);
    out << "]\n";
  }
}

void dumpMoDiagram(const string& path, const string& group,
    const string& formula, const string& construction,
    const string& headline, const NoteTiers& tiers,
    const MoColumn& left, const MoColumn& centre, const MoColumn& right,
    const vector<MoConnection>& links,
    const vector<double>& localisedShare, const vector<double>& centreOP,
    const vector<double>& mullikenMax, const vector<double>& normVals,
    const vector<double>& metalLigandOP = vector<double>())
{
  std::ofstream out(path.c_str());
  if (!out) return;
  out << "group: " << (group.empty() ? "-" : group) << "\n";
  out << "formula: " << (formula.empty() ? "-" : formula) << "\n";
  out << "construction: " << (construction.empty() ? "-" : construction) << "\n";
  out << "headline: " << escapeDumpText(headline) << "\n";
  if (!dumpLevelSource.empty()) out << "source: " << escapeDumpText(dumpLevelSource) << "\n";
  out << "note: " << escapeDumpText(shownNotes(tiers)) << "\n";
  out << "about: " << escapeDumpText(tiers.about) << "\n";
  if (!tiers.verdict.empty())
    out << "verdict: " << (tiers.verdictOk ? "ok " : "differs ") << escapeDumpText(tiers.verdict) << "\n";
  dumpColumn(out, "left", left, 0, 0, 0, 0);
  dumpColumn(out, "centre", centre,
             &localisedShare, &centreOP, &mullikenMax, &normVals, &metalLigandOP);
  dumpColumn(out, "right", right, 0, 0, 0, 0);
  out << "links: " << links.size() << "\n";
  for (size_t i = 0; i < links.size(); i++) {
    out << "  link " << i << " left=" << links[i].leftLevel
        << " centre=" << links[i].centreLevel
        << " right=" << links[i].rightLevel;
    if (links[i].weight >= 0.0)
      out << " weight=" << std::setprecision(4) << links[i].weight;
    out << "\n";
  }
}

const char* ligandFieldCharacterName(MoLigandField::Level::Character c)
{
  switch (c) {
    case MoLigandField::Level::BONDING:     return "BONDING";
    case MoLigandField::Level::NONBONDING:  return "NONBONDING";
    case MoLigandField::Level::ANTIBONDING: return "ANTIBONDING";
    default:                                return "UNKNOWN";
  }
}

void dumpLigandFieldColumn(std::ofstream& out, const string& name,
    const vector<MoLigandField::Level>& col)
{
  out << "  column " << name << ": " << col.size() << "\n";
  for (size_t i = 0; i < col.size(); i++) {
    const MoLigandField::Level& lv = col[i];
    out << "    level " << i
        << " label=" << escapeDumpText(lv.label)
        << " energy=" << std::setprecision(6) << lv.energy
        << " degeneracy=" << lv.degeneracy
        << " character=" << ligandFieldCharacterName(lv.character)
        << " pairing=" << lv.pairing
        << " metalWeight=" << std::setprecision(4) << lv.metalWeight
        << " ligandWeight=" << lv.ligandWeight
        << " capture=" << lv.capture
        << " energyDeviation=" << lv.energyDeviation
        << " canonicalMOs=[";
    for (size_t k = 0; k < lv.canonicalMOs.size(); k++) {
      if (k > 0) out << ",";
      out << lv.canonicalMOs[k] << ":" << lv.canonicalOverlap[k];
    }
    out << "]\n";
  }
}

/**
 * ECCE_MODIAGRAM_DUMP's SKELETON-only appended section (#162 follow-up,
 * the fragment-orbital ligand-field model): opened in APPEND mode, so
 * this runs strictly after dumpMoDiagram() has already written the
 * ordinary sections above -- every other fixture's golden stays byte-
 * identical because nothing calls this for them (MoLigandField::build()
 * itself declines with metalAtomIndex < 0, but this is gated on the
 * construction too, so the intent is not left to that alone).
 */
void dumpLigandFieldModel(const string& path, const MoLigandField::Result& r)
{
  std::ofstream out(path.c_str(), std::ios::app);
  if (!out) return;
  out << "model:\n";
  out << "  ok: " << (r.ok ? 1 : 0) << "\n";
  out << "  note: " << escapeDumpText(r.note) << "\n";
  out << "  method: " << escapeDumpText(r.method) << "\n";
  out << "  energyScaleNote: " << escapeDumpText(r.energyScaleNote) << "\n";
  out << "  ligandAsymmetry: " << std::setprecision(6) << r.ligandAsymmetry << "\n";
  out << "  couplingThreshold: " << r.couplingThreshold << "\n";
  out << "  thresholdWhy: " << escapeDumpText(r.thresholdWhy) << "\n";
  out << "  deltaO: " << r.deltaO << "\n";
  const char *verdictName = r.verdict == MoLigandField::Result::REPRESENTATIVE
      ? "REPRESENTATIVE"
      : r.verdict == MoLigandField::Result::APPROXIMATE ? "APPROXIMATE"
                                                         : "NOT_REPRESENTATIVE";
  out << "  verdict: " << verdictName << "\n";
  for (size_t i = 0; i < r.verdictReasons.size(); i++)
    out << "  verdictReason: " << escapeDumpText(r.verdictReasons[i]) << "\n";
  for (size_t i = 0; i < r.disagreements.size(); i++)
    out << "  disagreement: " << escapeDumpText(r.disagreements[i]) << "\n";

  dumpLigandFieldColumn(out, "metal", r.metal);
  dumpLigandFieldColumn(out, "interaction", r.interaction);
  dumpLigandFieldColumn(out, "ligandInteracting", r.ligandInteracting);
  dumpLigandFieldColumn(out, "ligandInternal", r.ligandInternal);

  out << "  links: " << r.links.size() << "\n";
  for (size_t i = 0; i < r.links.size(); i++) {
    out << "    link " << i << " metal=" << r.links[i].metalIndex
        << " model=" << r.links[i].modelIndex
        << " ligand=" << r.links[i].ligandIndex
        << " weight=" << std::setprecision(4) << r.links[i].weight << "\n";
  }

  out << "  couplings: " << r.couplings.size() << "\n";
  for (size_t i = 0; i < r.couplings.size(); i++) {
    out << "    coupling " << i << " label=" << escapeDumpText(r.couplings[i].label)
        << " energy=" << std::setprecision(6) << r.couplings[i].energy
        << " maxCoupling=" << r.couplings[i].maxCoupling
        << " interacting=" << (r.couplings[i].interacting ? 1 : 0) << "\n";
  }

  out << "  robustness: " << r.robustness.size() << "\n";
  for (size_t i = 0; i < r.robustness.size(); i++) {
    const MoLigandField::RobustnessPoint& p = r.robustness[i];
    out << "    scan scale=" << p.scale
        << " piAcceptorPattern=" << (p.piAcceptorPattern ? 1 : 0)
        << " deltaOSign=" << (p.deltaOSign ? 1 : 0)
        << " eg2AboveT2g=" << (p.eg2AboveT2g ? 1 : 0) << "\n";
    for (size_t k = 0; k < p.characterChanges.size(); k++)
      out << "      characterChange: " << escapeDumpText(p.characterChanges[k]) << "\n";
  }
}

/**
 * ECCE_EXIT_AFTER_DUMP=1 (#171): close the app once the dump this build
 * asked for has actually been written, so a capture script has a
 * deterministic end rather than a settle timer.  Deferred via
 * CallAfter() -- this runs from inside initialize()/build(), itself
 * called from Show() from deep inside AUI pane bookkeeping, and closing
 * the top window synchronously from there is exactly the kind of
 * reentrancy CLAUDE.md warns about for wx3.2/GTK3.
 *
 * ECCE_EXIT_AFTER_DUMP_DELAY_MS (#183, optional): a screenshot tool
 * needs the window to stay painted for a moment after the dump is
 * written, not close on the very next idle turn -- a wxTimer, not a
 * sleep here, since sleeping on the main thread would freeze the event
 * loop that is supposed to be painting the window in the meantime.
 */
void exitAfterDumpIfRequested()
{
  if (getenv("ECCE_EXIT_AFTER_DUMP") == 0) return;
  int delayMs = 0;
  if (const char *d = getenv("ECCE_EXIT_AFTER_DUMP_DELAY_MS")) delayMs = atoi(d);
  if (delayMs > 0) {
    //  Deliberately never deleted: the process exits moments after this
    //  fires, and freeing a timer from inside its own event handler is
    //  its own hazard for no benefit here.
    wxTimer *timer = new wxTimer();
    timer->Bind(wxEVT_TIMER, [](wxTimerEvent&) {
      wxWindow *top = wxTheApp->GetTopWindow();
      if (top != 0) top->Close(true);
    });
    timer->StartOnce(delayMs);
    return;
  }
  wxTheApp->CallAfter([]() {
    wxWindow *top = wxTheApp->GetTopWindow();
    if (top != 0) top->Close(true);
  });
}

}  // namespace


static string pickLabelOfMetal(const vector<string>& elements, int atom)
{
  if (atom < 0 || atom >= (int)elements.size()) return string();
  return elements[atom] + std::to_string(atom + 1);
}


//  ECCE_MODIAGRAM_CONSTRUCTION=skeleton|central|halves|groups|sets|fragorb
//  asks for one construction as the Built-as menu would (tests/modiagram).
static MoFragments::Fragmentation constructionFromEnv()
{
  const char *v = getenv("ECCE_MODIAGRAM_CONSTRUCTION");
  if (v == 0) return MoFragments::NOT_BUILT;
  const string w(v);
  if (w == "skeleton") return MoFragments::SKELETON;
  if (w == "central")  return MoFragments::CENTRAL;
  if (w == "halves")   return MoFragments::HALVES;
  if (w == "groups")   return MoFragments::GROUPS;
  if (w == "sets")     return MoFragments::EQUIVALENT_SETS;
  if (w == "fragorb")  return MoFragments::FRAGMENT_ORBITALS;
  return MoFragments::NOT_BUILT;
}


void MoDiagramPanel::build()
{
  //  Re-entrancy guard (#170): ewxProgressDialog::Update() below pumps
  //  the event loop, so a resize or a control the user clicks mid-build
  //  can call build() again before the first call returns. A re-entrant
  //  call just asks for one more pass once the in-flight one finishes,
  //  rather than two calls racing on p_canvas.
  if (p_building) {
    p_rebuildPending = true;
    return;
  }
  p_building = true;
  do {
    p_rebuildPending = false;
    buildOnce();
  } while (p_rebuildPending);
  p_building = false;
}


void MoDiagramPanel::buildOnce()
{
  if (p_canvas == 0) return;

  MoDiagramProgress progressDlg(wxGetTopLevelParent(this));

  IPropCalculation *calc = getCalculation();
  if (calc == 0) return;

  PropVector    *energies = (PropVector*)calc->getProperty("ORBENG");
  PropVector    *occs     = (PropVector*)calc->getProperty("ORBOCC");
  PropVecString *syms     = (PropVecString*)calc->getProperty("ORBSYM");

  MoColumn left, centre, right;
  vector<MoConnection> links;

  //  Stage 0 oracle (#171): read once, used at every return point below.
  //  The diagnostic vectors are declared here (empty until the fragment
  //  block near the end fills them) so every dump call site can pass
  //  the same names regardless of how early it returns.
  const char *dumpPathEnv = getenv("ECCE_MODIAGRAM_DUMP");
  const string dumpPath = dumpPathEnv != 0 ? string(dumpPathEnv) : string();
  dumpLevelSource = dumpPath.empty() ? string() : levelSource(calc);
  vector<double> localisedShare, centreOP, mullikenMaxVec, normVec;
  //  SKELETON only (#183): the metal's own Lowdin share of each centre
  //  level, and its Mulliken overlap population against every ligand
  //  atom (not the molecule's covalent bonds, which for a carbonyl are
  //  mostly C-O and would swamp the metal's own interaction).  Empty
  //  for every other construction, so classify()/connect() run exactly
  //  as before for them.
  vector<double> metalShare, metalLigandOP;

  if (energies == 0) {
    const string note = "This calculation has no orbital energies.";
    p_canvas->setDiagram(left, centre, right, links, false, note);
    NoteTiers noEnergies;
    noEnergies.summary = note;
    if (!dumpPath.empty()) {
      dumpMoDiagram(dumpPath, "", "",
          MoFragments::fragmentationName(p_fragmentation), "", noEnergies,
          left, centre, right, links,
          localisedShare, centreOP, mullikenMaxVec, normVec);
      exitAfterDumpIfRequested();
    }
    return;
  }

  vector<double> e, o;
  vector<string> s;
  for (int i = 0; i < energies->rows(); i++) e.push_back(energies->value(i));
  if (occs != 0 && occs->rows() == energies->rows()) {
    for (int i = 0; i < occs->rows(); i++) o.push_back(occs->value(i));
  }
  //  Only used when it lines up.  MoPanel::fillTable() reverses these
  //  vectors independently and indexes them by the same row, which is
  //  how a length mismatch mislabels every row rather than running out
  //  at the end; the same care is worth taking here.
  if (syms != 0 && syms->rows() == energies->rows()) {
    for (int i = 0; i < syms->rows(); i++) s.push_back(syms->value(i));
  }

  //  --- beta orbitals, for a merged open-shell level (#132) -----------
  //
  //  An unrestricted calculation (O2's triplet ground state is the live
  //  case) reports ORBOCC as the ALPHA occupation alone, so without this
  //  every level came out singly occupied whether or not it actually
  //  was.  Read here, matched to the alpha orbitals below (once the
  //  molecule's point group is known, since matching is by irrep first),
  //  and folded into each drawn level as MoLevel::occupancyBeta.
  PropVector    *energiesBeta = (PropVector*)calc->getProperty("ORBENGBETA");
  PropVector    *occsBeta     = (PropVector*)calc->getProperty("ORBOCCBETA");
  PropVecString *symsBeta     = (PropVecString*)calc->getProperty("ORBSYMBETA");

  vector<double> eB, oB;
  vector<string> sB;
  const bool haveBeta = (energiesBeta != 0 && occsBeta != 0 &&
      occsBeta->rows() == energiesBeta->rows());
  if (haveBeta) {
    for (int i = 0; i < energiesBeta->rows(); i++)
      eB.push_back(energiesBeta->value(i));
    for (int i = 0; i < occsBeta->rows(); i++)
      oB.push_back(occsBeta->value(i));
    if (symsBeta != 0 && symsBeta->rows() == energiesBeta->rows()) {
      for (int i = 0; i < symsBeta->rows(); i++) sB.push_back(symsBeta->value(i));
    }
  }

  //  --- the molecule's symmetry --------------------------------------
  //
  //  Needed BEFORE the orbitals are grouped, because the grouping uses
  //  the irreps' dimensions.
  //
  //  THE ANALYSIS RUNS ON THE SYMMETRISED GEOMETRY, not the one on
  //  screen.  gensym's operation matrices are written in each group's
  //  standard frame, so atoms only map onto atoms if the molecule is in
  //  that frame.  An optimised structure is neither aligned to it nor
  //  exactly symmetric, and SymmetryOps::find() fixes both: autosym
  //  writes the cleaned, reoriented coordinates back onto the fragment
  //  it was given, which is why it takes a non-const reference.
  //
  //  On a COPY, because that fragment is the one the viewer is
  //  displaying, and nudging every atom of it is not something opening
  //  a diagram should do.  Reading the coordinates back off the copy is
  //  then not an extra step, it is the point.
  //
  //  0.01 Angstrom, which is what the Symmetry panel's own field
  //  defaults to.  The threshold does NOT behave the way it reads: a
  //  LOOSER one finds LOWER symmetry.  Measured against autosym
  //  directly, on exact water and exact methane:
  //
  //      threshold   H2O      CH4
  //      0           C1       C1
  //      0.001       C2v      Td
  //      0.01        C2v      Td
  //      0.05        Cs       Td
  //
  //  0.05 was the first value tried here and it quietly cost water its
  //  C2 axis -- a correlation diagram in Cs instead of C2v, with no
  //  error and no way to tell from the picture.
  string group, why;
  //  Whether `why` says something went wrong (a warning under the diagram) or
  //  only how the diagram was drawn (the About text).
  bool whyWarn = true;
  vector<double> coords;
  vector<string> elements;
  //  Set below from coordinationSkeleton(), central atom first (#183):
  //  the SKELETON construction's metal-ligand classification needs to
  //  know which atom the metal is before fragments are even built, to
  //  protect its antibonding virtuals from the core/valence cutoffs
  //  below and to compute the metal-ligand overlap population later.
  int metalAtomIndex = -1;

  WxVizToolFW& fw = getFW();
  SGFragment *sgfrag = fw.getSceneGraph().getFragment();

  progressDlg.progress(5, _("Finding the point group"));

  if (sgfrag == 0 || sgfrag->numAtoms() == 0) {
    why = "No structure is loaded, so there is no symmetry to use.";
  } else {
    Fragment probe(*sgfrag);
    try {
      group = SymmetryOps::find(probe, 0.01);
    } catch (...) {
      group.clear();
      why = "The symmetry search could not be run.";
    }

    //  A COMPLEX IS CLASSIFIED BY ITS SKELETON.
    //
    //  Hexammine cobalt's donor set is a perfect octahedron and the
    //  complex is not, because six ammonia rotors cannot all be: a
    //  whole-molecule search returns C1 and the diagram comes out
    //  with no symmetry at all.  What a ligand field diagram
    //  classifies is the metal and the atoms bonded to it, which is
    //  why the course draws the thing in Oh.  So where there is a
    //  skeleton, the symmetry search runs on that.
    {
      vector<double> allCoords;
      vector<string> allElements;
      double *probeXyz = probe.coordinates();
      if (probeXyz != 0) {
        for (unsigned long a = 0; a < probe.numAtoms(); a++) {
          TAtm *atom = probe.atomRef((int)a);
          if (atom == 0) { allElements.clear(); break; }
          allElements.push_back(atom->atomicSymbol());
          for (int k = 0; k < 3; k++) allCoords.push_back(probeXyz[3*a + k]);
        }
      }

      vector<int> skeleton;
      if (!allElements.empty() &&
          MoFragments::coordinationSkeleton(allCoords, allElements,
                                            skeleton)) {
        Fragment bare;
        for (size_t k = 0; k < skeleton.size(); k++) {
          const int a = skeleton[k];
          const double xyz[3] = { allCoords[3*a], allCoords[3*a + 1],
                                  allCoords[3*a + 2] };
          bare.addAtom(allElements[a], xyz);
        }
        try {
          const string skeletonGroup = SymmetryOps::find(bare, 0.01);
          if (!skeletonGroup.empty()) group = skeletonGroup;
        } catch (...) {
          //  Keep the whole-molecule answer; it is no worse than it was.
        }
        //  skeleton[0] is the central atom, in coordinationSkeleton()'s
        //  own contract -- and it indexes allElements/allCoords, which
        //  this loop built in exactly probe's atom order, the same
        //  order `elements`/`coords` below are about to be built in.
        if (!skeleton.empty()) metalAtomIndex = skeleton[0];
      }
    }

    double *xyz = probe.coordinates();
    if (xyz != 0) {
      for (unsigned long a = 0; a < probe.numAtoms(); a++) {
        TAtm *atom = probe.atomRef((int)a);
        if (atom == 0) { elements.clear(); coords.clear(); break; }
        elements.push_back(atom->atomicSymbol());
        for (int k = 0; k < 3; k++) coords.push_back(xyz[a*3 + k]);
      }
    }
  }

  //  The atom pick (empty = Automatic).  A metal in it asks for the
  //  fragment-orbital diagram, anything else for the cut or the centre.
  int pickA = -1, pickB = -1;
  bool pickMetal = false;
  fillPickChoosers(coords, elements, pickA, pickB);
  if (pickA >= 0 && pickA < (int)elements.size()) {
    double eV;
    //  A metal alone asks for the fragment-orbital diagram; a bond from a
    //  metal (Co-Cl) cuts that bond like any other.
    pickMetal = pickB < 0 && MoFragments::valenceEnergy(elements[pickA], 2, eV);
  } else {
    pickA = pickB = -1;
  }
  const bool pickActive = (pickA >= 0);
  const MoFragments::Pick pickSel(pickA, pickB);
  //  The metal of the pick, for the summary line.
  int pickMetalAtom = -1;
  if (pickMetal) {
    double eV;
    pickMetalAtom = MoFragments::valenceEnergy(elements[pickA], 2, eV)
                    ? pickA : pickB;
  }
  const string pickLabelA = pickA >= 0 ? elements[pickA] + std::to_string(pickA + 1) : string();
  const string pickLabelB = pickB >= 0 ? elements[pickB] + std::to_string(pickB + 1) : string();

  //  autosym can still return a group with no table here (C7, C8, D8h
  //  and the like); without one nothing at all was drawn.  Fall back to
  //  drawing without symmetry.
  //  What the diagram did to the symmetry, said on it.  Kept apart from
  //  `why`, which MoFragments::build() overwrites.
  string symmetryNote, symmetryWarning;
  if (!group.empty() && CharacterTable::lookup(group) == 0) {
    symmetryWarning = "There is no character table for " + group + " here, "
                      "so the diagram is drawn without symmetry (C1).";
    group = "C1";
  }

  //  MULLIKEN'S AXES for a planar C2v or D2h molecule (J. Chem. Phys. 23,
  //  1997 (1955)): the molecule lies in the yz plane, x is perpendicular to
  //  it, and in D2h z is the in-plane axis the atoms extend furthest along
  //  (C=C in ethene).  autosym leaves the plane where it likes, which made
  //  water's HOMO 1b2.  The model columns and the calculation's labels both
  //  take their frame from `coords`, so they cannot disagree.
  if (group == "C2v" || group == "C2V" || group == "D2h" || group == "D2H") {
    const size_t n = coords.size() / 3;
    double ext[3] = { 0, 0, 0 };
    for (size_t a = 0; a < n; a++)
      for (int k = 0; k < 3; k++) ext[k] = std::max(ext[k], fabs(coords[3*a + k]));
    int normal = -1;
    for (int k = 0; k < 3; k++) if (ext[k] < 0.01) normal = k;
    if (normal >= 0) {
      int inPlane[2], m = 0;
      for (int k = 0; k < 3; k++) if (k != normal) inPlane[m++] = k;
      int zAxis, yAxis;
      if (group == "C2v" || group == "C2V") {
        //  The principal axis is the in-plane axis the atoms are not
        //  symmetric about: autosym puts C2 along z.
        zAxis = 2;
        if (normal == 2) zAxis = -1;
        yAxis = zAxis >= 0 ? (inPlane[0] == zAxis ? inPlane[1] : inPlane[0]) : -1;
      } else {
        zAxis = ext[inPlane[0]] >= ext[inPlane[1]] ? inPlane[0] : inPlane[1];
        yAxis = zAxis == inPlane[0] ? inPlane[1] : inPlane[0];
      }
      if (zAxis >= 0 && yAxis >= 0) {
        //  new x = old normal, new y = old yAxis, new z = old zAxis; a
        //  sign keeps the frame right-handed.
        static const int cyc[3][3] = { {0,1,2}, {1,2,0}, {2,0,1} };
        bool even = false;
        for (int c = 0; c < 3; c++)
          if (cyc[c][0] == normal && cyc[c][1] == yAxis && cyc[c][2] == zAxis) even = true;
        for (size_t a = 0; a < n; a++) {
          const double o[3] = { coords[3*a], coords[3*a+1], coords[3*a+2] };
          coords[3*a]   = even ? o[normal] : -o[normal];
          coords[3*a+1] = o[yAxis];
          coords[3*a+2] = o[zAxis];
        }
      }
    }
  }

  //  autosym accepts a group within its tolerance but leaves the atoms
  //  only that close to symmetric, while the fragment analysis matches
  //  atoms to 0.001 Angstrom: a code's own geometry, symmetric to 1e-6
  //  in a subgroup and to 1e-3 in this group, then drew no fragments.
  //  Symmetrising this copy changes nothing the calculation computed.
  string symmetrisedNote, symmetrisedGroup;
  if (!coords.empty() && !group.empty() && group != "C1") {
    vector<SymOp> ops;
    double shift = 0.0;
    if (MoFragments::symmetryOperations(group, ops) &&
        SymmetryAnalysis::symmetrise(coords, elements, ops, 0.05, &shift) &&
        shift >= 5.0e-4) {
      symmetrisedGroup = group;
      ostringstream text;
      text.setf(std::ios::fixed);
      text.precision(3);
      text << "The structure was symmetrised to " << group
           << " for this diagram (atoms moved by up to " << shift
           << " Å); the orbitals are the calculation's own.";
      symmetrisedNote = text.str();
    }
  }

  //  A bond that the molecule's own symmetry does not keep (one Co-Cl of CoCl4
  //  in Td, one Cr-C of Cr(CO)6 in Oh): the diagram is drawn, and the orbitals
  //  labelled, in the largest subgroup that keeps the two sides.
  string subgroupNote, subgroupWhy;
  vector<double> parentCoords;
  string fullGroup;
  if (pickActive && pickB >= 0 && !group.empty() && group != "C1" &&
      !coords.empty()) {
    vector<int> sideA;
    string bondNote;
    if (MoFragments::bondSides(coords, elements, pickA, pickB, sideA, bondNote)) {
      string sub, why2;
      vector<double> subCoords;
      if (MoFragments::subgroupKeeping(coords, elements, group, sideA, sub,
                                       subCoords, why2)) {
        if (sub != group) {
          fullGroup = group;
          subgroupNote = "Cutting " + pickLabelA + "\xE2\x80\x93" + pickLabelB +
              " does not keep the symmetry of the molecule (" + group + "); the "
              "orbitals are labelled in " + sub + ", the largest subgroup that "
              "keeps the two sides.";
          group = sub;
          parentCoords = coords;
          coords = subCoords;
        }
      } else {
        subgroupWhy = why2;
      }
    }
  }

  //  --- the molecular orbitals ---------------------------------------
  //
  //  Grouped by irrep DIMENSION where the group is known.  A degenerate
  //  set is degenerate because its irrep is, not because two energies
  //  came out close: methane from a semiempirical run spreads its 1t2
  //  over 2e-4 Hartree, which any tolerance tight enough to separate
  //  real levels splits into a doublet and a singlet.
  map<string,int> dimensions;
  const CharacterTable *table = CharacterTable::lookup(group);
  if (table != 0) {
    const vector<string>& irreps = table->irreps();
    for (size_t i = 0; i < irreps.size(); i++) {
      dimensions[MoDiagram::canonicalIrrep(irreps[i])] =
          table->dimension(irreps[i]);
    }
  }

  //  #147/#132: labelling in the FULL point group, from the orbitals
  //  themselves, rather than trusting a code that symmetry-adapts only
  //  in an abelian subgroup (ORCA's UseSym, some G16 jobs). Replaces
  //  `s` ONLY when the independent cross-check below fully agrees;
  //  otherwise `s` is untouched and the existing subgroup-downgrade
  //  path further down runs exactly as it already did.
  bool labelsInFullGroup = false;
  vector< vector<double> > rotatedAlpha;      // a subgroup's orbitals, split by irrep
  double windowLow = -1.0e30, windowHigh = 1.0e30;
  {
    progressDlg.progress(20, _("Assigning full-group symmetry labels"));
    drawnWindow(e, o, windowLow, windowHigh);
    vector<string> computedLabels;
    string computedNote;
    wxStopWatch throttle;
    //  The pick's subgroup is checked against the parent group's labels, which
    //  are validated against the code's own as usual.
    vector<string> parentLabels;
    bool parentOk = fullGroup.empty();
    if (!fullGroup.empty()) {
      string parentNote;
      parentOk = computeFullGroupLabels(calc, fullGroup, CharacterTable::lookup(fullGroup), sgfrag,
                                        parentCoords, elements, e, s, parentLabels, parentNote, "MO",
                                        windowLow, windowHigh);
      if (!parentOk)
        subgroupWhy = "The orbitals could not be labelled in " + fullGroup + " (" + lastMosymFail() +
                      "), so the labels in " + group + " cannot be checked against it.";
    }
    if (parentOk && computeFullGroupLabels(calc, group, table, sgfrag, coords, elements,
                               e, s, computedLabels, computedNote, "MO",
                               windowLow, windowHigh,
                               [&](double frac) {
                                 if (throttle.Time() < 100) return;
                                 throttle.Start();
                                 progressDlg.progress(20 + (int)(frac*20),
                                     _("Assigning full-group symmetry labels"));
                               }, !fullGroup.empty(), &rotatedAlpha,
                               fullGroup.empty() ? 0 : &parentLabels,
                               fullGroup.empty() ? 0 : &fullGroup,
                               fullGroup.empty() ? 0 : &parentCoords)) {
      s = computedLabels;
      labelsInFullGroup = true;
      symmetryNote += (symmetryNote.empty() ? "" : "  ") + computedNote;
    } else if (parentOk && !fullGroup.empty()) {
      subgroupWhy = "The labels in " + group + " could not be verified against " + fullGroup + ": " +
                    lastMosymFail() + ".";
    }
  }

  if (!s.empty() && (group == "C2v" || group == "C2V" || group == "D2h" || group == "D2H"))
    symmetryNote += (symmetryNote.empty() ? "" : "  ") +
        string("b1 and b2 (b2 and b3 in D2h) depend on the choice of axes; "
               "ECCE follows Mulliken's convention, as most textbooks do.");

  //  THE SAME, FOR BETA -- computed only when the code did not report
  //  ORBSYMBETA itself, since that is what the alpha-beta irrep match
  //  above needs to prefer irrep over energy order (#132).
  //  Also when the code reported beta labels but the alpha ones were replaced
  //  by full-group labels: ORCA's D2 names for beta would match none of them.
  if (haveBeta && (sB.empty() || labelsInFullGroup)) {
    progressDlg.progress(40, _("Assigning full-group symmetry labels (beta)"));
    double windowLowB, windowHighB;
    drawnWindow(eB, oB, windowLowB, windowHighB);
    //  The same orbitals as the alpha window, by index: spin-polarised
    //  energies differ, and a window cut at different orbitals labels
    //  different numbers of each irrep, so the k-th alpha and beta orbital
    //  of an irrep no longer correspond (CoCl4's t2).
    if (windowLow > -1.0e29 && windowHigh < 1.0e29 && eB.size() == e.size()) {
      size_t lo = e.size(), hi = 0;
      for (size_t i = 0; i < e.size(); i++) {
        if (e[i] < windowLow || e[i] > windowHigh) continue;
        lo = std::min(lo, i);
        hi = std::max(hi, i);
      }
      if (lo <= hi) {
        windowLowB = eB[lo] - 1.0e-9;
        windowHighB = eB[hi] + 1.0e-9;
      }
    }
    vector<string> computedBetaLabels;
    string computedBetaNote;
    wxStopWatch throttle;
    if (computeFullGroupLabels(calc, group, table, sgfrag, coords, elements,
                               eB, sB, computedBetaLabels, computedBetaNote,
                               "MOBETA", windowLowB, windowHighB,
                               [&](double frac) {
                                 if (throttle.Time() < 100) return;
                                 throttle.Start();
                                 progressDlg.progress(40 + (int)(frac*10),
                                     _("Assigning full-group symmetry labels (beta)"));
                               }, !fullGroup.empty())) {
      sB = computedBetaLabels;
    }
  }

  //  A linear molecule's labels in the linear spelling -- but only labels
  //  known to be in the FULL stand-in group.  In ORCA's own C2v labels b1
  //  and b2 are the two halves of pi, not delta.
  const bool linearMolecule = isCollinear(coords) &&
      (group == "C4V" || group == "C4v" || group == "D4H" || group == "D4h");
  if (linearMolecule && labelsInFullGroup) {
    for (size_t i = 0; i < s.size(); i++) {
      s[i] = linearName(MoDiagram::canonicalIrrep(s[i]));
    }
    //  The beta labels get the same spelling, or matchAlphaBeta() finds
    //  no irrep in common and the triplet's beta electrons are not drawn.
    for (size_t i = 0; i < sB.size(); i++) {
      sB[i] = linearName(MoDiagram::canonicalIrrep(sB[i]));
    }
  }

  //  Which beta orbital (an index into eB/oB/sB) matches each alpha
  //  orbital (an index into e/o/s) -- computed once, here, in the same
  //  spelling `s` ends up in, then read back per drawn LEVEL below once
  //  groupByIrrep() has grouped the alpha orbitals into levels.
  vector<int> betaOfAlpha;
  if (haveBeta) matchAlphaBeta(s, sB, (int)e.size(), (int)eB.size(), betaOfAlpha);

  //  1e-5, not 1e-4: same tightening as SymmetryAnalysis::
  //  fullLabelSpectrum() and for the same reason (#132 follow-up,
  //  Cr(CO)6) -- a real degenerate set agrees far tighter than either
  //  value, so this only removes the risk of two DIFFERENT same-named
  //  levels (unlikely, but the same-irrep guard below does not bound
  //  energy on its own) being averaged into one.
  MoDiagram::groupByIrrep(e, o, s, dimensions, 1.5e-5, centre.levels);
  centre.title = "Molecular orbitals";

  //  One atom: each level is named by its shell, not an irrep.
  {
    const vector<string> shells = RadialDist::atomOrbitalLabels(calc, false);
    if (shells.size() == e.size())
      for (MoLevel& lv : centre.levels)
        if (!lv.orbitals.empty() && !shells[lv.orbitals[0]].empty())
          lv.label = shells[lv.orbitals[0]];
  }

  //  MERGE: one spatial level per alpha level, at the alpha energy
  //  (unchanged above), with the matched beta electrons folded in as
  //  MoLevel::occupancyBeta -- see its header comment and
  //  MoDiagramCanvas::drawElectrons() for how the two spins are then
  //  drawn independently.  A level whose orbitals could not all be
  //  matched to a beta orbital is left with occupancyBeta unset (-1):
  //  a partial count would be worse than the old combined-spin arrow.
  if (haveBeta) {
    for (size_t i = 0; i < centre.levels.size(); i++) {
      MoLevel& level = centre.levels[i];
      double betaSum = 0.0;
      bool ok = !level.orbitals.empty();
      for (size_t k = 0; k < level.orbitals.size() && ok; k++) {
        const int a = level.orbitals[k];
        if (a < 0 || a >= (int)betaOfAlpha.size() || betaOfAlpha[a] < 0) {
          ok = false;
          break;
        }
        const int b = betaOfAlpha[a];
        if (b < 0 || b >= (int)oB.size()) { ok = false; break; }
        betaSum += oB[b];
      }
      if (ok) level.occupancyBeta = betaSum;
    }
  }

  //  The WHOLE spectrum, kept for reconcile() below.  Whether the
  //  fragments' irreps can be found among the molecule's is a question
  //  about every orbital the calculation has, not the window that is
  //  drawn: benzene's fragments make an a2g and a third b1u that sit in
  //  the virtuals above the cutoff, and comparing against the visible
  //  levels alone refused a correct D6h calculation.
  const vector<MoLevel> wholeSpectrum = centre.levels;

  //  MO COEFFICIENTS AND THE BASIS OVERLAP -- built ONCE here, before
  //  the core cutoff is even chosen, and reused below for the semicore
  //  check as well as further down for the fragment composition
  //  (composeLevels()) and the localisation check (#175).  Independent
  //  of any fragment construction: functionsPerAtom()/buildBasisOverlap()
  //  read `calc` and `sgfrag` directly, not the left/right columns
  //  MoFragments::build() has not even produced yet at this point.
  vector<int> moPerAtom, moShellOf, moPerAtomS;
  vector< vector<double> > moCoefficients;
  vector<double> moSflat, moSqrtS;
  //  The parent's own basis functions and atoms, in the stored frame the
  //  coefficients are in: the fragment-orbital construction projects in
  //  that frame, not in the symmetrised copy `coords` above.
  vector<EspBasisFunction> moBasisFns;
  vector<string> storedSymbols;
  vector<double> storedCoords;
  bool moHaveSqrtS = false;
  bool moSemiempirical = false;
  {
    PropTable *moCoefs = (PropTable*)calc->getProperty("MO");
    //  MOAOORDER-marked calcs (currently ORCA) store MO with columns in
    //  the PARSER's canonical order, not TGBSConfig's own -- see
    //  MoAoOrder.H. "elements" above is already sgfrag's own atom
    //  order, the same one functionsPerAtom() below assumes.
    std::unique_ptr<PropTable> moCoefsOwned;
    if (moCoefs != 0) {
      ICalculation *moAoEscalc = dynamic_cast<ICalculation*>(calc);
      //  gbsConfig() returns a NEW TGBSConfig the caller owns (see the
      //  other call sites in this file, which all delete it) -- only
      //  needed transiently here, to hand to reorderToNative().
      std::unique_ptr<TGBSConfig> moAoConfig(
          moAoEscalc != 0 ? moAoEscalc->gbsConfig() : 0);
      if (moAoConfig != 0) {
        PropTable *reordered = MoAoOrder::reorderToNative(
            moCoefs, calc, elements, moAoConfig.get());
        if (reordered != moCoefs) {
          moCoefsOwned.reset(reordered);
          moCoefs = reordered;
        }
      }
    }
    if (moCoefs != 0 && moCoefs->rows() > 0 && moCoefs->columns() > 0 &&
        functionsPerAtom(calc, sgfrag, moCoefs->columns(), moPerAtom,
                         moShellOf, &moSemiempirical)) {
      const int norb = moCoefs->rows();
      const int nbasis = moCoefs->columns();
      moCoefficients.assign(norb, vector<double>(nbasis));
      for (int m = 0; m < norb; m++)
        for (int mu = 0; mu < nbasis; mu++)
          moCoefficients[m][mu] = moCoefs->value(m, mu);
      //  Orbitals of a degenerate level that a pick's subgroup splits (t2 into
      //  a1 + e) are the combinations of one irrep each.
      if (rotatedAlpha.size() == moCoefficients.size() && !rotatedAlpha.empty() &&
          rotatedAlpha[0].size() == moCoefficients[0].size())
        moCoefficients = rotatedAlpha;

      progressDlg.progress(44, _("Computing overlap integrals"));
      wxStopWatch overlapThrottle;
      if (buildBasisOverlap(calc, sgfrag, nbasis, moPerAtomS, moSflat,
                            [&](double frac) {
                              if (overlapThrottle.Time() < 100) return;
                              overlapThrottle.Start();
                              progressDlg.progress(44 + (int)(frac*4),
                                  _("Computing overlap integrals"));
                            }, &moBasisFns)) {
        fragmentSymbols(sgfrag, storedSymbols, storedCoords);
        //  S^{1/2}, once per diagram -- every MO below reuses it, only
        //  c changes per orbital (#170, then reused again for #175).
        progressDlg.progress(48, wxString::FromUTF8("Orthogonalising the basis (Löwdin)"));
        wxStopWatch sqrtThrottle;
        moHaveSqrtS =
            MoComposition::buildSqrtOverlap(moSflat, nbasis, moSqrtS,
                [&](double frac) {
                  if (sqrtThrottle.Time() < 100) return;
                  sqrtThrottle.Start();
                  progressDlg.progress(48 + (int)(frac*2),
                      wxString::FromUTF8("Orthogonalising the basis (Löwdin)"));
                });
        if (!moHaveSqrtS && !moSemiempirical && why.empty()) {
          why = "The basis overlap matrix is too close to linearly "
                "dependent to take a square root of, so the fragment "
                "composition uses plain AO coefficients instead of a "
                "Löwdin share.";
        }
      }
    }
  }

  //  The charge, so the fragment levels carry the right electron
  //  count -- NO2- has one more than its atoms bring -- and so the
  //  heading can show it, which is what a reader checks the formula
  //  for.
  int charge = 0;
  {
    Fragment *frag = fw.getSceneGraph().getFragment();
    if (frag != 0) charge = (int)frag->charge();
  }
  //  The metal's unpaired d electrons follow it (fragment-orbital diagram).
  int multiplicity = 1;
  {
    ICalculation *esCalc = dynamic_cast<ICalculation*>(getCalculation());
    if (esCalc != 0 && esCalc->spinMultiplicity() != SpinMult::unknown)
      multiplicity = (int)esCalc->spinMultiplicity();
  }

  //  FRAGMENT ORBITALS (#301): offered when the parent's basis and
  //  overlap are in hand and the structure is a metal with ligands.  A
  //  code whose coefficients are orthonormalised (MOPAC) cannot be
  //  projected on them, and the panel says so.  Decided here, before the
  //  cutoffs: they must not fold away the metal's own occupied d levels.
  bool foOffered = false;
  string foNotOffered;
  //  The parent's basis and overlap are in hand and can be projected on.
  bool foProjectable = false;
  {
    const size_t nb = moBasisFns.size();
    if (!moSemiempirical && nb > 0 && moSflat.size() == nb*nb &&
        moCoefficients.size() == e.size() &&
        !moCoefficients.empty() && moCoefficients[0].size() == nb) {
      foProjectable = true;
    }
  }
  const string foSemiempirical = "Fragment orbitals are not offered for this "
      "calculation: its orbitals are in an orthonormalised basis, "
      "so they cannot be projected on fragment orbitals.";
  {
    string foReason;
    if (!storedSymbols.empty() &&
        FoDiagram::applicable(storedSymbols, storedCoords, 0, foReason)) {
      if (moSemiempirical) foNotOffered = foSemiempirical;
      else if (foProjectable) foOffered = true;
    } else if (foReason.compare(0, 11, "No extended") == 0) {
      //  A metal complex with an element that has no EHT parameters (Ag)
      //  would otherwise fall back to another construction without a word.
      foNotOffered = "The fragment-orbital diagram is not available: " +
                     foReason + " Use the MOs panel to look at the orbitals.";
    }
  }

  //  A single non-metal atom pick goes the fragment-orbital way when the
  //  one-atom diagram (the atom against its bonded atoms, in the
  //  molecule's group) would refuse it or leave atoms out (#301).
  int pickCentre = -1;
  if (pickActive && !pickMetal && pickB < 0 &&
      storedSymbols.size() == elements.size()) {
    MoColumn l2, r2;
    string n2;
    MoFragments::Fragmentation how2 = MoFragments::NOT_BUILT;
    const bool drawn = MoFragments::build(coords, elements, group, charge, l2,
                                          r2, n2, 0, 0, 0, &how2,
                                          MoFragments::NOT_BUILT, &pickSel);
    vector< std::pair<int,int> > bonds2;
    MoFragments::covalentBonds(coords, elements, bonds2);
    size_t neighbours = 0;
    for (size_t b = 0; b < bonds2.size(); b++) {
      if (bonds2[b].first == pickA || bonds2[b].second == pickA) neighbours++;
    }
    const bool whole = drawn && how2 == MoFragments::CENTRAL &&
                       neighbours + 1 == elements.size();
    if (!whole) {
      string foReason;
      if (moSemiempirical) {
        foNotOffered = foSemiempirical;
      } else if (!FoDiagram::applicable(storedSymbols, storedCoords, charge,
                                        foReason, pickA)) {
        foNotOffered = "The fragment-orbital diagram is not available for "
                       "this atom: " + foReason +
                       " Use the MOs panel to look at the orbitals.";
      } else if (foProjectable) {
        pickCentre = pickA;
      }
    }
  }
  bool foWanted = foOffered;
  const MoFragments::Fragmentation fromEnv = constructionFromEnv();
  if (pickActive) {
    foWanted = (foOffered && pickMetal) || pickCentre >= 0;
  } else if (fromEnv != MoFragments::NOT_BUILT &&
      fromEnv != MoFragments::FRAGMENT_ORBITALS) {
    foWanted = false;
  } else if (foOffered && p_construction != 0) {
    const int picked = p_construction->GetSelection();
    if (picked > 0 && picked <= (int)p_offered.size() &&
        p_offered[picked - 1] != MoFragments::FRAGMENT_ORBITALS) {
      foWanted = false;
    }
  }

  //  Both ends of the spectrum are folded away, not just the core, and
  //  both cutoffs report what they hid so an absence the reader cannot
  //  see does not pass for a complete diagram.
  MoDiagram::hideBelow(centre, MoDiagram::suggestCoreCutoff(centre.levels));

  //  SEMICORE (#175): an OCCUPIED level whose largest Lowdin share sits,
  //  at 90% or more, on one atom, AND whose energy is below TWICE that
  //  atom's own most negative tabulated valence energy, takes no part
  //  in bonding even though it sits well inside the valence energy
  //  WINDOW -- suggestCoreCutoff()'s single biggest-occupied-gap rule
  //  cannot catch it, because nothing separates it from genuine valence
  //  levels by a gap (Cr(CO)6's 3p semicore at -2.246 Ha, with the
  //  valence t2g at -0.354 and nothing between).
  //
  //  Checked AFTER hideBelow(), not before: a real core orbital (e.g.
  //  water's O 1s) is JUST as localised and JUST as far below twice the
  //  valence energy as a true semicore one is -- the energy-plus-share
  //  rule alone cannot tell the two apart, only "already caught by the
  //  ordinary big-gap cutoff" can. Running this against what hideBelow()
  //  leaves means it only ever sees levels the gap rule did NOT already
  //  remove, so a genuine core level is never double-counted or
  //  relabelled -- confirmed against the water fixtures, where nothing
  //  here fires at all (#175 capture).
  vector<bool> isSemicore(centre.levels.size(), false);
  if ((moHaveSqrtS || moSemiempirical) &&
      moPerAtomS.size() == elements.size()) {
    const vector<int> noShellSplit;
    const double HARTREE = 27.211386245988;   // eV per Hartree
    for (size_t i = 0; i < centre.levels.size(); i++) {
      const MoLevel& level = centre.levels[i];
      if (level.occupancy <= 0.0) continue;

      vector<double> atomSum(elements.size(), 0.0);
      int ncomp = 0;
      for (size_t k = 0; k < level.orbitals.size(); k++) {
        const int mo = level.orbitals[k];
        if (mo < 0 || mo >= (int)moCoefficients.size()) continue;
        const vector<double>& c = moCoefficients[mo];

        vector<double> shares;
        if (moSemiempirical) {
          //  Plain c^2 per atom -- the orthonormal-basis case, same
          //  convention MoComposition::compute()/share() use when no
          //  overlap matrix is given.
          const vector<MoComposition::Share> full = MoComposition::compute(
              c, moPerAtomS, noShellSplit, elements, vector<double>());
          shares.assign(elements.size(), 0.0);
          for (size_t f = 0; f < full.size(); f++) {
            if (full[f].atom >= 0 && full[f].atom < (int)shares.size()) {
              shares[full[f].atom] += full[f].share;
            }
          }
        } else {
          shares = MoComposition::lowdinShares(c, moPerAtomS, moSqrtS);
        }
        if (shares.size() != atomSum.size()) continue;
        for (size_t a = 0; a < shares.size(); a++) atomSum[a] += shares[a];
        ncomp++;
      }
      if (ncomp == 0) continue;

      int maxAtom = -1;
      double maxShare = 0.0;
      for (size_t a = 0; a < atomSum.size(); a++) {
        const double v = atomSum[a]/ncomp;
        if (v > maxShare) { maxShare = v; maxAtom = (int)a; }
      }
      if (maxAtom < 0 || maxShare < MoComposition::LOCALISED_SHARE_THRESHOLD)
        continue;

      bool haveVoie = false;
      double mostNegative = 0.0;
      for (int l = 0; l <= 2; l++) {
        double eV;
        if (!MoFragments::valenceEnergy(elements[maxAtom], l, eV)) continue;
        const double ha = eV/HARTREE;
        if (!haveVoie || ha < mostNegative) { mostNegative = ha; haveVoie = true; }
      }
      if (!haveVoie) continue;

      if (level.energy < 2.0*mostNegative) isSemicore[i] = true;
    }
  }
  //  The fragment-orbital diagram tells a core level by its capture, and a
  //  Co(III) t2g sits below twice its VOIE: the semicore rule would fold
  //  away the very levels the diagram is about.
  int keptSemicore = 0;
  if (foWanted) {
    for (size_t i = 0; i < isSemicore.size() && i < centre.levels.size(); i++) {
      if (isSemicore[i]) {
        keptSemicore += centre.levels[i].degeneracy > 0
                        ? centre.levels[i].degeneracy : 1;
      }
    }
  } else {
    MoDiagram::hideSemicore(centre, isSemicore);
  }

  //  PROTECT A METAL-HEAVY VIRTUAL FROM THE CUTOFFS (#183, "partner
  //  always shown"): suggestVirtualCutoff() folds by counting/gaps
  //  alone, and hideBeyondValence() folds by a fixed valence-orbital
  //  COUNT -- neither knows which virtual is a bonding level's
  //  antibonding partner. A lambda, not a one-off vector, because it
  //  has to be recomputed against whatever centre.levels IS at the
  //  time: hideAbove() below removes levels, so a vector sized for the
  //  list before it no longer lines up with the list after it, and
  //  hideBeyondValence() runs later still.  Only for the metal atom
  //  identified from the coordination skeleton, and only for
  //  UNoccupied levels (an occupied one is never hidden by hideAbove()
  //  anyway).
  auto protectMetalVirtuals = [&]() -> vector<bool> {
    vector<bool> protect(centre.levels.size(), false);
    if (metalAtomIndex < 0 || !(moHaveSqrtS || moSemiempirical) ||
        moPerAtomS.size() != elements.size() ||
        (size_t)metalAtomIndex >= elements.size()) {
      return protect;
    }
    const vector<int> noShellSplit;
    for (size_t i = 0; i < centre.levels.size(); i++) {
      const MoLevel& level = centre.levels[i];
      if (level.occupancy > 0.0) continue;

      double metalSum = 0.0;
      int ncomp = 0;
      for (size_t k = 0; k < level.orbitals.size(); k++) {
        const int mo = level.orbitals[k];
        if (mo < 0 || mo >= (int)moCoefficients.size()) continue;
        const vector<double>& c = moCoefficients[mo];

        vector<double> shares;
        if (moSemiempirical) {
          const vector<MoComposition::Share> full = MoComposition::compute(
              c, moPerAtomS, noShellSplit, elements, vector<double>());
          shares.assign(elements.size(), 0.0);
          for (size_t f = 0; f < full.size(); f++) {
            if (full[f].atom >= 0 && full[f].atom < (int)shares.size()) {
              shares[full[f].atom] += full[f].share;
            }
          }
        } else {
          shares = MoComposition::lowdinShares(c, moPerAtomS, moSqrtS);
        }
        if ((size_t)metalAtomIndex >= shares.size()) continue;
        metalSum += shares[metalAtomIndex];
        ncomp++;
      }
      if (ncomp == 0) continue;
      if (metalSum/ncomp >= 0.10) protect[i] = true;
    }
    return protect;
  };

  MoDiagram::hideAbove(centre,
                       MoDiagram::suggestVirtualCutoff(centre.levels),
                       protectMetalVirtuals());

  //  Set below when the pi-only filter actually removed something --
  //  used much further down to trim the FRAGMENT columns to match,
  //  which this block cannot do itself: the fragment columns are not
  //  even built yet at this point in the function.
  bool piOnlyApplied = false;

  //  Before anything indexes the levels: correlation links and the
  //  hit map are positions in this vector, so a level removed after
  //  they are built would shift every one of them.
  {
    const CharacterTable *table = CharacterTable::lookup(group);

    //  THE GEOMETRY FIRST, the table's naming only as a fallback.
    //
    //  Seven of the forty-six groups name a distinguished plane, so
    //  asking the table alone left ethene and water -- both planar,
    //  both with orbitals perpendicular to that plane -- unable to be
    //  asked the question at all. A plane containing every atom is
    //  the one reflection that leaves every atom where it is, which
    //  needs no naming convention.
    int planeClass = -1;
    if (table != 0) {
      planeClass = MoFragments::molecularPlaneClass(coords, elements, group);
      if (planeClass < 0 && MoDiagram::hasMolecularPlane(*table)) {
        //  Not planar, but the group names a plane: methanol's mirror
        //  swaps two hydrogens rather than fixing them, and the
        //  separation it defines is still a real one.
        planeClass = -2;
      }
    }
    const bool canSeparate = (table != 0) && (planeClass != -1);

    if (p_piOnly != 0 && p_piOnly->IsShown() != canSeparate) {
      p_piOnly->Show(canSeparate);
      if (!canSeparate) p_piOnly->SetValue(false);
      Layout();
    }

    if (canSeparate && p_piOnly != 0 && p_piOnly->GetValue()) {
      vector<MoLevel> kept;
      for (size_t i = 0; i < centre.levels.size(); i++) {
        const bool isPi = (planeClass >= 0)
            ? MoDiagram::isPiIrrep(*table, centre.levels[i].irrep, planeClass)
            : MoDiagram::isPiIrrep(*table, centre.levels[i].irrep);
        if (isPi) {
          kept.push_back(centre.levels[i]);
        }
      }
      //  Never leave the diagram empty: a molecule whose orbitals
      //  carry no labels has nothing to filter on, and showing
      //  nothing would read as a failure rather than as a filter
      //  that found nothing.
      if (!kept.empty()) {
        centre.levels = kept;
        piOnlyApplied = true;
      }
    }
  }

  {
    int valenceRoom = 0;
    for (size_t i = 0; i < elements.size(); i++) {
      valenceRoom += MoFragments::valenceOrbitals(elements[i]);
    }
    //  NOT protected here (#183): hideBeyondValence()'s cut is a basis-
    //  set-size artefact removal, not a chemistry judgement, and a flat
    //  10% metal-share protection there pulled in Cr(CO)6's whole tail
    //  of deep, weakly d-mixed Rydberg-like virtuals (24 shown levels
    //  became 44) rather than just the handful of real antibonding
    //  partners -- which suggestVirtualCutoff()'s own protection above
    //  already keeps whenever a partner is genuinely close to the
    //  valence region, confirmed against this fixture.
    //  The semicore levels kept for the fragment-orbital diagram take room.
    MoDiagram::hideBeyondValence(centre, valenceRoom + keptSemicore);
  }

  for (size_t i = 0; i < centre.levels.size(); i++) {
    ostringstream text;
    text.setf(std::ios::fixed);
    text.precision(4);
    text << centre.levels[i].energy;
    centre.levels[i].annotation = text.str();
  }

  //  Whether the calculation gave us orbital symmetry labels at all.
  //  Used far below to say which of the two reasons a correlation
  //  could not be drawn.
  const bool haveSymmetryLabels = !s.empty();

  //  What the diagram has to say about itself, in three tiers: a summary
  //  line, warnings (only when something is wrong) and the About text.
  const string unlabelledText =
      "This calculation reports no orbital symmetry labels, so the molecular "
      "levels are unlabelled and are correlated by what they are built from "
      "rather than by symmetry.";
  const string betaText =
      "This is an unrestricted calculation: the levels are drawn at their "
      "alpha energies, and the beta electrons are matched to them by symmetry.";

  //  --- the fragment columns -----------------------------------------
  //
  //  A correlation diagram is a symmetry argument, so these depend on
  //  the geometry and the point group and not on the calculation.  The
  //  molecular orbitals are still drawn when this cannot be made: a
  //  level diagram on its own is useful, and refusing to draw anything
  //  would be worse than saying why the rest is missing.
  bool haveFragments = false;

  //  BUILD IN THE GROUP THE CALCULATION ACTUALLY USED.
  //
  //  ORCA symmetry-adapts orbitals only in ABELIAN point groups.  It
  //  detects the full group correctly and then works in the largest
  //  abelian subgroup, and says so:
  //
  //      Point group                           .... Td
  //      Symmetry-adapted orbitals             .... D2
  //
  //  So its ORBSYM carries D2 labels -- A, B1, B2, B3 -- while the
  //  structure is Td, the fragment columns are built in Td, and the two
  //  label sets can never match.  Every ORCA calculation of a molecule
  //  whose group is not already abelian correlated nothing at all
  //  (#147); water was fine only because C2v is abelian.
  //
  //  A diagram in the smaller group is less informative -- methane's t2
  //  set splits into b1 + b2 + b3 -- but it is correct, and it is what
  //  the calculation computed.  Drawing it in Td instead would be
  //  claiming a degeneracy the numbers do not have.
  if (!s.empty() && !group.empty()) {
    const CharacterTable *structure = CharacterTable::lookup(group);

    //  What the calculation actually reported, canonicalised.
    set<string> reported;
    for (size_t i = 0; i < s.size(); i++) {
      const string canon = MoDiagram::canonicalIrrep(s[i]);
      if (!canon.empty()) reported.insert(canon);
    }

    bool fits = (structure != 0);
    if (fits) {
      const vector<string>& theirs = structure->irreps();
      for (set<string>::const_iterator it = reported.begin();
           fits && it != reported.end(); ++it) {
        bool found = false;
        for (size_t j = 0; j < theirs.size(); j++) {
          if (MoDiagram::canonicalIrrep(theirs[j]) == *it) found = true;
        }
        if (!found) fits = false;
      }
    }

    //  They are not this group's.  Find the smallest group whose irreps
    //  cover them: smallest, because a label set is contained in many
    //  larger tables and the one the code used is the tightest fit.
    if (!fits && !reported.empty()) {
      vector<string> all = CharacterTable::names();
      string best;
      int bestOrder = 0;
      for (size_t g = 0; g < all.size(); g++) {
        const CharacterTable *cand = CharacterTable::lookup(all[g]);
        if (cand == 0) continue;
        const vector<string>& theirs = cand->irreps();
        bool covers = true;
        for (set<string>::const_iterator it = reported.begin();
             covers && it != reported.end(); ++it) {
          bool found = false;
          for (size_t j = 0; j < theirs.size(); j++) {
            if (MoDiagram::canonicalIrrep(theirs[j]) == *it) found = true;
          }
          if (!found) covers = false;
        }
        if (!covers) continue;
        const int order = cand->order();
        if (best.empty() || order < bestOrder) { best = all[g]; bestOrder = order; }
      }

      if (!best.empty() && best != group) {
        ostringstream text;
        text << "The calculation worked in " << best << ", a subgroup of "
             << group << " -- ORCA and some other codes adapt orbitals "
                "only in abelian groups -- so the diagram is drawn in "
             << best << " to match it.";
        if (why.empty()) { why = text.str(); whyWarn = false; }
        group = best;
      }
    }
  }

  //  Which atoms actually ended up in each column -- needed below to
  //  compute each molecular level's composition, not just its irrep.
  vector<int> leftAtoms, rightAtoms;

  //  The fragment-orbital construction (FoDiagram) has its own columns,
  //  lines, placement and note; none of the symmetry-based machinery
  //  below runs for it.
  bool foDrawn = false;
  string foSummary, foAbout, foPieces;
  vector<string> foWarnings;
  //  The model-first diagram (ModelDiagram) draws through the same
  //  plumbing as the fragment-orbital one.
  bool modelDrawn = false;
  bool modelFolded = false;
  bool modelLfApplicable = false;
  string modelVerdict;
  bool modelVerdictOk = true;

  if (!elements.empty()) {
    //  OFFER THE CHOICE, AND HONOUR IT.
    //
    //  fillFragmentChooser() existed and was called from nowhere, so
    //  the list beside "Choose fragments" was never filled and the box
    //  did nothing whatever it was set to.  The orbits are what the
    //  choice is over -- a fragment has to be a whole orbit or there
    //  is nothing to project -- and MoFragments can work them out from
    //  the geometry and the group alone.
    vector< vector<int> > orbits;
    string orbitWhy;
    if (MoFragments::orbitsOf(coords, elements, group, orbits, orbitWhy)) {
      fillFragmentChooser(orbits, elements);
    }

    //  p_sideOfOrbit is empty unless the user is choosing, and
    //  MoFragments::build() then decides for itself -- which tries
    //  the two halves FIRST, so asking for halves means handing it no
    //  chosen split at all.
    const vector<int> *chosen =
        p_sideOfOrbit.empty() ? 0 : &p_sideOfOrbit;

    //  WHAT IS SELECTED, NOT WHERE IT SITS.
    //
    //  The list is rebuilt per molecule and so has a different length
    //  each time; reading the choice as an index into a fixed table
    //  is the mistake this tree has made before, where a default
    //  written against the longest list left a combo showing nothing
    //  at all. Keep the enum beside each row and read that.
    MoFragments::Fragmentation want = MoFragments::NOT_BUILT;
    {
      const int picked = (p_construction != 0)
                         ? p_construction->GetSelection() : 0;
      if (picked > 0 && picked <= (int)p_offered.size()) {
        want = p_offered[picked - 1];
      }
    }

    //  Refill for this molecule, keeping the selection if it still
    //  applies -- changing calculation should not silently change
    //  which construction is being drawn.
    if (p_construction != 0) {
      vector<MoFragments::Fragmentation> offers;
      MoFragments::availableFragmentations(coords, elements, group, offers);
      if (foOffered) {
        offers.insert(offers.begin(), MoFragments::FRAGMENT_ORBITALS);
      }

      if (offers != p_offered) {
        const MoFragments::Fragmentation was = want;
        p_offered = offers;

        p_construction->Clear();
        p_construction->Append(_("Automatic"));
        for (size_t i = 0; i < p_offered.size(); i++) {
          p_construction->Append(
              wxString(MoFragments::fragmentationName(p_offered[i]),
                       wxConvUTF8));
        }

        int restore = 0;
        for (size_t i = 0; i < p_offered.size(); i++) {
          if (p_offered[i] == was) restore = (int)i + 1;
        }
        p_construction->SetSelection(restore);
        if (restore == 0) want = MoFragments::NOT_BUILT;

        //  A molecule with nothing to choose between has no choice to
        //  offer, so do not pretend otherwise.
        p_construction->Enable(!p_offered.empty());
      }
    }

    if (fromEnv != MoFragments::NOT_BUILT &&
        (fromEnv != MoFragments::FRAGMENT_ORBITALS || foOffered)) {
      want = fromEnv;
    }

    //  A pick decides by itself, whatever Built as says.
    if (pickActive) {
      want = ((pickMetal && foOffered) || pickCentre >= 0)
             ? MoFragments::FRAGMENT_ORBITALS : MoFragments::NOT_BUILT;
    }

    //  Automatic means the fragment-orbital diagram where it is offered.
    if (!pickActive && want == MoFragments::NOT_BUILT && foOffered) {
      want = MoFragments::FRAGMENT_ORBITALS;
    }

    progressDlg.progress(50, _("Building fragment orbitals"));

    //  THE MODEL-FIRST DIAGRAM (#301): extended Hueckel for the fragments,
    //  the lines and the middle column, the calculation as a column of its
    //  own.  `la`/`ra` are the two fragments' atoms, in the stored frame.
    auto buildModel = [&](const vector<int>& la, const vector<int>& ra,
                          int leftCharge, bool leftMetal, string& mwhy) -> bool {
      if (!foProjectable || storedSymbols.size() != elements.size() ||
          la.empty() || ra.empty()) {
        return false;
      }
      ModelDiagram::Input in;
      in.symbols = storedSymbols;
      in.coordsAng = storedCoords;
      in.charge = charge;
      in.multiplicity = multiplicity;
      in.leftAtoms = la;
      in.rightAtoms = ra;
      //  Atoms in neither fragment (the rest of a ligand whose donor atoms
      //  the construction names) belong to the right one.
      {
        vector<bool> used(storedSymbols.size(), false);
        for (size_t k = 0; k < la.size(); k++) if (la[k] >= 0 && la[k] < (int)used.size()) used[la[k]] = true;
        for (size_t k = 0; k < ra.size(); k++) if (ra[k] >= 0 && ra[k] < (int)used.size()) used[ra[k]] = true;
        for (size_t a = 0; a < used.size(); a++) if (!used[a]) in.rightAtoms.push_back((int)a);
      }
      in.leftCharge = leftCharge;
      in.rightCharge = charge - leftCharge;
      in.leftMetal = leftMetal;
      in.showAll = p_showAll != 0 && p_showAll->IsChecked();
      in.ligandFieldView = p_ligandField != 0 && p_ligandField->IsChecked();
      in.piOnly = piOnlyApplied;
      ModelDiagram::Result mr;
      if (!ModelDiagram::build(in, moBasisFns, moSflat, moCoefficients, e,
                               wholeSpectrum, mr, mwhy)) {
        return false;
      }
      left = mr.left;
      right = mr.right;
      const string title = centre.title;
      centre = mr.centre;
      centre.title = mr.centre.title.empty() ? title : mr.centre.title;
      links = mr.links;
      foSummary = mr.summary;
      foWarnings = mr.warnings;
      foAbout = mr.about;
      modelVerdict = mr.verdict;
      modelVerdictOk = mr.verdictOk;
      haveFragments = true;
      foDrawn = true;
      modelDrawn = true;
      modelFolded = mr.foldedAny;
      modelLfApplicable = mr.lfApplicable;
      return true;
    };

    if (want == MoFragments::FRAGMENT_ORBITALS) {
      bool done = false;
      {
        vector<FragmentOrbitals::Fragment> frs;
        string w2;
        if (FragmentOrbitals::autoFragments(storedSymbols, storedCoords, charge,
                                            multiplicity, frs, w2, pickCentre)) {
          vector<int> la, ra;
          int lc = 0;
          vector<bool> isLeft(storedSymbols.size(), false);
          for (size_t f = 0; f < frs.size(); f++) {
            if (frs[f].kind != FragmentOrbitals::METAL) continue;
            for (size_t k = 0; k < frs[f].atoms.size(); k++) {
              la.push_back(frs[f].atoms[k]);
              isLeft[frs[f].atoms[k]] = true;
            }
            lc += frs[f].charge;
          }
          for (size_t a = 0; a < storedSymbols.size(); a++)
            if (!isLeft[a]) ra.push_back((int)a);
          string mw;
          done = buildModel(la, ra, lc, pickCentre < 0, mw);
          if (!done && !mw.empty())
            foNotOffered = "The model diagram could not be made: " + mw;
        }
      }
      if (done) {
        p_fragmentation = MoFragments::FRAGMENT_ORBITALS;
        foPieces = "its ligands";
      }
      if (!done) {
      FoDiagram::Result fo;
      string foWhy;
      if (FoDiagram::build(storedSymbols, storedCoords, charge, moBasisFns,
                           moSflat, moCoefficients, e, centre, fo, foWhy,
                           p_showAll != 0 && p_showAll->IsChecked(),
                           &wholeSpectrum, multiplicity, pickCentre)) {
        left = fo.left;
        right = fo.right;
        links = fo.links;
        foSummary = fo.summary;
        foWarnings = fo.warnings;
        foAbout = fo.about;
        foPieces = fo.pieces;
        p_fragmentation = MoFragments::FRAGMENT_ORBITALS;
        haveFragments = true;
        foDrawn = true;
      } else {
        //  Said, and the usual construction drawn instead.
        foNotOffered = "The fragment-orbital diagram could not be made: " +
                       foWhy;
        want = MoFragments::NOT_BUILT;
      }
      }
    }
    if (!foDrawn) {
      haveFragments = MoFragments::build(coords, elements, group, charge,
                                         left, right, why, &leftAtoms,
                                         &rightAtoms, chosen,
                                         &p_fragmentation, want,
                                         pickActive ? &pickSel : 0);
      whyWarn = !haveFragments;
      if (haveFragments && !leftAtoms.empty() &&
          !rightAtoms.empty() && !storedSymbols.empty() && foProjectable) {
        string mw;
        if (!ModelDiagram::applicable(storedSymbols, mw)) {
          if (foNotOffered.empty())
            foNotOffered = "The model diagram is not available: " + mw +
                           " Use the MOs panel to look at the orbitals.";
        } else if (buildModel(leftAtoms, rightAtoms, charge, false, mw)) {
          leftAtoms.clear();
          rightAtoms.clear();
        } else if (!mw.empty()) {
          foNotOffered = "The model diagram could not be made: " + mw;
        }
      }
    }
  }

  //  COMPOSITION, SUPPLIED WHENEVER IT IS AVAILABLE -- not only as a
  //  fallback for the cases below with no irrep to match on.
  //  MoDiagram::connect() decides for itself which evidence to use;
  //  what matters here is that shareLeft/shareRight/shellLeft/
  //  shellRight are populated whenever they can be, since that is
  //  the ONLY thing that can connect a homonuclear diatomic -- a
  //  single atom spans no irrep of the molecule's group at all, so
  //  the irrep-based match has nothing to match (#132).
  //
  //  localisedShare/centreOP feed MoDiagram::classify() below: one
  //  atom's share of a level (any construction) and that level's
  //  Mulliken overlap population across the molecule's own bonds
  //  (only meaningful for a no-central-atom construction) -- both need
  //  the REAL overlap matrix, which composeLevels()'s own Lowdin
  //  composition does not, so they are computed separately here rather
  //  than threaded into it.  (Declared earlier, at buildOnce()'s top, so
  //  every dump call site -- including the early returns above -- can
  //  pass the same names.)
  if (haveFragments && !leftAtoms.empty() && !rightAtoms.empty() &&
      !moCoefficients.empty()) {
    //  COMPOSITION IS LOWDIN, NOT PLAIN c^2 (#175): composeLevels()
    //  itself still reads plain c^2 (no overlap matrix), so what makes
    //  the difference is transforming each row to c' = S^{1/2} c
    //  BEFORE handing it in -- plain c'^2 of that IS the Lowdin share.
    //  Skipped for a semiempirical basis (NDDO coefficients are already
    //  in an orthonormal basis, S = I, so plain c^2 already IS their
    //  Lowdin share) and whenever S^1/2 itself failed above, in which
    //  case the raw coefficients are used exactly as before this fix.
    const bool useLowdinCompose = moHaveSqrtS && !moSemiempirical;
    vector< vector<double> > lowdinCoefficients;
    if (useLowdinCompose) {
      lowdinCoefficients.resize(moCoefficients.size());
      for (size_t m = 0; m < moCoefficients.size(); m++) {
        lowdinCoefficients[m] =
            MoComposition::transform(moCoefficients[m], moSqrtS);
      }
    }
    const vector< vector<double> >& composeCoefficients =
        useLowdinCompose ? lowdinCoefficients : moCoefficients;

    progressDlg.progress(60, _("Building fragment orbitals"));
    MoFragments::composeLevels(centre.levels, composeCoefficients, moPerAtom,
                               moShellOf, elements, leftAtoms, rightAtoms,
                               left, right);

    if (!moSflat.empty()) {
      vector< std::pair<int,int> > bonds;
      MoFragments::covalentBonds(coords, elements, bonds);

      //  Metal-vs-every-ligand-atom pairs, not covalentBonds() -- see
      //  metalShare/metalLigandOP's declaration above.
      const bool wantMetalLigand = (p_fragmentation == MoFragments::SKELETON &&
          metalAtomIndex >= 0 && (size_t)metalAtomIndex < elements.size() &&
          moHaveSqrtS);
      vector< std::pair<int,int> > metalLigandBonds;
      if (wantMetalLigand) {
        for (size_t a = 0; a < elements.size(); a++) {
          if ((int)a == metalAtomIndex) continue;
          metalLigandBonds.push_back(std::make_pair(metalAtomIndex, (int)a));
        }
        metalShare.assign(centre.levels.size(), 0.0);
        metalLigandOP.assign(centre.levels.size(), 0.0);
      }

      const bool debug = mosymDebug();
      centreOP.assign(centre.levels.size(), 0.0);
      //  Left empty (rather than filled with a measure that can't
      //  be trusted) if S is linearly dependent enough that S^1/2
      //  failed -- classify() skips the localisation rule on an
      //  empty vector.
      if (moHaveSqrtS) localisedShare.assign(centre.levels.size(), 0.0);
      //  Dump-only (#171): kept alongside centreOP/localisedShare so
      //  the oracle can show the same [MOLOC] diagnostics without
      //  reading debug stderr output.
      mullikenMaxVec.assign(centre.levels.size(), 0.0);
      normVec.assign(centre.levels.size(), 0.0);
      const vector<int> noShellSplit;
      for (size_t i = 0; i < centre.levels.size(); i++) {
        MoLevel& level = centre.levels[i];
        //  Per-atom shares, summed over this level's components and
        //  later divided by ncomp -- same averaging for both
        //  measures, only Lowdin feeds localisedShare[i] now (#170).
        //  Mulliken is kept only for the debug comparison line.
        vector<double> lowdinAtomSum(elements.size(), 0.0);
        vector<double> mullikenAtomSum(elements.size(), 0.0);
        double opSum = 0.0, normSum = 0.0, metalLigandOpSum = 0.0;
        int ncomp = 0;
        for (size_t k = 0; k < level.orbitals.size(); k++) {
          const int mo = level.orbitals[k];
          if (mo < 0 || mo >= (int)moCoefficients.size()) continue;
          ncomp++;
          const vector<double>& c = moCoefficients[mo];

          if (moHaveSqrtS) {
            const vector<double> lowdin =
                MoComposition::lowdinShares(c, moPerAtomS, moSqrtS);
            for (size_t a = 0; a < lowdin.size() &&
                               a < lowdinAtomSum.size(); a++) {
              lowdinAtomSum[a] += lowdin[a];
            }
          }

          //  shellOf left empty: one entry per ATOM, already
          //  summed over its shells -- Mulliken, kept for the
          //  debug comparison only.
          const vector<MoComposition::Share> full =
              MoComposition::compute(c, moPerAtomS,
                                     noShellSplit, elements, moSflat);
          for (size_t f = 0; f < full.size(); f++) {
            if (full[f].atom >= 0 &&
                full[f].atom < (int)mullikenAtomSum.size()) {
              mullikenAtomSum[full[f].atom] += full[f].share;
            }
          }

          //  c^T.S.c for this MO, independent of either share
          //  measure above -- the actual invariant #170 wants
          //  checked (should be 1 for a properly normalised MO).
          double norm = 0.0;
          const int nb = (int)c.size();
          if ((int)moSflat.size() == nb*nb) {
            for (int mu = 0; mu < nb; mu++) {
              double row = 0.0;
              for (int nu = 0; nu < nb; nu++) row += c[nu]*moSflat[mu*nb + nu];
              norm += c[mu]*row;
            }
          }
          normSum += norm;

          opSum += MoComposition::overlapPopulation(c, moPerAtomS, moSflat,
                                                     bonds);
          if (wantMetalLigand) {
            metalLigandOpSum += MoComposition::overlapPopulation(
                c, moPerAtomS, moSflat, metalLigandBonds);
          }
        }
        if (ncomp > 0) {
          centreOP[i] = opSum/ncomp;
          normVec[i] = normSum/ncomp;
          if (wantMetalLigand) {
            metalLigandOP[i] = metalLigandOpSum/ncomp;
            metalShare[i] = ((size_t)metalAtomIndex < lowdinAtomSum.size())
                ? lowdinAtomSum[metalAtomIndex]/ncomp : 0.0;
          }

          double lowdinMax = 0.0;
          if (moHaveSqrtS) {
            for (size_t a = 0; a < lowdinAtomSum.size(); a++) {
              const double v = lowdinAtomSum[a]/ncomp;
              if (v > lowdinMax) lowdinMax = v;
            }
            localisedShare[i] = lowdinMax;
          }

          double mullikenMax = 0.0;
          for (size_t a = 0; a < mullikenAtomSum.size(); a++) {
            const double v = mullikenAtomSum[a]/ncomp;
            if (v > mullikenMax) mullikenMax = v;
          }
          mullikenMaxVec[i] = mullikenMax;

          if (debug) {
            mosymPrintf("[MOLOC] %s E=%.4f lowdinMax=%.3f "
                        "mullikenMax=%.3f OP=%.4f norm=%.3f\n",
                        level.irrep.c_str(), level.energy,
                        lowdinMax, mullikenMax,
                        centreOP[i], normSum/ncomp);
          }
        }
      }
    }
  }

  //  ONLY A NO-CENTRAL-ATOM CONSTRUCTION HANDS THE OVERLAP POPULATION
  //  TO classify() -- for a central atom and its terminal atoms the
  //  existing count-across-columns rule is the right one and must not
  //  change (CH4, H2O, NH3, CO, N2 all behave today from it); the
  //  localisation check above runs either way, since a lone pair is a
  //  lone pair whichever construction this is.
  const bool useOverlapPopulation =
      (p_fragmentation == MoFragments::GROUPS ||
       p_fragmentation == MoFragments::EQUIVALENT_SETS);

  //  THE FRAGMENT COLUMNS DO NOT NEED THE CODE'S SYMMETRY LABELS.
  //
  //  They come from the geometry and the point group, and nothing
  //  else.  Gating them on ORBSYM -- which is what this did -- made a
  //  water whose code reported no labels come out as a bare column of
  //  levels with no atomic orbitals, no symmetry orbitals and no
  //  correlation lines: none of what the diagram is for.
  //
  //  What the labels are needed for is matching an orbital to a
  //  fragment level BY IRREP.  Where they are missing, or cannot be
  //  reconciled with the character table's spelling, or belong to a
  //  different group than the structure has, the matching falls back
  //  to composition -- which fragment and which shell an orbital
  //  actually draws from, from its own coefficients.  That needs no
  //  symmetry labels at all.
  bool byIrrep = haveFragments && !foDrawn;

  //  COLUMNS THAT CARRY NO IRREP ARE NOT COLUMNS THAT DISAGREE.
  //
  //  A diatomic's fragment is one atom, which spans no representation
  //  of the molecule's group, so nothing on either side can match the
  //  molecular labels -- and clearing the molecular labels for want of
  //  a match threw away the only symmetry the diagram had.  N2 then
  //  paired its orbitals by energy order alone and coloured 1-sigma-g
  //  with 2-sigma-u, which are not partners.
  bool columnsCarryIrreps = false;
  for (size_t j = 0; j < left.levels.size(); j++) {
    if (!left.levels[j].irrep.empty()) columnsCarryIrreps = true;
  }
  for (size_t j = 0; j < right.levels.size(); j++) {
    if (!right.levels[j].irrep.empty()) columnsCarryIrreps = true;
  }

  //  The fragment columns are always in the full stand-in group.
  if (linearMolecule && !foDrawn) {
    for (size_t j = 0; j < left.levels.size(); j++)
      left.levels[j].irrep = linearName(left.levels[j].irrep);
    for (size_t j = 0; j < right.levels.size(); j++)
      right.levels[j].irrep = linearName(right.levels[j].irrep);
  }

  if (byIrrep) {
    //  Reconcile the axis conventions BEFORE anything is matched on
    //  the names: in C2v the character table and the code need not
    //  agree on which mirror is sigma-v, and water comes out inside
    //  out if they are compared as they stand.
    string mismatch;
    if (!MoDiagram::reconcile(left.levels, right.levels, wholeSpectrum,
                              mismatch)) {
      byIrrep = false;
      why = mismatch;
      whyWarn = true;
    }
  }

  if (byIrrep && columnsCarryIrreps) {
    int matched = 0;
    for (size_t i = 0; i < centre.levels.size(); i++) {
      for (size_t j = 0; j < left.levels.size(); j++) {
        if (!centre.levels[i].irrep.empty() &&
            centre.levels[i].irrep == left.levels[j].irrep) {
          matched++;
          break;
        }
      }
    }
    //  ONLY WHERE THERE ARE LABELS TO DISAGREE.
    //
    //  With no labels at all every comparison misses, so this fired
    //  too and the diagram said both "it reports no orbital symmetry
    //  labels" and "the calculation's orbital labels are not irreps of
    //  Td" -- which cannot both be true, and was shown together.
    if (matched == 0 && !centre.levels.empty() && haveSymmetryLabels) {
      byIrrep = false;
      if (why.empty()) {
        ostringstream text;
        text << "The calculation's orbital labels are not irreps of "
             << group << ", so the levels are correlated by what they are "
                "built from rather than by symmetry.";
        why = text.str();
        whyWarn = true;
      }
    }
  }

  string fragmentAbout;
  string simplifiedNote;
  NoteTiers tiers;
  vector<string> pairWarnings;
  if (haveFragments && !foDrawn) {
    //  Without a usable irrep the centre levels carry none, so
    //  connect() matches on the shell a fragment level is and the
    //  share an orbital holds of it.  Left in place they would match
    //  on a spelling that means something different.
    if (!byIrrep && columnsCarryIrreps) {
      for (size_t i = 0; i < centre.levels.size(); i++) {
        centre.levels[i].irrep.clear();
      }
    }

    //  SKELETON (#183): metalShare/metalLigandOP are only ever populated
    //  for this construction (see their computation above), so passing
    //  them unconditionally is equivalent to gating on p_fragmentation
    //  and changes nothing for any other construction.
    const bool useMetalLigand = (p_fragmentation == MoFragments::SKELETON);

    progressDlg.progress(90, _("Classifying orbitals"));
    MoDiagram::classify(left.levels, centre.levels, right.levels,
                        left.fromHalves, localisedShare,
                        useOverlapPopulation ? centreOP : vector<double>(),
                        useMetalLigand ? metalShare : vector<double>(),
                        useMetalLigand ? metalLigandOP : vector<double>());
    MoDiagram::connect(left.levels, centre.levels, right.levels, links,
                       useMetalLigand ? 0.10 : MoDiagram::LINK_SHARE,
                       useMetalLigand);

    //  PI-ONLY: THE FRAGMENT COLUMNS MUST DROP TOO, not just the
    //  molecular one -- see the trimming block right after connect()
    //  below (piOnlyApplied).
    if (piOnlyApplied) {
      trimUnconnectedFragment(left, links, true);
      trimUnconnectedFragment(right, links, false);
    }

    const int pinnedShells =
        MoDiagram::placeFragments(centre, left, right, links);
    //  The fraction placeFragments() tests against
    //  NONBONDING_SHELL_SHARE, per molecular level and shell (#140).
    if (mosymDebug()) {
      for (size_t i = 0; i < centre.levels.size(); i++) {
        const MoLevel& mo = centre.levels[i];
        mosymPrintf("[MOSHELL] %s E=%.6f", mo.label.c_str(), mo.energy);
        for (int side = 0; side < 2; side++) {
          const MoColumn& col = (side == 0) ? left : right;
          for (size_t k = 0; k < col.shellKeys.size(); k++) {
            mosymPrintf(" %s=%.6f", col.shellKeys[k].c_str(),
                MoDiagram::nonbondingShellFraction(mo, side == 0, (int)k));
          }
        }
        mosymPrintf("\n");
      }
    }
    //  A main-group fragment's electrons: whole pairs in the placed
    //  energy order, not singly occupied symmetry orbitals.
    const bool centralMainGroup = [&]() {
      if (p_fragmentation != MoFragments::CENTRAL || elements.size() < 3) return false;
      for (size_t i = 0; i < elements.size(); i++)
        if (MoFragments::valenceOrbitals(elements[i]) > 4) return false;
      return true;
    }();
    if (centralMainGroup) MoDiagram::fillWholePairs(right.levels);
    MoDiagram::classifyByEnergy(left.levels, centre.levels,
                                right.levels, links, &pairWarnings);

    //  SAY WHICH OF THE TWO PLACEMENTS ACTUALLY HAPPENED.
    //
    //  placeFragments() puts a fragment level at the share-weighted
    //  mean of the orbitals it became -- deliberately NOT at its
    //  tabulated ionisation energy, which is a free atom's and says
    //  nothing about this molecule.  It falls back to the tabulated
    //  order only when nothing connected at all.  This note claimed
    //  the fallback unconditionally, so in the normal case it
    //  described the opposite of what had been done.
    //  NOTHING CORRELATED MEANS THERE IS NO CORRELATION DIAGRAM.
    //
    //  Drawing the fragment columns anyway puts two sets of levels
    //  side by side with no relationship between them and invites the
    //  reader to infer one.  The fragment energies in that state come
    //  from a table of free-atom ionisation energies, mapped onto the
    //  axis by order alone -- they are not this molecule's, and
    //  nothing about their heights relative to the orbitals means
    //  anything.  Reported as "a complete mess", which it is.
    //
    //  So the columns are dropped and the spectrum is drawn on its
    //  own, which is honest and still useful, with the reason and the
    //  remedy said plainly.
    //
    //  The panel is NOT removed from the Properties menu for this.
    //  A panel whose isRelevant() returns false is dropped silently,
    //  menu entry and all, and an MO diagram that simply is not there
    //  reads as a broken build rather than as a calculation that
    //  cannot support one -- which is exactly how the v8.13.0 hold was
    //  first reported.  Say why instead of disappearing.
    if (links.empty()) {
      left  = MoColumn();
      right = MoColumn();
      haveFragments = false;

      vector<string> noDiagram;
      noDiagram.push_back("No correlation diagram can be drawn for this calculation.");
      if (!haveSymmetryLabels) {
        //  NOT NAMED BY CODE.  This said "Gaussian jobs are generated
        //  with symmetry switched off", which was shown verbatim on an
        //  ORCA calculation.  The remedy is the same wherever the
        //  calculation came from, and it is now in one place.
        noDiagram.push_back("It reports no orbital symmetry labels, and without them "
                "there is nothing to match the fragment orbitals against. "
                "A job run with symmetry switched off reports none: tick "
                "\"Use symmetry\" in the Calculation Editor, beside the "
                "point group, and run it again.");
      } else {
        noDiagram.push_back("Its orbital labels could not be reconciled with the "
                "symmetry of the structure, so the fragment orbitals and "
                "the orbitals cannot be matched up. Running Find Symmetry "
                "on the structure, so that it agrees with the symmetry the "
                "calculation used, is usually what is needed.");
      }
      {
        string first = noDiagram[0];
        for (size_t k = 1; k < noDiagram.size(); k++) first += " " + noDiagram[k];
        tiers.warnings.push_back(first);
      }
      if (!why.empty()) tiers.warnings.push_back(why);
      if (!foNotOffered.empty()) tiers.warnings.push_back(foNotOffered);
      if (!symmetryWarning.empty()) tiers.warnings.push_back(symmetryWarning);
      if (!subgroupWhy.empty()) tiers.warnings.push_back(subgroupWhy);
      tiers.summary = "Molecular levels only \xC2\xB7 energies in Hartree";
      if (!group.empty() && haveSymmetryLabels)
        tiers.summary += " \xC2\xB7 labels in " + FoDiagram::groupMarkup(group);
      tiers.about = aboutText(
          "The orbital energies are shown on their own, without fragment "
          "columns or lines.",
          string(), haveBeta ? betaText : string(),
          haveSymmetryLabels ? string() : unlabelledText,
          symmetryNote, group == symmetrisedGroup ? symmetrisedNote : string(),
          centre, left, right);

      //  SAID ONCE, LOUDLY, AT THE TOP.
      //
      //  The explanation was the second sentence of a grey footnote
      //  under a spectrum that looks like a diagram with its sides
      //  missing, and it was read as "the MO diagram is broken" --
      //  which it was not; the calculation had been run with symmetry
      //  switched off, and nothing can be drawn from one that was.
      const string headline = haveSymmetryLabels
          ? "The orbital labels do not match the structure's symmetry"
          : "This job was run WITHOUT SYMMETRY - no diagram can be drawn";

      progressDlg.progress(97, _("Drawing"));
      p_canvas->setGroup(shownGroup(group, coords));
      p_canvas->setFormula(MoDiagram::formula(elements, charge));
      p_canvas->setDiagram(left, centre, right, links, false, "", headline);
      p_canvas->setCalcHeader(levelSource(getCalculation(), true));
  p_lastNote = levelSource(getCalculation()) + tiers.summary;
      for (size_t k = 0; k < tiers.warnings.size(); k++) p_lastNote += "\n" + tiers.warnings[k];
      p_canvas->setNotes(levelSource(getCalculation()) + tiers.summary,
                         tiers.warnings, tiers.about);
      if (!dumpPath.empty()) {
        dumpMoDiagram(dumpPath, group, MoDiagram::formula(elements, charge),
            MoFragments::fragmentationName(p_fragmentation),
            headline, tiers, left, centre, right, links,
            localisedShare, centreOP, mullikenMaxVec, normVec, metalLigandOP);
        exitAfterDumpIfRequested();
      }
      return;
    }

    tiers.warnings.insert(tiers.warnings.end(), pairWarnings.begin(), pairWarnings.end());
    if (centralMainGroup && !(p_showAll != 0 && p_showAll->IsChecked())) {
      vector<bool> gone;
      int leftOut = 0, blockLevels = 0;
      const size_t linksBefore = links.size();
      MoDiagram::simplifyCentral(left, centre, right, links, gone,
                                 leftOut, blockLevels);
      vector<double>* parallel[] = { &localisedShare, &centreOP, &mullikenMaxVec,
                                     &normVec, &metalLigandOP };
      for (size_t v = 0; v < sizeof parallel/sizeof parallel[0]; v++) {
        if (parallel[v]->size() != gone.size()) continue;
        vector<double> keep;
        for (size_t i = 0; i < gone.size(); i++)
          if (!gone[i]) keep.push_back((*parallel[v])[i]);
        parallel[v]->swap(keep);
      }
      if (leftOut > 0 || blockLevels > 0 || links.size() != linksBefore) {
      simplifiedNote = "Lines to non-bonding levels are not drawn";
      if (leftOut > 0)
        simplifiedNote += "; " + std::to_string(leftOut) + " ligand s orbital" +
                          (leftOut == 1 ? " is" : "s are") + " left out (under 10 % of any level drawn)";
      if (blockLevels > 0)
        simplifiedNote += "; the ligands' non-bonding levels are one grey block";
      simplifiedNote += ". Tick Show all levels to draw everything.";
      }
    }
    fragmentAbout = "Each fragment level is placed at the mean of the molecular "
        "orbitals it became, weighted by its share of them, and kept in the "
        "atom's own shell order. Its free-atom energy is given beside it.";
    if (pinnedShells > 0) {
      char share[16];
      snprintf(share, sizeof share, "%d %%",
               (int)lround(100.0*MoDiagram::NONBONDING_SHELL_SHARE));
      fragmentAbout += string(" A shell with a non-bonding orbital (one that is at "
          "least ") + share + " that shell, of the orbitals drawn) is drawn at "
          "that orbital's energy instead.";
    }
  }

  if (!simplifiedNote.empty()) fragmentAbout += "\n\n**Simplified.** " + simplifiedNote;
  if (p_ligandField != 0)
    p_ligandField->Enable(modelDrawn && (modelLfApplicable || p_ligandField->IsChecked()));
  if (p_showAll != 0)
    p_showAll->Enable((foDrawn && !modelDrawn) ||
                      (modelDrawn && (modelFolded || p_showAll->IsChecked())) ||
                      (haveFragments && !modelDrawn && p_fragmentation == MoFragments::CENTRAL));
  if (!why.empty()) {
    if (whyWarn) tiers.warnings.push_back(why);
  }
  if (!foNotOffered.empty()) tiers.warnings.push_back(foNotOffered);
  if (!symmetryWarning.empty()) tiers.warnings.push_back(symmetryWarning);
  if (!subgroupWhy.empty()) tiers.warnings.push_back(subgroupWhy);
  //  What the pick means, for the summary line and the About text.
  string pickSummary, pickAbout;
  if (pickActive && haveFragments) {
    const string dash = "\xE2\x80\x93";
    if (foDrawn && pickCentre >= 0 && !modelDrawn)
      pickSummary = pickLabelA + " against " + foPieces;
    else if (foDrawn && !modelDrawn)
      pickSummary = pickLabelOfMetal(elements, pickMetalAtom) +
                    " against its ligands";
    else if (p_fragmentation == MoFragments::CENTRAL)
      pickSummary = pickLabelA + " against its terminal atoms";
    else
      pickSummary = "the two sides of " + pickLabelA + dash + pickLabelB;
    pickAbout = "**Picked atoms.** One atom is drawn against the atoms bonded "
        "to it; two bonded atoms cut that bond and the two sides are drawn "
        "against each other (in and out of phase when they are equal). A "
        "metal gives the fragment-orbital diagram, each ligand being a "
        "connected piece left when the metal is removed.";
    if (pickCentre >= 0) {
      pickAbout += " A single atom that is not a metal is drawn the same way "
          "when it is not fixed by every symmetry operation of the molecule, "
          "or when atoms are bonded to neither it nor its neighbours: the "
          "atom against the connected pieces left when it is removed, each "
          "piece neutral.";
    }
  }
  if (foDrawn) {
    tiers.summary = foSummary;
    if (!pickSummary.empty()) {
      const string head = "Fragment levels from extended H\xC3\xBC" "ckel (qualitative)";
      if (tiers.summary.compare(0, head.size(), head) == 0)
        tiers.summary = "Fragments: " + pickSummary +
                        " (extended H\xC3\xBC" "ckel, qualitative)" +
                        tiers.summary.substr(head.size());
    }
    tiers.warnings.insert(tiers.warnings.begin(), foWarnings.begin(),
                          foWarnings.end());
    //  The fragment-orbital text is complete as it stands; what the panel
    //  adds follows it.
    string extra;
    if (!why.empty() && !whyWarn) extra += "\n\n" + why;
    if (haveBeta) extra += "\n\n**Unrestricted calculation.** " + betaText;
    if (!symmetryNote.empty()) extra += "\n\n**Orbital labels.** " + symmetryNote;
    if (!subgroupNote.empty()) extra += "\n\n" + subgroupNote;
    if (group == symmetrisedGroup) extra += "\n\n" + symmetrisedNote;
    tiers.about = foAbout + extra;
    if (!pickAbout.empty()) tiers.about += "\n\n" + pickAbout;
    if (modelDrawn) {
      tiers.verdict = modelVerdict;
      tiers.verdictOk = modelVerdictOk;
      if (!group.empty() && !s.empty())
        tiers.summary += " \xC2\xB7 labels in " + FoDiagram::groupMarkup(group) +
            (fullGroup.empty() ? string() : " (lowered from " + FoDiagram::groupMarkup(fullGroup) + " by the pick)");
    }
  } else {
    string construction = MoFragments::fragmentationName(p_fragmentation);
    tiers.summary = haveFragments && !construction.empty()
        ? "Fragments: " + (pickSummary.empty() ? construction : pickSummary)
        : string("Molecular levels only");
    tiers.summary += " \xC2\xB7 energies in Hartree";
    if (!simplifiedNote.empty()) tiers.summary += " \xC2\xB7 simplified (Show all levels draws everything)";
    if (!group.empty() && !s.empty())
      tiers.summary += " \xC2\xB7 labels in " + FoDiagram::groupMarkup(group) +
          (fullGroup.empty() ? string() : " (lowered from " + FoDiagram::groupMarkup(fullGroup) + " by the pick)");
    else if (s.empty())
      tiers.summary += " \xC2\xB7 no symmetry labels";
    const int hiddenCores = centre.hiddenCount + centre.hiddenSemicoreCount;
    if (hiddenCores > 0)
      tiers.summary += " \xC2\xB7 " + std::to_string(hiddenCores) +
                       (centre.hiddenSemicoreCount > 0 ? " core and semicore orbitals"
                        : hiddenCores == 1 ? " core orbital" : " core orbitals") +
                       " hidden";
    tiers.about = aboutText(
        (why.empty() || whyWarn) ? string() : why, fragmentAbout,
        haveBeta ? betaText : string(),
        s.empty() ? unlabelledText : string(), symmetryNote,
        group == symmetrisedGroup ? symmetrisedNote : string(),
        centre, left, right);
    if (!pickAbout.empty()) tiers.about += "\n\n" + pickAbout;
  }

  progressDlg.progress(97, _("Drawing"));
  p_canvas->setGroup(shownGroup(group, coords));
  //  The fragment-orbital diagram heads a complex with its ligands grouped.
  string headingFormula = MoDiagram::formula(elements, charge);
  if (foDrawn) {
    const string grouped =
        FoDiagram::complexFormula(storedSymbols, storedCoords, charge);
    if (!grouped.empty()) headingFormula = grouped;
  }
  p_canvas->setFormula(headingFormula);
  //  A pick that could not be drawn says so across the top, as the other
  //  refusals do; the reason is in the lines under the diagram.
  const string pickHeadline = (pickActive && !haveFragments && !why.empty())
      ? "The picked atoms cannot be drawn as two fragments" : string();
  p_canvas->setDiagram(left, centre, right, links, haveFragments, "", pickHeadline);
  p_canvas->setCalcHeader(levelSource(getCalculation(), true));
  p_lastNote = levelSource(getCalculation()) + tiers.summary;
  for (size_t k = 0; k < tiers.warnings.size(); k++) p_lastNote += "\n" + tiers.warnings[k];
  p_canvas->setNotes(levelSource(getCalculation()) + tiers.summary,
                     tiers.warnings, tiers.about);
  p_canvas->setVerdict(tiers.verdict, tiers.verdictOk);
  if (!dumpPath.empty()) {
    dumpMoDiagram(dumpPath, group, headingFormula,
        MoFragments::fragmentationName(p_fragmentation),
        pickHeadline, tiers, left, centre, right, links,
        localisedShare, centreOP, mullikenMaxVec, normVec, metalLigandOP);

    //  The fragment-orbital ligand-field MODEL (#162 follow-up): only
    //  for SKELETON, and only when a metal atom and a usable overlap
    //  matrix are both in hand -- exactly the inputs MoLigandField::
    //  build() declines without. Gated on the dump path, like the model
    //  computation itself: it is real linear algebra (a handful of
    //  small generalised eigenproblems, run three times over for the
    //  robustness scan), not something every interactive rebuild should
    //  pay for before the four-column canvas that will actually show it
    //  exists (a separate follow-up).
    if (p_fragmentation == MoFragments::SKELETON && metalAtomIndex >= 0 &&
        moHaveSqrtS && !moCoefficients.empty()) {
      MoLigandField::Result lfResult;
      MoLigandField::build(coords, elements, metalAtomIndex, e, o, moCoefficients,
                           moPerAtom, moShellOf, moSflat, moSqrtS, haveBeta, lfResult);
      dumpLigandFieldModel(dumpPath, lfResult);
    }

    exitAfterDumpIfRequested();
  }
}

#include <algorithm>
#include <cmath>
#include <ios>
#include <set>
#include <sstream>
#include <utility>

#include <wx/dcbuffer.h>
#include <wx/link.h>
#include <wx/sizer.h>

#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"
#include <wx/checklst.h>

#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxStaticText.H"

#include "tdat/MoComposition.H"
#include "tdat/MoFragments.H"
#include "tdat/PropTable.H"
#include "tdat/TGBSAngFunc.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TGaussianBasisSet.H"
#include "dsm/ICalculation.H"
#include "dsm/ICalcUtils.H"
#include "dsm/JCode.H"
#include "tdat/CharacterTable.H"
#include "tdat/SymmetryOps.H"
#include "tdat/SymmetryAnalysis.H"
#include "tdat/ShellRotation.H"
#include "tdat/BasisFlatten.H"
#include "tdat/EspField.H"
#include "tdat/TAtm.H"

#include "dsm/IPropCalculation.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "wxviz/WxVizToolFW.H"

#include "MoPanel.H"
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
  : p_canvas(0), p_fragments(0), p_autoFragments(0), p_construction(0), p_piOnly(0),
    p_fragmentation(MoFragments::NOT_BUILT)
{
}


MoDiagramPanel::MoDiagramPanel(IPropCalculation *calculation,
      wxWindow *parent, wxWindowID id, const wxPoint& pos,
      const wxSize& size, long style, const wxString& name)
  : p_canvas(0), p_fragments(0), p_autoFragments(0), p_construction(0), p_piOnly(0),
    p_fragmentation(MoFragments::NOT_BUILT)
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
static bool countFunctions(TGBSConfig *config, const vector<TAtm*>& atoms,
                           bool cartesian, vector<int>& counts,
                           vector<int>& shellOf)
{
  counts.clear();
  shellOf.clear();

  bool ok = true;
  for (size_t a = 0; a < atoms.size(); a++) {
    const string symbol = atoms[a]->atomicSymbol();
    int here = 0;

    vector<const TGaussianBasisSet*> list = config->getGBSList(symbol);
    for (size_t g = 0; g < list.size(); g++) {
      const TGaussianBasisSet *gbs = list[g];
      if (gbs == 0) continue;
      const int sets = gbs->num_contracted_sets(symbol.c_str());
      for (int ics = 0; ics < sets; ics++) {
        vector<TGaussianBasisSet::AngularMomentum> types =
            gbs->func_types(symbol.c_str(), ics);
        for (size_t t = 0; t < types.size(); t++) {
          const int l = (int)types[t];
          //  (l+1)(l+2)/2 Cartesian functions in a shell, 2l+1
          //  spherical ones -- six Cartesian d against five spherical.
          const int inShell = cartesian ? ((l+1)*(l+2))/2 : (2*l + 1);
          here += inShell;
          //  The angular momentum of each function, in the order the
          //  coefficients are stored.  A diatomic can be connected by
          //  nothing else: both its atoms carry exactly half of every
          //  orbital, so which SHELL a sigma-g came from is the only
          //  question with an answer.
          for (int f = 0; f < inShell; f++) shellOf.push_back(l);
        }
      }
    }
    if (here == 0) ok = false;
    counts.push_back(here);
  }

  return ok && !counts.empty();
}

static bool functionsPerAtom(IPropCalculation *expt, SGFragment *sgfrag,
                             int coefficientWidth,
                             vector<int>& counts, vector<int>& shellOf)
{
  counts.clear();
  shellOf.clear();

  ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
  if (escalc == 0 || sgfrag == 0) return false;

  TGBSConfig *config = escalc->gbsConfig();

  //  A semiempirical code writes no basis set; rebuild one from the
  //  Slater exponents it did report, as MoPanel and ComputeMoCmd both
  //  do.  Each fetches the config independently.
  if (config == 0 || config->empty()) {
    TGBSConfig *slater = ICalcUtils::slaterBasisConfig(expt);
    if (slater != 0) { delete config; config = slater; }
  }
  if (config == 0 || config->empty()) { delete config; return false; }

  const JCode *cap = escalc->application();
  TGBSAngFunc *angfunc = (cap == 0) ? 0 : cap->getAngFunc(config->coordsys());
  const bool recordedCartesian =
      (angfunc != 0 && angfunc->basisType() == TGBSAngFunc::Cartesian);
  delete angfunc;

  vector<TAtm*> *atoms = sgfrag->atoms();
  if (atoms == 0) { delete config; return false; }

  //  THE RECORDED SYSTEM FIRST, the other only if it does not
  //  reproduce the coefficient table's width -- same rule
  //  computeFullGroupLabels() uses, and for the same reason: the
  //  recorded convention is right far more often, so it is not worth
  //  silently preferring whichever happens to match when both do.
  bool ok = false;
  const bool tries[2] = { recordedCartesian, !recordedCartesian };
  for (int t = 0; t < 2 && !ok; t++) {
    vector<int> trialCounts, trialShellOf;
    if (!countFunctions(config, *atoms, tries[t], trialCounts, trialShellOf))
      continue;
    int total = 0;
    for (size_t i = 0; i < trialCounts.size(); i++) total += trialCounts[i];
    if (coefficientWidth > 0 && total != coefficientWidth) continue;
    counts.swap(trialCounts);
    shellOf.swap(trialShellOf);
    ok = true;
  }

  delete atoms;
  delete config;
  return ok;
}


/**
 * The real overlap matrix and per-atom function counts, for the
 * localisation and overlap-population checks classify() runs before
 * any construction-specific bonding rule (#132, 2026-09-27).
 *
 * functionsPerAtom() above gives perAtom/shellOf cheaply, but with no
 * overlap matrix at all -- fine for composeLevels()'s Lowdin
 * (sum-of-squares) composition, useless for an overlap population,
 * which is entirely a cross-atom S element. So this rebuilds the
 * basis the heavier way computeFullGroupLabels() does (BasisFlatten +
 * EspField::overlapOf), in the STORED frame the coefficients are
 * actually in -- same reasoning as that function's header comment --
 * but stops once S exists, since nothing here needs a character table.
 */
static bool buildBasisOverlap(IPropCalculation *expt, SGFragment *sgfrag,
                              int coefficientWidth,
                              vector<int>& perAtom, vector<double>& Sflat)
{
  perAtom.clear();
  Sflat.clear();

  ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
  if (escalc == 0 || sgfrag == 0) return false;

  TGBSConfig *config = escalc->gbsConfig();
  if (config == 0 || config->empty()) {
    TGBSConfig *slater = ICalcUtils::slaterBasisConfig(expt);
    if (slater != 0) { delete config; config = slater; }
  }
  if (config == 0 || config->empty()) { delete config; return false; }

  const JCode *code = escalc->application();
  if (code == 0) { delete config; return false; }

  vector<TAtm*> *atoms = sgfrag->atoms();
  double *xyz = sgfrag->coordinates();
  const unsigned long natoms = sgfrag->numAtoms();
  if (atoms == 0 || xyz == 0 || natoms == 0 || atoms->size() != natoms) {
    delete atoms;
    delete config;
    return false;
  }
  vector<string> storedElements(natoms);
  vector<double> storedCoords(natoms*3);
  for (unsigned long a = 0; a < natoms; a++) {
    storedElements[a] = (*atoms)[a]->atomicSymbol();
    for (int k = 0; k < 3; k++) storedCoords[a*3+k] = xyz[a*3+k];
  }
  delete atoms;

  //  THE RECORDED SYSTEM FIRST -- same two-try rule as
  //  computeFullGroupLabels() and functionsPerAtom(), for the same
  //  reason: the coefficient table's width alone decides Cartesian vs
  //  spherical, and the recorded convention is right far more often.
  vector<EspBasisFunction> basis;
  {
    int lengthShellCart[7] = { 1, 3, 6, 10, 15, 21, 28 };
    int lengthShellSph[7]  = { 1, 3, 5, 7, 9, 11, 13 };
    const TGaussianBasisSet::CoordinateSystem recorded = config->coordsys();
    const TGaussianBasisSet::CoordinateSystem other =
        (recorded == TGaussianBasisSet::Spherical)
          ? TGaussianBasisSet::Cartesian : TGaussianBasisSet::Spherical;
    const TGaussianBasisSet::CoordinateSystem tries[2] = { recorded, other };
    for (int t = 0; t < 2 && basis.empty(); t++) {
      TGBSAngFunc *candidate = code->getAngFunc(tries[t]);
      if (candidate == 0) continue;
      const bool sph = (tries[t] == TGaussianBasisSet::Spherical);
      vector<EspBasisFunction> trial;
      if (BasisFlatten::flatten(storedElements, storedCoords, config, code,
                                candidate, candidate->maxShells(),
                                sph ? lengthShellSph : lengthShellCart,
                                trial) &&
          (int)trial.size() == coefficientWidth) {
        basis.swap(trial);
      }
      delete candidate;
    }
  }
  delete config;
  if (basis.empty()) return false;
  for (size_t i = 0; i < basis.size(); i++) if (basis[i].empty()) return false;

  perAtom.clear();
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
  if (perAtom.size() != natoms) { perAtom.clear(); return false; }

  const size_t nbasis = basis.size();
  Sflat.assign(nbasis*nbasis, 0.0);
  for (size_t i = 0; i < nbasis; i++)
    for (size_t j = 0; j < nbasis; j++)
      Sflat[i*nbasis + j] = EspField::overlapOf(basis[i], basis[j]);

  return true;
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
  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);

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
  p_piOnly->SetValue(false);
  p_piOnly->Show(false);
  p_piOnly->SetToolTip("Show only the orbitals that change sign in the "
                       "molecular plane");
  row->Add(p_piOnly, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);

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
  row->Add(new ewxStaticText(this, wxID_ANY, _("Built as:")),
           0, wxALIGN_CENTER_VERTICAL|wxLEFT, 4);
  row->Add(p_construction, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);

  p_autoFragments = new ewxCheckBox(this, wxID_ANY, _("Choose fragments"));
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
                              "yourself, instead of the construction "
                              "the diagram chose");
  row->Add(p_autoFragments, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);

  //  SAY WHAT A FRAGMENT IS, ON THE PANEL.
  //
  //  "Choose fragments" with a list of atom counts beside it does not
  //  explain itself, and the explanation was in a tooltip, which is
  //  invisible until you already suspect there is something to learn.
  //  A diagram correlates a molecule against TWO groups of atoms, and
  //  the only thing being chosen here is which group each set goes in.
  p_fragmentHint = new ewxStaticText(this, wxID_ANY,
      _("Ticked sets become the left column, the rest the right"));
  row->Add(p_fragmentHint, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);

  p_fragments = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition,
                                   wxSize(220, 70));
  p_fragments->SetToolTip("Each row is a set of atoms the symmetry makes "
                          "equivalent. Ticked sets form the left column of "
                          "the diagram; the rest form the right.");
  row->Add(p_fragments, 1, wxEXPAND|wxALL, 4);

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
  row->Add(p_showEnergies, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);

  sizer->Add(row, 0, wxEXPAND);

  //  Bound on the controls themselves.  A static table does not reach
  //  this panel for a control with a pushed handler chain, which is
  //  what the ewx classes install -- the same reason #81's radio box
  //  never received its event.
  p_autoFragments->Bind(wxEVT_CHECKBOX, &MoDiagramPanel::onFragmentChanged,
                        this);
  p_construction->Bind(wxEVT_CHOICE, &MoDiagramPanel::onFragmentChanged,
                       this);
  p_piOnly->Bind(wxEVT_CHECKBOX, &MoDiagramPanel::onFragmentChanged, this);
  p_fragments->Bind(wxEVT_CHECKLISTBOX, &MoDiagramPanel::onFragmentChanged,
                    this);
  p_showEnergies->Bind(wxEVT_CHECKBOX, &MoDiagramPanel::onShowEnergies, this);

  p_fragments->Enable(false);
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
  //  The energies change how much room each row needs, so the whole
  //  layout is recomputed, not just repainted.
  p_canvas->Refresh();
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
  static bool mosymFail(int where, const char *why)
  {
    if (getenv("ECCE_DEBUG_MOSYM")) {
      fprintf(stderr, "[MOSYM] full-group labels not computed (step %d): %s\n",
              where, why);
      fflush(stderr);
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
  void matchAlphaBeta(const vector<string>& sAlpha, const vector<string>& sBeta,
                      int nAlpha, int nBeta, vector<int>& betaOfAlpha)
  {
    betaOfAlpha.assign(nAlpha, -1);
    const bool haveLabels = (int)sAlpha.size() == nAlpha &&
        (int)sBeta.size() == nBeta && !sAlpha.empty() && !sBeta.empty();
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
    } else {
      for (int i = 0; i < nAlpha && i < nBeta; i++) betaOfAlpha[i] = i;
    }
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
                              const char *moPropertyName = "MO")
  {
    if (getenv("ECCE_DEBUG_MOSYM"))
      fprintf(stderr, "[MOSYM] group %s, %d orbitals, %d reported labels\n",
              group.c_str(), (int)e.size(), (int)reported.size());
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
          if (t == 1 && getenv("ECCE_DEBUG_MOSYM"))
            fprintf(stderr, "[MOSYM] coefficient width matches the %s "
                    "basis, not the recorded one\n",
                    sph ? "spherical" : "Cartesian");
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
    for (int i = 0; i < nbasis; i++)
      for (int j = 0; j < nbasis; j++)
        S[i][j] = EspField::overlapOf(basis[i], basis[j]);

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
    const int fullLabelled = SymmetryAnalysis::fullLabelSpectrum(
        orbitals, e, perAtom, shellTypeOf, S, images, classOfOp,
        opsInCoeffFrame, angfunc, *table, 1.0e-4, fullDerived);
    if (fullLabelled <= 0) return mosymFail(131, "fullLabelled <= 0");

    //  NO CODE LABELS TO CROSS-CHECK AGAINST (#132).
    //
    //  The subgroup cross-check below is the only independent evidence
    //  this function otherwise insists on, and it needs the code's own
    //  labels to exist at all.  Where they do not, accept the computed
    //  full-group labels on a weaker but still real condition instead:
    //  every orbital in the calculation must have been labelled, not
    //  just the ones later shown.  A partial labelling here would mean
    //  some shell the angular table or the projection could not handle,
    //  which is exactly the situation the cross-check exists to catch,
    //  and there is no second source to catch it instead.
    if (reported.empty()) {
      if (fullLabelled != norb)
        return mosymFail(132, "reported.empty() && fullLabelled != norb");

      computed.assign(e.size(), string());
      for (int i = 0; i < norb && i < (int)computed.size(); i++) {
        if (!fullDerived[i].empty()) computed[i] = fullDerived[i];
      }
      ostringstream text;
      text << "Symmetry labels computed from the orbitals in " << group
           << "; the code reported none.";
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
      string a = subDerived[i], b = reported[i];
      for (size_t c = 0; c < a.size(); c++) a[c] = toupper((unsigned char)a[c]);
      for (size_t c = 0; c < b.size(); c++) b[c] = toupper((unsigned char)b[c]);
      if (a == b) agree++;
    }
    //  ALL OR NOTHING: a single disagreement means the axis convention
    //  or the frame premise is wrong somewhere, and a diagram is not
    //  the place to find out which -- keep the code's own labels.
    if (getenv("ECCE_DEBUG_MOSYM"))
      fprintf(stderr, "[MOSYM] subgroup %s cross-check: %d of %d agree\n",
              subgroupName.c_str(), agree, checked);
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


void MoDiagramPanel::build()
{
  if (p_canvas == 0) return;

  IPropCalculation *calc = getCalculation();
  if (calc == 0) return;

  PropVector    *energies = (PropVector*)calc->getProperty("ORBENG");
  PropVector    *occs     = (PropVector*)calc->getProperty("ORBOCC");
  PropVecString *syms     = (PropVecString*)calc->getProperty("ORBSYM");

  MoColumn left, centre, right;
  vector<MoConnection> links;

  if (energies == 0) {
    p_canvas->setDiagram(left, centre, right, links, false,
                         "This calculation has no orbital energies.");
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
  vector<double> coords;
  vector<string> elements;

  WxVizToolFW& fw = getFW();
  SGFragment *sgfrag = fw.getSceneGraph().getFragment();

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
  {
    vector<string> computedLabels;
    string computedNote;
    if (computeFullGroupLabels(calc, group, table, sgfrag, coords, elements,
                               e, s, computedLabels, computedNote)) {
      s = computedLabels;
      labelsInFullGroup = true;
      if (why.empty()) why = computedNote;
    }
  }

  //  THE SAME, FOR BETA -- computed only when the code did not report
  //  ORBSYMBETA itself, since that is what the alpha-beta irrep match
  //  above needs to prefer irrep over energy order (#132).
  if (haveBeta && sB.empty()) {
    vector<string> computedBetaLabels;
    string computedBetaNote;
    if (computeFullGroupLabels(calc, group, table, sgfrag, coords, elements,
                               eB, sB, computedBetaLabels, computedBetaNote,
                               "MOBETA")) {
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
  }

  //  Which beta orbital (an index into eB/oB/sB) matches each alpha
  //  orbital (an index into e/o/s) -- computed once, here, in the same
  //  spelling `s` ends up in, then read back per drawn LEVEL below once
  //  groupByIrrep() has grouped the alpha orbitals into levels.
  vector<int> betaOfAlpha;
  if (haveBeta) matchAlphaBeta(s, sB, (int)e.size(), (int)eB.size(), betaOfAlpha);

  MoDiagram::groupByIrrep(e, o, s, dimensions, 1.0e-4, centre.levels);
  centre.title = "Molecular orbitals";

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

  //  Both ends of the spectrum are folded away, not just the core, and
  //  both cutoffs report what they hid so an absence the reader cannot
  //  see does not pass for a complete diagram.
  MoDiagram::hideBelow(centre, MoDiagram::suggestCoreCutoff(centre.levels));
  MoDiagram::hideAbove(centre,
                       MoDiagram::suggestVirtualCutoff(centre.levels));

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
    MoDiagram::hideBeyondValence(centre, valenceRoom);
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

  ostringstream note;
  if (s.empty()) {
    note << "This calculation reports no orbital symmetry labels, so the "
            "molecular levels are unlabelled and are correlated by what "
            "they are built from rather than by symmetry.";
  } else {
    note << "Energies in Hartree.";
  }
  if (haveBeta) {
    note << "  Unrestricted: levels at alpha energies; beta electrons "
            "matched by symmetry.";
  }

  //  --- the fragment columns -----------------------------------------
  //
  //  A correlation diagram is a symmetry argument, so these depend on
  //  the geometry and the point group and not on the calculation.  The
  //  molecular orbitals are still drawn when this cannot be made: a
  //  level diagram on its own is useful, and refusing to draw anything
  //  would be worse than saying why the rest is missing.
  bool haveFragments = false;

  //  The charge, so the fragment levels carry the right electron
  //  count -- NO2- has one more than its atoms bring -- and so the
  //  heading can show it, which is what a reader checks the formula
  //  for.
  int charge = 0;
  {
    Fragment *frag = fw.getSceneGraph().getFragment();
    if (frag != 0) charge = (int)frag->charge();
  }

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
        if (why.empty()) why = text.str();
        group = best;
      }
    }
  }

  //  Which atoms actually ended up in each column -- needed below to
  //  compute each molecular level's composition, not just its irrep.
  vector<int> leftAtoms, rightAtoms;

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

    haveFragments = MoFragments::build(coords, elements, group, charge,
                                       left, right, why, &leftAtoms,
                                       &rightAtoms, chosen,
                                       &p_fragmentation, want);
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
  //  than threaded into it.
  vector<double> localisedShare, centreOP;
  if (haveFragments && !leftAtoms.empty() && !rightAtoms.empty()) {
    PropTable *moCoefs = (PropTable*)calc->getProperty("MO");
    if (moCoefs != 0 && moCoefs->rows() > 0 && moCoefs->columns() > 0) {
      vector<int> perAtom, shellOf;
      //  functionsPerAtom() reads the STORED frame's atoms (sgfrag's
      //  own), which is the order the coefficients are in -- the same
      //  frame computeFullGroupLabels() insists on for the same reason.
      if (functionsPerAtom(calc, sgfrag, moCoefs->columns(), perAtom,
                           shellOf)) {
        const int norb = moCoefs->rows();
        const int nbasis = moCoefs->columns();
        vector< vector<double> > coefficients(norb, vector<double>(nbasis));
        for (int m = 0; m < norb; m++)
          for (int mu = 0; mu < nbasis; mu++)
            coefficients[m][mu] = moCoefs->value(m, mu);

        MoFragments::composeLevels(centre.levels, coefficients, perAtom,
                                   shellOf, elements, leftAtoms, rightAtoms,
                                   left, right);

        vector<int> perAtomS;
        vector<double> Sflat;
        if (buildBasisOverlap(calc, sgfrag, nbasis, perAtomS, Sflat)) {
          vector< std::pair<int,int> > bonds;
          MoFragments::covalentBonds(coords, elements, bonds);

          const bool debug = (getenv("ECCE_DEBUG_MOSYM") != 0);
          localisedShare.assign(centre.levels.size(), 0.0);
          centreOP.assign(centre.levels.size(), 0.0);
          const vector<int> noShellSplit;
          for (size_t i = 0; i < centre.levels.size(); i++) {
            MoLevel& level = centre.levels[i];
            vector<double> atomShareSum(elements.size(), 0.0);
            double opSum = 0.0;
            int ncomp = 0;
            for (size_t k = 0; k < level.orbitals.size(); k++) {
              const int mo = level.orbitals[k];
              if (mo < 0 || mo >= (int)coefficients.size()) continue;
              ncomp++;

              //  shellOf left empty: one entry per ATOM, already
              //  summed over its shells -- exactly what a "which one
              //  atom does this level sit on" check wants.
              const vector<MoComposition::Share> full =
                  MoComposition::compute(coefficients[mo], perAtomS,
                                         noShellSplit, elements, Sflat);
              for (size_t f = 0; f < full.size(); f++) {
                if (full[f].atom >= 0 &&
                    full[f].atom < (int)atomShareSum.size()) {
                  atomShareSum[full[f].atom] += full[f].share;
                }
              }

              opSum += MoComposition::overlapPopulation(coefficients[mo],
                                                         perAtomS, Sflat,
                                                         bonds);
            }
            if (ncomp > 0) {
              double maxShare = 0.0;
              for (size_t a = 0; a < atomShareSum.size(); a++) {
                const double v = atomShareSum[a]/ncomp;
                if (v > maxShare) maxShare = v;
              }
              localisedShare[i] = maxShare;
              centreOP[i] = opSum/ncomp;
              if (debug) {
                fprintf(stderr, "[MOLOC] %s E=%.4f maxAtomShare=%.3f "
                        "OP=%.4f\n", level.irrep.c_str(), level.energy,
                        maxShare, centreOP[i]);
              }
            }
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
  bool byIrrep = haveFragments;

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
  if (linearMolecule) {
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
      }
    }
  }

  if (haveFragments) {
    //  Without a usable irrep the centre levels carry none, so
    //  connect() matches on the shell a fragment level is and the
    //  share an orbital holds of it.  Left in place they would match
    //  on a spelling that means something different.
    if (!byIrrep && columnsCarryIrreps) {
      for (size_t i = 0; i < centre.levels.size(); i++) {
        centre.levels[i].irrep.clear();
      }
    }

    MoDiagram::classify(left.levels, centre.levels, right.levels,
                        left.fromHalves, localisedShare,
                        useOverlapPopulation ? centreOP : vector<double>());
    MoDiagram::connect(left.levels, centre.levels, right.levels, links);

    //  PI-ONLY: THE FRAGMENT COLUMNS MUST DROP TOO, not just the
    //  molecular one -- see the trimming block right after connect()
    //  below (piOnlyApplied).
    if (piOnlyApplied) {
      trimUnconnectedFragment(left, links, true);
      trimUnconnectedFragment(right, links, false);
    }

    MoDiagram::placeFragments(centre, left, right, links);
    MoDiagram::classifyByEnergy(left.levels, centre.levels,
                                right.levels, links);

    //  SAY WHICH OF THE TWO PLACEMENTS ACTUALLY HAPPENED.
    //
    //  placeFragments() puts a fragment level at the share-weighted
    //  mean of the orbitals it became -- deliberately NOT at its
    //  tabulated ionisation energy, which is a free atom's and says
    //  nothing about this molecule.  It falls back to the tabulated
    //  order only when nothing connected at all.  This note claimed
    //  the fallback unconditionally, so in the normal case it
    //  described the opposite of what had been done.
    note << "  Molecular levels are the calculation's own orbital "
            "energies in Hartree.";

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

      note.str("");
      note << "No correlation diagram can be drawn for this calculation.";
      if (!haveSymmetryLabels) {
        //  NOT NAMED BY CODE.  This said "Gaussian jobs are generated
        //  with symmetry switched off", which was shown verbatim on an
        //  ORCA calculation.  The remedy is the same wherever the
        //  calculation came from, and it is now in one place.
        note << "  It reports no orbital symmetry labels, and without them "
                "there is nothing to match the fragment orbitals against. "
                "A job run with symmetry switched off reports none: tick "
                "\"Use symmetry\" in the Calculation Editor, beside the "
                "point group, and run it again.";
      } else {
        note << "  Its orbital labels could not be reconciled with the "
                "symmetry of the structure, so the fragment orbitals and "
                "the orbitals cannot be matched up. Running Find Symmetry "
                "on the structure, so that it agrees with the symmetry the "
                "calculation used, is usually what is needed.";
      }
      note << "  The orbital energies are shown on their own below.";
      if (!why.empty()) note << "  " << why;

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

      p_canvas->setGroup(group);
      p_canvas->setFormula(MoDiagram::formula(elements, charge));
      p_canvas->setDiagram(left, centre, right, links, false, note.str(),
                           headline);
      return;
    }

    note << " Each fragment level is placed at the mean of the "
            "molecular orbitals it became, weighted by its share of "
            "them; its free-atom energy is given beside it.";
  }

  if (!why.empty()) note << "  " << why;

  p_canvas->setGroup(group);
  p_canvas->setFormula(MoDiagram::formula(elements, charge));
  p_canvas->setDiagram(left, centre, right, links, haveFragments, note.str());
}

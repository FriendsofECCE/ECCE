#include <algorithm>
#include <cmath>
#include <ios>
#include <sstream>

#include <wx/dcbuffer.h>
#include <wx/link.h>
#include <wx/sizer.h>

#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"
#include <wx/checklst.h>

#include "wxgui/ewxCheckBox.H"

#include "tdat/MoFragments.H"
#include "tdat/PropTable.H"
#include "tdat/TGBSAngFunc.H"
#include "dsm/TGBSConfig.H"
#include "dsm/ICalculation.H"
#include "dsm/ICalcUtils.H"
#include "dsm/JCode.H"
#include "tdat/CharacterTable.H"
#include "tdat/SymmetryOps.H"
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
  : p_canvas(0), p_fragments(0), p_autoFragments(0)
{
}


MoDiagramPanel::MoDiagramPanel(IPropCalculation *calculation,
      wxWindow *parent, wxWindowID id, const wxPoint& pos,
      const wxSize& size, long style, const wxString& name)
  : p_canvas(0), p_fragments(0), p_autoFragments(0)
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
//   The count is checked against the coefficient table's own width by
//   the caller.  It has to be: a mapping that is off by one atom
//   produces a perfectly plausible population rather than an error,
//   and would quietly connect the wrong levels.
/////////////////////////////////////////////////////////////////////////////
static bool functionsPerAtom(IPropCalculation *expt, SGFragment *sgfrag,
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
  const bool cartesian =
      (angfunc != 0 && angfunc->basisType() == TGBSAngFunc::Cartesian);
  delete angfunc;

  vector<TAtm*> *atoms = sgfrag->atoms();
  if (atoms == 0) { delete config; return false; }

  bool ok = true;
  for (size_t a = 0; a < atoms->size(); a++) {
    const string symbol = (*atoms)[a]->atomicSymbol();
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

  delete atoms;
  delete config;
  return ok && !counts.empty();
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

  p_autoFragments = new ewxCheckBox(this, wxID_ANY, _("Choose fragments"));
  p_autoFragments->SetValue(false);
  p_autoFragments->SetToolTip("Group the sets of equivalent atoms yourself "
                              "instead of letting the diagram decide");
  row->Add(p_autoFragments, 0, wxALIGN_CENTER_VERTICAL|wxALL, 4);

  p_fragments = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition,
                                   wxSize(220, 70));
  p_fragments->SetToolTip("Ticked sets form the left column; the rest form "
                          "the right");
  row->Add(p_fragments, 1, wxEXPAND|wxALL, 4);

  sizer->Add(row, 0, wxEXPAND);

  //  Bound on the controls themselves.  A static table does not reach
  //  this panel for a control with a pushed handler chain, which is
  //  what the ewx classes install -- the same reason #81's radio box
  //  never received its event.
  p_autoFragments->Bind(wxEVT_CHECKBOX, &MoDiagramPanel::onFragmentChanged,
                        this);
  p_fragments->Bind(wxEVT_CHECKLISTBOX, &MoDiagramPanel::onFragmentChanged,
                    this);

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


void MoDiagramPanel::onFragmentChanged(wxCommandEvent& event)
{
  event.Skip();
  if (p_canvas == 0) return;
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
  MoDiagram::groupByIrrep(e, o, s, dimensions, 1.0e-4, centre.levels);
  centre.title = "Molecular orbitals";

  //  Both ends of the spectrum are folded away, not just the core, and
  //  both cutoffs report what they hid so an absence the reader cannot
  //  see does not pass for a complete diagram.
  MoDiagram::hideBelow(centre, MoDiagram::suggestCoreCutoff(centre.levels));
  MoDiagram::hideAbove(centre,
                       MoDiagram::suggestVirtualCutoff(centre.levels));

  for (size_t i = 0; i < centre.levels.size(); i++) {
    ostringstream text;
    text.setf(std::ios::fixed);
    text.precision(4);
    text << centre.levels[i].energy;
    centre.levels[i].annotation = text.str();
  }

  ostringstream note;
  if (s.empty()) {
    note << "This calculation reports no orbital symmetry labels, so the "
            "molecular levels are unlabelled and are correlated by what "
            "they are built from rather than by symmetry.";
  } else {
    note << "Energies in Hartree.";
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

  if (!elements.empty()) {
    haveFragments = MoFragments::build(coords, elements, group, charge,
                                       left, right, why);
  }

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

  if (byIrrep) {
    //  Reconcile the axis conventions BEFORE anything is matched on
    //  the names: in C2v the character table and the code need not
    //  agree on which mirror is sigma-v, and water comes out inside
    //  out if they are compared as they stand.
    string mismatch;
    if (!MoDiagram::reconcile(left.levels, right.levels, centre.levels,
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
    if (matched == 0 && !centre.levels.empty()) {
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

    MoDiagram::classify(left.levels, centre.levels, right.levels);
    MoDiagram::connect(left.levels, centre.levels, right.levels, links);
    MoDiagram::placeFragments(centre, left, right, links);
    MoDiagram::classifyByEnergy(left.levels, centre.levels,
                                right.levels, links);

    note << "  Molecular levels are the calculation's own orbital "
            "energies in Hartree. Fragment levels are placed by their "
            "valence ionisation energies (shown in eV), in order and "
            "spacing but not on this axis.";
  }

  if (!why.empty()) note << "  " << why;

  p_canvas->setGroup(group);
  p_canvas->setFormula(MoDiagram::formula(elements, charge));
  p_canvas->setDiagram(left, centre, right, links, haveFragments, note.str());
}

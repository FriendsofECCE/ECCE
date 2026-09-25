#ifndef MODIAGRAMPANEL__H
#define MODIAGRAMPANEL__H

#include <string>
#include <vector>

#include "VizPropertyPanel.H"
#include "tdat/MoDiagram.H"
#include "tdat/MoFragments.H"
#include "MoDiagramCanvas.H"

class wxCheckListBox;
class wxCheckBox;
class wxChoice;
class wxSizer;

using std::string;
using std::vector;

class MoDiagramCanvas;


/**
 * A molecular orbital correlation diagram.
 *
 * Three columns: the central atom's orbitals, the molecular orbitals,
 * and the terminal atoms' symmetry orbitals, with the molecular ones to
 * scale in energy and lines joining what symmetry allows to combine.
 *
 * Opens FLOATING, not docked: three columns side by side do not fit a
 * side pane, which addPropertyPanel() floors at 200 wide and the panels
 * themselves ask 400 of.
 *
 * Degrades rather than refusing.  A molecule with a clear central atom
 * gets the full diagram; one without gets the molecular orbitals alone,
 * labelled by irrep and with their degeneracies and occupancies -- which
 * is still the useful half for anything too big or too unsymmetric to
 * correlate.  Deciding which is what isRelevant() and the layout below
 * are for.
 */
class MoDiagramPanel : public VizPropertyPanel,
                       public MoDiagramClickTarget
{
   DECLARE_DYNAMIC_CLASS(MoDiagramPanel)
   DECLARE_EVENT_TABLE()

   public:

      MoDiagramPanel();
      MoDiagramPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="MO Diagram");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="MO Diagram");
      virtual ~MoDiagramPanel();

      /**
       * A level was clicked: show that orbital in the 3-D viewer.
       *
       * @param orbengIndex  0-based index into ORBENG.  One orbital per
       *                     level even where the level is degenerate --
       *                     the partners' shapes are related by the
       *                     group's own operations, so any one of them
       *                     stands for the set.
       */
      virtual void orbitalClicked(int orbengIndex);

      /** Needs orbital energies; without them there is no diagram. */
      virtual bool isRelevant(IPropCalculation *propCalc) const;

      virtual bool prefersFloating() const;
      virtual wxSize preferredFloatingSize() const;

      virtual void refresh();

   protected:

      /**
       * Which side of the diagram each set of equivalent atoms is on.
       *
       * A FRAGMENT HAS TO BE A UNION OF ORBITS: the group must map it
       * onto itself or it has no symmetry orbitals at all.  So the
       * choice is offered as a grouping of the orbits rather than a
       * selection of atoms, and an impossible fragmentation cannot be
       * expressed.
       *
       * Empty means decide automatically, which handles a central atom
       * with equivalent neighbours, and a diatomic, and refuses
       * everything else.
       */
      vector<int> p_sideOfOrbit;

      /** Which construction the last diagram was built by. */
      MoFragments::Fragmentation p_fragmentation;

      /** The constructions this molecule offers, in menu order. */
      vector<MoFragments::Fragmentation> p_offered;

      void buildFragmentChooser(wxSizer *sizer);
      void fillFragmentChooser(const vector< vector<int> >& orbits,
                               const vector<string>& elements);
      void onFragmentChanged(wxCommandEvent& event);

      /** Show or hide each level's energy. */
      void onShowEnergies(wxCommandEvent& event);

      virtual void initialize();

   private:

      void build();

      MoDiagramCanvas *p_canvas;
      wxCheckListBox  *p_fragments;
      wxStaticText    *p_fragmentHint;
      wxCheckBox      *p_autoFragments;
      wxChoice        *p_construction;
      wxCheckBox      *p_piOnly;
      wxCheckBox      *p_showEnergies;
};

#endif

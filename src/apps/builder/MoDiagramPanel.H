#ifndef MODIAGRAMPANEL__H
#define MODIAGRAMPANEL__H

#include <string>
#include <vector>

#include "VizPropertyPanel.H"
#include "tdat/MoDiagram.H"

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
class MoDiagramPanel : public VizPropertyPanel
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

      /** Needs orbital energies; without them there is no diagram. */
      virtual bool isRelevant(IPropCalculation *propCalc) const;

      virtual bool prefersFloating() const;
      virtual wxSize preferredFloatingSize() const;

      virtual void refresh();

   protected:

      virtual void initialize();

   private:

      void build();

      MoDiagramCanvas *p_canvas;
};

#endif

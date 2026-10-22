#ifndef REACTIONPANEL__H
#define REACTIONPANEL__H

#include <string>
#include "PropertyPanel.H"
#include "ReactionEnergies.H"

class wxChoice;
class wxGrid;
class wxStaticText;
class XYPlotCanvas;

/**
 * Reaction Energies (#419): for a transition-state job (QST, NEB-TS) the
 * barrier and reaction energy from its own final energy and those of the
 * reactant and product calculations it refers to.  A table, a three-level
 * diagram and Export .csv; warnings name what differs between the three.
 */
class ReactionPanel : public PropertyPanel
{
   DECLARE_DYNAMIC_CLASS(ReactionPanel)

   public:
      ReactionPanel();
      ReactionPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="ReactionPanel");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="ReactionPanel");
      virtual ~ReactionPanel();

      /** Only for a QST or NEB-TS job. */
      virtual bool isRelevant(IPropCalculation *propCalc) const;
      virtual int minimumHeight() const { return 380; }
      virtual int openingHeight() const { return 460; }

      const RxnResult& result() const { return p_result; }

      /** The table as CSV; false if the file cannot be written. */
      bool exportCsv(const wxString& path, bool kcal);

   protected:
      void fill();
      void OnChange(wxCommandEvent& event);
      void OnExport(wxCommandEvent& event);

   private:
      RxnResult p_result;
      wxChoice *p_unit;
      wxChoice *p_quantity;
      wxStaticText *p_text;
      XYPlotCanvas *p_plot;
      wxGrid *p_grid;
};

#endif

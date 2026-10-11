#ifndef QUADPOLEPANEL__H
#define QUADPOLEPANEL__H


#include "MomentsPanel.H"

class ewxStaticText;



/**
 * Specialization of Moments panel that:
 *   a) uses a different vector command
 *   b) may render the table differently
 */
class QuadPolePanel : public MomentsPanel 
{
   DECLARE_DYNAMIC_CLASS(QuadPolePanel)

   public:
      QuadPolePanel();
      QuadPolePanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="QuadPolePanel");
      bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="QuadPolePanel");
      virtual ~QuadPolePanel();

      virtual void receiveFocus();
      virtual void loseFocus();
      virtual bool isRelevant(IPropCalculation *propCalc) const;

   protected:
      virtual void fillTable();
      virtual void showVectors(bool guessScale);
      // Only the polarizability is drawn (as an ellipsoid); the other
      // tensors show their table.
      virtual bool drawsOverlay() const { return isPolarizability(); }
      virtual bool isPolarizability() const;

      bool p_useOriginalGeomHack;

      // The semi-axes in Angstrom at the current scale.
      ewxStaticText *p_axesText;
};

#endif // QUADPOLEPANEL__H


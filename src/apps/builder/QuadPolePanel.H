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
      // The polarizability (an ellipsoid) and the quadrupole to hexadecapole
      // (angular surfaces) are drawn; the other tensors show their table.
      virtual bool drawsOverlay() const
         { return isPolarizability() || isMultipole(); }
      virtual bool isPolarizability() const;
      virtual bool isMultipole() const;
      void showMultipole(bool guessScale);

      // For the multipoles: the code's name (it fixes the units) and the
      // origin choice ("charge" or "mass") of the surface.
      string p_code;
      string p_origin;

      bool p_useOriginalGeomHack;

      // The semi-axes in Angstrom at the current scale.
      ewxStaticText *p_axesText;
};

#endif // QUADPOLEPANEL__H


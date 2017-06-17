#ifndef QUADPOLEPANEL__H
#define QUADPOLEPANEL__H


#include "MomentsPanel.H"



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

      bool p_useOriginalGeomHack;
};

#endif // QUADPOLEPANEL__H


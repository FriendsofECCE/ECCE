#ifndef TABLEATOMCOORDS__H
#define TABLEATOMCOORDS__H


#include "MomentsPanel.H"




/**
 */
class TableAtomCoords : public MomentsPanel
{
   DECLARE_DYNAMIC_CLASS(TableAtomCoords)

   public:

      TableAtomCoords();
      TableAtomCoords(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="TableAtomCoords");
      virtual ~TableAtomCoords();

   protected:
      virtual void fillTable();
      virtual void showVectors(bool guessScale);
      virtual string getConfigKey();


};

#endif // TABLEATOMCOORDS__H


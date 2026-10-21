#ifndef TABLEPANEL__H
#define TABLEPANEL__H

#include "TableGUI.H"


class wxGrid;


/**
 * A panel for viewing normal mode animations and vectors.
 */
class TablePanel : public TableGUI
{
   DECLARE_DYNAMIC_CLASS(TablePanel)
   DECLARE_EVENT_TABLE()

   public:
      //  Geometry trace and the other tables: rows to read, not a sliver.
      virtual int openingHeight() const { return 380; }
      TablePanel();
      TablePanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="TablePropertyPanel");
      virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="TablePropertyPanel");
      virtual ~TablePanel();

   protected:
      void initialize();
      void fillTable();

   private:
      wxGrid *p_grid;  
};

#endif // TABLEPANEL_H


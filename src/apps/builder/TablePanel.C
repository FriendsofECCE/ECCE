#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

#include <wx/link.h>
#include <wx/sizer.h>

#include "util/InternalException.H"
#include "util/PreferenceLabels.H"

#include "tdat/PropTable.H"
#include "tdat/TProperty.H"

#include "dsm/IPropCalculation.H"

#include "TablePanel.H"



// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE( TablePanel, TableGUI )

END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(TablePanel, TableGUI)


TablePanel::TablePanel()
  : TableGUI(),
    p_grid(NULL)
{
}


TablePanel::TablePanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : TableGUI(),
    p_grid(NULL)
{
   Create(calculation, parent, id, pos, size, style, name);
}


TablePanel::~TablePanel()
{
}


bool TablePanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
   if (!TableGUI::Create(calculation, parent, id, pos, size, style )) {
      wxFAIL_MSG( wxT("TablePanel creation failed") );
      return false;
   }

   // Customizations of DB code
   p_grid = (wxGrid*)FindWindow(ID_GRID_TABLE);
   p_grid->SetRowLabelSize(0);

   initialize();

   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);

   return true;
}


void TablePanel::initialize()
{
   fillTable();
}


/**
 * Fill the table and initialize the UI.
 */
void TablePanel::fillTable()
{
   IPropCalculation *expt = getCalculation();

   // Clear table
   p_grid->ClearGrid();
   p_grid->DeleteCols(0,p_grid->GetNumberCols());
   p_grid->DeleteRows(0,p_grid->GetNumberRows());

   set<string> props = getPropertyNames();
   if (props.size() > 0) {
     set<string>::iterator it = props.begin();
     TProperty *prop = (TProperty*) expt->getProperty(*it);
     PropTable *tab = dynamic_cast<PropTable*>(prop);
     if (tab != (PropTable*)0) {
       p_grid->InsertCols(0, tab->columns());
       p_grid->InsertRows(0, tab->rows());

       int cols = tab->columns();
       int rows = tab->rows();
       vector<string>* colLabs = tab->colLabels();
       int ic, ir;
       for (ic=0; ic<cols; ic++) {
         p_grid->SetColLabelValue(ic, (*colLabs)[ic].c_str());
       }

       vector<string>* rowLabs = tab->rowLabels();
       for (ir=0; ir<rows; ir++) {
         p_grid->SetRowLabelValue(ir, (*rowLabs)[ir].c_str());
       }

       wxString strval;
       for (ic=0; ic<cols; ic++) {
         for (ir=0; ir<rows; ir++) {
           strval = wxString::Format (_T(PrefLabels::DOUBLEFORMAT), tab->value(ir,ic));
           p_grid->SetCellValue(ir, ic, strval);
         }
       }

       p_grid->AutoSize();
     } else {
        INTERNALEXCEPTION(tab,"Incorrect cast to a table");
     }
   }
}





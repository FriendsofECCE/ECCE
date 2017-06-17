#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

#include "wx/sizer.h"
#include "wx/grid.h" 

#include "util/EventDispatcher.H"
#include "util/PreferenceLabels.H"

#include "tdat/TProperty.H"
#include "tdat/PropVector.H"

#include "dsm/IPropCalculation.H"
#include "dsm/ICalculation.H"

#include "wxgui/ewxConfig.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/WindowEvent.H"
#include "wxgui/ewxGrid.H"

#include "DefaultVecPanel.H"


// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE( DefaultVecPanel, PropertyPanel )

END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(DefaultVecPanel, PropertyPanel)


DefaultVecPanel::DefaultVecPanel()
  : PropertyPanel(),
    p_grid(NULL)
{
}


DefaultVecPanel::DefaultVecPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : PropertyPanel(),
    p_grid(NULL)
{
   Create(calculation, parent, id, pos, size, style, name);
}


DefaultVecPanel::~DefaultVecPanel()
{
}



bool DefaultVecPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
   if (!PropertyPanel::Create(calculation, parent, id, pos, size, style )) {
      wxFAIL_MSG( wxT("PropertyPanel creation failed") );
      return false;
   }

   wxPanel *panel = this;

   this->SetSizer(new wxBoxSizer(wxVERTICAL));

   wxBoxSizer* tableSizer = new wxBoxSizer(wxHORIZONTAL);
   panel->GetSizer()->Add(tableSizer, 1, wxGROW|wxALL, 0);
   p_grid = new ewxGrid( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
   p_grid->EnableEditing(false);
   p_grid->SetDefaultColSize(50);
   p_grid->SetDefaultRowSize(25);
   p_grid->SetColLabelSize(40);
   p_grid->SetRowLabelSize(50);
   //p_grid->SetRowLabelSize(0);
   p_grid->SetColLabelSize(0);
   p_grid->SetMinSize(wxSize(200,175));
   p_grid->CreateGrid(5, 5, wxGrid::wxGridSelectRows);
   tableSizer->Add(p_grid, 1, wxGROW|wxALL, 3);



   fillTable();

   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);


   return true;
}


/**
 * Fill the table and initialize the UI.
 */
void DefaultVecPanel::fillTable()
{

   IPropCalculation *expt = getCalculation();

   // Clear table
   p_grid->ClearGrid();
   p_grid->DeleteCols(0,p_grid->GetNumberCols());
   p_grid->DeleteRows(0,p_grid->GetNumberRows());

  set<string> keys = getPropertyNames();
  if (keys.size() > 0) {
     // Should be just one property so that is all we will process
     string key = *keys.begin();
     TProperty *prop = (TProperty*) expt->getProperty(key);
     PropVector *vec = dynamic_cast<PropVector*>(prop);
     if (vec) {
        const vector<string> *rowLabels = vec->rowLabels();


        int nRows = vec->rows();
        // This class displays tables that are stored as vectors 
        // nRows is the number of elements in the vectors.  The actual
        // number of columns is not in the data table so this class assumes
        // (and is designed to display) 3 columns of data
        // compute the nxm dimensions
        p_grid->InsertRows(0,nRows);
        p_grid->InsertCols(0,1);
        wxString cellv;
        for (int idx=0; idx<nRows; idx++) {
           if (rowLabels) {
              cellv = (*rowLabels)[idx].c_str();
           } else {
              // No labels
              cellv = "";
           }
           p_grid->SetRowLabelValue(idx, cellv);
           cellv = wxString::Format (_T(PrefLabels::DOUBLEFORMAT), vec->value(idx));
           p_grid->SetCellValue(idx, 0, cellv);
        }
     }

  }

   p_grid->AutoSize();
}

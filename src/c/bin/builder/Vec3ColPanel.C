#include <iostream>
  using std::cout;
  using std::endl;
#include <limits>

#include <wx/link.h>
#include <wx/sizer.h>

#include "util/PreferenceLabels.H"

#include "tdat/PropVector.H"
#include "tdat/TProperty.H"

#include "dsm/IPropCalculation.H"

#include "wxgui/ewxGrid.H"

#include "Vec3ColPanel.H"


//static const char *INIFILE = "wxbuilder.ini";


// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE( Vec3ColPanel, PropertyPanel )

END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(Vec3ColPanel, PropertyPanel)


Vec3ColPanel::Vec3ColPanel()
  : PropertyPanel(),
    p_grid(NULL)
{
}


Vec3ColPanel::Vec3ColPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : PropertyPanel(),
    p_grid(NULL)
{
   Create(calculation, parent, id, pos, size, style, name);
}


Vec3ColPanel::~Vec3ColPanel()
{
}



bool Vec3ColPanel::Create(IPropCalculation *calculation,
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
   p_grid = new ewxGrid( panel, wxID_ANY, wxDefaultPosition, wxSize(200, 225), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
   p_grid->SetDefaultColSize(50);
   p_grid->SetDefaultRowSize(25);
   p_grid->SetColLabelSize(40);
   p_grid->SetRowLabelSize(50);
   p_grid->SetRowLabelSize(0);
   p_grid->SetColLabelSize(0);
   p_grid->CreateGrid(5, 5, wxGrid::wxGridSelectRows);
   tableSizer->Add(p_grid, 1, wxGROW|wxALL, 3);



   fillTable();

   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);


   return true;
}


/**
 * Generate key fro preference files in case we need to
 * store any preferences later.
 */
string Vec3ColPanel::getConfigKey()
{
  string ret;
  set<string> keys = getPropertyNames();
  if (keys.size() > 0) {
     ret = "Vec3ColPanel/" + *keys.begin();
  }
  return ret;
}




/**
 * Fill the table and initialize the UI.
 */
void Vec3ColPanel::fillTable()
{

   IPropCalculation *expt = getCalculation();

   // Clear table
   p_grid->ClearGrid();
   p_grid->DeleteCols(0,p_grid->GetNumberCols());
   p_grid->DeleteRows(0,p_grid->GetNumberRows());

  set<string> keys = getPropertyNames();
  if (keys.size() > 0) {
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
        p_grid->InsertRows(0,static_cast<int>(ceil(nRows/3.0)));
        p_grid->InsertCols(0,3);
        wxString cellv;
        for (int idx=0; idx<nRows; idx++) {
           if (rowLabels) {
              cellv = wxString::Format (wxString("%s=")+_T(PrefLabels::DOUBLEFORMAT), (*rowLabels)[idx].c_str(),vec->value(idx));
           } else {
              // No labels
              cellv = wxString::Format (_T(PrefLabels::DOUBLEFORMAT), vec->value(idx));
           }
           cellv.Replace(" ","",true);
           p_grid->SetCellValue((idx/3),idx%3,cellv);
        }
     }

  }

   p_grid->AutoSize();
}

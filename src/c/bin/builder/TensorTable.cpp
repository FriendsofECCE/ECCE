#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <limits>

#include <wx/link.h>
#include <wx/log.h>
#include <wx/sizer.h>
#include <wx/tglbtn.h> // EVT_TOGGLEBUTTON
#include <wx/grid.h>
#include <wx/slider.h>

#include "util/EventDispatcher.H"
#include "util/PreferenceLabels.H"

#include "tdat/PropTable.H"
#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"
#include "tdat/SingleGrid.H"
#include "tdat/TProperty.H"

#include "dsm/IPropCalculation.H"
#include "dsm/ICalculation.H"
#include "dsm/PropFactory.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxColor.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/WindowEvent.H"
#include "wxgui/ewxGrid.H"

#include "inv/ChemKit/ChemColor.H"

#include "TensorTable.H"


const wxWindowID TensorTable::ID_DIALOG_TENSOR = wxNewId();
const wxWindowID TensorTable::ID_TENSOR_TABLE = wxNewId();

// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE( TensorTable, ewxDialog )

    //EVT_GRID_SELECT_CELL(MoCoeffs::OnMoSelection)

END_EVENT_TABLE()


IMPLEMENT_DYNAMIC_CLASS(TensorTable, ewxDialog)


TensorTable::TensorTable()
  : ewxDialog()
{
}


TensorTable::TensorTable(wxWindow *parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
  : ewxDialog(parent, id, title, pos, size, style, name)
{

   Create();
}

TensorTable::~TensorTable()
{
}



bool TensorTable::Create()
{
   //
   // Create simple gui with 3x3 data
   //

   ////@begin MoGUI content construction
   TensorTable* itemDialog1 = this;

   wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
   itemDialog1->SetSizer(itemBoxSizer2);

   // Make a sizer with a couple of labels
   wxBoxSizer* labelSizer = new wxBoxSizer(wxHORIZONTAL);
   itemBoxSizer2->Add(labelSizer, 0, wxGROW|wxALL, 0);
   p_label = new ewxStaticText(this, wxID_ANY, ""); // Atom Label
   labelSizer->Add(p_label,0,wxGROW|wxALL,5);
   p_units = new ewxStaticText(this, wxID_ANY, ""); // units
   labelSizer->Add(p_units,0,wxGROW|wxALL,5);



   // The grid itself
   wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
   itemBoxSizer2->Add(itemBoxSizer3, 1, wxGROW|wxALL, 0);

   p_grid = new ewxGrid( itemDialog1, ID_TENSOR_TABLE, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );


   p_grid->CreateGrid(3,3,wxGrid::wxGridSelectRows);
   itemBoxSizer3->Add(p_grid, 1, wxGROW|wxALL, 0);


   // The dialog buttons 
   wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
   itemBoxSizer2->Add(itemBoxSizer5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

   wxStdDialogButtonSizer* itemStdDialogButtonSizer6 = new wxStdDialogButtonSizer;

   itemBoxSizer5->Add(itemStdDialogButtonSizer6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
   wxButton* itemButton7 = new wxButton( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
   itemStdDialogButtonSizer6->AddButton(itemButton7);


   itemStdDialogButtonSizer6->Realize();

   initialize();

   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);



   return true;
}

void TensorTable::initialize()
{
   p_grid->DeleteRows(0,p_grid->GetNumberRows());
   p_grid->DeleteCols(0,p_grid->GetNumberCols());

   wxListItem itemCol;
   //itemCol.SetAlign(wxLIST_FORMAT_CENTRE);

   itemCol.SetText(_T("#"));

   p_grid->InsertCols(0,1);
   p_grid->SetColLabelValue(0,"X");

   p_grid->InsertCols(1,1);
   p_grid->SetColLabelValue(1,"Y");

   p_grid->InsertCols(2,1);
   p_grid->SetColLabelValue(2,"Z");

   p_grid->InsertRows(0,1);
   p_grid->SetRowLabelValue(0,"X");
   p_grid->InsertRows(1,1);
   p_grid->SetRowLabelValue(1,"Y");
   p_grid->InsertRows(2,1);
   p_grid->SetRowLabelValue(2,"Z");
}




/**
 * Fill the table and initialize the UI.
 * This code taken (with much liberty) from tensor_cdlg
 */
void TensorTable::fillTable(PropVector *vec, int index)
{
   SetTitle( wxString::Format (_T("ECCE Tensor Values: %d"), index+1));


   if (vec) {

      // TODO provide unit information - where to put it?
      //const Property_Ref *propRef = PropFactory::getPropRef(vec->type());
      string unit = vec->units();
      p_units->SetLabel(unit.c_str());

      // Get the row labels
      const vector<string> *rowLabels = vec->rowLabels();

      char buf[32];
      static const char* defaultOrder[] = {"xx", "yx", "zx", "xy", "yy",
         "zy", "xz", "yz", "zz"};
      string atomLabel;
      string curAtom;
      int coordNum;
      int atomNum;
      char lcoord;
      char rcoord;
      int row;
      int col;
      int startIdx = index * 9; // 1st 9 numbers for atom 1, 2nd 9 for atom 2...



      p_grid->BeginBatch();
      for (int i=startIdx; i<startIdx+9; i++) {
         if (rowLabels) {
            atomLabel = (*rowLabels)[i].substr(0, (*rowLabels)[i].length()-3);
            lcoord = tolower((*rowLabels)[i][(*rowLabels)[i].length()-2]);
            rcoord = tolower((*rowLabels)[i][(*rowLabels)[i].length()-1]);
         } else {
            cerr <<  "No labels exist for shielding tensors!" << endl;;
            atomNum = (int)(i/9);
            coordNum = i%9;
            sprintf(buf, "%d", atomNum);
            atomLabel = buf;
            lcoord = defaultOrder[coordNum][0];
            rcoord = defaultOrder[coordNum][1];
         }

         switch (lcoord) {
            case 'x' : row = 0; break;
            case 'y' : row = 1; break;
            case 'z' : row = 2; break;
            default  : row = 0; 
         }

         switch (rcoord) {
            case 'x' : col = 0; break;
            case 'y' : col = 1; break;
            case 'z' : col = 2; break;
            default  : col = 0; 
         }
         if (i == startIdx) p_label->SetLabel(atomLabel.c_str());
         p_grid->SetCellValue(row,col,
               wxString::Format (_T(PrefLabels::DOUBLEFORMAT), vec->value(i)));

      }


      p_grid->EndBatch();
   }


   p_grid->AutoSize();
   GetSizer()->Fit(this);
}

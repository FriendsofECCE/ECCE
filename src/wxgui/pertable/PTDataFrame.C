/**
 * @file
 *
 * Periodic table data dialog class.
 *
 *
 */

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "PTDataFrame.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <wx/colordlg.h>

#include <vector>
  using std::vector;

#include "tdat/TChartOfNuclides.H"
#include "tdat/TPerTab.H"
#include "tdat/TRefIsotope.H"

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/SFile.H"

#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceTool.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxColor.H"
#include "wxgui/ewxColorDialog.H"
#include "wxgui/ewxGrid.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/PTDataFrame.H"


IMPLEMENT_DYNAMIC_CLASS( PTDataFrame, PTDataFrameGUI )

PTDataFrame::PTDataFrame() {}

PTDataFrame::~PTDataFrame()
{
  delete p_tChartNuclides;
}

PTDataFrame::PTDataFrame(int atomicNum, PerTabPanel * parent, wxWindowID id,
			 const wxString& caption, const wxPoint& pos,
			 const wxSize& size, long style):
  PTDataFrameGUI((wxWindow *) parent, id, caption, pos, size, style)
{
  // Set the name of the frame, necessary for the help handler to find right url
  SetName("PerTabData");
  // Get Registry
  ResourceDescriptor rs = ResourceDescriptor::getResourceDescriptor();


  // Set desktop icon
  SetIcon(wxIcon(ewxBitmap::pixmapFile(rs.getTool(PERTAB)->getIcon()),
                                       wxBITMAP_TYPE_XPM));

  p_atomicSymbolLabel->SetFont(ewxStyledWindow::getBigAtomicSymbolFont());
  p_electronConfigLabel->SetFont(ewxStyledWindow::getSmallLabelFont());
  p_refLabel->SetFont(ewxStyledWindow::getSmallLabelFont());
  p_ref2Label->SetFont(ewxStyledWindow::getSmallLabelFont());

  p_atomicNum = atomicNum;
  p_parent = parent;
  p_tChartNuclides = new TChartOfNuclides();
  //	Start with size 0x3
  p_isotopeGrid = new ewxGrid( p_dataPanel, ID_PTDATA_ISOTOPEGRID, wxDefaultPosition, wxDefaultSize, wxNO_BORDER );
  p_isotopeGrid->CreateGrid(0, 3, wxGrid::wxGridSelectCells);

  //	Generate the comlumn label row
  p_isotopeGrid->SetLabelValue( wxHORIZONTAL, "Mass Number", 0);
  p_isotopeGrid->SetLabelValue( wxHORIZONTAL, "Abundance", 1);
  p_isotopeGrid->SetLabelValue( wxHORIZONTAL, "Atomic Mass", 2);

  //	Remove the Row label
  p_isotopeGrid->SetRowLabelSize(1);
  p_isotopeGrid->AutoSize();

  //	Set Readonly
  p_isotopeGrid->SetReadOnly(true);
  
  p_isotopeSizer->Add(p_isotopeGrid, 1, wxGROW|wxALIGN_CENTER|wxALL, 10);

  //	Set Readonly
  p_ionPotentGrid->SetReadOnly(true);

  FILE *fp;
  string ionFile = Ecce::ecceDataPrefPath();
  ionFile += "/IonizationPotent.txt";
  SFile file(ionFile);
  if (!(fp = fopen(file.path(true).c_str(),"r"))) {
    string msg = file.path();
    msg += " not found!";
    EE_RT_ASSERT(false, EE_WARNING, msg);
  }
  else {
    char strBuffer[20];
    int strPtr = 0;
    
    int row = 0;
    int column = 0;
    while(!feof(fp)) {
      strBuffer[strPtr] = fgetc(fp);
      switch (strBuffer[strPtr]) {
      case ',':
	strBuffer[strPtr] = 0;
	strPtr = 0;
	if (row || column) {
	  if (row == 0) {
	    p_ionPotentGrid->SetLabelValue(wxHORIZONTAL, strBuffer, column-1);
	  }
	  else if (column == 0) {
	    p_ionPotentGrid->SetLabelValue(wxVERTICAL, strBuffer, row-1);
	  }
	  else {
	    p_ionPotentGrid->SetCellValue(row-1, column-1, strBuffer);
	  }
	}
	column ++;
	break;
      case '\n':
	strBuffer[strPtr] = 0;
	strPtr = 0;
	if (row || column) {
	  if (row == 0) {
	    p_ionPotentGrid->SetLabelValue(wxHORIZONTAL, strBuffer, column-1);
	  }
	  else if (column == 0) {
	    p_ionPotentGrid->SetLabelValue(wxVERTICAL, strBuffer, row-1);
	  }
	  else {
	    p_ionPotentGrid->SetCellValue(row-1, column-1, strBuffer);
	  }
	}
	column = 0;
	row ++;
	break;
      default:
	strPtr ++;
	break;
      }
    }
  }

  p_ionPotentGrid->AutoSize();

  p_customColorPanel->SetEventHandler(this);

  p_covRadInput->SetValidator(ewxNumericValidator(0.0, 100.0, 1.0));
  p_vdwRadInput->SetValidator(ewxNumericValidator(0.0, 100.0, 1.0));
  p_bftInput->SetValidator(ewxNumericValidator(0.0, 100.0, 1.0));

  setContent(atomicNum);
}


void PTDataFrame::setContent(int atomicNum)
{
  p_atomicNum = atomicNum;
  
  SetTitle("ECCE Periodic Table: Atomic Data of " +
           p_parent->getTPerTab()->name(atomicNum));

  char strBuffer[10];

  // Refresh Reference Data Panel
  //	Refresh Symbol Section on the left
  sprintf(strBuffer, "%3d", atomicNum);
  p_atomicNumLabel->SetLabel(strBuffer);

  sprintf(strBuffer, "%10.6f", p_parent->getTPerTab()->atomicWeight(atomicNum));
  p_atomicWeightLabel->SetLabel(strBuffer);

  p_atomicSymbolLabel->SetLabel(p_parent->getTPerTab()->atomicSymbol(atomicNum));
  
  p_electronConfigLabel->SetLabel(p_parent->getTPerTab()->electronConfig(atomicNum));

  p_elementNameLabel->SetLabel(p_parent->getTPerTab()->name(atomicNum));

  p_electronAffinityLabel->SetLabel("Electron Affinities: " +
                                    p_parent->getTPerTab()->electronAffinity(atomicNum) + " eV");


  p_isoTableLabel->SetLabel("Significant Isotopes of " + p_parent->getTPerTab()->name(atomicNum));
  //	Refresh Table Section on the right
  //		First delete current data table
  p_isotopeGrid->DeleteRows(0, p_isotopeGrid->GetNumberRows());
  
  //		Then create new data entries
  vector<TRefIsotope *> * iso = p_tChartNuclides->isotopeList(atomicNum);
  p_isotopeGrid->AppendRows(iso->size());

  TRefIsotope *current;
  char str[32];
  for (size_t idx = 0; idx<iso->size(); idx++) {
    
    current = (*iso)[idx];

    sprintf(str,"%d",current->atomicMassNumber());
    p_isotopeGrid->SetCellValue(idx, 0, str);

    sprintf(str,"%f",current->abundance()*100.0);
    p_isotopeGrid->SetCellValue(idx, 1, str);
    
    sprintf(str,"%f",current->isotopeWeight());
    p_isotopeGrid->SetCellValue(idx, 2, str);
  }

  p_isotopeGrid->SetGridCursor(100, 100);
  p_isotopeGrid->ClearSelection();

  p_dataPanel->Layout();


  p_ionPotentGrid->MakeCellVisible(atomicNum-1, 0);
  p_ionPotentGrid->SelectRow(atomicNum-1);


  p_customColorPanel->SetBackgroundColour(p_parent->getUserElementColor(atomicNum));
  p_customColorPanel->Refresh();

  p_defaultColorPanel->SetBackgroundColour(p_parent->getDefaultElementColor(atomicNum));
  p_defaultColorPanel->Refresh();


  sprintf(str, "%6.4f", p_parent->getUserCovRad(atomicNum));
  p_covRadInput->SetValue(str);

  sprintf(str, "%6.4f", p_parent->getUserVdwRad(atomicNum));
  p_vdwRadInput->SetValue(str);

  sprintf(str, "%6.4f", p_parent->getUserBft(atomicNum));
  p_bftInput->SetValue(str);

  sprintf(str, "(Default=%6.4f)", p_parent->getDefaultCovRad(atomicNum));
  p_defCovRadLabel->SetLabel(str);

  sprintf(str, "(Default=%6.4f)", p_parent->getDefaultVdwRad(atomicNum));
  p_defVdwRadLabel->SetLabel(str);

  sprintf(str, "(Default=%6.4f)", p_parent->getDefaultBft(atomicNum));
  p_defBftLabel->SetLabel(str);

  p_resetColorButton->Enable(p_defaultColorPanel->GetBackgroundColour() !=
			     p_customColorPanel->GetBackgroundColour());
  
  p_resetRadiiButton->Enable(!equal(p_parent->getUserCovRad(p_atomicNum), p_parent->getDefaultCovRad(p_atomicNum)) ||
			     !equal(p_parent->getUserVdwRad(p_atomicNum), p_parent->getDefaultVdwRad(p_atomicNum)) ||
			     !equal(p_parent->getUserBft(p_atomicNum), p_parent->getDefaultBft(p_atomicNum)));
  
  p_resetAllColorButton->Enable(p_parent->colorChanged());
  p_resetAllRadiiButton->Enable(p_parent->radiiChanged());

}


void PTDataFrame::OnCloseWindow( wxCloseEvent& event )
{
  Show(false);
}


void PTDataFrame::OnCloseClicked( wxCommandEvent& event )
{
  Show(false);
}


void PTDataFrame::OnEditColorClicked( wxCommandEvent& event )
{
  chooseColor();
}


void PTDataFrame::chooseColor()
{
  ewxColorDialog * colorDialog = new ewxColorDialog( p_customColorPanel->GetBackgroundColour(), this);
  if (colorDialog->ShowModal() == wxID_OK) {
    setElementColor((ewxColor) (colorDialog->GetColor()));
  }
  //  colorDialog->Destroy();
}


void PTDataFrame::OnResetColorClicked( wxCommandEvent& event )
{
  setElementColor( p_defaultColorPanel->GetBackgroundColour() );
}


void PTDataFrame::OnResetAllColorClicked( wxCommandEvent& event )
{
  ewxMessageDialog dialog( this, "Do you really want to reset\nall elements' colors?",
			  "Reset all colors",
			  wxNO_DEFAULT|wxOK|wxCANCEL|wxICON_QUESTION);
  switch ( dialog.ShowModal() ) {
  case wxID_OK:
    p_parent->resetAllElementColor();
    setContent(p_atomicNum);
    break;
  case wxID_CANCEL:
    break;
  }
}


void PTDataFrame::OnResetRadiiClicked( wxCommandEvent& event )
{
  p_parent->resetElementRadii(p_atomicNum);
  setContent(p_atomicNum);
}


void PTDataFrame::OnResetAllRadiiClicked( wxCommandEvent& event )
{
  ewxMessageDialog dialog( this, "Do you really want to reset\nall elements' radii?",
			  "Reset all radii",
			  wxNO_DEFAULT|wxOK|wxCANCEL|wxICON_QUESTION);
  switch ( dialog.ShowModal() ) {
  case wxID_OK:
    p_parent->resetAllElementRadii();
    setContent(p_atomicNum);
    break;
  case wxID_CANCEL:
    break;
  }
}


void PTDataFrame::OnRadiiUpdated( wxCommandEvent& event )
{
  char *tail = (char*)0;
  double covRadInput = strtod( p_covRadInput->GetValue().c_str(), &tail);
  tail = (char*)0;
  double vdwRadInput = strtod( p_vdwRadInput->GetValue().c_str(), &tail);
  tail = (char*)0;
  double bftInput = strtod( p_bftInput->GetValue().c_str(), &tail);

  p_parent->setElementRadii(p_atomicNum, covRadInput, vdwRadInput, bftInput);
  setContent(p_atomicNum);
}


bool PTDataFrame::equal( double a, double b)
{
  return (a - b) < 1e-6 && (b - a) < 1e-6;
}


void PTDataFrame::setElementColor(ewxColor color)
{
  p_customColorPanel->SetBackgroundColour(color);
  p_customColorPanel->Refresh();
  p_parent->setElementColor(p_atomicNum, color);
  p_resetColorButton->Enable(p_defaultColorPanel->GetBackgroundColour() !=
			     p_customColorPanel->GetBackgroundColour());
  p_resetAllColorButton->Enable(p_parent->colorChanged());
}

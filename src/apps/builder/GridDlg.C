#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
#include <limits>

#include "wx/link.h"
#include "wx/log.h"
#include "wx/sizer.h"
#include "wx/tglbtn.h" // EVT_TOGGLEBUTTON

#include "dsm/PropFactory.H"
#include "dsm/ICalculation.H"

#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxNumericValidator.H"

#include "GridDlg.H"


IMPLEMENT_DYNAMIC_CLASS(GridDlg, GridGUI)


GridDlg::GridDlg()
  : GridGUI()
{
   Init();
}


GridDlg::GridDlg( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
  : GridGUI(parent, id, caption, pos, size, style)
{
   Init();
   enableUniformGrid(true);
}

GridDlg::~GridDlg()
{
}

void GridDlg::Init()
{
   ewxTextCtrl *txt;

   ewxNumericValidator validator(5,100,30);

   // Resolution Ranges
   validator.setHardRange("[5,)");
   validator.setValue(30);
   

   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_XRES);
   txt->SetValidator(validator);
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_YRES);
   txt->SetValidator(validator);
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_ZRES);
   txt->SetValidator(validator);
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_UNIFORM);
   txt->SetValidator(validator);

   // bounding box
   // 2/13/08 soft range code doesn't work - not used anywhere either
   //validator.setSoftMinMax(-100.0,100.0);
   validator.setHardRange("(-10000,100000)");
   validator.setValue(-5.0);

   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRLGRID_FROMX);
   txt->SetValidator(validator);
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_FROMY);
   txt->SetValidator(validator);
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_FROMZ);
   txt->SetValidator(validator);

   validator.setValue(5.0);
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_TOX);
   txt->SetValidator(validator);
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_TOY);
   txt->SetValidator(validator);
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_TOZ);
   txt->SetValidator(validator);
}

void GridDlg::enableUniformGrid(bool enabled)
{
   ewxTextCtrl *txt;
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_XRES);
   txt->Enable(!enabled);
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_YRES);
   txt->Enable(!enabled);
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_ZRES);
   txt->Enable(!enabled);
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_UNIFORM);
   txt->Enable(enabled);
   ewxCheckBox *box = (ewxCheckBox*)FindWindow(ID_CHECKBOX_GRID_UNIFORM);
   box->SetValue(enabled);
}

void GridDlg::OnCheckboxGridUniformClick( wxCommandEvent& event )
{
   ewxCheckBox *box = (ewxCheckBox*)FindWindow(ID_CHECKBOX_GRID_UNIFORM);
   enableUniformGrid(box->IsChecked());
   event.Skip();
}


void GridDlg::OnOkClick( wxCommandEvent& event )
{

   event.Skip();
}

void GridDlg::OnApplyClick( wxCommandEvent& event )
{

   event.Skip();
}


void GridDlg::OnCloseWindow( wxCloseEvent& event )
{
   event.Skip();
}


void GridDlg::OnTextctrlGridUniformEnter( wxCommandEvent& event )
{
   ewxTextCtrl *txt;
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_UNIFORM);
   wxString value = txt->GetValue().c_str();

   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_XRES);
   txt->SetValue(value);
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_YRES);
   txt->SetValue(value);
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_ZRES);
   txt->SetValue(value);

   event.Skip();
}


void GridDlg::setResolution(int x, int y, int z, bool uniform)
{
   ewxTextCtrl *txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_XRES);
   txt->SetValue(wxString::Format (_T("%d"), x));
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_YRES);
   txt->SetValue(wxString::Format (_T("%d"), y));
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_ZRES);
   txt->SetValue(wxString::Format (_T("%d"), z));

   ewxCheckBox *box = (ewxCheckBox*)FindWindow(ID_CHECKBOX_GRID_UNIFORM);
   box->SetValue(uniform);

   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_UNIFORM);
   txt->SetValue(wxString::Format (_T("%d"), x)); // pick any of x,y,z
   TransferDataFromWindow();
}


/**
 * get the resolution of the grid.
 */
void GridDlg::getResolution(int& x, int&y, int& z, bool& uniform) const
{
   ewxTextCtrl *txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_XRES);
   x = atoi(txt->GetValue());
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_YRES);
   y = atoi(txt->GetValue());
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_ZRES);
   z = atoi(txt->GetValue());

   ewxCheckBox *box = (ewxCheckBox*)FindWindow(ID_CHECKBOX_GRID_UNIFORM);
   uniform = box->IsChecked();
}


void GridDlg::setExtent(float fromx, float tox,
      float fromy, float toy,
      float fromz, float toz)
{
   ewxTextCtrl *txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_XRES);

   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRLGRID_FROMX);
   txt->SetValue(wxString::Format (_T("%f"), fromx));
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_FROMY);
   txt->SetValue(wxString::Format (_T("%f"), fromy));
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_FROMZ);
   txt->SetValue(wxString::Format (_T("%f"), fromz));

   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_TOX);
   txt->SetValue(wxString::Format (_T("%f"), tox));
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_TOY);
   txt->SetValue(wxString::Format (_T("%f"), toy));
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_TOZ);
   txt->SetValue(wxString::Format (_T("%f"), toz));
   TransferDataFromWindow();
}


void GridDlg::getExtent(float& fromx, float& tox,
      float& fromy, float& toy,
      float& fromz, float& toz) const
{
   ewxTextCtrl *txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_XRES);
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRLGRID_FROMX);
   fromx = atof(txt->GetValue());
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_FROMY);
   fromy = atof(txt->GetValue());
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_FROMZ);
   fromz = atof(txt->GetValue());

   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_TOX);
   tox = atof(txt->GetValue());
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_TOY);
   toy = atof(txt->GetValue());
   txt = (ewxTextCtrl*)FindWindow(ID_TEXTCTRL_GRID_TOZ);
   toz = atof(txt->GetValue());
}




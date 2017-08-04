/////////////////////////////////////////////////////////////////////////////
// Name:        ewxColorDialog.C
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// Created:     Tue 16 Nov 2004 04:14:28 PM PST
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxColorDialog.H"
#include "wxgui/ewxColor.H"
#include "wxgui/ewxSpinCtrl.H"

// ColorButton Class
BEGIN_EVENT_TABLE( ColorButton, wxPanel )

  EVT_LEFT_DOWN( ColorButton::OnPanelLeftDown )

END_EVENT_TABLE()

ColorButton::ColorButton() {}

ColorButton::~ColorButton() {}

ColorButton::ColorButton( wxColour color, wxWindow* parent,
                          wxWindowID id, const wxPoint& pos,
                          const wxSize& size, long style )
{
  wxPanel::Create( parent, id, pos, size, style );
  SetBackgroundColour(color);
}


void ColorButton::OnPanelLeftDown( wxMouseEvent& event )
{
  ((ewxColorDialog *) GetParent())->SetColor(GetBackgroundColour());
  event.Skip();
}


// ewxColorDialog Class
BEGIN_EVENT_TABLE( ewxColorDialog, wxDialog )

  EVT_COMMAND_SCROLL( ID_SLIDER_RED, ewxColorDialog::OnSliderScroll )
  EVT_COMMAND_SCROLL( ID_SLIDER_GREEN, ewxColorDialog::OnSliderScroll )
  EVT_COMMAND_SCROLL( ID_SLIDER_BLUE, ewxColorDialog::OnSliderScroll )

  EVT_SPINCTRL( ID_SPINCTRL_RED, ewxColorDialog::OnSpinctrlUpdated )
  EVT_SPINCTRL( ID_SPINCTRL_GREEN, ewxColorDialog::OnSpinctrlUpdated )
  EVT_SPINCTRL( ID_SPINCTRL_BLUE, ewxColorDialog::OnSpinctrlUpdated )

END_EVENT_TABLE()

const ewxColor ewxColorDialog::PredefinedColors[] =
{
  ewxColor("#FFFFFF"), ewxColor("#C0C0FF"), ewxColor("#C0E0FF"), ewxColor("#C0FFFF"), ewxColor("#80FF80"), ewxColor("#FFFFC0"), ewxColor("#FFC0C0"), ewxColor("#FFC0FF"),
  ewxColor("#E0E0E0"), ewxColor("#8080FF"), ewxColor("#80C0FF"), ewxColor("#80FFFF"), ewxColor("#COFFCO"), ewxColor("#FFFF80"), ewxColor("#FF8080"), ewxColor("#FF80FF"),
  ewxColor("#C0C0C0"), ewxColor("#0000FF"), ewxColor("#0080FF"), ewxColor("#00FFFF"), ewxColor("#00FF00"), ewxColor("#FFFF00"), ewxColor("#FF0000"), ewxColor("#FF00FF"),
  ewxColor("#808080"), ewxColor("#0000C0"), ewxColor("#0040C0"), ewxColor("#00C0C0"), ewxColor("#00C000"), ewxColor("#C0C000"), ewxColor("#C00000"), ewxColor("#C000C0"),
  ewxColor("#404040"), ewxColor("#000080"), ewxColor("#004080"), ewxColor("#008080"), ewxColor("#008000"), ewxColor("#808000"), ewxColor("#800000"), ewxColor("#800080"),
  ewxColor("#000000"), ewxColor("#000040"), ewxColor("#404080"), ewxColor("#004040"), ewxColor("#004000"), ewxColor("#404000"), ewxColor("#400000"), ewxColor("#400040")
};
  /*
  wxColour(225, 128, 128), wxColour(255, 255, 128), wxColour(128, 255, 128), wxColour(0, 255, 128),
  wxColour(128, 255, 255), wxColour(0, 128, 255), wxColour(255, 128, 192), wxColour(255, 128, 255),
  wxColour(255, 0, 0), wxColour(255, 255, 0), wxColour(128, 255, 0), wxColour(0, 255, 64),
  wxColour(0, 255, 255), wxColour(0, 128, 192), wxColour(128, 128, 192), wxColour(255, 0, 255),
  wxColour(128, 64, 64), wxColour(255, 128, 64), wxColour(0, 255, 255), wxColour(0, 128, 128),
  wxColour(0, 64, 128), wxColour(128, 128, 255), wxColour(128, 0, 64), wxColour(255, 0, 128),
  wxColour(128, 0, 0), wxColour(255, 128, 0), wxColour(0, 128, 0), wxColour(0, 128, 64),
  wxColour(0, 255, 255), wxColour(0, 0, 160), wxColour(128, 0, 128), wxColour(128, 0, 255),
  wxColour(64, 0, 0), wxColour(129, 64, 0), wxColour(0, 64, 0), wxColour(0, 64, 64),
  wxColour(0, 0, 128), wxColour(0, 0, 64), wxColour(64, 0, 64), wxColour(64, 0, 128),
  wxColour(0, 0, 0), wxColour(128, 128, 0), wxColour(128, 128, 64), wxColour(128, 128, 128),
  wxColour(64, 128, 128), wxColour(192, 192, 192), wxColour(64, 0, 64), wxColour(255, 255, 255)
  */


ewxColorDialog::ewxColorDialog( )
{
}


ewxColorDialog::~ewxColorDialog( )
{
}

ewxColorDialog::ewxColorDialog( wxColour color, wxWindow* parent,
				wxWindowID id, const wxString& caption,
				const wxPoint& pos, const wxSize& size,
				long style )
{
  Create(color, parent, id, caption, pos, size, style);
}


bool ewxColorDialog::Create( wxColour color, wxWindow* parent,
			     wxWindowID id, const wxString& caption,
			     const wxPoint& pos, const wxSize& size,
			     long style )
{
  SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
  ewxDialog::Create( parent, id, caption, pos, size, style );

  CreateControls(color);
  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);
  Centre();
  return TRUE;
}


void ewxColorDialog::CreateControls( wxColour color )
{
  ewxColorDialog* item1 = this;

  wxBoxSizer* item2 = new wxBoxSizer(wxVERTICAL);
  item1->SetSizer(item2);
  item1->SetAutoLayout(TRUE);
  wxBoxSizer* item3 = new wxBoxSizer(wxHORIZONTAL);
  item2->Add(item3, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);
  wxBoxSizer* item4 = new wxBoxSizer(wxVERTICAL);
  item3->Add(item4, 0, wxGROW|wxALL, 0);
  wxStaticText* item5 = new wxStaticText( item1, wxID_STATIC, _("Basic Colors:"), wxDefaultPosition, wxDefaultSize, 0 );
  item4->Add(item5, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);
  wxGridSizer* item6 = new wxGridSizer(6, 8, 0, 0);
  p_basicColorGrid = item6;
  item4->Add(item6, 0, wxALIGN_LEFT|wxALL, 5);

  //  wxStaticText* item55 = new wxStaticText( item1, wxID_STATIC, _("Custom colors:"), wxDefaultPosition, wxDefaultSize, 0 );
  //  item4->Add(item55, 0, wxALIGN_LEFT|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);
  //  wxGridSizer* item56 = new wxGridSizer(2, 8, 0, 0);
  //  p_customColorGrid = item56;
  //  item4->Add(item56, 0, wxALIGN_LEFT|wxALL, 5);

  wxBoxSizer* item73 = new wxBoxSizer(wxVERTICAL);
  item3->Add(item73, 0, wxGROW|wxALL, 5);

  wxStaticText* item75 = new wxStaticText( item1, wxID_STATIC, _("Current Color:"), wxDefaultPosition, wxDefaultSize, 0 );
  item73->Add(item75, 0, wxALIGN_LEFT|wxALL, 0);

  p_currentColorPanel = new wxPanel( item1, ID_CURRENTCOLOR, wxDefaultPosition, wxSize(80, 40), wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
  item73->Add(p_currentColorPanel, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  wxFlexGridSizer* item77 = new wxFlexGridSizer(6, 3, 0, 0);
  item73->Add(item77, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  wxStaticText* item78 = new wxStaticText( item1, wxID_STATIC, _("Red:"), wxDefaultPosition, wxDefaultSize, 0 );
  item77->Add(item78, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 1);

  p_sliderRed = new wxSlider( item1, ID_SLIDER_RED, 0, 0, 255, wxDefaultPosition, wxSize(120, -1), wxSL_HORIZONTAL );
  item77->Add(p_sliderRed, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

  p_spinRed = new ewxSpinCtrl( item1, ID_SPINCTRL_RED, _T("0"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 0, 255, 0 );
  item77->Add(p_spinRed, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

  wxStaticText* item81 = new wxStaticText( item1, wxID_STATIC, _("Green:"), wxDefaultPosition, wxDefaultSize, 0 );
  item77->Add(item81, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 1);

  p_sliderGreen = new wxSlider( item1, ID_SLIDER_GREEN, 0, 0, 255, wxDefaultPosition, wxSize(120, -1), wxSL_HORIZONTAL );
  item77->Add(p_sliderGreen, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

  p_spinGreen = new ewxSpinCtrl( item1, ID_SPINCTRL_GREEN, _T("0"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 0, 255, 0 );
  item77->Add(p_spinGreen, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

  wxStaticText* item84 = new wxStaticText( item1, wxID_STATIC, _("Blue:"), wxDefaultPosition, wxDefaultSize, 0 );
  item77->Add(item84, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 1);

  p_sliderBlue = new wxSlider( item1, ID_SLIDER_BLUE, 0, 0, 255, wxDefaultPosition, wxSize(120, -1), wxSL_HORIZONTAL );
  item77->Add(p_sliderBlue, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

  p_spinBlue = new ewxSpinCtrl( item1, ID_SPINCTRL_BLUE, _T("0"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 0, 255, 0 );
  item77->Add(p_spinBlue, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 1);

  wxStaticLine* item87 = new wxStaticLine( item1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );

  item2->Add(item87, 0, wxGROW|wxLEFT|wxRIGHT, 5);
  wxBoxSizer* item88 = new wxBoxSizer(wxHORIZONTAL);
  item2->Add(item88, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

  item88->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL|wxALL, 2);
  ewxButton* item89 = new ewxButton( item1, wxID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
  item88->Add(item89, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);
  item88->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL|wxALL, 2);
  ewxButton* item90 = new ewxButton( item1, wxID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
  item88->Add(item90, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);
  item88->Add(0, 0, 1, wxALIGN_CENTER_VERTICAL|wxALL, 2);

  //  wxButton* item91 = new wxButton( item1, ID_CUSTOMCOLOR, _("Add to Custom Colors"), wxDefaultPosition, wxDefaultSize, 0 );
  //  item88->Add(item91, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

  // Recursively set the ewxStyle
  setStyles(this, true);

  for (int i=0; i<48; i++) {
    ColorButton * a = new ColorButton( PredefinedColors[i], this, 30000+i);
    item6->Add(a, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);
  }

  SetColor(color);
}


void ewxColorDialog::OnSliderScroll( wxScrollEvent& event ) 
{
  switch (event.GetId()) {
  case ID_SLIDER_RED:
    p_red = event.GetPosition();
    break;
  case ID_SLIDER_GREEN:
    p_green = event.GetPosition();
    break;
  case ID_SLIDER_BLUE:
    p_blue = event.GetPosition();
    break;
  }

  UpdateAll();
}


void ewxColorDialog::OnSpinctrlUpdated( wxSpinEvent& event )
{
  switch (event.GetId()) {
  case ID_SPINCTRL_RED:
    p_red = event.GetPosition();
    break;
  case ID_SPINCTRL_GREEN:
    p_green = event.GetPosition();
    break;
  case ID_SPINCTRL_BLUE:
    p_blue = event.GetPosition();
    break;
  }

  UpdateAll();
}


void ewxColorDialog::UpdateAll( )
{
  p_spinRed->SetValue(p_red);
  p_spinGreen->SetValue(p_green);
  p_spinBlue->SetValue(p_blue);
  
  p_sliderRed->SetValue(p_red);
  p_sliderGreen->SetValue(p_green);
  p_sliderBlue->SetValue(p_blue);
  
  p_currentColorPanel->SetBackgroundColour(wxColour(p_red, p_green, p_blue));
  p_currentColorPanel->Refresh();
}


void ewxColorDialog::SetColor(wxColour color)
{
  p_red = color.Red();
  p_green = color.Green();
  p_blue = color.Blue();
  UpdateAll();
}


wxColour ewxColorDialog::GetColor()
{
  return wxColour(p_red, p_green, p_blue);
}

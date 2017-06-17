/////////////////////////////////////////////////////////////////////////////
// Name:        NModesGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "NModesGUI.H"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

////@begin includes
#include "wxgui/ewxGrid.H"
#include "wxgui/ewxBitmapButton.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxRadioBox.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "NModesGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID NModesGUI::ID_BITMAPBUTTON_START = wxNewId();
const wxWindowID NModesGUI::ID_BITMAPBUTTON_STOP = wxNewId();
const wxWindowID NModesGUI::ID_RADIOBOX_NMODE_VIZTYPE = wxNewId();
const wxWindowID NModesGUI::ID_GRID_NMODE = wxNewId();
const wxWindowID NModesGUI::ID_PANEL_NMODE = wxNewId();
const wxWindowID NModesGUI::ID_CHECKBOX_NMODE_VECSIGN = wxNewId();
const wxWindowID NModesGUI::ID_TEXTCTRL_NMODE_DELAY = wxNewId();
const wxWindowID NModesGUI::ID_BUTTON_NMODE_VECCOLOR = wxNewId();

/*!
 * NModesGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( NModesGUI, VizPropertyPanel )

/*!
 * NModesGUI event table definition
 */

BEGIN_EVENT_TABLE( NModesGUI, VizPropertyPanel )

////@begin NModesGUI event table entries
    EVT_GRID_SELECT_CELL( NModesGUI::OnSelectCell )

    EVT_RADIOBOX( ID_RADIOBOX_NMODE_VIZTYPE, NModesGUI::OnRadioboxNmodeViztypeSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_NMODE_DELAY, NModesGUI::OnTextctrlNmodeDelayEnter )

    EVT_BUTTON( ID_BITMAPBUTTON_START, NModesGUI::OnBitmapbuttonStartClick )

    EVT_BUTTON( ID_BITMAPBUTTON_STOP, NModesGUI::OnBitmapbuttonStopClick )

    EVT_BUTTON( ID_BUTTON_NMODE_VECCOLOR, NModesGUI::OnButtonNmodeVeccolorClick )

    EVT_CHECKBOX( ID_CHECKBOX_NMODE_VECSIGN, NModesGUI::OnCheckboxNmodeVecsignClick )

////@end NModesGUI event table entries

END_EVENT_TABLE()

/*!
 * NModesGUI constructors
 */

NModesGUI::NModesGUI( )
{
}

NModesGUI::NModesGUI( IPropCalculation* calculation, wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(calculation, parent, id, pos, size, style);
}

/*!
 * NModesGUI creator
 */

bool NModesGUI::Create( IPropCalculation* calculation, wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin NModesGUI member initialisation
    p_gridPlotSizer = NULL;
    p_ = NULL;
    p_sliderSizer = NULL;
    p_styleSizer = NULL;
    p_animateSizer = NULL;
    p_vectorSizer = NULL;
////@end NModesGUI member initialisation

////@begin NModesGUI creation
    VizPropertyPanel::Create( calculation, parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
////@end NModesGUI creation
    return true;
}

/*!
 * Control creation for NModesGUI
 */

void NModesGUI::CreateControls()
{    
////@begin NModesGUI content construction
    NModesGUI* itemVizPropertyPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemVizPropertyPanel1->SetSizer(itemBoxSizer2);

    p_gridPlotSizer = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(p_gridPlotSizer, 1, wxGROW|wxALL, 0);

    ewxGrid* itemGrid4 = new ewxGrid( itemVizPropertyPanel1, ID_GRID_NMODE, wxDefaultPosition, wxSize(200, 225), wxSUNKEN_BORDER|wxHSCROLL|wxVSCROLL );
    itemGrid4->SetDefaultColSize(50);
    itemGrid4->SetDefaultRowSize(25);
    itemGrid4->SetColLabelSize(40);
    itemGrid4->SetRowLabelSize(50);
    itemGrid4->CreateGrid(5, 5, wxGrid::wxGridSelectRows);
    p_gridPlotSizer->Add(itemGrid4, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer5, 0, wxALIGN_LEFT|wxALL, 0);

    p_ = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer5->Add(p_, 0, wxALIGN_LEFT|wxALL, 0);

    wxString itemRadioBox7Strings[] = {
        _("&Animation"),
        _("&Vector")
    };
    ewxRadioBox* itemRadioBox7 = new ewxRadioBox( itemVizPropertyPanel1, ID_RADIOBOX_NMODE_VIZTYPE, _T(""), wxDefaultPosition, wxDefaultSize, 2, itemRadioBox7Strings, 1, wxRA_SPECIFY_ROWS );
    p_->Add(itemRadioBox7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    p_sliderSizer = new wxBoxSizer(wxHORIZONTAL);
    p_->Add(p_sliderSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText9 = new ewxStaticText( itemVizPropertyPanel1, wxID_STATIC, _("Scale:"), wxDefaultPosition, wxDefaultSize, 0 );
    p_sliderSizer->Add(itemStaticText9, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    p_styleSizer = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer5->Add(p_styleSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    p_animateSizer = new wxBoxSizer(wxHORIZONTAL);
    p_styleSizer->Add(p_animateSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    ewxStaticText* itemStaticText12 = new ewxStaticText( itemVizPropertyPanel1, wxID_STATIC, _("Delay: "), wxDefaultPosition, wxDefaultSize, 0 );
    p_animateSizer->Add(itemStaticText12, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl13 = new ewxTextCtrl( itemVizPropertyPanel1, ID_TEXTCTRL_NMODE_DELAY, _T(""), wxDefaultPosition, wxSize(30, -1), wxTE_PROCESS_ENTER );
    p_animateSizer->Add(itemTextCtrl13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText14 = new ewxStaticText( itemVizPropertyPanel1, wxID_STATIC, _("50-5000 ms "), wxDefaultPosition, wxDefaultSize, 0 );
    p_animateSizer->Add(itemStaticText14, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    p_animateSizer->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBitmap itemBitmapButton16Bitmap(itemVizPropertyPanel1->GetBitmapResource(wxT("player_play.png")));
    ewxBitmapButton* itemBitmapButton16 = new ewxBitmapButton( itemVizPropertyPanel1, ID_BITMAPBUTTON_START, itemBitmapButton16Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    if (ShowToolTips())
        itemBitmapButton16->SetToolTip(_("animate normal mode"));
    p_animateSizer->Add(itemBitmapButton16, 0, wxALIGN_CENTER_VERTICAL, 0);

    p_animateSizer->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM, 5);

    wxBitmap itemBitmapButton18Bitmap(itemVizPropertyPanel1->GetBitmapResource(wxT("player_stop.png")));
    ewxBitmapButton* itemBitmapButton18 = new ewxBitmapButton( itemVizPropertyPanel1, ID_BITMAPBUTTON_STOP, itemBitmapButton18Bitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|wxBU_EXACTFIT );
    if (ShowToolTips())
        itemBitmapButton18->SetToolTip(_("stop animation"));
    p_animateSizer->Add(itemBitmapButton18, 0, wxALIGN_CENTER_VERTICAL, 0);

    p_vectorSizer = new wxBoxSizer(wxHORIZONTAL);
    p_styleSizer->Add(p_vectorSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    ewxButton* itemButton20 = new ewxButton( itemVizPropertyPanel1, ID_BUTTON_NMODE_VECCOLOR, _T(""), wxDefaultPosition, wxSize(24, 24), wxBU_EXACTFIT );
    p_vectorSizer->Add(itemButton20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxCheckBox* itemCheckBox21 = new ewxCheckBox( itemVizPropertyPanel1, ID_CHECKBOX_NMODE_VECSIGN, _("Use Negative Displacement"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox21->SetValue(false);
    p_vectorSizer->Add(itemCheckBox21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

////@end NModesGUI content construction
}

/*!
 * wxEVT_GRID_SELECT_CELL event handler for ID_GRID_NMODE
 */

void NModesGUI::OnSelectCell( wxGridEvent& event )
{
////@begin wxEVT_GRID_SELECT_CELL event handler for ID_GRID_NMODE in NModesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_GRID_SELECT_CELL event handler for ID_GRID_NMODE in NModesGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_RADIOBOX_NMODE_VIZTYPE
 */

void NModesGUI::OnRadioboxNmodeViztypeSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_RADIOBOX_NMODE_VIZTYPE in NModesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_RADIOBOX_NMODE_VIZTYPE in NModesGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NMODE_DELAY
 */

void NModesGUI::OnTextctrlNmodeDelayEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NMODE_DELAY in NModesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_NMODE_DELAY in NModesGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_START
 */

void NModesGUI::OnBitmapbuttonStartClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_START in NModesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_START in NModesGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_STOP
 */

void NModesGUI::OnBitmapbuttonStopClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_STOP in NModesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BITMAPBUTTON_STOP in NModesGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NMODE_VECCOLOR
 */

void NModesGUI::OnButtonNmodeVeccolorClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NMODE_VECCOLOR in NModesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_NMODE_VECCOLOR in NModesGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NMODE_VECSIGN
 */

void NModesGUI::OnCheckboxNmodeVecsignClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NMODE_VECSIGN in NModesGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_NMODE_VECSIGN in NModesGUI. 
}

/*!
 * Should we show tooltips?
 */

bool NModesGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap NModesGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin NModesGUI bitmap retrieval
    wxUnusedVar(name);
    if (name == wxT("player_play.png"))
    {
        ewxBitmap bitmap(_T("player_play.png"), wxBITMAP_TYPE_PNG);
        return bitmap;
    }
    else if (name == wxT("player_stop.png"))
    {
        ewxBitmap bitmap(_T("player_stop.png"), wxBITMAP_TYPE_PNG);
        return bitmap;
    }
    return wxNullBitmap;
////@end NModesGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon NModesGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin NModesGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end NModesGUI icon retrieval
}

/////////////////////////////////////////////////////////////////////////////
// Name:        SolvateEdGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "SolvateEdGUI.H"
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
#include "wxgui/ewxComboBox.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxMenuBar.H"
#include "wxgui/ewxFrame.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "SolvateEdGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID SolvateEdGUI::ID_COMBOBOX_SOLVATEED_SOLVENT = wxNewId();
const wxWindowID SolvateEdGUI::ID_STATIC_SLV_X_DIM = wxNewId();
const wxWindowID SolvateEdGUI::ID_STATIC_SLV_DIM_HEADER = wxNewId();
const wxWindowID SolvateEdGUI::ID_STATIC_SLV_TATOMS = wxNewId();
const wxWindowID SolvateEdGUI::ID_BUTTON_SOLVATEED_SOLVATE = wxNewId();
const wxWindowID SolvateEdGUI::ID_TEXTCTRL_SLV_MIN_NX = wxNewId();
const wxWindowID SolvateEdGUI::ID_STATIC_SLV_Y_LABEL = wxNewId();
const wxWindowID SolvateEdGUI::ID_TEXTCTRL_SLV_MIN_NZ = wxNewId();
const wxWindowID SolvateEdGUI::ID_STATIC_SLV_X_LABEL = wxNewId();
const wxWindowID SolvateEdGUI::ID_STATIC_SLV_Y_DIM = wxNewId();
const wxWindowID SolvateEdGUI::ID_TEXTCTRL_SLV_MIN_NY = wxNewId();
const wxWindowID SolvateEdGUI::ID_BUTTON_SOLVATEED_BUILDER = wxNewId();
const wxWindowID SolvateEdGUI::ID_BUTTON_SOLVATEED_CLEAR_SLV = wxNewId();
const wxWindowID SolvateEdGUI::ID_STATIC_SLV_Z_LABEL = wxNewId();
const wxWindowID SolvateEdGUI::ID_TEXTCTRL_SLV_RAD = wxNewId();
const wxWindowID SolvateEdGUI::ID_MENU_FEEDBACK = wxNewId();
const wxWindowID SolvateEdGUI::ID_SOLVATEED_FRAME = wxNewId();
const wxWindowID SolvateEdGUI::ID_STATIC_SLV_Z_DIM = wxNewId();

/*!
 * SolvateEdGUI type definition
 */

IMPLEMENT_CLASS( SolvateEdGUI, ewxFrame )

/*!
 * SolvateEdGUI event table definition
 */

BEGIN_EVENT_TABLE( SolvateEdGUI, ewxFrame )

////@begin SolvateEdGUI event table entries
    EVT_CLOSE( SolvateEdGUI::OnCloseWindow )

    EVT_MENU( wxID_SAVE, SolvateEdGUI::OnSaveClick )

    EVT_MENU( wxID_EXIT, SolvateEdGUI::OnExitClick )

    EVT_MENU( wxID_HELP, SolvateEdGUI::OnHelpClick )

    EVT_MENU( ID_MENU_FEEDBACK, SolvateEdGUI::OnMenuFeedbackClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_SLV_RAD, SolvateEdGUI::OnTextctrlSlvRadEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_SLV_MIN_NX, SolvateEdGUI::OnTextctrlSlvMinNxEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_SLV_MIN_NY, SolvateEdGUI::OnTextctrlSlvMinNyEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_SLV_MIN_NZ, SolvateEdGUI::OnTextctrlSlvMinNzEnter )

    EVT_BUTTON( ID_BUTTON_SOLVATEED_SOLVATE, SolvateEdGUI::OnButtonSolvateEdSolvateClick )

    EVT_BUTTON( ID_BUTTON_SOLVATEED_CLEAR_SLV, SolvateEdGUI::OnButtonSolvateEdClearSlvClick )

////@end SolvateEdGUI event table entries

END_EVENT_TABLE()

/*!
 * SolvateEdGUI constructors
 */

SolvateEdGUI::SolvateEdGUI( )
{
}

SolvateEdGUI::SolvateEdGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * SolvateEdGUI creator
 */

bool SolvateEdGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin SolvateEdGUI member initialisation
////@end SolvateEdGUI member initialisation

////@begin SolvateEdGUI creation
    SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end SolvateEdGUI creation
    return true;
}

/*!
 * Control creation for SolvateEdGUI
 */

void SolvateEdGUI::CreateControls()
{    
////@begin SolvateEdGUI content construction
    SolvateEdGUI* itemFrame1 = this;

    ewxMenuBar* menuBar = new ewxMenuBar;
    wxMenu* itemMenu3 = new wxMenu;
    itemMenu3->Append(wxID_SAVE, _("Save\tCtrl+s"), _T(""), wxITEM_NORMAL);
    itemMenu3->AppendSeparator();
    itemMenu3->Append(wxID_EXIT, _("Quit\tCtrl+q"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu3, _("File"));
    wxMenu* itemMenu7 = new wxMenu;
    itemMenu7->Append(wxID_HELP, _("on Solvation Editor..."), _T(""), wxITEM_NORMAL);
    itemMenu7->Append(ID_MENU_FEEDBACK, _("Support..."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu7, _("Help"));
    itemFrame1->SetMenuBar(menuBar);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(itemBoxSizer10);

    wxStaticBox* itemStaticBoxSizer11Static = new wxStaticBox(itemFrame1, wxID_ANY, _("Chemical System"));
    wxStaticBoxSizer* itemStaticBoxSizer11 = new wxStaticBoxSizer(itemStaticBoxSizer11Static, wxHORIZONTAL);
    itemBoxSizer10->Add(itemStaticBoxSizer11, 0, wxGROW|wxLEFT|wxRIGHT|wxBOTTOM, 5);

    ewxButton* itemButton12 = new ewxButton( itemFrame1, ID_BUTTON_SOLVATEED_BUILDER, _("Builder"), wxDefaultPosition, wxSize(64, 64), 0 );
    itemStaticBoxSizer11->Add(itemButton12, 0, wxALIGN_TOP|wxALL, 5);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxVERTICAL);
    itemStaticBoxSizer11->Add(itemBoxSizer13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer13->Add(itemBoxSizer14, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText15 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Total Number of Atoms:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemStaticText15, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText16 = new ewxStaticText( itemFrame1, ID_STATIC_SLV_TATOMS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemStaticText16, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText17 = new ewxStaticText( itemFrame1, ID_STATIC_SLV_DIM_HEADER, _("System Dimensions"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer13->Add(itemStaticText17, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer13->Add(itemBoxSizer18, 0, wxALIGN_LEFT|wxALL, 0);

    itemBoxSizer18->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL, 5);

    ewxStaticText* itemStaticText20 = new ewxStaticText( itemFrame1, ID_STATIC_SLV_X_LABEL, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText20, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText21 = new ewxStaticText( itemFrame1, ID_STATIC_SLV_X_DIM, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText21, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText22 = new ewxStaticText( itemFrame1, ID_STATIC_SLV_Y_LABEL, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText22, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText23 = new ewxStaticText( itemFrame1, ID_STATIC_SLV_Y_DIM, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText23, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText24 = new ewxStaticText( itemFrame1, ID_STATIC_SLV_Z_LABEL, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText24, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText25 = new ewxStaticText( itemFrame1, ID_STATIC_SLV_Z_DIM, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText25, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxStaticBox* itemStaticBoxSizer26Static = new wxStaticBox(itemFrame1, wxID_ANY, _("Solvate Settings"));
    wxStaticBoxSizer* itemStaticBoxSizer26 = new wxStaticBoxSizer(itemStaticBoxSizer26Static, wxHORIZONTAL);
    itemBoxSizer10->Add(itemStaticBoxSizer26, 0, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer27 = new wxBoxSizer(wxVERTICAL);
    itemStaticBoxSizer26->Add(itemBoxSizer27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer28 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer27->Add(itemBoxSizer28, 0, wxALIGN_LEFT|wxRIGHT|wxTOP|wxBOTTOM, 0);

    ewxStaticText* itemStaticText29 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Solvent Radius Scaling Parameter:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer28->Add(itemStaticText29, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl30 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_SLV_RAD, _("1.0"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer28->Add(itemTextCtrl30, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText31 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Minimum Number of Solvent Replicas"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer27->Add(itemStaticText31, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer32 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer27->Add(itemBoxSizer32, 0, wxALIGN_LEFT|wxALL, 5);

    itemBoxSizer32->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL, 5);

    ewxStaticText* itemStaticText34 = new ewxStaticText( itemFrame1, wxID_STATIC, _("nx:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(itemStaticText34, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl35 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_SLV_MIN_NX, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(itemTextCtrl35, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText36 = new ewxStaticText( itemFrame1, wxID_STATIC, _("    ny:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(itemStaticText36, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl37 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_SLV_MIN_NY, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(itemTextCtrl37, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText38 = new ewxStaticText( itemFrame1, wxID_STATIC, _("    nz:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer32->Add(itemStaticText38, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl39 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_SLV_MIN_NZ, _("1"), wxDefaultPosition, wxDefaultSize, 0 );
    itemTextCtrl39->SetMaxLength(1);
    itemBoxSizer32->Add(itemTextCtrl39, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer40 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer10->Add(itemBoxSizer40, 0, wxGROW|wxRIGHT|wxBOTTOM, 5);

    ewxStaticText* itemStaticText41 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Solvent:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemStaticText41, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    wxString* itemComboBox42Strings = NULL;
    ewxComboBox* itemComboBox42 = new ewxComboBox( itemFrame1, ID_COMBOBOX_SOLVATEED_SOLVENT, _T(""), wxDefaultPosition, wxDefaultSize, 0, itemComboBox42Strings, wxCB_DROPDOWN );
    itemBoxSizer40->Add(itemComboBox42, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton43 = new ewxButton( itemFrame1, ID_BUTTON_SOLVATEED_SOLVATE, _("Solvate"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemButton43, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    ewxButton* itemButton44 = new ewxButton( itemFrame1, ID_BUTTON_SOLVATEED_CLEAR_SLV, _("Clear Solvent"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton44->SetDefault();
    itemBoxSizer40->Add(itemButton44, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5);

    ewxStaticLine* itemStaticLine45 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer10->Add(itemStaticLine45, 0, wxGROW|wxBOTTOM, 5);

////@end SolvateEdGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_SOLVATEED_FRAME
 */

void SolvateEdGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_SOLVATEED_FRAME in SolvateEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_SOLVATEED_FRAME in SolvateEdGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE
 */

void SolvateEdGUI::OnSaveClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in SolvateEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_SAVE in SolvateEdGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
 */

void SolvateEdGUI::OnExitClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in SolvateEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in SolvateEdGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
 */

void SolvateEdGUI::OnHelpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in SolvateEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in SolvateEdGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK
 */

void SolvateEdGUI::OnMenuFeedbackClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK in SolvateEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK in SolvateEdGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_RAD
 */

void SolvateEdGUI::OnTextctrlSlvRadEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_RAD in SolvateEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_RAD in SolvateEdGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_MIN_NX
 */

void SolvateEdGUI::OnTextctrlSlvMinNxEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_MIN_NX in SolvateEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_MIN_NX in SolvateEdGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_MIN_NY
 */

void SolvateEdGUI::OnTextctrlSlvMinNyEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_MIN_NY in SolvateEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_MIN_NY in SolvateEdGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_MIN_NZ
 */

void SolvateEdGUI::OnTextctrlSlvMinNzEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_MIN_NZ in SolvateEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_SLV_MIN_NZ in SolvateEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SOLVATEED_SOLVATE
 */

void SolvateEdGUI::OnButtonSolvateEdSolvateClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SOLVATEED_SOLVATE in SolvateEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SOLVATEED_SOLVATE in SolvateEdGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SOLVATEED_CLEAR_SLV
 */

void SolvateEdGUI::OnButtonSolvateEdClearSlvClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SOLVATEED_CLEAR_SLV in SolvateEdGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_SOLVATEED_CLEAR_SLV in SolvateEdGUI. 
}

/*!
 * Should we show tooltips?
 */

bool SolvateEdGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap SolvateEdGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin SolvateEdGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end SolvateEdGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon SolvateEdGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin SolvateEdGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end SolvateEdGUI icon retrieval
}

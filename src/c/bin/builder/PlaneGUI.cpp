/////////////////////////////////////////////////////////////////////////////
// Name:        PlaneGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "PlaneGUI.H"
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
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "PlaneGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID PlaneGUI::ID_PANEL_TAB_SELECTION = wxNewId();
const wxWindowID PlaneGUI::ID_SPINCTRL_PLANE_H = wxNewId();
const wxWindowID PlaneGUI::ID_SPINCTRL_PLANE_L = wxNewId();
const wxWindowID PlaneGUI::ID_BUTTON_PLANE_COLOR = wxNewId();
const wxWindowID PlaneGUI::ID_NOTEBOOK_PLANE = wxNewId();
const wxWindowID PlaneGUI::ID_TEXTCTRL_PLANE_NAME = wxNewId();
const wxWindowID PlaneGUI::ID_SPINCTRL_PLANE_K = wxNewId();
const wxWindowID PlaneGUI::ID_DIALOG_PLANE = wxNewId();
const wxWindowID PlaneGUI::ID_PANEL_TAB_MILLER = wxNewId();
const wxWindowID PlaneGUI::ID_CHECKBOX_PLANE_TRANSPARENT = wxNewId();

/*!
 * PlaneGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( PlaneGUI, ewxDialog )

/*!
 * PlaneGUI event table definition
 */

BEGIN_EVENT_TABLE( PlaneGUI, ewxDialog )

////@begin PlaneGUI event table entries
    EVT_SPINCTRL( ID_SPINCTRL_PLANE_H, PlaneGUI::OnSpinctrlPlaneHUpdated )

    EVT_SPINCTRL( ID_SPINCTRL_PLANE_K, PlaneGUI::OnSpinctrlPlaneKUpdated )

    EVT_SPINCTRL( ID_SPINCTRL_PLANE_L, PlaneGUI::OnSpinctrlPlaneLUpdated )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PLANE_NAME, PlaneGUI::OnTextctrlPlaneNameEnter )

    EVT_BUTTON( ID_BUTTON_PLANE_COLOR, PlaneGUI::OnButtonPlaneColorClick )

    EVT_CHECKBOX( ID_CHECKBOX_PLANE_TRANSPARENT, PlaneGUI::OnCheckboxPlaneTransparentClick )

////@end PlaneGUI event table entries

END_EVENT_TABLE()

/*!
 * PlaneGUI constructors
 */

PlaneGUI::PlaneGUI( )
{
}

PlaneGUI::PlaneGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * PlaneGUI creator
 */

bool PlaneGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin PlaneGUI member initialisation
////@end PlaneGUI member initialisation

////@begin PlaneGUI creation
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end PlaneGUI creation
    return true;
}

/*!
 * Control creation for PlaneGUI
 */

void PlaneGUI::CreateControls()
{    
////@begin PlaneGUI content construction
    PlaneGUI* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_LEFT|wxALL, 3);

    ewxNotebook* itemNotebook4 = new ewxNotebook( itemDialog1, ID_NOTEBOOK_PLANE, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

    ewxPanel* itemPanel5 = new ewxPanel( itemNotebook4, ID_PANEL_TAB_MILLER, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel5->SetSizer(itemBoxSizer6);

    ewxStaticText* itemStaticText7 = new ewxStaticText( itemPanel5, wxID_STATIC, _("Miller Plane:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemStaticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    ewxStaticText* itemStaticText8 = new ewxStaticText( itemPanel5, wxID_STATIC, _("h:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemStaticText8, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxSpinCtrl* itemSpinCtrl9 = new ewxSpinCtrl( itemPanel5, ID_SPINCTRL_PLANE_H, _("1"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, -10, 10, 1 );
    itemBoxSizer6->Add(itemSpinCtrl9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemBoxSizer6->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText11 = new ewxStaticText( itemPanel5, wxID_STATIC, _("k:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemStaticText11, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxSpinCtrl* itemSpinCtrl12 = new ewxSpinCtrl( itemPanel5, ID_SPINCTRL_PLANE_K, _("1"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, -10, 10, 1 );
    itemBoxSizer6->Add(itemSpinCtrl12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemBoxSizer6->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText14 = new ewxStaticText( itemPanel5, wxID_STATIC, _("l:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer6->Add(itemStaticText14, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxSpinCtrl* itemSpinCtrl15 = new ewxSpinCtrl( itemPanel5, ID_SPINCTRL_PLANE_L, _("1"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, -10, 10, 1 );
    itemBoxSizer6->Add(itemSpinCtrl15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemNotebook4->AddPage(itemPanel5, _("Miller Planes"));

    ewxPanel* itemPanel16 = new ewxPanel( itemNotebook4, ID_PANEL_TAB_SELECTION, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxVERTICAL);
    itemPanel16->SetSizer(itemBoxSizer17);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer17->Add(itemBoxSizer18, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    ewxStaticText* itemStaticText19 = new ewxStaticText( itemPanel16, wxID_STATIC, _("A mean square plane will be generated based on the selection.\n Please select at least three atoms."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer18->Add(itemStaticText19, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    itemNotebook4->AddPage(itemPanel16, _("Mean Planes"));

    itemBoxSizer3->Add(itemNotebook4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer20 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer20, 0, wxALIGN_LEFT|wxALL, 5);

    ewxStaticText* itemStaticText21 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Name: "), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer20->Add(itemStaticText21, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl22 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_PLANE_NAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer20->Add(itemTextCtrl22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemBoxSizer20->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText24 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Color: "), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer20->Add(itemStaticText24, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxButton* itemButton25 = new ewxButton( itemDialog1, ID_BUTTON_PLANE_COLOR, _T(""), wxDefaultPosition, wxSize(24, 24), 0 );
    itemBoxSizer20->Add(itemButton25, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemBoxSizer20->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxCheckBox* itemCheckBox27 = new ewxCheckBox( itemDialog1, ID_CHECKBOX_PLANE_TRANSPARENT, _("Transparent"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox27->SetValue(true);
    itemBoxSizer20->Add(itemCheckBox27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

////@end PlaneGUI content construction
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PLANE_H
 */

void PlaneGUI::OnSpinctrlPlaneHUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PLANE_H in PlaneGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PLANE_H in PlaneGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PLANE_K
 */

void PlaneGUI::OnSpinctrlPlaneKUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PLANE_K in PlaneGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PLANE_K in PlaneGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PLANE_L
 */

void PlaneGUI::OnSpinctrlPlaneLUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PLANE_L in PlaneGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PLANE_L in PlaneGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLANE_NAME
 */

void PlaneGUI::OnTextctrlPlaneNameEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLANE_NAME in PlaneGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLANE_NAME in PlaneGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLANE_COLOR
 */

void PlaneGUI::OnButtonPlaneColorClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLANE_COLOR in PlaneGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLANE_COLOR in PlaneGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLANE_TRANSPARENT
 */

void PlaneGUI::OnCheckboxPlaneTransparentClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLANE_TRANSPARENT in PlaneGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLANE_TRANSPARENT in PlaneGUI. 
}

/*!
 * Should we show tooltips?
 */

bool PlaneGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap PlaneGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin PlaneGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end PlaneGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon PlaneGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin PlaneGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end PlaneGUI icon retrieval
}

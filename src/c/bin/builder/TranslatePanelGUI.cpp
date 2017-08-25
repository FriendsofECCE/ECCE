/////////////////////////////////////////////////////////////////////////////
// Name:        TranslatePanelGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "TranslatePanelGUI.H"
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
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "TranslatePanelGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID TranslatePanelGUI::ID_DIALOG_TRANSL = wxNewId();
const wxWindowID TranslatePanelGUI::ID_BUTTON_TRANSL_GO = wxNewId();
const wxWindowID TranslatePanelGUI::ID_TEXTCTRL_TRANSL_X = wxNewId();
const wxWindowID TranslatePanelGUI::ID_TEXTCTRL_TRANSL_Z = wxNewId();
const wxWindowID TranslatePanelGUI::ID_TEXTCTRL_TRANSL_Y = wxNewId();

/*!
 * TranslatePanelGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( TranslatePanelGUI, ewxPanel )

/*!
 * TranslatePanelGUI event table definition
 */

BEGIN_EVENT_TABLE( TranslatePanelGUI, ewxPanel )

////@begin TranslatePanelGUI event table entries
    EVT_BUTTON( ID_BUTTON_TRANSL_GO, TranslatePanelGUI::OnButtonTranslGoClick )

////@end TranslatePanelGUI event table entries

END_EVENT_TABLE()

/*!
 * TranslatePanelGUI constructors
 */

TranslatePanelGUI::TranslatePanelGUI( )
{
}

TranslatePanelGUI::TranslatePanelGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * TranslatePanelGUI creator
 */

bool TranslatePanelGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin TranslatePanelGUI member initialisation
////@end TranslatePanelGUI member initialisation

////@begin TranslatePanelGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end TranslatePanelGUI creation
    return true;
}

/*!
 * Control creation for TranslatePanelGUI
 */

void TranslatePanelGUI::CreateControls()
{    
////@begin TranslatePanelGUI content construction
    TranslatePanelGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxFlexGridSizer* itemFlexGridSizer4 = new wxFlexGridSizer(3, 3, 0, 0);
    itemBoxSizer3->Add(itemFlexGridSizer4, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText5 = new ewxStaticText( itemPanel1, wxID_STATIC, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl6 = new ewxTextCtrl( itemPanel1, ID_TEXTCTRL_TRANSL_X, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemTextCtrl6, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton7 = new ewxButton( itemPanel1, ID_BUTTON_TRANSL_GO, _("Go"), wxDefaultPosition, wxSize(30, -1), 0 );
    itemFlexGridSizer4->Add(itemButton7, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText8 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText8, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl9 = new ewxTextCtrl( itemPanel1, ID_TEXTCTRL_TRANSL_Y, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemTextCtrl9, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemFlexGridSizer4->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText11 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText11, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl12 = new ewxTextCtrl( itemPanel1, ID_TEXTCTRL_TRANSL_Z, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemTextCtrl12, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemFlexGridSizer4->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

////@end TranslatePanelGUI content construction
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_TRANSL_GO
 */

void TranslatePanelGUI::OnButtonTranslGoClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_TRANSL_GO in TranslatePanelGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_TRANSL_GO in TranslatePanelGUI. 
}

/*!
 * Should we show tooltips?
 */

bool TranslatePanelGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap TranslatePanelGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin TranslatePanelGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end TranslatePanelGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon TranslatePanelGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin TranslatePanelGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end TranslatePanelGUI icon retrieval
}

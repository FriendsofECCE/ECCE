/////////////////////////////////////////////////////////////////////////////
// Name:        WxBasisSetCoefficientsGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxBasisSetCoefficientsGUI.H"
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
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "WxBasisSetCoefficientsGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxBasisSetCoefficientsGUI::ID_TEXTCTRL_WXBASISSETCOEFFICIENTS_DETAIL = wxNewId();
const wxWindowID WxBasisSetCoefficientsGUI::ID_DIALOG_WXBASISSETCOEFFICIENTS = wxNewId();
const wxWindowID WxBasisSetCoefficientsGUI::ID_BUTTON_WXBASISSETCOEFFICIENTS_HELP = wxNewId();
const wxWindowID WxBasisSetCoefficientsGUI::ID_BUTTON_WXBASISSETCOEFFICIENTS_CLOSE = wxNewId();
const wxWindowID WxBasisSetCoefficientsGUI::ID_STATIC_WXBASISSETCOEFFICIENTS_HEADER = wxNewId();

/*!
 * WxBasisSetCoefficientsGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WxBasisSetCoefficientsGUI, ewxDialog )

/*!
 * WxBasisSetCoefficientsGUI event table definition
 */

BEGIN_EVENT_TABLE( WxBasisSetCoefficientsGUI, ewxDialog )

////@begin WxBasisSetCoefficientsGUI event table entries
    EVT_BUTTON( ID_BUTTON_WXBASISSETCOEFFICIENTS_CLOSE, WxBasisSetCoefficientsGUI::closeButtonClickCB )

////@end WxBasisSetCoefficientsGUI event table entries

END_EVENT_TABLE()

/*!
 * WxBasisSetCoefficientsGUI constructors
 */

WxBasisSetCoefficientsGUI::WxBasisSetCoefficientsGUI( )
{
}

WxBasisSetCoefficientsGUI::WxBasisSetCoefficientsGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * WxBasisSetCoefficientsGUI creator
 */

bool WxBasisSetCoefficientsGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxBasisSetCoefficientsGUI member initialisation
////@end WxBasisSetCoefficientsGUI member initialisation

////@begin WxBasisSetCoefficientsGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WxBasisSetCoefficientsGUI creation
    return true;
}

/*!
 * Control creation for WxBasisSetCoefficientsGUI
 */

void WxBasisSetCoefficientsGUI::CreateControls()
{    
////@begin WxBasisSetCoefficientsGUI content construction
    WxBasisSetCoefficientsGUI* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    ewxStaticText* itemStaticText3 = new ewxStaticText( itemDialog1, ID_STATIC_WXBASISSETCOEFFICIENTS_HEADER, _("Custom formatted for NWChem"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    itemBoxSizer2->Add(itemStaticText3, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl4 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_WXBASISSETCOEFFICIENTS_DETAIL, _T(""), wxDefaultPosition, wxSize(300, 300), wxTE_MULTILINE|wxTE_READONLY );
    itemTextCtrl4->SetFont(wxFont(10, wxDEFAULT, wxNORMAL, wxNORMAL, false, _T("Luxi Mono")));
    itemBoxSizer2->Add(itemTextCtrl4, 1, wxGROW|wxALL, 3);

    wxGridSizer* itemGridSizer5 = new wxGridSizer(1, 2, 0, 0);
    itemBoxSizer2->Add(itemGridSizer5, 0, wxGROW, 3);

    ewxButton* itemButton6 = new ewxButton( itemDialog1, ID_BUTTON_WXBASISSETCOEFFICIENTS_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer5->Add(itemButton6, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton7 = new ewxButton( itemDialog1, ID_BUTTON_WXBASISSETCOEFFICIENTS_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer5->Add(itemButton7, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

////@end WxBasisSetCoefficientsGUI content construction
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISSETCOEFFICIENTS_CLOSE
 */

void WxBasisSetCoefficientsGUI::closeButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISSETCOEFFICIENTS_CLOSE in WxBasisSetCoefficientsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXBASISSETCOEFFICIENTS_CLOSE in WxBasisSetCoefficientsGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxBasisSetCoefficientsGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxBasisSetCoefficientsGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxBasisSetCoefficientsGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxBasisSetCoefficientsGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxBasisSetCoefficientsGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxBasisSetCoefficientsGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxBasisSetCoefficientsGUI icon retrieval
}

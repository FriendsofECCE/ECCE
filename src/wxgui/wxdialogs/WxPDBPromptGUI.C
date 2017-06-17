/////////////////////////////////////////////////////////////////////////////
// Name:        WxPDBPromptGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxPDBPromptGUI.H"
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
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
////@end includes

#include "wxgui/WxPDBPromptGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxPDBPromptGUI::ID_CHAIN_CHOICE = wxNewId();
const wxWindowID WxPDBPromptGUI::ID_BUTTON_HELP = wxNewId();
const wxWindowID WxPDBPromptGUI::ID_PDBPROMPT_DIALOG = wxNewId();
const wxWindowID WxPDBPromptGUI::ID_BUTTON_CANCEL = wxNewId();
const wxWindowID WxPDBPromptGUI::ID_ALT_LOC_CHOICE = wxNewId();
const wxWindowID WxPDBPromptGUI::ID_BUTTON_OK = wxNewId();
const wxWindowID WxPDBPromptGUI::ID_SPINCTRL_PDBMODEL = wxNewId();
const wxWindowID WxPDBPromptGUI::ID_PROMPT_MODEL_OPTIONS_TEXT = wxNewId();

/*!
 * WxPDBPromptGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WxPDBPromptGUI, ewxDialog )

/*!
 * WxPDBPromptGUI event table definition
 */

BEGIN_EVENT_TABLE( WxPDBPromptGUI, ewxDialog )

////@begin WxPDBPromptGUI event table entries
    EVT_SPINCTRL( ID_SPINCTRL_PDBMODEL, WxPDBPromptGUI::OnSpinctrlPdbmodelUpdated )

    EVT_CHOICE( ID_ALT_LOC_CHOICE, WxPDBPromptGUI::OnAltLocChoiceSelected )

    EVT_CHOICE( ID_CHAIN_CHOICE, WxPDBPromptGUI::OnChainChoiceSelected )

    EVT_BUTTON( ID_BUTTON_OK, WxPDBPromptGUI::OnButtonOkClick )

    EVT_BUTTON( ID_BUTTON_CANCEL, WxPDBPromptGUI::OnButtonCancelClick )

    EVT_BUTTON( ID_BUTTON_HELP, WxPDBPromptGUI::OnButtonHelpClick )

////@end WxPDBPromptGUI event table entries

END_EVENT_TABLE()

/*!
 * WxPDBPromptGUI constructors
 */

WxPDBPromptGUI::WxPDBPromptGUI( )
{
}

WxPDBPromptGUI::WxPDBPromptGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * WxPDBPromptGUI creator
 */

bool WxPDBPromptGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxPDBPromptGUI member initialisation
////@end WxPDBPromptGUI member initialisation

////@begin WxPDBPromptGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WxPDBPromptGUI creation
    return true;
}

/*!
 * Control creation for WxPDBPromptGUI
 */

void WxPDBPromptGUI::CreateControls()
{    
////@begin WxPDBPromptGUI content construction
    WxPDBPromptGUI* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText4 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Read which model:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxSpinCtrl* itemSpinCtrl5 = new ewxSpinCtrl( itemDialog1, ID_SPINCTRL_PDBMODEL, _("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 1 );
    itemBoxSizer3->Add(itemSpinCtrl5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText6 = new ewxStaticText( itemDialog1, ID_PROMPT_MODEL_OPTIONS_TEXT, _("[1]"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText6, 0, wxALIGN_BOTTOM|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer7, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText8 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Alternate location:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer7->Add(itemStaticText8, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* itemChoice9Strings = NULL;
    ewxChoice* itemChoice9 = new ewxChoice( itemDialog1, ID_ALT_LOC_CHOICE, wxDefaultPosition, wxDefaultSize, 0, itemChoice9Strings, 0 );
    itemChoice9->SetStringSelection(_("A"));
    itemBoxSizer7->Add(itemChoice9, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer10, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText11 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Select chain:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemStaticText11, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString* itemChoice12Strings = NULL;
    ewxChoice* itemChoice12 = new ewxChoice( itemDialog1, ID_CHAIN_CHOICE, wxDefaultPosition, wxDefaultSize, 0, itemChoice12Strings, 0 );
    itemChoice12->SetStringSelection(_("All"));
    itemBoxSizer10->Add(itemChoice12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer13, 0, wxGROW|wxALL, 5);

    ewxStaticLine* itemStaticLine14 = new ewxStaticLine( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer13->Add(itemStaticLine14, 1, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer15 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer15, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    ewxButton* itemButton16 = new ewxButton( itemDialog1, ID_BUTTON_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer15->Add(itemButton16, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton17 = new ewxButton( itemDialog1, ID_BUTTON_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer15->Add(itemButton17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton18 = new ewxButton( itemDialog1, ID_BUTTON_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer15->Add(itemButton18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end WxPDBPromptGUI content construction
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PDBMODEL
 */

void WxPDBPromptGUI::OnSpinctrlPdbmodelUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PDBMODEL in WxPDBPromptGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PDBMODEL in WxPDBPromptGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_ALT_LOC_CHOICE
 */

void WxPDBPromptGUI::OnAltLocChoiceSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_ALT_LOC_CHOICE in WxPDBPromptGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_ALT_LOC_CHOICE in WxPDBPromptGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHAIN_CHOICE
 */

void WxPDBPromptGUI::OnChainChoiceSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHAIN_CHOICE in WxPDBPromptGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHAIN_CHOICE in WxPDBPromptGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_OK
 */

void WxPDBPromptGUI::OnButtonOkClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_OK in WxPDBPromptGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_OK in WxPDBPromptGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CANCEL
 */

void WxPDBPromptGUI::OnButtonCancelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CANCEL in WxPDBPromptGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_CANCEL in WxPDBPromptGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_HELP
 */

void WxPDBPromptGUI::OnButtonHelpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_HELP in WxPDBPromptGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_HELP in WxPDBPromptGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxPDBPromptGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxPDBPromptGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxPDBPromptGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxPDBPromptGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxPDBPromptGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxPDBPromptGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxPDBPromptGUI icon retrieval
}

/////////////////////////////////////////////////////////////////////////////
// Name:        ResidueIndexPromptGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "ResidueIndexPromptGUI.H"
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
#include "wxgui/ewxRadioButton.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "wxviz/ResidueIndexPromptGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID ResidueIndexPromptGUI::ID_CHOICE_RES_SELECT = wxNewId();
const wxWindowID ResidueIndexPromptGUI::ID_RADIOBUTTON_RESPROMPT_SELECT = wxNewId();
const wxWindowID ResidueIndexPromptGUI::ID_RADIOBUTTON_RESPROMPT_NEW = wxNewId();
const wxWindowID ResidueIndexPromptGUI::ID_CHOICE_RES_NEW = wxNewId();
const wxWindowID ResidueIndexPromptGUI::ID_RADIOBUTTON_RESPROMPT_DELETE = wxNewId();
const wxWindowID ResidueIndexPromptGUI::wxID_STATIC_NAME = wxNewId();
const wxWindowID ResidueIndexPromptGUI::ID_TEXTCTRL_RESPROMPT_NAME = wxNewId();
const wxWindowID ResidueIndexPromptGUI::ID_DIALOG_RESPROMPT = wxNewId();

/*!
 * ResidueIndexPromptGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( ResidueIndexPromptGUI, ewxDialog )

/*!
 * ResidueIndexPromptGUI event table definition
 */

BEGIN_EVENT_TABLE( ResidueIndexPromptGUI, ewxDialog )

////@begin ResidueIndexPromptGUI event table entries
    EVT_RADIOBUTTON( ID_RADIOBUTTON_RESPROMPT_SELECT, ResidueIndexPromptGUI::OnRadiobuttonRespromptSelectSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_RESPROMPT_NEW, ResidueIndexPromptGUI::OnRadiobuttonRespromptNewSelected )

    EVT_RADIOBUTTON( ID_RADIOBUTTON_RESPROMPT_DELETE, ResidueIndexPromptGUI::OnRadiobuttonRespromptDeleteSelected )

    EVT_BUTTON( wxID_OK, ResidueIndexPromptGUI::OnOkClick )

    EVT_BUTTON( wxID_CANCEL, ResidueIndexPromptGUI::OnCancelClick )

////@end ResidueIndexPromptGUI event table entries

END_EVENT_TABLE()

/*!
 * ResidueIndexPromptGUI constructors
 */

ResidueIndexPromptGUI::ResidueIndexPromptGUI( )
{
}

ResidueIndexPromptGUI::ResidueIndexPromptGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * ResidueIndexPromptGUI creator
 */

bool ResidueIndexPromptGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin ResidueIndexPromptGUI member initialisation
    p_masterSizer = NULL;
    p_sizerForSelectPopup = NULL;
    p_sizerForInsertPopup = NULL;
////@end ResidueIndexPromptGUI member initialisation

////@begin ResidueIndexPromptGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end ResidueIndexPromptGUI creation
    return true;
}

/*!
 * Control creation for ResidueIndexPromptGUI
 */

void ResidueIndexPromptGUI::CreateControls()
{    
////@begin ResidueIndexPromptGUI content construction
    ResidueIndexPromptGUI* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(itemBoxSizer4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxRadioButton* itemRadioButton5 = new ewxRadioButton( itemDialog1, ID_RADIOBUTTON_RESPROMPT_SELECT, _("Select residue to edit"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton5->SetValue(true);
    itemBoxSizer4->Add(itemRadioButton5, 0, wxALIGN_LEFT|wxALL, 0);

    ewxRadioButton* itemRadioButton6 = new ewxRadioButton( itemDialog1, ID_RADIOBUTTON_RESPROMPT_NEW, _("Insert new residue"), wxDefaultPosition, wxDefaultSize, wxRB_SINGLE );
    itemRadioButton6->SetValue(false);
    itemBoxSizer4->Add(itemRadioButton6, 0, wxALIGN_LEFT|wxALL, 0);

    ewxRadioButton* itemRadioButton7 = new ewxRadioButton( itemDialog1, ID_RADIOBUTTON_RESPROMPT_DELETE, _("Clear residue info"), wxDefaultPosition, wxDefaultSize, 0 );
    itemRadioButton7->SetValue(false);
    if (ShowToolTips())
        itemRadioButton7->SetToolTip(_("Removes residue info from system treating the system as simply atoms and bonds."));
    itemBoxSizer4->Add(itemRadioButton7, 0, wxALIGN_LEFT|wxALL, 0);

    p_masterSizer = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer3->Add(p_masterSizer, 0, wxALIGN_TOP|wxALL, 0);

    p_sizerForSelectPopup = new wxBoxSizer(wxHORIZONTAL);
    p_masterSizer->Add(p_sizerForSelectPopup, 0, wxALIGN_LEFT|wxALL, 0);

    wxString* itemChoice10Strings = NULL;
    ewxChoice* itemChoice10 = new ewxChoice( itemDialog1, ID_CHOICE_RES_SELECT, wxDefaultPosition, wxDefaultSize, 0, itemChoice10Strings, 0 );
    p_sizerForSelectPopup->Add(itemChoice10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_sizerForInsertPopup = new wxBoxSizer(wxHORIZONTAL);
    p_masterSizer->Add(p_sizerForInsertPopup, 0, wxALIGN_LEFT|wxALL, 0);

    wxString* itemChoice12Strings = NULL;
    ewxChoice* itemChoice12 = new ewxChoice( itemDialog1, ID_CHOICE_RES_NEW, wxDefaultPosition, wxDefaultSize, 0, itemChoice12Strings, 0 );
    p_sizerForInsertPopup->Add(itemChoice12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText13 = new ewxStaticText( itemDialog1, wxID_STATIC_NAME, _("Name"), wxDefaultPosition, wxDefaultSize, 0 );
    p_sizerForInsertPopup->Add(itemStaticText13, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 0);

    ewxTextCtrl* itemTextCtrl14 = new ewxTextCtrl( itemDialog1, ID_TEXTCTRL_RESPROMPT_NAME, _("UNK"), wxDefaultPosition, wxSize(50, -1), 0 );
    p_sizerForInsertPopup->Add(itemTextCtrl14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer15 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer15, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    ewxStaticText* itemStaticText16 = new ewxStaticText( itemDialog1, wxID_STATIC, _("To avoid this dialog in the future, specify which residue\nyou intend to edit in the Residue Tool by checking the\nappropriate box."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer15->Add(itemStaticText16, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer17 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer17, 1, wxGROW|wxALL, 0);

    ewxStaticLine* itemStaticLine18 = new ewxStaticLine( itemDialog1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer17->Add(itemStaticLine18, 1, wxGROW|wxALL, 5);

    wxBoxSizer* itemBoxSizer19 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer19, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    ewxButton* itemButton20 = new ewxButton( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(itemButton20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton21 = new ewxButton( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(itemButton21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end ResidueIndexPromptGUI content construction
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_RESPROMPT_SELECT
 */

void ResidueIndexPromptGUI::OnRadiobuttonRespromptSelectSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_RESPROMPT_SELECT in ResidueIndexPromptGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_RESPROMPT_SELECT in ResidueIndexPromptGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_RESPROMPT_NEW
 */

void ResidueIndexPromptGUI::OnRadiobuttonRespromptNewSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_RESPROMPT_NEW in ResidueIndexPromptGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_RESPROMPT_NEW in ResidueIndexPromptGUI. 
}

/*!
 * wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_RESPROMPT_DELETE
 */

void ResidueIndexPromptGUI::OnRadiobuttonRespromptDeleteSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_RESPROMPT_DELETE in ResidueIndexPromptGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_RADIOBUTTON_RESPROMPT_DELETE in ResidueIndexPromptGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void ResidueIndexPromptGUI::OnOkClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK in ResidueIndexPromptGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK in ResidueIndexPromptGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void ResidueIndexPromptGUI::OnCancelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in ResidueIndexPromptGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL in ResidueIndexPromptGUI. 
}

/*!
 * Should we show tooltips?
 */

bool ResidueIndexPromptGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap ResidueIndexPromptGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin ResidueIndexPromptGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end ResidueIndexPromptGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon ResidueIndexPromptGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin ResidueIndexPromptGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end ResidueIndexPromptGUI icon retrieval
}

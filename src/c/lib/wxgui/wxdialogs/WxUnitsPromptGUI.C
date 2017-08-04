/////////////////////////////////////////////////////////////////////////////
// Name:        WxUnitsPromptGUI.C
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxUnitsPromptGUI.H"
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
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
////@end includes

#include "wxgui/WxUnitsPromptGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxUnitsPromptGUI::ID_CHOICE_UNITS = wxNewId();
const wxWindowID WxUnitsPromptGUI::ID_DIALOG = wxNewId();
const wxWindowID WxUnitsPromptGUI::ID_CHECKBOX_GENBONDS = wxNewId();

/*!
 * WxUnitsPromptGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WxUnitsPromptGUI, ewxDialog )

/*!
 * WxUnitsPromptGUI event table definition
 */

BEGIN_EVENT_TABLE( WxUnitsPromptGUI, ewxDialog )

////@begin WxUnitsPromptGUI event table entries
////@end WxUnitsPromptGUI event table entries

END_EVENT_TABLE()

/*!
 * WxUnitsPromptGUI constructors
 */

WxUnitsPromptGUI::WxUnitsPromptGUI( )
{
}

WxUnitsPromptGUI::WxUnitsPromptGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, caption, pos, size, style);
}

/*!
 * WxUnitsPromptGUI creator
 */

bool WxUnitsPromptGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxUnitsPromptGUI member initialisation
    p_units = NULL;
    p_genBonds = NULL;
////@end WxUnitsPromptGUI member initialisation

////@begin WxUnitsPromptGUI creation
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    ewxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WxUnitsPromptGUI creation
    return true;
}

/*!
 * Control creation for WxUnitsPromptGUI
 */

void WxUnitsPromptGUI::CreateControls()
{    
////@begin WxUnitsPromptGUI content construction
    WxUnitsPromptGUI* itemDialog1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemDialog1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    ewxStaticText* itemStaticText4 = new ewxStaticText( itemDialog1, wxID_STATIC, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString p_unitsStrings[] = {
        _("Angstroms"),
        _("Bohr"),
        _("Picometers"),
        _("Nanometers")
    };
    p_units = new ewxChoice( itemDialog1, ID_CHOICE_UNITS, wxDefaultPosition, wxDefaultSize, 4, p_unitsStrings, 0 );
    p_units->SetStringSelection(_("Angstroms"));
    itemBoxSizer3->Add(p_units, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_genBonds = new ewxCheckBox( itemDialog1, ID_CHECKBOX_GENBONDS, _("Display Bonds"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    p_genBonds->SetValue(true);
    p_genBonds->Show(false);
    itemBoxSizer2->Add(p_genBonds, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxStdDialogButtonSizer* itemStdDialogButtonSizer7 = new wxStdDialogButtonSizer;

    itemBoxSizer2->Add(itemStdDialogButtonSizer7, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
    ewxButton* itemButton8 = new ewxButton( itemDialog1, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStdDialogButtonSizer7->AddButton(itemButton8);

    ewxButton* itemButton9 = new ewxButton( itemDialog1, wxID_OK, _("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStdDialogButtonSizer7->AddButton(itemButton9);

    itemStdDialogButtonSizer7->Realize();

////@end WxUnitsPromptGUI content construction
}

/*!
 * Should we show tooltips?
 */

bool WxUnitsPromptGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxUnitsPromptGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxUnitsPromptGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxUnitsPromptGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxUnitsPromptGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxUnitsPromptGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxUnitsPromptGUI icon retrieval
}

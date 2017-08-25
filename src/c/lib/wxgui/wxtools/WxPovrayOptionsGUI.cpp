/////////////////////////////////////////////////////////////////////////////
// Name:        WxPovrayOptionsGUI.C
// Purpose:     
// Author:      Jeff Daily
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxPovrayOptionsGUI.H"
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
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "wxgui/WxPovrayOptionsGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxPovrayOptionsGUI::ID_POVRAY_PANEL = wxNewId();
const wxWindowID WxPovrayOptionsGUI::ID_POVRAY_HEIGHT = wxNewId();
const wxWindowID WxPovrayOptionsGUI::ID_POVRAY_WIDTH = wxNewId();
const wxWindowID WxPovrayOptionsGUI::ID_POVRAY_DISPLAY = wxNewId();
const wxWindowID WxPovrayOptionsGUI::ID_POVRAY_ISOSURFACES = wxNewId();
const wxWindowID WxPovrayOptionsGUI::ID_POVRAY_BONDS = wxNewId();
const wxWindowID WxPovrayOptionsGUI::ID_POVRAY_FINISH = wxNewId();

/*!
 * WxPovrayOptionsGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WxPovrayOptionsGUI, ewxPanel )

/*!
 * WxPovrayOptionsGUI event table definition
 */

BEGIN_EVENT_TABLE( WxPovrayOptionsGUI, ewxPanel )

////@begin WxPovrayOptionsGUI event table entries
////@end WxPovrayOptionsGUI event table entries

END_EVENT_TABLE()

/*!
 * WxPovrayOptionsGUI constructors
 */

WxPovrayOptionsGUI::WxPovrayOptionsGUI( )
{
}

WxPovrayOptionsGUI::WxPovrayOptionsGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * WxPovrayOptionsGUI creator
 */

bool WxPovrayOptionsGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxPovrayOptionsGUI member initialisation
////@end WxPovrayOptionsGUI member initialisation

////@begin WxPovrayOptionsGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WxPovrayOptionsGUI creation
    return true;
}

/*!
 * Control creation for WxPovrayOptionsGUI
 */

void WxPovrayOptionsGUI::CreateControls()
{    
////@begin WxPovrayOptionsGUI content construction
    WxPovrayOptionsGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxGROW, 5);

    ewxStaticText* itemStaticText4 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Finish:"), wxDefaultPosition, wxSize(75, -1), 0 );
    itemBoxSizer3->Add(itemStaticText4, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString itemChoice5Strings[] = {
        _("Plastic"),
        _("Soft"),
        _("Metallic"),
        _("Wood")
    };
    ewxChoice* itemChoice5 = new ewxChoice( itemPanel1, ID_POVRAY_FINISH, wxDefaultPosition, wxDefaultSize, 4, itemChoice5Strings, 0 );
    itemChoice5->SetStringSelection(_("Plastic"));
    itemBoxSizer3->Add(itemChoice5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer6, 0, wxGROW, 5);

    ewxStaticText* itemStaticText7 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Bonds:"), wxDefaultPosition, wxSize(75, -1), 0 );
    itemBoxSizer6->Add(itemStaticText7, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString itemChoice8Strings[] = {
        _("Bicolor-Proportional"),
        _("Bicolor-Symmetric"),
        _("Brass")
    };
    ewxChoice* itemChoice8 = new ewxChoice( itemPanel1, ID_POVRAY_BONDS, wxDefaultPosition, wxDefaultSize, 3, itemChoice8Strings, 0 );
    itemChoice8->SetStringSelection(_("Bicolor-Proportional"));
    itemBoxSizer6->Add(itemChoice8, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer9, 0, wxGROW, 5);

    ewxStaticText* itemStaticText10 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Isosurfaces:"), wxDefaultPosition, wxSize(75, -1), 0 );
    itemBoxSizer9->Add(itemStaticText10, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString itemChoice11Strings[] = {
        _("Glassy"),
        _("Soft")
    };
    ewxChoice* itemChoice11 = new ewxChoice( itemPanel1, ID_POVRAY_ISOSURFACES, wxDefaultPosition, wxDefaultSize, 2, itemChoice11Strings, 0 );
    itemChoice11->SetStringSelection(_("Glassy"));
    itemBoxSizer9->Add(itemChoice11, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxStaticBox* itemStaticBoxSizer12Static = new wxStaticBox(itemPanel1, wxID_ANY, _("Preview"));
    wxStaticBoxSizer* itemStaticBoxSizer12 = new wxStaticBoxSizer(itemStaticBoxSizer12Static, wxHORIZONTAL);
    itemBoxSizer2->Add(itemStaticBoxSizer12, 0, wxGROW, 5);

    ewxCheckBox* itemCheckBox13 = new ewxCheckBox( itemPanel1, ID_POVRAY_DISPLAY, _("Display"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox13->SetValue(false);
    itemStaticBoxSizer12->Add(itemCheckBox13, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText14 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer12->Add(itemStaticText14, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl15 = new ewxTextCtrl( itemPanel1, ID_POVRAY_WIDTH, _("800"), wxDefaultPosition, wxSize(50, -1), 0 );
    itemStaticBoxSizer12->Add(itemTextCtrl15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxStaticText* itemStaticText16 = new ewxStaticText( itemPanel1, wxID_STATIC, _("Height:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticBoxSizer12->Add(itemStaticText16, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl17 = new ewxTextCtrl( itemPanel1, ID_POVRAY_HEIGHT, _("600"), wxDefaultPosition, wxSize(50, -1), 0 );
    itemStaticBoxSizer12->Add(itemTextCtrl17, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end WxPovrayOptionsGUI content construction
}

/*!
 * Should we show tooltips?
 */

bool WxPovrayOptionsGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxPovrayOptionsGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxPovrayOptionsGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxPovrayOptionsGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxPovrayOptionsGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxPovrayOptionsGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxPovrayOptionsGUI icon retrieval
}

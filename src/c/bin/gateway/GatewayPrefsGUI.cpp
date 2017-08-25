/////////////////////////////////////////////////////////////////////////////
// Name:        GatewayPrefsGUI.C
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "GatewayPrefsGUI.H"
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
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxFrame.H"
////@end includes

#include "GatewayPrefsGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID GatewayPrefsGUI::ID_SHOWBUSY_CHECKBOX = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_BEEPWARN_CHECKBOX = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_ALWAYS_ON_TOP = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_UNIT_CHOICE = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_TIMEFORMAT_CHOICE = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_ORIENTATION = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_FONTSIZE = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_CLOSE_BUTTON = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_BEEPERROR_CHECKBOX = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_SHOWCONFIRM_CHECKBOX = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_THEME_CHOICE = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_RESETALL_BUTTON = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_GATEWAYPREF_FRAME = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_LEFTCLICK_NEWAPP = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_HELP_BUTTON = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_SAVEPASSWORDS_CHECKBOX = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_DATEFORMAT_CHOICE = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_FOCUS_FOLLOW_MOUSE = wxNewId();
const wxWindowID GatewayPrefsGUI::ID_CLOSESHELLS_CHECKBOX = wxNewId();

/*!
 * GatewayPrefsGUI type definition
 */

IMPLEMENT_CLASS( GatewayPrefsGUI, ewxFrame )

/*!
 * GatewayPrefsGUI event table definition
 */

BEGIN_EVENT_TABLE( GatewayPrefsGUI, ewxFrame )

////@begin GatewayPrefsGUI event table entries
    EVT_CLOSE( GatewayPrefsGUI::OnCloseWindow )

    EVT_CHOICE( ID_THEME_CHOICE, GatewayPrefsGUI::OnThemeChoiceSelected )

    EVT_CHOICE( ID_FONTSIZE, GatewayPrefsGUI::OnGlobalPrefChange )

    EVT_CHOICE( ID_DATEFORMAT_CHOICE, GatewayPrefsGUI::OnGlobalPrefChange )

    EVT_CHOICE( ID_TIMEFORMAT_CHOICE, GatewayPrefsGUI::OnGlobalPrefChange )

    EVT_CHOICE( ID_UNIT_CHOICE, GatewayPrefsGUI::OnGlobalPrefChange )

    EVT_CHECKBOX( ID_BEEPERROR_CHECKBOX, GatewayPrefsGUI::OnGlobalPrefChange )

    EVT_CHECKBOX( ID_BEEPWARN_CHECKBOX, GatewayPrefsGUI::OnGlobalPrefChange )

    EVT_CHECKBOX( ID_FOCUS_FOLLOW_MOUSE, GatewayPrefsGUI::OnGlobalPrefChange )

    EVT_CHECKBOX( ID_SHOWCONFIRM_CHECKBOX, GatewayPrefsGUI::OnGatewayPrefChange )

    EVT_CHECKBOX( ID_SHOWBUSY_CHECKBOX, GatewayPrefsGUI::OnGatewayPrefChange )

    EVT_CHECKBOX( ID_ALWAYS_ON_TOP, GatewayPrefsGUI::OnGatewayPrefChange )

    EVT_CHECKBOX( ID_LEFTCLICK_NEWAPP, GatewayPrefsGUI::OnGatewayPrefChange )

    EVT_CHECKBOX( ID_CLOSESHELLS_CHECKBOX, GatewayPrefsGUI::OnGatewayPrefChange )

    EVT_CHECKBOX( ID_SAVEPASSWORDS_CHECKBOX, GatewayPrefsGUI::OnGatewayPrefChange )

    EVT_CHOICE( ID_ORIENTATION, GatewayPrefsGUI::OnGatewayPrefChange )

    EVT_BUTTON( ID_CLOSE_BUTTON, GatewayPrefsGUI::OnCloseButtonClick )

    EVT_BUTTON( ID_RESETALL_BUTTON, GatewayPrefsGUI::OnResetAllButtonClick )

////@end GatewayPrefsGUI event table entries

END_EVENT_TABLE()

/*!
 * GatewayPrefsGUI constructors
 */

GatewayPrefsGUI::GatewayPrefsGUI( )
{
}

GatewayPrefsGUI::GatewayPrefsGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * GatewayPrefsGUI creator
 */

bool GatewayPrefsGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin GatewayPrefsGUI member initialisation
    p_colorTheme = NULL;
    p_fontSize = NULL;
    p_dateFormat = NULL;
    p_timeFormat = NULL;
    p_unit = NULL;
    p_beepError = NULL;
    p_beepWarn = NULL;
    p_focus = NULL;
    p_confirmExit = NULL;
    p_showBusy = NULL;
    p_alwaysOnTop = NULL;
    p_leftClickNewApp = NULL;
    p_closeShells = NULL;
    p_savePasswords = NULL;
    p_orientation = NULL;
    p_stateIconSizer = NULL;
    p_resetAll = NULL;
////@end GatewayPrefsGUI member initialisation

////@begin GatewayPrefsGUI creation
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end GatewayPrefsGUI creation
    return true;
}

/*!
 * Control creation for GatewayPrefsGUI
 */

void GatewayPrefsGUI::CreateControls()
{    
////@begin GatewayPrefsGUI content construction
    GatewayPrefsGUI* itemFrame1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(itemBoxSizer2);

    wxStaticBox* itemStaticBoxSizer3Static = new wxStaticBox(itemFrame1, wxID_ANY, _("Global Preferences"));
    wxStaticBoxSizer* itemStaticBoxSizer3 = new wxStaticBoxSizer(itemStaticBoxSizer3Static, wxVERTICAL);
    itemBoxSizer2->Add(itemStaticBoxSizer3, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

    wxFlexGridSizer* itemFlexGridSizer4 = new wxFlexGridSizer(2, 2, 5, 5);
    itemStaticBoxSizer3->Add(itemFlexGridSizer4, 0, wxGROW|wxALL, 5);

    ewxStaticText* itemStaticText5 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Color Theme:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText5, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    wxString p_colorThemeStrings[] = {
        _("Site Default"),
        _("Classic"),
        _("PNNL"),
        _("EMSL"),
        _("PNNL_EMSL")
    };
    p_colorTheme = new ewxChoice( itemFrame1, ID_THEME_CHOICE, wxDefaultPosition, wxDefaultSize, 5, p_colorThemeStrings, 0 );
    p_colorTheme->SetStringSelection(_("Site Default"));
    itemFlexGridSizer4->Add(p_colorTheme, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0);

    ewxStaticText* itemStaticText7 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Font Size:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText7, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    wxString p_fontSizeStrings[] = {
        _("Small"),
        _("Medium"),
        _("Large"),
        _("Extra Large")
    };
    p_fontSize = new ewxChoice( itemFrame1, ID_FONTSIZE, wxDefaultPosition, wxDefaultSize, 4, p_fontSizeStrings, 0 );
    p_fontSize->SetStringSelection(_("Medium"));
    itemFlexGridSizer4->Add(p_fontSize, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0);

    ewxStaticText* itemStaticText9 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Date Format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText9, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    wxString* p_dateFormatStrings = NULL;
    p_dateFormat = new ewxChoice( itemFrame1, ID_DATEFORMAT_CHOICE, wxDefaultPosition, wxDefaultSize, 0, p_dateFormatStrings, 0 );
    itemFlexGridSizer4->Add(p_dateFormat, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0);

    ewxStaticText* itemStaticText11 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Time Format:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText11, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString* p_timeFormatStrings = NULL;
    p_timeFormat = new ewxChoice( itemFrame1, ID_TIMEFORMAT_CHOICE, wxDefaultPosition, wxDefaultSize, 0, p_timeFormatStrings, 0 );
    itemFlexGridSizer4->Add(p_timeFormat, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    ewxStaticText* itemStaticText13 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Units:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText13, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString* p_unitStrings = NULL;
    p_unit = new ewxChoice( itemFrame1, ID_UNIT_CHOICE, wxDefaultPosition, wxDefaultSize, 0, p_unitStrings, 0 );
    itemFlexGridSizer4->Add(p_unit, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    ewxStaticText* itemStaticText15 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Notifications:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText15, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    p_beepError = new ewxCheckBox( itemFrame1, ID_BEEPERROR_CHECKBOX, _("Beep on Errors"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    p_beepError->SetValue(true);
    itemFlexGridSizer4->Add(p_beepError, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    itemFlexGridSizer4->Add(1, 1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);

    p_beepWarn = new ewxCheckBox( itemFrame1, ID_BEEPWARN_CHECKBOX, _("Beep on Warnings"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    p_beepWarn->SetValue(true);
    itemFlexGridSizer4->Add(p_beepWarn, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    ewxStaticText* itemStaticText19 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Focus Behavior:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer4->Add(itemStaticText19, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    p_focus = new ewxCheckBox( itemFrame1, ID_FOCUS_FOLLOW_MOUSE, _("Follow Mouse"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    p_focus->SetValue(false);
    if (ShowToolTips())
        p_focus->SetToolTip(_("Auto select and focus on the content when mouse is over an input field"));
    itemFlexGridSizer4->Add(p_focus, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    wxStaticBox* itemStaticBoxSizer21Static = new wxStaticBox(itemFrame1, wxID_ANY, _("Gateway Preferences"));
    wxStaticBoxSizer* itemStaticBoxSizer21 = new wxStaticBoxSizer(itemStaticBoxSizer21Static, wxHORIZONTAL);
    itemBoxSizer2->Add(itemStaticBoxSizer21, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

    wxFlexGridSizer* itemFlexGridSizer22 = new wxFlexGridSizer(2, 2, 5, 5);
    itemStaticBoxSizer21->Add(itemFlexGridSizer22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_confirmExit = new ewxCheckBox( itemFrame1, ID_SHOWCONFIRM_CHECKBOX, _("Request Confirmation on Exit"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    p_confirmExit->SetValue(true);
    itemFlexGridSizer22->Add(p_confirmExit, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    p_showBusy = new ewxCheckBox( itemFrame1, ID_SHOWBUSY_CHECKBOX, _("Show System Busy Icon"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    p_showBusy->SetValue(true);
    itemFlexGridSizer22->Add(p_showBusy, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    p_alwaysOnTop = new ewxCheckBox( itemFrame1, ID_ALWAYS_ON_TOP, _("Always On Top"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    p_alwaysOnTop->SetValue(false);
    if (ShowToolTips())
        p_alwaysOnTop->SetToolTip(_("Note: Your window manager may override this setting"));
    itemFlexGridSizer22->Add(p_alwaysOnTop, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    p_leftClickNewApp = new ewxCheckBox( itemFrame1, ID_LEFTCLICK_NEWAPP, _("Left Click Invoke New Tool"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    p_leftClickNewApp->SetValue(false);
    itemFlexGridSizer22->Add(p_leftClickNewApp, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    p_closeShells = new ewxCheckBox( itemFrame1, ID_CLOSESHELLS_CHECKBOX, _("Close Remote Shells on Exit"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    p_closeShells->SetValue(false);
    itemFlexGridSizer22->Add(p_closeShells, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    p_savePasswords = new ewxCheckBox( itemFrame1, ID_SAVEPASSWORDS_CHECKBOX, _("Save Passwords"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    p_savePasswords->SetValue(false);
    if (ShowToolTips())
        p_savePasswords->SetToolTip(_("Remember data and compute server passwords between invocations"));
    p_savePasswords->Show(false);
    itemFlexGridSizer22->Add(p_savePasswords, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    wxString p_orientationStrings[] = {
        _("Horizontal Orientation"),
        _("Vertical Orientation")
    };
    p_orientation = new ewxChoice( itemFrame1, ID_ORIENTATION, wxDefaultPosition, wxDefaultSize, 2, p_orientationStrings, 0 );
    p_orientation->SetStringSelection(_("Horizontal Orientation"));
    itemFlexGridSizer22->Add(p_orientation, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    wxStaticBox* itemStaticBoxSizer30Static = new wxStaticBox(itemFrame1, wxID_ANY, _("Run State Icons"));
    wxStaticBoxSizer* itemStaticBoxSizer30 = new wxStaticBoxSizer(itemStaticBoxSizer30Static, wxHORIZONTAL);
    itemBoxSizer2->Add(itemStaticBoxSizer30, 1, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

    p_stateIconSizer = new wxGridSizer(6, 6, 5, 5);
    itemStaticBoxSizer30->Add(p_stateIconSizer, 1, wxGROW|wxALL, 5);

    ewxStaticText* itemStaticText32 = new ewxStaticText( itemFrame1, wxID_STATIC, _("State"), wxDefaultPosition, wxDefaultSize, 0 );
    p_stateIconSizer->Add(itemStaticText32, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText33 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Default"), wxDefaultPosition, wxDefaultSize, 0 );
    p_stateIconSizer->Add(itemStaticText33, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText34 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Custom"), wxDefaultPosition, wxDefaultSize, 0 );
    p_stateIconSizer->Add(itemStaticText34, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText35 = new ewxStaticText( itemFrame1, wxID_STATIC, _("State"), wxDefaultPosition, wxDefaultSize, 0 );
    p_stateIconSizer->Add(itemStaticText35, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText36 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Default"), wxDefaultPosition, wxDefaultSize, 0 );
    p_stateIconSizer->Add(itemStaticText36, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    ewxStaticText* itemStaticText37 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Custom"), wxDefaultPosition, wxDefaultSize, 0 );
    p_stateIconSizer->Add(itemStaticText37, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);

    wxBoxSizer* itemBoxSizer38 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer38, 0, wxGROW, 0);

    itemBoxSizer38->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton40 = new ewxButton( itemFrame1, ID_CLOSE_BUTTON, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer38->Add(itemButton40, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer38->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_resetAll = new ewxButton( itemFrame1, ID_RESETALL_BUTTON, _("Reset All"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer38->Add(p_resetAll, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer38->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    ewxButton* itemButton44 = new ewxButton( itemFrame1, ID_HELP_BUTTON, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer38->Add(itemButton44, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    itemBoxSizer38->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end GatewayPrefsGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_GATEWAYPREF_FRAME
 */

void GatewayPrefsGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_GATEWAYPREF_FRAME in GatewayPrefsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_GATEWAYPREF_FRAME in GatewayPrefsGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_THEME_CHOICE
 */

void GatewayPrefsGUI::OnThemeChoiceSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_THEME_CHOICE in GatewayPrefsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_THEME_CHOICE in GatewayPrefsGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_FONTSIZE
 */

void GatewayPrefsGUI::OnGlobalPrefChange( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_FONTSIZE in GatewayPrefsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_FONTSIZE in GatewayPrefsGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_SHOWCONFIRM_CHECKBOX
 */

void GatewayPrefsGUI::OnGatewayPrefChange( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_SHOWCONFIRM_CHECKBOX in GatewayPrefsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_SHOWCONFIRM_CHECKBOX in GatewayPrefsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CLOSE_BUTTON
 */

void GatewayPrefsGUI::OnCloseButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CLOSE_BUTTON in GatewayPrefsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CLOSE_BUTTON in GatewayPrefsGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RESETALL_BUTTON
 */

void GatewayPrefsGUI::OnResetAllButtonClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RESETALL_BUTTON in GatewayPrefsGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_RESETALL_BUTTON in GatewayPrefsGUI. 
}

/*!
 * Should we show tooltips?
 */

bool GatewayPrefsGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap GatewayPrefsGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin GatewayPrefsGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end GatewayPrefsGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon GatewayPrefsGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin GatewayPrefsGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end GatewayPrefsGUI icon retrieval
}

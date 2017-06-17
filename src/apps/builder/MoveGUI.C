/////////////////////////////////////////////////////////////////////////////
// Name:        MoveGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "MoveGUI.H"
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
#include "wxgui/ewxButton.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "MoveGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID MoveGUI::ID_TEXTCTRL_MOVE_TRANSL_Z = wxNewId();
const wxWindowID MoveGUI::ID_TEXTCTRL_MOVE_ALGN_X = wxNewId();
const wxWindowID MoveGUI::ID_TEXTCTRL_MOVE_ALGN_ATOM = wxNewId();
const wxWindowID MoveGUI::ID_DIALOG = wxNewId();
const wxWindowID MoveGUI::ID_TEXTCTRL_MOVE_ALGN_NX = wxNewId();
const wxWindowID MoveGUI::ID_BUTTON_MOVE_ACTION = wxNewId();
const wxWindowID MoveGUI::ID_TEXTCTRL_MOVE_ALGN_NY = wxNewId();
const wxWindowID MoveGUI::ID_PANEL_MOVE_ALGN_LN = wxNewId();
const wxWindowID MoveGUI::ID_TEXTCTRL_MOVE_ALGN_Z = wxNewId();
const wxWindowID MoveGUI::ID_TEXTCTRL_MOVE_ROT_ANGL = wxNewId();
const wxWindowID MoveGUI::ID_BUTTON_MOVE_CNTR_X = wxNewId();
const wxWindowID MoveGUI::ID_TEXTCTRL_MOVE_ALGN_ATOM1 = wxNewId();
const wxWindowID MoveGUI::ID_PANEL_MOVE_ALGN_PLN = wxNewId();
const wxWindowID MoveGUI::ID_CHOICE_MOVE_ALGN_LN = wxNewId();
const wxWindowID MoveGUI::ID_TEXTCTRL_MOVE_ALGN_Y = wxNewId();
const wxWindowID MoveGUI::ID_BUTTON_MOVE_CNTR_XYZ = wxNewId();
const wxWindowID MoveGUI::ID_TEXTCTRL_MOVE_TRANSL_X = wxNewId();
const wxWindowID MoveGUI::ID_BUTTON_MOVE_CNTR_Z = wxNewId();
const wxWindowID MoveGUI::ID_CHOICE_MOVE_ALGN_OPT = wxNewId();
const wxWindowID MoveGUI::ID_BUTTON_MOVE_CNTR_Y = wxNewId();
const wxWindowID MoveGUI::ID_TEXTCTRL_MOVE_TRANSL_Y = wxNewId();
const wxWindowID MoveGUI::ID_PANEL_MOVE_TRANSLATE = wxNewId();
const wxWindowID MoveGUI::ID_TEXTCTRL_MOVE_ALGN_ATOM2 = wxNewId();
const wxWindowID MoveGUI::ID_CHOICE_MOVE_ROT_AXS = wxNewId();
const wxWindowID MoveGUI::ID_PANEL_MOVE_ROTATE = wxNewId();
const wxWindowID MoveGUI::ID_TEXTCTRL_MOVE_ALGN_NZ = wxNewId();
const wxWindowID MoveGUI::ID_PANEL_MOVE_CENTER = wxNewId();
const wxWindowID MoveGUI::ID_CHOICE_MOVE_OPTS = wxNewId();

/*!
 * MoveGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( MoveGUI, ewxPanel )

/*!
 * MoveGUI event table definition
 */

BEGIN_EVENT_TABLE( MoveGUI, ewxPanel )

////@begin MoveGUI event table entries
    EVT_CHOICE( ID_CHOICE_MOVE_OPTS, MoveGUI::OnChoiceMoveOptsSelected )

    EVT_BUTTON( ID_BUTTON_MOVE_ACTION, MoveGUI::OnButtonMoveActionClick )

    EVT_BUTTON( ID_BUTTON_MOVE_CNTR_XYZ, MoveGUI::OnButtonMoveCntrXyzClick )

    EVT_BUTTON( ID_BUTTON_MOVE_CNTR_X, MoveGUI::OnButtonMoveCntrXClick )

    EVT_BUTTON( ID_BUTTON_MOVE_CNTR_Y, MoveGUI::OnButtonMoveCntrYClick )

    EVT_BUTTON( ID_BUTTON_MOVE_CNTR_Z, MoveGUI::OnButtonMoveCntrZClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MOVE_TRANSL_X, MoveGUI::OnTextctrlMoveTranslXEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MOVE_TRANSL_Y, MoveGUI::OnTextctrlMoveTranslYEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MOVE_TRANSL_Z, MoveGUI::OnTextctrlMoveTranslZEnter )

    EVT_CHOICE( ID_CHOICE_MOVE_ROT_AXS, MoveGUI::OnChoiceMoveRotAxsSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MOVE_ROT_ANGL, MoveGUI::OnTextctrlMoveRotAnglEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MOVE_ALGN_ATOM, MoveGUI::OnTextctrlMoveAlgnAtomEnter )

    EVT_CHOICE( ID_CHOICE_MOVE_ALGN_LN, MoveGUI::OnChoiceMoveAlgnLnSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MOVE_ALGN_X, MoveGUI::OnTextctrlMoveAlgnXEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MOVE_ALGN_Y, MoveGUI::OnTextctrlMoveAlgnYEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MOVE_ALGN_Z, MoveGUI::OnTextctrlMoveAlgnZEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MOVE_ALGN_ATOM1, MoveGUI::OnTextctrlMoveAlgnAtom1Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MOVE_ALGN_ATOM2, MoveGUI::OnTextctrlMoveAlgnAtom2Enter )

    EVT_CHOICE( ID_CHOICE_MOVE_ALGN_OPT, MoveGUI::OnChoiceMoveAlgnOptSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MOVE_ALGN_NX, MoveGUI::OnTextctrlMoveAlgnNxEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MOVE_ALGN_NY, MoveGUI::OnTextctrlMoveAlgnNyEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_MOVE_ALGN_NZ, MoveGUI::OnTextctrlMoveAlgnNzEnter )

////@end MoveGUI event table entries

END_EVENT_TABLE()

/*!
 * MoveGUI constructors
 */

MoveGUI::MoveGUI( )
{
}

MoveGUI::MoveGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * MoveGUI creator
 */

bool MoveGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin MoveGUI member initialisation
    p_gridSizer = NULL;
////@end MoveGUI member initialisation

////@begin MoveGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end MoveGUI creation
    return true;
}

/*!
 * Control creation for MoveGUI
 */

void MoveGUI::CreateControls()
{    
////@begin MoveGUI content construction
    MoveGUI* itemPanel1 = this;

    p_gridSizer = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(p_gridSizer);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    p_gridSizer->Add(itemBoxSizer3, 1, wxGROW|wxALL, 0);

    wxString itemChoice4Strings[] = {
        _("Center"),
        _("Translate"),
        _("Rotate"),
        _("Align to Line"),
        _("Align to Plane")
    };
    ewxChoice* itemChoice4 = new ewxChoice( itemPanel1, ID_CHOICE_MOVE_OPTS, wxDefaultPosition, wxDefaultSize, 5, itemChoice4Strings, 0 );
    itemChoice4->SetStringSelection(_("Center"));
    itemBoxSizer3->Add(itemChoice4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer3->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton6 = new ewxButton( itemPanel1, ID_BUTTON_MOVE_ACTION, _("Go"), wxDefaultPosition, wxSize(30, -1), 0 );
    itemButton6->SetDefault();
    itemBoxSizer3->Add(itemButton6, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel7 = new ewxPanel( itemPanel1, ID_PANEL_MOVE_CENTER, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel7->Show(false);
    p_gridSizer->Add(itemPanel7, 1, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxVERTICAL);
    itemPanel7->SetSizer(itemBoxSizer8);

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer8->Add(itemBoxSizer9, 1, wxGROW|wxALL, 0);

    ewxButton* itemButton10 = new ewxButton( itemPanel7, ID_BUTTON_MOVE_CNTR_XYZ, _("XYZ"), wxDefaultPosition, wxSize(40, -1), 0 );
    itemButton10->SetDefault();
    itemBoxSizer9->Add(itemButton10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer9->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton12 = new ewxButton( itemPanel7, ID_BUTTON_MOVE_CNTR_X, _("X"), wxDefaultPosition, wxSize(40, -1), 0 );
    itemButton12->SetDefault();
    itemBoxSizer9->Add(itemButton12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer9->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton14 = new ewxButton( itemPanel7, ID_BUTTON_MOVE_CNTR_Y, _("Y"), wxDefaultPosition, wxSize(40, -1), 0 );
    itemBoxSizer9->Add(itemButton14, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer9->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton16 = new ewxButton( itemPanel7, ID_BUTTON_MOVE_CNTR_Z, _("Z"), wxDefaultPosition, wxSize(40, -1), 0 );
    itemBoxSizer9->Add(itemButton16, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel17 = new ewxPanel( itemPanel1, ID_PANEL_MOVE_TRANSLATE, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel17->Show(false);
    p_gridSizer->Add(itemPanel17, 0, wxALIGN_LEFT|wxALL, 3);

    wxBoxSizer* itemBoxSizer18 = new wxBoxSizer(wxVERTICAL);
    itemPanel17->SetSizer(itemBoxSizer18);

    wxBoxSizer* itemBoxSizer19 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer18->Add(itemBoxSizer19, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText20 = new ewxStaticText( itemPanel17, wxID_STATIC, _("Translation Vector  X:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(itemStaticText20, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl21 = new ewxTextCtrl( itemPanel17, ID_TEXTCTRL_MOVE_TRANSL_X, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer19->Add(itemTextCtrl21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText22 = new ewxStaticText( itemPanel17, wxID_STATIC, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(itemStaticText22, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl23 = new ewxTextCtrl( itemPanel17, ID_TEXTCTRL_MOVE_TRANSL_Y, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer19->Add(itemTextCtrl23, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText24 = new ewxStaticText( itemPanel17, wxID_STATIC, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer19->Add(itemStaticText24, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl25 = new ewxTextCtrl( itemPanel17, ID_TEXTCTRL_MOVE_TRANSL_Z, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer19->Add(itemTextCtrl25, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel26 = new ewxPanel( itemPanel1, ID_PANEL_MOVE_ROTATE, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel26->Show(false);
    p_gridSizer->Add(itemPanel26, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer27 = new wxBoxSizer(wxVERTICAL);
    itemPanel26->SetSizer(itemBoxSizer27);

    wxBoxSizer* itemBoxSizer28 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer27->Add(itemBoxSizer28, 1, wxGROW|wxALL, 0);

    wxString itemChoice29Strings[] = {
        _("X-axis"),
        _("Y-axis"),
        _("Z-axis")
    };
    ewxChoice* itemChoice29 = new ewxChoice( itemPanel26, ID_CHOICE_MOVE_ROT_AXS, wxDefaultPosition, wxDefaultSize, 3, itemChoice29Strings, 0 );
    itemChoice29->SetStringSelection(_("X-axis"));
    itemBoxSizer28->Add(itemChoice29, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer28->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText31 = new ewxStaticText( itemPanel26, wxID_STATIC, _("Rotation Angle:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer28->Add(itemStaticText31, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl32 = new ewxTextCtrl( itemPanel26, ID_TEXTCTRL_MOVE_ROT_ANGL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer28->Add(itemTextCtrl32, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 3);

    ewxPanel* itemPanel33 = new ewxPanel( itemPanel1, ID_PANEL_MOVE_ALGN_LN, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel33->Show(false);
    p_gridSizer->Add(itemPanel33, 0, wxALIGN_LEFT|wxALL, 5);

    wxBoxSizer* itemBoxSizer34 = new wxBoxSizer(wxVERTICAL);
    itemPanel33->SetSizer(itemBoxSizer34);

    wxBoxSizer* itemBoxSizer35 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer34->Add(itemBoxSizer35, 0, wxALIGN_LEFT|wxRIGHT|wxTOP|wxBOTTOM, 0);

    ewxStaticText* itemStaticText36 = new ewxStaticText( itemPanel33, wxID_STATIC, _("Align selected atom to line from origin"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer35->Add(itemStaticText36, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer37 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer34->Add(itemBoxSizer37, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText38 = new ewxStaticText( itemPanel33, wxID_STATIC, _("Atom:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer37->Add(itemStaticText38, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl39 = new ewxTextCtrl( itemPanel33, ID_TEXTCTRL_MOVE_ALGN_ATOM, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer37->Add(itemTextCtrl39, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer40 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer34->Add(itemBoxSizer40, 0, wxALIGN_LEFT|wxALL, 0);

    wxString itemChoice41Strings[] = {
        _("X-axis"),
        _("Y-axis"),
        _("Z-axis"),
        _("Other")
    };
    ewxChoice* itemChoice41 = new ewxChoice( itemPanel33, ID_CHOICE_MOVE_ALGN_LN, wxDefaultPosition, wxDefaultSize, 4, itemChoice41Strings, 0 );
    itemChoice41->SetStringSelection(_("X-axis"));
    itemBoxSizer40->Add(itemChoice41, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText42 = new ewxStaticText( itemPanel33, wxID_STATIC, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemStaticText42, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl43 = new ewxTextCtrl( itemPanel33, ID_TEXTCTRL_MOVE_ALGN_X, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer40->Add(itemTextCtrl43, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText44 = new ewxStaticText( itemPanel33, wxID_STATIC, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemStaticText44, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl45 = new ewxTextCtrl( itemPanel33, ID_TEXTCTRL_MOVE_ALGN_Y, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer40->Add(itemTextCtrl45, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText46 = new ewxStaticText( itemPanel33, wxID_STATIC, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemStaticText46, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl47 = new ewxTextCtrl( itemPanel33, ID_TEXTCTRL_MOVE_ALGN_Z, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer40->Add(itemTextCtrl47, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel48 = new ewxPanel( itemPanel1, ID_PANEL_MOVE_ALGN_PLN, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    p_gridSizer->Add(itemPanel48, 0, wxALIGN_LEFT|wxALL, 0);

    wxBoxSizer* itemBoxSizer49 = new wxBoxSizer(wxVERTICAL);
    itemPanel48->SetSizer(itemBoxSizer49);

    wxBoxSizer* itemBoxSizer50 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer49->Add(itemBoxSizer50, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText51 = new ewxStaticText( itemPanel48, wxID_STATIC, _("Align selected atoms to plane through origin"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer50->Add(itemStaticText51, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer52 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer49->Add(itemBoxSizer52, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText53 = new ewxStaticText( itemPanel48, wxID_STATIC, _("Atom 1:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer52->Add(itemStaticText53, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl54 = new ewxTextCtrl( itemPanel48, ID_TEXTCTRL_MOVE_ALGN_ATOM1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer52->Add(itemTextCtrl54, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText55 = new ewxStaticText( itemPanel48, wxID_STATIC, _("Atom 2:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer52->Add(itemStaticText55, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl56 = new ewxTextCtrl( itemPanel48, ID_TEXTCTRL_MOVE_ALGN_ATOM2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer52->Add(itemTextCtrl56, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer57 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer49->Add(itemBoxSizer57, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText58 = new ewxStaticText( itemPanel48, wxID_STATIC, _("Normal:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer57->Add(itemStaticText58, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice59Strings[] = {
        _("XY-plane"),
        _("XZ-plane"),
        _("YZ-plane"),
        _("Other")
    };
    ewxChoice* itemChoice59 = new ewxChoice( itemPanel48, ID_CHOICE_MOVE_ALGN_OPT, wxDefaultPosition, wxDefaultSize, 4, itemChoice59Strings, 0 );
    itemChoice59->SetStringSelection(_("XY-plane"));
    itemBoxSizer57->Add(itemChoice59, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText60 = new ewxStaticText( itemPanel48, wxID_STATIC, _("X:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer57->Add(itemStaticText60, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl61 = new ewxTextCtrl( itemPanel48, ID_TEXTCTRL_MOVE_ALGN_NX, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer57->Add(itemTextCtrl61, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText62 = new ewxStaticText( itemPanel48, wxID_STATIC, _("Y:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer57->Add(itemStaticText62, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl63 = new ewxTextCtrl( itemPanel48, ID_TEXTCTRL_MOVE_ALGN_NY, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer57->Add(itemTextCtrl63, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText64 = new ewxStaticText( itemPanel48, wxID_STATIC, _("Z:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer57->Add(itemStaticText64, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl65 = new ewxTextCtrl( itemPanel48, ID_TEXTCTRL_MOVE_ALGN_NZ, _T(""), wxDefaultPosition, wxSize(30, -1), 0 );
    itemBoxSizer57->Add(itemTextCtrl65, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

////@end MoveGUI content construction
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_OPTS
 */

void MoveGUI::OnChoiceMoveOptsSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_OPTS in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_OPTS in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_ACTION
 */

void MoveGUI::OnButtonMoveActionClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_ACTION in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_ACTION in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_XYZ
 */

void MoveGUI::OnButtonMoveCntrXyzClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_XYZ in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_XYZ in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_X
 */

void MoveGUI::OnButtonMoveCntrXClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_X in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_X in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_Y
 */

void MoveGUI::OnButtonMoveCntrYClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_Y in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_Y in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_Z
 */

void MoveGUI::OnButtonMoveCntrZClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_Z in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MOVE_CNTR_Z in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_TRANSL_X
 */

void MoveGUI::OnTextctrlMoveTranslXEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_TRANSL_X in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_TRANSL_X in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_TRANSL_Y
 */

void MoveGUI::OnTextctrlMoveTranslYEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_TRANSL_Y in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_TRANSL_Y in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_TRANSL_Z
 */

void MoveGUI::OnTextctrlMoveTranslZEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_TRANSL_Z in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_TRANSL_Z in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_ROT_AXS
 */

void MoveGUI::OnChoiceMoveRotAxsSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_ROT_AXS in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_ROT_AXS in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ROT_ANGL
 */

void MoveGUI::OnTextctrlMoveRotAnglEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ROT_ANGL in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ROT_ANGL in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_ATOM
 */

void MoveGUI::OnTextctrlMoveAlgnAtomEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_ATOM in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_ATOM in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_ALGN_LN
 */

void MoveGUI::OnChoiceMoveAlgnLnSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_ALGN_LN in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_ALGN_LN in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_X
 */

void MoveGUI::OnTextctrlMoveAlgnXEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_X in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_X in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_Y
 */

void MoveGUI::OnTextctrlMoveAlgnYEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_Y in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_Y in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_Z
 */

void MoveGUI::OnTextctrlMoveAlgnZEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_Z in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_Z in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_ATOM1
 */

void MoveGUI::OnTextctrlMoveAlgnAtom1Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_ATOM1 in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_ATOM1 in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_ATOM2
 */

void MoveGUI::OnTextctrlMoveAlgnAtom2Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_ATOM2 in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_ATOM2 in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_ALGN_OPT
 */

void MoveGUI::OnChoiceMoveAlgnOptSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_ALGN_OPT in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_MOVE_ALGN_OPT in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_NX
 */

void MoveGUI::OnTextctrlMoveAlgnNxEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_NX in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_NX in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_NY
 */

void MoveGUI::OnTextctrlMoveAlgnNyEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_NY in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_NY in MoveGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_NZ
 */

void MoveGUI::OnTextctrlMoveAlgnNzEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_NZ in MoveGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MOVE_ALGN_NZ in MoveGUI. 
}

/*!
 * Should we show tooltips?
 */

bool MoveGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap MoveGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin MoveGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end MoveGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon MoveGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin MoveGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end MoveGUI icon retrieval
}

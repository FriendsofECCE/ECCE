/////////////////////////////////////////////////////////////////////////////
// Name:        PBCGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "PBCGUI.H"
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
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "PBCGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_A3Y = wxNewId();
const wxWindowID PBCGUI::ID_SPINCTRL_PBC_ZREP = wxNewId();
const wxWindowID PBCGUI::ID_CHOICE_PBC_TYPE = wxNewId();
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_A1X = wxNewId();
const wxWindowID PBCGUI::ID_CHECKBOX_PBC_SHOWLATTICE = wxNewId();
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_A2Z = wxNewId();
const wxWindowID PBCGUI::ID_CHECKBOX_PBC_LOCK = wxNewId();
const wxWindowID PBCGUI::ID_BUTTON_PBC_EQUIVRECT = wxNewId();
const wxWindowID PBCGUI::ID_CHECKBOX_PBC_CENTER = wxNewId();
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_B = wxNewId();
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_A2X = wxNewId();
const wxWindowID PBCGUI::ID_PANEL = wxNewId();
const wxWindowID PBCGUI::ID_SPINCTRL_PBC_YREP = wxNewId();
const wxWindowID PBCGUI::ID_CHECKBOX_PBC_UNIFORM = wxNewId();
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_BETA = wxNewId();
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_A2Y = wxNewId();
const wxWindowID PBCGUI::ID_BUTTON_PBC_RESTORE = wxNewId();
const wxWindowID PBCGUI::ID_BUTTON_PBC_REPLICATE = wxNewId();
const wxWindowID PBCGUI::ID_PANEL_PBC = wxNewId();
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_A3X = wxNewId();
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_A1Z = wxNewId();
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_ALPHA = wxNewId();
const wxWindowID PBCGUI::ID_BUTTON_PBC_FOLD = wxNewId();
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_A3Z = wxNewId();
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_C = wxNewId();
const wxWindowID PBCGUI::ID_BUTTON_PBC_SUPER = wxNewId();
const wxWindowID PBCGUI::ID_PBC_GENERATE = wxNewId();
const wxWindowID PBCGUI::ID_BUTTON_PBC_DELETE = wxNewId();
const wxWindowID PBCGUI::ID_SPINCTRL_PBC_XREP = wxNewId();
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_A = wxNewId();
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_A1Y = wxNewId();
const wxWindowID PBCGUI::ID_TEXTCTRL_PBC_GAMMA = wxNewId();
const wxWindowID PBCGUI::ID_BUTTON_PBC_CREATE = wxNewId();

/*!
 * PBCGUI type definition
 */

IMPLEMENT_DYNAMIC_CLASS( PBCGUI, ewxPanel )

/*!
 * PBCGUI event table definition
 */

BEGIN_EVENT_TABLE( PBCGUI, ewxPanel )

////@begin PBCGUI event table entries
    EVT_BUTTON( ID_BUTTON_PBC_CREATE, PBCGUI::OnButtonPbcCreateClick )

    EVT_CHOICE( ID_CHOICE_PBC_TYPE, PBCGUI::OnChoicePbcTypeSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_A1X, PBCGUI::OnTextctrlPbcA1xEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_A1Y, PBCGUI::OnTextctrlPbcA1yEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_A1Z, PBCGUI::OnTextctrlPbcA1zEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_A2X, PBCGUI::OnTextctrlPbcA2xEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_A2Y, PBCGUI::OnTextctrlPbcA2yEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_A2Z, PBCGUI::OnTextctrlPbcA2zEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_A3X, PBCGUI::OnTextctrlPbcA3xEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_A3Y, PBCGUI::OnTextctrlPbcA3yEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_A3Z, PBCGUI::OnTextctrlPbcA3zEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_A, PBCGUI::OnTextctrlPbcAEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_ALPHA, PBCGUI::OnTextctrlPbcAlphaEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_B, PBCGUI::OnTextctrlPbcBEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_BETA, PBCGUI::OnTextctrlPbcBetaEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_C, PBCGUI::OnTextctrlPbcCEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PBC_GAMMA, PBCGUI::OnTextctrlPbcGammaEnter )

    EVT_CHECKBOX( ID_CHECKBOX_PBC_CENTER, PBCGUI::OnCheckboxPbcCenterClick )

    EVT_CHECKBOX( ID_CHECKBOX_PBC_SHOWLATTICE, PBCGUI::OnCheckboxPbcShowlatticeClick )

    EVT_BUTTON( ID_BUTTON_PBC_FOLD, PBCGUI::OnButtonPbcFoldClick )

    EVT_BUTTON( ID_BUTTON_PBC_DELETE, PBCGUI::OnButtonPbcDeleteClick )

    EVT_BUTTON( ID_BUTTON_PBC_EQUIVRECT, PBCGUI::OnButtonPbcEquivrectClick )

    EVT_BUTTON( ID_PBC_GENERATE, PBCGUI::OnPbcGenerateClick )

    EVT_SPINCTRL( ID_SPINCTRL_PBC_XREP, PBCGUI::OnSpinctrlPbcXrepUpdated )
    EVT_TEXT( ID_SPINCTRL_PBC_XREP, PBCGUI::OnSpinctrlPbcXrepTextUpdated )

    EVT_SPINCTRL( ID_SPINCTRL_PBC_YREP, PBCGUI::OnSpinctrlPbcYrepUpdated )
    EVT_TEXT( ID_SPINCTRL_PBC_YREP, PBCGUI::OnSpinctrlPbcYrepTextUpdated )

    EVT_SPINCTRL( ID_SPINCTRL_PBC_ZREP, PBCGUI::OnSpinctrlPbcZrepUpdated )
    EVT_TEXT( ID_SPINCTRL_PBC_ZREP, PBCGUI::OnSpinctrlPbcZrepTextUpdated )

    EVT_CHECKBOX( ID_CHECKBOX_PBC_UNIFORM, PBCGUI::OnCheckboxPbcUniformClick )

    EVT_BUTTON( ID_BUTTON_PBC_REPLICATE, PBCGUI::OnButtonPbcReplicateClick )

    EVT_BUTTON( ID_BUTTON_PBC_RESTORE, PBCGUI::OnButtonPbcRestoreClick )

    EVT_BUTTON( ID_BUTTON_PBC_SUPER, PBCGUI::OnButtonPbcSuperClick )

////@end PBCGUI event table entries

END_EVENT_TABLE()

/*!
 * PBCGUI constructors
 */

PBCGUI::PBCGUI( )
{
}

PBCGUI::PBCGUI( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
    Create(parent, id, pos, size, style);
}

/*!
 * PBCGUI creator
 */

bool PBCGUI::Create( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style )
{
////@begin PBCGUI member initialisation
    p_createSizer = NULL;
    p_latticePanel = NULL;
    p_operationsSizer = NULL;
    p_symSizer = NULL;
////@end PBCGUI member initialisation

////@begin PBCGUI creation
    ewxPanel::Create( parent, id, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end PBCGUI creation
    return true;
}

/*!
 * Control creation for PBCGUI
 */

void PBCGUI::CreateControls()
{    
////@begin PBCGUI content construction
    PBCGUI* itemPanel1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemPanel1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_LEFT|wxALL, 1);

    p_createSizer = new ewxButton( itemPanel1, ID_BUTTON_PBC_CREATE, _("Create Lattice"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        p_createSizer->SetToolTip(_("Create a unit lattice"));
    itemBoxSizer3->Add(p_createSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer5, 0, wxGROW|wxALL, 0);

    p_latticePanel = new ewxPanel( itemPanel1, ID_PANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer5->Add(p_latticePanel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxVERTICAL);
    p_latticePanel->SetSizer(itemBoxSizer7);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer7->Add(itemBoxSizer8, 0, wxGROW|wxALL, 0);

    ewxStaticText* itemStaticText9 = new ewxStaticText( p_latticePanel, wxID_STATIC, _("Cell Type:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemStaticText9, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    wxString itemChoice10Strings[] = {
        _("Lattice"),
        _("Lattice Vectors")
    };
    ewxChoice* itemChoice10 = new ewxChoice( p_latticePanel, ID_CHOICE_PBC_TYPE, wxDefaultPosition, wxDefaultSize, 2, itemChoice10Strings, 0 );
    itemChoice10->SetStringSelection(_("Lattice Vectors"));
    itemBoxSizer8->Add(itemChoice10, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer11 = new wxFlexGridSizer(3, 4, 0, 0);
    itemFlexGridSizer11->AddGrowableCol(1);
    itemBoxSizer7->Add(itemFlexGridSizer11, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 1);

    ewxStaticText* itemStaticText12 = new ewxStaticText( p_latticePanel, wxID_STATIC, _("a1:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemStaticText12, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl13 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_A1X, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemTextCtrl13, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxTextCtrl* itemTextCtrl14 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_A1Y, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemTextCtrl14, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxTextCtrl* itemTextCtrl15 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_A1Z, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemTextCtrl15, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxStaticText* itemStaticText16 = new ewxStaticText( p_latticePanel, wxID_STATIC, _("a2:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemStaticText16, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    ewxTextCtrl* itemTextCtrl17 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_A2X, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemTextCtrl17, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxTextCtrl* itemTextCtrl18 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_A2Y, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemTextCtrl18, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxTextCtrl* itemTextCtrl19 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_A2Z, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemTextCtrl19, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxStaticText* itemStaticText20 = new ewxStaticText( p_latticePanel, wxID_STATIC, _("a3:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemStaticText20, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    ewxTextCtrl* itemTextCtrl21 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_A3X, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemTextCtrl21, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxTextCtrl* itemTextCtrl22 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_A3Y, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemTextCtrl22, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxTextCtrl* itemTextCtrl23 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_A3Z, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer11->Add(itemTextCtrl23, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    wxFlexGridSizer* itemFlexGridSizer24 = new wxFlexGridSizer(2, 4, 0, 0);
    itemFlexGridSizer24->AddGrowableCol(1);
    itemFlexGridSizer24->AddGrowableCol(3);
    itemBoxSizer7->Add(itemFlexGridSizer24, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 2);

    ewxStaticText* itemStaticText25 = new ewxStaticText( p_latticePanel, wxID_STATIC, _("a:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer24->Add(itemStaticText25, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    ewxTextCtrl* itemTextCtrl26 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_A, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer24->Add(itemTextCtrl26, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxStaticText* itemStaticText27 = new ewxStaticText( p_latticePanel, wxID_STATIC, _("alpha:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer24->Add(itemStaticText27, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    ewxTextCtrl* itemTextCtrl28 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_ALPHA, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer24->Add(itemTextCtrl28, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxStaticText* itemStaticText29 = new ewxStaticText( p_latticePanel, wxID_STATIC, _("b:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer24->Add(itemStaticText29, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    ewxTextCtrl* itemTextCtrl30 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_B, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer24->Add(itemTextCtrl30, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxStaticText* itemStaticText31 = new ewxStaticText( p_latticePanel, wxID_STATIC, _("beta:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer24->Add(itemStaticText31, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    ewxTextCtrl* itemTextCtrl32 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_BETA, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer24->Add(itemTextCtrl32, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxStaticText* itemStaticText33 = new ewxStaticText( p_latticePanel, wxID_STATIC, _("c:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer24->Add(itemStaticText33, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    ewxTextCtrl* itemTextCtrl34 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_C, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer24->Add(itemTextCtrl34, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxStaticText* itemStaticText35 = new ewxStaticText( p_latticePanel, wxID_STATIC, _("gamma:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer24->Add(itemStaticText35, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    ewxTextCtrl* itemTextCtrl36 = new ewxTextCtrl( p_latticePanel, ID_TEXTCTRL_PBC_GAMMA, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer24->Add(itemTextCtrl36, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 2);

    wxBoxSizer* itemBoxSizer37 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer7->Add(itemBoxSizer37, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox38 = new ewxCheckBox( p_latticePanel, ID_CHECKBOX_PBC_LOCK, _("Lock"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox38->SetValue(true);
    if (ShowToolTips())
        itemCheckBox38->SetToolTip(_("Lock coordinates when editing lattice"));
    itemBoxSizer37->Add(itemCheckBox38, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemBoxSizer37->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxCheckBox* itemCheckBox40 = new ewxCheckBox( p_latticePanel, ID_CHECKBOX_PBC_CENTER, _("Center Lattice"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox40->SetValue(true);
    itemBoxSizer37->Add(itemCheckBox40, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemBoxSizer37->Add(10, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxCheckBox* itemCheckBox42 = new ewxCheckBox( p_latticePanel, ID_CHECKBOX_PBC_SHOWLATTICE, _("Show Lattice"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox42->SetValue(true);
    itemBoxSizer37->Add(itemCheckBox42, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    p_operationsSizer = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer7->Add(p_operationsSizer, 0, wxALIGN_LEFT|wxALL, 1);

    ewxButton* itemButton44 = new ewxButton( p_latticePanel, ID_BUTTON_PBC_FOLD, _("Fold"), wxDefaultPosition, wxDefaultSize, 0 );
    p_operationsSizer->Add(itemButton44, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    p_operationsSizer->Add(3, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton46 = new ewxButton( p_latticePanel, ID_BUTTON_PBC_DELETE, _("Delete Lattice"), wxDefaultPosition, wxDefaultSize, 0 );
    p_operationsSizer->Add(itemButton46, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    p_operationsSizer->Add(3, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton48 = new ewxButton( p_latticePanel, ID_BUTTON_PBC_EQUIVRECT, _("Equiv. Rectangle"), wxDefaultPosition, wxDefaultSize, 0 );
    p_operationsSizer->Add(itemButton48, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    p_symSizer = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer7->Add(p_symSizer, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText50 = new ewxStaticText( p_latticePanel, wxID_STATIC, _("Sym. Group"), wxDefaultPosition, wxDefaultSize, 0 );
    p_symSizer->Add(itemStaticText50, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    p_symSizer->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton52 = new ewxButton( p_latticePanel, ID_PBC_GENERATE, _("Generate"), wxDefaultPosition, wxDefaultSize, 0 );
    p_symSizer->Add(itemButton52, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    wxStaticBox* itemStaticBoxSizer53Static = new wxStaticBox(p_latticePanel, wxID_ANY, _("Replication"));
    wxStaticBoxSizer* itemStaticBoxSizer53 = new wxStaticBoxSizer(itemStaticBoxSizer53Static, wxVERTICAL);
    itemBoxSizer7->Add(itemStaticBoxSizer53, 0, wxGROW|wxALL, 0);

    wxBoxSizer* itemBoxSizer54 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer53->Add(itemBoxSizer54, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    ewxStaticText* itemStaticText55 = new ewxStaticText( p_latticePanel, wxID_STATIC, _("a:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer54->Add(itemStaticText55, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    ewxSpinCtrl* itemSpinCtrl56 = new ewxSpinCtrl( p_latticePanel, ID_SPINCTRL_PBC_XREP, _("1"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 1, 100, 1 );
    itemBoxSizer54->Add(itemSpinCtrl56, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxStaticText* itemStaticText57 = new ewxStaticText( p_latticePanel, wxID_STATIC, _("b:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer54->Add(itemStaticText57, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    ewxSpinCtrl* itemSpinCtrl58 = new ewxSpinCtrl( p_latticePanel, ID_SPINCTRL_PBC_YREP, _("1"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 1, 100, 1 );
    itemBoxSizer54->Add(itemSpinCtrl58, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxStaticText* itemStaticText59 = new ewxStaticText( p_latticePanel, wxID_STATIC, _("c:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer54->Add(itemStaticText59, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

    ewxSpinCtrl* itemSpinCtrl60 = new ewxSpinCtrl( p_latticePanel, ID_SPINCTRL_PBC_ZREP, _("1"), wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 1, 100, 1 );
    itemBoxSizer54->Add(itemSpinCtrl60, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2);

    ewxCheckBox* itemCheckBox61 = new ewxCheckBox( p_latticePanel, ID_CHECKBOX_PBC_UNIFORM, _("Uniform"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox61->SetValue(true);
    itemBoxSizer54->Add(itemCheckBox61, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxBoxSizer* itemBoxSizer62 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer53->Add(itemBoxSizer62, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    ewxButton* itemButton63 = new ewxButton( p_latticePanel, ID_BUTTON_PBC_REPLICATE, _("Replicate"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer62->Add(itemButton63, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemBoxSizer62->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton65 = new ewxButton( p_latticePanel, ID_BUTTON_PBC_RESTORE, _("Unit Cell"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemButton65->SetToolTip(_("Restore unit cell"));
    itemBoxSizer62->Add(itemButton65, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    itemBoxSizer62->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxButton* itemButton67 = new ewxButton( p_latticePanel, ID_BUTTON_PBC_SUPER, _("Super Lattice"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer62->Add(itemButton67, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

////@end PBCGUI content construction
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_CREATE
 */

void PBCGUI::OnButtonPbcCreateClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_CREATE in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_CREATE in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PBC_TYPE
 */

void PBCGUI::OnChoicePbcTypeSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PBC_TYPE in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PBC_TYPE in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A1X
 */

void PBCGUI::OnTextctrlPbcA1xEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A1X in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A1X in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A1Y
 */

void PBCGUI::OnTextctrlPbcA1yEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A1Y in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A1Y in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A1Z
 */

void PBCGUI::OnTextctrlPbcA1zEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A1Z in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A1Z in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A2X
 */

void PBCGUI::OnTextctrlPbcA2xEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A2X in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A2X in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A2Y
 */

void PBCGUI::OnTextctrlPbcA2yEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A2Y in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A2Y in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A2Z
 */

void PBCGUI::OnTextctrlPbcA2zEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A2Z in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A2Z in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A3X
 */

void PBCGUI::OnTextctrlPbcA3xEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A3X in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A3X in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A3Y
 */

void PBCGUI::OnTextctrlPbcA3yEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A3Y in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A3Y in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A3Z
 */

void PBCGUI::OnTextctrlPbcA3zEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A3Z in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A3Z in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A
 */

void PBCGUI::OnTextctrlPbcAEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_A in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_ALPHA
 */

void PBCGUI::OnTextctrlPbcAlphaEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_ALPHA in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_ALPHA in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_B
 */

void PBCGUI::OnTextctrlPbcBEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_B in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_B in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_BETA
 */

void PBCGUI::OnTextctrlPbcBetaEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_BETA in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_BETA in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_C
 */

void PBCGUI::OnTextctrlPbcCEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_C in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_C in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_GAMMA
 */

void PBCGUI::OnTextctrlPbcGammaEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_GAMMA in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PBC_GAMMA in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PBC_CENTER
 */

void PBCGUI::OnCheckboxPbcCenterClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PBC_CENTER in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PBC_CENTER in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PBC_SHOWLATTICE
 */

void PBCGUI::OnCheckboxPbcShowlatticeClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PBC_SHOWLATTICE in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PBC_SHOWLATTICE in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_FOLD
 */

void PBCGUI::OnButtonPbcFoldClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_FOLD in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_FOLD in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_DELETE
 */

void PBCGUI::OnButtonPbcDeleteClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_DELETE in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_DELETE in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_EQUIVRECT
 */

void PBCGUI::OnButtonPbcEquivrectClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_EQUIVRECT in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_EQUIVRECT in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PBC_GENERATE
 */

void PBCGUI::OnPbcGenerateClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PBC_GENERATE in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PBC_GENERATE in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PBC_XREP
 */

void PBCGUI::OnSpinctrlPbcXrepUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PBC_XREP in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PBC_XREP in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_PBC_XREP
 */

void PBCGUI::OnSpinctrlPbcXrepTextUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_PBC_XREP in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_PBC_XREP in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PBC_YREP
 */

void PBCGUI::OnSpinctrlPbcYrepUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PBC_YREP in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PBC_YREP in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_PBC_YREP
 */

void PBCGUI::OnSpinctrlPbcYrepTextUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_PBC_YREP in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_PBC_YREP in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PBC_ZREP
 */

void PBCGUI::OnSpinctrlPbcZrepUpdated( wxSpinEvent& event )
{
////@begin wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PBC_ZREP in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_PBC_ZREP in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_PBC_ZREP
 */

void PBCGUI::OnSpinctrlPbcZrepTextUpdated( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_PBC_ZREP in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_PBC_ZREP in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PBC_UNIFORM
 */

void PBCGUI::OnCheckboxPbcUniformClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PBC_UNIFORM in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PBC_UNIFORM in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_REPLICATE
 */

void PBCGUI::OnButtonPbcReplicateClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_REPLICATE in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_REPLICATE in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_RESTORE
 */

void PBCGUI::OnButtonPbcRestoreClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_RESTORE in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_RESTORE in PBCGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_SUPER
 */

void PBCGUI::OnButtonPbcSuperClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_SUPER in PBCGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PBC_SUPER in PBCGUI. 
}

/*!
 * Should we show tooltips?
 */

bool PBCGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap PBCGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin PBCGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end PBCGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon PBCGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin PBCGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end PBCGUI icon retrieval
}

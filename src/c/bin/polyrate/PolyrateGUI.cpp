/////////////////////////////////////////////////////////////////////////////
// Name:        PolyrateGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "PolyrateGUI.H"
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
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/ewxMenuBar.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxButton.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxFrame.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "PolyrateGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID PolyrateGUI::ID_LISTBOX_PLYRT_TRSN = wxNewId();
const wxWindowID PolyrateGUI::ID_CHOICE_PLYRT_CMPST = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_SYMNUM = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_MINS = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_TMPRTR = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_DVALS = wxNewId();
const wxWindowID PolyrateGUI::ID_BUTTON_PLYRT_TRSNDL = wxNewId();
const wxWindowID PolyrateGUI::ID_CHOICE_PLYRT_SCHM = wxNewId();
const wxWindowID PolyrateGUI::ID_PANEL_PLYRT_TORID = wxNewId();
const wxWindowID PolyrateGUI::ID_STATICTEXT = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_DEG3 = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_DEG2 = wxNewId();
const wxWindowID PolyrateGUI::ID_PANEL_PLYRT_MORSE = wxNewId();
const wxWindowID PolyrateGUI::ID_BUTTON_PLYRT_MRSADD = wxNewId();
const wxWindowID PolyrateGUI::ID_CHECKBOX_PLYRT_ANLYS = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_NRG1 = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_DEG1 = wxNewId();
const wxWindowID PolyrateGUI::ID_BUTTON_PLYRT_ADDRNG = wxNewId();
const wxWindowID PolyrateGUI::wxID_STATIC_PLYRT_NRG1 = wxNewId();
const wxWindowID PolyrateGUI::ID_NOTEBOOK = wxNewId();
const wxWindowID PolyrateGUI::wxID_STATIC_PLYRT_MINS = wxNewId();
const wxWindowID PolyrateGUI::ID_CHECKBOX_PLYRT_SDDL = wxNewId();
const wxWindowID PolyrateGUI::ID_CHOICE_PLYRT_AMOD = wxNewId();
const wxWindowID PolyrateGUI::ID_CHOICE_PLYRT_AXIS = wxNewId();
const wxWindowID PolyrateGUI::ID_LISTBOX_PLYRT_TMPLST = wxNewId();
const wxWindowID PolyrateGUI::ID_BUTTON_PLYRT_OPTADD = wxNewId();
const wxWindowID PolyrateGUI::wxID_STATIC_PLYRT_MAXS = wxNewId();
const wxWindowID PolyrateGUI::ID_CHOICE_PLYRT_APLVL = wxNewId();
const wxWindowID PolyrateGUI::ID_BUTTON_PLYRT_DELETE = wxNewId();
const wxWindowID PolyrateGUI::ID_BUTTON_PLYRT_CLRST = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_TRSNMD = wxNewId();
const wxWindowID PolyrateGUI::ID_MENU_FEEDBACK = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_SBGRP = wxNewId();
const wxWindowID PolyrateGUI::ID_LISTBOX_PLYRT_RNGLST = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_SCLRV = wxNewId();
const wxWindowID PolyrateGUI::ID_BUTTON_PLYRT_FINAL_EDIT = wxNewId();
const wxWindowID PolyrateGUI::ID_NOTEBOOK_PLYRT_ANHRM = wxNewId();
const wxWindowID PolyrateGUI::ID_LISTBOX_PLYRT_OPTLST = wxNewId();
const wxWindowID PolyrateGUI::ID_PANEL_PLYRT_ANHRM = wxNewId();
const wxWindowID PolyrateGUI::ID_BUTTON_PLYRT_TRSNAD = wxNewId();
const wxWindowID PolyrateGUI::wxID_STATIC_PLYRT_DVALS = wxNewId();
const wxWindowID PolyrateGUI::wxID_STATIC_PLYRT_NRG2 = wxNewId();
const wxWindowID PolyrateGUI::ID_BUTTON_PLYRT_ADD = wxNewId();
const wxWindowID PolyrateGUI::ID_NOTEBOOK1 = wxNewId();
const wxWindowID PolyrateGUI::ID_BUTTON_PLYRT_DELRNG = wxNewId();
const wxWindowID PolyrateGUI::ID_PANEL_PLYRT_TRSNAH = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_SGMR = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_SGMF = wxNewId();
const wxWindowID PolyrateGUI::ID_BUTTON_PLYRT_LAUNCH = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_MAXS = wxNewId();
const wxWindowID PolyrateGUI::ID_LISTBOX_PLYRT_MRS = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_NRG2 = wxNewId();
const wxWindowID PolyrateGUI::ID_MENU_PLYRT_CLOSE = wxNewId();
const wxWindowID PolyrateGUI::ID_MENU_PLYRT_SAVE = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_DMIN = wxNewId();
const wxWindowID PolyrateGUI::ID_CHOICE_PLYRT_DTYP = wxNewId();
const wxWindowID PolyrateGUI::ID_PANEL_PLYRT_STATES = wxNewId();
const wxWindowID PolyrateGUI::ID_CHECKBOX_PLYRT_FROPT = wxNewId();
const wxWindowID PolyrateGUI::ID_FEEDBACK_PLYRT = wxNewId();
const wxWindowID PolyrateGUI::ID_CHOICE_PLYRT_CMPAX = wxNewId();
const wxWindowID PolyrateGUI::wxID_STATIC_PLYRT_CHKOPT = wxNewId();
const wxWindowID PolyrateGUI::ID_PLYRT_FRAME = wxNewId();
const wxWindowID PolyrateGUI::ID_MENU_PLYRT_HELP = wxNewId();
const wxWindowID PolyrateGUI::ID_BUTTON_PLYRT_MRSDEL = wxNewId();
const wxWindowID PolyrateGUI::ID_BUTTON_PLYRT_OPTDEL = wxNewId();
const wxWindowID PolyrateGUI::wxID_STATIC_PLYRT_NRG3 = wxNewId();
const wxWindowID PolyrateGUI::ID_PANEL_PLYRT_ALGNMNT = wxNewId();
const wxWindowID PolyrateGUI::wxID_STATIC_PLYRT_DATTYP = wxNewId();
const wxWindowID PolyrateGUI::ID_PANEL_PLYRT_TRSNOPT = wxNewId();
const wxWindowID PolyrateGUI::ID_CHOICE_PLYRT_CMPMD = wxNewId();
const wxWindowID PolyrateGUI::ID_CHOICE_PLYRT_MODE = wxNewId();
const wxWindowID PolyrateGUI::ID_PANEL_PLYRT_TMPRTR = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_MRSMD = wxNewId();
const wxWindowID PolyrateGUI::ID_PANEL_PLYRT_TRSNS = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_NRG3 = wxNewId();
const wxWindowID PolyrateGUI::ID_TEXTCTRL_PLYRT_FSCL = wxNewId();

/*!
 * PolyrateGUI type definition
 */

IMPLEMENT_CLASS( PolyrateGUI, ewxFrame )

/*!
 * PolyrateGUI event table definition
 */

BEGIN_EVENT_TABLE( PolyrateGUI, ewxFrame )

////@begin PolyrateGUI event table entries
    EVT_CLOSE( PolyrateGUI::OnCloseWindow )

    EVT_CHECKBOX( ID_CHECKBOX_PLYRT_FROPT, PolyrateGUI::OnCheckboxPlyrtFroptClick )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PLYRT_SCLRV, PolyrateGUI::OnTextctrlPlyrtSclrvEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PLYRT_SGMF, PolyrateGUI::OnTextctrlPlyrtSgmfEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PLYRT_SGMR, PolyrateGUI::OnTextctrlPlyrtSgmrEnter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PLYRT_FSCL, PolyrateGUI::OnTextctrlPlyrtFsclEnter )

    EVT_CHECKBOX( ID_CHECKBOX_PLYRT_SDDL, PolyrateGUI::OnCheckboxPlyrtSddlClick )

    EVT_BUTTON( ID_BUTTON_PLYRT_ADD, PolyrateGUI::OnButtonPlyrtAddClick )

    EVT_BUTTON( ID_BUTTON_PLYRT_DELETE, PolyrateGUI::OnButtonPlyrtDeleteClick )

    EVT_CHECKBOX( ID_CHECKBOX_PLYRT_ANLYS, PolyrateGUI::OnCheckboxPlyrtAnlysClick )

    EVT_CHOICE( ID_CHOICE_PLYRT_CMPST, PolyrateGUI::OnChoicePlyrtCmpstSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PLYRT_DEG1, PolyrateGUI::OnTextctrlPlyrtDeg1Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PLYRT_NRG1, PolyrateGUI::OnTextctrlPlyrtNrg1Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PLYRT_DEG2, PolyrateGUI::OnTextctrlPlyrtDeg2Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PLYRT_NRG2, PolyrateGUI::OnTextctrlPlyrtNrg2Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PLYRT_DEG3, PolyrateGUI::OnTextctrlPlyrtDeg3Enter )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PLYRT_NRG3, PolyrateGUI::OnTextctrlPlyrtNrg3Enter )

    EVT_BUTTON( ID_BUTTON_PLYRT_CLRST, PolyrateGUI::OnButtonPlyrtClrstClick )

    EVT_CHOICE( ID_CHOICE_PLYRT_CMPMD, PolyrateGUI::OnChoicePlyrtCmpmdSelected )

    EVT_BUTTON( ID_BUTTON_PLYRT_MRSADD, PolyrateGUI::OnButtonPlyrtMrsaddClick )

    EVT_BUTTON( ID_BUTTON_PLYRT_MRSDEL, PolyrateGUI::OnButtonPlyrtMrsdelClick )

    EVT_LISTBOX( ID_LISTBOX_PLYRT_MRS, PolyrateGUI::OnListboxPlyrtMrsSelected )

    EVT_BUTTON( ID_BUTTON_PLYRT_TRSNAD, PolyrateGUI::OnButtonPlyrtTrsnadClick )

    EVT_BUTTON( ID_BUTTON_PLYRT_TRSNDL, PolyrateGUI::OnButtonPlyrtTrsndlClick )

    EVT_LISTBOX( ID_LISTBOX_PLYRT_TRSN, PolyrateGUI::OnListboxPlyrtTrsnSelected )

    EVT_CHOICE( ID_CHOICE_PLYRT_MODE, PolyrateGUI::OnChoicePlyrtModeSelected )

    EVT_CHOICE( ID_CHOICE_PLYRT_DTYP, PolyrateGUI::OnChoicePlyrtDtypSelected )

    EVT_TEXT_ENTER( ID_TEXTCTRL_PLYRT_DVALS, PolyrateGUI::OnTextctrlPlyrtDvalsEnter )

    EVT_BUTTON( ID_BUTTON_PLYRT_OPTADD, PolyrateGUI::OnButtonPlyrtOptaddClick )

    EVT_BUTTON( ID_BUTTON_PLYRT_OPTDEL, PolyrateGUI::OnButtonPlyrtOptdelClick )

    EVT_LISTBOX( ID_LISTBOX_PLYRT_OPTLST, PolyrateGUI::OnListboxPlyrtOptlstSelected )

    EVT_CHOICE( ID_CHOICE_PLYRT_CMPAX, PolyrateGUI::OnChoicePlyrtCmpaxSelected )

    EVT_CHOICE( ID_CHOICE_PLYRT_AXIS, PolyrateGUI::OnChoicePlyrtAxisSelected )

    EVT_BUTTON( ID_BUTTON_PLYRT_FINAL_EDIT, PolyrateGUI::OnButtonPlyrtFinalEditClick )

    EVT_BUTTON( ID_BUTTON_PLYRT_LAUNCH, PolyrateGUI::OnButtonPlyrtLaunchClick )

    EVT_MENU( ID_MENU_PLYRT_SAVE, PolyrateGUI::OnMenuPlyrtSaveClick )

    EVT_MENU( ID_MENU_PLYRT_CLOSE, PolyrateGUI::OnMenuPlyrtCloseClick )

    EVT_MENU( ID_MENU_FEEDBACK, PolyrateGUI::OnMenuFeedbackClick )

    EVT_MENU( ID_MENU_PLYRT_HELP, PolyrateGUI::OnMenuPlyrtHelpClick )

////@end PolyrateGUI event table entries

END_EVENT_TABLE()

/*!
 * PolyrateGUI constructors
 */

PolyrateGUI::PolyrateGUI( )
{
}

PolyrateGUI::PolyrateGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * PolyrateGUI creator
 */

bool PolyrateGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin PolyrateGUI member initialisation
////@end PolyrateGUI member initialisation

////@begin PolyrateGUI creation
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end PolyrateGUI creation
    return true;
}

/*!
 * Control creation for PolyrateGUI
 */

void PolyrateGUI::CreateControls()
{    
////@begin PolyrateGUI content construction
    PolyrateGUI* itemFrame1 = this;

    ewxMenuBar* menuBar = new ewxMenuBar;
    wxMenu* itemMenu161 = new wxMenu;
    itemMenu161->Append(ID_MENU_PLYRT_SAVE, _("Save"), _T(""), wxITEM_NORMAL);
    itemMenu161->AppendSeparator();
    itemMenu161->Append(ID_MENU_PLYRT_CLOSE, _("Close"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu161, _("File"));
    wxMenu* itemMenu165 = new wxMenu;
    itemMenu165->Append(ID_MENU_FEEDBACK, _("Support..."), _T(""), wxITEM_NORMAL);
    itemMenu165->Append(ID_MENU_PLYRT_HELP, _("on POLYRATE..."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu165, _("Help"));
    itemFrame1->SetMenuBar(menuBar);

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(itemBoxSizer2);

    wxBoxSizer* itemBoxSizer3 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer3, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox4 = new ewxCheckBox( itemFrame1, ID_CHECKBOX_PLYRT_FROPT, _("Evaluate Both Forward and Reverse Reactions"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox4->SetValue(false);
    itemBoxSizer3->Add(itemCheckBox4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer5, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText6 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Scale Factor for Reverse Reaction Rates:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemStaticText6, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl7 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_PLYRT_SCLRV, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer5->Add(itemTextCtrl7, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer8 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer8, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText9 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Symmetry Factors:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer8->Add(itemStaticText9, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer10, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText11 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Forward Reaction:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemStaticText11, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl12 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_PLYRT_SGMF, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemTextCtrl12, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer10->Add(50, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);

    ewxStaticText* itemStaticText14 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Reverse Reaction:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemStaticText14, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl15 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_PLYRT_SGMR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer10->Add(itemTextCtrl15, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer16, 0, wxALIGN_LEFT|wxALL, 0);

    ewxStaticText* itemStaticText17 = new ewxStaticText( itemFrame1, wxID_STATIC, _("Frequency Scale Factor:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add(itemStaticText17, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl18 = new ewxTextCtrl( itemFrame1, ID_TEXTCTRL_PLYRT_FSCL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add(itemTextCtrl18, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer19 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer19, 0, wxALIGN_LEFT|wxALL, 0);

    ewxCheckBox* itemCheckBox20 = new ewxCheckBox( itemFrame1, ID_CHECKBOX_PLYRT_SDDL, _("Indicate Saddle Point"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox20->SetValue(true);
    itemBoxSizer19->Add(itemCheckBox20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxNotebook* itemNotebook21 = new ewxNotebook( itemFrame1, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

    ewxPanel* itemPanel22 = new ewxPanel( itemNotebook21, ID_PANEL_PLYRT_TMPRTR, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer23 = new wxBoxSizer(wxVERTICAL);
    itemPanel22->SetSizer(itemBoxSizer23);

    wxBoxSizer* itemBoxSizer24 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer23->Add(itemBoxSizer24, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText25 = new ewxStaticText( itemPanel22, wxID_STATIC, _("Temperatures at which to Evaluate Rate Constants:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer24->Add(itemStaticText25, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer26 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer23->Add(itemBoxSizer26, 0, wxGROW|wxALL, 0);
    wxFlexGridSizer* itemFlexGridSizer27 = new wxFlexGridSizer(3, 2, 0, 0);
    itemBoxSizer26->Add(itemFlexGridSizer27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 0);
    ewxStaticText* itemStaticText28 = new ewxStaticText( itemPanel22, wxID_STATIC, _("Temperature:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer27->Add(itemStaticText28, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl29 = new ewxTextCtrl( itemPanel22, ID_TEXTCTRL_PLYRT_TMPRTR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer27->Add(itemTextCtrl29, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemFlexGridSizer27->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton31 = new ewxButton( itemPanel22, ID_BUTTON_PLYRT_ADD, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer27->Add(itemButton31, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemFlexGridSizer27->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton33 = new ewxButton( itemPanel22, ID_BUTTON_PLYRT_DELETE, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer27->Add(itemButton33, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer26->Add(40, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxString* itemListBox35Strings = NULL;
    ewxListBox* itemListBox35 = new ewxListBox( itemPanel22, ID_LISTBOX_PLYRT_TMPLST, wxDefaultPosition, wxDefaultSize, 0, itemListBox35Strings, wxLB_SINGLE );
    itemBoxSizer26->Add(itemListBox35, 1, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer36 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer23->Add(itemBoxSizer36, 0, wxALIGN_LEFT|wxALL, 0);
    ewxCheckBox* itemCheckBox37 = new ewxCheckBox( itemPanel22, ID_CHECKBOX_PLYRT_ANLYS, _("Apply Detailed Analysis to Rates"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox37->SetValue(true);
    itemBoxSizer36->Add(itemCheckBox37, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook21->AddPage(itemPanel22, _("Temperatures"));

    ewxPanel* itemPanel38 = new ewxPanel( itemNotebook21, ID_PANEL_PLYRT_STATES, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer39 = new wxBoxSizer(wxVERTICAL);
    itemPanel38->SetSizer(itemBoxSizer39);

    wxBoxSizer* itemBoxSizer40 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer39->Add(itemBoxSizer40, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText41 = new ewxStaticText( itemPanel38, wxID_STATIC, _("Component"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer40->Add(itemStaticText41, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice42Strings[] = {
        _("Reactant 1"),
        _("Reactant 2"),
        _("Transition State"),
        _("Product 1"),
        _("Product 2")
    };
    ewxChoice* itemChoice42 = new ewxChoice( itemPanel38, ID_CHOICE_PLYRT_CMPST, wxDefaultPosition, wxDefaultSize, 5, itemChoice42Strings, 0 );
    itemChoice42->SetStringSelection(_("Transition State"));
    itemBoxSizer40->Add(itemChoice42, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

    wxFlexGridSizer* itemFlexGridSizer43 = new wxFlexGridSizer(4, 3, 0, 0);
    itemBoxSizer39->Add(itemFlexGridSizer43, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText44 = new ewxStaticText( itemPanel38, wxID_STATIC, _("Degeneracy"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(itemStaticText44, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText45 = new ewxStaticText( itemPanel38, wxID_STATIC, _("Energy"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(itemStaticText45, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer43->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxTextCtrl* itemTextCtrl47 = new ewxTextCtrl( itemPanel38, ID_TEXTCTRL_PLYRT_DEG1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(itemTextCtrl47, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer48 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer43->Add(itemBoxSizer48, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);
    ewxTextCtrl* itemTextCtrl49 = new ewxTextCtrl( itemPanel38, ID_TEXTCTRL_PLYRT_NRG1, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemTextCtrl49, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText50 = new ewxStaticText( itemPanel38, wxID_STATIC_PLYRT_NRG1, _("Hartree"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemStaticText50, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText51 = new ewxStaticText( itemPanel38, wxID_STATIC, _("     (Ground State)"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer48->Add(itemStaticText51, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer43->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxTextCtrl* itemTextCtrl53 = new ewxTextCtrl( itemPanel38, ID_TEXTCTRL_PLYRT_DEG2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(itemTextCtrl53, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer54 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer43->Add(itemBoxSizer54, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 0);
    ewxTextCtrl* itemTextCtrl55 = new ewxTextCtrl( itemPanel38, ID_TEXTCTRL_PLYRT_NRG2, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer54->Add(itemTextCtrl55, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText56 = new ewxStaticText( itemPanel38, wxID_STATIC_PLYRT_NRG2, _("Hartree"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer54->Add(itemStaticText56, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP|wxADJUST_MINSIZE, 3);

    itemFlexGridSizer43->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxTextCtrl* itemTextCtrl58 = new ewxTextCtrl( itemPanel38, ID_TEXTCTRL_PLYRT_DEG3, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(itemTextCtrl58, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer59 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer43->Add(itemBoxSizer59, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 0);
    ewxTextCtrl* itemTextCtrl60 = new ewxTextCtrl( itemPanel38, ID_TEXTCTRL_PLYRT_NRG3, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer59->Add(itemTextCtrl60, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText61 = new ewxStaticText( itemPanel38, wxID_STATIC_PLYRT_NRG3, _("Hartree"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer59->Add(itemStaticText61, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxButton* itemButton62 = new ewxButton( itemPanel38, ID_BUTTON_PLYRT_CLRST, _("Clear States"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer43->Add(itemButton62, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook21->AddPage(itemPanel38, _("Electronic States"));

    ewxPanel* itemPanel63 = new ewxPanel( itemNotebook21, ID_PANEL_PLYRT_ANHRM, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer64 = new wxBoxSizer(wxVERTICAL);
    itemPanel63->SetSizer(itemBoxSizer64);

    wxBoxSizer* itemBoxSizer65 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer64->Add(itemBoxSizer65, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText66 = new ewxStaticText( itemPanel63, wxID_STATIC, _("Component"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer65->Add(itemStaticText66, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice67Strings[] = {
        _("Reactant 1"),
        _("Reactant 2"),
        _("Transition State"),
        _("Product 1"),
        _("Product 2")
    };
    ewxChoice* itemChoice67 = new ewxChoice( itemPanel63, ID_CHOICE_PLYRT_CMPMD, wxDefaultPosition, wxDefaultSize, 5, itemChoice67Strings, 0 );
    itemChoice67->SetStringSelection(_("Reactant 1"));
    itemBoxSizer65->Add(itemChoice67, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText68 = new ewxStaticText( itemPanel63, ID_STATICTEXT, _("\n\n"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer65->Add(itemStaticText68, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxADJUST_MINSIZE, 5);

    ewxStaticText* itemStaticText69 = new ewxStaticText( itemPanel63, wxID_STATIC_PLYRT_CHKOPT, _("Warning: Additional hindered rotor\ntorsion mode options must be set\nto generate valid input file"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText69->SetForegroundColour(wxColour(255, 0, 0));
    itemStaticText69->Show(false);
    itemBoxSizer65->Add(itemStaticText69, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxNotebook* itemNotebook70 = new ewxNotebook( itemPanel63, ID_NOTEBOOK_PLYRT_ANHRM, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

    ewxPanel* itemPanel71 = new ewxPanel( itemNotebook70, ID_PANEL_PLYRT_MORSE, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer72 = new wxBoxSizer(wxVERTICAL);
    itemPanel71->SetSizer(itemBoxSizer72);

    wxBoxSizer* itemBoxSizer73 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer72->Add(itemBoxSizer73, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText74 = new ewxStaticText( itemPanel71, wxID_STATIC, _("Stretching Mode:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer73->Add(itemStaticText74, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl75 = new ewxTextCtrl( itemPanel71, ID_TEXTCTRL_PLYRT_MRSMD, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer73->Add(itemTextCtrl75, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer76 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer72->Add(itemBoxSizer76, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer77 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer76->Add(itemBoxSizer77, 0, wxALIGN_BOTTOM|wxALL, 0);
    ewxButton* itemButton78 = new ewxButton( itemPanel71, ID_BUTTON_PLYRT_MRSADD, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton78->SetDefault();
    itemBoxSizer77->Add(itemButton78, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    ewxButton* itemButton79 = new ewxButton( itemPanel71, ID_BUTTON_PLYRT_MRSDEL, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer77->Add(itemButton79, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxString* itemListBox80Strings = NULL;
    ewxListBox* itemListBox80 = new ewxListBox( itemPanel71, ID_LISTBOX_PLYRT_MRS, wxDefaultPosition, wxSize(250, 80), 0, itemListBox80Strings, wxLB_SINGLE );
    itemBoxSizer76->Add(itemListBox80, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook70->AddPage(itemPanel71, _("Morse"));

    ewxPanel* itemPanel81 = new ewxPanel( itemNotebook70, ID_PANEL_PLYRT_TRSNS, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer82 = new wxBoxSizer(wxVERTICAL);
    itemPanel81->SetSizer(itemBoxSizer82);

    ewxNotebook* itemNotebook83 = new ewxNotebook( itemPanel81, ID_NOTEBOOK1, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

    ewxPanel* itemPanel84 = new ewxPanel( itemNotebook83, ID_PANEL_PLYRT_TORID, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer85 = new wxBoxSizer(wxVERTICAL);
    itemPanel84->SetSizer(itemBoxSizer85);

    wxBoxSizer* itemBoxSizer86 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer85->Add(itemBoxSizer86, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText87 = new ewxStaticText( itemPanel84, wxID_STATIC, _("Torsion Mode:"), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemStaticText87->SetToolTip(_("Use POLYRATE indexing for normal modes"));
    itemBoxSizer86->Add(itemStaticText87, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl88 = new ewxTextCtrl( itemPanel84, ID_TEXTCTRL_PLYRT_TRSNMD, _T(""), wxDefaultPosition, wxSize(40, -1), 0 );
    if (ShowToolTips())
        itemTextCtrl88->SetToolTip(_("Use POLYRATE indexing for normal modes"));
    itemBoxSizer86->Add(itemTextCtrl88, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText89 = new ewxStaticText( itemPanel84, wxID_STATIC, _("Number of Distinct Minima:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer86->Add(itemStaticText89, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl90 = new ewxTextCtrl( itemPanel84, ID_TEXTCTRL_PLYRT_DMIN, _T(""), wxDefaultPosition, wxSize(40, -1), 0 );
    itemBoxSizer86->Add(itemTextCtrl90, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer91 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer85->Add(itemBoxSizer91, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText92 = new ewxStaticText( itemPanel84, wxID_STATIC, _("Symmetry Number of Lowest Energy Minima:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer91->Add(itemStaticText92, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl93 = new ewxTextCtrl( itemPanel84, ID_TEXTCTRL_PLYRT_SYMNUM, _T(""), wxDefaultPosition, wxSize(40, -1), 0 );
    itemBoxSizer91->Add(itemTextCtrl93, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer94 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer85->Add(itemBoxSizer94, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText95 = new ewxStaticText( itemPanel84, wxID_STATIC, _("Number of Atoms in First Subgroup:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer94->Add(itemStaticText95, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl96 = new ewxTextCtrl( itemPanel84, ID_TEXTCTRL_PLYRT_SBGRP, _T(""), wxDefaultPosition, wxSize(40, -1), 0 );
    itemBoxSizer94->Add(itemTextCtrl96, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer97 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer85->Add(itemBoxSizer97, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText98 = new ewxStaticText( itemPanel84, wxID_STATIC, _("Parameterization Scheme:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer97->Add(itemStaticText98, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice99Strings[] = {
        _("Frequency-Barrier"),
        _("Frequency-Rectilinear"),
        _("Frequency-Curvilinear"),
        _("Barrier-Rectilinear"),
        _("Barrier-Curvilinear")
    };
    ewxChoice* itemChoice99 = new ewxChoice( itemPanel84, ID_CHOICE_PLYRT_SCHM, wxDefaultPosition, wxDefaultSize, 5, itemChoice99Strings, 0 );
    itemChoice99->SetStringSelection(_("Frequency-Barrier"));
    itemBoxSizer97->Add(itemChoice99, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer100 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer85->Add(itemBoxSizer100, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText101 = new ewxStaticText( itemPanel84, wxID_STATIC, _("Approximation Level:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer100->Add(itemStaticText101, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice102Strings[] = {
        _("Full"),
        _("Single Conformer"),
        _("Single Frequency")
    };
    ewxChoice* itemChoice102 = new ewxChoice( itemPanel84, ID_CHOICE_PLYRT_APLVL, wxDefaultPosition, wxDefaultSize, 3, itemChoice102Strings, 0 );
    itemChoice102->SetStringSelection(_("Full"));
    itemBoxSizer100->Add(itemChoice102, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer103 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer85->Add(itemBoxSizer103, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer104 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer103->Add(itemBoxSizer104, 0, wxALIGN_BOTTOM|wxALL, 0);
    ewxButton* itemButton105 = new ewxButton( itemPanel84, ID_BUTTON_PLYRT_TRSNAD, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton105->SetDefault();
    itemBoxSizer104->Add(itemButton105, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    ewxButton* itemButton106 = new ewxButton( itemPanel84, ID_BUTTON_PLYRT_TRSNDL, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer104->Add(itemButton106, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    wxString* itemListBox107Strings = NULL;
    ewxListBox* itemListBox107 = new ewxListBox( itemPanel84, ID_LISTBOX_PLYRT_TRSN, wxDefaultPosition, wxSize(250, 80), 0, itemListBox107Strings, wxLB_SINGLE );
    itemBoxSizer103->Add(itemListBox107, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook83->AddPage(itemPanel84, _("Torsion Mode"));

    ewxPanel* itemPanel108 = new ewxPanel( itemNotebook83, ID_PANEL_PLYRT_TRSNOPT, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer109 = new wxBoxSizer(wxVERTICAL);
    itemPanel108->SetSizer(itemBoxSizer109);

    wxBoxSizer* itemBoxSizer110 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer109->Add(itemBoxSizer110, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText111 = new ewxStaticText( itemPanel108, wxID_STATIC, _("Mode:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer110->Add(itemStaticText111, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemChoice112Strings = NULL;
    ewxChoice* itemChoice112 = new ewxChoice( itemPanel108, ID_CHOICE_PLYRT_MODE, wxDefaultPosition, wxDefaultSize, 0, itemChoice112Strings, 0 );
    itemBoxSizer110->Add(itemChoice112, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer113 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer109->Add(itemBoxSizer113, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText114 = new ewxStaticText( itemPanel108, wxID_STATIC_PLYRT_DATTYP, _("Data Type:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer113->Add(itemStaticText114, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice115Strings[] = {
        _("Barrier Heights"),
        _("First Subgroup Atoms"),
        _("Indices of Rotation Axis Atoms"),
        _("Symmetry Numbers"),
        _("Excitation Energies"),
        _("Barrier Frequencies"),
        _("Moments of Inertia")
    };
    ewxChoice* itemChoice115 = new ewxChoice( itemPanel108, ID_CHOICE_PLYRT_DTYP, wxDefaultPosition, wxDefaultSize, 7, itemChoice115Strings, 0 );
    itemChoice115->SetStringSelection(_("Barrier Heights"));
    itemBoxSizer113->Add(itemChoice115, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer116 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer109->Add(itemBoxSizer116, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText117 = new ewxStaticText( itemPanel108, wxID_STATIC, _("Data Values:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer116->Add(itemStaticText117, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl118 = new ewxTextCtrl( itemPanel108, ID_TEXTCTRL_PLYRT_DVALS, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    itemBoxSizer116->Add(itemTextCtrl118, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText119 = new ewxStaticText( itemPanel108, wxID_STATIC_PLYRT_DVALS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer116->Add(itemStaticText119, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer120 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer109->Add(itemBoxSizer120, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer121 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer120->Add(itemBoxSizer121, 0, wxALIGN_BOTTOM|wxALL, 0);
    ewxButton* itemButton122 = new ewxButton( itemPanel108, ID_BUTTON_PLYRT_OPTADD, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer121->Add(itemButton122, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);

    ewxButton* itemButton123 = new ewxButton( itemPanel108, ID_BUTTON_PLYRT_OPTDEL, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer121->Add(itemButton123, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);

    wxString* itemListBox124Strings = NULL;
    ewxListBox* itemListBox124 = new ewxListBox( itemPanel108, ID_LISTBOX_PLYRT_OPTLST, wxDefaultPosition, wxSize(250, 80), 0, itemListBox124Strings, wxLB_SINGLE );
    itemBoxSizer120->Add(itemListBox124, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook83->AddPage(itemPanel108, _("Mode Options"));

    ewxPanel* itemPanel125 = new ewxPanel( itemNotebook83, ID_PANEL_PLYRT_TRSNAH, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer126 = new wxBoxSizer(wxVERTICAL);
    itemPanel125->SetSizer(itemBoxSizer126);

    wxBoxSizer* itemBoxSizer127 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer126->Add(itemBoxSizer127, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText128 = new ewxStaticText( itemPanel125, wxID_STATIC, _("Mode:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer127->Add(itemStaticText128, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemChoice129Strings = NULL;
    ewxChoice* itemChoice129 = new ewxChoice( itemPanel125, ID_CHOICE_PLYRT_AMOD, wxDefaultPosition, wxDefaultSize, 0, itemChoice129Strings, 0 );
    itemBoxSizer127->Add(itemChoice129, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer130 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer126->Add(itemBoxSizer130, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText131 = new ewxStaticText( itemPanel125, wxID_STATIC, _("Minimum Value of Reaction Coordinate:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer130->Add(itemStaticText131, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl132 = new ewxTextCtrl( itemPanel125, ID_TEXTCTRL_PLYRT_MINS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer130->Add(itemTextCtrl132, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText133 = new ewxStaticText( itemPanel125, wxID_STATIC_PLYRT_MINS, _("Bohr"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer130->Add(itemStaticText133, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer134 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer126->Add(itemBoxSizer134, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText135 = new ewxStaticText( itemPanel125, wxID_STATIC, _("Maximum Value of Reaction Coordinate:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer134->Add(itemStaticText135, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl136 = new ewxTextCtrl( itemPanel125, ID_TEXTCTRL_PLYRT_MAXS, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer134->Add(itemTextCtrl136, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText137 = new ewxStaticText( itemPanel125, wxID_STATIC_PLYRT_MAXS, _("Bohr"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer134->Add(itemStaticText137, 0, wxALIGN_BOTTOM|wxALL|wxADJUST_MINSIZE, 5);

    wxBoxSizer* itemBoxSizer138 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer126->Add(itemBoxSizer138, 0, wxALIGN_LEFT|wxALL, 0);
    wxBoxSizer* itemBoxSizer139 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer138->Add(itemBoxSizer139, 0, wxALIGN_BOTTOM|wxALL, 0);
    ewxButton* itemButton140 = new ewxButton( itemPanel125, ID_BUTTON_PLYRT_ADDRNG, _("Add"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer139->Add(itemButton140, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);

    ewxButton* itemButton141 = new ewxButton( itemPanel125, ID_BUTTON_PLYRT_DELRNG, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer139->Add(itemButton141, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 3);

    wxString* itemListBox142Strings = NULL;
    ewxListBox* itemListBox142 = new ewxListBox( itemPanel125, ID_LISTBOX_PLYRT_RNGLST, wxDefaultPosition, wxSize(250, 80), 0, itemListBox142Strings, wxLB_SINGLE );
    itemBoxSizer138->Add(itemListBox142, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook83->AddPage(itemPanel125, _("Ranges"));

    itemBoxSizer82->Add(itemNotebook83, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    itemNotebook70->AddPage(itemPanel81, _("Hindered Rotor"));

    itemBoxSizer64->Add(itemNotebook70, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    itemNotebook21->AddPage(itemPanel63, _("Anharmonicity"));

    ewxPanel* itemPanel143 = new ewxPanel( itemNotebook21, ID_PANEL_PLYRT_ALGNMNT, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER|wxTAB_TRAVERSAL );
    wxBoxSizer* itemBoxSizer144 = new wxBoxSizer(wxVERTICAL);
    itemPanel143->SetSizer(itemBoxSizer144);

    wxBoxSizer* itemBoxSizer145 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer144->Add(itemBoxSizer145, 0, wxALIGN_LEFT|wxALL, 0);
    ewxStaticText* itemStaticText146 = new ewxStaticText( itemPanel143, wxID_STATIC, _("Align Linear Component"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer145->Add(itemStaticText146, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice147Strings[] = {
        _("Reactant 1"),
        _("Reactant 2"),
        _("Transition State"),
        _("Product 1"),
        _("Product 2")
    };
    ewxChoice* itemChoice147 = new ewxChoice( itemPanel143, ID_CHOICE_PLYRT_CMPAX, wxDefaultPosition, wxDefaultSize, 5, itemChoice147Strings, 0 );
    itemChoice147->SetStringSelection(_("Transition State"));
    itemBoxSizer145->Add(itemChoice147, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText148 = new ewxStaticText( itemPanel143, wxID_STATIC, _("Axis:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer145->Add(itemStaticText148, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString itemChoice149Strings[] = {
        _("X"),
        _("Y"),
        _("Z")
    };
    ewxChoice* itemChoice149 = new ewxChoice( itemPanel143, ID_CHOICE_PLYRT_AXIS, wxDefaultPosition, wxDefaultSize, 3, itemChoice149Strings, 0 );
    itemChoice149->SetStringSelection(_("Z"));
    itemBoxSizer145->Add(itemChoice149, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemNotebook21->AddPage(itemPanel143, _("Alignment"));

    itemBoxSizer2->Add(itemNotebook21, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 0);

    wxBoxSizer* itemBoxSizer150 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer150, 0, wxGROW|wxRIGHT|wxTOP|wxBOTTOM, 0);

    ewxStaticLine* itemStaticLine151 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer150->Add(itemStaticLine151, 1, wxGROW|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer152 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer152, 0, wxALIGN_LEFT|wxALL, 0);

    ewxButton* itemButton153 = new ewxButton( itemFrame1, ID_BUTTON_PLYRT_FINAL_EDIT, _("Final Edit..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer152->Add(itemButton153, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer152->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton155 = new ewxButton( itemFrame1, ID_BUTTON_PLYRT_LAUNCH, _("Launch..."), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer152->Add(itemButton155, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer156 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer156, 0, wxGROW|wxRIGHT|wxTOP|wxBOTTOM, 0);

    ewxStaticLine* itemStaticLine157 = new ewxStaticLine( itemFrame1, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer156->Add(itemStaticLine157, 1, wxGROW|wxTOP|wxBOTTOM, 5);

    wxBoxSizer* itemBoxSizer158 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer2->Add(itemBoxSizer158, 0, wxGROW|wxALL, 0);

    WxFeedback* itemWxFeedback159 = new WxFeedback( itemFrame1, ID_FEEDBACK_PLYRT, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxCLIP_CHILDREN  );
    itemBoxSizer158->Add(itemWxFeedback159, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

////@end PolyrateGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_PLYRT_FRAME
 */

void PolyrateGUI::OnCloseWindow( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_PLYRT_FRAME in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_PLYRT_FRAME in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLYRT_FROPT
 */

void PolyrateGUI::OnCheckboxPlyrtFroptClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLYRT_FROPT in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLYRT_FROPT in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_SCLRV
 */

void PolyrateGUI::OnTextctrlPlyrtSclrvEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_SCLRV in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_SCLRV in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_SGMF
 */

void PolyrateGUI::OnTextctrlPlyrtSgmfEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_SGMF in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_SGMF in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_SGMR
 */

void PolyrateGUI::OnTextctrlPlyrtSgmrEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_SGMR in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_SGMR in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_FSCL
 */

void PolyrateGUI::OnTextctrlPlyrtFsclEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_FSCL in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_FSCL in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLYRT_SDDL
 */

void PolyrateGUI::OnCheckboxPlyrtSddlClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLYRT_SDDL in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLYRT_SDDL in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_ADD
 */

void PolyrateGUI::OnButtonPlyrtAddClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_ADD in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_ADD in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_DELETE
 */

void PolyrateGUI::OnButtonPlyrtDeleteClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_DELETE in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_DELETE in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLYRT_ANLYS
 */

void PolyrateGUI::OnCheckboxPlyrtAnlysClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLYRT_ANLYS in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_PLYRT_ANLYS in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_CMPST
 */

void PolyrateGUI::OnChoicePlyrtCmpstSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_CMPST in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_CMPST in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DEG1
 */

void PolyrateGUI::OnTextctrlPlyrtDeg1Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DEG1 in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DEG1 in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_NRG1
 */

void PolyrateGUI::OnTextctrlPlyrtNrg1Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_NRG1 in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_NRG1 in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DEG2
 */

void PolyrateGUI::OnTextctrlPlyrtDeg2Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DEG2 in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DEG2 in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_NRG2
 */

void PolyrateGUI::OnTextctrlPlyrtNrg2Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_NRG2 in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_NRG2 in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DEG3
 */

void PolyrateGUI::OnTextctrlPlyrtDeg3Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DEG3 in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DEG3 in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_NRG3
 */

void PolyrateGUI::OnTextctrlPlyrtNrg3Enter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_NRG3 in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_NRG3 in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_CLRST
 */

void PolyrateGUI::OnButtonPlyrtClrstClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_CLRST in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_CLRST in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_CMPMD
 */

void PolyrateGUI::OnChoicePlyrtCmpmdSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_CMPMD in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_CMPMD in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_MRSADD
 */

void PolyrateGUI::OnButtonPlyrtMrsaddClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_MRSADD in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_MRSADD in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_MRSDEL
 */

void PolyrateGUI::OnButtonPlyrtMrsdelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_MRSDEL in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_MRSDEL in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PLYRT_MRS
 */

void PolyrateGUI::OnListboxPlyrtMrsSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PLYRT_MRS in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PLYRT_MRS in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_TRSNAD
 */

void PolyrateGUI::OnButtonPlyrtTrsnadClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_TRSNAD in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_TRSNAD in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_TRSNDL
 */

void PolyrateGUI::OnButtonPlyrtTrsndlClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_TRSNDL in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_TRSNDL in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PLYRT_TRSN
 */

void PolyrateGUI::OnListboxPlyrtTrsnSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PLYRT_TRSN in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PLYRT_TRSN in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_MODE
 */

void PolyrateGUI::OnChoicePlyrtModeSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_MODE in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_MODE in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_DTYP
 */

void PolyrateGUI::OnChoicePlyrtDtypSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_DTYP in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_DTYP in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DVALS
 */

void PolyrateGUI::OnTextctrlPlyrtDvalsEnter( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DVALS in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_PLYRT_DVALS in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_OPTADD
 */

void PolyrateGUI::OnButtonPlyrtOptaddClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_OPTADD in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_OPTADD in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_OPTDEL
 */

void PolyrateGUI::OnButtonPlyrtOptdelClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_OPTDEL in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_OPTDEL in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PLYRT_OPTLST
 */

void PolyrateGUI::OnListboxPlyrtOptlstSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PLYRT_OPTLST in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_PLYRT_OPTLST in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_CMPAX
 */

void PolyrateGUI::OnChoicePlyrtCmpaxSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_CMPAX in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_CMPAX in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_AXIS
 */

void PolyrateGUI::OnChoicePlyrtAxisSelected( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_AXIS in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_PLYRT_AXIS in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_FINAL_EDIT
 */

void PolyrateGUI::OnButtonPlyrtFinalEditClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_FINAL_EDIT in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_FINAL_EDIT in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_LAUNCH
 */

void PolyrateGUI::OnButtonPlyrtLaunchClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_LAUNCH in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_PLYRT_LAUNCH in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_PLYRT_SAVE
 */

void PolyrateGUI::OnMenuPlyrtSaveClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_PLYRT_SAVE in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_PLYRT_SAVE in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_PLYRT_CLOSE
 */

void PolyrateGUI::OnMenuPlyrtCloseClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_PLYRT_CLOSE in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_PLYRT_CLOSE in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK
 */

void PolyrateGUI::OnMenuFeedbackClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FEEDBACK in PolyrateGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_PLYRT_HELP
 */

void PolyrateGUI::OnMenuPlyrtHelpClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_PLYRT_HELP in PolyrateGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_PLYRT_HELP in PolyrateGUI. 
}

/*!
 * Should we show tooltips?
 */

bool PolyrateGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap PolyrateGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin PolyrateGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end PolyrateGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon PolyrateGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin PolyrateGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end PolyrateGUI icon retrieval
}

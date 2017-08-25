/////////////////////////////////////////////////////////////////////////////
// Name:        WxMachineRegisterGUI.C
// Purpose:     
// Author:      Kenneth R. Swanson
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxMachineRegisterGUI.H"
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
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxFrame.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "WxMachineRegisterGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxMachineRegisterGUI::ID_CHECKBOX_QUEUE_ALLOCATION = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_TEXT_MACHINE_FULLNAME = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_BUTTON_MACHINE_CHANGE = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_SPIN_QUEUE_MAXMEMORY = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_PANEL_WXMACHINEREGISTER_APPLICATIONS = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_CHECKBOX_REMSHELL_SSH_PASS = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_TEXT_MACHINE_REFNAME = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_TEXT_QUEUE_NAME = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_CHOICEBOX_QUEUE = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_CHOICEBOX_QUEUE_MANAGER = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_SPIN_MACHINE_NUMNODES = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_PANEL = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_TEXT_MACHINE_PROC = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_CHECKBOX_REMSHELL_RSH = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_SPIN_QUEUE_MAXWALL = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_CHECKBOX_REMSHELL_SSH = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_BUTTON_FORM_CLOSE = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_CHECKBOX_REMSHELL_SSH_FTP = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_SPIN_QUEUE_MINSCRATCH = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_SPIN_MACHINE_NUMPROCS = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_BUTTON_FORM_CLEAR = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_PANEL_QUEUES = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_PANEL_WXMACHINEREGISTER_MISCPATHS = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_CHECKBOX_REMSHELL_GLOBUS = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_BUTTON_QUEUE_CHANGE = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_LISTBOX_MACHINES = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_BUTTON_QUEUE_REMOVE = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_BUTTON_QUEUE_CLEAR = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_BUTTON_MACHINE_DELETE = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_CHECKBOX_REMSHELL_TELNET = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_TEXT_MACHINE_VENDOR = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_SPIN_QUEUE_MINPROCS = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_BUTTON_HELP = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_TEXT_MACHINE_MODEL = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_CHECKBOX_REMSHELL_GLOBUS_SSH = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_SPIN_QUEUE_MAXPROCS = wxNewId();
const wxWindowID WxMachineRegisterGUI::ID_FRAME_MAIN = wxNewId();

/*!
 * WxMachineRegisterGUI type definition
 */

IMPLEMENT_CLASS( WxMachineRegisterGUI, ewxFrame )

/*!
 * WxMachineRegisterGUI event table definition
 */

BEGIN_EVENT_TABLE( WxMachineRegisterGUI, ewxFrame )

////@begin WxMachineRegisterGUI event table entries
    EVT_CLOSE( WxMachineRegisterGUI::mainWindowCloseCB )

    EVT_LISTBOX( ID_LISTBOX_MACHINES, WxMachineRegisterGUI::machinesListBoxSelectedCB )

    EVT_TEXT( ID_TEXT_MACHINE_FULLNAME, WxMachineRegisterGUI::machineFullNameUpdatedCB )

    EVT_TEXT( ID_TEXT_MACHINE_REFNAME, WxMachineRegisterGUI::machineRefNameUpdatedCB )

    EVT_TEXT( ID_TEXT_MACHINE_VENDOR, WxMachineRegisterGUI::machineVendorTextUpdatedCB )

    EVT_TEXT( ID_TEXT_MACHINE_MODEL, WxMachineRegisterGUI::machineModelTextUpdatedCB )

    EVT_TEXT( ID_TEXT_MACHINE_PROC, WxMachineRegisterGUI::machineProcTextUpdatedCB )

    EVT_CHOICE( ID_CHOICEBOX_QUEUE, WxMachineRegisterGUI::queueChoiceboxSelectedCB )

    EVT_BUTTON( ID_BUTTON_QUEUE_CHANGE, WxMachineRegisterGUI::queueChangeButtonClickedCB )

    EVT_BUTTON( ID_BUTTON_QUEUE_REMOVE, WxMachineRegisterGUI::queueRemoveButtonClickedCB )

    EVT_BUTTON( ID_BUTTON_QUEUE_CLEAR, WxMachineRegisterGUI::queueClearButtonClickedCB )

    EVT_BUTTON( ID_BUTTON_MACHINE_CHANGE, WxMachineRegisterGUI::machineChangeButtonClickedCB )

    EVT_BUTTON( ID_BUTTON_MACHINE_DELETE, WxMachineRegisterGUI::machineDeleteButtonClickedCB )

    EVT_BUTTON( ID_BUTTON_FORM_CLEAR, WxMachineRegisterGUI::formClearButtonClickedCB )

    EVT_BUTTON( ID_BUTTON_FORM_CLOSE, WxMachineRegisterGUI::formCloseButtonClickedCB )

    EVT_BUTTON( ID_BUTTON_HELP, WxMachineRegisterGUI::helpButtonClickedCB )

////@end WxMachineRegisterGUI event table entries

END_EVENT_TABLE()

/*!
 * WxMachineRegisterGUI constructors
 */

WxMachineRegisterGUI::WxMachineRegisterGUI( )
{
}

WxMachineRegisterGUI::WxMachineRegisterGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * WxMachineRegisterGUI creator
 */

bool WxMachineRegisterGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxMachineRegisterGUI member initialisation
////@end WxMachineRegisterGUI member initialisation

////@begin WxMachineRegisterGUI creation
    SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WxMachineRegisterGUI creation
    return true;
}

/*!
 * Control creation for WxMachineRegisterGUI
 */

void WxMachineRegisterGUI::CreateControls()
{    
////@begin WxMachineRegisterGUI content construction
    WxMachineRegisterGUI* itemFrame1 = this;

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxHORIZONTAL);
    itemFrame1->SetSizer(itemBoxSizer2);

    ewxPanel* itemPanel3 = new ewxPanel( itemFrame1, ID_PANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer2->Add(itemPanel3, 1, wxGROW, 3);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
    itemPanel3->SetSizer(itemBoxSizer4);

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer5, 0, wxGROW, 3);

    wxString* itemListBox6Strings = NULL;
    ewxListBox* itemListBox6 = new ewxListBox( itemPanel3, ID_LISTBOX_MACHINES, wxDefaultPosition, wxSize(150, -1), 0, itemListBox6Strings, wxLB_SINGLE );
    itemBoxSizer5->Add(itemListBox6, 0, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer7 = new wxBoxSizer(wxVERTICAL);
    itemBoxSizer5->Add(itemBoxSizer7, 1, wxGROW, 3);

    wxFlexGridSizer* itemFlexGridSizer8 = new wxFlexGridSizer(4, 2, 0, 0);
    itemFlexGridSizer8->AddGrowableCol(1);
    itemBoxSizer7->Add(itemFlexGridSizer8, 1, wxGROW, 3);

    ewxStaticText* itemStaticText9 = new ewxStaticText( itemPanel3, wxID_STATIC, _("Machine:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer8->Add(itemStaticText9, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl10 = new ewxTextCtrl( itemPanel3, ID_TEXT_MACHINE_FULLNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemTextCtrl10->SetToolTip(_("(Required)\nFully-qualified Machine Name\n(e.g.,\"machine.anywhere.com\")"));
    itemFlexGridSizer8->Add(itemTextCtrl10, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText11 = new ewxStaticText( itemPanel3, wxID_STATIC, _("Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer8->Add(itemStaticText11, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl12 = new ewxTextCtrl( itemPanel3, ID_TEXT_MACHINE_REFNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    if (ShowToolTips())
        itemTextCtrl12->SetToolTip(_("(Required)\nComputer Logical Name\n(e.g., \"curie-batch\")"));
    itemFlexGridSizer8->Add(itemTextCtrl12, 1, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText13 = new ewxStaticText( itemPanel3, wxID_STATIC, _("Vendor:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer8->Add(itemStaticText13, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer8->Add(itemBoxSizer14, 1, wxGROW|wxALIGN_CENTER_VERTICAL, 3);

    ewxTextCtrl* itemTextCtrl15 = new ewxTextCtrl( itemPanel3, ID_TEXT_MACHINE_VENDOR, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemTextCtrl15, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText16 = new ewxStaticText( itemPanel3, wxID_STATIC, _("Model:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
    itemBoxSizer14->Add(itemStaticText16, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl17 = new ewxTextCtrl( itemPanel3, ID_TEXT_MACHINE_MODEL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemTextCtrl17, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText18 = new ewxStaticText( itemPanel3, wxID_STATIC, _("Processor:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemStaticText18, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl19 = new ewxTextCtrl( itemPanel3, ID_TEXT_MACHINE_PROC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer14->Add(itemTextCtrl19, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer20 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer7->Add(itemBoxSizer20, 0, wxALIGN_LEFT, 3);

    ewxStaticText* itemStaticText21 = new ewxStaticText( itemPanel3, wxID_STATIC, _("Total # Processors:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer20->Add(itemStaticText21, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxSpinCtrl* itemSpinCtrl22 = new ewxSpinCtrl( itemPanel3, ID_SPIN_MACHINE_NUMPROCS, _("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100000, 1 );
    itemBoxSizer20->Add(itemSpinCtrl22, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer20->Add(15, 5, 0, wxGROW|wxALL, 3);

    ewxStaticText* itemStaticText24 = new ewxStaticText( itemPanel3, wxID_STATIC, _("# Nodes"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer20->Add(itemStaticText24, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxSpinCtrl* itemSpinCtrl25 = new ewxSpinCtrl( itemPanel3, ID_SPIN_MACHINE_NUMNODES, _("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100000, 1 );
    itemBoxSizer20->Add(itemSpinCtrl25, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 3);

    wxBoxSizer* itemBoxSizer26 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer7->Add(itemBoxSizer26, 0, wxALIGN_LEFT, 3);

    ewxCheckBox* itemCheckBox27 = new ewxCheckBox( itemPanel3, ID_CHECKBOX_REMSHELL_SSH, _("ssh"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox27->SetValue(false);
    itemCheckBox27->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxBOLD, false, _T("Sans")));
    itemBoxSizer26->Add(itemCheckBox27, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox28 = new ewxCheckBox( itemPanel3, ID_CHECKBOX_REMSHELL_SSH_FTP, _("ssh/ftp"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox28->SetValue(false);
    itemCheckBox28->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxBOLD, false, _T("Sans")));
    itemBoxSizer26->Add(itemCheckBox28, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox29 = new ewxCheckBox( itemPanel3, ID_CHECKBOX_REMSHELL_SSH_PASS, _("sshpass"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox29->SetValue(false);
    itemCheckBox29->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxBOLD, false, _T("Sans")));
    itemBoxSizer26->Add(itemCheckBox29, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox30 = new ewxCheckBox( itemPanel3, ID_CHECKBOX_REMSHELL_RSH, _("rsh"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox30->SetValue(false);
    itemCheckBox30->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxBOLD, false, _T("Sans")));
    itemBoxSizer26->Add(itemCheckBox30, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox31 = new ewxCheckBox( itemPanel3, ID_CHECKBOX_REMSHELL_TELNET, _("telnet"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox31->SetValue(false);
    itemCheckBox31->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxBOLD, false, _T("Sans")));
    itemBoxSizer26->Add(itemCheckBox31, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox32 = new ewxCheckBox( itemPanel3, ID_CHECKBOX_REMSHELL_GLOBUS, _("globus"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox32->SetValue(false);
    itemCheckBox32->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxBOLD, false, _T("Sans")));
    itemBoxSizer26->Add(itemCheckBox32, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox33 = new ewxCheckBox( itemPanel3, ID_CHECKBOX_REMSHELL_GLOBUS_SSH, _("globus-ssh"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox33->SetValue(false);
    itemCheckBox33->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxBOLD, false, _T("Sans")));
    itemBoxSizer26->Add(itemCheckBox33, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel34 = new ewxPanel( itemPanel3, ID_PANEL_WXMACHINEREGISTER_APPLICATIONS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer7->Add(itemPanel34, 0, wxGROW|wxALL, 3);

    ewxPanel* itemPanel35 = new ewxPanel( itemPanel3, ID_PANEL_WXMACHINEREGISTER_MISCPATHS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer7->Add(itemPanel35, 0, wxGROW|wxALL, 3);

    ewxPanel* itemPanel36 = new ewxPanel( itemPanel3, ID_PANEL_QUEUES, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer7->Add(itemPanel36, 0, wxGROW|wxALL, 3);

    wxStaticBox* itemStaticBoxSizer37Static = new wxStaticBox(itemPanel36, wxID_ANY, _("Queues"));
    wxStaticBoxSizer* itemStaticBoxSizer37 = new wxStaticBoxSizer(itemStaticBoxSizer37Static, wxVERTICAL);
    itemPanel36->SetSizer(itemStaticBoxSizer37);

    wxBoxSizer* itemBoxSizer38 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer37->Add(itemBoxSizer38, 0, wxGROW, 3);

    ewxStaticText* itemStaticText39 = new ewxStaticText( itemPanel36, wxID_STATIC, _("Queue Manager:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer38->Add(itemStaticText39, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemChoice40Strings = NULL;
    ewxChoice* itemChoice40 = new ewxChoice( itemPanel36, ID_CHOICEBOX_QUEUE_MANAGER, wxDefaultPosition, wxSize(200, -1), 0, itemChoice40Strings, 0 );
    itemBoxSizer38->Add(itemChoice40, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemBoxSizer38->Add(20, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxCheckBox* itemCheckBox42 = new ewxCheckBox( itemPanel36, ID_CHECKBOX_QUEUE_ALLOCATION, _("Allocation Accounts Used"), wxDefaultPosition, wxDefaultSize, wxCHK_2STATE );
    itemCheckBox42->SetValue(false);
    itemBoxSizer38->Add(itemCheckBox42, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer43 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer37->Add(itemBoxSizer43, 0, wxGROW, 3);

    ewxStaticText* itemStaticText44 = new ewxStaticText( itemPanel36, wxID_STATIC, _("Queues:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer43->Add(itemStaticText44, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxString* itemChoice45Strings = NULL;
    ewxChoice* itemChoice45 = new ewxChoice( itemPanel36, ID_CHOICEBOX_QUEUE, wxDefaultPosition, wxSize(200, -1), 0, itemChoice45Strings, 0 );
    itemBoxSizer43->Add(itemChoice45, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText46 = new ewxStaticText( itemPanel36, wxID_STATIC, _("Queue Name:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer43->Add(itemStaticText46, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxTextCtrl* itemTextCtrl47 = new ewxTextCtrl( itemPanel36, ID_TEXT_QUEUE_NAME, _T(""), wxDefaultPosition, wxSize(0, -1), 0 );
    itemBoxSizer43->Add(itemTextCtrl47, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxBoxSizer* itemBoxSizer48 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer37->Add(itemBoxSizer48, 0, wxGROW, 3);

    wxBoxSizer* itemBoxSizer49 = new wxBoxSizer(wxHORIZONTAL);
    itemStaticBoxSizer37->Add(itemBoxSizer49, 0, wxALIGN_LEFT, 3);

    wxFlexGridSizer* itemFlexGridSizer50 = new wxFlexGridSizer(3, 6, 0, 0);
    itemFlexGridSizer50->AddGrowableCol(0);
    itemFlexGridSizer50->AddGrowableCol(3);
    itemBoxSizer49->Add(itemFlexGridSizer50, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText51 = new ewxStaticText( itemPanel36, wxID_STATIC, _("Min Processors:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer50->Add(itemStaticText51, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxSpinCtrl* itemSpinCtrl52 = new ewxSpinCtrl( itemPanel36, ID_SPIN_QUEUE_MINPROCS, _("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100000, 1 );
    itemFlexGridSizer50->Add(itemSpinCtrl52, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemFlexGridSizer50->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText54 = new ewxStaticText( itemPanel36, wxID_STATIC, _("Max Processors:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer50->Add(itemStaticText54, 1, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxSpinCtrl* itemSpinCtrl55 = new ewxSpinCtrl( itemPanel36, ID_SPIN_QUEUE_MAXPROCS, _("1"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100000, 1 );
    itemFlexGridSizer50->Add(itemSpinCtrl55, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    itemFlexGridSizer50->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText57 = new ewxStaticText( itemPanel36, wxID_STATIC, _("Max Wall Time:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer50->Add(itemStaticText57, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxSpinCtrl* itemSpinCtrl58 = new ewxSpinCtrl( itemPanel36, ID_SPIN_QUEUE_MAXWALL, _("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000, 0 );
    itemFlexGridSizer50->Add(itemSpinCtrl58, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText59 = new ewxStaticText( itemPanel36, wxID_STATIC, _("min"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText59->SetFont(wxFont(8, wxDEFAULT, wxNORMAL, wxBOLD, false, _T("Sans")));
    itemFlexGridSizer50->Add(itemStaticText59, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText60 = new ewxStaticText( itemPanel36, wxID_STATIC, _("Max Memory:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer50->Add(itemStaticText60, 1, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxSpinCtrl* itemSpinCtrl61 = new ewxSpinCtrl( itemPanel36, ID_SPIN_QUEUE_MAXMEMORY, _("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000, 0 );
    itemFlexGridSizer50->Add(itemSpinCtrl61, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText62 = new ewxStaticText( itemPanel36, wxID_STATIC, _("MW"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText62->SetFont(wxFont(8, wxDEFAULT, wxNORMAL, wxBOLD, false, _T("Sans")));
    itemFlexGridSizer50->Add(itemStaticText62, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText63 = new ewxStaticText( itemPanel36, wxID_STATIC, _("Min Scratch:"), wxDefaultPosition, wxDefaultSize, 0 );
    itemFlexGridSizer50->Add(itemStaticText63, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 3);

    ewxSpinCtrl* itemSpinCtrl64 = new ewxSpinCtrl( itemPanel36, ID_SPIN_QUEUE_MINSCRATCH, _("0"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000, 0 );
    itemFlexGridSizer50->Add(itemSpinCtrl64, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText65 = new ewxStaticText( itemPanel36, wxID_STATIC, _("MB"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText65->SetFont(wxFont(8, wxDEFAULT, wxNORMAL, wxBOLD, false, _T("Sans")));
    itemFlexGridSizer50->Add(itemStaticText65, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    wxGridSizer* itemGridSizer66 = new wxGridSizer(1, 3, 0, 0);
    itemStaticBoxSizer37->Add(itemGridSizer66, 0, wxGROW, 3);

    ewxButton* itemButton67 = new ewxButton( itemPanel36, ID_BUTTON_QUEUE_CHANGE, _("Add/Change Queue"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer66->Add(itemButton67, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton68 = new ewxButton( itemPanel36, ID_BUTTON_QUEUE_REMOVE, _("Remove Queue"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer66->Add(itemButton68, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton69 = new ewxButton( itemPanel36, ID_BUTTON_QUEUE_CLEAR, _("Clear All Queues"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer66->Add(itemButton69, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxGridSizer* itemGridSizer70 = new wxGridSizer(1, 5, 0, 0);
    itemBoxSizer4->Add(itemGridSizer70, 0, wxGROW, 3);

    ewxButton* itemButton71 = new ewxButton( itemPanel3, ID_BUTTON_MACHINE_CHANGE, _("Add/Change"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton71->Enable(false);
    itemGridSizer70->Add(itemButton71, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton72 = new ewxButton( itemPanel3, ID_BUTTON_MACHINE_DELETE, _("Delete Machine"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton72->Enable(false);
    itemGridSizer70->Add(itemButton72, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton73 = new ewxButton( itemPanel3, ID_BUTTON_FORM_CLEAR, _("Clear Form"), wxDefaultPosition, wxDefaultSize, 0 );
    itemButton73->Enable(false);
    itemGridSizer70->Add(itemButton73, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton74 = new ewxButton( itemPanel3, ID_BUTTON_FORM_CLOSE, _("Close"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer70->Add(itemButton74, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxButton* itemButton75 = new ewxButton( itemPanel3, ID_BUTTON_HELP, _("Help"), wxDefaultPosition, wxDefaultSize, 0 );
    itemGridSizer70->Add(itemButton75, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

////@end WxMachineRegisterGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_FRAME_MAIN
 */

void WxMachineRegisterGUI::mainWindowCloseCB( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_FRAME_MAIN in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_FRAME_MAIN in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_MACHINES
 */

void WxMachineRegisterGUI::machinesListBoxSelectedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_MACHINES in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX_MACHINES in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_FULLNAME
 */

void WxMachineRegisterGUI::machineFullNameUpdatedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_FULLNAME in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_FULLNAME in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_REFNAME
 */

void WxMachineRegisterGUI::machineRefNameUpdatedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_REFNAME in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_REFNAME in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_VENDOR
 */

void WxMachineRegisterGUI::machineVendorTextUpdatedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_VENDOR in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_VENDOR in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_MODEL
 */

void WxMachineRegisterGUI::machineModelTextUpdatedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_MODEL in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_MODEL in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_PROC
 */

void WxMachineRegisterGUI::machineProcTextUpdatedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_PROC in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXT_MACHINE_PROC in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICEBOX_QUEUE
 */

void WxMachineRegisterGUI::queueChoiceboxSelectedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICEBOX_QUEUE in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICEBOX_QUEUE in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_QUEUE_CHANGE
 */

void WxMachineRegisterGUI::queueChangeButtonClickedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_QUEUE_CHANGE in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_QUEUE_CHANGE in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_QUEUE_REMOVE
 */

void WxMachineRegisterGUI::queueRemoveButtonClickedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_QUEUE_REMOVE in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_QUEUE_REMOVE in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_QUEUE_CLEAR
 */

void WxMachineRegisterGUI::queueClearButtonClickedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_QUEUE_CLEAR in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_QUEUE_CLEAR in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINE_CHANGE
 */

void WxMachineRegisterGUI::machineChangeButtonClickedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINE_CHANGE in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINE_CHANGE in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINE_DELETE
 */

void WxMachineRegisterGUI::machineDeleteButtonClickedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINE_DELETE in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_MACHINE_DELETE in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FORM_CLEAR
 */

void WxMachineRegisterGUI::formClearButtonClickedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FORM_CLEAR in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FORM_CLEAR in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FORM_CLOSE
 */

void WxMachineRegisterGUI::formCloseButtonClickedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FORM_CLOSE in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_FORM_CLOSE in WxMachineRegisterGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_HELP
 */

void WxMachineRegisterGUI::helpButtonClickedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_HELP in WxMachineRegisterGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_HELP in WxMachineRegisterGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxMachineRegisterGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxMachineRegisterGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxMachineRegisterGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxMachineRegisterGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxMachineRegisterGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxMachineRegisterGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxMachineRegisterGUI icon retrieval
}

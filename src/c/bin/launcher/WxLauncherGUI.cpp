/////////////////////////////////////////////////////////////////////////////
// Name:        WxLauncherGUI.C
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxLauncherGUI.H"
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
#include "wxgui/WxParameterEdit.H"
#include "wxgui/ewxMenuBar.H"
#include "wxgui/ewxButton.H"
#include "wxgui/ewxStaticLine.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/WxTimeSpanEdit.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/ewxFrame.H"
#include "wxgui/ewxTextCtrl.H"
////@end includes

#include "WxLauncherGUI.H"

////@begin XPM images

////@end XPM images
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_MAXWALLTIME = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_MACHINE = wxNewId();
const wxWindowID WxLauncherGUI::ID_TEXTCTRL_WXLAUNCHER_PASSWORD2 = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_BATCHNODES = wxNewId();
const wxWindowID WxLauncherGUI::ID_STATIC_WXLAUNCHER_MACHINENAME = wxNewId();
const wxWindowID WxLauncherGUI::ID_BUTTON_WXLAUNCHER_SHELLOPEN = wxNewId();
const wxWindowID WxLauncherGUI::ID_STATIC_WXLAUNCHER_MAXWALLTIME = wxNewId();
const wxWindowID WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_FINISHLAUNCH = wxNewId();
const wxWindowID WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_MACHREGISTER = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_6 = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_MINSCRATCH = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_BATCH3 = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_BATCHPROCSLABELS = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_PASSWORD1 = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_BATCHPROCS = wxNewId();
const wxWindowID WxLauncherGUI::ID_STATIC_WXLAUNCHER_QUEUELABELLEFT = wxNewId();
const wxWindowID WxLauncherGUI::ID_TEXTCTRL_WXLAUNCHER_USERNAME = wxNewId();
const wxWindowID WxLauncherGUI::ID_TIMESPANEDIT_WXLAUNCHER_WALLTIME = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_QUEUE = wxNewId();
const wxWindowID WxLauncherGUI::ID_FRAME_WXLAUNCHER = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_SCRATCHDIR = wxNewId();
const wxWindowID WxLauncherGUI::ID_CHOICE_WXLAUNCHER_MACHINES = wxNewId();
const wxWindowID WxLauncherGUI::ID_STATIC_WXLAUNCHER_NOMACHINE = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_ALLOCACCT = wxNewId();
const wxWindowID WxLauncherGUI::ID_PARAMEDIT_WXLAUNCHER_BATCHPROCS = wxNewId();
const wxWindowID WxLauncherGUI::ID_PARAMEDIT_WXLAUNCHER_WKSTNPROCS = wxNewId();
const wxWindowID WxLauncherGUI::ID_TEXTCTRL_WXLAUNCHER_CALCDIR = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_WRKSTSPCFC = wxNewId();
const wxWindowID WxLauncherGUI::ID_TEXTCTRL_WXLAUNCHER_SCRATCHDIR = wxNewId();
const wxWindowID WxLauncherGUI::ID_STATICTEXT = wxNewId();
const wxWindowID WxLauncherGUI::ID_CHOICE_WXLAUNCHER_PRIORITY = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_BATCH0 = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_BATCHPROCSMAX = wxNewId();
const wxWindowID WxLauncherGUI::ID_CHOICE_WXLAUNCHER_REMSHELL = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_DIRECTORIES = wxNewId();
const wxWindowID WxLauncherGUI::ID_TEXTCTRL_WXLAUNCHER_ALLOCACCT = wxNewId();
const wxWindowID WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_STAGELAUNCH = wxNewId();
const wxWindowID WxLauncherGUI::ID_FEEDBACK_WXLAUNCHER_MESSAGES = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_BATCHSPCFC = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_WKSTNPROCS = wxNewId();
const wxWindowID WxLauncherGUI::ID_PARAMEDIT_WXLAUNCHER_BATCHNODES = wxNewId();
const wxWindowID WxLauncherGUI::ID_STATIC_WXLAUNCHER_QUEUELABELRIGHT = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_WALLTIME = wxNewId();
const wxWindowID WxLauncherGUI::ID_STATIC_WXLAUNCHER_USERNAMELABEL = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_PRIORITY = wxNewId();
const wxWindowID WxLauncherGUI::ID_TEXTCTRL_WXLAUNCHER_PASSWORD1 = wxNewId();
const wxWindowID WxLauncherGUI::ID_STATIC_WXLAUNCHER_REQUIREDFIELDSKEY = wxNewId();
const wxWindowID WxLauncherGUI::ID_STATIC_WXLAUNCHER_PASSWORD1LABEL = wxNewId();
const wxWindowID WxLauncherGUI::ID_PARAMEDIT_WXLAUNCHER_MAXMEMORY = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_BATCH2 = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_USERNAME = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_PASSWORD2 = wxNewId();
const wxWindowID WxLauncherGUI::ID_STATIC_WXLAUNCHER_PASSWORD2LABEL = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_CALCDIR = wxNewId();
const wxWindowID WxLauncherGUI::ID_PARAMEDIT_WXLAUNCHER_MINSCRATCH = wxNewId();
const wxWindowID WxLauncherGUI::ID_CHOICE_WXLAUNCHER_QUEUE = wxNewId();
const wxWindowID WxLauncherGUI::ID_BUTTON_WXLAUNCHER_LAUNCH = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_MAXMEMORY = wxNewId();
const wxWindowID WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_HELPSUPPORT = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL_WXLAUNCHER_REMSHELL = wxNewId();
const wxWindowID WxLauncherGUI::ID_PANEL1 = wxNewId();
const wxWindowID WxLauncherGUI::ID_STATIC_WXLAUNCHER_BATCHPROCSMAX = wxNewId();

/*!
 * WxLauncherGUI type definition
 */

IMPLEMENT_CLASS( WxLauncherGUI, ewxFrame )

/*!
 * WxLauncherGUI event table definition
 */

BEGIN_EVENT_TABLE( WxLauncherGUI, ewxFrame )

////@begin WxLauncherGUI event table entries
    EVT_CLOSE( WxLauncherGUI::mainWindowCloseCB )

    EVT_CHOICE( ID_CHOICE_WXLAUNCHER_MACHINES, WxLauncherGUI::machinesChoiceSelectedCB )

    EVT_COMMAND(ID_PARAMEDIT_WXLAUNCHER_WKSTNPROCS, ewxEVT_PARAMETER_VALUE_CHANGED, WxLauncherGUI::wkstnProcsParamValueChangedCB)

    EVT_CHOICE( ID_CHOICE_WXLAUNCHER_PRIORITY, WxLauncherGUI::priorityChoiceSelectedCB )

    EVT_TEXT( ID_TEXTCTRL_WXLAUNCHER_ALLOCACCT, WxLauncherGUI::allocationAccountTextCtrlUpdateCB )

    EVT_CHOICE( ID_CHOICE_WXLAUNCHER_QUEUE, WxLauncherGUI::queueChoiceSelectedCB )

    EVT_COMMAND(ID_TIMESPANEDIT_WXLAUNCHER_WALLTIME, ewxEVT_TIMESPAN_VALUE_CHANGED, WxLauncherGUI::wallTimeLimitChangedCB)

    EVT_COMMAND(ID_PARAMEDIT_WXLAUNCHER_BATCHPROCS, ewxEVT_PARAMETER_VALUE_CHANGED, WxLauncherGUI::batchProcsParamValueChangedCB)

    EVT_COMMAND(ID_PARAMEDIT_WXLAUNCHER_BATCHNODES, ewxEVT_PARAMETER_VALUE_CHANGED, WxLauncherGUI::batchNodesParamValueChangedCB)

    EVT_COMMAND(ID_PARAMEDIT_WXLAUNCHER_MINSCRATCH, ewxEVT_PARAMETER_VALUE_CHANGED, WxLauncherGUI::minScratchParamValueChangedCB)

    EVT_COMMAND(ID_PARAMEDIT_WXLAUNCHER_MAXMEMORY, ewxEVT_PARAMETER_VALUE_CHANGED, WxLauncherGUI::maxMemoryParamValueChangedCB)

    EVT_TEXT( ID_TEXTCTRL_WXLAUNCHER_USERNAME, WxLauncherGUI::usernameTextCtrlUpdateCB )

    EVT_TEXT( ID_TEXTCTRL_WXLAUNCHER_PASSWORD1, WxLauncherGUI::password1TextCtrlUpdateCB )

    EVT_TEXT( ID_TEXTCTRL_WXLAUNCHER_PASSWORD2, WxLauncherGUI::password2TextCtrlUpdateCB )

    EVT_CHOICE( ID_CHOICE_WXLAUNCHER_REMSHELL, WxLauncherGUI::remshellChoiceSelectedCB )

    EVT_BUTTON( ID_BUTTON_WXLAUNCHER_SHELLOPEN, WxLauncherGUI::shellOpenButtonClickCB )

    EVT_TEXT( ID_TEXTCTRL_WXLAUNCHER_CALCDIR, WxLauncherGUI::calculationDirectoryTextCtrlUpdateCB )

    EVT_TEXT( ID_TEXTCTRL_WXLAUNCHER_SCRATCHDIR, WxLauncherGUI::scratchDirectoryTextCtrlUpdateCB )

    EVT_BUTTON( ID_BUTTON_WXLAUNCHER_LAUNCH, WxLauncherGUI::launchButtonClickCB )

    EVT_MENU( ID_MENUITEM_WXLAUNCHER_MACHREGISTER, WxLauncherGUI::machRegisterMenuitemClickCB )

    EVT_MENU( ID_MENUITEM_WXLAUNCHER_STAGELAUNCH, WxLauncherGUI::stageLaunchMenuItemClickCB )

    EVT_MENU( ID_MENUITEM_WXLAUNCHER_FINISHLAUNCH, WxLauncherGUI::finishLaunchMenuItemClickCB )

    EVT_MENU( wxID_EXIT, WxLauncherGUI::quitMenuItemClickCB )

    EVT_MENU( wxID_HELP, WxLauncherGUI::helpLauncherMenuitemClickCB )

    EVT_MENU( ID_MENUITEM_WXLAUNCHER_HELPSUPPORT, WxLauncherGUI::helpSupportMenuitemClickCB )

////@end WxLauncherGUI event table entries

END_EVENT_TABLE()

/*!
 * WxLauncherGUI constructors
 */

WxLauncherGUI::WxLauncherGUI( )
{
}

WxLauncherGUI::WxLauncherGUI( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    Create( parent, id, caption, pos, size, style );
}

/*!
 * WxLauncherGUI creator
 */

bool WxLauncherGUI::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
////@begin WxLauncherGUI member initialisation
////@end WxLauncherGUI member initialisation

////@begin WxLauncherGUI creation
    SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
    ewxFrame::Create( parent, id, caption, pos, size, style );

    CreateControls();
    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
    Centre();
////@end WxLauncherGUI creation
    return true;
}

/*!
 * Control creation for WxLauncherGUI
 */

void WxLauncherGUI::CreateControls()
{    
////@begin WxLauncherGUI content construction
    WxLauncherGUI* itemFrame1 = this;

    ewxMenuBar* menuBar = new ewxMenuBar;
    wxMenu* itemMenu132 = new wxMenu;
    itemMenu132->Append(ID_MENUITEM_WXLAUNCHER_MACHREGISTER, _("Register Machines..."), _T(""), wxITEM_NORMAL);
    itemMenu132->AppendSeparator();
    itemMenu132->Append(ID_MENUITEM_WXLAUNCHER_STAGELAUNCH, _("Stage Launch"), _T(""), wxITEM_NORMAL);
    itemMenu132->Enable(ID_MENUITEM_WXLAUNCHER_STAGELAUNCH, false);
    itemMenu132->Append(ID_MENUITEM_WXLAUNCHER_FINISHLAUNCH, _("Finish Staged Launch"), _T(""), wxITEM_NORMAL);
    itemMenu132->Enable(ID_MENUITEM_WXLAUNCHER_FINISHLAUNCH, false);
    itemMenu132->AppendSeparator();
    itemMenu132->Append(wxID_EXIT, _("&Quit\tCtrl-q"), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu132, _("Job"));
    wxMenu* itemMenu139 = new wxMenu;
    itemMenu139->Append(wxID_HELP, _("on Launcher..."), _T(""), wxITEM_NORMAL);
    itemMenu139->Append(ID_MENUITEM_WXLAUNCHER_HELPSUPPORT, _("Support..."), _T(""), wxITEM_NORMAL);
    menuBar->Append(itemMenu139, _("Help"));
    itemFrame1->SetMenuBar(menuBar);

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
    itemFrame1->SetSizer(itemBoxSizer2);

    ewxPanel* itemPanel3 = new ewxPanel( itemFrame1, ID_PANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer2->Add(itemPanel3, 1, wxGROW, 5);

    wxBoxSizer* itemBoxSizer4 = new wxBoxSizer(wxVERTICAL);
    itemPanel3->SetSizer(itemBoxSizer4);

    ewxPanel* itemPanel5 = new ewxPanel( itemPanel3, ID_PANEL1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer4->Add(itemPanel5, 0, wxGROW|wxALL, 3);

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel5->SetSizer(itemBoxSizer6);

    ewxStaticText* itemStaticText7 = new ewxStaticText( itemPanel5, wxID_STATIC, _("Machine:"), wxDefaultPosition, wxSize(120, -1), wxALIGN_LEFT );
    itemBoxSizer6->Add(itemStaticText7, 0, wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText8 = new ewxStaticText( itemPanel5, wxID_STATIC, _T(""), wxDefaultPosition, wxSize(7, -1), 0 );
    itemBoxSizer6->Add(itemStaticText8, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    ewxPanel* itemPanel9 = new ewxPanel( itemPanel5, ID_PANEL_WXLAUNCHER_MACHINE, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer6->Add(itemPanel9, 1, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer10 = new wxFlexGridSizer(1, 2, 0, 3);
    itemFlexGridSizer10->AddGrowableCol(1);
    itemPanel9->SetSizer(itemFlexGridSizer10);

    wxString* itemChoice11Strings = NULL;
    ewxChoice* itemChoice11 = new ewxChoice( itemPanel9, ID_CHOICE_WXLAUNCHER_MACHINES, wxDefaultPosition, wxSize(120, -1), 0, itemChoice11Strings, 0 );
    itemFlexGridSizer10->Add(itemChoice11, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText12 = new ewxStaticText( itemPanel9, ID_STATIC_WXLAUNCHER_MACHINENAME, _("(Machine Full Name)"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    itemFlexGridSizer10->Add(itemStaticText12, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText13 = new ewxStaticText( itemPanel5, ID_STATIC_WXLAUNCHER_NOMACHINE, _("(None available)"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE );
    itemStaticText13->SetFont(wxFont(10, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemStaticText13->Show(false);
    itemBoxSizer6->Add(itemStaticText13, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 5);

    ewxPanel* itemPanel14 = new ewxPanel( itemPanel3, ID_PANEL_WXLAUNCHER_WRKSTSPCFC, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel14->Show(false);
    itemBoxSizer4->Add(itemPanel14, 0, wxGROW|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer15 = new wxFlexGridSizer(1, 2, 0, 3);
    itemFlexGridSizer15->AddGrowableCol(1);
    itemPanel14->SetSizer(itemFlexGridSizer15);

    ewxPanel* itemPanel16 = new ewxPanel( itemPanel14, ID_PANEL_WXLAUNCHER_WKSTNPROCS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer15->Add(itemPanel16, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer17 = new wxFlexGridSizer(1, 3, 0, 3);
    itemFlexGridSizer17->AddGrowableCol(2);
    itemPanel16->SetSizer(itemFlexGridSizer17);

    ewxStaticText* itemStaticText18 = new ewxStaticText( itemPanel16, wxID_STATIC, _("Processors:"), wxDefaultPosition, wxSize(120, -1), wxALIGN_LEFT );
    itemFlexGridSizer17->Add(itemStaticText18, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 3);

    ewxStaticText* itemStaticText19 = new ewxStaticText( itemPanel16, wxID_STATIC, _T(""), wxDefaultPosition, wxSize(7, -1), 0 );
    itemFlexGridSizer17->Add(itemStaticText19, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    WxParameterEdit* itemWxParameterEdit20 = new WxParameterEdit( itemPanel16, ID_PARAMEDIT_WXLAUNCHER_WKSTNPROCS, wxDefaultPosition, wxSize(100, -1), wxNO_BORDER|wxTAB_TRAVERSAL );
    itemWxParameterEdit20->setMaximum(100);
    itemWxParameterEdit20->setMinimum(1);
    itemWxParameterEdit20->setRangeVisible(1);
    itemWxParameterEdit20->setUnitsLabel(_T(""));
    itemWxParameterEdit20->setUnitsVisible(0);
    itemWxParameterEdit20->setValue(1);
    itemFlexGridSizer17->Add(itemWxParameterEdit20, 0, wxALIGN_LEFT|wxALIGN_BOTTOM, 3);

    ewxPanel* itemPanel21 = new ewxPanel( itemPanel14, ID_PANEL_WXLAUNCHER_PRIORITY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer15->Add(itemPanel21, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer22 = new wxFlexGridSizer(1, 3, 0, 3);
    itemFlexGridSizer22->AddGrowableCol(2);
    itemPanel21->SetSizer(itemFlexGridSizer22);

    ewxStaticText* itemStaticText23 = new ewxStaticText( itemPanel21, wxID_STATIC, _("Priority:"), wxDefaultPosition, wxSize(90, -1), wxALIGN_RIGHT );
    itemFlexGridSizer22->Add(itemStaticText23, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText24 = new ewxStaticText( itemPanel21, wxID_STATIC, _T(""), wxDefaultPosition, wxSize(7, -1), 0 );
    itemFlexGridSizer22->Add(itemStaticText24, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString* itemChoice25Strings = NULL;
    ewxChoice* itemChoice25 = new ewxChoice( itemPanel21, ID_CHOICE_WXLAUNCHER_PRIORITY, wxDefaultPosition, wxSize(100, -1), 0, itemChoice25Strings, 0 );
    itemFlexGridSizer22->Add(itemChoice25, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxPanel* itemPanel26 = new ewxPanel( itemPanel3, ID_PANEL_WXLAUNCHER_BATCHSPCFC, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel26->Show(false);
    itemBoxSizer4->Add(itemPanel26, 0, wxGROW|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer27 = new wxFlexGridSizer(4, 1, 0, 3);
    itemFlexGridSizer27->AddGrowableRow(0);
    itemFlexGridSizer27->AddGrowableRow(1);
    itemFlexGridSizer27->AddGrowableRow(2);
    itemFlexGridSizer27->AddGrowableRow(3);
    itemFlexGridSizer27->AddGrowableCol(0);
    itemPanel26->SetSizer(itemFlexGridSizer27);

    ewxPanel* itemPanel28 = new ewxPanel( itemPanel26, ID_PANEL_WXLAUNCHER_BATCH0, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer27->Add(itemPanel28, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);

    wxFlexGridSizer* itemFlexGridSizer29 = new wxFlexGridSizer(1, 2, 0, 0);
    itemFlexGridSizer29->AddGrowableRow(0);
    itemFlexGridSizer29->AddGrowableCol(0);
    itemFlexGridSizer29->AddGrowableCol(1);
    itemPanel28->SetSizer(itemFlexGridSizer29);

    ewxPanel* itemPanel30 = new ewxPanel( itemPanel28, ID_PANEL_WXLAUNCHER_ALLOCACCT, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer29->Add(itemPanel30, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer31 = new wxFlexGridSizer(1, 3, 3, 3);
    itemFlexGridSizer31->AddGrowableCol(2);
    itemPanel30->SetSizer(itemFlexGridSizer31);

    ewxStaticText* itemStaticText32 = new ewxStaticText( itemPanel30, wxID_STATIC, _("Alloc. Account:"), wxDefaultPosition, wxSize(120, -1), wxALIGN_LEFT );
    itemFlexGridSizer31->Add(itemStaticText32, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText33 = new ewxStaticText( itemPanel30, wxID_STATIC, _("*"), wxDefaultPosition, wxSize(7, -1), 0 );
    itemStaticText33->SetForegroundColour(wxColour(255, 0, 0));
    itemStaticText33->SetFont(wxFont(14, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemFlexGridSizer31->Add(itemStaticText33, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl34 = new ewxTextCtrl( itemPanel30, ID_TEXTCTRL_WXLAUNCHER_ALLOCACCT, _T(""), wxDefaultPosition, wxSize(100, -1), wxTE_PROCESS_ENTER );
    if (ShowToolTips())
        itemTextCtrl34->SetToolTip(_("(Required)\nAccount number to charge for the use of the computing resource."));
    itemFlexGridSizer31->Add(itemTextCtrl34, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxPanel* itemPanel35 = new ewxPanel( itemPanel28, ID_PANEL_WXLAUNCHER_QUEUE, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer29->Add(itemPanel35, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer36 = new wxFlexGridSizer(1, 3, 0, 3);
    itemFlexGridSizer36->AddGrowableCol(2);
    itemPanel35->SetSizer(itemFlexGridSizer36);

    wxBoxSizer* itemBoxSizer37 = new wxBoxSizer(wxHORIZONTAL);
    itemFlexGridSizer36->Add(itemBoxSizer37, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5);

    ewxStaticText* itemStaticText38 = new ewxStaticText( itemPanel35, ID_STATIC_WXLAUNCHER_QUEUELABELRIGHT, _("Queue:"), wxDefaultPosition, wxSize(90, -1), wxALIGN_RIGHT );
    itemBoxSizer37->Add(itemStaticText38, 0, wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText39 = new ewxStaticText( itemPanel35, ID_STATIC_WXLAUNCHER_QUEUELABELLEFT, _("Queue:"), wxDefaultPosition, wxSize(120, -1), wxALIGN_LEFT );
    itemStaticText39->Show(false);
    itemBoxSizer37->Add(itemStaticText39, 0, wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText40 = new ewxStaticText( itemPanel35, wxID_STATIC, _T(""), wxDefaultPosition, wxSize(7, -1), 0 );
    itemFlexGridSizer36->Add(itemStaticText40, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString* itemChoice41Strings = NULL;
    ewxChoice* itemChoice41 = new ewxChoice( itemPanel35, ID_CHOICE_WXLAUNCHER_QUEUE, wxDefaultPosition, wxSize(100, -1), 0, itemChoice41Strings, 0 );
    itemFlexGridSizer36->Add(itemChoice41, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxPanel* itemPanel42 = new ewxPanel( itemPanel26, ID_PANEL_WXLAUNCHER_WALLTIME, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer27->Add(itemPanel42, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer43 = new wxFlexGridSizer(1, 3, 0, 3);
    itemFlexGridSizer43->AddGrowableRow(0);
    itemFlexGridSizer43->AddGrowableCol(2);
    itemPanel42->SetSizer(itemFlexGridSizer43);

    wxBoxSizer* itemBoxSizer44 = new wxBoxSizer(wxVERTICAL);
    itemFlexGridSizer43->Add(itemBoxSizer44, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText45 = new ewxStaticText( itemPanel42, wxID_STATIC, _("Wall Time Limit:"), wxDefaultPosition, wxSize(120, -1), 0 );
    itemBoxSizer44->Add(itemStaticText45, 0, wxALIGN_CENTER_HORIZONTAL, 3);

    ewxPanel* itemPanel46 = new ewxPanel( itemPanel42, ID_PANEL_WXLAUNCHER_MAXWALLTIME, wxDefaultPosition, wxSize(120, -1), wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer44->Add(itemPanel46, 0, wxGROW, 3);

    wxBoxSizer* itemBoxSizer47 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel46->SetSizer(itemBoxSizer47);

    ewxStaticText* itemStaticText48 = new ewxStaticText( itemPanel46, wxID_STATIC, _("Max."), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemStaticText48->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemBoxSizer47->Add(itemStaticText48, 0, wxGROW|wxRIGHT, 3);

    ewxStaticText* itemStaticText49 = new ewxStaticText( itemPanel46, ID_STATIC_WXLAUNCHER_MAXWALLTIME, _("02d 14h 30m"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemStaticText49->SetFont(wxFont(8, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemBoxSizer47->Add(itemStaticText49, 1, wxGROW|wxLEFT, 3);

    ewxStaticText* itemStaticText50 = new ewxStaticText( itemPanel42, wxID_STATIC, _T(""), wxDefaultPosition, wxSize(5, -1), 0 );
    itemFlexGridSizer43->Add(itemStaticText50, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_TOP|wxADJUST_MINSIZE, 5);

    WxTimeSpanEdit* itemWxTimeSpanEdit51 = new WxTimeSpanEdit( itemPanel42, ID_TIMESPANEDIT_WXLAUNCHER_WALLTIME, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemWxTimeSpanEdit51->setMaxValue(4320);
    itemWxTimeSpanEdit51->setRangeEnforced(1);
    itemWxTimeSpanEdit51->setValue(1440);
    itemFlexGridSizer43->Add(itemWxTimeSpanEdit51, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxPanel* itemPanel52 = new ewxPanel( itemPanel26, ID_PANEL_WXLAUNCHER_BATCH2, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer27->Add(itemPanel52, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5);

    wxFlexGridSizer* itemFlexGridSizer53 = new wxFlexGridSizer(1, 3, 0, 3);
    itemFlexGridSizer53->AddGrowableRow(1);
    itemFlexGridSizer53->AddGrowableCol(0);
    itemFlexGridSizer53->AddGrowableCol(1);
    itemPanel52->SetSizer(itemFlexGridSizer53);

    ewxPanel* itemPanel54 = new ewxPanel( itemPanel52, ID_PANEL_WXLAUNCHER_BATCHPROCS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer53->Add(itemPanel54, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer55 = new wxFlexGridSizer(1, 4, 3, 3);
    itemFlexGridSizer55->AddGrowableCol(2);
    itemPanel54->SetSizer(itemFlexGridSizer55);

    ewxPanel* itemPanel56 = new ewxPanel( itemPanel54, ID_PANEL_WXLAUNCHER_BATCHPROCSLABELS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer55->Add(itemPanel56, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 3);

    wxBoxSizer* itemBoxSizer57 = new wxBoxSizer(wxVERTICAL);
    itemPanel56->SetSizer(itemBoxSizer57);

    ewxStaticText* itemStaticText58 = new ewxStaticText( itemPanel56, wxID_STATIC, _("Processors:"), wxDefaultPosition, wxSize(120, -1), 0 );
    itemBoxSizer57->Add(itemStaticText58, 0, wxALIGN_LEFT, 3);

    ewxPanel* itemPanel59 = new ewxPanel( itemPanel56, ID_PANEL_WXLAUNCHER_BATCHPROCSMAX, wxDefaultPosition, wxSize(120, -1), wxNO_BORDER );
    itemBoxSizer57->Add(itemPanel59, 0, wxGROW, 3);

    wxBoxSizer* itemBoxSizer60 = new wxBoxSizer(wxHORIZONTAL);
    itemPanel59->SetSizer(itemBoxSizer60);

    ewxStaticText* itemStaticText61 = new ewxStaticText( itemPanel59, wxID_STATIC, _("Max. per Node"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
    itemStaticText61->SetFont(wxFont(8, wxDEFAULT, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemBoxSizer60->Add(itemStaticText61, 0, wxGROW|wxRIGHT, 3);

    ewxStaticText* itemStaticText62 = new ewxStaticText( itemPanel59, ID_STATIC_WXLAUNCHER_BATCHPROCSMAX, _("1"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
    itemStaticText62->SetFont(wxFont(8, wxDEFAULT, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemBoxSizer60->Add(itemStaticText62, 1, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3);

    ewxStaticText* itemStaticText63 = new ewxStaticText( itemPanel54, wxID_STATIC, _T(""), wxDefaultPosition, wxSize(5, -1), 0 );
    itemFlexGridSizer55->Add(itemStaticText63, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_TOP|wxADJUST_MINSIZE, 5);

    WxParameterEdit* itemWxParameterEdit64 = new WxParameterEdit( itemPanel54, ID_PARAMEDIT_WXLAUNCHER_BATCHPROCS, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemWxParameterEdit64->setMaximum(100);
    itemWxParameterEdit64->setMinimum(1);
    itemWxParameterEdit64->setRangeVisible(1);
    itemWxParameterEdit64->setUnitsLabel(_T(""));
    itemWxParameterEdit64->setUnitsVisible(0);
    itemWxParameterEdit64->setValue(1);
    itemFlexGridSizer55->Add(itemWxParameterEdit64, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxPanel* itemPanel65 = new ewxPanel( itemPanel52, ID_PANEL_WXLAUNCHER_BATCHNODES, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer53->Add(itemPanel65, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer66 = new wxFlexGridSizer(1, 3, 3, 3);
    itemFlexGridSizer66->AddGrowableCol(2);
    itemPanel65->SetSizer(itemFlexGridSizer66);

    ewxStaticText* itemStaticText67 = new ewxStaticText( itemPanel65, wxID_STATIC, _("Nodes:"), wxDefaultPosition, wxSize(90, -1), wxALIGN_RIGHT );
    itemFlexGridSizer66->Add(itemStaticText67, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText68 = new ewxStaticText( itemPanel65, wxID_STATIC, _T(""), wxDefaultPosition, wxSize(7, -1), 0 );
    itemFlexGridSizer66->Add(itemStaticText68, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    WxParameterEdit* itemWxParameterEdit69 = new WxParameterEdit( itemPanel65, ID_PARAMEDIT_WXLAUNCHER_BATCHNODES, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemWxParameterEdit69->setMaximum(100);
    itemWxParameterEdit69->setMinimum(1);
    itemWxParameterEdit69->setRangeVisible(1);
    itemWxParameterEdit69->setUnitsLabel(_T(""));
    itemWxParameterEdit69->setUnitsVisible(0);
    itemWxParameterEdit69->setValue(1);
    if (ShowToolTips())
        itemWxParameterEdit69->SetToolTip(_("Number of physical nodes to use in the calculation.\n(At least equal to # processors divided by \"Max. procs per node\".)"));
    itemFlexGridSizer66->Add(itemWxParameterEdit69, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxPanel* itemPanel70 = new ewxPanel( itemPanel26, ID_PANEL_WXLAUNCHER_BATCH3, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer27->Add(itemPanel70, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5);

    wxFlexGridSizer* itemFlexGridSizer71 = new wxFlexGridSizer(1, 2, 3, 3);
    itemFlexGridSizer71->AddGrowableCol(0);
    itemFlexGridSizer71->AddGrowableCol(1);
    itemPanel70->SetSizer(itemFlexGridSizer71);

    ewxPanel* itemPanel72 = new ewxPanel( itemPanel70, ID_PANEL_WXLAUNCHER_MINSCRATCH, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer71->Add(itemPanel72, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer73 = new wxFlexGridSizer(1, 3, 3, 3);
    itemFlexGridSizer73->AddGrowableCol(2);
    itemPanel72->SetSizer(itemFlexGridSizer73);

    ewxStaticText* itemStaticText74 = new ewxStaticText( itemPanel72, wxID_STATIC, _("Scratch Space:"), wxDefaultPosition, wxSize(120, -1), wxALIGN_LEFT );
    itemFlexGridSizer73->Add(itemStaticText74, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText75 = new ewxStaticText( itemPanel72, wxID_STATIC, _T(""), wxDefaultPosition, wxSize(5, -1), 0 );
    itemFlexGridSizer73->Add(itemStaticText75, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    WxParameterEdit* itemWxParameterEdit76 = new WxParameterEdit( itemPanel72, ID_PARAMEDIT_WXLAUNCHER_MINSCRATCH, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemWxParameterEdit76->setMaximum(100);
    itemWxParameterEdit76->setMinimum(1);
    itemWxParameterEdit76->setRangeVisible(1);
    itemWxParameterEdit76->setUnitsLabel(_T(""));
    itemWxParameterEdit76->setUnitsVisible(0);
    itemWxParameterEdit76->setValue(1);
    itemFlexGridSizer73->Add(itemWxParameterEdit76, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxPanel* itemPanel77 = new ewxPanel( itemPanel70, ID_PANEL_WXLAUNCHER_MAXMEMORY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer71->Add(itemPanel77, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer78 = new wxFlexGridSizer(1, 3, 3, 3);
    itemFlexGridSizer78->AddGrowableCol(2);
    itemPanel77->SetSizer(itemFlexGridSizer78);

    ewxStaticText* itemStaticText79 = new ewxStaticText( itemPanel77, wxID_STATIC, _("Memory Limit:"), wxDefaultPosition, wxSize(90, -1), wxALIGN_RIGHT );
    itemFlexGridSizer78->Add(itemStaticText79, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText80 = new ewxStaticText( itemPanel77, wxID_STATIC, _T(""), wxDefaultPosition, wxSize(7, -1), 0 );
    itemFlexGridSizer78->Add(itemStaticText80, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    WxParameterEdit* itemWxParameterEdit81 = new WxParameterEdit( itemPanel77, ID_PARAMEDIT_WXLAUNCHER_MAXMEMORY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemWxParameterEdit81->setMaximum(100);
    itemWxParameterEdit81->setMinimum(1);
    itemWxParameterEdit81->setRangeVisible(1);
    itemWxParameterEdit81->setUnitsLabel(_T(""));
    itemWxParameterEdit81->setUnitsVisible(0);
    itemWxParameterEdit81->setValue(1);
    itemFlexGridSizer78->Add(itemWxParameterEdit81, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    wxBoxSizer* itemBoxSizer82 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer82, 0, wxGROW, 5);

    ewxStaticLine* itemStaticLine83 = new ewxStaticLine( itemPanel3, wxID_STATIC, wxDefaultPosition, wxSize(7, -1), wxLI_HORIZONTAL );
    itemBoxSizer82->Add(itemStaticLine83, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxTOP, 3);

    ewxStaticText* itemStaticText84 = new ewxStaticText( itemPanel3, wxID_STATIC, _("Remote Machine Access"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer82->Add(itemStaticText84, 0, wxALIGN_TOP|wxTOP, 3);

    ewxStaticLine* itemStaticLine85 = new ewxStaticLine( itemPanel3, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL|wxDOUBLE_BORDER );
    itemBoxSizer82->Add(itemStaticLine85, 1, wxGROW|wxLEFT|wxRIGHT|wxTOP, 3);

    ewxPanel* itemPanel86 = new ewxPanel( itemPanel3, ID_PANEL_WXLAUNCHER_6, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer4->Add(itemPanel86, 0, wxGROW, 3);

    wxFlexGridSizer* itemFlexGridSizer87 = new wxFlexGridSizer(3, 1, 0, 3);
    itemFlexGridSizer87->AddGrowableRow(0);
    itemFlexGridSizer87->AddGrowableRow(1);
    itemFlexGridSizer87->AddGrowableRow(2);
    itemFlexGridSizer87->AddGrowableCol(0);
    itemPanel86->SetSizer(itemFlexGridSizer87);

    wxFlexGridSizer* itemFlexGridSizer88 = new wxFlexGridSizer(2, 2, 0, 3);
    itemFlexGridSizer88->AddGrowableRow(1);
    itemFlexGridSizer88->AddGrowableCol(1);
    itemFlexGridSizer87->Add(itemFlexGridSizer88, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5);

    ewxPanel* itemPanel89 = new ewxPanel( itemPanel86, ID_PANEL_WXLAUNCHER_USERNAME, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer88->Add(itemPanel89, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer90 = new wxFlexGridSizer(1, 3, 0, 3);
    itemFlexGridSizer90->AddGrowableCol(2);
    itemPanel89->SetSizer(itemFlexGridSizer90);

    ewxStaticText* itemStaticText91 = new ewxStaticText( itemPanel89, ID_STATIC_WXLAUNCHER_USERNAMELABEL, _("Username:"), wxDefaultPosition, wxSize(120, -1), wxALIGN_LEFT );
    itemFlexGridSizer90->Add(itemStaticText91, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText92 = new ewxStaticText( itemPanel89, wxID_STATIC, _("*"), wxDefaultPosition, wxSize(7, -1), 0 );
    itemStaticText92->SetForegroundColour(wxColour(255, 0, 0));
    itemStaticText92->SetFont(wxFont(14, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemFlexGridSizer90->Add(itemStaticText92, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl93 = new ewxTextCtrl( itemPanel89, ID_TEXTCTRL_WXLAUNCHER_USERNAME, _T(""), wxDefaultPosition, wxSize(100, -1), wxTE_PROCESS_ENTER );
    itemFlexGridSizer90->Add(itemTextCtrl93, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxPanel* itemPanel94 = new ewxPanel( itemPanel86, ID_PANEL_WXLAUNCHER_PASSWORD1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer88->Add(itemPanel94, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer95 = new wxFlexGridSizer(1, 3, 0, 3);
    itemFlexGridSizer95->AddGrowableCol(2);
    itemPanel94->SetSizer(itemFlexGridSizer95);

    ewxStaticText* itemStaticText96 = new ewxStaticText( itemPanel94, ID_STATIC_WXLAUNCHER_PASSWORD1LABEL, _("Password:"), wxDefaultPosition, wxSize(90, -1), wxALIGN_RIGHT );
    itemFlexGridSizer95->Add(itemStaticText96, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText97 = new ewxStaticText( itemPanel94, wxID_STATIC, _("*"), wxDefaultPosition, wxSize(7, -1), 0 );
    itemStaticText97->SetForegroundColour(wxColour(255, 0, 0));
    itemStaticText97->SetFont(wxFont(14, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemFlexGridSizer95->Add(itemStaticText97, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl98 = new ewxTextCtrl( itemPanel94, ID_TEXTCTRL_WXLAUNCHER_PASSWORD1, _T(""), wxDefaultPosition, wxSize(100, -1), wxTE_PROCESS_ENTER|wxTE_PASSWORD );
    itemFlexGridSizer95->Add(itemTextCtrl98, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    itemFlexGridSizer88->Add(1, 1, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxPanel* itemPanel100 = new ewxPanel( itemPanel86, ID_PANEL_WXLAUNCHER_PASSWORD2, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer88->Add(itemPanel100, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer101 = new wxFlexGridSizer(1, 3, 3, 3);
    itemFlexGridSizer101->AddGrowableCol(2);
    itemPanel100->SetSizer(itemFlexGridSizer101);

    ewxStaticText* itemStaticText102 = new ewxStaticText( itemPanel100, ID_STATIC_WXLAUNCHER_PASSWORD2LABEL, _("Globus Pass:"), wxDefaultPosition, wxSize(90, -1), wxALIGN_RIGHT );
    itemFlexGridSizer101->Add(itemStaticText102, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText103 = new ewxStaticText( itemPanel100, wxID_STATIC, _("*"), wxDefaultPosition, wxSize(7, -1), 0 );
    itemStaticText103->SetForegroundColour(wxColour(255, 0, 0));
    itemStaticText103->SetFont(wxFont(14, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemFlexGridSizer101->Add(itemStaticText103, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl104 = new ewxTextCtrl( itemPanel100, ID_TEXTCTRL_WXLAUNCHER_PASSWORD2, _T(""), wxDefaultPosition, wxSize(100, -1), wxTE_PROCESS_ENTER|wxTE_PASSWORD );
    if (ShowToolTips())
        itemTextCtrl104->SetToolTip(_("(Required)\nGlobus authentication passphrase"));
    itemFlexGridSizer101->Add(itemTextCtrl104, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    wxFlexGridSizer* itemFlexGridSizer105 = new wxFlexGridSizer(1, 2, 0, 3);
    itemFlexGridSizer105->AddGrowableCol(1);
    itemFlexGridSizer87->Add(itemFlexGridSizer105, 0, wxGROW|wxALIGN_CENTER_VERTICAL, 5);

    ewxPanel* itemPanel106 = new ewxPanel( itemPanel86, ID_PANEL_WXLAUNCHER_REMSHELL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemFlexGridSizer105->Add(itemPanel106, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer107 = new wxFlexGridSizer(1, 3, 3, 3);
    itemFlexGridSizer107->AddGrowableCol(3);
    itemPanel106->SetSizer(itemFlexGridSizer107);

    ewxStaticText* itemStaticText108 = new ewxStaticText( itemPanel106, wxID_STATIC, _("Remote Shell:"), wxDefaultPosition, wxSize(120, -1), wxALIGN_LEFT );
    itemFlexGridSizer107->Add(itemStaticText108, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText109 = new ewxStaticText( itemPanel106, wxID_STATIC, _T(""), wxDefaultPosition, wxSize(7, -1), 0 );
    itemFlexGridSizer107->Add(itemStaticText109, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    wxString* itemChoice110Strings = NULL;
    ewxChoice* itemChoice110 = new ewxChoice( itemPanel106, ID_CHOICE_WXLAUNCHER_REMSHELL, wxDefaultPosition, wxSize(120, -1), 0, itemChoice110Strings, 0 );
    itemFlexGridSizer107->Add(itemChoice110, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxButton* itemButton111 = new ewxButton( itemPanel86, ID_BUTTON_WXLAUNCHER_SHELLOPEN, _("Open..."), wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS );
    if (ShowToolTips())
        itemButton111->SetToolTip(_("Open Remote Shell\nTest remote access and/or find appropriate run and scratch directories"));
    itemFlexGridSizer105->Add(itemButton111, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 3);

    ewxPanel* itemPanel112 = new ewxPanel( itemPanel3, ID_PANEL_WXLAUNCHER_DIRECTORIES, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemBoxSizer4->Add(itemPanel112, 0, wxGROW|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer113 = new wxFlexGridSizer(2, 1, 0, 3);
    itemFlexGridSizer113->AddGrowableCol(0);
    itemPanel112->SetSizer(itemFlexGridSizer113);

    ewxPanel* itemPanel114 = new ewxPanel( itemPanel112, ID_PANEL_WXLAUNCHER_CALCDIR, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel114->Show(false);
    itemFlexGridSizer113->Add(itemPanel114, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer115 = new wxFlexGridSizer(2, 3, 3, 3);
    itemFlexGridSizer115->AddGrowableCol(2);
    itemPanel114->SetSizer(itemFlexGridSizer115);

    ewxStaticText* itemStaticText116 = new ewxStaticText( itemPanel114, wxID_STATIC, _("Run Directory:"), wxDefaultPosition, wxSize(120, -1), wxALIGN_LEFT );
    itemFlexGridSizer115->Add(itemStaticText116, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText117 = new ewxStaticText( itemPanel114, wxID_STATIC, _("*"), wxDefaultPosition, wxSize(7, -1), 0 );
    itemStaticText117->SetForegroundColour(wxColour(255, 0, 0));
    itemStaticText117->SetFont(wxFont(14, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemFlexGridSizer115->Add(itemStaticText117, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl118 = new ewxTextCtrl( itemPanel114, ID_TEXTCTRL_WXLAUNCHER_CALCDIR, _T(""), wxDefaultPosition, wxSize(350, -1), wxTE_PROCESS_ENTER );
    itemFlexGridSizer115->Add(itemTextCtrl118, 1, wxGROW|wxALIGN_CENTER_VERTICAL, 3);

    ewxPanel* itemPanel119 = new ewxPanel( itemPanel112, ID_PANEL_WXLAUNCHER_SCRATCHDIR, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
    itemPanel119->Show(false);
    itemFlexGridSizer113->Add(itemPanel119, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 3);

    wxFlexGridSizer* itemFlexGridSizer120 = new wxFlexGridSizer(2, 3, 3, 3);
    itemFlexGridSizer120->AddGrowableCol(2);
    itemPanel119->SetSizer(itemFlexGridSizer120);

    ewxStaticText* itemStaticText121 = new ewxStaticText( itemPanel119, wxID_STATIC, _("Scratch Directory:"), wxDefaultPosition, wxSize(120, -1), wxALIGN_LEFT );
    itemFlexGridSizer120->Add(itemStaticText121, 0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 3);

    ewxStaticText* itemStaticText122 = new ewxStaticText( itemPanel119, wxID_STATIC, _T(""), wxDefaultPosition, wxSize(7, -1), 0 );
    itemFlexGridSizer120->Add(itemStaticText122, 0, wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);

    ewxTextCtrl* itemTextCtrl123 = new ewxTextCtrl( itemPanel119, ID_TEXTCTRL_WXLAUNCHER_SCRATCHDIR, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    if (ShowToolTips())
        itemTextCtrl123->SetToolTip(_("(Optional)\nTemporary storage for large intermediate calculation files (defaults to run directory if not specified)."));
    itemFlexGridSizer120->Add(itemTextCtrl123, 1, wxGROW|wxALIGN_CENTER_VERTICAL, 3);

    wxBoxSizer* itemBoxSizer124 = new wxBoxSizer(wxHORIZONTAL);
    itemBoxSizer4->Add(itemBoxSizer124, 0, wxGROW, 5);

    itemBoxSizer124->Add(5, 5, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticText* itemStaticText126 = new ewxStaticText( itemPanel3, ID_STATICTEXT, _("*"), wxDefaultPosition, wxDefaultSize, 0 );
    itemStaticText126->SetForegroundColour(wxColour(255, 0, 0));
    itemStaticText126->SetFont(wxFont(14, wxSWISS, wxNORMAL, wxNORMAL, false, _T("Sans")));
    itemBoxSizer124->Add(itemStaticText126, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxStaticText* itemStaticText127 = new ewxStaticText( itemPanel3, ID_STATIC_WXLAUNCHER_REQUIREDFIELDSKEY, _("Required value"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer124->Add(itemStaticText127, 1, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 3);

    ewxButton* itemButton128 = new ewxButton( itemPanel3, ID_BUTTON_WXLAUNCHER_LAUNCH, _("Launch"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer124->Add(itemButton128, 0, wxALIGN_CENTER_VERTICAL|wxALL, 3);

    ewxStaticLine* itemStaticLine129 = new ewxStaticLine( itemPanel3, wxID_STATIC, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    itemBoxSizer4->Add(itemStaticLine129, 0, wxGROW|wxTOP|wxBOTTOM, 3);

    WxFeedback* itemWxFeedback130 = new WxFeedback( itemPanel3, ID_FEEDBACK_WXLAUNCHER_MESSAGES, wxDefaultPosition, wxDefaultSize, wxNO_BORDER );
    itemBoxSizer4->Add(itemWxFeedback130, 1, wxGROW, 3);

////@end WxLauncherGUI content construction
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_FRAME_WXLAUNCHER
 */

void WxLauncherGUI::mainWindowCloseCB( wxCloseEvent& event )
{
////@begin wxEVT_CLOSE_WINDOW event handler for ID_FRAME_WXLAUNCHER in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_CLOSE_WINDOW event handler for ID_FRAME_WXLAUNCHER in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_MACHINES
 */

void WxLauncherGUI::machinesChoiceSelectedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_MACHINES in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_MACHINES in WxLauncherGUI. 
}

/*!
 * ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_WKSTNPROCS
 */

void WxLauncherGUI::wkstnProcsParamValueChangedCB( wxCommandEvent& event )
{
////@begin ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_WKSTNPROCS in WxLauncherGUI.
    // Before editing this code, remove the block markers.
   event.Skip();
////@end ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_WKSTNPROCS in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_PRIORITY
 */

void WxLauncherGUI::priorityChoiceSelectedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_PRIORITY in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_PRIORITY in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_ALLOCACCT
 */

void WxLauncherGUI::allocationAccountTextCtrlUpdateCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_ALLOCACCT in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_ALLOCACCT in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_QUEUE
 */

void WxLauncherGUI::queueChoiceSelectedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_QUEUE in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_QUEUE in WxLauncherGUI. 
}

/*!
 * ewxEVT_TIMESPAN_VALUE_CHANGED event handler for ID_TIMESPANEDIT_WXLAUNCHER_WALLTIME
 */

void WxLauncherGUI::wallTimeLimitChangedCB( wxCommandEvent& event )
{
////@begin ewxEVT_TIMESPAN_VALUE_CHANGED event handler for ID_TIMESPANEDIT_WXLAUNCHER_WALLTIME in WxLauncherGUI.
    //  Before editing this code, remove the block markers.
    event.Skip();
////@end ewxEVT_TIMESPAN_VALUE_CHANGED event handler for ID_TIMESPANEDIT_WXLAUNCHER_WALLTIME in WxLauncherGUI. 
}

/*!
 * ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_BATCHPROCS
 */

void WxLauncherGUI::batchProcsParamValueChangedCB( wxCommandEvent& event )
{
////@begin ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_BATCHPROCS in WxLauncherGUI.
    // Before editing this code, remove the block markers.
   event.Skip();
////@end ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_BATCHPROCS in WxLauncherGUI. 
}

/*!
 * ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_BATCHNODES
 */

void WxLauncherGUI::batchNodesParamValueChangedCB( wxCommandEvent& event )
{
////@begin ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_BATCHNODES in WxLauncherGUI.
    // Before editing this code, remove the block markers.
   event.Skip();
////@end ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_BATCHNODES in WxLauncherGUI. 
}

/*!
 * ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_MINSCRATCH
 */

void WxLauncherGUI::minScratchParamValueChangedCB( wxCommandEvent& event )
{
////@begin ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_MINSCRATCH in WxLauncherGUI.
    // Before editing this code, remove the block markers.
   event.Skip();
////@end ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_MINSCRATCH in WxLauncherGUI. 
}

/*!
 * ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_MAXMEMORY
 */

void WxLauncherGUI::maxMemoryParamValueChangedCB( wxCommandEvent& event )
{
////@begin ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_MAXMEMORY in WxLauncherGUI.
    // Before editing this code, remove the block markers.
   event.Skip();
////@end ewxEVT_PARAMETER_VALUE_CHANGED event handler for ID_PARAMEDIT_WXLAUNCHER_MAXMEMORY in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_USERNAME
 */

void WxLauncherGUI::usernameTextCtrlUpdateCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_USERNAME in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_USERNAME in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_PASSWORD1
 */

void WxLauncherGUI::password1TextCtrlUpdateCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_PASSWORD1 in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_PASSWORD1 in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_PASSWORD2
 */

void WxLauncherGUI::password2TextCtrlUpdateCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_PASSWORD2 in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_PASSWORD2 in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_REMSHELL
 */

void WxLauncherGUI::remshellChoiceSelectedCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_REMSHELL in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_CHOICE_WXLAUNCHER_REMSHELL in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXLAUNCHER_SHELLOPEN
 */

void WxLauncherGUI::shellOpenButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXLAUNCHER_SHELLOPEN in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXLAUNCHER_SHELLOPEN in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_CALCDIR
 */

void WxLauncherGUI::calculationDirectoryTextCtrlUpdateCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_CALCDIR in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_CALCDIR in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_SCRATCHDIR
 */

void WxLauncherGUI::scratchDirectoryTextCtrlUpdateCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_SCRATCHDIR in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_WXLAUNCHER_SCRATCHDIR in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXLAUNCHER_LAUNCH
 */

void WxLauncherGUI::launchButtonClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXLAUNCHER_LAUNCH in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_WXLAUNCHER_LAUNCH in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_MACHREGISTER
 */

void WxLauncherGUI::machRegisterMenuitemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_MACHREGISTER in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_MACHREGISTER in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_STAGELAUNCH
 */

void WxLauncherGUI::stageLaunchMenuItemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_STAGELAUNCH in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_STAGELAUNCH in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_FINISHLAUNCH
 */

void WxLauncherGUI::finishLaunchMenuItemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_FINISHLAUNCH in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_FINISHLAUNCH in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT
 */

void WxLauncherGUI::quitMenuItemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_EXIT in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP
 */

void WxLauncherGUI::helpLauncherMenuitemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for wxID_HELP in WxLauncherGUI. 
}

/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_HELPSUPPORT
 */

void WxLauncherGUI::helpSupportMenuitemClickCB( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_HELPSUPPORT in WxLauncherGUI.
    // Before editing this code, remove the block markers.
    event.Skip();
////@end wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUITEM_WXLAUNCHER_HELPSUPPORT in WxLauncherGUI. 
}

/*!
 * Should we show tooltips?
 */

bool WxLauncherGUI::ShowToolTips()
{
    return true;
}

/*!
 * Get bitmap resources
 */

wxBitmap WxLauncherGUI::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WxLauncherGUI bitmap retrieval
    wxUnusedVar(name);
    return wxNullBitmap;
////@end WxLauncherGUI bitmap retrieval
}

/*!
 * Get icon resources
 */

wxIcon WxLauncherGUI::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WxLauncherGUI icon retrieval
    wxUnusedVar(name);
    return wxNullIcon;
////@end WxLauncherGUI icon retrieval
}

/**
 * @file
 *
 * The Launcher window or tab around a LaunchPanel (LaunchPanel.C).
 */
#include "util/LiveCount.H"
#include "wx/wxprec.h"
#include "wx/wx.h"

#include "wxgui/ewxApp.H"
#include "wxgui/ewxMenuBar.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/ToolTabs.H"
#include "WxLauncher.H"

IMPLEMENT_CLASS(WxLauncher, ewxFrame)

WxLauncher::WxLauncher(wxWindow* parent)
{
  LiveCount::add("WxLauncher", 1);
    SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
    ewxFrame::Create(parent, WxLauncherGUI::ID_FRAME_WXLAUNCHER,
                     SYMBOL_WXLAUNCHERGUI_TITLE, wxDefaultPosition,
                     wxDefaultSize, SYMBOL_WXLAUNCHERGUI_STYLE);

    ewxMenuBar* menuBar = new ewxMenuBar;
    wxMenu* job = new wxMenu;
    job->Append(WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_MACHREGISTER, _("Register Machines..."));
    job->AppendSeparator();
    job->Append(WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_STAGELAUNCH, _("Stage Launch"));
    job->Enable(WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_STAGELAUNCH, false);
    job->Append(WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_FINISHLAUNCH, _("Finish Staged Launch"));
    job->Enable(WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_FINISHLAUNCH, false);
    job->AppendSeparator();
    job->Append(wxID_EXIT, _("&Quit\tCtrl-q"));
    menuBar->Append(job, _("Job"));
    wxMenu* help = new wxMenu;
    help->Append(wxID_HELP, _("on Launcher..."));
    help->Append(ewxApp::ID_REPORT_PROBLEM, _("Report a Problem..."));
    help->Append(wxID_ABOUT, _("About ECCE..."));
    menuBar->Append(help, _("Help"));
    SetMenuBar(menuBar);

    p_panel = new LaunchPanel(this, this);
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(p_panel, 1, wxEXPAND);
    SetSizer(sizer);
    sizer->Fit(this);
    sizer->SetSizeHints(this);
    Centre();

    Bind(wxEVT_CLOSE_WINDOW, &WxLauncher::closeCB, this);
    Bind(wxEVT_MENU, [this](wxCommandEvent& e) { p_panel->machRegisterMenuitemClickCB(e); },
         WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_MACHREGISTER);
    Bind(wxEVT_MENU, [this](wxCommandEvent& e) { p_panel->stageLaunchMenuItemClickCB(e); },
         WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_STAGELAUNCH);
    Bind(wxEVT_MENU, [this](wxCommandEvent& e) { p_panel->finishLaunchMenuItemClickCB(e); },
         WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_FINISHLAUNCH);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(); }, wxID_EXIT);
    Bind(wxEVT_MENU, [this](wxCommandEvent& e) { p_panel->helpLauncherMenuitemClickCB(e); },
         wxID_HELP);
    Bind(wxEVT_MENU, [this](wxCommandEvent& e) { p_panel->helpSupportMenuitemClickCB(e); },
         WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_HELPSUPPORT);

    ewxWindowUtils::setToolIcon(this, LAUNCHER);
}


WxLauncher::~WxLauncher()
{
  LiveCount::add("WxLauncher", -1);
    ToolTabs::release(this, false);
    delete p_msgs;
    if (p_toolPrefs != NULL)
    {
        saveSettings();
        delete p_toolPrefs;
    }
}


void WxLauncher::loadSettings(const string& fname)
{
    p_toolPrefs = new Preferences(fname);
    int x = -1, y = -1;
    if (p_toolPrefs->isDefined("LauncherFrame.X"))
        p_toolPrefs->getInt("LauncherFrame.X", x);
    if (p_toolPrefs->isDefined("LauncherFrame.Y"))
        p_toolPrefs->getInt("LauncherFrame.Y", y);
    Move(x, y);
}


void WxLauncher::saveSettings()
{
    wxRect r = GetRect();
    p_toolPrefs->setInt("LauncherFrame.X", r.GetLeft());
    p_toolPrefs->setInt("LauncherFrame.Y", r.GetTop());
    p_toolPrefs->saveFile();
}


void WxLauncher::closeCB(wxCloseEvent&)
{
    p_panel->applyEdits();
    Destroy();
}


void WxLauncher::deferMessages(bool on)
{
    if (p_msgs) p_msgs->deferMessages(on);
}


void WxLauncher::relayout()
{
    if (ToolTabs::layout(this))
        return;
    GetSizer()->SetSizeHints(this);
}


void WxLauncher::launchAllowed(bool launch, bool finish)
{
    wxMenuBar *bar = GetMenuBar();
    if (!bar) return;
    bar->Enable(WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_STAGELAUNCH, launch);
    bar->Enable(WxLauncherGUI::ID_MENUITEM_WXLAUNCHER_FINISHLAUNCH, finish);
}


bool WxLauncher::Show(bool show)
{
    if (ToolTabs::hosts(this)) {
        if (show) ToolTabs::select(this);
        return true;
    }
    return ewxFrame::Show(show);
}


void WxLauncher::Raise()
{
    if (ToolTabs::hosts(this)) ToolTabs::select(this);
    else ewxFrame::Raise();
}


void WxLauncher::SetTitle(const wxString& title)
{
    ewxFrame::SetTitle(title);
    ToolTabs::setTitle(this, title);
}


bool WxLauncher::Destroy()
{
    ToolTabs::release(this, false);
    return ewxFrame::Destroy();
}


// Iconize(false) on the hidden frame would map it.
void WxLauncher::Iconize(bool iconize)
{
    if (!ToolTabs::hosts(this)) ewxFrame::Iconize(iconize);
}


bool WxLauncher::Layout()
{
    return ToolTabs::layout(this) ? true : ewxFrame::Layout();
}


wxMenuBar *WxLauncher::GetMenuBar() const
{
    return ToolTabs::menuBar(this, wxFrame::GetMenuBar());
}


wxWindow *WxLauncher::FindWindow(long id) const
{
    wxWindow *page = ToolTabs::client(this);
    return page ? page->FindWindow(id) : wxFrame::FindWindow(id);
}

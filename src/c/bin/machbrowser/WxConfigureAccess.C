/**
 * @file
 * @author Ken Swanson
 *
 *  Small dialogue for specifying the communication settings (shell, username,
 *  and password(s)) for a machine by a specific user.
 *
 */


#include "wx/event.h"
#include "wx/wx.h"

#include "util/BrowserHelp.H"
#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/EcceSortedVector.H"
#include "util/ResourceUtils.H"
#include "util/SFile.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"
#include "util/STLUtil.H"
#include "util/TypedFile.H"
#include "util/TempStorage.H"

#include "tdat/QueueMgr.H"
#include "tdat/RefMachine.H"
#include "tdat/AuthCache.H"

#include "dsm/MachinePreferences.H"
#include "dsm/NWChemMDModel.H"
#include "dsm/CodeFactory.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/JCode.H"
#include "dsm/MdTask.H"

#include "comm/RCommand.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"

#include "WxConfigureAccess.H"

IMPLEMENT_CLASS(WxConfigureAccess, WxConfigureAccessGUI)

WxConfigureAccess::WxConfigureAccess(wxWindow* parent,
                                     wxWindowID id,
                                     const wxString& caption,
                                     const wxPoint& pos,
                                     const wxSize& size,
                                     long style )
                  : WxConfigureAccessGUI(parent, id, caption, pos, size, style)
{
    p_remoteShellChoice =     (ewxChoice *)(FindWindowById(ID_CHOICE_WXCONFIGUREACCESS_REMOTESHELL));
    p_remoteShellPanel  =      (ewxPanel *)(FindWindowById(ID_PANEL_WXCONFIGUREACCESS_REMOTESHELL));

    p_usernameTextCtrl  =   (ewxTextCtrl *)(FindWindowById(ID_TEXTCTRL_WXCONFIGUREACCESS_USERNAME));
    p_usernamePanel     =      (ewxPanel *)(FindWindowById(ID_PANEL_WXCONFIGUREACCESS_USERNAME));

    p_password1Label    = (ewxStaticText *)(FindWindowById(ID_STATIC_WXCONFIGUREACCESS_PASSWORD1));
    p_password1TextCtrl =   (ewxTextCtrl *)(FindWindowById(ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD1));
    p_password1Panel    =      (ewxPanel *)(FindWindowById(ID_PANEL_WXCONFIGUREACCESS_PASSWORD1));

    p_password2Label    = (ewxStaticText *)(FindWindowById(ID_STATIC_WXCONFIGUREACCESS_PASSWORD2));
    p_password2TextCtrl =   (ewxTextCtrl *)(FindWindowById(ID_TEXTCTRL_WXCONFIGUREACCESS_PASSWORD2));
    p_password2Panel    =      (ewxPanel *)(FindWindowById(ID_PANEL_WXCONFIGUREACCESS_PASSWORD2));

    p_okayButton        =     (ewxButton *)(FindWindowById(ID_BUTTON_WXCONFIGUREACCESS_OKAY));
    p_cancelButton      =     (ewxButton *)(FindWindowById(ID_BUTTON_WXCONFIGUREACCESS_CANCEL));

    this->Layout();
    this->Fit();
}


void WxConfigureAccess::showConfigure(MachinePreferences *prefs, bool *cncld)
{
    p_slctPrefs = prefs;
    this->updateControls();

    p_okayButton->Enable(false);

    int code = this->ShowModal();
    *cncld = (code == 1);

    if (!(*cncld))
    {
        int slctIdx = p_remoteShellChoice->GetSelection();
        string slctShell = "";

        if (slctIdx >= 0)
            slctShell = p_remoteShellChoice->GetString(slctIdx);

        p_slctPrefs->setRemoteShell(slctShell);

        if (p_slctPrefs->isOptionSupported("UN"))
        {
            string ptext = (string)p_usernameTextCtrl->GetValue();

            if ((STLUtil::trim(ptext)).empty())
                p_slctPrefs->setUsername(Ecce::realUser());
            else
                p_slctPrefs->setUsername(ptext);
        }

    }
}


void WxConfigureAccess::usernameTextCtrlUpdatedCB( wxCommandEvent& event )
{
    if (!p_inCtrlUpdate)
    {
        p_okayButton->Enable(true);
    }
}


void WxConfigureAccess::password1TextCtrlUpdatedCB( wxCommandEvent& event )
{
    if (!p_inCtrlUpdate)
    {
        p_okayButton->Enable(true);
    }
}


void WxConfigureAccess::password2TextCtrlUpdatedCB( wxCommandEvent& event )
{
    if (!p_inCtrlUpdate)
    {
        p_okayButton->Enable(true);
    }
}


void WxConfigureAccess::okayButtonClickCB( wxCommandEvent& event )
{
    p_cncld = false;
    this->EndModal(0);
}


void WxConfigureAccess::cancelButtonClickCB( wxCommandEvent& event )
{
    p_cncld = true;
    this->EndModal(1);
}


void WxConfigureAccess::updateControls()
{
    p_inCtrlUpdate = true;

    RefMachine *machRgstn = p_slctPrefs->getRegisteredMachine();

    this->SetTitle("Setup Remote Access -- " + machRgstn->refname());
    string shellName = this->refreshShells(machRgstn);
    this->refreshAuthentication(machRgstn, shellName);

    p_inCtrlUpdate = false;

    this->Layout();
    this->Fit();
}


string WxConfigureAccess::refreshShells(RefMachine *machRgstn)
{
    int slctIdx = wxNOT_FOUND;
    string slctShell = "";

    p_remoteShellChoice->Clear();
    vector<string>* machShells = machRgstn->remshells();

    if ((machShells != NULL) && (machShells->size() > 0))
    {
        for (int i = 0; i < machShells->size(); i++)
        {
            p_remoteShellChoice->Append((*machShells)[i]);
        }

        slctIdx = p_remoteShellChoice->FindString(p_slctPrefs->getRemoteShell());
        if (slctIdx < 0) slctIdx = 0;

        p_remoteShellChoice->SetSelection(slctIdx);
        slctShell = p_remoteShellChoice->GetString(slctIdx);
    }

    return slctShell;
}


void WxConfigureAccess::refreshAuthentication(RefMachine *machRgstn, string shellName)
{
    string uname = "";
    bool remoteGlobus = false;
    bool supported = p_slctPrefs->isOptionSupported("UN");

    if (supported)
    {
        uname = p_slctPrefs->getUsername();

        if (!shellName.empty())
            remoteGlobus = this->checkRemoteGlobus(machRgstn, shellName);
    }

    p_usernameTextCtrl->SetValue(uname);
    p_usernameTextCtrl->Enable(!remoteGlobus);
    p_usernamePanel->Show(supported);

    this->refreshPasswords(remoteGlobus, shellName,
                           machRgstn->fullname(), uname);
}


void WxConfigureAccess::refreshPasswords(const bool& remoteGlobus,
                        const string& shellName, const string& machName,
                        const string& user)
{
    bool supported = p_slctPrefs->isOptionSupported("PW");
    bool pswd2Rqrd = false;

    if (supported)
    {
        string pswd1 = "";

        string url = shellName + "://" + machName;
        BasicAuth *ba = AuthCache::getCache().getAuthentication(url, user,"",1);
        if (ba != NULL) {
          pswd1 = ba->m_pass;
          delete ba;
        }

        p_password1TextCtrl->SetValue(pswd1);

        //  UI now indicates required fields with asterisk.
        //  Make sure they are included in the labels for the password fields.
        if (remoteGlobus)
        {
            pswd2Rqrd = checkGlobusSSH(remoteGlobus, shellName);
            p_password1Label->SetLabel(pswd2Rqrd ? "ssh Password:": "Pass Phrase:");
        }
        else
            p_password1Label->SetLabel("Password:");

        if (pswd2Rqrd)
        {
            string pswd2 = "";

            string url = "ssh://" + machName;
            ba = AuthCache::getCache().getAuthentication(url, user, "", 1);
            if (ba != NULL) {
              pswd2 = ba->m_pass;
              delete ba;
            }

            p_password2TextCtrl->SetValue(pswd2);
        }
    }

    p_password1Panel->Show(supported);
    p_password2Panel->Show(supported && pswd2Rqrd);
}


void WxConfigureAccess::remoteShellChoiceSelectedCB(wxCommandEvent& event)
{
    if (!p_inCtrlUpdate)
    {
        //  Again, just enable/disable according to current options
        string slctShell = (string)p_remoteShellChoice->GetStringSelection();
        RefMachine *machRgstn = p_slctPrefs->getRegisteredMachine();
        this->refreshAuthentication(machRgstn, slctShell);

        p_okayButton->Enable(true);

        this->Layout();
        this->Fit();
    }
}


bool WxConfigureAccess::checkRemoteGlobus(RefMachine *machRgstn,
                                          string shellName)
{
    string machName = machRgstn->fullname();
    bool isGlobus = (shellName.find("Globus") != string::npos);
    bool isRemote = RCommand::isRemote(machName, shellName);

    return (isGlobus && isRemote);
}


bool WxConfigureAccess::checkGlobusSSH(RefMachine *machRgstn, string shellName)
{
    bool remoteGlobus = this->checkRemoteGlobus(machRgstn, shellName);

    return (this->checkGlobusSSH(remoteGlobus, shellName));
}


bool WxConfigureAccess::checkGlobusSSH(bool remoteGlobus, string shellName)
{
    return ((remoteGlobus && (shellName.find("ssh") != string::npos)));
}


bool WxConfigureAccess::checkGlobusSSH()
{
    string slctShell = (string)p_remoteShellChoice->GetStringSelection();
    RefMachine *machRgstn = p_slctPrefs->getRegisteredMachine();

    return (this->checkGlobusSSH(machRgstn, slctShell));
}


string WxConfigureAccess::getPassword()
{
    string password = STLUtil::trim((string)(p_password1TextCtrl->GetValue()));

    return password;
}


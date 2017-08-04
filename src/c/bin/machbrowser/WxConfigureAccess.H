/**
 *  @file
 *  @author Ken Swanson
 *
 *  Small dialogue for specifying the communication settings (shell, username,
 *  and password(s)) for a machine by a specific user.
 *
 */


//  ***  Developer note  ***
//  A lot of the code for selection of shells, displaying and processing username,
//  password(s), etc., is copied out of WxLauncher and modified.  It would seem
//  reasonable to consolidate this functionality into one control class, that
//  extends ewxPanel, and use that class in both this dialogue and in the WxLauncher
//  application.  Then this dialogue would consist of that plus the Okay and Cancel
//  buttons.  Would simplify a lot, and eliminate the duplicated functionality.
//  (KRS 2006.0621)

#ifndef WXCONFIGUREACCESS_HH
#define WXCONFIGUREACCESS_HH

#include <string>
    using std::string;

#include "WxConfigureAccessGUI.H"

class ewxButton;
class ewxChoice;
class ewxPanel;
class ewxStaticText;
class ewxTextCtrl;
class MachinePreferences;
class RefMachine;

class WxConfigureAccess: public WxConfigureAccessGUI
{
    DECLARE_CLASS(WxConfigureAccess);
    
    public:
        //  Constructor(s), Destructor
        WxConfigureAccess(      wxWindow* parent,
                                wxWindowID id = SYMBOL_WXCONFIGUREACCESSGUI_IDNAME,
                          const wxString& caption = SYMBOL_WXCONFIGUREACCESSGUI_TITLE,
                          const wxPoint& pos = SYMBOL_WXCONFIGUREACCESSGUI_POSITION,
                          const wxSize& size = SYMBOL_WXCONFIGUREACCESSGUI_SIZE,
                                long style = SYMBOL_WXCONFIGUREACCESSGUI_STYLE );

        void remoteShellChoiceSelectedCB( wxCommandEvent& event );
        void usernameTextCtrlUpdatedCB( wxCommandEvent& event );
        void password1TextCtrlUpdatedCB( wxCommandEvent& event );
        void password2TextCtrlUpdatedCB( wxCommandEvent& event );
        void okayButtonClickCB( wxCommandEvent& event );
        void cancelButtonClickCB( wxCommandEvent& event );

        void showConfigure(MachinePreferences *prefs, bool *cncld);
        string getPassword();

    private:
        bool checkRemoteGlobus(RefMachine *machRgstn, string shellName);

        bool checkGlobusSSH(RefMachine *machRgstn, string shellName);
        bool checkGlobusSSH(bool remoteGlobus, string shellName);
        bool checkGlobusSSH();

        void updateControls();
        
        string refreshShells(RefMachine *machRgstn);
        void refreshAuthentication(RefMachine *machRgstn, string shellName);
        void refreshPasswords(const bool& remoteGlobus, const string& shellName,
                              const string& machName, const string& user);

        ewxChoice               *p_remoteShellChoice;
        ewxTextCtrl             *p_usernameTextCtrl;
        ewxTextCtrl             *p_password1TextCtrl;
        ewxStaticText           *p_password1Label;
        ewxTextCtrl             *p_password2TextCtrl;
        ewxStaticText           *p_password2Label;
        ewxButton               *p_okayButton;
        ewxButton               *p_cancelButton;

        ewxPanel                *p_remoteShellPanel;
        ewxPanel                *p_usernamePanel;
        ewxPanel                *p_password1Panel;
        ewxPanel                *p_password2Panel;

        bool                     p_cncld;
        MachinePreferences      *p_slctPrefs;
        bool                     p_inCtrlUpdate;
};


#endif  // WXCONFIGUREACCESS_HH

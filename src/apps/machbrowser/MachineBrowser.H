/**
 *  @file
 *  @author Ken Swanson
 *
 *  Utility for viewing the status of machines that are registered to run
 *  calculations.  Specific information includes general machine status,
 *  queue or processes, and disk usage.
 *
 */


#ifndef MACHINEBROWSER_HH
#define MACHINEBROWSER_HH

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <string>
   using std::string;

#include "util/StringConverter.H"
#include "util/Preferences.H"
#include "util/JMSPublisher.H"
#include "util/TDateTime.H"

#include "tdat/QueueMgr.H"
#include "tdat/RefQueueMgr.H"

#include "dsm/MachinePreferences.H"

#include "comm/RCommand.H"

#include "wxgui/WxJMSListener.H"
#include "wxgui/ewxWindowUtils.H"
#include "wxgui/DefaultTableModel.H"
#include "wxgui/WxTableView.H"

#include "MachineTableElement.H"
#include "MachineBrowserGUI.H"
#include "WxMachineStatus.H"
#include "WxConfigureAccess.H"


#define ITM_MACHINECONFIG               22584
#define ITM_MACHINESTATUS               22587
#define ITM_MACHINECOMMAND              22588

#define ID_MACHINES_TABLEVIEW           22585
#define ID_MACHINEBROWSER_MACHINESTATUS 22586

#define WXMACHINEBROWSER_DFLT_LEFT         -1
#define WXMACHINEBROWSER_DFLT_TOP          -1
#define WXMACHINEBROWSER_DFLT_WIDTH       700
#define WXMACHINEBROWSER_DFLT_HEIGHT      600


class MachineBrowser: public MachineBrowserGUI,
                      public JMSPublisher
{
    DECLARE_EVENT_TABLE()

    public:
        /// Constructors, Destructor
        MachineBrowser(      wxWindow* parent,
                             wxWindowID id = SYMBOL_MACHINEBROWSERGUI_IDNAME,
                       const wxString& caption = SYMBOL_MACHINEBROWSERGUI_TITLE,
                       const wxPoint& pos = SYMBOL_MACHINEBROWSERGUI_POSITION,
                       const wxSize& size = SYMBOL_MACHINEBROWSERGUI_SIZE,
                             long style = SYMBOL_MACHINEBROWSERGUI_STYLE);
        virtual ~MachineBrowser();

        void refreshList();

        //  Event Handlers
        void machineQueryButtonClickCB(wxCommandEvent& event);
        void diskQueryButtonClickCB(wxCommandEvent& event);
        void processQueryButtonClickCB(wxCommandEvent& event);
        void machineRegisterMenuItemClickCB( wxCommandEvent& event );
        void configureButtonClickCB( wxCommandEvent& event );
        void machinesTableViewSelectionChangedCB(wxCommandEvent& event);
        void exitMenuitemClickCB(wxCommandEvent& event);
        void helpAppMenuitemClickCB(wxCommandEvent& event);
        void helpSupportMenuitemClickCB(wxCommandEvent& event);
        void viewDetachButtonClickCB(wxCommandEvent& event);
        void mainWindowCloseCB(wxCloseEvent& event);

        WxMachineStatus * getStatusFrame(MachinePreferences *prefs);

        string getSelectionName();


    private:
        //  Methods
        void createControls();
        void updateListModel();
        void setViewFilter(int k);
        void toggleSortDirection();
        void showRegister(MachineTableElement *elmt = NULL);
        void updateQueryButtons();
        void refreshControls();

   //     int locateElement(string refName)

        void loadSettings(Preferences *prefs);
        void saveSettings(Preferences *prefs);
        string queryStatus(int mode);

        MachineTableElement * findTableElement(string rfncName);

        WxTableView             *p_machView;
        DefaultTableModel       *p_machModel;

        //  Variables, Constants, Member Data
        MachineTableElement         *p_slctElmt;
        ewxButton                   *p_machineQueryButton;
        ewxButton                   *p_diskQueryButton;
        ewxButton                   *p_processQueryButton;

        vector<string>               p_statusQueries;

        ewxTextCtrl                 *p_statusTextCtrl;
        ewxStaticText               *p_modeDscpnLabel;
        ewxStaticText               *p_machRgstnLabel;

        vector<string>               p_modeDscpns;
        ewxButton                   *p_viewDetachButton;
        ewxButton                   *p_machineConfigButton;

        WxMachineStatus             *p_statFrame;
        WxConfigureAccess           *p_configDialog;

        bool                         p_inCtrlUpdate;

        bool        checkConnectAllowed(MachinePreferences *prefs);
};


#endif  // MACHINEBROWSER_HH

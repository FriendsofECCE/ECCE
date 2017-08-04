//.......1.........2.........3.........4.........5.........6.........7.........8

/**
 *  @file
 *  @author Ken Swanson
 *
 *  Helper window for displaying the status of the machine.  This is brought
 *  up when the user clicks the "Show in Child Window" button on the main
 *  MachineBrowser frame.
 *
 */


#ifndef WXMACHINESTATUS_HH
#define WXMACHINESTATUS_HH

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <string>
    using std::string;

#include "WxMachineStatusGUI.H"

class ewxButton;
class ewxStaticText;
class ewxTextCtrl;

class WxMachineStatus: public WxMachineStatusGUI
{
    public:
    //  Constructor(s), Destructor
        WxMachineStatus(      wxWindow* parent,
                              wxWindowID id = SYMBOL_WXMACHINESTATUSGUI_IDNAME,
                        const wxString& cptn = SYMBOL_WXMACHINESTATUSGUI_TITLE,
                        const wxPoint& pos = SYMBOL_WXMACHINESTATUSGUI_POSITION,
                        const wxSize& size = SYMBOL_WXMACHINESTATUSGUI_SIZE,
                              long style = SYMBOL_WXMACHINESTATUSGUI_STYLE );
        virtual ~WxMachineStatus();

        //  Event handlers
        void mainWindowCloseCB(wxCloseEvent& event);
        void closeButtonClickCB(wxCommandEvent& event);

        //  Public methods
        void showStatus(string dscpn, string status);


    private:
        void                         initialize();
        void                         refresh();

        ewxTextCtrl                 *p_statusTextCtrl;
        ewxStaticText               *p_modeDscpnLabel;
        ewxButton                   *p_closeButton;
        bool                         p_inCtrlUpdate;
};

#endif  // MACHINESTATUS_HH

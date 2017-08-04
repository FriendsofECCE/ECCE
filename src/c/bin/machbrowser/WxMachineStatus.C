/**
 *  @file
 *  @author Ken Swanson
 *
 *  Helper window for displaying the status of the machine.  This is brought
 *  up when the user clicks the "Show in Child Window" button on the main
 *  MachineBrowser frame.
 *
 */

#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"

#include "WxMachineStatus.H"

WxMachineStatus::WxMachineStatus(      wxWindow* parent,
                                       wxWindowID id,
                                 const wxString& cptn,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                       long style)
            : WxMachineStatusGUI(parent, id, cptn, pos, size, style)
{
    this->initialize();
    p_inCtrlUpdate = false;
}

WxMachineStatus::~WxMachineStatus()
{
}


/**
 *  Initialize components of graphical interface.
 */
void WxMachineStatus::initialize()
{
    p_modeDscpnLabel
        = (ewxStaticText *)(FindWindowById(ID_STATIC_WXMACHINESTATUS_MODE));

    p_statusTextCtrl
        = (ewxTextCtrl*)(FindWindowById(ID_TEXTCTRL_WXMACHINESTATUS_STATUS));
    p_statusTextCtrl->SetEditable(false);

    p_closeButton
        = (ewxButton *)(FindWindowById(ID_BUTTON_WXMACHINESTATUS_CLOSE));

    this->Fit();
    this->SetAutoLayout(true);
}


/**
 *  Display status information in the (read-only) text area
 *      @param dscpn    Brief description of what is displayed
 *      @param status   Status information for machine
 */
void WxMachineStatus::showStatus(string dscpn, string status)
{
    p_modeDscpnLabel->SetLabel(dscpn);
    p_statusTextCtrl->SetValue(status);
    p_statusTextCtrl->Refresh();
}


/**
 *  Functionality in response to window close event.
 *  If the user is closing via the window decorations, then the close event is
 *  vetoed and the window is hidden; otherwise, if closed programmatically by
 *  an owner window, the window is destroyed.
 */
void WxMachineStatus::mainWindowCloseCB(wxCloseEvent& event)
{
    if (event.CanVeto())
    {
        //  Closing via window decorations
        event.Veto();
        this->Hide();
    }
    else
    {
        //  Forced Close
        this->Destroy();
    }
}


/**
 *  Calls the Close() method on the window, which initiates a CloseEvent on the
 *  window.  This method retains the default behavior of the Close() method, in
 *  that it does not try to force closure of the window.  Thus, the CloseEvent
 *  that is fired can be vetoed.  The mainWindowCloseCB() method will then hide
 *  the window instead of destroying it.
 */
void WxMachineStatus::closeButtonClickCB(wxCommandEvent& event)
{
    this->Close();
}

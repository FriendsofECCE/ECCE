/**
 *  @file
 *  @author Ken Swanson
 *
 *  Parent application for the MachineBrowser utility.
 *
 */


#ifndef MACHINEBROWSERAPP_HH
#define MACHINEBROWSERAPP_HH

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <string>
   using std::string;

#include "wxgui/WxAppJMSMessageDispatch.H"

#include "MachineBrowser.H"

class MachineBrowserApp : public WxAppJMSMessageDispatch
{

    public:
        MachineBrowserApp();
        ~MachineBrowserApp();

        virtual bool OnInit();
        virtual int OnExit();
        virtual void subscribeMessages();
        virtual string identify();

        string getName();

        void machineConfigChangedMCB(JMSMessage &msg);
        void machineRegChangedMCB(JMSMessage &msg);
        void authMCB(JMSMessage &msg);


    protected:
        MachineBrowser *p_brwsrFrame;


};

DECLARE_APP(MachineBrowserApp)

#endif // MACHINEBROWSERAPP_HH

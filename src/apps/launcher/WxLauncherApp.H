/**
 * @file
 * @author Ken Swanson
 *
 */

#ifndef WXLAUNCHERAPP_HH
#define WXLAUNCHERAPP_HH

#include "util/JMSMessage.H"

#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/ewxApp.H"

#include "WxLauncher.H"

class LauncherMessages;

class WxLauncherApp : public ewxApp
{
    public:

        WxLauncherApp();
        virtual bool OnInit();
        virtual int OnExit();
        virtual string getName() const;


    private:
        WxLauncher *p_launchFrame;
        LauncherMessages *p_msgs;
};

DECLARE_APP(WxLauncherApp)

#endif // WXLAUNCHERAPP_HH

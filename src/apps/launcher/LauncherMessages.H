/**
 * @file
 *
 * The Launcher's message handlers (#983); see LauncherMessages.C.
 */
#ifndef LAUNCHERMESSAGES_HH
#define LAUNCHERMESSAGES_HH

#include "util/JMSMessage.H"

#include "wxgui/WxJMSMessageDispatch.H"

class LaunchPanel;
class WxLauncher;

class LauncherMessages : public WxJMSMessageDispatch
{
    public:
        // host NULL: the panel embedded in the editor's Set-up area (no window to
        // invoke, raise or close), which only follows data and machine changes.
        LauncherMessages(LaunchPanel *panel, WxLauncher *host);

        void subscribeFollow();

        void subscribeMessages();

        void invokeMCB(JMSMessage& msg);
        void quitMCB(JMSMessage& msg);
        void identifyMCB(JMSMessage& msg);
        void invokeStatusMCB(JMSMessage& msg);

        void calcMoveMCB(JMSMessage& msg);
        void calcDeleteMCB(JMSMessage& msg);
        void calcCodeMCB(JMSMessage& msg);
        void statusChangeMCB(JMSMessage& msg);
        void propertyChangeMCB(JMSMessage& msg);

        void updatePrefsMCB(JMSMessage& msg);

        void machConfChangedMCB(JMSMessage& msg);
        void machregChangedMCB(JMSMessage& msg);
        void msgAuthMCB(JMSMessage& msg);

        string identify();

    private:
        LaunchPanel *p_panel;
        WxLauncher *p_host;   // the window or tab around the panel
};

#endif

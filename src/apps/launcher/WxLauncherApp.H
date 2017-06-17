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

class WxLauncherApp : public ewxApp, public WxJMSMessageDispatch
{
    public:

        WxLauncherApp();
        virtual bool OnInit();
        virtual int OnExit();
        virtual string getName() const;

        void subscribeMessages();

        void invokeMCB(JMSMessage& msg);
        void quitMCB(JMSMessage& msg);
        void identifyMCB(JMSMessage& msg);
        void invokeStatusMCB(JMSMessage& msg);

        void calcMoveMCB(JMSMessage& msg);
        void calcDeleteMCB(JMSMessage& msg);
        void calcCodeMCB(JMSMessage& msg);
        void statusChangeMCB(JMSMessage& msg);

        void updatePrefsMCB(JMSMessage& msg);

        void machConfChangedMCB(JMSMessage& msg);
        void machregChangedMCB(JMSMessage& msg);
        void msgAuthMCB(JMSMessage& msg);

        string identify();

    private:
        WxLauncher *p_launchFrame;
        static int p_indentLevel;
};

DECLARE_APP(WxLauncherApp)

#endif // WXLAUNCHERAPP_HH

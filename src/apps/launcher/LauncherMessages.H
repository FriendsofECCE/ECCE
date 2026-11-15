/**
 * @file
 *
 * The Launcher's message handlers (#983); see LauncherMessages.C.
 */
#ifndef LAUNCHERMESSAGES_HH
#define LAUNCHERMESSAGES_HH

#include "util/JMSMessage.H"

#include "wxgui/WxJMSMessageDispatch.H"

class WxLauncher;

class LauncherMessages : public WxJMSMessageDispatch
{
    public:
        LauncherMessages(WxLauncher *frame);

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
        WxLauncher *p_frame;
};

#endif

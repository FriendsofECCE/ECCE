/**
 *  @file
 *  @author Ken Swanson
 *
 *  Parent application for the MachineRegister utility.  Provides JMS
 *  listening services.
 *
 */


#ifndef WXMACHINEREGISTERAPP_HH
#define WXMACHINEREGISTERAPP_HH

#include "util/JMSMessage.hpp"

#include "wxgui/WxJMSMessageDispatch.hpp"
#include "wxgui/ewxApp.hpp"

#include "WxMachineRegister.hpp"


class WxMachineRegisterApp : public ewxApp, public WxJMSMessageDispatch
{

    public:
        WxMachineRegisterApp();

        virtual bool OnInit();
        virtual int OnExit();

        string identify();
        string getName();

        void subscribeMessages();

        void invokeMCB(JMSMessage& msg);
        void quitMCB(JMSMessage& msg);
        void identifyMCB(JMSMessage& msg);
        void invokeStatusMCB(JMSMessage& msg);
        void authMCB(JMSMessage& msg);

    private:

        WxMachineRegister *p_rgstrFrame;
};

DECLARE_APP(WxMachineRegisterApp)

#endif // WXMACHINEREGISTERAPP_HH

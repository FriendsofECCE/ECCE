/**
 * @file
 * @author Ken Swanson
 *
 */

#ifndef WXBASISTOOLAPP_HH
#define WXBASISTOOLAPP_HH

#include "util/JMSMessage.H"


#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/ewxApp.H"

#include "WxBasisTool.H"



class WxBasisToolApp : public ewxApp, public WxJMSMessageDispatch
{

    public:
        WxBasisToolApp();
        virtual bool OnInit();
        virtual int OnExit();
        virtual string getName() const;

        void subscribeMessages();

        void invokeMCB(JMSMessage& msg);
        void quitMCB(JMSMessage& msg);
        void identifyMCB(JMSMessage& msg);
        void invokeStatusMCB(JMSMessage& mesg);
//        void updatePrefsMCB(JMSMessage& msg);
        void msgAuthMCB(JMSMessage& mesg);
        // Message Methods
        void perTabPrefsChangedMCB(JMSMessage& msg);
        void statePrefsChangedMCB(JMSMessage& msg);
        void miscPrefsChangedMCB(JMSMessage& msg);
        void toolStatusMCB(JMSMessage& msg);
        void projectRenamedMCB(JMSMessage& msg);
        void projectDeletedMCB(JMSMessage& msg);
        void projectFormulaChangedMCB(JMSMessage& msg);
        void projectStatusChangedMCB(JMSMessage& msg);
        void projectCalcChangedMCB(JMSMessage& msg);
        void projectBasisChangedMCB(JMSMessage& msg);
        void filechooserMCB(JMSMessage& msg);
        void filechooserErrorMCB(JMSMessage& msg);
        string identify();

    private:
        WxBasisTool *p_basisToolFrame;
};

DECLARE_APP(WxBasisToolApp)

#endif      // WXBASISTOOLAPP_HH

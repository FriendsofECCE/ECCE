/**
 * @file
 *
 * The Basis Set Tool's message handlers on a dispatcher its window owns
 * (#983).
 */
#ifndef BASISTOOLMESSAGES_HH
#define BASISTOOLMESSAGES_HH

#include "util/JMSMessage.H"
#include "wxgui/WxJMSMessageDispatch.H"

class WxBasisTool;

class BasisToolMessages : public WxJMSMessageDispatch
{
    public:
        BasisToolMessages(WxBasisTool *frame);

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

#endif

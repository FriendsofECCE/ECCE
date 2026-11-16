/**
 * @file
 *
 * The Calculation Editor window's messages: a dispatcher of its own
 * instead of the wxApp, so that tool windows can share one process (#983).
 */
#ifndef CALCEDMESSAGES_H_
#define CALCEDMESSAGES_H_

#include "util/JMSMessage.H"
#include "wxgui/WxJMSMessageDispatch.H"

class CalcEd;

class CalcEdMessages : public WxJMSMessageDispatch
{
  public:
    CalcEdMessages(CalcEd *calced);

    void subscribeMessages();
    void showEditor();
    bool shown() const { return p_shown; }

    void invokeMCB(JMSMessage& msg);
    void quitMCB(JMSMessage& msg);
    void identifyMCB(JMSMessage& msg);
    void invokeStatusMCB(JMSMessage& msg);

    // Calc/Project change messages
    void calcMoveMCB(JMSMessage& msg);
    void calcDeleteMCB(JMSMessage& msg);
    void calcBasisMCB(JMSMessage& msg);
    void calcCodeMCB(JMSMessage& msg);
    void calcDetailsMCB(JMSMessage& msg);
    void calcLaunchMCB(JMSMessage& msg);
    void calcRuntypeMCB(JMSMessage& msg);
    void calcSpinMCB(JMSMessage& msg);
    void calcStateMCB(JMSMessage& msg);
    void calcSubjectMCB(JMSMessage& msg);
    void calcTheoryMCB(JMSMessage& msg);
    void calcPropertyMCB(JMSMessage& msg);

    void updatePrefsMCB(JMSMessage& msg);
    void updatePerTabMCB(JMSMessage& msg);

    void msgAuthMCB(JMSMessage& msg);

    string identify();

  private:
    CalcEd *p_calced;
    bool p_shown;
};

#endif // CALCEDMESSAGES_H_

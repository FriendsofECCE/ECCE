#ifndef CALCEDAPP_H_
#define CALCEDAPP_H_

#include "util/JMSMessage.H"

#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/ewxApp.H"

#include "CalcEd.H"

class CalcEdApp : public ewxApp,
                  public WxJMSMessageDispatch
{
  public:
    CalcEdApp();
    virtual ~CalcEdApp();
    virtual bool OnInit();
    virtual int OnExit();
    virtual string getName() const;

    void subscribeMessages();

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
};

DECLARE_APP(CalcEdApp)

#endif // CALCEDAPP_H_

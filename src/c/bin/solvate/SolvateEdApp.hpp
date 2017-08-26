#ifndef SOLVATEEDAPP_H_
#define SOLVATEEDAPP_H_

#include "util/JMSMessage.hpp"

#include "wxgui/WxJMSMessageDispatch.hpp"
#include "wxgui/ewxApp.hpp"

#include "SolvateEd.hpp"

class SolvateEdApp : public ewxApp,
                     public WxJMSMessageDispatch
{
  public:
    SolvateEdApp();
    virtual ~SolvateEdApp();
    virtual bool OnInit();
    virtual int OnExit();
    virtual string getName() const;

    void subscribeMessages();

    void invokeMCB(JMSMessage& msg);
    void quitMCB(JMSMessage& msg);
    void identifyMCB(JMSMessage& msg);

    // Calc/Project change messages
    void calcMoveMCB(JMSMessage& msg);
    void calcDeleteMCB(JMSMessage& msg);
    void calcStateMCB(JMSMessage& msg);
    void calcSubjectMCB(JMSMessage& msg);

    void msgAuthMCB(JMSMessage& msg);

    string identify();

  private:
    SolvateEd *p_solvate;
};

DECLARE_APP(SolvateEdApp)

#endif // SOLVATEEDAPP_H_

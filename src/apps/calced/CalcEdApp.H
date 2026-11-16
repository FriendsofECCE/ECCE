#ifndef CALCEDAPP_H_
#define CALCEDAPP_H_

#include "wxgui/ewxApp.H"

#include "CalcEd.H"

class CalcEdMessages;

class CalcEdApp : public ewxApp
{
  public:
    CalcEdApp();
    virtual ~CalcEdApp();
    virtual bool OnInit();
    virtual int OnExit();
    virtual string getName() const;

  private:
    CalcEd *p_calced;
    CalcEdMessages *p_msgs;
};

DECLARE_APP(CalcEdApp)

#endif // CALCEDAPP_H_

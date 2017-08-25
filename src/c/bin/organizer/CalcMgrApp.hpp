/**
 * @file
 *
 * @author Lisong Sun
 *
 */

#ifndef _CALCMGRAPP_H_
#define _CALCMGRAPP_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "CalcMgrApp.C"
#endif

#include "wx/wx.h"
#include "wxgui/ewxApp.H"

class CalcMgr;
class WxJMSMessageDispatch;

class CalcMgrApp : public ewxApp, public WxJMSMessageDispatch
{
public:

DECLARE_CLASS(CalcMgrApp)

  CalcMgrApp();

  virtual bool OnInit();
  virtual string getName() const;

  virtual int OnExit();

  void quitMCB(JMSMessage& msg);
  void identifyMCB(JMSMessage& msg);
  void statePrefChangeMCB(JMSMessage& msg);
  void miscPrefChangeMCB(JMSMessage& msg);
  void invokeMCB(JMSMessage& msg);
  void msgStatusMCB(JMSMessage& msg);
  void msgAuthMCB(JMSMessage& msg);
  void msgRenameMCB(JMSMessage& msg);
  void msgURLCreatedMCB(JMSMessage& msg);
  void msgURLRemovedMCB(JMSMessage& msg);
  void msgChangedMCB(JMSMessage& msg);
  void msgMachConfChangedMCB(JMSMessage& msg);
  void msgMachRegChangedMCB(JMSMessage& msg);
  void msgDataServerMCB(JMSMessage& msg);

protected:
  
  CalcMgr * p_calcMgr;
};

DECLARE_APP(CalcMgrApp)

#endif
// _CALCMGRAPP_H_

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

#include <map>
#include <wx/weakref.h>
#include "wx/wx.h"
#include "wxgui/ewxApp.H"

#include "CalcMgr.H"
class WxJMSMessageDispatch;

class CalcMgrApp : public ewxApp, public WxJMSMessageDispatch
{
public:

DECLARE_CLASS(CalcMgrApp)

  CalcMgrApp();

  virtual bool OnInit();
  virtual string getName() const;

  virtual int OnExit();
  void openUrlWhenReady(const string& url, int tries);

  void quitMCB(JMSMessage& msg);
  void identifyMCB(JMSMessage& msg);
  void launchRequestMCB(JMSMessage& msg);
  void rerunRequestMCB(JMSMessage& msg);
  void terminateRequestMCB(JMSMessage& msg);
  void annotateRequestMCB(JMSMessage& msg);
  void moveRequestMCB(JMSMessage& msg);
  void workflowRequestMCB(JMSMessage& msg);
  void reconnectRequestMCB(JMSMessage& msg);
  void fetchRequestMCB(JMSMessage& msg);
  void reparseRequestMCB(JMSMessage& msg);
  void miscPrefChangeMCB(JMSMessage& msg);
  void invokeMCB(JMSMessage& msg);
  void msgStatusMCB(JMSMessage& msg);
  void msgAuthMCB(JMSMessage& msg);
  void msgRenameMCB(JMSMessage& msg);
  void msgURLCreatedMCB(JMSMessage& msg);
  void msgURLRemovedMCB(JMSMessage& msg);
  void msgChangedMCB(JMSMessage& msg);
  void msgSubjectMCB(JMSMessage& msg);
  void msgProgressMCB(JMSMessage& msg);
  void msgMachConfChangedMCB(JMSMessage& msg);
  void msgMachRegChangedMCB(JMSMessage& msg);
  void msgDataServerMCB(JMSMessage& msg);

protected:
  // ecce-reparse: a request waits here for the 'reparse' notice of each
  // calculation it started; `lines` holds the answers so far.
  struct PendingReparse {
    string id;
    std::map<string, string> waiting;   // url -> calculation name
    vector<string> lines;
  };
  vector<PendingReparse> p_pendingReparse;
  void answerReparse(const string& id, const vector<string>& lines);
  void reparseNotice(const string& url, const string& value);

  // Weak: null once the Organizer window is deleted, so a message that
  // arrives during shutdown (e.g. inside a nested event loop) is dropped.
  wxWeakRef<CalcMgr> p_calcMgr;
};

DECLARE_APP(CalcMgrApp)

#endif
// _CALCMGRAPP_H_

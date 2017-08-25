/**
* @file
*
*
*/
#ifndef WXJMSMESSAGEDISPATCH_H
#define WXJMSMESSAGEDISPATCH_H

#include <vector>
  using std::vector;

#include "wxgui/WxJMSListener.H"
#include "wxgui/WxJMSSubscriber.H"
#include "util/JMSPublisher.H"
#include "util/JMSMessage.H"
#include "util/NotImplementedException.H"

#include "wx/wx.h"
#include "wx/event.h"
#include "wx/socket.h"

class WxJMSMessageDispatch;

/**
 * Helper class to schedule work method for handling sigchld events.
 * We do this so that we don't do any nasty things that use system
 * resources inside the signal handler itself (like sending messages).
 */
class SigTimer: public wxTimer
{
   public:
      SigTimer(WxJMSMessageDispatch * parent, int pid);
      void Notify();
   private:
      WxJMSMessageDispatch * p_parent;
      int p_pid;
};

class TimeOutAppExec: public wxTimer
{
public:
  TimeOutAppExec(WxJMSMessageDispatch * parent);
  void Notify();
private:
  WxJMSMessageDispatch * p_parent;
};

class TimeOutAppInvoke: public wxTimer
{
public:
  TimeOutAppInvoke(WxJMSMessageDispatch * parent);
  void Notify();
private:
  WxJMSMessageDispatch * p_parent;
};

class TimeOutInvokeStatusCheck: public wxTimer
{
public:
  TimeOutInvokeStatusCheck(WxJMSMessageDispatch * parent);
  void Notify();
private:
  WxJMSMessageDispatch * p_parent;
};


/**
 * This class serves as a JMS publisher/subscriber for Ecce apps built
 * with the wxWidgets toolkit.
 *
 * This class subscribes to common topics that must be handled by
 * all apps, this includes raise/iconify, invoke, and poll.  
 * 
 * This class also provides a clean interface for invoking another
 * Ecce app (the startApp method).
 * The startApp method wraps the steps required to check for existence
 * and invoke an Ecce app with a specific context. 
 */
class WxJMSMessageDispatch : public WxJMSSubscriber, public WxJMSListener
{

public: 

  WxJMSMessageDispatch(const string& app_name, bool app=true);
  virtual ~WxJMSMessageDispatch();

  void setPollContext(const string& contextId);
  string getPollContext() const;
    
  void registerTopShell(wxWindow *w);
  void unregisterTopShell(wxWindow *w);
  bool publish(const string& topic,
               const JMSMessage& message) const;
  JMSMessage* newMessage() const;
  JMSMessage* newMessage(const Target& target) const;

  virtual bool subscribe(const char* topic, 
                 WxJMSListener *l,
                 wxJmsCBFunc handler,
                 bool filterSelf = true);
  void notifyReady();
  void appExec();
  void appInvoke();
  void invokeStatusCheck();
  void disconnect();

protected:
  bool subscribe(const char* topic, 
                 wxJmsCBFunc handler,
                 bool filterSelf = true);

  void raiseAllWindows();
  void registerMyselfAsAppExecer();
  static void appQuit(int arg);

private:

  void setIconifiedMCB(JMSMessage& msg);
  void raiseMCB(JMSMessage& msg);

  void getAppMCB(JMSMessage& msg);
  void pollReplyMCB(JMSMessage& msg);
  void internalInvokeStatusMCB(JMSMessage& msg);
  void appReadyMCB(JMSMessage& msg);

  void pollMCB(JMSMessage& msg);
  void internalInvokeMCB(JMSMessage& msg);
  void completeStartupProcess(int pid, const string& status);
  void sendFailedStatusMessage(int pid);


  JMSPublisher* p_publisher;
  vector<wxWindow*> p_topShells;
  vector<bool> p_minimizedByGateway;
  vector<StartupPacket> p_startups;

  /** Special invoke handler so both dispatcher and app can subscribe. */
  wxJMSCallbackStructure *p_invokeMCBPtr;

  TimeOutAppExec * p_toAppExec;
  TimeOutAppInvoke * p_toAppInvoke;
  TimeOutInvokeStatusCheck * p_toInvokeStatusCheck;

  /** a key used to decide, during startup polling, if contexts match */
  string p_contextId;

  void setTheDispatcher(WxJMSMessageDispatch *instance);
  void handleSigChild(int pid);

  /** A pointer to the dispatcher that controls app startup. Each process
      should only have one of these.  Needed for sigchild handling to
      cleanup zombies without causing other problems. */
  static WxJMSMessageDispatch *p_theDispatcher;
  friend class SigTimer;

};

#endif 

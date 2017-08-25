/**
* @file
*
*
*/

#include "wx/wx.h"
#include "wx/timer.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

//#define debug

#ifdef debug
#include <iostream>
  using std::cerr;
  using std::cout;
  using std::endl;
#endif

#include <algorithm>
  using std::find;

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/JMSMessage.H"
#include "util/StringConverter.H"

#include "tdat/AuthCache.H"

#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceTool.H"

#include "wxgui/WxJMSMessageDispatch.H"

// KLS 9/6/06
// Made this static instance pointer for signal handling
WxJMSMessageDispatch *WxJMSMessageDispatch::p_theDispatcher;

SigTimer::SigTimer(WxJMSMessageDispatch * parent, int pid) 
        : p_parent(parent), p_pid(pid) 
{ 
}
void SigTimer::Notify() 
{
  if (p_pid > 0) {
     p_parent->handleSigChild(p_pid);
  }
}


TimeOutAppExec::TimeOutAppExec(WxJMSMessageDispatch * parent):wxTimer()
{
  p_parent=parent;
}


void TimeOutAppExec::Notify()
{
  p_parent->appExec();
}


TimeOutAppInvoke::TimeOutAppInvoke(WxJMSMessageDispatch * parent):wxTimer()
{
  p_parent=parent;
}


void TimeOutAppInvoke::Notify()
{
  p_parent->appInvoke();
}


TimeOutInvokeStatusCheck::TimeOutInvokeStatusCheck(WxJMSMessageDispatch * parent):wxTimer()
{
  p_parent=parent;
}


void TimeOutInvokeStatusCheck::Notify()
{
  p_parent->invokeStatusCheck();
}


/**
 * Constructor - Subscribe (asynchronously) to general gui message topics,
 * providing common callback functions.
 * @param app_name name of subscriber 
 * @param app true if this is an app that will process invoke messages; false
 *   for frames that are part of messaging but not applications in and of
 *   themselves.
 */
WxJMSMessageDispatch::WxJMSMessageDispatch(const string& app_name, bool app)
  : WxJMSSubscriber(app_name)
{
  p_publisher = new JMSPublisher(app_name);
  p_invokeMCBPtr = 0;

  subscribe("ecce_poll",
            (wxJmsCBFunc)&WxJMSMessageDispatch::pollMCB);

  /*
    Gateway shouldn't listen to this message since it is the sender.
    Without this logic, gateway will be minimized by itself when switching
    desktop and won't be able to restore when switched back.
  */
  if (app_name != "Gateway") {
    subscribe("ecce_set_iconified",
              (wxJmsCBFunc)&WxJMSMessageDispatch::setIconifiedMCB);
  }

  subscribe("ecce_raise", (wxJmsCBFunc)&WxJMSMessageDispatch::raiseMCB);

  if (app) {
     WxJMSSubscriber::subscribe("ecce_invoke", this,
           (wxJmsCBFunc)&WxJMSMessageDispatch::internalInvokeMCB);
  }

  p_toAppExec = new TimeOutAppExec(this);
  p_toAppInvoke = new TimeOutAppInvoke(this);
  p_toInvokeStatusCheck = new TimeOutInvokeStatusCheck(this);
}



/**
 * Destructor
 * Disconnects if not already disconnected.
 */
WxJMSMessageDispatch::~WxJMSMessageDispatch()
{
  disconnect();
}


void WxJMSMessageDispatch::setTheDispatcher(WxJMSMessageDispatch *theInstance)
{
   if (p_theDispatcher != 0) {
      cerr << "OH OH You should not be having two dispatchers " << endl;
   }
   p_theDispatcher = theInstance;
}


/**
 * Disconnect from messaging.
 * This method is safe to call repeatedly. There is no way to reconnect
 * after disconnecting though this could be implemented easily if we move
 * the connect code out of the constructor.
 */
void WxJMSMessageDispatch::disconnect()
{
   if (p_publisher != 0) {
      int child;
      child=waitpid(-1, NULL, WNOHANG);
      while (child > 0)  { 
         child=waitpid(-1, NULL, WNOHANG);
      }

      p_toAppExec->Stop();
      p_toAppInvoke->Stop();
      p_toInvokeStatusCheck->Stop();

      delete p_toAppExec;
      p_toAppExec = 0;

      delete p_toAppInvoke;
      p_toAppInvoke = 0;

      delete p_toInvokeStatusCheck;
      p_toInvokeStatusCheck = 0;

      delete p_publisher;      
      p_publisher = 0;
   }
}


/**
 * Registers the window as top-level, for the purpose of automatically
 * providing desktop iconify/uniconify functionality.  Non top-level
 * windows are ignored.
 */
void WxJMSMessageDispatch::registerTopShell(wxWindow *wid)
{
  // Add the top level window to the vector
  if ( (wid->IsTopLevel()) &&
       (find(p_topShells.begin(), 
             p_topShells.end(), 
             wid) == p_topShells.end()) ) {
    p_topShells.push_back(wid);
    p_minimizedByGateway.push_back(false);
  }
}


/**
 * Unregister the window (see registerTopShell()).
 */
void WxJMSMessageDispatch::unregisterTopShell(wxWindow *wid)
{
  p_topShells.clear();
  p_minimizedByGateway.clear();
}


/**
 * Publisher interface method - publish a method.
 */
bool WxJMSMessageDispatch::publish(const string& topic,
                const JMSMessage& message) const
{
  return p_publisher->publish(topic,message);
}


/**
 * Publisher interface method - get a new message.
 */
JMSMessage* WxJMSMessageDispatch::newMessage() const
{
  return p_publisher->newMessage();
}


/**
 * Publisher interface method - get a new message for a specific Target.
 */
JMSMessage* WxJMSMessageDispatch::newMessage(const Target& target) const
{
  return p_publisher->newMessage(target);
}


/**
 * Subscriber interface method - subscribe to a topic.  
 * Subscriptions can be added by arbitrary WxJMSListener objects.
 */
bool WxJMSMessageDispatch::subscribe(const char* topicStr,
                                WxJMSListener *l,
                                wxJmsCBFunc handler,
                                bool filterSelf) 
{
   bool ret = true;
   string top(topicStr);
   if (top == "ecce_invoke") {
      p_invokeMCBPtr = new wxJMSCallbackStructure;
      p_invokeMCBPtr->funcPtr.classFunc = handler;
      p_invokeMCBPtr->classPtr = l;
   } else {
      ret=WxJMSSubscriber::subscribe(topicStr, l, handler, filterSelf);
   }
   return ret;
}


/**
 * Subscriber interface method - subscribe to a topic.  
 * This method should only be used by subclasses so that the recieving
 * class type is correct.
 */
bool WxJMSMessageDispatch::subscribe(const char* topic, 
                                     wxJmsCBFunc handler,
                                     bool filterSelf)
{
  bool ret = true;
  string top(topic);
  if (top == "ecce_invoke") {
    p_invokeMCBPtr = new wxJMSCallbackStructure;
    p_invokeMCBPtr->funcPtr.classFunc = handler;
    p_invokeMCBPtr->classPtr = this;
  } else {
    ret=WxJMSSubscriber::subscribe(topic, this, handler, filterSelf);
  }
  return ret;
}


/**
 * Every app needs to call this function after it has initialized.
 * This signals the invoking app that it is ready to receive 
 * messages.
 */
void WxJMSMessageDispatch::notifyReady()
{
  JMSMessage* msg = newMessage();
  publish("ecce_app_ready", *msg);
  delete msg;
}


/**
 * Raises an applications windows to the top of the window
 * hierarchy.
 */
void WxJMSMessageDispatch::raiseAllWindows()
{
  for (int win=p_topShells.size()-1; win>=0; win--) {
    //    wxTopLevelWindow * topLevelWin =
    //      dynamic_cast<wxTopLevelWindow *>(p_topShells[win]);
    //    if (topLevelWin) {
    //      topLevelWin->Iconize(false);
    //      topLevelWin->Raise();
    //      topLevelWin->Show(true);
    //      topLevelWin->SetFocus();
    //      topLevelWin->RequestUserAttention();
    //    }
    //    else {
    (p_topShells[win])->Raise();
    (p_topShells[win])->Show(true);
    //    }
  }
}


/**
 * Any app that wants to be able to startup other apps, needs to
 * call this method.  It subscribes to the topics needed to perform
 * startup. 
 *
 * Within a process, this should only be called once.  
 * Within ecce, this should only be called once.
 */
void WxJMSMessageDispatch::registerMyselfAsAppExecer() 
{
   setTheDispatcher(this);
   subscribe("ecce_get_app",
         (wxJmsCBFunc)&WxJMSMessageDispatch::getAppMCB, 
         false);

   subscribe("ecce_app_ready", 
         (wxJmsCBFunc)&WxJMSMessageDispatch::appReadyMCB);

   subscribe("ecce_poll_reply", 
         (wxJmsCBFunc)&WxJMSMessageDispatch::pollReplyMCB);

   subscribe("ecce_internal_invoke_status", 
         (wxJmsCBFunc)&WxJMSMessageDispatch::internalInvokeStatusMCB, 
         false); 

   // The execer needs to handle child death events.
// Temporarily remove on theory it might be related to msg issues
//   signal(SIGCHLD, WxJMSMessageDispatch::appQuit);
}


/**
 * Will iconize or uniconize and application window and its
 * child windows.
 */
void WxJMSMessageDispatch::setIconifiedMCB(JMSMessage& msg)
{
  int new_state = msg.getIntProperty("state");
  wxTopLevelWindow * topWin = 0;
  
  for (int win=p_topShells.size()-1; win>=0; win--) {
    topWin  = dynamic_cast<wxTopLevelWindow*>(p_topShells[win]);
    if (topWin) {
      if (new_state == Iconic) {
        if (!topWin->IsIconized()) {
          topWin->Iconize(true);
          // Mark this window as minimized by gateway
          p_minimizedByGateway[win] = true;
        }
      }
      else {
        // Only restore those windows that was minimized by gateway
        if (p_minimizedByGateway[win]) {
          topWin->Iconize(false);
          p_minimizedByGateway[win] = false;
        }
      }
    }
    else {
      cerr << "Invlaid window type" << endl;
    }
  }
}


/** 
 * Raise an applications windows to the top of the window
 * heirarchy display.
 */
void WxJMSMessageDispatch::raiseMCB(JMSMessage& msg)
{
  raiseAllWindows();
}


// --------------------------------------------------------------------
// Methods (from caller's side) used to invoke a new ecce app
// --------------------------------------------------------------------

/****************************************************************************
 Method : getAppMCB
 Summary: 
          This is the callback handler used by the ecce_get_app topic,
          which is used as a messaging interface for the startApp method.

          Send a message to ecce_get_app if you want to invoke an app.
          The message should contain these properties:

              appname, forcenew, and calcurl
****************************************************************************/
void WxJMSMessageDispatch::getAppMCB(JMSMessage& startMsg)
{
  // First change the sender in the startMsg to be me instead
  // of the app it originally came from
  // 4/7/06 The above is from the original design.  Not sure if its needed.
  // However, we want the receiver to know the msg originator 
  // so send in as contextid property

  string origSender = "";
  if (startMsg.getIntProperty("p2p")) {
    origSender = startMsg.getSender().getID();
    startMsg.addProperty("origsendername", startMsg.getSender().getName());
    startMsg.addProperty("origsenderid", origSender);
  }

  Sender myself("", getMyID());
  startMsg.setSender(myself);
  
  // First make a new StartupPacket:
  StartupPacket packet;
  packet.appName = startMsg.getProperty("appname");
  packet.startMsg = startMsg;
  packet.pidToWatch = 0;
  packet.execStarted = false;
  packet.invokeStarted = false;
  
  // Add request to Q if an app of the same name isn't already being started:
  bool match = false;
  for (int i = 0; i < p_startups.size(); i++) {
    if (p_startups[i].appName == packet.appName) {
      match = true;
      break;
    }
  }
  if (!match) {
    p_startups.push_back(packet);
    
    if (!startMsg.getIntProperty("forcenew")) {

      Target target(packet.appName,"");  // poll only apps of that type
      JMSMessage* msg = newMessage(target);
      // Ids sometimes have trailing / sometimes not so we add if missing
      string contextid = startMsg.getProperty("calcurl");
      if (contextid != "")
        if (contextid.find("http") == 0 &&
            contextid[contextid.length()-1] != '/') {
          contextid.append("/");
      }

      // For tools without a calcurl, use requesters id
      // This can be used by application to ensure that only one
      // helper dialog exists (e.g. viz display style selector)
      if (contextid == "" && origSender != "") {
         contextid = origSender;
      }

      msg->addProperty("contextid", contextid);
      publish("ecce_poll", *msg);
      delete msg;
    }

    p_toAppExec->Start(100, true);
    
  } else {
    // Send the status message to myself to stop the squiggle

    Target target("", getMyID());
    JMSMessage* statusMsg = newMessage(target);
    statusMsg->addProperty("status", "ok");
    publish("ecce_invoke_status", *statusMsg);
    delete statusMsg;
  }
}


/**
 * This is the callback handler used by the ecce_poll_reply topic, which
 * is used by an execer app.  This message is sent in response to an 
 * ecce_poll message, which the execer app sends to determine if there
 * already is a specific application running. 
 */
void WxJMSMessageDispatch::pollReplyMCB(JMSMessage& msg)
{
  string appName = msg.getSender().getName();
  string appID = msg.getSender().getID();

  for (int i = 0; i < p_startups.size(); i++) {
  
    if (appID == p_startups[i].appID) {
      break; // this means that that same app is already invoking something,
             // so exec a new app
    }
    if (appName == p_startups[i].appName && p_startups[i].appID == "" &&
        !p_startups[i].execStarted ) {

      p_startups[i].appID = appID;
      break;
    }
  }
}


/**
 * This method is used internally to keep track of whether the invoke
 * has finished.  An "ecce_invoke_status" message is published to the execer
 * application, letting it know that the invoke process has finished.  
 *   
 */
void WxJMSMessageDispatch::internalInvokeStatusMCB(JMSMessage& msg)
{
  EE_ASSERT(msg.containsProperty("pid"), EE_FATAL, "Invalid status msg.");
  int pid = msg.getIntProperty("pid");
  string status = msg.getProperty("status"); 

  completeStartupProcess(pid, status);

}


void WxJMSMessageDispatch::completeStartupProcess(int pid, const string& status)
{
   // first send the real status message that each app can catch
   Target target("", getMyID());
   JMSMessage* statusMsg = newMessage(target);
   statusMsg->addProperty("status", status);
   publish("ecce_invoke_status", *statusMsg);
   delete statusMsg;

   // now remove that startup packet from the vector
   vector<StartupPacket>::iterator it;

   for (it = p_startups.begin(); it != p_startups.end(); it++) {    
      if ((*it).pidToWatch == pid) {
         p_startups.erase(it);
         break;
      }
   }
}


/**
 * When the execer app receives this message, it knows that
 * the app it just started is alive and ready to receive messages.
 */
void WxJMSMessageDispatch::appReadyMCB(JMSMessage& msg)
{
  string appName = msg.getSender().getName();
  string appID = msg.getSender().getID();

  for (int i = 0; i < p_startups.size(); i++) {
    if (appName == p_startups[i].appName && p_startups[i].execStarted &&
	p_startups[i].appID == "") {
      p_startups[i].appID = appID;
      sscanf(appID.c_str(), "%d", &(p_startups[i].pidToWatch));
      break;
    }
  }
}


/**
 * This method is called after the other apps have had a chance
 * to respond to the poll sent in startApp.  If no response is received 
 * from poll, then exec a new app.  After app is exec'd, call
 * another timeout method to give the startee time to get set up,
 * and send its ready message back to the caller.
 */
void WxJMSMessageDispatch::appExec()
{
  for (int i = 0; i < p_startups.size(); i++) {
    
    if (p_startups[i].execStarted ) {
      continue;
    }
    p_startups[i].execStarted = true;

    string authPipeName = AuthCache::pipeName();

    // If no app replied, exec a new one
    if (p_startups[i].appID == "" ) { // response from poll would set this

      // Fork and exec
      pid_t pid;
      
      // Child process, exec new app 
      if ( (pid = fork()) == 0 ) {
        // Determine command-line arguments in proper format for execv function
        string app = p_startups[i].appName;
        char* argv[4] = {0,0,0,0};
        string binsh = "/bin/sh";
        argv[0] = (char*)binsh.c_str();
        string minc = "-c";
        argv[1] = (char*)minc.c_str();

        ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
        // The InvokArg XML specification should prepend "./" when the app
        // is in the bin directory.  When it isn't, such as for the viewer
        // wrapper script, then don't prepend "./" and $PATH should find it
        string apppath = rd.getTool(app)->getInvokeArg() +
                         " -pipe " + authPipeName;
        argv[2] = (char*)apppath.c_str();
        char path[] = "/bin/sh";

        execv(path, argv);
        cerr << "exec failed!" << endl;
        _exit(-1); // don't flush the buffers, since they are shared by parent
        
        // Parent process, wait for child to finish startup
      } else if (pid > 0 ){
        // pid of child that will be monitored for failure
        p_startups[i].pidToWatch = pid; 

        p_toAppInvoke->Start(100, true);

        AuthCache::getCache().pipeOut(authPipeName);
      } else {
        EE_ASSERT(false, EE_FATAL, "Fork failed!");
      }
      
    } else {
      
      // watch the poll respondee for failure
      sscanf(p_startups[i].appID.c_str(), "%d", &(p_startups[i].pidToWatch));
      appInvoke(); // App already exists, so just directly invoke it
    }

    // only one app exec'd per call
    break;
  }
}


/**
 * The ultimate goal of this function is to send an ecce_invoke
 * message (with the context to invoke) to one and only one app
 * of the tool type designated in the startApp function.
 *
 * This method first checks to see if an app is up and ready for 
 * messaging.  If an app responded to the poll, then
 * p_replierID will have a value, and it is ready for invoke.
 *
 * If an app was exec'd, and it finished starting up, and called
 * the notifyReady() method, then p_lastAppInvoked will have a 
 * value, and it is ready to invoke.
 *
 * After the invoke message is sent, another timeout is called
 * to give the invokee time to either 1) finish and return with a 
 * ecce_invoke_status message, 2) fail (which will be caught by
 * a waitpid() call).
 */
void WxJMSMessageDispatch::appInvoke()
{
  bool needTimeout = false;
  
  for (int i = 0; i < p_startups.size(); i++) {
    
    // Only perform this check if in between exec and invoke stages
    if (p_startups[i].execStarted && !p_startups[i].invokeStarted) {
      
      // We are waiting for new app to start up
      if (p_startups[i].appID == "") { // app hasn't started yet
	
        // First check that target process is still alive, if not, send error
        int status;
        int child;

        // child terminated
        if((child=waitpid(p_startups[i].pidToWatch, &status, WNOHANG)) > 0)  { 

          // Send a failed message to ecce_invoke_status, so caller can handle
          // the failure
          sendFailedStatusMessage(p_startups[i].pidToWatch);
          
        } else {
          // Recursively call the timeout until the newly exec'd app either:
          // 1) fails, or 2) calls notifyReady()
          needTimeout = true;     
        }
        
        // We are calling invoke on an app that is started and ready for 
        // messages
      } else {
        
        // send the invoke message    
        p_startups[i].invokeStarted = true;
        Target invokeTarget("", p_startups[i].appID);
        p_startups[i].startMsg.setTarget(invokeTarget); 
        // forward all the properties 
        // from the orginal request

        publish("ecce_invoke", p_startups[i].startMsg);        
      
        // Now wait for some time and then check the child pid to see
        // if it crashed during the invoke method:
        p_toInvokeStatusCheck->Start(100, true);
      }
    }
  }
  if(needTimeout) {
    //    appInvoke();
    p_toAppInvoke->Start(100, true);
  }
}

void WxJMSMessageDispatch::sendFailedStatusMessage(int pid)
{
   Target myself("", getMyID());

   JMSMessage* errMsg = newMessage(myself);
   errMsg->addProperty("status", "failed");
   errMsg->addIntProperty("pid", pid);
   publish("ecce_internal_invoke_status", *errMsg);
   delete errMsg;
}

/** 
 * This method recursively calls timeouts until 1) the app
 * being invoked fails, or 2) the app sends a ecce_internal_invoke_status
 * message indicating the invoke was successful.
 */
void WxJMSMessageDispatch::invokeStatusCheck()
{
  vector<StartupPacket>::iterator it;
  bool needTimeout = false;

  for (it = p_startups.begin(); it != p_startups.end(); it++) {

    // If not in the last stage of startup (invoke) then ignore
    if (!(*it).invokeStarted) {
      continue;
    }

    int status, pid;
    pid = waitpid((*it).pidToWatch, &status, WNOHANG);

    if(pid > 0)  {  // my child terminated

      sendFailedStatusMessage(pid);

    } else {
      /* This should never happen now that gateway starts all apps

      if (pid < 0) { // This isn't my child - send message out for
                     // its parent to respond
        JMSMessage* msg = newMessage();
        msg->addIntProperty("pid", (*it).pidToWatch);
        publish("ecce_check_child",*msg);
        delete msg;
            
      }
      */ 
      needTimeout = true; // we need to wait again      
    }
  } 
  if (needTimeout) {
    p_toInvokeStatusCheck->Start(100, true);
  }
}


/**
 * Reply to startup poll message.
 * The way we want our tools to work now is
 *  . no context - start new tool
 *  . mismatched context - start new tool
 *  . same context - no new tool
 */
void WxJMSMessageDispatch::pollMCB(JMSMessage& msg)
{
  // Get contextid (probably url) to see if the poll matches my context 
  string contextid = msg.getProperty("contextid");
  string mycontextid = getPollContext();
  if (mycontextid == contextid) {
     Target target("", msg.getSender().getID());
     JMSMessage* reply = newMessage(target);
     publish("ecce_poll_reply", *reply);
     delete reply;
  }
}

/**
 * Set a context string that uniquely identifies the tool context.
 * The context is probably a url.
 * Urls missing the trailing '/' will have it added to avoid
 * problems with comparing two urls differing only in the presense
 * or absense of the trailing '/'.
 */
void WxJMSMessageDispatch::setPollContext(const string& contextId)
{
   p_contextId = contextId;
   if (p_contextId != "" ) {
      // If the context id is a url, make sure it ends with /
      if (p_contextId.find("http") == 0 &&
          p_contextId[p_contextId.length()-1] != '/') {
         p_contextId.append("/");
      }
   }

}

string WxJMSMessageDispatch::getPollContext() const
{
   return p_contextId;
}

void WxJMSMessageDispatch::internalInvokeMCB(JMSMessage& msg)
{
   // Call the app's registered callback function:
   EE_ASSERT(p_invokeMCBPtr, EE_WARNING, 
         "You did not subscribe to ecce_invoke!");

   // moved to before calling invoke to eliminate timing problems with
   // new builder request/response type messages like "getcharge"
   // made companion change in wx code.
   raiseAllWindows();

   WxJMSListener *classPtr = p_invokeMCBPtr->classPtr;
   wxJmsCBFunc cbFunc = p_invokeMCBPtr->funcPtr.classFunc;        
   (classPtr->*cbFunc)(msg);


   // Send an ok message after the call is complete
   Target target("", msg.getSender().getID());
   JMSMessage* internalStatusMsg = newMessage(target);
   internalStatusMsg->addProperty("status","ok");
   internalStatusMsg->addProperty("pid", getMyID());

   publish("ecce_internal_invoke_status", *internalStatusMsg);

   delete internalStatusMsg;
}


/**
 * SIGCHLD signal handler.
 * Schedules a timer so that actual signal handling can be done safely.
 * signal handlers shouldn't do anything that uses system resources.
 * The time on the timer is just some small number - could possibly even
 * be 0.  Our handler will then reap the dead child and clean up properly
 * if the child is an app that died during startup.
 */
void WxJMSMessageDispatch::appQuit(int signum)
{

   if (p_theDispatcher != 0) {  // shouldn't happen but just in case...
      int status;
      int pid = wait3(&status,WNOHANG,NULL);

      SigTimer *timer = new SigTimer(p_theDispatcher, pid);
      timer->Start(10,true);

      // Reschedule signal handler
      signal(SIGCHLD, WxJMSMessageDispatch::appQuit);
   }
}

void WxJMSMessageDispatch::handleSigChild(int pid)
{
  if (pid > 0) {
     vector<StartupPacket>::iterator it;
     for (it = p_startups.begin(); it != p_startups.end(); it++) {    
        if ((*it).pidToWatch == pid) {
           sendFailedStatusMessage(pid);

           break;
        }
     }
  }
}

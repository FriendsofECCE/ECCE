/**
 * @file
 *
 * libmosquitto's thread receives the messages; each is handed to the GUI
 * thread with wxApp::CallAfter, so callbacks run exactly where the socket
 * event handler's did.
 */
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
  using namespace std;

#include "util/ErrMsg.H"
#include "util/JMSMessage.H"
#include "util/MqttLink.H"
#include "wxgui/WxJMSSubscriber.H"

#include "wx/wx.h"

/**
 * Constructor - need to supply name of app/tool in order
 * to be able to identify unique subscriber key
 */
WxJMSSubscriber::WxJMSSubscriber(const string& toolName) {
  p_toolName = toolName;
  p_started = false;
  p_messagingEnabled = (getenv("ECCE_NO_MESSAGING") == NULL) ? true : false;
  if (p_messagingEnabled) {
    p_ep = std::make_shared<MqttEndpoint>(toolName);
    p_ep->handler = [this](const string& t, JMSMessage& m) { deliver(t, m); };
    p_ep->poster = [](std::shared_ptr<MqttEndpoint> ep, MqttInbound in) {
      // The endpoint is held by the closure; process() drops the message if
      // the subscriber was unsubscribed meanwhile.
      if (wxTheApp)
        wxTheApp->CallAfter([ep, in]() { ep->process(in); });
    };
  }
}

/**
 * Destructor - Unsubscribes from JMS
 */
WxJMSSubscriber::~WxJMSSubscriber() {
  unsubscribe();
}

/**
 * Unsubscribes and frees memory used by cb structures.
 */
void WxJMSSubscriber::unsubscribe() {
  if (!p_ep) return;

  for (wxCbMap::iterator it = cbStructs.begin(); it != cbStructs.end(); it++)
    delete (*it).second;
  cbStructs.clear();

  p_ep->kill();
  p_ep->clear();
  MqttLink::instance().deactivate(p_ep);
  p_ep.reset();
  p_started = false;
}

/**
 * Subscribes to every topic added so far.  An app should call this only
 * once, after all subscriptions have been subscribed to.
 */
bool WxJMSSubscriber::startSubscriber() {
  if (!p_ep) return false;
  p_started = true;
  MqttLink::instance().activate(p_ep);
  return true;
}

/**
 * Holds (and discards) all incoming messages for this
 * subscriber until notified otherwise.
 */
bool WxJMSSubscriber::holdMessages() {
  if (!p_ep) return false;
  p_ep->hold(true);
  return true;
}

/**
 * Resumes normal handling of incoming messages.
 */
void WxJMSSubscriber::answerAs(const string& alias, const string& topic)
{
  if (p_ep) p_ep->answerAs(alias, topic);
}


bool WxJMSSubscriber::resumeMessaging() {
  if (!p_ep) return false;
  p_ep->hold(false);
  return true;
}

/**
 * Records the callback; only one subscription per topic.
 */
bool WxJMSSubscriber::subscribeInternal(const char* topicStr,
                                        wxJMSCallbackStructure* cb,
                                        bool filterSelf) {
  bool ret = false;
  EE_ASSERT(cb, EE_FATAL, "cbStruct is NULL!");

  string topic(topicStr);
  if (cbStructs.find(topic) == cbStructs.end()) {
    cbStructs[topic] = cb;
    p_ep->add(topic, filterSelf);
    if (p_started) MqttLink::instance().activate(p_ep);
    ret = true;
  } else {
    EE_RT_ASSERT(false, EE_WARNING,
                 "Subscribe failed! You already subscribed to topic " + topic);
    delete cb;
  }
  return ret;
}

bool WxJMSSubscriber::subscribe(const char* topicStr,
                                WxJMSListener *l,
                                wxJmsCBFunc handler,
                                bool filterSelf) {
  bool ret = false;
  if (p_ep) {
    wxJMSCallbackStructure* cbStruct = new wxJMSCallbackStructure;
    cbStruct->funcPtr.classFunc = handler;
    cbStruct->classPtr = l;
    ret = subscribeInternal(topicStr, cbStruct, filterSelf);
  }
  return ret;
}

/**
 * For static callback functions.
 */
bool WxJMSSubscriber::subscribe(const char* topicStr,
                               wxJmsStaticCBFunc handler,
                               bool filterSelf) {
  bool ret = false;
  if (p_ep) {
    wxJMSCallbackStructure* cbStruct = new wxJMSCallbackStructure;
    cbStruct->funcPtr.staticFunc = handler;
    cbStruct->classPtr = NULL;
    ret = subscribeInternal(topicStr, cbStruct, filterSelf);
  }
  return ret;
}

/**
 * Use this to get your tool name, as used for message identification.
 */
string WxJMSSubscriber::getMyName() const {
  return p_toolName;
}

/**
 * Use this to get your process ID (pid), as used for message identification.
 */
string WxJMSSubscriber::getMyID() const {
  return std::to_string(getpid());
}

/**
 * GUI thread: finds the callback for the topic and makes the call.
 */
void WxJMSSubscriber::deliver(const string& topic, JMSMessage& msg) {
  wxCbMap::iterator it = cbStructs.find(topic);
  if (it == cbStructs.end()) return;

  wxJMSCallbackStructure *cbStruct = (*it).second;
  WxJMSListener *classPtr = cbStruct->classPtr;
  if (classPtr != NULL)
    (classPtr->*(cbStruct->funcPtr.classFunc))(msg);
  else
    (*cbStruct->funcPtr.staticFunc)(msg);
}

bool WxJMSSubscriber::messagingEnabled() {
  return p_messagingEnabled;
}

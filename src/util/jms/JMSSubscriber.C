///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: JMSSubscriber.C
//
// DESIGN:
//    One MqttEndpoint per subscriber.  libmosquitto's thread queues the
//    messages and writes to the endpoint's self-pipe; getSocketID() is that
//    pipe's read end, so the owner's loop (poll() in the job store)
//    calls processMessage() on its own thread.
///////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <unistd.h>

#include "util/ErrMsg.H"
#include "util/JMSMessage.H"
#include "util/JMSSubscriber.H"
#include "util/MqttLink.H"

JMSSubscriber::JMSSubscriber(const string& toolName) {
  p_toolName = toolName;
  p_started = false;
  p_messagingEnabled = (getenv("ECCE_NO_MESSAGING") == NULL) ? true : false;
  if (p_messagingEnabled) {
    p_ep = std::make_shared<MqttEndpoint>(toolName);
    p_ep->enablePipe();
    p_ep->handler = [this](const string& t, JMSMessage& m) { deliver(t, m); };
  }
}

JMSSubscriber::~JMSSubscriber() {
  unsubscribe();
}

int JMSSubscriber::getSocketID() {
  return p_ep ? p_ep->pipeFd() : -1;
}

void JMSSubscriber::unsubscribe() {
  if (!p_ep) return;
  for (cbMap::iterator it = cbStructs.begin(); it != cbStructs.end(); it++)
    delete (*it).second;
  cbStructs.clear();
  p_ep->kill();
  p_ep->clear();
  MqttLink::instance().deactivate(p_ep);
  p_ep.reset();
  p_started = false;
}

// Must come after all subscribe calls: the broker is told about every
// topic at once.
bool JMSSubscriber::startSubscriber() {
  if (!p_ep) return false;
  p_started = true;
  MqttLink::instance().activate(p_ep);
  return true;
}

bool JMSSubscriber::holdMessages() {
  if (!p_ep) return false;
  p_ep->hold(true);
  return true;
}

bool JMSSubscriber::resumeMessaging() {
  if (!p_ep) return false;
  p_ep->hold(false);
  return true;
}

bool JMSSubscriber::subscribeInternal(const char* topicStr,
                                      JMSCallbackStructure* cb,
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

bool JMSSubscriber::subscribe(const char* topicStr,
                              jmsCBFunc handler,
                              bool filterSelf) {
  bool ret = false;
  if (p_ep) {
    JMSCallbackStructure* cbStruct = new JMSCallbackStructure;
    cbStruct->funcPtr.classFunc = handler;
    cbStruct->classPtr = this;
    ret = subscribeInternal(topicStr, cbStruct, filterSelf);
  }
  return ret;
}

bool JMSSubscriber::subscribe(const char* topicStr,
                              jmsStaticCBFunc handler,
                              bool filterSelf) {
  bool ret = false;
  if (p_ep) {
    JMSCallbackStructure* cbStruct = new JMSCallbackStructure;
    cbStruct->funcPtr.staticFunc = handler;
    cbStruct->classPtr = NULL;
    ret = subscribeInternal(topicStr, cbStruct, filterSelf);
  }
  return ret;
}

string JMSSubscriber::getMyName() const {
  return p_toolName;
}

string JMSSubscriber::getMyID() const {
  return std::to_string(getpid());
}

void JMSSubscriber::processMessage() {
  // A callback may unsubscribe; keep the endpoint alive until it returns.
  std::shared_ptr<MqttEndpoint> ep = p_ep;
  if (ep) ep->drain();
}

void JMSSubscriber::deliver(const string& topic, JMSMessage& msg) {
  cbMap::iterator it = cbStructs.find(topic);
  if (it == cbStructs.end()) return;
  JMSCallbackStructure *cbStruct = (*it).second;
  if (cbStruct->classPtr != NULL)
    (cbStruct->classPtr->*(cbStruct->funcPtr.classFunc))(msg);
  else
    (*cbStruct->funcPtr.staticFunc)(msg);
}

bool JMSSubscriber::messagingEnabled() {
  return p_messagingEnabled;
}

///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: JMSSubscriber.H
//
//
// CLASS SYNOPSIS:
//    Publish/subscribe messaging over the process's MQTT connection
//    (util/MqttLink.H).  Messages are queued by libmosquitto's thread and
//    delivered on the caller's thread by processMessage(), which a loop
//    that is not thread safe (Xt) runs when getSocketID() is readable.
//
// EXPORTED TYPES:
//    Class JMSSubscriber
//
// DESCRIPTION:
//    This class provides asynchronous subscriptions using the pub/sub model.
//    This class allows only 1 subscription per topic (subscriptions are 
//    cached).
//    This class assumes that all registered callback functions are members
//    of itself, so that means you need to inherit from JMSSubscriber if
//    you want to subscribe with a callback function.
//    
//    
///////////////////////////////////////////////////////////////////////////////
#ifndef JMSSUBSCRIBER_HH
#define JMSSUBSCRIBER_HH

// System includes
   #include <map>
   #include <memory>
   #include <string>

// Library includes:

// Portability for SGI/linux build:
     using std::map;
     using std::string;
     using std::less;


class JMSSubscriber;
class JMSMessage;
class MqttEndpoint;
class Target;

// Generic typedef for callback function pointer
   typedef void (JMSSubscriber::*jmsCBFunc)(JMSMessage&);
   typedef void (*jmsStaticCBFunc)(JMSMessage&);

// Struct for holding cbfunction and class pointers
   union CBFuncPtr {
     jmsCBFunc classFunc;
     jmsStaticCBFunc staticFunc;
   };
   struct JMSCallbackStructure {     
     CBFuncPtr funcPtr;
     JMSSubscriber* classPtr;
   };

// Typedef to simplify dealing with maps
   typedef map<string, JMSCallbackStructure*, less<string> > cbMap;

class JMSSubscriber {

public:

  // Constructors
     JMSSubscriber(const string& toolName);   

  // Virtual Destructor
     virtual ~JMSSubscriber();
     void destroy();

  // ACCESSORS
     string getMyName() const;  
     string getMyID() const;
     int    getSocketID();
     bool   messagingEnabled();

  // SUBSCRIBE METHODS

     // Registers a message handler for ALL messages posted to this topic
     bool subscribe(const char* topic, jmsCBFunc handler,
                    bool filterSelf = true);

     bool subscribe(const char* topic, jmsStaticCBFunc handler,
                    bool filterSelf = true);

     // Starts the subscriber connection for this subscriber
     bool startSubscriber();

     // Unsubscribes all topics for this subscriber
     void unsubscribe();

    // When getSocketID() is readable, use this to run the callbacks of
    // the messages that have arrived:
    void processMessage();

    // If you want to temporarily suspend messaging:
    bool holdMessages();

    // If you want to start messaging up again:
    bool resumeMessaging();

private:

  bool subscribeInternal(const char* topic, JMSCallbackStructure* cb,
                         bool filterSelf);
  void deliver(const string& topic, JMSMessage& msg);

  // holds the Ecce app's callback classes & functions (one pair for each
  // subscription)
  cbMap cbStructs;

  // Tool info used to stamp messages:
  string p_toolName;
  bool p_messagingEnabled;
  bool p_started;

  std::shared_ptr<MqttEndpoint> p_ep;

}; // JMSSubscriber

#endif /* JMSSUBSCRIBER_HH */

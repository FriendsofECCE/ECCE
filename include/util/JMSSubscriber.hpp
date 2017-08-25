///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: JMSSubscriber.H
//
//
// CLASS SYNOPSIS:
//    Provides an interface to the Java Messaging Service (JMS) using
//    the Java Native Interface (JNI).
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
   #include <string>

// Library includes:

// Portability for SGI/linux build:
     using std::map;
     using std::string;
     using std::less;


class JMSSubscriber;
class JMSMessage;
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
     int    getPort();
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

    // When you are notified of a message, use this to read the message
    // from the socket and call the callback function:
    void processMessage();

    // If you want to temporarily suspend messaging:
    bool holdMessages();

    // If you want to start messaging up again:
    bool resumeMessaging();

private:

  // Methods
  void initSocket();
  bool subscribeInternal(const char* topic, JMSCallbackStructure* cb,
                         bool filterSelf);

  // Data:

  // holds the Ecce app's callback classes & functions (one pair for each
  // subscription)
  cbMap cbStructs;

  // Tool info used to stamp messages:
  string p_toolName;
  bool p_messagingEnabled;

  // Socket stuff
  int p_inPort;
  int p_inputSocket;

}; // JMSSubscriber

#endif /* JMSSUBSCRIBER_HH */



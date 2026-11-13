///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: JMSPublisher.H
//
//
// CLASS SYNOPSIS:
//    Provides an interface to the Java Messaging Service (JMS) using
//    the Java Native Interface (JNI).
//
// EXPORTED TYPES:
//    Class JMSPublisher
//
// DESCRIPTION:
//    User can publish messages via JMS.  Caches publishers.
//    
//    
///////////////////////////////////////////////////////////////////////////////
#ifndef JMSPUBLISHER_HH
#define JMSPUBLISHER_HH

// System includes
   #include <string>

// Library includes:

// Portability for SGI/linux build:
     using std::string;


class JMSMessage;
class Target;


class JMSPublisher {

public:

  // Constructors
     JMSPublisher(const string& toolName); // use this to identify yourself

  // Virtual Destructor
     virtual ~JMSPublisher(void);


  // ACCESSORS
     string getMyName() const;  
     string getMyID() const;
     // A dispatcher stamps its subscriber's instance id (#983).
     void setMyID(const string& id) { p_id = id; }

  // ACTING TOOL (#983): with several tools in one process, a publisher
  // shared by them (EDSIFactory's) stamps the tool whose code is running,
  // so each tool's self filter drops its own notices and only those.
  // Only with MqttLink::localDelivery(); otherwise nothing changes.
     void followActingTool(bool on) { p_followActing = on; }
     // The process's default (the Organizer in one process).
     static void setActingTool(const string& name, const string& id);
     // Overrides the default while in scope (a tool window saving).
     class ActingTool {
     public:
       ActingTool(const string& name, const string& id);
       ~ActingTool();
     private:
       string p_prevName, p_prevId;
       ActingTool(const ActingTool&) = delete;
       ActingTool& operator=(const ActingTool&) = delete;
     };


  // PUBLISH METHODS

     // publish a regular message - no reply
     bool publish(const string& topic,
                  const JMSMessage& message) const;

     bool invoke(JMSMessage& msg, 
                 const string& appName, 
                 int forcenew, 
                 const string& url="",
                 const string& actionTopic="");

     bool test();

  // MESSAGE FACTORY methods:

     // Use this to create new  messages - it will stamp your sender info
     JMSMessage* newMessage() const;
     JMSMessage* newMessage(const Target& target) const;


private:

  // Data:

  // Tool info used to stamp messages:
  string p_toolName;
  string p_id;               // empty: the pid
  bool p_followActing = false;
  static bool p_messagingEnabled;
  static string p_actingName, p_actingId;


}; // JMSPublisher

#endif /* JMSPUBLISHER_HH */



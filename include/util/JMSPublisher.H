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
  static bool p_messagingEnabled;


}; // JMSPublisher

#endif /* JMSPUBLISHER_HH */



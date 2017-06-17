/**
* @file
*
*
*/
#ifndef JMSMESSAGE_HH
#define JMSMESSAGE_HH

// System includes
#include <string>
#include <map>

// Portability for SGI/linux build:
  using std::string;
  using std::map;
  using std::less;


// Constants 

// Max size of a datagram packet sent between C++ and java
// (Have to put this up here so I can use it to declare byte arrays)
// (Hopefully there aren't any name collisions)
const size_t MAX_PACKET_LENGTH = 4096;

// The following are constants used in various messages.  There was no
// good place to put them, so I just dumped them here.
// Values of argument for Ecce_Quit
static const int QuitNow = 1;
static const int QuitGracefully = 0;

// Values of argument to Ecce_Set_Iconified
static const int Iconic = 1;
static const int NonIconic = 0;



/**
 * Parent class for Sender and Target objects - they can be subclassed
 * later if we want to add more data to them
 */
class  MessageParticipant {

public:
  
  // Constructors
  MessageParticipant();
  MessageParticipant(const string& name, const string& id);

  // Virtual Destructor
  virtual ~MessageParticipant();

  // Make all methods virtual so they can be subclassed properly
  
  // Accesstors
  virtual string getName() const;
  virtual string getID() const;
  virtual string toString() const;

  // Modifiers
  virtual void setName(const string& name);
  virtual void setID(const string& id);
  virtual void loadFromString(const string& mpStr); 

private:

  // data
  string p_name;
  string p_id;

};



/**
 * Sender class - used to identify sender of message
 */
class Sender : public MessageParticipant {
public:
  Sender(const string& name, const string& id);
  Sender();
};



/** 
 * Target class - used to identify recipient of message
 */
class Target : public MessageParticipant {
public:
  // for now, there is no difference, but we might want to overload
  // methods if we want target to contain more info (like calc id)
  Target(const string& name, const string& id);
  Target();
};



/** 
 * Message object passed back to subscriber's callback function.
 * When the Open JMS server sends an incoming message to the java
 * subscriber, the java code uses JNI to call a C++ handler, which
 * then creates a JMSMessage object and passes it to the true cb function.
 */
class JMSMessage {

public:

  // Constructors
     JMSMessage();  // generally don't use this constructor
     JMSMessage(const Sender& sender);

  // Virtual Destructor
     virtual ~JMSMessage();

  // Modifiers
     void addProperty (const string& key, const string& value);
     void addIntProperty(const string& key, const int& value);
     void setReplyTopic(const string& topicName);
     void setTarget(const Target& target);
     void setSender(const Sender& sender);

  // Accessors
     bool   containsProperty(const string& key) const;
     string getProperty(const string& key) const;
     int    getIntProperty(const string& key) const;
     bool   isRepliable() const;
     string getReplyTopic() const;
     Target getTarget() const;
     Sender getSender() const;

     // Methods for handling "original" sender
     // which is different when using point-to-point messaging for application
     // started by a 3rd party (ie the gateway)
     // These are made messages just to group them into an obvious coherent
     // (ok semi-coherent) capability.  We've had lots of bugs in this area
     // If you are creating a message that starts an app (invoke) AND you 
     // expect point to point communications messaging, call the
     // markAsPointToPoint.  Dispatcher handles things under the hood
     void markAsPointToPoint();
     string getOriginalSenderName() const;
     string getOriginalSenderId() const;

  // String conversion functions used to marshal data between
  // C++ and java over the JNI - some sort of parsing is going on,
  // but we can change the parsing later if we like
     string targetToString() const;
     string senderToString() const;
     string bodyToString() const;

     void loadBody(const string& body);
     void loadSender(const string& sender);
     void loadTarget(const string& target);

private:

  // Methods:
     void addItem(const string& itemName, const string& item,
               string& packet) const;
     bool getItem(const string& itemName, string& item, string& packet);

  // what is the content of the message (as key/value pairs)
  map<string, string, less<string> > p_properties;

  // who sent the message
  Sender p_sender;

  // who is the message directed to
  Target p_target;

  // what is the special reply topic, if message was sent via 
  // a JMSMessageDispatch::publishRequest call
  string p_replyTopicName;


}; 



/** 
 * Struct of data needed to start up an app:
 */
struct StartupPacket {
  JMSMessage startMsg;
  string appName;
  string appID;
  int    pidToWatch;
  string invokeStatus;
  bool   invokeStarted;
  bool   execStarted;
};



class DatagramUtil {
  
public :

  // Methods:
  static string getItem(const string& itemName, const string& packet);

  static int    getServerPort();

  // Use this function to force a re-load from port file:
  static void   loadServerPort();

  static void   addItem(const string& itemName, const string& item,
                        string& packet);
  static void   addIntItem(const string& itemName, const int& item,
                           string& packet);
  static void   addBoolItem(const string& itemName, const bool& item,
                            string& packet);

  static bool   sendPacket(const string& packet);
  

private :
  static string getEnv(const char* propertyName);
  static int JMSDISPATCH_PORT;
  
};


#endif


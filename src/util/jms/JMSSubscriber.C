//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: JMSSubscriber.C
//
//
// DESIGN:
//
///////////////////////////////////////////////////////////////////////////////

// System includes:
   #include <sys/types.h>
   #include <unistd.h>
   #include <fcntl.h>

   #include <stdio.h>
   #include <stdlib.h>
   #include <sys/socket.h>
   #include <netinet/in.h>

#include <fstream>
#include <strstream>
using namespace std;
   #include "util/SFile.H"

// Application includes:
   #include "util/JMSSubscriber.H"
   #include "util/JMSMessage.H"
   #include "util/ErrMsg.H"

//#define debug

/*******************************************************************
 Method : Constructor 
 Summary: 
********************************************************************/
JMSSubscriber::JMSSubscriber(const string& toolName) {

  // Initialize variables:
  p_toolName = toolName;  // To identify unique subscriber key
  p_inPort = -1;
  p_inputSocket = -1;
  p_messagingEnabled =  (getenv("ECCE_NO_MESSAGING") == NULL) ? true : false;
  
  if (p_messagingEnabled)
    initSocket();
}

/*******************************************************************
 Method : initSocket
 Summary: Initializes the server socket that is used for incoming
          messages from the embedded java virtual machine.  Datagram
          packets are sent from the java jms message handler to
          this socket.
********************************************************************/
void JMSSubscriber::initSocket() {
 
  // Create datagram socket:
  int s;
  struct sockaddr_in addIN;

  if ( (s = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
    perror (("Can't create subscriber socket for " + getMyName()).c_str());

  } else {

    addIN.sin_family      = AF_INET;
    addIN.sin_port        = htons(0); // let the port get chosen dynamically
    addIN.sin_addr.s_addr = INADDR_ANY;
    
    if ( bind(s, (struct sockaddr *)&addIN, sizeof(addIN) ) < 0 ) {
      perror (("Can't bind subscriber socket for + " + getMyName()).c_str());
      close(s);

    } else {
    
#ifdef __GNUC__
      socklen_t len = sizeof(addIN);
#else
      int len = sizeof(addIN);
#endif
      if (getsockname(s, (struct sockaddr *) &addIN, &len) < 0) {
        perror ("getsockname() failed.  Can't determine subscriber port!");
        close(s);

      } else {
        p_inPort = ntohs(addIN.sin_port);
#ifdef debug
        cout << getMyName() << " listening on port " << p_inPort << endl;
#endif
        p_inputSocket = s;

        // Now make this socket nonblocking so it doesn't hang XWindows:
        if (fcntl(p_inputSocket, F_SETFL, O_NONBLOCK) < 0) {
          perror (("Error making subscriber socket nonblocking for app + " 
                   + getMyName()).c_str());
        }
      }
    }
  }
}

int JMSSubscriber::getPort() {
  return p_inPort;

}

int JMSSubscriber::getSocketID() {  
  return p_inputSocket;
}

/*******************************************************************
 Method : Destructor 
 Summary: 
********************************************************************/
JMSSubscriber::~JMSSubscriber() {
  unsubscribe();
}

/*******************************************************************
 Method : unsubscribe
 Summary: Unsubscribes to JMS and frees memory used by
          cb structures.
********************************************************************/
void JMSSubscriber::unsubscribe() {

  if (p_messagingEnabled && p_inPort != -1) {

    // delete the map of callback structures
    cbMap::iterator it;
    for (it = cbStructs.begin(); it != cbStructs.end(); it++) {
      
      if ((*it).second != NULL)
        delete (*it).second;
    }
    cbStructs.clear();
    
    // close the connection to the jms server
    string packet;
    DatagramUtil::addItem("METHOD", "unsubscribe", packet);
    DatagramUtil::addItem("NAME", getMyName(), packet);
    DatagramUtil::addItem("ID", getMyID(), packet);
    DatagramUtil::sendPacket(packet);
    
    // close the server socket
    close(p_inputSocket);
    
    p_inputSocket = -1;
    p_inPort = -1;
    
  }  
}

/*******************************************************************
 Method : startSubscriber
 Summary: Starts the connection to activate all subscriptions
          for this subscriber.  An app should call this only once,
          after all subscriptions have been subscribed to.
********************************************************************/
bool JMSSubscriber::startSubscriber() {

  bool ret = false;

  if (p_messagingEnabled && p_inPort != -1) {
      
    // Create the packet to server to start up subscriber connection
    string packet;
    DatagramUtil::addItem("METHOD", "startsubscriber", packet);
    DatagramUtil::addItem("NAME", getMyName(), packet);
    DatagramUtil::addItem("ID", getMyID(), packet);

    // Send the packet
    ret = DatagramUtil::sendPacket(packet);
    if (ret == false) {
      EE_RT_ASSERT(false, EE_WARNING, "Error sending packet");
    }
  }  
  return ret;
}

/*******************************************************************
 Method : holdMessages
 Summary: Holds (and discards) all incoming messages for this 
          subscriber until notified otherwise.
********************************************************************/
bool JMSSubscriber::holdMessages() {
 
  bool ret = false;

  // Create the hold packet
  string packet;
  DatagramUtil::addItem("METHOD", "hold", packet);
  DatagramUtil::addItem("NAME", getMyName(), packet);
  DatagramUtil::addItem("ID", getMyID(), packet);

  // Send the packet
  ret = DatagramUtil::sendPacket(packet);
    
  if (ret == false) {
    EE_RT_ASSERT(false, EE_WARNING, "Error sending hold packet");
  }
  
  return ret;  
}
/*******************************************************************
 Method : resumeMessaging
 Summary: Resumes normal handling of incoming messages.
********************************************************************/
bool JMSSubscriber::resumeMessaging() {
 
  bool ret = false;

  // Create the hold packet
  string packet;
  DatagramUtil::addItem("METHOD", "resume",packet);
  DatagramUtil::addItem("NAME", getMyName(), packet);
  DatagramUtil::addItem("ID", getMyID(), packet);

  // Send the packet
  ret = DatagramUtil::sendPacket(packet);
    
  if (ret == false) {
    EE_RT_ASSERT(false, EE_WARNING, "Error sending hold packet");
  }
  
  return ret;  
}

/*******************************************************************
 Method : subscribeInternal
 Summary: Does the real subscribe.  Should only be called if
          messaging is enabled and the receive port/socket are valid.
********************************************************************/
bool JMSSubscriber::subscribeInternal(const char* topicStr,
                                      JMSCallbackStructure* cb,
                                      bool filterSelf) {
 
  bool ret = false;
  EE_ASSERT(cb, EE_FATAL, "cbStruct is NULL!");

  // First check that the user hasn't already subscribed to this topic:
  string topic(topicStr);
  cbMap::iterator it = cbStructs.find(topic);
  string errMsg = "Subscribe failed!";
  
  if (it == cbStructs.end()) {
    
    // Create the subscribe packet
    string packet;
    DatagramUtil::addItem("METHOD", "subscribe",packet);
    DatagramUtil::addItem("TOPIC", topicStr,packet);
    DatagramUtil::addItem("NAME", getMyName(), packet);
    DatagramUtil::addItem("ID", getMyID(), packet);
    DatagramUtil::addIntItem("PORT", p_inPort, packet);
    DatagramUtil::addBoolItem("SELFFILTER", filterSelf, packet);
    
    // Only one subscription per topic...cache the callback 
    //structures:
    cbStructs[topic] = cb;
    ret = DatagramUtil::sendPacket(packet);
    
  } else {
    errMsg += "You already subscribed to topic " + topic;
    delete cb;
  }
    
  if (ret == false) {
    EE_RT_ASSERT(false, EE_WARNING, errMsg);
  }
  
  return ret;  
}

/*******************************************************************
 Method : subscribe
 Summary: Uses JMS to subscribe to a message, filtering on the given
          target.
********************************************************************/
bool JMSSubscriber::subscribe(const char* topicStr,
                              jmsCBFunc handler,
                              bool filterSelf) {
  bool ret = false;

  if (p_messagingEnabled && p_inPort != -1) {
    // Make a new callback structure:
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

  if (p_messagingEnabled && p_inPort != -1) {
  
    // Make a new callback structure:
    JMSCallbackStructure* cbStruct = new JMSCallbackStructure;
    cbStruct->funcPtr.staticFunc = handler;
    cbStruct->classPtr = NULL;
    
    ret = subscribeInternal(topicStr, cbStruct, filterSelf);
  }
  return ret;
}


/*******************************************************************
 Method : getToolName
 Summary: Use this to get your tool name, as used for message
          identification.
********************************************************************/
string JMSSubscriber::getMyName() const {
  return p_toolName;
}

/*******************************************************************
 Method : getProcessID
 Summary: Use this to get your process ID (pid), as used for message
          identification.
********************************************************************/
string JMSSubscriber::getMyID() const {

  // Determine unique process ID for identification and monitoring
  // purposes using waitpid on that specific ID (platform specific):
  int pid;

  pid = getpid();

  char buf[100];
  sprintf(buf, "%d", pid);
  return buf;
}

/*************************************************************************
 Method : processMessage
 Summary: 
          Read the datagram socket, parse it into a message, a class
          pointer, and a callback function pointer.  Then make the call.
 *************************************************************************/
void JMSSubscriber::processMessage() {

  if (p_messagingEnabled && p_inPort != -1) {

    ssize_t size;
    char buf[MAX_PACKET_LENGTH + 1];
    string packet;
    
    /*
     * This should only be called when a datagram packet is present on 
     * the socket.  However, we have found this to not be the case. So,
     * p_inputSocket recv is nonblocking, and we ignore case where
     * size = -1.
     */
    size = recv(p_inputSocket, buf, 
                sizeof(char)*MAX_PACKET_LENGTH, 0);

    ostrstream os;
    os << getMyName() << " received packet " << size << endl;
    if (size != -1) { // a packet was indeed there
      buf[size] = '\0';
      packet = buf;
      
      // Create and parse the message
      JMSMessage msg;
      msg.loadBody(DatagramUtil::getItem("BODY", packet));
      msg.loadSender(DatagramUtil::getItem("SENDER", packet));
      msg.loadTarget(DatagramUtil::getItem("TARGET", packet));
      os << msg.targetToString() << " " << msg.senderToString() << " " << msg.bodyToString() << endl;
      
      // Create the callback:
      string topic = DatagramUtil::getItem("TOPIC", packet);
      
      cbMap::iterator it = cbStructs.find(topic);
      if (it != cbStructs.end()) {     
        JMSCallbackStructure *cbStruct = (*it).second;
        JMSSubscriber *classPtr = cbStruct->classPtr;
        
        if (classPtr != NULL) {
          jmsCBFunc cbFunc = cbStruct->funcPtr.classFunc;        
          (classPtr->*cbFunc)(msg);

        } else {
          jmsStaticCBFunc cbFunc = cbStruct->funcPtr.staticFunc;
          (*cbFunc)(msg);
        }
      }

    } else { // No packet was there - false alarm
      EE_ASSERT(0, EE_WARNING, 
                "No packet was available, but XWindows socket "
                "listener thought so.");
    }
    /*
    SFile debug("/tmp/jmsdebug.txt");
    if (debug.exists()) {
       os << endl << ends;
       ofstream ofs("/tmp/jmsdebug.txt",ios::app);
       ofs << os.str();
       ofs.close();
    }
    */
    
  }
}

bool JMSSubscriber::messagingEnabled() {
  return p_messagingEnabled;
}


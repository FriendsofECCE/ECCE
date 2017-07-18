/**
 * @file 
 *
 *
 */
#include <iostream>
  using namespace std;


#include <fstream>
#include <strstream>
using namespace std;
   #include "util/SFile.H"


#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "util/JMSMessage.H"
#include "util/ErrMsg.H"
#include "wxgui/WxJMSSubscriber.H"

#include "wx/wx.h"
#include "wx/socket.h"



//Wx Event Table - Identifies event handler for input socket
const long SOCKET_EVENT_ID = wxNewId();
BEGIN_EVENT_TABLE(WxJMSSubscriber, wxEvtHandler)
  EVT_SOCKET(SOCKET_EVENT_ID,  WxJMSSubscriber::processMessage)
END_EVENT_TABLE()



/**
 * Constructor - need to supply name of app/tool in order
 * to be able to identify unique subscriber key
 */
WxJMSSubscriber::WxJMSSubscriber(const string& toolName) {

  p_toolName = toolName;
  p_inputSocket = 0;
  p_inPort = -1;
  p_messagingEnabled = (getenv("ECCE_NO_MESSAGING") == NULL) ? true : false;
  if (p_messagingEnabled)
    initSocket();
}



/**
 * Destructor - Unsubscribes from JMS
 */
WxJMSSubscriber::~WxJMSSubscriber() {
  unsubscribe();
}



/**
 * Initializes server socket that is used for incoming
 * messages from the embedded java virtual machine.  Datagram
 * packets are sent from the java jms message handler to
 * this socket.
 */
void WxJMSSubscriber::initSocket() {

  // Create datagram socket, Let Wx select a port for us - (Service(0))
  wxIPV4address bindAddress;
  bindAddress.Service(0); 
  p_inputSocket = new wxDatagramSocket(bindAddress);
  
  // Check state of new socket, if its OK:
  //   Make sure event notification is disabled until subscription is started
  //   Save port number for future reference
  //   Configure socket as non-blocking
  if (p_inputSocket->Ok()) {
    p_inputSocket->Notify(false);
    p_inputSocket->GetLocal(bindAddress);
    p_inPort = bindAddress.Service();
  //cout << "init socket ok" << p_toolName << " " << p_inPort << endl;
    p_inputSocket->SetFlags(wxSOCKET_NOWAIT);
  }
  else {
    cerr << "Subscriber socket creation failed." << endl;
  }
}



/**
 * Returns Port bound to input socket.  Port is -1 if
 * it has not been assigned or an error occurred when the
 * input socket was created. */
int WxJMSSubscriber::getPort() {
  return p_inPort;

}



/**
 * Unsubscribes to JMS and frees memory used by cb structures.
 */
void WxJMSSubscriber::unsubscribe() {

  if (p_messagingEnabled && p_inPort != -1) {

    // delete the map of callback structures
    wxCbMap::iterator it;
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
    
    // safely deletes input socket and deactivates event handler.
    p_inputSocket->Destroy();

    p_inputSocket = 0;
    p_inPort = -1;
  }  
}



/**
 * Starts the connection to activate all subscriptions
 * for this subscriber.  An app should call this only once,
 * after all subscriptions have been subscribed to.  Also,
 * socket event notifications are turned on so incomming
 * messages may be processed.
 */
bool WxJMSSubscriber::startSubscriber() {

  bool ret = false;

  if (p_messagingEnabled && p_inPort != -1) {

    // Enablet socket event handler for input events
    p_inputSocket->SetEventHandler(*this, SOCKET_EVENT_ID);
    p_inputSocket->SetNotify(wxSOCKET_INPUT_FLAG);
    p_inputSocket->Notify(true);
      
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



/**
 * Holds (and discards) all incoming messages for this 
 * subscriber until notified otherwise.
 */
bool WxJMSSubscriber::holdMessages() {
 
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



/**
 * Resumes normal handling of incoming messages.
 */
bool WxJMSSubscriber::resumeMessaging() {
 
  bool ret = false;

  // Create the resume packet
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



/**
 * Does the real subscribe.  Should only be called if messaging 
 * is enabled and the receive port/socket are valid.
 */
bool WxJMSSubscriber::subscribeInternal(const char* topicStr,
                                        wxJMSCallbackStructure* cb,
                                        bool filterSelf) {
  bool ret = false;
  EE_ASSERT(cb, EE_FATAL, "cbStruct is NULL!");

  // First check that the user hasn't already subscribed to this topic:
  string topic(topicStr);
  wxCbMap::iterator it = cbStructs.find(topic);
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



/**
 * Uses JMS to subscribe to a message, filtering on the given target.
 */
bool WxJMSSubscriber::subscribe(const char* topicStr,
                                WxJMSListener *l,
                                wxJmsCBFunc handler,
                                bool filterSelf) {
  bool ret = false;

  if (p_messagingEnabled && p_inPort != -1) {
    // Make a new callback structure:
    wxJMSCallbackStructure* cbStruct = new wxJMSCallbackStructure;
    cbStruct->funcPtr.classFunc = handler;
    cbStruct->classPtr = l;
    ret = subscribeInternal(topicStr, cbStruct, filterSelf);
  }
  return ret;
}



/**
 * Uses JMS to subscribe to a message, filtering on the given target.
 * For static callback functions.
 */
bool WxJMSSubscriber::subscribe(const char* topicStr,
                               wxJmsStaticCBFunc handler,
                               bool filterSelf) {
  bool ret = false;

  if (p_messagingEnabled && p_inPort != -1) {
  
    // Make a new callback structure:
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

  // Determine unique process ID for identification and monitoring
  // purposes using waitpid on that specific ID (platform specific):
  int pid = getpid();

  char buf[100];
  sprintf(buf, "%d", pid);
  return buf;
}



/**
 * Read the datagram socket, parse it into a message, a class
 * pointer, and a callback function pointer.  Then make the call.
 */
void WxJMSSubscriber::processMessage(wxSocketEvent& event) {

  if (p_messagingEnabled && p_inPort != -1) {

    wxUint32 packetSize;
    char buf[MAX_PACKET_LENGTH + 1];
    string packet;
    
    /*
     * This should only be called when a datagram packet is present on 
     * the socket.  However, we have found this to not be the case. So,
     * p_inputSocket recv is nonblocking, and we ignore case where
     * there was nothing to read -or- an error occurred.
     */
    p_inputSocket->Read(buf, sizeof(char)*MAX_PACKET_LENGTH);
    packetSize = p_inputSocket->LastCount();
    ostrstream os;
    os << getMyName() << " received packet " << packetSize << endl;
    if ((!p_inputSocket->Error() > 0) && 
        (packetSize > 0)) { 

      buf[packetSize] = '\0';
      packet = buf;
      
      // Create and parse the message
      JMSMessage msg;
      msg.loadBody(DatagramUtil::getItem("BODY", packet));
      msg.loadSender(DatagramUtil::getItem("SENDER", packet));
      msg.loadTarget(DatagramUtil::getItem("TARGET", packet));
      os << msg.targetToString() << " " << msg.senderToString() << " " << msg.bodyToString() << endl;
      
      // Create the callback:
      string topic = DatagramUtil::getItem("TOPIC", packet);
      
      wxCbMap::iterator it = cbStructs.find(topic);
      if (it != cbStructs.end()) {     
        wxJMSCallbackStructure *cbStruct = (*it).second;
        WxJMSListener *classPtr = cbStruct->classPtr;
        
        if (classPtr != NULL) {
          wxJmsCBFunc cbFunc = cbStruct->funcPtr.classFunc;        
          (classPtr->*cbFunc)(msg);

        } else {
          wxJmsStaticCBFunc cbFunc = cbStruct->funcPtr.staticFunc;
          (*cbFunc)(msg);
        }
      }

    } else { // No packet was there - false alarm
      EE_ASSERT(0, EE_WARNING, 
                "No packet was available, but wxSocket "
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



bool WxJMSSubscriber::messagingEnabled() {
  return p_messagingEnabled;
}


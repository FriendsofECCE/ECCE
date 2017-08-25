/**
 * @file
 *
 *
 */
#include <iostream>
using std::cerr;
using std::endl;

#include <fstream>
using std::ifstream;

#include <vector>
using std::vector;

#include <sys/socket.h>   // defines socket functions & data types
#include <netinet/in.h>   // defines internet domain addressing structure
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "util/JMSMessage.H"
#include "util/StringTokenizer.H"
#include "util/ErrMsg.H"
#include "util/Ecce.H"
#include "util/STLUtil.H"


/**
 * Default Constructor.
 * Creates a blank message.
 */
JMSMessage::JMSMessage() 
{
}  
  

/** 
 * Constructor
 * Stamps the sender information on the message.  For now this
 * includes the tool name and process id...but this can change if
 * we need to.
 */
JMSMessage::JMSMessage(const Sender& sender) 
{
    p_sender = sender;
    // p_target and p_replyTopicName are blank
}



/**
 * Destructor.
 */
JMSMessage::~JMSMessage() 
{

}

/***************************************************************************
 Method:  isRepliable
 Summary: Returns true if this message wants a reply.  Replies are special.
          They don't have to be declared by a separate subcriber and 
          must go to the special reply topic, which is temporary.  Once
          the first reply is sent, the temporary reply topic is deleted.
          All other replies are ignored.
***************************************************************************/
bool JMSMessage::isRepliable() const {
    bool ret = false;
    if (p_replyTopicName != "") {
        ret = true;
    }
    return ret;
}

/***************************************************************************
 Method:  setReplyTopic
 Summary: This should only get set by the message handler, when a published
          message is received from the JMS server.  This information is
          forwarded from the JMS server.
***************************************************************************/
void JMSMessage::setReplyTopic(const string& topicName) {
    p_replyTopicName = topicName;
}

/***************************************************************************
 Method:  getReplyTopic
 Summary: The average user won't care about this function.  Only
          JMSMessageDispatch should use this when creating a reply message.
***************************************************************************/
string JMSMessage::getReplyTopic () const {
    return p_replyTopicName;
}

/***************************************************************************
 Method:  setTarget
 Summary: Indicate who this message is for.  This piece of info is used
          by the JMS server to filter out unwanted messages for each
          subscriber.

          Right now it has the same info as the sender: tool and process
          id, but any part could be blank.
***************************************************************************/   
void JMSMessage::setTarget(const Target& target) {
    p_target = target;
}

/***************************************************************************
 Method:  getTarget
 Summary: 
***************************************************************************/
Target JMSMessage::getTarget() const {
    return p_target;
}

/***************************************************************************
 Method:  setSender
 Summary: 
***************************************************************************/
void JMSMessage::setSender(const Sender& sender) {
    p_sender=sender;
}

/***************************************************************************
 Method:  getSender
 Summary: 
***************************************************************************/
Sender JMSMessage::getSender() const {
    return p_sender;
}

/***************************************************************************
 Method:  containsProperty
 Summary: Returns true if this message contains this property.
***************************************************************************/
bool JMSMessage::containsProperty(const string& key) const {
  map<string, string, less<string> >::const_iterator it =
    p_properties.find(key);

  bool ret=false;
  if (it != p_properties.end()) {
    ret = true;
  }
  return ret;
}

/***************************************************************************
 Method:  addProperty
 Summary: The body of the message is stored as key/value pairs.  This
          is analogous to the JMS MapMessage type.
***************************************************************************/
void JMSMessage::addProperty(const string& key, const string& value) {
  p_properties[key] = value;
}

void JMSMessage::addIntProperty(const string& key, const int& value) {

  // first convert the int to a string:

  char buf[100];
  sprintf(buf, "%d", value);
  p_properties[key] = buf;
}

/***************************************************************************
 Method:  getProperty
 Summary: Returns the empty string if the property does not exist.
***************************************************************************/
string JMSMessage::getProperty(const string& key) const {
  
  string value;
  map<string, string, less<string> >:: const_iterator it;
  if ( (it = p_properties.find(key)) != p_properties.end()) {
    value = (*it).second;
  }
  return value;
}

/***************************************************************************
 Method:  getIntProperty
 Summary: Returns 0 if the property does not exist.
***************************************************************************/
int JMSMessage::getIntProperty(const string& key) const {
  int ret = 0;
  string value = getProperty(key);
  if (value != "") {
    sscanf(value.c_str(),"%d",&ret);
  }
  return ret;
}

/****************************************************************************
 Method:  bodyToString
 Summary: Writes all the key/value pairs using the following tag delimiters:

          NAMESTART key1 NAMEEND
          VALUESTART value1 VALUEEND
          NAMESTART key2 NAMEEND
          VALUESTART value 2VALUEEND
          ...

*****************************************************************************/
string JMSMessage::bodyToString() const {

  string ret;
  map<string, string, less<string> >::const_iterator it, start;
  start = p_properties.begin();
  
  for (it = start; it != p_properties.end(); it++) {
    addItem("NAME", (*it).first, ret);
    addItem("VALUE", (*it).second, ret);
  }

  return ret;
}

/***************************************************************************
 Method:  senderToString
 Summary: 
***************************************************************************/
string JMSMessage::senderToString() const {
    return p_sender.toString();
}

/***************************************************************************
 Method:  targetToString
 Summary: 
***************************************************************************/
string JMSMessage::targetToString() const {
    return p_target.toString();
}

/***************************************************************************
 Method:  loadSender
 Summary: Unmarshals the sender information from its string equivalent.
***************************************************************************/
void JMSMessage::loadSender(const string& sender) {
    p_sender.loadFromString(sender);
}

/***************************************************************************
 Method:  loadTarget
 Summary: Unmarshals the target information from its string equivalent.
***************************************************************************/
void JMSMessage::loadTarget(const string& target) {
    p_target.loadFromString(target);
}

/***************************************************************************
 Method:  loadBody
 Summary: Umarshals the body (key-value pairs) from its string equivalent.
***************************************************************************/
void JMSMessage::loadBody(const string& bodyStr) {

    p_properties.clear();

    if (bodyStr != "") {
        // parse out the message content as key/value pairs:
        string body = bodyStr;
        string name, value;
        bool status;
        do {
          status = getItem("NAME", name, body);
          status &= getItem("VALUE", value, body);
          EE_ASSERT(status, EE_FATAL, "Invalid message body format!");
          addProperty(name,value);
        } while (body != "");
    }
}

/***************************************************************************
 Method:  addItem
 Summary: Used to put an item (NAME or VALUE) in the body string packet.
***************************************************************************/
void JMSMessage::addItem(const string& itemName, const string& item,
                         string& packet) const {
  
  packet += itemName + "START" + item + itemName + "END";

}

/***************************************************************************
 Method:  getItem
 Summary: Used to retrieve an item(NAME or VALUE) from the body string
          packet.
***************************************************************************/
bool JMSMessage::getItem(const string& itemName, string& item,
                         string& packet) {
  item = "";
  bool ret = true;
  string starttag = itemName + "START";
  string endtag = itemName + "END";

  size_t startpos = packet.find(starttag);
  size_t beginItem = startpos;
  size_t endpos = packet.find(endtag);
  size_t endItem = endpos + endtag.length();

  if (startpos == string::npos || endpos == string::npos) {
    EE_ASSERT(false, EE_WARNING, "Invalid message property format.");
    ret = false;

  } else {
    if ( (startpos += starttag.length()) < endpos) {     
      item = packet.substr(startpos, endpos - startpos);
    }
    // remove the item from the packet
    packet.replace(beginItem, endItem - beginItem, ""); 
  }
  return ret;

}

void JMSMessage::markAsPointToPoint()
{
   addIntProperty("p2p",1);
}

string JMSMessage::getOriginalSenderName() const
{
   return getProperty("origsendername");
}


string JMSMessage::getOriginalSenderId() const
{
   return getProperty("origsenderid");
}


///////////////////////////////////////////////////////////////////////////
// MessageParticipant Methods
///////////////////////////////////////////////////////////////////////////

// Constructors
MessageParticipant::MessageParticipant() {
    // blank data
}

MessageParticipant::MessageParticipant(const string& name, const string& id) {
    p_name = name;
    p_id = id;
}

// Virtual Destructor
MessageParticipant::~MessageParticipant() {
}

// Accesstors
string MessageParticipant::getName() const {
    return p_name;
}

string MessageParticipant::getID() const {
    return p_id;
}

string MessageParticipant::toString() const {
    string ret = p_name + "\n" + p_id;
    return ret;
}

// Modifiers
void MessageParticipant::setName(const string& name) {
    p_name = name;
}

void MessageParticipant::setID(const string& id) {
    p_id = id;
}

void MessageParticipant::loadFromString(const string& mpStr) {
    
    if (mpStr != "") {
      
      size_t newlinePos = mpStr.find('\n');
      if (newlinePos != string::npos) {
        p_name = mpStr.substr(0, newlinePos);
        if (newlinePos < mpStr.length() -1) {
          p_id = mpStr.substr(newlinePos + 1);
        }
      } else {
        string errMsg = "Invalid target string format: " + mpStr;
        EE_ASSERT(false, EE_FATAL, errMsg);
      }
    }
}

Sender::Sender(const string& name, const string& id) 
    : MessageParticipant(name,id) {
}

Sender::Sender() {
}

Target::Target(const string& name, const string& id) 
    : MessageParticipant(name,id) {
}

Target::Target() {
}


////////////////////////////////////////////////////////////////////////////
// DATAGRAM UTIL METHODS 
////////////////////////////////////////////////////////////////////////////

// Static initialization:
int DatagramUtil::JMSDISPATCH_PORT = -1; 

string DatagramUtil::getItem(const string& itemName, const string& packet) { 
  
  string ret;
  string starttag = itemName + "START";
  string endtag = itemName + "END";

  size_t startpos = packet.find(starttag);
  size_t endpos = packet.find(endtag);
  
  if (startpos == string::npos || endpos == string::npos) {
    cerr << "Invalid datagram packet format: " << packet << endl;

  } else if ( (startpos += starttag.length()) < endpos) {
     
    ret = packet.substr(startpos, endpos - startpos);
  }
  return ret;
}

void DatagramUtil::addItem(const string& itemName, const string& item,
                           string& packet) {
  
  packet += itemName + "START" + item + itemName + "END";

}

void DatagramUtil::addIntItem(const string& itemName, const int& item,
                              string& packet) {
  
  char buf[100];
  sprintf(buf, "%d", item);
  addItem(itemName, buf, packet);

}

void DatagramUtil::addBoolItem(const string& itemName, const bool& item,
                               string& packet) {

  string itemStr;
  if (item) {
    itemStr = "true";
  } else {
    itemStr = "false";
  }
  addItem(itemName, itemStr, packet);
}

void DatagramUtil::loadServerPort() {
  
    // determine the file name
    string fileName = Ecce::realUserPrefPath();
    string display = getEnv("DISPLAY");
    string host = getEnv("HOST");
 
    /*
    int colPos = display.find(":");
    if (colPos != string::npos) {
      display = display.substr(colPos+1);
    }
    int dotPos = display.find(".");
    if (dotPos == string::npos) {
      display += ".0";
    }
    */
    fileName += host + "_" + display;

    // Read and parse the file:
    ifstream infile(fileName.c_str());
    string errMsg = "Could not open ";
    errMsg += fileName;
    EE_RT_ASSERT(infile.good(), EE_FATAL, errMsg);

    infile >> JMSDISPATCH_PORT;    
    infile.close();
}

int DatagramUtil::getServerPort() {

  if (JMSDISPATCH_PORT == -1) {
    loadServerPort();
  } 
  return JMSDISPATCH_PORT;
}

string DatagramUtil::getEnv(const char* propertyName) {
  
    char*  property = getenv(propertyName);
    string errMsg = "You Must Define ";
    errMsg += propertyName;
    EE_RT_ASSERT(property, EE_FATAL, errMsg);
    return property;
}

bool DatagramUtil::sendPacket (const string& packet) {
  bool ret = true;

  // Create datagram socket:
  int s;
  struct sockaddr_in addIN;

  if ( (s = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
    perror ("Error creating client datagram socket: ");
    ret = false;

  } else {

    addIN.sin_family      = AF_INET;
    addIN.sin_port        = htons(getServerPort());
    addIN.sin_addr.s_addr = INADDR_ANY;
 
    // Check if packet size is too big (we are only receiving 4096 bytes)
    if (packet.length() > MAX_PACKET_LENGTH) {
      EE_RT_ASSERT(0, EE_WARNING, "Packet size greater than 4096 bytes!");
    }

    // write message
    if (sendto(s, packet.c_str(), sizeof(char) * packet.length(), 0,
               (struct sockaddr *)&addIN, sizeof(addIN) ) < 0) {  
      perror("Error sending datagram packet: ");
      ret = false;
    }     
    close(s);
      
  }
  return ret;
}

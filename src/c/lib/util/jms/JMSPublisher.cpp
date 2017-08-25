//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: JMSPublisher.C
//
//
// DESIGN: 
//
///////////////////////////////////////////////////////////////////////////////

// System includes:
   #include <sys/types.h>
   #include <unistd.h>
   #include <stdio.h>

// Application includes:
   #include "util/JMSPublisher.H"
   #include "util/ErrMsg.H"
   #include "util/Host.H"
   #include "util/JMSMessage.H"


// Static initialization:
   bool JMSPublisher::p_messagingEnabled = 
   (getenv("ECCE_NO_MESSAGING") == NULL) ? true : false;
 
/*******************************************************************
 Method : Constructor 
 Summary: 
********************************************************************/
JMSPublisher::JMSPublisher(const string& toolName) {

  // To stamp Sender info:
  p_toolName = toolName;
}

/*******************************************************************
 Method : Destructor 
 Summary: Disconnects to openJMS server.
********************************************************************/
JMSPublisher::~JMSPublisher() {
 
  if (p_messagingEnabled) {

    // Nothing to do
  }
}

/*******************************************************************
 Method : getToolName
 Summary: Use this to get your tool name, as used for message
          identification.
********************************************************************/
string JMSPublisher::getMyName() const {
  return p_toolName;
}

/*******************************************************************
 Method : getProcessID
 Summary: Use this to get your process ID (pid), as used for message
          identification.
********************************************************************/
string JMSPublisher::getMyID() const {

  // Determine unique process ID for identification and monitoring
  // purposes using waitpid on that specific ID (platform specific):
  int pid;

  pid = getpid();

  char buf[100];
  sprintf(buf, "%d", pid);
  return buf;
}

/*******************************************************************
 Method : newMessage
 Summary: Use JMSPublisher as your message factory.  This
          way it stamps your proper ID/return information on 
          every message you send.
********************************************************************/
JMSMessage* JMSPublisher::newMessage() const {
    Sender sender(p_toolName, getMyID());
    JMSMessage* msg = new JMSMessage(sender);
    return msg;
}

JMSMessage* JMSPublisher::newMessage(const Target& target) const {
  JMSMessage* msg = newMessage();
  msg->setTarget(target);
  return msg;
}

/*******************************************************************
 Method : publish
 Summary: 
********************************************************************/
bool JMSPublisher::publish(const string& topic,
                                 const JMSMessage& msg) const {
  bool ret = false;
  if (p_messagingEnabled) {

    // Convert the parameters to a datagram packet:
    string packet;
    DatagramUtil::addItem("METHOD", "publish", packet);
    DatagramUtil::addItem("TOPIC", topic, packet);
    DatagramUtil::addItem("BODY", msg.bodyToString(), packet);
    DatagramUtil::addItem("SENDER", msg.senderToString(), packet);
    DatagramUtil::addItem("TARGET", msg.targetToString(), packet);
    DatagramUtil::addItem("REPLYTOPIC", msg.getReplyTopic(), packet);

    // publish the message
    ret = DatagramUtil::sendPacket(packet);
    if (!ret) {
      EE_ASSERT(false, EE_WARNING, "publish failed!");
    }
  }
  return ret;
}



/**
 * A convenience method to publish a message that invokes and application.
 * @param app - application key name to start
 * @param forcenew - if 1, force new instance of the app
 * @param url - context for the new app
 * @param actionTopic - secondary topic to publish after startup.  This is
 *    useful for requesting a tool and then requesting it do something
 *    such as go into some context.
 */
bool JMSPublisher::invoke(JMSMessage& msg, 
                         const string& app, 
                         int forcenew, 
                         const string& url,
                         const string& actionTopic)
{
   bool ret = false;

   if (p_messagingEnabled) {
      // Add standard properties
      msg.addProperty("appname", app);
      msg.addIntProperty("forcenew", forcenew);
      if (url != "") {
         msg.addProperty("calcurl", url);
      }
      if (actionTopic != "") {
         msg.addProperty("actiontopic", actionTopic);
      }


      // Convert the parameters to a datagram packet:
      string packet;
      DatagramUtil::addItem("METHOD", "publish", packet);
      // ecce_get_app is the topic we need to do an invoke
      DatagramUtil::addItem("TOPIC", "ecce_get_app", packet);
      DatagramUtil::addItem("BODY", msg.bodyToString(), packet);
      DatagramUtil::addItem("SENDER", msg.senderToString(), packet);
      DatagramUtil::addItem("TARGET", msg.targetToString(), packet);
      DatagramUtil::addItem("REPLYTOPIC", msg.getReplyTopic(), packet);

      // publish the message
      ret = DatagramUtil::sendPacket(packet);
      if (!ret) {
         EE_ASSERT(false, EE_WARNING, "publish failed!");
      }
   }
   return ret;
}

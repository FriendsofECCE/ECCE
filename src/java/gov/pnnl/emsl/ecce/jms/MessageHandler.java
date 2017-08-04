//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: MessageHandler.java
//
// DESIGN: Interfaces with JNI C++ message handler
//////////////////////////////////////////////////////////////////////////////
package gov.pnnl.emsl.ecce.jms;

// Includes:
   import javax.jms.TextMessage;
   import javax.jms.Message;
   import javax.jms.Topic;
   import java.net.DatagramSocket;
   import java.net.DatagramPacket;
   import java.net.InetAddress;
   import java.net.UnknownHostException;
   import java.io.IOException;

public class MessageHandler implements javax.jms.MessageListener {
    
  // Data
  private String          topic; // the topic being subscribed to
  private String          subscriberID; // my unique subscriber id
  private int             outPort;
  private DatagramSocket  inSocket;
  private DatagramPacket  messagePacket;
  private boolean         p_selfFilter;

  // Constructors
  public MessageHandler(int port, String top, String id) {   
      this(port, top, id, false);
  }

  public MessageHandler(int port, String top, String id, boolean selfFilter) {
      p_selfFilter = selfFilter;
      outPort = port;
      topic = top;
      subscriberID = id;
      
      try {
          inSocket = new DatagramSocket();
      } catch (Exception exc) {
          System.out.println("Unable to create a socket");
          System.out.println("Error: "+exc.toString());
          System.exit(99);
      }
  }
  
  public void onMessage (Message msg) {

      if (!MessageHelper.messagingSuspended(subscriberID)){
          try {
              // Cast the mesage as a text message
              TextMessage txtMsg = (TextMessage) msg;
              
              // load target
              Target targetObj = new Target(msg);
              Sender senderObj = new Sender(msg);
              String body   = txtMsg.getText();
              String target = targetObj.toString();
              String sender = senderObj.toString();

              if (!p_selfFilter || senderObj.getID() != subscriberID) {
      
                  try {
                      // Create DatagramPacket
                      InetAddress localAddr = InetAddress.getLocalHost();
                      String message = marshallMessage(body, sender, target);
                      
                      messagePacket  = new DatagramPacket(message.getBytes(), 
                                                          message.length(),
                                                          localAddr, outPort);
                      
                  } catch (UnknownHostException exc) {
                      System.out.println("Unable to get local host address");
                      System.out.println(exc.toString());
                      System.exit(99);
                  }
                  // Send through socket to C++ handler
                  inSocket.send(messagePacket);       
              }
          } catch (Exception e) {
              System.out.println(e.toString());
          }    
      }
  }

  private String marshallMessage(String body, String sender, String target) {
    
    String ret = new String();
    ret = "BODYSTART" + body + "BODYEND" + 
          "SENDERSTART" + sender + "SENDEREND" +
          "TARGETSTART" + target + "TARGETEND" +
          "TOPICSTART" + topic + "TOPICEND";
  
    return ret;
  }
}






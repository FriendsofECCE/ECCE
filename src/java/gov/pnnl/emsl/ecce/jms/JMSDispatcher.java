//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: JMSDispatcher.java
//
// DESIGN: Sets up a JMS connection and a server socket, and then allows client
//         (i.e., Ecce apps) to communicate with a JMS pub/sub server by
//         sending datagram packet requests to the socket. 
//
//////////////////////////////////////////////////////////////////////////////
package gov.pnnl.emsl.ecce.jms;


// Includes:
   import javax.jms.*;
   import java.util.Properties;
   import java.util.HashMap;
   import java.util.Iterator;
   import java.util.Collection;
   import java.util.LinkedList;
   import java.util.StringTokenizer;
   import java.util.Vector;
   import javax.naming.Context;
   import javax.naming.InitialContext;

   import java.net.DatagramSocket;
   import java.net.DatagramPacket;

   import java.io.File;
   import java.io.FileOutputStream;
   import java.io.PrintWriter;
   import java.io.FileReader;
   import java.io.BufferedReader;


public class JMSDispatcher {

    //---------------------------------------------------------------
    // DATA MEMBERS:
    //---------------------------------------------------------------

    /** 
     *One connection for all publishers
     */
    private TopicConnection p_pubConnection;

    /**
     *One topic session for all publishers
     */
    private TopicSession    p_pubSession;

    /** 
     *Server socket for incoming message requests
     */
    private DatagramSocket p_serverSocket;

    /**
     *The server port, where clients will send requests
     */
     private int p_port;

    /**
     *Caches valid message topics and their default filtering status
     */
     private HashMap<String,String> p_messageTopics;

    /**
     *Holds hostname/display identifier
     */
     private String p_hostnameKey;

    /**
     *Caches JMS Publishers so we don't have to recreate each time
     */
     private HashMap<String,TopicPublisher> p_publishers; 

    /**
     *Caches JMS Topics so we don't have to look up each time
     */
     private HashMap<String,Topic> p_topics; 

    /**
     *Cache separate subscriber connections for each app, so subscriptions
     *can be individually terminated at any time without affecting other
     *subscriptions.
     */
     private HashMap<String,TopicConnection> p_subscriberConnections;

    /**
     *Cache local subscriptions that will not go through the server:
     */
     private HashMap<String,HashMap<String,HashMap<String,MessageHandler>>> p_localSubscribers;

    /**
     *Set on startup with the -v options. Determines how much debug info 
     *to print out
     */
     private boolean p_verbose;

    /**
     *Set on startup with the -v options. Determines how much debug info 
     *to print out
     */
     private boolean p_openjms;

    /**
     * Keep track of the first app to start up - when this app unsubscribes,
     * then shut down the server
     */
     private String p_startName;
     private String p_startID;

    /**
     * Caches the jndi initial context so we dont have to recreate
     */
     private Context p_jndi;

    /**
     * Keep our own queue of incoming packets, so we can keep the
     * system socket buffer clean, ensuring it won't overflow and
     * drop packets.
     */
     private LinkedList<String> p_packetQueue;


    //---------------------------------------------------------------
    // INNER CLASSES:
    //---------------------------------------------------------------
    /*******************************************************************
     Class:   PacketHandler
     Summary: Coninuously gets packets from the front of the packet
              queue and processes them one at a time until the
              process terminates.

              Using a queue makes sure that incoming messages get
              processed in the right order.
    *******************************************************************/
    public class PacketHandler extends Thread {

        /**
         * Processes incoming packet.
         *
         * Wakes up when main loop notifies this thread that something
         * is available.
         */
        public void run() {
            String packet;

            while(true) {
                int size;

                // process packets in queue as fast as I can
                synchronized(p_packetQueue){
                    size = p_packetQueue.size();
                }
                while(size > 0) {  // can't put synch statement inside parens
                    
                    try {                    
                        // Get the packet and remove it from the queue:
                        synchronized(p_packetQueue) {
                            packet = p_packetQueue.get(0);
                            p_packetQueue.remove(0);
                        }
                        
                        // Process the packet:
                        String method = MessageHelper.getItem(packet,"METHOD");
                        
                        // Print debug info
                        if (p_verbose) {
                            System.out.println(
                                       "************** Incoming " + method + 
                                       " request **************\n"
                                       + "Packet =\n" + packet);
                        }
                        // Service request
                        if (method.equals("subscribe")) {
                            subscribe(packet);
                        } else if (method.equals("unsubscribe")) {
                            unsubscribe(packet);
                        }else if (method.equals("publish")) {
                            publish(packet);
                        } else if (method.equals("hold")) {
                            holdMessages(packet);
                        } else if (method.equals("resume")) {
                            resumeMessaging(packet);
                        } else if (method.equals("startsubscriber")) {
                            startSubscriber(packet);
                            
                        } else {
                            System.out.println("Invalid packet: " + packet
                                           + " sent to JMSDispatcher.java");
                        }
                    
                        synchronized(p_packetQueue){
                            size = p_packetQueue.size();
                        }
                    } catch (Exception e) {
                        if (e.toString() != "java.lang.NullPointerException")
                            System.out.println(e.toString());
                        e.printStackTrace();
                    }
                }
                // Now wait until some more get put on the queue:
                synchronized(p_packetQueue) {
                    try {
                        p_packetQueue.wait();
                    } catch (InterruptedException ie) {
                        if (ie.toString() != "java.lang.NullPointerException")
                            System.out.println(ie.toString());
                        ie.printStackTrace();
                    }
                }
            }
        } // end run

    }

    //---------------------------------------------------------------
    // MEMBER FUNCTIONS:
    //---------------------------------------------------------------

    /*******************************************************************
      Method : Constructor
      Summary: Performs a JNDI lookup of a TopicConnectionFactory 
               from the openJMS JNDI server.  Then creates a topic
	       connection and publisher session.  The connection is the
               heavyweight object, and only one needs to be created for
               all subscribers/publishers.  Because only one publisher
               is created for each topic (all apps share), we only
               need one topic session for all publishers.

               Constructor also sets up a server socket, where all
               Ecce messaging requests are sent.  It saves the socket ID
               to file, so that it may be obtained by other apps.

               Finally, it loads a configuration file that contains a
               list of all the valid topics, and the default filtering
               that must be performed for each topic.
    *******************************************************************/
    public JMSDispatcher(boolean verboseFlag, boolean openjmsFlag) {

        // Initialize variables:
        p_publishers = new HashMap<String,TopicPublisher>(); 
        p_topics = new HashMap<String,Topic>();
        p_subscriberConnections = new HashMap<String,TopicConnection>();
        p_messageTopics = new HashMap<String,String>();
        p_packetQueue = new LinkedList<String>();
        p_localSubscribers = new HashMap<String,HashMap<String,HashMap<String,MessageHandler>>>();
        p_verbose = verboseFlag;
        p_openjms = openjmsFlag;
        p_startName = null;
        p_startID = null;

        if (p_verbose) {
            System.out.println("Initializing JMSDispatcher...");
        }

        // Cache one jndi connection for everybody:
        // jndi server is located in $ECCE_HOME/siteconfig/jndi.properties)
        try {
            p_jndi = new InitialContext();   
        } catch (Exception e0) {
            System.out.println("\nERROR:  Unable to establish connection with ECCE messaging server.\n");
            if (e0.toString() != "java.lang.NullPointerException") {
                System.out.println("=========================== Error Details ============================\n");
                System.out.println(e0.toString());
                System.out.println("======================================================================\n");
            }
        }
        // Now create the publisher session:
        try {

            p_pubConnection = createConnection();
            p_pubSession = p_pubConnection.createTopicSession(false, 
                                                  Session.AUTO_ACKNOWLEDGE);
            p_pubConnection.start();	
            
        } catch (Exception e1) {
            if (e1.toString() != "java.lang.NullPointerException")
                System.out.println(e1.toString());
            //e1.printStackTrace();
            System.exit(-1);
        }

        // Now create the server socket:
        try {
            p_serverSocket = new DatagramSocket();
            p_port = p_serverSocket.getLocalPort();
            if (p_verbose) {
                System.out.println("Connecting to port " + p_port);

                System.out.println("system receive buffer size = " +
                                   p_serverSocket.getReceiveBufferSize());

                System.out.println("system send buffer size = " + 
                                   p_serverSocket.getSendBufferSize());
            }

        } catch (Exception e2) {

            System.out.println("Unable to create server socket");
            if (e2.toString() != "java.lang.NullPointerException")
                System.out.println(e2.toString());
            //e2.printStackTrace();
            quit();
        }

        // Save the port # to file, so the ecce apps can find it:
        saveServerPort();

        if (p_verbose) {
            System.out.println("Initialization complete.");
        }

        // Determine the unique hostname/display identifier:
        String display = getSystemProperty("DISPLAY");
        String host = getSystemProperty("ECCE_HOST");

        int colPos = display.indexOf(":");
        if (colPos != -1) {
            display = display.substring(colPos+1);
        }
        int dotPos = display.indexOf(".");
        if (dotPos == -1) {
            display += ".0";
        }
        p_hostnameKey = host + ":" + display;

        // Load the list of messages from the config file:
        loadMessageTopics();
    }

    /*******************************************************************
     Method : loadMessageTopics
     Summary: Read these from config file. Since topics are administered
              by the server, any topics other than these that try
              to be sent, will be discarded.
    *******************************************************************/
    private void loadMessageTopics() {

         try {
            // Get path to config file
            String path = getSystemProperty("ECCE_HOME");
            if (path == null || path.equals("")) {
                throw new Exception ("ECCE_HOME environment variable "+
                                     "must be set!");
            }
            path += "/data/client/config/ecce_messages";
                        
            File msgFile = new File(path);

            if (!msgFile.exists()) {
                throw new Exception("$ECCE_HOME/data/client/config/"+
                                    "ecce_messages does not exist!");
            }

            BufferedReader input = 
                new BufferedReader(new FileReader(msgFile));
            
            String line;
            String filterType = null;
            String messageTopic;

            while( (line=input.readLine()) != null) {
                line = line.trim();

                if (line.equals("")) {
                    continue; // skip blank lines
                    
                } else if (line.startsWith("//") || line.startsWith("#")) {
                   continue;
                } else if (line.startsWith("FILTER BY:")) {

                    filterType = line.substring(10);
                    filterType = filterType.trim();

                } else if (filterType == null) {
                    throw new Exception ("Invalid file format!");

                } else {
                    
                    messageTopic = line;
                    p_messageTopics.put(messageTopic, filterType); 
                }
            }

        } catch(Exception e) {
            System.out.println("Error configuring message topics:\n"
                               + e.toString());
            quit();
        }       
    }


    /*******************************************************************
     Method : saveServerPort
     Summary: Saves dynamic server port to config file.  Same file is read
              by C++ apps.
    *******************************************************************/
    private void saveServerPort() {
        
        try {
            String path = getSystemProperty("ECCE_USERHOME");
            String display = getSystemProperty("DISPLAY");
            String host = getSystemProperty("ECCE_HOST");

            /*
            int colPos = display.indexOf(":");
            if (colPos != -1) {
                display = display.substring(colPos+1);
            }
            int dotPos = display.indexOf(".");
            if (dotPos == -1) {
                display += ".0";
            }
            */
            String dirPath = new String(path + "/.ECCE");
            String filePath = 
                new String(path + "/.ECCE/" + host + "_" + display);
 
            File dir = new File (dirPath);
            if (!dir.exists()) {
                dir.mkdir();
            }
            File file = new File(filePath);
            PrintWriter pw = new PrintWriter(new FileOutputStream(file));
            pw.print(p_port);
            pw.close();
            
        } catch(Exception e) {
            System.out.println("Error saving port to file (no write access?):\n"
                               + e.toString());
            quit();
        }
    }
    
    /*******************************************************************
     Method : getSystemProperty
     Summary: 
    *******************************************************************/
    private String getSystemProperty(String propertyName) {

        String property = System.getProperty(propertyName);
        if (property == null) {
            System.out.println("You Must Define " + propertyName);
            quit();
        }
        return property;
    }

    /*******************************************************************
      Method : Start
      Summary: Starts the main loop where the dispatcher listens
               for incoming commands (publish, subscribe, unsubscribe)
               on its server socket.
    *******************************************************************/
    public void mainLoop() {

        try{           
            // First start a separate thread that processes incoming
            // requests from the queue;
             PacketHandler ph = new PacketHandler();
             ph.start();

            byte buf[];
            DatagramPacket packet;

            // KLS 10/19/06
            // Moved this outside the loop to save memory.
            buf = new byte[MessageHelper.MAX_PACKET_LENGTH +1];
            packet = new DatagramPacket(buf,
                                            MessageHelper.MAX_PACKET_LENGTH);
           
            while(true) {
                
                // Read datagram packet
                p_serverSocket.receive(packet);
                String data = new String(packet.getData());

                // Need to strip off the garbage characters from the
                // buffer:
                data = data.substring(0, packet.getLength());

                // Put this packet on the end of the queue
                synchronized(p_packetQueue) {
                    p_packetQueue.addLast(data);
                    // if other thread is waiting, tell him to go
                    p_packetQueue.notify(); 
                }

            }
        } catch (Exception e) {
            if (e.toString() != "java.lang.NullPointerException")
                System.out.println(e.toString());
            e.printStackTrace();
            quit();
        }
    }

   /*******************************************************************
      Method : quit
      Summary:
               Closes JMS connections and server socket.
    *******************************************************************/
    public synchronized void quit() {
        
         try {          
            // Close connection and all publishers/subscribers
            if (p_verbose) {
                System.out.println("Closing JMS connection....");
            }
            p_pubConnection.close();
            
            // Now loop through all the subscriber connections:
            // Hopefully there aren't any
            if (p_verbose) 
               System.out.println("Closing subscriber connections");
            Iterator it = p_subscriberConnections.keySet().iterator();
            while (it.hasNext()) {
               String key = (String)it.next();
               if (p_verbose) 
                  System.out.println("closing connection " + key);
               TopicConnection curConnection = p_subscriberConnections.get(key);
               curConnection.close();
            }

            if (p_verbose) {
                System.out.println("Done.");
            }
        } catch (Exception e) {
            System.out.println("Error closing JMS connection!");
            e.printStackTrace();
        }
        try {          
            // Close jndi connection
            if (p_verbose) {
                System.out.println("Closing jndi connection....");
            }
            p_jndi.close();
            if (p_verbose) {
                System.out.println("Done.");
            }
        } catch (Exception e) {
            System.out.println("Error closing jndi connection!");
            e.printStackTrace();
        }
        try {
            // Close the server socket
            // (don't do this here, let program clean up on exit)
            //System.out.println("Closing server socket....");
            //p_serverSocket.close();
            //System.out.println("Done.");
        } catch (Exception e) {
            System.out.println("Error closing server socket!");
            e.printStackTrace();
        }    
        if (p_verbose) {
            System.out.println("JMSDispatcher terminated.");
        }
        System.exit(0);
    }

    /**********************************************************************
     Method:  publish
     Summary: Publishes the given topic
    **********************************************************************/
    public synchronized void publish (String data) throws Exception {

	if (p_verbose) {
	    System.out.println("************** Outgoing in publish **************\n"
			       + "data =\n" + data);
	}

        String topicName = MessageHelper.getItem(data, "TOPIC");

        // See if this is a local subscription by its filter type:
        String filterType = getFilterType(topicName);
        
        // We do publish 2 ways:
        // 1) If this is a local ONLY message, send directly:
        if (filterType.equals("USER:HOSTNAME:DISPLAY")) {
            publishLocal(data);
            
        // 2) Otherwise, route through jms server
        } else {
            publishServer(data);
        }
    }

    /**********************************************************************
     Method:  publishLocal
     Summary: Directly call the MessageHandlers for this topic.
    **********************************************************************/
    public void publishLocal (String data) throws Exception {

        String topicName = MessageHelper.getItem(data, "TOPIC");
        String body = MessageHelper.getItem(data, "BODY");
        String senderStr = MessageHelper.getItem(data, "SENDER");
        String targetStr = MessageHelper.getItem(data, "TARGET");
        String replyTopicName = MessageHelper.getItem(data, "REPLYTOPIC");

        HashMap appsForTopic = (HashMap)p_localSubscribers.get(topicName);
        TextMessage msg = createMessage(body, senderStr, targetStr);

	if (p_verbose) {
	    System.out.println("************** Outgoing in publishLocal **************"
			       + "\ntopic\t = " + topicName 
			       + "\nbody\t = " + body
			       + "\nsender\t = " + senderStr
			       + "\ntarget\t = " + targetStr
			       + "\nreplytopic\t = " + replyTopicName
			       + "\n");
	}

        // If this topic has subscribers
        if (appsForTopic != null) {
	    
	    if (p_verbose) {
		System.out.println("************** appsForTopic in publishLocal **************\n"
				   + appsForTopic.values().toString());
	    }



            Target target = new Target(targetStr);
            Sender sender = new Sender(senderStr);

            String name = target.getName();
            //System.out.println("target name= " + name);
            if (name != null && name.equals("")) {
                name = null;
            }                                  
            String id = target.getID();
            //System.out.println("target id = " + id);
            if (id != null && id.equals("")) {
                id = null;
            }
            //System.out.println("topic = " + topicName);
            // We are sending it to all local subscribers:
            if (name == null && id == null) {
                //System.out.println("sending to all local subscribers");
                HashMap appsForName;
                Iterator nameIt = appsForTopic.values().iterator();
                Iterator idIt;
                MessageHandler handler;

                while (nameIt.hasNext()) {
                    appsForName = (HashMap) nameIt.next();
                    idIt = appsForName.values().iterator();

                    while(idIt.hasNext()) {
                        handler = (MessageHandler) idIt.next();
                        handler.onMessage(msg);
                    }
                }
                
            // We are sending it to all subscribers of the same name:
            } else if (name != null && id == null) {
                //System.out.println("sending to all " + name);
                HashMap appsForName = (HashMap)appsForTopic.get(name);
                
                if (appsForName != null) {
                    Iterator it = appsForName.values().iterator();
                    MessageHandler handler;
                    while (it.hasNext()) {
                        handler = (MessageHandler)it.next();
                        handler.onMessage(msg);
                    }
                }                
                
            // We are sending it to one particular subscriber:
            } else {                
                //System.out.println("sending to" + id);
                HashMap appsForName;
                Iterator it = appsForTopic.values().iterator();

                while (it.hasNext()) {
                    appsForName = (HashMap) it.next();

                    if (appsForName.containsKey(id)) {
                        MessageHandler handler = 
                            (MessageHandler)appsForName.get(id);
                        handler.onMessage(msg);
                        break;
                    }
                }
            }
        }      
    }          


    /************************************************************************
      Method: publishServer
      Summary: Receives three parts of a message from the datagram packet:

           1) body
           2) sender
           3) target
           
           Each part is marshalled/unmarshalled via methods in the 
           JMSMessage class.  We can change it later if we need to, and
           it won't effect the client code.

           For now, the body is set of key/value pairs (which is similar 
           to java's MapMessage type). The body is marshalled to a
           line-delimited string to simplify the JNI interface.

           Example body as string:
              key1
              value1
              key2
              value2
              ...
    *************************************************************************/
    public void publishServer (String data) throws Exception {

        String topicName = MessageHelper.getItem(data, "TOPIC");
        String body = MessageHelper.getItem(data, "BODY");
        String sender = MessageHelper.getItem(data, "SENDER");
        String target = MessageHelper.getItem(data, "TARGET");
        String replyTopicName = MessageHelper.getItem(data, "REPLYTOPIC");

	if (p_verbose) {
	    System.out.println("************** Outgoing in publishServer **************"
			       + "\ntopic\t = " + topicName 
			       + "\nbody\t = " + body
			       + "\nsender\t = " + sender
			       + "\ntarget\t = " + target
			       + "\nreplytopic\t = " + replyTopicName
			       + "\n");
	}

        try {
            TopicPublisher publisher;
            
            TextMessage msg = createMessage(body, sender, target);
            if (!replyTopicName.equals("")) {
              Topic replyTopic = getTopic(replyTopicName);
              msg.setJMSReplyTo(replyTopic);
            }

            if (p_publishers.containsKey(topicName)) {
                publisher = p_publishers.get(topicName);     
            } else {
                
                Topic topic = getTopic(topicName);
                publisher = p_pubSession.createPublisher(topic);
                p_publishers.put(topicName, publisher);
            }
            
            // Send a non-persistent message of priority 4 (the JMS default)
            // that won't expire until all subscribers have received it
            publisher.publish(msg, DeliveryMode.NON_PERSISTENT, 4, 0);

        } catch (Exception e) {
            //System.out.println(e.toString());
            e.printStackTrace();
        }
    }

    /************************************************************************
     Method:  createMessage
     Summary: This is the one function that stamps properties on outgoing
              messages.  Modify this if you need to add more target
              properties for better filtering.
    *************************************************************************/
    private TextMessage createMessage(String bodyStr, String senderStr,
                                      String targetStr) throws Exception {
        TextMessage msg = p_pubSession.createTextMessage();
        msg.setText(bodyStr);
        msg.setStringProperty("username", System.getProperty("user.name"));
        msg.setStringProperty("hostname", p_hostnameKey);
        Target target = new Target(targetStr);
        Sender sender = new Sender(senderStr);
        if (!target.getID().equals("")) {
            msg.setStringProperty("targetid", target.getID());
        }
        if (!target.getName().equals("")) {
            msg.setStringProperty("targetname", target.getName());
        }

        msg.setStringProperty("senderid", sender.getID());
        msg.setStringProperty("sendername", sender.getName());
        target.toMsg(msg);
        sender.toMsg(msg);       

        // Record the id and name of the first app
        // This assumes the first app to send a message is the main app, so
        // when this app unsubscribes, this program will quit.  This isn't
        // a good way to handle this.  Main app should send a separate
        // message for this.
        if (p_startName == null) {
            p_startName = sender.getName();
            p_startID = sender.getID();
            if (p_verbose) {
                System.out.println("start id = " + p_startID + " start name = "
                                   + p_startName);
            }
        }
        return msg;
    }
	

    private Topic getTopic(String topicName) throws Exception 
    {

       Topic topic;

       if (p_topics.containsKey(topicName)) {
          topic = p_topics.get(topicName);

       } else {
          if (p_openjms) {
             topic = p_pubSession.createTopic(topicName);

          } else {
             topic = (Topic)p_jndi.lookup(topicName);
          }
          p_topics.put(topicName, topic);
       }

       return topic;
    }



    /**
     * Closes the TopicConnection for the given subscriber.  
     * This automatically removes all the subscriptions from the JMS
     * server.
     */
    public synchronized void unsubscribe(String data) 
    {

        try {
          
            String subscriberID = MessageHelper.getItem(data, "ID");
            String subscriberName = MessageHelper.getItem(data, "NAME");
          

            if (p_verbose) {
               System.out.println("Unsubscribing " 
                     + subscriberID+subscriberName);
            }
            // First close server subscriptions:
            TopicConnection connection = 
               getSubscriberConnection(subscriberID, subscriberName);

            if (connection == null) {
               System.out.println(
                     "Can't unsubscribe - no subscriptions exist yet!");
            } else {                    
               connection.close();
               p_subscriberConnections.remove(subscriberID +
                     subscriberName);
            }

            // Now close local subscriptions:
            Iterator it = p_localSubscribers.values().iterator();
            HashMap appsForTopic;
            HashMap appsForName;

            while(it.hasNext()) {
               appsForTopic = (HashMap) it.next();
               if(appsForTopic.containsKey(subscriberName)) {
                  appsForName = 
                     (HashMap) appsForTopic.get(subscriberName);
                  appsForName.remove(subscriberID);
               }
            }


            // Quit if the app is the first app that was started up
            // (i.e., the "main" app)
            // 09/15/06 Changed code to do normal unsubscribe before
            // quit.  Not sure if its useful but maybe there is a 
            // problem in timing causing the stack traces.
            if (p_startName.equals(subscriberName) && 
                p_startID.equals(subscriberID)) {
                quit();
            }

        } catch (Exception e) {
            e.printStackTrace();
        }
    }


    /**********************************************************************
     Method:  subscribe
     Summary: Subscribes to the given topic.
    **********************************************************************/
    public synchronized void subscribe (String data) throws Exception {


        String topicName = MessageHelper.getItem(data, "TOPIC");

        // See if this is a local subscription by its filter type:
        String filterType = getFilterType(topicName);
        
        // We do subscriptions 2 ways:
        // 1) If this is a local ONLY message, add subscriber to
        // internal cache:
        if (filterType.equals("USER:HOSTNAME:DISPLAY")) {
            subscribeLocal(data);
            
        // 2) Otherwise, add this subscriber to the JMS Server:
        } else {
            subscribeServer(data);
        }
    }
    /**********************************************************************
     Method:  subscribeLocal
     Summary: Caches the subscriptions locally, so it never has to
              go through the server.
    **********************************************************************/
    public void subscribeLocal (String data) throws Exception {

        String topicName = MessageHelper.getItem(data, "TOPIC");
        String subscriberName = MessageHelper.getItem(data, "NAME");
        String subscriberID = MessageHelper.getItem(data, "ID");
        int outPort = MessageHelper.getIntItem(data, "PORT");
        boolean selfFilter = MessageHelper.getBoolItem(data,"SELFFILTER");

        MessageHandler handler = new MessageHandler(outPort, topicName,
                                                 subscriberID+subscriberName,
                                                    selfFilter);

        HashMap<String,HashMap<String,MessageHandler>> appsByTopic = p_localSubscribers.get(topicName);
        if (appsByTopic == null) {
            appsByTopic = new HashMap<String,HashMap<String,MessageHandler>>();
            p_localSubscribers.put(topicName, appsByTopic);
        }
        HashMap<String,MessageHandler> appsByName = appsByTopic.get(subscriberName);
        if (appsByName == null) {
            appsByName = new HashMap<String,MessageHandler>();
            appsByTopic.put(subscriberName, appsByName);
        }
        // should only be one handler per topic per subscriber:
        appsByName.put(subscriberID, handler);
    }
            
    /**********************************************************************
     Method:  subscribeServer
     Summary: Subscribes to the given topic.  Uses the target to create
              a filter for the subscription.  By default, the subscription
              always filters on user name, so that an Ecce app will only
              get messages from the same user who started the app.

              If this subscriber has never subscribed before, it creates
              a new topic connection for this subscriber.
    **********************************************************************/
    public void subscribeServer(String data) throws Exception {

        String topicName = MessageHelper.getItem(data, "TOPIC");
        String subscriberName = MessageHelper.getItem(data, "NAME");
        String subscriberID = MessageHelper.getItem(data, "ID");
        int outPort = MessageHelper.getIntItem(data, "PORT");
        boolean selfFilter = MessageHelper.getBoolItem(data,"SELFFILTER");

        String filterType = getFilterType(topicName);
        String messageSelector = new String();
        
        if (filterType.equals("USER")) {
            messageSelector = "username='"
                + System.getProperty("user.name") + "'";
        }
            
        // Add the self filter
        if (selfFilter == true) {
            if (!messageSelector.equals("")) {
                messageSelector += " AND ";
            }
            messageSelector += "senderid<>'" + subscriberID + "'";
        }
        
        // Add the target filter
        if (!messageSelector.equals("")) {
            messageSelector += " AND ";
        }
        messageSelector += "(targetid is NULL OR targetid ='" 
            + subscriberID + "') AND (targetname is NULL or targetname ='" 
            + subscriberName + "')";
        
        // Create message handler
        MessageHandler handler = new MessageHandler(outPort, topicName,
                                                 subscriberID+subscriberName);
        
        // Get the topic connection for this subscriber:
        TopicConnection connection = getSubscriberConnection(subscriberID, 
                                                             subscriberName);
        try {
            
            TopicSession session = connection.createTopicSession(false, 
                                               Session.AUTO_ACKNOWLEDGE);
            Topic topic = getTopic(topicName);
            
            // subscribe
            TopicSubscriber subscriber = 
                session.createSubscriber(topic, messageSelector, false);
            
            subscriber.setMessageListener(handler);
            
        } catch (Exception e) {
            System.out.println("Failed to add subscriber:  " + subscriberID
                               + subscriberName +
                               "\nDid you call startSubscriber to early??");
            throw e;
        }
    }

   /**********************************************************************
     Method:  holdMessages
     Summary: Lets the calling app temporarily suspend messaging.
    **********************************************************************/
    public synchronized void holdMessages(String data) throws Exception {
        
        MessageHelper.holdMessages(data);

    }

   /**********************************************************************
     Method:  resumeMessaging
     Summary: Lets the calling app resume messaging if it was suspended.
    **********************************************************************/
    public synchronized void resumeMessaging(String data) throws Exception {
        
        MessageHelper.resumeMessaging(data);

    }

   /**********************************************************************
     Method:  startSubscriber
     Summary: After the app has subscribed to all the topics it needs to,
              then it calls this message to start the connection for that
              subscriber.  Connection has to be started AFTER the subscribers
              have been added, because the session can have only ONE
              thread.
    **********************************************************************/
    public synchronized void startSubscriber(String data) throws Exception {

        try {
          
            String subscriberID = MessageHelper.getItem(data, "ID");
            String subscriberName = MessageHelper.getItem(data, "NAME");
 
            if (p_verbose) {
                System.out.println("Starting connection for " 
                                   + subscriberID+subscriberName);
            }
            TopicConnection connection = 
                getSubscriberConnection(subscriberID, subscriberName);
                
            if (connection == null) {
                System.out.println("Can't start subscriber connection - "+
                                   "no subscriptions exist yet!");
            } else { 
                connection.start();
            }
            
        } catch (Exception e) {
            e.printStackTrace();
        }
    }


    private String getFilterType(String topicName) throws Exception{
        String filterType = null;
        
        if (p_messageTopics.containsKey(topicName)) {
	    filterType = p_messageTopics.get(topicName);
	    
	} else {
            throw new Exception("Unregistered topic: " + topicName + ".  " +
                                "See $ECCE_HOME/data/client/config/" + 
                                "ecce_messages");
        }
        return filterType;
    }

    private TopicConnection getSubscriberConnection(String subscriberID,
                                                    String subscriberName) 
    throws Exception {

        TopicConnection connection =  null;
	String key = subscriberID + subscriberName;

        // Get the connection for the given app (key)
	if (!p_subscriberConnections.containsKey(key)) {
            // If not in the map, create a new one
            connection = createConnection();
            p_subscriberConnections.put(key, connection);

	} else {
            connection = p_subscriberConnections.get(key);
        }
 
        return connection;
    }

    private TopicConnection createConnection() throws Exception{
        TopicConnection ret = null;
        
        // Create the JMS Connection:
        try {
            // lookup the connection factory from the context
            TopicConnectionFactory connectionFactory = (TopicConnectionFactory)
                p_jndi.lookup("JmsTopicConnectionFactory");

            // if we can't find the factory then throw an exception
            if (connectionFactory == null) {
                throw new RuntimeException(
                   "Failed to locate connection factory");
            }

            // create anonymous connection
            ret = connectionFactory.createTopicConnection();
            
        } catch (Exception e) {
            throw e;
        }
        return ret;
    }

   public static void main (String[] args) {

       // parse command-line arguments
       boolean verboseFlag = false;
       boolean openjmsFlag = false;
       String appCommand = null;
       int idx = 0;

       while (idx < args.length) {

           if (args[idx].equals("-v")) {
               verboseFlag = true;

           } else if (args[idx].equals("-c")) {
               appCommand = new String();
               while (++idx < args.length) {
                   appCommand += args[idx] + " ";
               }
               // System.out.println("command = " + appCommand);

           } else if (args[idx].equals("-o")) {
               openjmsFlag = true;
           }

           idx ++;
       }
       
       JMSDispatcher dispatcher = new JMSDispatcher(verboseFlag,
                                                    openjmsFlag);
       try {           
           
           // Start up the root app, if provided
           if (appCommand != null && !appCommand.equals("")) {
                Execer execer = new Execer(dispatcher, appCommand,
                                           verboseFlag);
                execer.start();
            }            
            dispatcher.mainLoop();     
            
       } catch (Exception e) {
           if (e.toString() != "java.lang.NullPointerException")
               System.out.println(e.toString());
           e.printStackTrace();
       }
   }
}





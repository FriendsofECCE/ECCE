// Includes:
   import javax.jms.*;
   import java.util.Properties;
   import java.util.HashMap;
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

//import org.exolab.jms.client.JmsTopicConnectionFactory;
//import org.exolab.jms.jndi.rmi.RmiJndiInitialContextFactory;


public class JMSServerTest {

    //---------------------------------------------------------------
    // DATA MEMBERS:
    //---------------------------------------------------------------
    private TopicConnection p_connection;

    private TopicSession    p_session;

    private Topic           p_topic;

    private TopicPublisher  p_publisher;

    //---------------------------------------------------------------
    // MEMBER FUNCTIONS:
    //---------------------------------------------------------------

    /*******************************************************************
      Method : Constructor

    *******************************************************************/
    public JMSServerTest() {

        // Now create the JMS Connection:
        try {
            Properties props = new Properties();

            // Create connection and session (note that the url for the
            // jndi server is located in $ECCE_HOME/siteconfig/jndi.properties)
            Context context = new InitialContext();

            // lookup the connection factory from the context
            TopicConnectionFactory connectionFactory = (TopicConnectionFactory)
                context.lookup("JmsTopicConnectionFactory");
	
            // if we can't find the factory then throw an exception
            if (connectionFactory == null) {
                throw new RuntimeException(
                                 "Failed to locate connection factory");
            }
       
            p_connection = connectionFactory.createTopicConnection();
            
	
            p_session = p_connection.createTopicSession(false, 
                                                  Session.AUTO_ACKNOWLEDGE);
            
            p_topic = (Topic) context.lookup("ecce_poll");

            // Create publisher:
            p_publisher = p_session.createPublisher(p_topic);
            
            // Subscribe to test message:
            for (int i = 0; i < 10; i++) {
                subscribeToTestMessage();
            }
            
            // Have to start connection after all subscribers added:
            p_connection.start();

        } catch (Exception e1) {
            System.out.println("Unable to connect to JMS server!");
            System.out.println(e1.getMessage());
            e1.printStackTrace();
            System.exit(-1);
        }

    }

    public void subscribeToTestMessage() {

        try {
            TopicSubscriber subscriber =  p_session.createSubscriber(p_topic, 
                                                         null,   // no filter 
                                                         false); // nolocal
        
            subscriber.setMessageListener
                ( 
                 new MessageListener() {
                         public void onMessage(Message msg) {
                             System.out.println("just got message");
                         }
                     }
                 );
            
        } catch (Exception ex) {
            ex.printStackTrace(); 
        }

    }

    public void sendTestMessage () {

        try {          
            TextMessage msg = p_session.createTextMessage();
            p_publisher.publish(msg, DeliveryMode.NON_PERSISTENT, 4, 0);

        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    public static void main (String[] args) {

        try {
            JMSServerTest test = new JMSServerTest();
            
            while(true) {
                //Thread.sleep(100);
                test.sendTestMessage();
            }

        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}








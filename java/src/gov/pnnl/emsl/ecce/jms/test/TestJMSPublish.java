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



/**
 * This class is a simple publisher test program.
 *
 * It can be used in conjunction with scripts to hammer the server.
 * TestJMSPublish -topic <topic> -cnt <number>
 */
public class TestJMSPublish {

    //---------------------------------------------------------------
    // DATA MEMBERS:
    //---------------------------------------------------------------
    private TopicConnection p_connection;

    private TopicSession    p_session;

    private Topic           p_topic;

    private TopicPublisher  p_publisher;

    private String          p_topicName = "ecce_poll";

    private int             p_cnt = 1;
    //---------------------------------------------------------------
    // MEMBER FUNCTIONS:
    //---------------------------------------------------------------

    /*******************************************************************
      Method : Constructor

    *******************************************************************/
    public TestJMSPublish(String topic, int cnt) {
        if (topic != null) p_topicName = topic;
        if (cnt > 0) p_cnt = cnt;

        // Now create the JMS Connection:
        try {

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
            
            p_topic = (Topic) context.lookup(p_topicName);

            // Create publisher:
            p_publisher = p_session.createPublisher(p_topic);
            
            // Have to start connection after all subscribers added:
            p_connection.start();

        } catch (Exception e1) {
            System.out.println("Unable to connect to JMS server!");
            System.out.println(e1.getMessage());
            e1.printStackTrace();
            System.exit(-1);
        }

    }

    public void close() {

      try {
       p_connection.close();
      } catch (Exception ex) {
        System.out.println(ex.getMessage());
        System.exit(-1);
      }
    }


    public void sendTestMessage () {

        try {          
            for (int idx=1; idx<=p_cnt; idx++) {
              System.out.println("Sending " + p_topicName + ": " + idx);
              TextMessage msg = p_session.createTextMessage(p_topicName);
              p_publisher.publish(msg, DeliveryMode.NON_PERSISTENT, 4, 0);
            }

        } catch (Exception e) {
            e.printStackTrace();
        }

    }

    public static void main (String[] args) {

        // TODO - get cmd line args
        try {
            TestJMSPublish test = new TestJMSPublish("ecce_poll",1);
            
            test.sendTestMessage();
            test.close();

        } catch (Exception e) {
            e.printStackTrace();
            System.exit(-1);
        }
    }
}








// Includes:
   import javax.jms.*;
   import java.util.Properties;
   import java.util.HashMap;
   import java.util.StringTokenizer;
   import java.util.Vector;
   import java.io.IOException;
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
 * This class is a simple subscriber test program.
 *
 * It can be used in conjunction with scripts to hammer the server.
 * TestJMSSubscribe -topic <topic> -cnt <number>
 */
public class TestJMSSubscribe implements MessageListener {

    //---------------------------------------------------------------
    // DATA MEMBERS:
    //---------------------------------------------------------------
    private TopicConnection p_connection;

    private TopicSession    p_session;

    private Topic           p_topic;

    private String          p_topicName = "ecce_poll";

    private int             p_cnt = 1;

    private int             p_curCnt = 0;

    //---------------------------------------------------------------
    // MEMBER FUNCTIONS:
    //---------------------------------------------------------------

    /*******************************************************************
      Method : Constructor

    *******************************************************************/
    public TestJMSSubscribe(String topic, int cnt) {

        if (topic != null) p_topicName = topic;
        if (cnt > 0) p_cnt = cnt;

    }

    public void initiate() {
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
            
            p_topic = (Topic) context.lookup(p_topicName);

            subscribeToTestMessage();

            // Have to start connection after all subscribers added:
            p_connection.start();


        } catch (Exception e1) {
            System.out.println("Unable to connect to JMS server!");
            System.out.println(e1.getMessage());
            e1.printStackTrace();
            System.exit(-1);
        }
    }

    public void makeItWait() {
       try {
         System.in.read();
       } catch (IOException ignore) {
       }
    }

    public void subscribeToTestMessage() {

        try {
            TopicSubscriber subscriber =  p_session.createSubscriber(p_topic, 
                                                         null,   // no filter 
                                                         false); // nolocal
        
            subscriber.setMessageListener(this);
            
        } catch (Exception ex) {
            ex.printStackTrace(); 
        }

    }

    public void onMessage(Message msg) {
      System.out.println("***Got Message" + msg.toString());
      p_curCnt++;
      if (p_curCnt >= p_cnt) {
         exit();
      }
    }

    public void exit() {
      System.out.println("exit begin");

      try {
      System.out.println("close begin");
//       p_connection.close();
      System.out.println("close end");
      } catch (Exception ex) {
        System.out.println(ex.getMessage());
      }
      System.out.println("exit");
      System.exit(-1);
    }

    public void startTimeout(long mills) {
       Runner runner = new Runner(mills);
       Thread hack = new Thread(runner);
       hack.start();
       exit();
    }

    private class Runner implements Runnable {
       private long p_mills = 5000;

       public Runner(long mills) 
       {
          p_mills = mills;
       }

       public void run() {
          try {
            Thread.currentThread().sleep(p_mills);
            System.out.println("timed out dude!");
          } catch (InterruptedException ignore) {
          }
       }
    }

    public static void main (String[] args) {

        // TODO support cmd lines args
        try {
            // constructor does all
            TestJMSSubscribe test = new TestJMSSubscribe("ecce_poll",1);
            //test.startTimeout((long)8000);
            test.initiate();
            test.makeItWait();
            
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}








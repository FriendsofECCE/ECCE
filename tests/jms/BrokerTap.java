// Subscribes to every topic on an ECCE broker, as any client of a shared
// broker could, and reports each message it can read:
//   TAP <topic> secret=<yes|no>
// "secret" is whether the second argument appears in the body or in any
// property.  Used by run_tests.py; never installed.
//
//   java -cp <activemq-all.jar>:<dir> BrokerTap <port> <seconds> <secret>
import java.util.Enumeration;
import javax.jms.*;
import org.apache.activemq.ActiveMQConnectionFactory;

public class BrokerTap {
    public static void main(String[] a) throws Exception {
        final String secret = a[2];
        Connection c = new ActiveMQConnectionFactory(
            "tcp://localhost:" + a[0]).createConnection();
        Session s = c.createSession(false, Session.AUTO_ACKNOWLEDGE);
        MessageConsumer mc = s.createConsumer(s.createTopic(">"));
        mc.setMessageListener(new MessageListener() {
            public void onMessage(Message m) {
                try {
                    String topic = ((Topic) m.getJMSDestination()).getTopicName();
                    if (topic.startsWith("ActiveMQ.")) return;
                    boolean seen = m instanceof TextMessage
                        && ((TextMessage) m).getText().contains(secret);
                    for (Enumeration e = m.getPropertyNames(); e.hasMoreElements();) {
                        Object v = m.getObjectProperty((String) e.nextElement());
                        if (v != null && v.toString().contains(secret)) seen = true;
                    }
                    System.out.println("TAP " + topic + " secret=" + (seen ? "yes" : "no"));
                    System.out.flush();
                } catch (Exception e) {
                    System.out.println("TAPERR " + e);
                }
            }
        });
        c.start();
        System.out.println("READY");
        System.out.flush();
        Thread.sleep(Long.parseLong(a[1]) * 1000);
        c.close();
    }
}

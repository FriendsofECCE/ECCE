//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: MessageHelper
//
// DESIGN: Helps keep track of whether messages are suspended for a given
//         subscriber.
//
//////////////////////////////////////////////////////////////////////////////
package gov.pnnl.emsl.ecce.jms;


// Includes:
import java.util.HashMap;


public class MessageHelper {

  // DATA MEMBERS:
     public static final int MAX_PACKET_LENGTH = 4096;

     private static HashMap<String,String> suspendedSubscribers;


 // STATIC INITIALIZATION:
    static {
         suspendedSubscribers = new HashMap<String,String>();
    }

  // MEMBER FUNCTIONS:

   /**********************************************************************
     Method:  holdMessages
     Summary: Discards all incoming messages for the given subscriber
              until resumeMessaging is called.
    **********************************************************************/
    public static synchronized void holdMessages(String data) {

        try {
          
            String subscriberID = getItem(data, "ID");
            String subscriberName = getItem(data, "NAME");
            String key = subscriberID+subscriberName;
          
            //System.out.println("Temporarily stopping messages for " + key);
  	    suspendedSubscribers.put(key, "stopped");
        } catch (Exception e) {
            e.printStackTrace();
        }
    }


   /**********************************************************************
     Method:  resumeMessaging
     Summary: Resumes incoming messages for the given subscriber.
    **********************************************************************/
    public static synchronized void resumeMessaging(String data) {

        try {
          
            String subscriberID = getItem(data, "ID");
            String subscriberName = getItem(data, "NAME");
            String key = subscriberID+subscriberName;
            
            //System.out.println("Resuming messages for " + key);
           
            if (suspendedSubscribers.containsKey(key)) {
                suspendedSubscribers.remove(key);
            } else {
                //System.out.println(
                //           "Messaging not suspended - nothing to resume!");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }


    public static boolean messagingSuspended(String subscriberID) {
        boolean ret = false;
        
        if (suspendedSubscribers.containsKey(subscriberID)) {
            ret = true;
        }
        return ret;
    }

    /*******************************************************************
      Method : getItem
      Summary: Used to retrieve a tagged item from the datagram packet.
    *******************************************************************/ 
    public static String getItem(String packetData, String itemName) {

        String starttag = itemName + "START";
        String endtag = itemName + "END";
        String ret = new String("");

        int startpos = packetData.indexOf(starttag);
        int endpos = packetData.indexOf(endtag);

        if (startpos == -1 || endpos == -1) {
            System.out.println("Invalid datagram packet format: " +
                               packetData);
        } else if (startpos + starttag.length() < endpos) {

            ret = packetData.substring(startpos + starttag.length(),
                                          endpos);
        }
        return ret;
    }
    /*******************************************************************
      Method : getIntItem
      Summary: Used to retrieve a tagged item from the datagram packet
               and convert it to an integer.
    *******************************************************************/ 
    public static int getIntItem(String packetData, String itemName) {

        String item = getItem(packetData, itemName);
        int ret = Integer.parseInt(item);
        return ret;
    }
    /*******************************************************************
      Method : getBoolItem
      Summary: Used to retrieve a tagged item from the datagram packet
               and convert it to a boolean.
    *******************************************************************/ 
    public static boolean getBoolItem(String packetData, String itemName) {

        String item = getItem(packetData, itemName);
        boolean ret = Boolean.valueOf(item).booleanValue();
        return ret;
    }

}







//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Sender.java
//
// DESIGN: Used to marshal and unmarshal the sender info - which is used
//         to set properties and apply filters to the message.  Marshalling
//         is required because of the JNI interface.
//////////////////////////////////////////////////////////////////////////////
package gov.pnnl.emsl.ecce.jms;

import javax.jms.Message;

public class Sender {
    
    // This class's data has to be synchronized with the C++ equivalents
    // but we can add more if we need to
    private String p_name;
    private String p_id;
    
    public Sender() {
        p_name = new String();
        p_id = new String();
    }

    public Sender(String senderStr) {
        p_name = new String();
        p_id = new String();
        
        if (!senderStr.equals("")) {

            int newlinePos = senderStr.indexOf('\n');
            if (newlinePos != -1) {
                p_name = senderStr.substring(0, newlinePos);
                if (newlinePos < senderStr.length() -1) {
                    p_id = senderStr.substring(newlinePos + 1);
                }
            } else {
                System.out.println ("Invalid sender string format: " 
                                    + senderStr);
            }
        }            
    }

    public Sender (String name, String id) {
        p_name = name;
        p_id = id;
    }
    public Sender(Message msg) throws Exception {
        p_name = msg.getStringProperty("senderName");
        p_id = msg.getStringProperty("senderID");

    }   

    public String getName() {
        return p_name;
    }
    
    public String getID() {
        return p_id;
    }
    
    public String toString() {
     
        String ret = p_name + "\n" + p_id;
        return ret;
    }

    public void toMsg(Message msg) throws Exception{
        msg.setStringProperty("senderName", p_name);
        msg.setStringProperty("senderID", p_id);
    }

    public void setName(String name) {
       p_name = name;
    }
    
    public void setID(String id) {
        p_id = id;
    }
}

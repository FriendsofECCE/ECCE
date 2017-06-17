//////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Target.java
//
// DESIGN: Used to marshal and unmarshal the target info - which is used
//         to set properties and apply filters to the message.  Marshalling
//         is required because of the JNI interface.
//////////////////////////////////////////////////////////////////////////////
package gov.pnnl.emsl.ecce.jms;

import javax.jms.Message;
import java.util.StringTokenizer;

public class Target {
    
    // This class's data has to be synchronized with the C++ equivalents
    // but we can add more if we need to
    private String p_name;
    private String p_id;
    
    public Target() {
        p_name = new String();
        p_id = new String();
    }

    public Target(String targetStr) {
        p_name = new String();
        p_id = new String();
        
        if (!targetStr.equals("")) {

            int newlinePos = targetStr.indexOf('\n');
            if (newlinePos != -1) {
                p_name = targetStr.substring(0, newlinePos);
                if (newlinePos < targetStr.length() -1) {
                    p_id = targetStr.substring(newlinePos + 1);
                }
            } else {
                System.out.println ("Invalid target string format: " 
                                    + targetStr);
            }
        }            
    }

    public Target(String name, String id) {
        p_name = name;
        p_id = id;
    }

    public Target(Message msg) throws Exception {
        p_name = msg.getStringProperty("targetName");
        p_id = msg.getStringProperty("targetID");
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
        msg.setStringProperty("targetName", p_name);
        msg.setStringProperty("targetID", p_id);
    }

    public String getMessageSelector() {
        String messageSelector = new String();

        
        if (!p_name.equals("")) {
            messageSelector = "targetName='" + p_name + "'";
        }

        if (!p_id.equals("")) {
            
            if (!messageSelector.equals("")) {
                messageSelector = "(" + messageSelector + " OR targetID='"
                                  + p_id + "')" ;
            } else {
                messageSelector += "targetID='" + p_id + "'";
            }
        } 
          
        return messageSelector;
    }


    public void setName(String name) {
       p_name = name;
    }
    
    public void setID(String id) {
        p_id = id;
    }
}

/**
 * @file 
 *
 *
 */
#ifndef WXJMSSUBSCRIBER_HH
#define WXJMSSUBSCRIBER_HH

#include <map>
#include <string>
  using std::map;
  using std::string;
  using std::less;

#include "wx/wx.h"
#include "wx/socket.h"
#include "wx/event.h"

#include "wxgui/WxJMSListener.H"

class WxJMSSubscriber;
class JMSMessage;


// Typedef to simplify dealing with maps
typedef map<string, wxJMSCallbackStructure*, less<string> > wxCbMap;



/**
 * This class provides asynchronous subscriptions using the pub/sub model.
 * This class allows only 1 subscription per topic (subscriptions are 
 * cached).  This class assumes that all registered callback functions 
 * are members of itself, so that means you need to inherit from 
 * WxJMSSubscriber if you want to subscribe with a callback function.
 */
class WxJMSSubscriber : public wxEvtHandler {

  public:

    WxJMSSubscriber(const string& toolName);   
    virtual ~WxJMSSubscriber();

    string getMyName() const;  
    string getMyID() const;
    int    getPort();
    bool   messagingEnabled();

    virtual bool subscribe(const char* topic, 
                   WxJMSListener *l, wxJmsCBFunc handler,
                   bool filterSelf = true);
    bool subscribe(const char* topic, 
                   wxJmsStaticCBFunc handler,
                   bool filterSelf = true);

    bool startSubscriber();
    void unsubscribe();

    void processMessage(wxSocketEvent& event);
    bool holdMessages();
    bool resumeMessaging();

  private:

    void initSocket();
    bool subscribeInternal(const char* topic, 
                           wxJMSCallbackStructure* cb,
                           bool filterSelf);


    wxCbMap cbStructs;               /**< holds the Ecce app's callback classes & 
                                      * functions - one pair for each 
                                      * subscription */
    string p_toolName;               /**< Name of tool - used to stamp messages */
    bool p_messagingEnabled;         /**< Indicates if messaging is enabled */
    wxDatagramSocket *p_inputSocket; /**< Subscription socket, receives published
                                      * messages from JMS server.  Socket is a 
                                      * UDP (datagram) socket. */
    int p_inPort;                    /**< Port used to read/process messages */


    DECLARE_EVENT_TABLE()

}; 

#endif 



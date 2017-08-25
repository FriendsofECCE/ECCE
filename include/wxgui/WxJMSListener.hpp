/**
 * @file 
 *
 *
 */
#ifndef WXJMSLISTENER_HH
#define WXJMSLISTENER_HH


class WxJMSListener;
class JMSMessage;

// Generic typedef for callback function pointer
typedef void (WxJMSListener::*wxJmsCBFunc)(JMSMessage&);
typedef void (*wxJmsStaticCBFunc)(JMSMessage&);

// Struct for holding cbfunction and class pointers
union wxCBFuncPtr {
  wxJmsCBFunc classFunc;
  wxJmsStaticCBFunc staticFunc;
};
struct wxJMSCallbackStructure {     
  wxCBFuncPtr funcPtr;
  WxJMSListener* classPtr;
};



/**
 * This class acts as a type for the purpose of registering listeners
 * for message callbacks that are separate from the WxJMSSubscriber
 * class.  This distinction is necessary because we want individual frames
 * to be listeners and cannot use inheritance of WxJMSSubscriber since it
 * is a wxEvtHandler which will conflict with wxFrames inheritance.
 */
class WxJMSListener 
{

}; 

#endif 



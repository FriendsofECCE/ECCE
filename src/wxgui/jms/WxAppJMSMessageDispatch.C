/**
 * @file
 *
 */

#include "dsm/ResourceDescriptor.H"
#include "wxgui/WxAppJMSMessageDispatch.H"

WxAppJMSMessageDispatch::WxAppJMSMessageDispatch(const string& name, bool app)
  : ewxApp(), WxJMSMessageDispatch(name, app)
{
   p_appName = name;
}

/**
 * Destructor.
 */
WxAppJMSMessageDispatch::~WxAppJMSMessageDispatch()
{
}

string WxAppJMSMessageDispatch::getName() const
{
   return p_appName;
}

int WxAppJMSMessageDispatch::OnExit()
{
   unsubscribe();
   return 0;
}

/**
 * Subscribes to quit, invoke, identify messages.
 */
void WxAppJMSMessageDispatch::subscribeMessages()
{
   subscribe("ecce_invoke",
         (wxJmsCBFunc)&WxAppJMSMessageDispatch::invokeMCB);
   subscribe("ecce_quit",
         (wxJmsCBFunc)&WxAppJMSMessageDispatch::quitMCB);
   subscribe("ecce_identify", 
         (wxJmsCBFunc)&WxAppJMSMessageDispatch::identifyMCB);

}


/**
 * Invoke message call back function
 *
 * Deiconize and raise the frame
 * Note that there is a bug in wx-gtk1.2, the Raise or Iconize(false) won't really raise iconified window.
 * There is no such a problem with wx-gtk2.0.
 */
void WxAppJMSMessageDispatch::invokeMCB(JMSMessage& msg)
{
   wxTopLevelWindow *top = (wxTopLevelWindow*)GetTopWindow();
   top->Iconize(false);
   top->Raise();
   top->Show(true);
   top->SetFocus();
   top->RequestUserAttention();
}

/**
 * Quit message call back function
 */
void WxAppJMSMessageDispatch::quitMCB(JMSMessage& msg)
{
   //  if (msg.getSender().getName() == GATEWAY)
   if (msg.getSender().getName() == GATEWAY) {

      GetTopWindow()->Close(true);
      wxApp::OnExit();
   }
}


/**
 * Identify message call back function
 *
 */
void WxAppJMSMessageDispatch::identifyMCB(JMSMessage&)
{
   string value = identify();

   JMSMessage *msg = newMessage();
   msg->addProperty("info",value);
   publish("ecce_identify_reply", *msg);
   delete msg;
}


/**
* @file
*
*
*/
#ifndef WXAPPJMSMESSAGEDISPATCH_H
#define WXAPPJMSMESSAGEDISPATCH_H


#include "wxgui/ewxApp.H"
#include "wxgui/WxJMSMessageDispatch.H"


/**
 * A subclass that implements basic ecce messaging intended for
 * use by apps that don't fully participate in messaging like
 * the machine browser.
 * This class handles:
 *   <li>invoke
 *   <li>quit
 *   <li>identify
 */
class WxAppJMSMessageDispatch: public ewxApp, public WxJMSMessageDispatch 
{

   public: 

      WxAppJMSMessageDispatch(const string& app_name, bool app=true);
      virtual ~WxAppJMSMessageDispatch();

      virtual void subscribeMessages();
      virtual string getName() const;
      virtual string identify() = 0;

      int OnExit();

   protected:
      void identifyMCB(JMSMessage& msg);
      void quitMCB(JMSMessage& msg);
      void invokeMCB(JMSMessage& msg);

      string p_appName;
};

#endif 

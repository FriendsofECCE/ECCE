/**
 * @file
 * @author Karen Schuchardt
 *
 */

#ifndef BUILERAPP_HH
#define BUILDERAPP_HH

#include "util/JMSMessage.H"

#include "wxgui/WxDavAuth.H"
#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/ewxApp.H"

#include "Builder.H"

class BuilderApp : public ewxApp, public WxDavAuth, public WxJMSMessageDispatch
{
public:

  BuilderApp();
  virtual ~BuilderApp();
  virtual bool OnInit();
  virtual int OnExit();
  virtual string getName() const;

  void subscribeMessages();

  void invokeMCB(JMSMessage& msg);
  void quitMCB(JMSMessage& msg);
  void identifyMCB(JMSMessage& msg);
  void invokeStatusMCB(JMSMessage& msg);

  void statePrefsMCB(JMSMessage& msg);
  void unitsPrefsMCB(JMSMessage& msg);
  void pertabPrefsMCB(JMSMessage& msg);
  void miscPrefsMCB(JMSMessage& msg);
  void ffConfigEditMCB(JMSMessage& msg);

  void urlRenameMCB(JMSMessage& msg);
  void urlRemoveMCB(JMSMessage& msg);
  void urlStateMCB(JMSMessage& msg);

  void propertyChangeMCB(JMSMessage& msg);

  void authChangedMCB(JMSMessage& msg);

  void setSelectionMCB(JMSMessage& msg);

  string identify();

  bool checkServer() throw (RetryException,EcceException);
  bool checkServerSetup() throw (RetryException,EcceException);
  bool checkUser();
  
  void usage();

private:
  Builder *p_builder;
};

DECLARE_APP(BuilderApp)

#endif

/**
 * @file
 * @author Karen Schuchardt
 *
 */

#ifndef BUILERAPP_HH
#define BUILDERAPP_HH

#include "util/JMSMessage.H"

#include "wxgui/WxDavAuth.H"
#include "wxgui/ewxApp.H"

#include "Builder.H"

class BuilderMessages;

class BuilderApp : public ewxApp, public WxDavAuth
{
public:

  BuilderApp();
  virtual ~BuilderApp();
  virtual bool OnInit();
  virtual int OnExit();
  virtual string getName() const;

  bool checkServer();
  bool checkServerSetup();
  bool checkUser();
  
  void usage();

private:
  Builder *p_builder;
  BuilderMessages *p_msgs;   // the window's messages
};

DECLARE_APP(BuilderApp)

#endif

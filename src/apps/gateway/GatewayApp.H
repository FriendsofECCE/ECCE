/**
 * @file
 *
 * @author Lisong Sun
 *
 */

#ifndef _GATEWAYAPP_H_
#define _GATEWAYAPP_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "GatewayApp.C"
#endif

#include "wx/image.h"
#include "wxgui/WxDavAuth.H"
#include "wxgui/WxJMSMessageDispatch.H"
#include "wxgui/WxJMSSubscriber.H"
#include "wxgui/ewxApp.H"

class Gateway;
class SessionWatch;

static string cmdLineVersion;

class GatewayApp: public ewxApp, public WxDavAuth, public WxJMSMessageDispatch
{
  public:
    /** Parent for dialogs: the Gateway frame only when it is shown. */
    wxWindow* dialogParent();

public:

  GatewayApp();

  void checkSessionEnd();
  
protected:

  bool OnInit();
  int OnExit();
  string getName() const;

  void activityMCB(JMSMessage& msg);
  void identifyMCB(JMSMessage&);
  void identifyReplyMCB(JMSMessage& msg);
  void raiseMeMCB(JMSMessage&);
  void toolStartStatusMCB(JMSMessage& msg);
  void preferenceMCB(JMSMessage& msg);
  void authMCB(JMSMessage& msg);

  void sendIconify(int iconicState);
  void notifyExit();

  bool checkServer();
  bool checkServerSetup();
  bool checkUser();
  void reconnectJobStoreMessaging();
  int otherSessionApps() const;

  Gateway * p_gateway;

  //  Ends the session when the last app closes, while the frame is hidden.
  SessionWatch * p_sessionWatch;
  bool p_sessionSeen;
  int p_idleTicks;

  friend class Gateway;
};

DECLARE_APP(GatewayApp)

#endif
  // _GATEWAYAPP_H_

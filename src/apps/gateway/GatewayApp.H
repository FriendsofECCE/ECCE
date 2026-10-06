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

#include <set>

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

  // The name that actually authenticates the session's FIRST login
  // (this process's own startup checks, below) becomes the session's
  // server user from then on -- see Ecce::setSessionServerUser() and
  // the long comment on the definition. A later auth (e.g. retrying
  // into a folder this user has no access to) must NOT do this, hence
  // the p_sessionLoginFinalized guard.
  void authorizationAccepted(const AuthEvent& event);

  void sendIconify(int iconicState);
  void notifyExit();

  bool checkServer();
  bool checkServerSetup();
  bool checkUser();
  void reconnectJobStoreMessaging();
  int otherSessionApps() const;
  void procSessionApps(const char *const *notApps,
                       std::set<long>& apps) const;

  Gateway * p_gateway;

  //  Ends the session when the last app closes, while the frame is hidden.
  SessionWatch * p_sessionWatch;
  bool p_sessionSeen;
  int p_idleTicks;

  // True if `ecce -l` set the session's login before this process ever
  // started (captured at the very top of OnInit(), before anything
  // could touch it) -- ECCE_SERVER_LOGIN is session-only by design, so
  // the name that authenticates while it's set must never be written
  // to ~/.ECCE/ServerLogin[.remote].
  bool p_loginFromDashL;

  // Set true the first time authorizationAccepted() finalizes the
  // session's server user, so later ones (a different folder's auth)
  // are left alone.
  bool p_sessionLoginFinalized;

  friend class Gateway;
};

DECLARE_APP(GatewayApp)

#endif
  // _GATEWAYAPP_H_

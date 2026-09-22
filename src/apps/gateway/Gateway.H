/**
 * @file
 * @author Lisong Sun
 *
 * Gateway Application.
 *
 * This file implements the Ecce Gateway Application.
 *
 */

#ifndef _GATEWAY_H_
#define _GATEWAY_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "Gateway.C"
#endif

#include "wx/frame.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "wx/timer.h"

#define SYMBOL_GATEWAY_STYLE wxCAPTION|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxCLOSE_BOX
#define SYMBOL_GATEWAY_TITLE _("ECCE")
#define SYMBOL_GATEWAY_IDNAME 40000
#define SYMBOL_GATEWAY_SIZE wxSize(544, 68)
#define SYMBOL_GATEWAY_POSITION wxDefaultPosition

#define wxID_FEEDBACK 10001
#define wxID_PREFERENCE 10002
#define wxID_WINLIST 10003
#define ID_GATEWAY_QUIT_STOP_SERVER 10004

//  Issue #93: the Gateway window is hidden by default -- the launcher it
//  provided now lives in the Organizer (File > New Structure, and the
//  Tools menu), so every session no longer opens two windows one of
//  which is used about twice.
//
//  The window is HIDDEN, not removed. The frame is still constructed and
//  still owns the session: JMS, the ecce_get_app subscription, service
//  startup, and the Quit-and-Stop-Server path all live on it and are
//  untouched. That keeps the change to a single Show() call, and makes
//  it reversible without a rebuild:
//
//      ECCE_GATEWAY_WINDOW=1 ecce
//
//  brings the old window back exactly as it was, which is the point --
//  this is a UI decision that may turn out to be controversial, and the
//  cost of being wrong should be an env var rather than a revert.
bool eccGatewayWindowEnabled();

#define ID_GATEWAY_APPSBASE 11000

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif
#ifndef wxFIXED_MINSIZE
#define wxFIXED_MINSIZE 0
#endif

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxFrame.H"

class EcceTool;
class ewxMenu;
class Gateway;
class GatewayApp;
class GatewayPrefs;
class JMSMessage;
class Target;

class Gateway: public ewxFrame
{    

  DECLARE_CLASS( Gateway )
  DECLARE_EVENT_TABLE()

public:

  ~Gateway();
  Gateway(GatewayApp* app, wxWindow* parent,
          wxWindowID id = SYMBOL_GATEWAY_IDNAME, 
          const wxString& caption = SYMBOL_GATEWAY_TITLE,
          const wxPoint& pos = SYMBOL_GATEWAY_POSITION,
          const wxSize& size = SYMBOL_GATEWAY_SIZE,
          long style = SYMBOL_GATEWAY_STYLE);
  
protected:

  Gateway();

  bool Create(wxWindow* parent, wxWindowID id = SYMBOL_GATEWAY_IDNAME,
              const wxString& caption = SYMBOL_GATEWAY_TITLE,
              const wxPoint& pos = SYMBOL_GATEWAY_POSITION,
              const wxSize& size = SYMBOL_GATEWAY_SIZE,
              long style = SYMBOL_GATEWAY_STYLE);
  
  void CreateControls();

  void clearTargetList();
  void quit(bool doSave);

  void startActivity();
  void endActivity();
  void clearActivities();

  void OnCloseWindow(wxCloseEvent& event);
  void OnIconize(wxIconizeEvent &event);
  // GitHub #67: clamp a zero-height window, deferred to avoid reentrancy.
  void OnSize(wxSizeEvent& event);
  void OnTimer(wxTimerEvent& event);
  void OnMenuClick(wxCommandEvent& event);
  
  void setBusyCursor(bool isBusy);
  void toolActivate(wxCommandEvent &event);
  void exitGateway();
  void addMenuEntry(JMSMessage& msg);
  void preferenceChanged();

  // State
  static char *p_bigBusyPixmaps[];
  short p_numActivities;
  short p_pixmapIndex;
  GatewayPrefs * p_prefsDlg;
  wxTimer* p_timer;
  bool p_fixingSize;

  GatewayApp* p_app;

  EcceTool* p_main;
  EcceTool* p_help;
  EcceTool* p_feedback;
  EcceTool* p_prefs;
  EcceTool* p_win;

  ewxMenu* p_winMenu;

  ewxBitmap p_iconmain[10], p_iconhelp, p_iconfeedback,
    p_iconprefs, p_iconwindows;

  vector<Target *> p_targetList;
  Preferences * p_preference;

  /** Sept 07.  wx sends an iconize event during the destroy process for
   some reason.  Since in our case, this gets propagated to all tools, we
   need a special hack to know to ignore it. */
  bool p_isLastIconifyHack;

  friend class GatewayApp;
};

#endif

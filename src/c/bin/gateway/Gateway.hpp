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

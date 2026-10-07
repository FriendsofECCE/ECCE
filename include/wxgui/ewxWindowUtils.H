/**
 * @file
 *
 *
 */
#ifndef _EWXWINDOWUTILS_H  
#define _EWXWINDOWUTILS_H  

#include <string>
using std::string;

class wxWindow;
class wxSizer;
class wxTopLevelWindow;
#include <wx/gdicmn.h>
class Preferences;
class ewxConfig;

// Bring a shown top-level window to the front and ask for keyboard focus.
// GTK on an X11 display uses a fresh server timestamp (Mutter drops
// activation requests with a stale one, #120); other displays and ports use
// the toolkit's own call. ECCE_DEBUG_RAISE=1 logs the path taken to stderr.
void ewxRaiseWindow(wxWindow *win);


/**
 * This class contains a set of static utility functions that can be
 * applied to arbitrary windows.  The methods are collected in a class so
 * that we can avoid having to duplicate code for dialogs, frames...
 */
class ewxWindowUtils 
{
public:

  // Set a top-level window's desktop icon from its ResourceDescriptor
  // <Tool> entry, tolerating a missing one.
  //
  // Every app used to do this inline as
  //   SetIcon(wxIcon(ewxBitmap::pixmapFile(rs.getTool(X)->getIcon()), ...))
  // with no check, and getTool() returns NULL whenever that <Tool> is not
  // registered. metadyn SIGSEGV'd on EVERY launch for three weeks because
  // disconnecting MetaDyn from the menus also removed its registration
  // (found by tests/apps). A missing icon is not worth crashing over:
  // this leaves the window's default icon instead, and returns false.
  static bool setToolIcon(wxTopLevelWindow *win,
                          const std::string& toolName);

  static void saveWindowSettings(wxWindow *win,
                                 ewxConfig * config,
                                 bool saveSize);

  static void restoreWindowSettings(wxWindow *win,
                                    ewxConfig * config,
                                    bool restoreSize);

  static void saveWindowSettings(wxWindow *win,
                                 const string& prefix,
                                 Preferences & prefs,
                                 bool saveSize);

  static bool restoreWindowSettings(wxWindow *win,
                                    const string& prefix,
                                    Preferences & prefs,
                                    bool saveSize);

  static wxWindow *getTopWindow(wxWindow *win);

  static void processGlobalPreferenceChange(wxWindow *top);

  static void setUnitFamily(wxWindow *top, const string& family);

  static bool confirmExit(wxWindow *owner);

  static void setCustomDisabledStyle(wxWindow *win, bool enabled);

  // Small screens (#189).  The most client area a top-level window may use
  // so that its frame, including a title bar, stays inside the usable area
  // of its display.
  static wxSize clientCapForDisplay(wxTopLevelWindow *win);

  // Move the window back fully onto its display; never resizes it.
  static void keepOnDisplay(wxTopLevelWindow *win);

  // Cap the window to its display and keep it on screen.  Content whose
  // natural size exceeds the cap is moved into a scrolled window inside
  // the window's own sizer; fixedRow, a sizer inside that sizer (or NULL
  // to take a trailing row of buttons), stays below it and always visible.
  // Safe to call again after the content changes.
  static void fitToDisplay(wxTopLevelWindow *win, wxSizer *fixedRow = NULL);
};

#endif

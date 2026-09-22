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
class wxTopLevelWindow;
class Preferences;
class ewxConfig;


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
};

#endif

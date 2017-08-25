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

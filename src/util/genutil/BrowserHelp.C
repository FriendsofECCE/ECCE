/**
 * @file
 *
 *
 */

#include <iostream>
  using std::cout;
  using std::endl;
#include <strstream>
  using std::ostrstream;

#include <stdlib.h>              // system()
#include <string.h>
#include <unistd.h>               // access()

#include "util/Ecce.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/StringTokenizer.H"
#include "util/BrowserHelp.H"
#include "util/LocalData.H"
#include "util/NullPointerException.H"


///////////////////////////////////////////////////////////////////////////////
// class statics
// Statics hold mapping data so its loaded only once.
///////////////////////////////////////////////////////////////////////////////
string BrowserHelp::p_helpFile = "help.urls";
string BrowserHelp::p_urlPrefix = "";
string BrowserHelp::p_filePrefix = "";
Preferences *BrowserHelp::p_urlTranslations = 0;




/**
 * Constructor.
 * Help url mapping data is loaded if not already loaded.
 */
BrowserHelp::BrowserHelp()
{
  initialize();
}



/**
 * Destructor.
 * The static data is not deleted.  If this becomes a problem we
 * need to add a finalize type method.
 */
BrowserHelp::~BrowserHelp()
{
}



/**
 * Returns the URL for the specified key.
 */
string BrowserHelp::URL(const string& key) const
{
  string ret;
  if (!BrowserHelp::p_urlTranslations->getString(key,ret)) {
    BrowserHelp::p_urlTranslations->getString("HomeFallback",ret);
    if (ret == "") {
       std::cerr << "Warning - no fallback help url" << std::endl;
    }
  }
  
  if (!LocalData::dir().empty() && ret.find("http://") == string::npos) {
    // A file:// entry naming a page the install lacks opens the home page.
    if (ret.find("file://") == 0 &&
        access((p_filePrefix + ret.substr(7)).c_str(), R_OK) == 0)
      return "file://" + p_filePrefix + ret.substr(7);
    return localURL(ret.find("file://") == 0 ? string("HomeFallback") : ret);
  }

  if (ret.size() > 0 && ret.find("http://") == string::npos &&
                        ret.find("file://") == string::npos)
    ret.insert(0, p_urlPrefix);
  else if (ret.size() > 0 && (int)ret.find("file://") == 0)
    ret.insert(7, p_filePrefix);

  return ret;
}




/**
 * Local mode has no data server to serve help, so a page is opened from
 * the install tree instead.  The help CGIs only wrap a page in frames, so
 * "cshelp?tool&page" and "toolhelp?tool&page" become that page itself.
 * Pages written for the server refer to images as "/EcceHelp/...", which
 * a browser cannot resolve under file://, so images may be missing.
 */
string BrowserHelp::localURL(const string& entry) const
{
  string root = p_filePrefix + "EcceHelp/";
  string page = "homepage.html";
  size_t q = entry.find('?');
  if (entry.compare(0, 9, "EcceHelp/") == 0) {
    page = entry.substr(9);
  } else if (q != string::npos &&
             (entry.find("cshelp") != string::npos ||
              entry.find("toolhelp") != string::npos)) {
    string args = entry.substr(q + 1);
    size_t amp = args.find('&');
    string tool = args.substr(0, amp);
    string file = amp == string::npos ? "" : args.substr(amp + 1);
    string anchor;
    size_t amp2 = file.find('&');
    if (amp2 != string::npos) {
      anchor = "#" + file.substr(amp2 + 1);
      file = file.substr(0, amp2);
    }
    if (file.empty()) file = "overview.shtml";
    if (access((root + tool + "/" + file).c_str(), R_OK) == 0)
      return "file://" + root + tool + "/" + file + anchor;
  }
  size_t hash = page.find('#');
  if (access((root + page.substr(0, hash)).c_str(), R_OK) != 0)
    page = "homepage.html";
  return "file://" + root + page;
}


/**
 * The help page refered to by "key" is displayed in the help tool.
 * If the help tool isn't already running, it will be started.
 * Because of this, this method may take some time and callers may want
 * to provide some sort of busy notification.
 */
void BrowserHelp::showHelpPage(const string& key) const
{
   displayURL(URL(key));
}



/**
 * The ecce support queue feedback submission form is displayed in a web
 * browser window.  If a web browser isn't already running, it will be started.
 * Because of this, this method may take some time and callers may want
 * to provide some sort of busy notification.
 */
void BrowserHelp::showFeedbackPage(void) const
{
#if 000
   // old ecce-support queue
   string email = "ecce-support@emsl.pnl.gov";

   if (getenv("ECCE_SUPPORT") != NULL)
      email = getenv("ECCE_SUPPORT");

   displayURL(URL("Feedback") + email, true);
#else
   // new NWChem wiki ECCE forums
   string forums = "http://www.nwchem-sw.org/index.php/Special:AWCforum/sc/id4/ECCE:_Extensible_Computational_Chemistry_Environment.html";

   displayURL(forums, true);
#endif
}



/**
 * A new browser window is started to display the given url
 */
void BrowserHelp::showPage(const string& url) const
{
  displayURL(url, true);
}




/**
 * Reads in the help map file.  If the help file isn't found,
 * Dynamically determines which browser to use falling back to netscape
 * @throw NullPointerException if ECCE_HELP is not defined.  ECCE_HELP 
 */
void BrowserHelp::initialize()
{
  if (p_urlTranslations == (Preferences*)0)
    p_urlTranslations = new Preferences(BrowserHelp::p_helpFile, true);

  if (p_urlPrefix == "") {
    char* ehelp = getenv("ECCE_HELP");
    NULLPOINTEREXCEPTION(ehelp,"ECCE_HELP environment variable must be defined");
    p_urlPrefix = ehelp;
    if (p_urlPrefix.find_last_of('/') != p_urlPrefix.length()-1)
      p_urlPrefix.append("/");
  }

  if (p_filePrefix == "") {
    p_filePrefix = Ecce::ecceDataPath();
    p_filePrefix += "/client/WebHelp/";
  }
}

/**
 * The browser command, which may carry arguments.  Order: ECCE_BROWSER,
 * then Edit > Preferences, then whatever opener is on PATH.  Read on every
 * call so a preference change needs no restart.
 */
string BrowserHelp::browserCommand()
{
  const char* env = getenv("ECCE_BROWSER");
  if (env != NULL && *env != '\0')
    return env;

  Preferences pref(PrefLabels::GLOBALPREFFILE);
  string fromPref;
  if (pref.getString(PrefLabels::BROWSER, fromPref) && !fromPref.empty())
    return fromPref;

  // Debian ships firefox-esr, not firefox, so a bare "firefox" fails there.
  return findBrowserOnPath();
}

/**
 * First of these found on PATH; falls back to the literal "firefox" (the
 * pre-existing default) if none are -- system() will then report that
 * failure the same way it always did for a missing browser.
 */
string BrowserHelp::findBrowserOnPath()
{
  static const char* candidates[] = {
    "xdg-open", "x-www-browser", "sensible-browser",
    "firefox", "firefox-esr"
  };
  const char* path = getenv("PATH");
  if (path != NULL) {
    for (size_t c = 0; c < sizeof(candidates)/sizeof(candidates[0]); c++) {
      StringTokenizer tok(path, ":");
      while (tok.hasMoreTokens()) {
        string dir = tok.next();
        if (dir.empty())
          continue;
        string full = dir + "/" + candidates[c];
        if (access(full.c_str(), X_OK) == 0)
          return candidates[c];
      }
    }
  }
  return "firefox";
}

/**
 * xdg-open/x-www-browser/sensible-browser are generic openers that just
 * exec whatever the desktop's real browser is -- they don't understand
 * a "--new-window" argument themselves (it would be passed straight
 * through as if it were the URL). Only pass it to an actual browser
 * binary.
 */
bool BrowserHelp::supportsNewWindowFlag(const string& cmd)
{
  string base = cmd.substr(0, cmd.find_first_of(" \t"));
  size_t slash = base.find_last_of('/');
  if (slash != string::npos)
    base = base.substr(slash+1);
  return base != "xdg-open" && base != "x-www-browser" &&
         base != "sensible-browser";
}



/**
 * Sends command to netscape to display the url.  If netscape isn't
 * running, it will be started.
 */
void BrowserHelp::displayURL(const string& url, bool new_window)
{
   ostrstream os;
   // Get rid of quotes at either end of url
   int i;
   int i1=-1, i2=-1;
   bool start=true, end=true;
   for (i=0; i<(int)url.size(); i++) {
      if (start && url[i] == '"') {
         i1 = i+1;
      } else if (start && url[i] != ' ' && i1 == -1) {
         start = false;
      }
   }
   for (i=(int)url.size()-1; i>=0; i--) {
      if (end && url[i] == '"') {
         i2 = i-1;
      } else if (end && url[i] != ' ' && i2 == -1) {
         end = false;
      }
   }
   if (!start) i1 = 0;
   if (!end) i2 = url.size()-1;
   string noQuoteUrl = url.substr(i1,i2-i1+1);

   // The old Netscape/Mozilla "-remote openURL(...)" remote-control
   // protocol isn't honored by modern browsers -- they just start (or
   // raise) normally, ignoring the URL entirely, and still exit 0, so the
   // old exit-status-based fallback below never triggered. Pass the URL
   // as a plain argument instead, which every modern browser (including
   // Firefox) supports directly.
   string cmd = browserCommand();
   if (new_window && supportsNewWindowFlag(cmd)) cmd += " --new-window";
   cmd += " '" + noQuoteUrl + "' 2> /dev/null &";
   system(cmd.c_str());
}


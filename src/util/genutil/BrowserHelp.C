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

#include "util/Ecce.H"
#include "util/Preferences.H"
#include "util/StringTokenizer.H"
#include "util/BrowserHelp.H"
#include "util/NullPointerException.H"


///////////////////////////////////////////////////////////////////////////////
// class statics
// Statics hold mapping data so its loaded only once.
///////////////////////////////////////////////////////////////////////////////
string BrowserHelp::p_helpCmd = "";
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
  
  if (ret.size() > 0 && ret.find("http://") == string::npos &&
                        ret.find("file://") == string::npos)
    ret.insert(0, p_urlPrefix);
  else if (ret.size() > 0 && (int)ret.find("file://") == 0)
    ret.insert(7, p_filePrefix);

  return ret;
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

  if (p_helpCmd == "") {
    char* ehelpCmd = NULL;
    ehelpCmd = getenv("ECCE_BROWSER");

    // fallback to firefox if help browser variable not set
    if (ehelpCmd!=NULL && strcmp(ehelpCmd, "")!=0)
      p_helpCmd = ehelpCmd;
    else
      p_helpCmd = "firefox";
  }
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

   string cmd;

   cmd = p_helpCmd + " -remote 'openURL(" + noQuoteUrl;
   cmd += new_window? ", new-window)' 2> /dev/null": ")' 2> /dev/null";
   int status = system(cmd.c_str());
   if (status != 0) {
      cmd = p_helpCmd + " '" + noQuoteUrl + "'" + "&";
      system(cmd.c_str());
   }
}


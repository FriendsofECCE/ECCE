/**
 * @file
 *
 *
 */

#include <strstream>
  using std::ostrstream;

#include <iostream>
  using std::cout;
  using std::endl;

#include <sys/utsname.h> // uname

#include <wx/wx.h>

#include "util/NotImplementedException.H"
#include "util/NullPointerException.H"
#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"

#include "tdat/AuthCache.H"

#include "dsm/EDSIServerCentral.H"
#include "dsm/EcceDAVClient.H"

#include "wxgui/WxDavAuth.H"
#include "wxgui/WxAuth.H"
#include "wxgui/ewxMessageDialog.H"

/**
 * This class implements the AuthEventListener interface by using the
 * authorization cache and prompting when necessary.
 *
 * A typical use would be to inherit from this class in any app that
 * accesses dav.
 */
WxDavAuth::WxDavAuth(wxWindow *window) 
{
   p_window = window;
   p_promptCount = 0;
}



WxDavAuth::WxDavAuth(const WxDavAuth& rhs) 
{
   p_window = rhs.p_window;
   p_promptCount = 0;
   throw NotImplementedException("DavAuth copy constructor!", WHERE);
}



WxDavAuth::~WxDavAuth()
{}



void WxDavAuth::setAuthDialogParent(wxWindow *window)
{
   p_window = window;
}


/**
 * Get the authentication for the url and user.
 * Unlike the other getAuthorization methods, this one will only try
 * the first match it finds in the cache, then prompt.
 */
BasicAuth *WxDavAuth::getAuthorization(const string& url, const string& user)
{
   // Don't need this, the dialog accepts null for parent
   //NULLPOINTEREXCEPTION(p_window,
         //"Parent widget for authization window not set");

   BasicAuth *ba = 0;

   p_promptCount = 0;


   string username = user;
   if (username == "") {
      username = Ecce::serverUser();
   }

  EcceURL eurl(url);

  string whereami = "";
  struct utsname _uname;
  if (uname(&_uname) != -1)
    whereami = _uname.nodename;

   if (username!=Ecce::serverUser() ||
       eurl.getProtocol()=="http" ||
       eurl.getHost()!=whereami) {
     // Try for the first match in the cache
     ba = AuthCache::getCache().getAuthentication(url, username, "", 1);

     if (ba == 0) {
        // Try prompting just once
        string password;
        if (prompt(url, false, username, password, 1)) {
           ba = AuthCache::createBasicAuth(username, password);
        }
     }
   } else {
     ba = AuthCache::getCache().getBogusAuthentication();
   }

   return ba;
}



/**
 * Get authorization information in response to AuthEvent.
 * All possible matches from the cache will be tried before
 * prompting.
 *
 * The user and password fields of the event may be replacedd
 *
 * @throw RetryException after n user prompts
 * @return true if there is a new value to try.
 */
bool WxDavAuth::getAuthorization(AuthEvent& event) throw (RetryException)
{
   bool ret = false;

   // Don't really need this?
   //NULLPOINTEREXCEPTION(p_window,
         //"Parent widget for authization window not set");


   BasicAuth *ba = NULL;

   // Distinguish between cache retries and prompt retries
   // We might have lots of items in cache that COULD match
   if (event.m_retryCount == 1) {
      p_promptCount = 0;
   }


   // Supply default user name if needed
   // If left blank, matching policies will fail.
   if (event.m_user == "") {
      event.m_user = Ecce::serverUser();
   }

   // If we've started to prompt, no point in looking in cache.
   if (p_promptCount == 0) {
      ba = AuthCache::getCache().getAuthentication(event.m_url,
            event.m_user,
            event.m_realm,
            event.m_retryCount);
   }

   if (ba == 0) {
      // try prompting then
      p_promptCount++;

      if (p_promptCount > 3) {
         throw RetryException("Maximum retries exceeded (3).",WHERE);
      }

      static bool tryFileOnce = true;
      if (tryFileOnce) {
        tryFileOnce = false;
        string passFile = Ecce::realUserPrefPath();
        passFile += "ServerPass";
        FILE *fp = fopen(passFile.c_str(), "r");
        if (fp != NULL) {
          char buf[256];
          if (fgets(buf, sizeof(buf), fp) != NULL) {
            buf[strlen(buf)-1] = '\0';
            event.m_password = buf;
            ret = true;
          }
        }
      }

      if (!ret)
        ret = prompt(event.m_url,
              event.m_newUser,
              event.m_user,
              event.m_password,
              p_promptCount);
   } else {
      // Got something from cache
      event.m_user = ba->m_user;
      event.m_password = ba->m_pass;
      delete ba;
      ret = true;
   }


   return ret;
}




/**
 * Prompt user for password.
 * Username can also be changed.
 * @param retryCount if != 1 (first time), window looks slightly different??
 */
bool WxDavAuth::prompt(const string& strurl,
      const bool& newUser,
      string& user,
      string& password,
      int retryCount)
{
   bool ret;

   EcceURL url(strurl);

   // Only prompt for data server passwords because the underlying RCommand
   // code will prompt for any machine passwords it needs
   if (url.getProtocol() == "http") {
     WxAuth authDlg(p_window);

     if (newUser) {
       authDlg.setPrompt("You do not have an existing data server account!\nPlease enter a new data server password to create one:");
       authDlg.setPasswordLabel("  New\nPassword:");
     }

     authDlg.showChangeBtn(true);
     authDlg.setServer(url.getHost());
     authDlg.setProtocol("http");
     authDlg.setUser(user);

     int status;
     bool done = false;
     while (!done) {
       ret = false;
       done = true;
       status = authDlg.ShowModal();

       if (status == wxID_OK) {
         ret = true;
         user = authDlg.getUser();
         password = authDlg.getPassword();
         string newPassword = authDlg.getNewPassword();

         if (newUser) {
           if (password.length() >= 6) {
             ret = createNewUser(strurl, user, password);
           } else {
             done = false;
             ewxMessageDialog dlg(NULL,
                           "Unable to create new data server account.\n"
                           "Password must be at least 6 characters.",
                           "Create Data Server Account Failed",
                           wxOK|wxICON_EXCLAMATION|wxSTAY_ON_TOP,
                           wxDefaultPosition);
             dlg.ShowModal();
             authDlg.setPassword("");
           }
         } else if (newPassword != "") {
           if (newPassword.length() >= 6) {
             done = false;
             ret = changePassword(strurl, user, password, newPassword);
             if (ret) {
               done = true;
               // if the change succeeded, new password must be used
               password = newPassword;
             }
           } else {
             done = false;
             ewxMessageDialog dlg(NULL,
                           "Unable to change your data server password.\n"
                           "New password must be at least 6 characters.",
                           "Change Data Server Password Failed",
                           wxOK|wxICON_EXCLAMATION|wxSTAY_ON_TOP,
                           wxDefaultPosition);
             dlg.ShowModal();
             authDlg.setNewPassword("");
           }
         }
       }
     }
   }

   return ret;
}



bool WxDavAuth::createNewUser(const string& strurl, const string& user,
                              const string& password)
{
   bool ret = false;

   string urlbase = strurl;
   string::size_type pos = urlbase.rfind("/");
   if (pos != string::npos)
     urlbase.replace(pos+1, urlbase.length()-pos-1, "");

   string urlcgi = urlbase + "cgi-bin/ecce_accounts?parama=create&paramu=";
   string urlstr = urlcgi + user + "&paramr=Ecce-";

   string realm = EDSIServerCentral::readRealm();
   urlstr += realm;

   urlstr += "&paramn=" + password;

   EcceDAVClient edcCreate(urlstr);
   ostrstream ostr;
   ret = edcCreate.doPost(ostr) == 200;
   if (ret) {
     // just in case they already have this user,url cached even
     // though it is a new user
     AuthCache::getCache().changePass(urlbase, user, password);
   } else {
     string msg = "Unable to create new data server account.\n"
                  "Contact your site ECCE administrator.";
     ewxMessageDialog dlg(NULL, msg.c_str(),
                          "Create Data Server Account Failed",
                          wxOK|wxICON_EXCLAMATION|wxSTAY_ON_TOP,
                          wxDefaultPosition);
     dlg.ShowModal();
   }

   return ret;
}



bool WxDavAuth::changePassword(const string& strurl, const string& user,
                               const string& password,const string& newPassword)
{
   bool ret = false;

   string urlbase = strurl;
   string::size_type pos = urlbase.rfind("/");
   if (pos != string::npos)
     urlbase.replace(pos+1, urlbase.length()-pos-1, "");

   string urlcgi = urlbase + "cgi-bin/ecce_accounts?parama=modify&paramu=";
   string urlstr = urlcgi + user + "&paramo=" + password;
   urlstr += "&paramn=" + newPassword;

   EcceDAVClient edcModify(urlstr);
   ostrstream mostr;
   ret = edcModify.doPost(mostr) == 200;
   if (ret) {
     AuthCache::getCache().changePass(urlbase, user, newPassword);
   } else {
     ewxMessageDialog dlg(NULL,
                          "Unable to change your data server password.\n"
                          "Current password may be incorrect.",
                          "Change Data Server Password Failed",
                          wxOK|wxICON_EXCLAMATION|wxSTAY_ON_TOP,
                          wxDefaultPosition);
     dlg.ShowModal();
   }

   return ret;
}


/**
 * Add to cache because it worked.  The addAuthentication method is
 * responsible for not adding duplicates.
 */
void WxDavAuth::authorizationAccepted(const AuthEvent& event)
{
   AuthCache::getCache().addAuthentication(event.m_url, event.m_user,
                                           event.m_password,
                                           event.m_realm, true);
}


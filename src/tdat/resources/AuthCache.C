#include <sys/utsname.h> // uname
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <iostream>
  using std::cout;
  using std::endl;
#include <fstream>
  using std::ofstream;

#include "util/ErrMsg.H"
#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"
#include "util/TempStorage.H"
#include "util/JMSPublisher.H"

#include "tdat/RefMachine.H"
#include "tdat/AuthCache.H"


//--------------------------------------------------------------------------
//                         Class AuthTuple
//--------------------------------------------------------------------------
struct AuthTuple
{
  string url;
  string user;
  string pass;
};



//----------------------------------------------------------------------------
// The cache class.
// Non- persistent cache not really supported
// The cache is a list ordered front to back with the most recently used
// credentials maintained at the front of the list.
//----------------------------------------------------------------------------
AuthCache *AuthCache::p_singleton = NULL;
// Doing this at static init time is too soon.
//AuthCache *AuthCache::p_singleton = new AuthCache();

AuthCache::AuthCache()
{
  p_userPolicy = ANY_USER;

  // Need to use BEST_URL because mod_dav is set up with lots of authentication
  // domains and we want to match even if that changes.  
  //p_URLPolicy = EXACT_URL;
  p_URLPolicy = BEST_URL;
}

AuthCache::~AuthCache()
{
  flushCache();
}

void AuthCache::setURLPolicy(AuthCache::URLPolicy policy)
{
  p_URLPolicy = policy;
}

/**
 * Clear the in-memory cache.
 */
void AuthCache::flushCache()
{
  vector<AuthTuple*>::iterator it;
  for (it = p_memcache.begin(); it!=p_memcache.end(); it++) {
    delete (*it);
  }
  p_memcache.clear();
}


string AuthCache::pipeName()
{
  // Use the child pid for generating a unique FIFO file name
  // Originally I was trying to do this under the user's preference
  // directory, but that is AFS here in EMSL and AFS doesn't support FIFO!
  string authPipeTemplate = TempStorage::getTempRootPath();
  authPipeTemplate += "/AuthPipe.XXXXXX";
  char *authPipeName = new char[authPipeTemplate.length() + 1];
  strcpy(authPipeName, authPipeTemplate.c_str());
  // Would really prefer it to just generate the name of the file rather
  // than creating it, but Linux doesn't have a recommended way of doing
  // that so just create and delete it to accomplish the same thing
  int fd = mkstemp(authPipeName);
  if (fd != -1) {
    // Get rid of file because it needs to be created as a FIFO
    close(fd);
    unlink(authPipeName);
  }

  string ret = authPipeName;

  return ret;
}


void AuthCache::pipeOut(const string& pipeName)
{
  // pipeName will be a unique name per invocation
  if (mkfifo(pipeName.c_str(), S_IRUSR|S_IWUSR) == 0) {
    // Open will block until the reader opens the pipe as well
    // Blocking here is suboptimal because the caller (e.g. gateway) will go
    // out to lunch if the child app doesn't start properly.  Some kind of
    // timeout after a few seconds would be much preferred.
    int fd = open(pipeName.c_str(), O_WRONLY);
    if (fd != -1) {
      string line;
      line = StringConverter::toString(p_memcache.size()) + "\n";
      write(fd, line.c_str(), line.length());
      for (int idx=0; idx<p_memcache.size(); idx++) {
        AuthTuple *cur = p_memcache[idx];
        line = cur->url + "|" + cur->user + "|" + cur->pass + "\n";
        write(fd, line.c_str(), line.length());
      }
      close(fd);
    }
  }
}


void AuthCache::pipeIn(const string& pipeName)
{
  FILE *fp;
  int it=0;
  // Wait up to 10 seconds for the pipe to exist just in case the
  // child process is invoked first (e.g. launcher - > eccejobmaster)
  while ((fp = fopen(pipeName.c_str(), "r"))==NULL && it++<10) {
    sleep(1);
  }
  if (fp != NULL) {
    char buf[256];
    if (fgets(buf, sizeof(buf), fp) != NULL) {
      buf[strlen(buf)-1] = '\0';
      int nlines = (int)strtol(buf, NULL, 10);
      for (int in=0; in<nlines; in++) {
        if (fgets(buf, sizeof(buf), fp) != NULL) {
          buf[strlen(buf)-1] = '\0';
          StringTokenizer next(buf, "|");
          AuthTuple *tuple = new AuthTuple();
          tuple->url = next.next();
          tuple->user = next.next();
          tuple->pass = next.next();
          p_memcache.push_back(tuple);
        } else
          break;
      }
    }

    fclose(fp);
  }
  unlink(pipeName.c_str());
}


void AuthCache::msgIn(const JMSMessage& msg, const string& callerID)
{

  // already cached when the message is from the same app
  if (msg.getSender().getID() != callerID) {
    EcceURL url(msg.getProperty("url"));

    addAuthentication(url.getRef(), msg.getProperty("user"),
                      msg.getProperty("auth"), msg.getProperty("realm"), false);
  }
}


void AuthCache::pipeClean(const string& pipeName)
{
  if (pipeName != "") {
    FILE *fp = fopen(pipeName.c_str(), "r");
    if (fp != NULL) {
      char buf[256];
      if (fgets(buf, sizeof(buf), fp) != NULL) {
        buf[strlen(buf)-1] = '\0';
        int nlines = (int)strtol(buf, NULL, 10);
        for (int in=0; in<nlines; in++) {
          if (fgets(buf, sizeof(buf), fp) == NULL)
            break;
        }
      }
      fclose(fp);
    }
    unlink(pipeName.c_str());
  }
}


int AuthCache::size() const
{
  return p_memcache.size();
}


/**
 * The user has changed a password for a specific url/user so we need
 * to update our cache.
 */
void AuthCache::changePass(const string& urlbase, const string& user,
                           const string& newpass)
{
  vector<AuthTuple*>::const_iterator it;

  for (it = p_memcache.begin(); it!=p_memcache.end(); it++) {
    AuthTuple *cur = *it;
    if (cur->user==user && cur->url.find(urlbase)==0) {
      cur->pass = newpass;
    }
  }
}

////////////////////////////////////////////////////////////////////////////
// Description
//  Looks up either compute server or data server passwords in the
//  current authentication cache.  
////////////////////////////////////////////////////////////////////////////
BasicAuth *AuthCache::getAuthentication
(
   const string& urlstr,
   const string& inituser,
   const string& realm,
   int retryCount
)
{
  BasicAuth *ret = NULL;
  string user = inituser;
  string pass = "";

  EcceURL url(urlstr);
  string machine = url.getHost();
  string protocol = url.getProtocol();

  RefMachine* machineReference = RefMachine::refLookup(machine);
  if (machineReference != (RefMachine*)0) {
    vector<string>* remshells = machineReference->remshells();

    int it, numShells = remshells->size();
    for (it=0; it<numShells && (*remshells)[it]!=protocol; it++);
    if (it < numShells)
      pass = forMachine(url.getRef(), user, retryCount);
  }

  if (pass == "") {
    string key = makeKey(urlstr, realm);
    pass = find(key, user, retryCount);

    /**
     * ANY_USER.  I think its a bit
     * confusing - the user doesn't know who they are authenticated as.
     */
    if (pass=="" && p_userPolicy==ANY_USER) {
      pass = findAny(key, user, retryCount);
    }

    if (pass=="" && p_URLPolicy==BEST_URL) {
      pass = findBest(key, user, retryCount);
    }

    if (pass=="" && p_URLPolicy==LAST_URL) {
      if (p_memcache.size() > 0) {
        pass = p_memcache[0]->pass;
      }
    }
  }

  if (pass != "") {
    ret = new BasicAuth();
    ret->m_user = user;
    ret->m_pass = pass;
  }

  return ret;
}

////////////////////////////////////////////////////////////////////////////
// Description
//  Return empty authentication because this should only be called in a
//  degenerate case (same user, same machine) so the password information
//  won't be used anyway.
////////////////////////////////////////////////////////////////////////////
BasicAuth *AuthCache::getBogusAuthentication()
{
  BasicAuth *ret = new BasicAuth();

  ret->m_user = "";
  ret->m_pass = "";

  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
//   Hack to use existing machine stuff
/////////////////////////////////////////////////////////////////////////////
string AuthCache::forMachine(const string& url, const string& user, int retryCount)
{
   string ret = "";
   AuthTuple *cur;
   int idx = 1;
   if (retryCount <= size()) {
      vector<AuthTuple*>::iterator it;

      for (it = p_memcache.begin(); it!=p_memcache.end();) {
         cur = *it;
         if (cur->url == url && cur->user == user) {
            if (retryCount == idx) {
               if (cur->pass.length() == 0) {
                  p_memcache.erase(it);
                  delete cur;
               } else {
                  ret = cur->pass;
                  break;
               }
            } else {
               // only advance iterator if erase was not done
               it++;
               idx++;
            }
         } else {
           // only advance iterator if erase was not done
           it++;
         }
      }
   }
   return ret;
}


/**
 * Determine if an exact match for the given url and user exists in cache.
 */
bool AuthCache::exists(const string& url, const string& user) const
{
   bool ret = false;

   vector<AuthTuple*>::const_iterator it;
   for (it = p_memcache.begin(); it!=p_memcache.end(); it++) {
      AuthTuple *cur = *it;
      if (cur->url == url && cur->user == user) {
         ret = true;
         break;
      }
   }
   return ret;
}


/**
 * Determine if an exact match for the given url and user exists in cache.
 */
int AuthCache::index(const string& url, const string& user) const
{
   int ret = -1;
   vector<AuthTuple*>::const_iterator it;
   int idx = p_memcache.size()-1;
   for (it = p_memcache.begin(); it!=p_memcache.end(); it++) {
      AuthTuple *cur = *it;
      if (cur->url == url && cur->user == user) {
         ret = idx;
         break;
      }
      idx--;
   }
   return ret;
}



/**
 * Find exact match on user/url.
 */
string AuthCache::find(const string& url, const string& user, int retryCount)
{
   string ret = "";

   AuthTuple *cur;
   int idx = 1;
   if (retryCount <= size()) {
      vector<AuthTuple*>::iterator it;

      for (it = p_memcache.begin(); it!=p_memcache.end();) {
         cur = *it;
         if (cur->url == url && cur->user == user) {
            if (retryCount == idx) {
               if (cur->pass.length() == 0) {
                  p_memcache.erase(it);
                  delete cur;
               } else {
                  ret = cur->pass;
                  break;
               }
            } else {
               // only advance iterator if erase was not done
               it++;
               idx++;
            }
         } else {
           // only advance iterator if erase was not done
           it++;
         }
      }
   }
   return ret;
}


/**
 * Find a match that considers the ulr string minus the realm that
 * is currently appended.
 * This is useful to avoid failures/reprompting if the same authentication
 * works for lots of realms.  Note that on mod_dav, each url can have
 * a unique realm name.
 */
string AuthCache::findBest(const string& url, const string& user, 
                           int retryCount)
{
   string ret = "";
   vector<AuthTuple*>::iterator it;
   int idx=1;

   AuthTuple *cur;
   for (it = p_memcache.begin(); it!=p_memcache.end();) {
      cur = *it;
      // Get server/port part (minus the realm)
      string server = cur->url;
      int pos = server.find('"');
      if (pos != string::npos) {
         server = server.substr(0,pos);
      }
      if (cur->user == user && url.find(server) == 0) {
         if (retryCount == idx) {
            if (cur->pass.length() == 0) {
               p_memcache.erase(it);
               delete cur;
            } else {
               ret = cur->pass;
               break;
            }
         } else {
            it++;
            idx++;
         }
      } else {
         it++;
      }
   }
   return ret;
}



string AuthCache::findAny(const string& url, string& user, int retryCount)
{
   string ret = "";
   vector<AuthTuple*>::iterator it;
   int which = retryCount;
   int idx = 1;
   AuthTuple *cur;
   for (it = p_memcache.begin(); it!=p_memcache.end();) {
      cur = *it;
      if (cur->url == url) {
         if (which == idx) {
            if (cur->pass.length() == 0) {
               p_memcache.erase(it);
               delete cur;
            } else {
               user = cur->user;
               ret = cur->pass;
               break;
            }
         } else {
            it++;
            idx++;
         }
      } else {
         it++;
      }
   }
   return ret;
}


BasicAuth *AuthCache::createBasicAuth(const string& user, const string& password)
{
  BasicAuth *ret = new BasicAuth();
  ret->m_user = user;
  ret->m_pass = password;

  return ret;

}


/**
 * Add authentication to cache if a cache is being used.
 * If it already exists in the cache, it will be elevated to the most recent
 * entry.
 * When a password expires, it is in the cache but the value is incorrect
 * so we have to decide when to remove/replace it.
 */
bool AuthCache::addAuthentication
(
  const string& url,
  const string& user,
  const string& pass,
  const string& realm,
  const bool& publish
)
{
   bool ret = false;

   string key = makeKey(url, realm);
   int idx = index(key, user);
   if (idx < 0) {
      // Not found so just add it
      ret = true;

      // Always cache in memory even if disk cache
      addIt(key, user, pass);

   } else {
      if (idx != size()-1) {
         ret = true;
         //Found but we want it at the end
         // NOTE: Comment this whole block out to remove re-ordering
         // remove first so that it now will be at end of queue
         remove(url, user, realm);

         // Always cache in memory even if disk cache
         addIt(key, user, pass);

      } else {
         // Found it and its at the beginning but maybe the password changed
         vector<AuthTuple*>::iterator it = p_memcache.begin();
         AuthTuple *last = (*it);
         if (pass != last->pass) {
            ret = true;
            p_memcache.erase(it);
            // Always cache in memory even if disk cache
            addIt(key, user, pass);
         }
      }
   }

   if (ret && publish) {
      JMSPublisher publisher("AuthCache");

      JMSMessage *msg = publisher.newMessage();
      msg->addProperty("url", url);
      msg->addProperty("user", user);
      msg->addProperty("auth", pass);
      msg->addProperty("realm", realm);

      publisher.publish("ecce_auth_changed",*msg);
      delete msg;
   }

   return ret;
}


/**
 * Make the key into the cache.
 * The key currently consists of the server/port plus realm name.
 * In the past we kept the full url but that did not seem like the best
 * strategy for mod_dav with unique realms.
 */
string AuthCache::makeKey(const string& urlstr, const string& realm) const
{
   string ret;
   EcceURL url(urlstr);
   ret = url.getRef() + realm;
   return ret;
}



/**
 * Remove entry matching url, user, and realm.
 */
bool AuthCache::remove
(
  const string& url,
  const string& user,
  const string& realm
)
{
   bool ret = false;

   int size = p_memcache.size();
   vector<AuthTuple*>::iterator it = p_memcache.begin();
   string key = makeKey(url,realm);
   for (int idx=0; idx<size; idx++) {
      if (p_memcache[idx]->url == key && p_memcache[idx]->user == user) {
         delete (*(it+=idx));
         p_memcache.erase(it);
         ret = true;
         break;
      }
   }

   return ret;
}


/**
 * Add new entry to cache.
 * Currently, the cache has a hardwired size limit.  Once reached,
 * the oldest entry will be removed to make room for the new one.
 */
void AuthCache::addIt
(
  const string& url,
  const string& user,
  const string& pass
)
{
   AuthTuple *tuple = new AuthTuple();
   tuple->url = url;
   tuple->user = user;
   tuple->pass = pass;

   p_memcache.insert(p_memcache.begin(), tuple);
}



/**
 * Get singleton instance.
 */
AuthCache& AuthCache::getCache()
{
   if (p_singleton == NULL) {
      p_singleton = new AuthCache();
   }
   return *p_singleton;
}


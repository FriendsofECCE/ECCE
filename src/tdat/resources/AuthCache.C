#include <sys/utsname.h> // uname
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

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

  // Seed from the session credential store so that a process which was not
  // handed a -pipe at launch, or which was launched before anybody had
  // authenticated yet, still starts out holding this session's data server
  // credentials instead of prompting for them all over again.
  sessionLoad();
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


//--------------------------------------------------------------------------
//                      The session credential store
//--------------------------------------------------------------------------
//
// ECCE is several cooperating processes (gateway, organizer, builder,
// calced, ...) all talking WebDAV to the same per-user data server, and
// each one used to discover that server's password independently.  There
// were two sharing mechanisms and both are one-shot:
//
//   * a FIFO handed to a child on its command line as "-pipe <name>"
//     (pipeOut()/pipeIn() below), which carries whatever the parent had
//     cached at the instant of the fork and nothing learned later; and
//   * the "ecce_auth_changed" JMS broadcast published by
//     addAuthentication(), which only reaches processes already
//     subscribed at the moment it is sent.
//
// So the now-usual ordering -- gateway starts with an empty cache,
// immediately launches the Organizer (issue #93), and the user
// authenticates in the Organizer -- leaves anything started before that
// point with nothing, and anything started after it dependent on its
// parent having heard the broadcast.  What the user sees is being asked
// for the same password more than once in a session.
//
// The fix is to give the credential a home that does not depend on
// ordering: a small file in the user's own ECCE state directory, keyed by
// host and DISPLAY exactly as the JMSDispatcher port file is (see
// DatagramUtil::loadServerPort), created mode 0600, and deleted by
// ecce-gateway-stop / ecce-gateway-reap when the session's services go
// away.  Every process loads it when its AuthCache is constructed and
// rewrites it when it learns a credential, so sharing works in both
// directions and regardless of who started first.
//
// Security properties, stated plainly:
//
//   * The file holds the data server password in plaintext.  It is
//     created 0600 in $HOME/.ECCE -- the user's own directory on the
//     user's own machine.  Anyone able to read it can already read
//     everything else ECCE stores for that user and could equally attach
//     to any of these processes.  Same trust boundary as ~/.netrc,
//     ~/.pgpass or git's credential store.
//   * It is not a new exposure of the plaintext: the "-pipe" FIFO
//     already carries the same password in the clear through
//     $ECCE_TMPDIR, as does the JMS broadcast.
//   * It is keyed by $DISPLAY, so a second seat, a VNC session or an
//     X-forwarded session does not silently inherit another session's
//     credentials -- the same per-DISPLAY keying the JMSDispatcher
//     pidfile turned out to need.
//   * It does not outlive the session: ecce-gateway-stop and
//     ecce-gateway-reap remove it along with the port file.
//   * Only http/https credentials are stored.  Compute-machine
//     passwords (ssh/scp, which also pass through this cache) stay
//     memory-only; they are credentials for *other* hosts and
//     persisting them is not what this buys anything for.
//   * Nothing is written to a command line or to a log.  The password
//     never appears in ps; only ever the file's name.

/**
 * Path of the session credential store, or "" when this process has no
 * session to speak of (no ECCE_REALUSERHOME, HOST or DISPLAY in the
 * environment -- a batch-side tool, say).  Checks the environment
 * directly rather than calling Ecce::realUserPrefPath(), which is fatal
 * when ECCE_REALUSERHOME is unset and this runs from a constructor.
 */
string AuthCache::sessionFile()
{
  const char *home = getenv("ECCE_REALUSERHOME");
  const char *host = getenv("HOST");
  const char *display = getenv("DISPLAY");

  if (home == (const char*)0 || *home == '\0' ||
      host == (const char*)0 || *host == '\0' ||
      display == (const char*)0 || *display == '\0') {
    return "";
  }

  string key = string(host) + "_" + display;
  // DISPLAY is normally ":1" or "host:1.0", but nothing stops it holding
  // a '/', which would turn this into a path.  Flatten anything that is
  // not plainly filename material.
  for (int idx = 0; idx < (int)key.length(); idx++) {
    char ch = key[idx];
    if (!isalnum((unsigned char)ch) && ch != '.' && ch != '-' && ch != '_') {
      key[idx] = '_';
    }
  }

  return string(home) + "/.ECCE/authcache_" + key;
}


/**
 * Only data server (http/https) credentials belong in the session store.
 * The cache is keyed by makeKey() results, which begin with the protocol.
 */
bool AuthCache::sessionWorthy(const string& url)
{
  return url.compare(0, 7, "http://") == 0 ||
         url.compare(0, 8, "https://") == 0;
}


/**
 * Read the session store into the in-memory cache.  Entries already in
 * memory win: this only ever seeds, it never overwrites something this
 * process learned first hand.
 */
void AuthCache::sessionLoad()
{
  string path = sessionFile();
  if (path == "") return;

  FILE *fp = fopen(path.c_str(), "r");
  if (fp == NULL) return;

  char buf[1024];
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    string line = buf;
    while (line.length() > 0 &&
           (line[line.length()-1] == '\n' || line[line.length()-1] == '\r')) {
      line.erase(line.length()-1);
    }
    if (line == "") continue;

    // url|user|pass.  The password may itself contain '|', so split on
    // the first two separators only and keep the remainder verbatim --
    // note this is deliberately not StringTokenizer, which would drop
    // everything after a '|' in the password.
    string::size_type first = line.find('|');
    if (first == string::npos) continue;
    string::size_type second = line.find('|', first+1);
    if (second == string::npos) continue;

    string url = line.substr(0, first);
    string user = line.substr(first+1, second-first-1);
    string pass = line.substr(second+1);

    if (!sessionWorthy(url)) continue;
    if (exists(url, user)) continue;

    addIt(url, user, pass);
  }
  fclose(fp);
}


/**
 * Write the http/https entries of the in-memory cache to the session
 * store.  Goes to a temporary file created 0600 and renamed into place,
 * so a concurrently starting process never reads a half-written file and
 * the real file is never momentarily world readable.
 */
void AuthCache::sessionSave() const
{
  string path = sessionFile();
  if (path == "") return;

  // Per-pid temporary name: several ECCE processes can learn the same
  // credential at once (the JMS broadcast reaches all of them), and a
  // shared temporary name would let one unlink the file another was
  // still writing.
  char suffix[32];
  sprintf(suffix, ".tmp.%d", (int)getpid());
  string tmpPath = path + suffix;
  // O_EXCL after an explicit unlink: refuse to follow anything left
  // behind at that name rather than writing a password through it.
  unlink(tmpPath.c_str());
  int fd = open(tmpPath.c_str(), O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
  if (fd == -1) return;
  // open()'s mode argument is masked by umask; make certain of 0600.
  if (fchmod(fd, S_IRUSR|S_IWUSR) != 0) {
    close(fd);
    unlink(tmpPath.c_str());
    return;
  }

  bool ok = true;
  for (int idx = 0; ok && idx < (int)p_memcache.size(); idx++) {
    AuthTuple *cur = p_memcache[idx];
    if (!sessionWorthy(cur->url)) continue;
    // A newline anywhere would corrupt the file.  Such a credential
    // cannot round trip through the FIFO format either, so skip it
    // rather than writing something that reads back wrong.
    if (cur->url.find('\n') != string::npos ||
        cur->user.find('\n') != string::npos ||
        cur->pass.find('\n') != string::npos) continue;

    string line = cur->url + "|" + cur->user + "|" + cur->pass + "\n";
    if (write(fd, line.c_str(), line.length()) != (ssize_t)line.length()) {
      ok = false;
    }
  }
  close(fd);

  if (!ok || rename(tmpPath.c_str(), path.c_str()) != 0) {
    unlink(tmpPath.c_str());
  }
}


/**
 * Remove the session store.  The gateway shutdown scripts do this too;
 * this exists so the session boundary can also be enforced from inside
 * the app (Quit and Stop Server).
 */
void AuthCache::sessionClear()
{
  string path = sessionFile();
  if (path != "") {
    unlink(path.c_str());
  }
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
    // A plain O_WRONLY open() blocks until a reader opens the pipe too --
    // if the child app never starts properly, the caller (e.g. gateway)
    // hangs forever. O_NONBLOCK makes open() fail immediately with ENXIO
    // instead of blocking when there's no reader yet, so poll for up to
    // 10 seconds (matching pipeIn()'s own existing timeout) rather than
    // waiting indefinitely.
    int fd = -1;
    for (int it = 0; it < 10 && fd == -1; it++) {
      fd = open(pipeName.c_str(), O_WRONLY|O_NONBLOCK);
      if (fd == -1) sleep(1);
    }
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
          // The session store may already have supplied this url/user
          // when the cache was constructed. A duplicate is not merely
          // untidy: getAuthentication() walks the cache by retryCount,
          // so a second copy of the same credential burns one of the
          // three attempts WxDavAuth allows before it gives up. The
          // parent handing us this pipe is live, so where the two
          // disagree its value wins.
          int at = -1;
          for (int seen = 0; at < 0 && seen < (int)p_memcache.size(); seen++) {
            if (p_memcache[seen]->url == tuple->url &&
                p_memcache[seen]->user == tuple->user) {
              at = seen;
            }
          }
          if (at >= 0) {
            p_memcache[at]->pass = tuple->pass;
            delete tuple;
          } else {
            p_memcache.push_back(tuple);
          }
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

  // The stored copy would otherwise still hand every later process the
  // password the user just replaced.
  sessionSave();
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
                  // erase(it) invalidates it, so it must be reseated from
                  // erase's return value rather than left to be reused by
                  // the loop's condition check.
                  it = p_memcache.erase(it);
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
                  // erase(it) invalidates it, so it must be reseated from
                  // erase's return value rather than left to be reused by
                  // the loop's condition check.
                  it = p_memcache.erase(it);
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
               // erase(it) invalidates it, so it must be reseated from
               // erase's return value rather than left to be reused by
               // the loop's condition check.
               it = p_memcache.erase(it);
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
               // erase(it) invalidates it, so it must be reseated from
               // erase's return value rather than left to be reused by
               // the loop's condition check.
               it = p_memcache.erase(it);
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

   // Persist for the rest of the session so that processes which start
   // later -- or which were already running and never heard the
   // broadcast below -- do not have to ask the user again.  Done on any
   // real change, not only on publish, so a process that learned the
   // credential over JMS also leaves the store correct if it happens to
   // be the first one to see it.
   if (ret && sessionWorthy(key)) {
      sessionSave();
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


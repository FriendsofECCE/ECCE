#ifndef AUTHCACHE_H
#define AUTHCACHE_H

#include <string>
#include <vector>
  using std::string;
  using std::vector;

#include "util/JMSMessage.H"

class AuthTuple;


/** 
 * Structure for holding username and password.  Useful because
 * when prompting, the user name can be changed so both may need
 * to be returned.
 */
struct BasicAuth 
{
  string m_user;
  string m_pass;
};


/**
  This class provides caching of the tuples of url, user, and password.
  */
class AuthCache 
{
  public:
    /** 
     * BEST_USER - match url but any user. Useful because $USER and dav user
     * don't necessarily match up.
     * EXACT_USER - must match exactly.
     */
    enum UserPolicy { EXACT_USER, ANY_USER};

    /**
     * NOTE BEST_URL implies EXACT_USER.
     * LAST_URL - always use last entry in cache
     * BEST_URL - match url minum realm
     */
    enum URLPolicy { EXACT_URL, BEST_URL, LAST_URL};

    static AuthCache& getCache();

    void setURLPolicy(AuthCache::URLPolicy policy);

    void flushCache();

        // Update matching passwords when data server password changes
    void changePass(const string& urlbase, const string& user,
                    const string& newpass);

    static string pipeName();
    void pipeOut(const string& pipeName);
    void pipeIn(const string& pipeName);
    void msgIn(const JMSMessage& msg, const string& callerID);
    static void pipeClean(const string& pipeName);

    virtual BasicAuth *getAuthentication(const string& url, 
                                         const string& user,
                                         const string& realm="",
                                         int retryCount=1);

    virtual BasicAuth *getBogusAuthentication();

    virtual bool addAuthentication(const string& url, const string& user,
                                   const string& pass, const string& realm,
                                   const bool& publish);

    virtual bool remove(const string& url, const string& user, 
                        const string& realm="");

    static BasicAuth *createBasicAuth(const string& user, const string& password);

    virtual int size() const;


  protected:
    AuthCache();
    virtual ~AuthCache();
    AuthCache(const AuthCache& rhs);
    string find(const string& url, const string& user,int retryCount);
    bool exists(const string& url, const string& user) const;
    int  index(const string& url, const string& user) const;
    string findBest(const string& url, const string& user, int retryCount);
    string findAny(const string& url, string& user, int retryCount);
    string forMachine(const string& machine, const string& user,
                          int retryCount);

    string makeKey(const string& url, const string& realm) const;

    void addIt(const string& url,const string& user, const string& pass);

    static AuthCache  *p_singleton;
    UserPolicy         p_userPolicy;
    URLPolicy          p_URLPolicy;
    vector<AuthTuple*> p_memcache;
};
#endif

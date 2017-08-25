/**
 * @file
 *
 */
#ifndef _EDSIFACTORY_H
#define _EDSIFACTORY_H


#include <string>
#include <vector>
  using std::string;
  using std::vector;


#include "dsm/EDSI.H"

#include "util/InvalidException.H"
#include "util/JMSMessage.H"


class AuthEventListener;
class ICalculation;
class IPropCache;
class ITraversable;
class JMSPublisher;
class ProgressEventListener;
class PropertyTask;
class Resource;
class ResourceType;
class Session;
class VDoc;


/**
 * Manages and maintains all Resource objects.  This EDSIFactory owns all the
 * Resource objects so callers should not delete any pointers that they are
 * returned through these functions.  The data structure concept implemented 
 * here is a pool.
 */
class EDSIFactory {

  public: 

    friend class Resource;

    ~EDSIFactory();

    static Resource *getResource(const EcceURL& url);

    //////////////////////
    // pool maintainence
    //////////////////////
    static void createPoolResource(const EcceURL& url);
    static void removePoolResource(const EcceURL& url);
    static void renamePoolResource(const EcceURL& oldUrl,const EcceURL& newUrl);
    static void changePoolResource(const EcceURL& url);

    ///////////////////////////////
    // specific interface getters
    ///////////////////////////////
    static EDSI* getEDSI(const EcceURL& url);
    static VDoc* getVdoc(const EcceURL& url);
    static IPropCache* getIPropCache(const EcceURL& url, 
                                     const bool storage = true, 
                                     const int maxStorageFailure = 10);

    static void addAuthEventListener(AuthEventListener *l);
    static void removeAuthEventListener(AuthEventListener *l);

    static void addProgressEventListener(ProgressEventListener *l);
    static void removeProgressEventListener(ProgressEventListener *l);

    //////////////
    // messaging
    //////////////
    static JMSPublisher * getPublisher(const string &app_name = "EDSIFactory");
    static void notifyCreate(const string & url);
    static void notifyRename(const string & oldurl, const string & newurl);
    static void notifyRemove(const string & url);
    static void notifyProperty(const string & url, const string & name, 
                               const string & value);
    static void notifyState(const string & url, const string & state);

    ///////////////
    // Deprecated
    ///////////////
    static PropertyTask* getPropertyTask(const EcceURL& url);
    static ITraversable* getITraversable(const EcceURL& url);

  protected:

    typedef map<string, Resource *> PoolMap;
    typedef map<string, Resource *>::iterator PoolIter;

    static AuthEventListener     *p_aeListener;
    static ProgressEventListener *p_progressListener;
    static JMSPublisher          *p_publisher;
    static PoolMap               p_pool;

  private:

    //////////////////////////
    // resource getter steps
    //////////////////////////
    static Resource *getResource(const EcceURL& url, 
                                 ResourceMetaDataResult result,
                                 bool skipPoolCheck = false);
    static Resource *getResource(const EcceURL& url, 
                                 ResourceType * resourceType,
                                 bool skipPoolCheck = false);

    ////////////////////////////////
    // resource creation functions
    ////////////////////////////////
    static EcceURL* createResource(EDSI* interface, 
                                   const string& name, 
                                   ResourceType* resourceType) 
                                   throw(InvalidException);

    static EcceURL* createCalculation(EDSI* interface, 
                                  const string& name,
                                  string appType);

    static EcceURL* createProject(EDSI* interface, const string& name);

    static EcceURL* createSession(EDSI* interface, const string& name, 
                                  ResourceDescriptor::APPLICATIONTYPE appType = 
                                  ResourceDescriptor::AT_MDSTUDY);

    static EcceURL* createTask(EDSI* interface, const string& name,
                               ResourceDescriptor::CONTENTTYPE taskType, 
                               ResourceDescriptor::APPLICATIONTYPE appType,
                               EcceURL *sessionId);

    static VDoc* createVdoc(const EcceURL& url);

    static void refreshResourceProperties(const EcceURL& url);

    ///////////////////////////////
    // Session model manipulation
    ///////////////////////////////
    static void addSessionTask(const EcceURL& taskId, const EcceURL& sessionId,
                              ResourceDescriptor::ResourceCategory sessionType);

    static void addSessionReactionTask(const EcceURL& taskId, 
                                       const EcceURL& sessionId);

    static void addSessionCondensedReactionTask(const EcceURL& taskId, 
                                                const EcceURL& sessionId);

    static void dumpPool();
};

#endif

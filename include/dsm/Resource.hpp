/**
* @file
*
*
*/
#ifndef RESOURCE_HH
#define RESOURCE_HH


#include <string>
  using std::string;
#include <vector>
  using std::vector;
#include <iostream>
  using std::istream;


#include "util/EcceURL.H"
#include "util/RetryException.H"
#include "util/CancelException.H"
#include "util/InvalidException.H"
#include "util/DavException.H"

#include "dsm/PropertyView.H"
#include "dsm/ResourceProperty.H"

class EcceURL;
class SFile;
class EDSI;
class EDSIMessage;
class ResourceType;
class Session;
class TDateTime;
class MetaDataResult;

/**
 * Represents a specific Ecce resource.  Default file operation
 * implementations are provided as well as access to the resource's
 * default set of metadata.  Resource registry information 
 * (i.e. ResourceDescriptor.xml) pertaining to the type of resource 
 * being represented is also provided via the getDescriptor() method.
 */
class Resource {

  public:

    friend class EDSIFactory;

    Resource(const Resource& rhs);
    Resource& operator=(const Resource& rhs);
    bool operator==(const Resource& rhs) const;
    bool operator!=(const Resource& rhs) const;
    virtual ~Resource();

    virtual EcceURL getURL() const;
    virtual bool isValid() throw (RetryException, CancelException);
    virtual bool copy(EcceURL& target);
    virtual bool move(EcceURL& target);

    virtual ResourceDescriptor::RESOURCETYPE getResourceType() const;
    virtual ResourceDescriptor::CONTENTTYPE getContentType() const;
    virtual ResourceDescriptor::APPLICATIONTYPE getApplicationType() const;

    virtual ResourceType *getDescriptor() const;
    virtual void setDescriptor(ResourceType *resourceType);

    virtual string getProp(const string &key,
                           bool searchInputProviders=false,
                           string linkName = "") const;
    virtual EcceMap *getProps(bool searchInputProviders=false,
                              string linkName = "") const;
    virtual bool addProp(const string &key, const string &value);
    virtual bool addProps(const vector<MetaDataResult> &results);

    virtual string getName() const;
    virtual TDateTime * getCreationDate() const;
    virtual TDateTime * getModifiedDate() const;
    virtual int getSize() const;

    virtual Resource *createChild(string name, ResourceType *childType);
    virtual Resource *createChild(string name, 
                                  ResourceDescriptor::RESOURCETYPE rt, 
                                  ResourceDescriptor::CONTENTTYPE ct, 
                                  ResourceDescriptor::APPLICATIONTYPE at);
    virtual Resource *createChild(string name, SFile *file);

    virtual vector<Resource*> * getChildren(bool refresh = false);
    virtual vector<Resource*> * getCachedChildren();
    virtual bool remove();

    virtual SFile *getDocument(const SFile *dest);
    virtual istream *getDocument();

    virtual string messages(void) const;

    vector<EcceURL> descendantSearch(const string& key, const string& substr);

    virtual EcceURL *getSessionId() const;
    virtual Session *getSession() const;

    virtual bool hasAccess();

    virtual ResourceTool *getDefaultTool();

    virtual Resource *getInputProvider(Session * session,
                                       const string& linkName="") const;
    virtual ResourceDescriptor::CONTENTTYPE getInputProviderType(const string& linkName="");

    virtual bool canModifyInputProvider();

    virtual int getSessionIndex() const;

    static bool isHideInternal();

    static void setHideInternal(bool hideInternal);

    virtual bool setReviewed(bool flag);

    virtual void getDeleteMessage(bool & canDelete, string & message);

    virtual void notifyProperty(const string & name, const string & value);

    virtual void notifyState(const string & state);

    virtual void clearChildren();

  protected:

    enum ACCESS {
      UNKNOWN,
      FAILED,
      SUCCEEDED
    };

    Resource();
    Resource(const EcceURL& resourceId) throw(InvalidException);

    EDSI *getEDSI() const throw(InvalidException);

    virtual bool setURL(const EcceURL& url);
    virtual Resource * addChild(const EcceURL & childUrl);
    virtual void eraseChild(const EcceURL & childUrl);

    virtual void cacheProps(const vector<MetaDataResult> &results);
    virtual void updateProps();
    virtual void clearProps();

    string getMimeType(const EcceURL& resource) const;

    EcceURL p_url;                 /**< Resource address */
    EDSI *p_edsi;                  /**< Dav server interface */
    EcceMap *p_properties;         /**< Resource metadata */
    ResourceType *p_descriptor;    /**< Registry information for resource */
    vector<Resource*> *p_children; /**< List of resources children, if any */
    EDSIMessage *p_msgStack;       /**< Holds Dav server messages */

    ACCESS p_access;
  
    bool p_localHideInternal;
    static bool p_hideInternal;
};


#endif

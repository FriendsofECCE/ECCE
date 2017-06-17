/**
 * @file
 *
 *
 */

#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

#include <string>
  using std::string;
#include <vector>
  using std::vector;
#include <fstream>
  using std::ofstream;
  using std::ifstream;
#include <iostream>
  using std::istream;


#include "util/EcceURL.H"
#include "util/EcceMap.H"
#include "util/TypedFile.H"
#include "util/SFile.H"
#include "util/TempStorage.H"
#include "util/ResourceUtils.H"
#include "util/TDateTime.H"
#include "util/StringConverter.H"
#include "util/JMSMessage.H"
#include "util/JMSPublisher.H"
#include "util/MismatchedTypeException.H"

#include "dsm/CodeFactory.H"
#include "dsm/JCode.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIExporter.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"
#include "dsm/TaskJob.H"
#include "dsm/VDoc.H"
#include "dsm/ResourceProperty.H"


bool Resource::p_hideInternal = true;

/**
 * Constructor
*/
Resource::Resource()
{
  p_edsi = 0;
  p_msgStack = new EDSIMessage();
  p_properties = new EcceMap();
  p_descriptor = 0;
  p_children = 0;
  p_access = UNKNOWN;
  p_localHideInternal = false;
}



/**
 * Constructor
 *
 * @param resourceId
 * @throw InvalidException
 */
Resource::Resource(const EcceURL& resourceId) throw(InvalidException)
{
  p_url = resourceId;
  p_edsi = 0;
  p_properties = new EcceMap();
  p_descriptor = 0;
  p_children = 0;
  p_msgStack = new EDSIMessage;
  p_access = UNKNOWN;
  p_localHideInternal = false;
}



/**
 * Copy constructor
 *
 * @param rhs
 */
Resource::Resource(const Resource& rhs)
{
  p_url = rhs.getURL();

  p_edsi = 0;

  p_properties = new EcceMap();
  *p_properties = *(rhs.p_properties);

  p_descriptor = rhs.p_descriptor;

  p_children = 0; // new vector<Resource*>;

  p_msgStack = new EDSIMessage;

  p_access = UNKNOWN;

  p_localHideInternal = rhs.p_localHideInternal;
}



/**
 * Copy Assignment
 *
 * @param rhs
 * @return
 */
Resource& Resource::operator=(const Resource& rhs)
{
  if (this != &rhs) {

    p_url = rhs.getURL();

    if (p_properties != 0) {
      clearProps();
    }
    *p_properties = *(rhs.p_properties);

    p_descriptor = rhs.p_descriptor;

    clearChildren();
    p_access = UNKNOWN;
  }
  return *this;
}



/**
 * Equality
 *
 * @param rhs
 * @return
 */
bool Resource::operator==(const Resource& rhs) const
{
  return (this->getURL() == rhs.getURL());
}



/**
 * Inequality
 * @param rhs
 * @return
 */
bool Resource::operator!=(const Resource& rhs) const
{
  return (this->getURL() != rhs.getURL());
}



/**
 * Destructor
 */
Resource::~Resource()
{
  //  cerr << "Delete Resource:\n\t" << p_url.toString() << endl;
  if (p_edsi != 0) {
    delete p_edsi;
    p_edsi = 0;
  }
  if (p_msgStack != 0) {
    delete p_msgStack;
    p_msgStack = 0;
  }
  if (p_properties != 0) {
    p_properties->clear();
    delete p_properties;
    p_properties = 0;
  }
  clearChildren();
}



/**
 * Gets the base url for this resource.
 *
 * @return Base url for task
 */
EcceURL Resource::getURL() const
{
  return p_url;
}



/**
 * Sets the base url for this resource.
 *
 * @param url New base Url for resource.
 */
bool Resource::setURL(const EcceURL& url)
{
  p_url = url;
  return true;
}



/**
 * Checks to make sure that the Resource exists.
 *
 * @throw RetryException Indicates maximum number of retries
 *        to verify existence of task were exceeded.
 * @throw CancelException Indicates the existence check was cancelled.
 * 
 * @return Returns true if the task was found, otherwise
 *         false is returned.
 */
bool Resource::isValid() throw (RetryException, CancelException)
{
  // Make sure tho set to current resource url before executing
  getEDSI()->setURL(getURL());

  bool ret = getEDSI()->exists();

  if (getEDSI()->m_msgStack.findKey("CANCELED"))
    throw CancelException(WHERE);
  else if (getEDSI()->m_msgStack.findKey("TOO_MANY_RETRIES"))
    throw RetryException("Maximum number of retries exceeded.",WHERE);

  return ret;
}



/**
 * Removes a resource and all subdirectories, if any, from the
 * Dav server.
 * Since the pool is introduced and this object will be deleted after
 * calling EDSIFactory::removeResource(p_url);
 * Not sure if there is any better way to slove the problem since it is
 * risky and resource object become invalid after calling remove
 *
 * @return Returns true if resource was removed, otherwise false.
 */
bool Resource::remove(void)
{
  bool ret = true;
  p_msgStack->clear();

  getEDSI()->setURL(p_url);
  if (!getEDSI()->removeResource()) {
    p_msgStack->add(getEDSI()->m_msgStack);
    ret =  false;
  }

  EDSIFactory::removePoolResource(p_url);

  return ret;
}



/**
 * Copies a resource to a different location on the Dav server or
 * another DAV server or from local file system to Dav server.
 *
 * @param target New location of resource.
 *
 * @return Returns true if resource was copied, otherwise fals.
 */
bool Resource::copy(EcceURL& target)
{
  EcceURL originalUrl = getEDSI()->getURL();

  bool ret = false;
  p_msgStack->clear();

  getEDSI()->setURL(p_url);
  if (getEDSI()->exists()) {

    // Same dav copy
    if (EcceURL::isSameDomain(target, getURL())) {
      ret = getEDSI()->copyResource(target, EDSI::SORTOF);
      if (!ret) {
        p_msgStack->add(getEDSI()->m_msgStack);
      }
    }
    // File sys to dav copy
    else if (p_url.isLocal()) {
      EDSI *edsi = EDSIFactory::getEDSI(target);
      // check to see if this resource exists on the server already
      if (edsi->exists()) {
        // since a file with that name already exists, we'll change the name of
        // the file being uploaded for the user to something uniqe
        string filename = target.getFilePathTail();
        target = target.getParent();
        edsi->setURL(target);
        string newName = edsi->uniqueName(filename);
        target = target.getChild(newName);
        edsi->setURL(target);
      }

      ifstream is(p_url.getPath().c_str());
      ret = edsi->putDataSet(is);
      is.close();
    }
    // Dav to dav copy
    else {
      EDSIExporter exporter;
      exporter.setSourceURL(getURL());
      exporter.setTargetURL(target.getParent());
      try {
        // Server to Server copy
        EcceURL result = exporter.xport();
        ret = true;
      } 
      catch (IOException& ex) {
        p_msgStack->add(ex.what());
      }
    }
  }
  else {
    p_msgStack->add("UNABLE_TO_COPY",
                    "Resource to move no longer exists");
  }

  getEDSI()->setURL(originalUrl);
  return ret;
}



/**
 * Moves a resource to a different location on the Dav server.
 *
 * @param target New location of resource.
 *
 * @return Returns true if resource was moved, otherwise fals.
 */
bool Resource::move(EcceURL& target)
{
  EcceURL originalUrl = getEDSI()->getURL();

  bool ret = false;
  p_msgStack->clear();

  getEDSI()->setURL(p_url);
  if (getEDSI()->exists()) {

    if (EcceURL::isSameDomain(target, getURL())) {
      // Local Move
      ret = getEDSI()->moveResource(target);
      if (!ret) {
        p_msgStack->add(getEDSI()->m_msgStack);
      }
    }
    // File sys to dav move always considered as copy
    else if (p_url.isLocal()) {
      EDSI *edsi = EDSIFactory::getEDSI(target);
      // check to see if this resource exists on the server already
      if (edsi->exists()) {
        // since a file with that name already exists, we'll change the name of
        // the file being uploaded for the user to something uniqe
        string filename = target.getFilePathTail();
        target = target.getParent();
        edsi->setURL(target);
        string newName = edsi->uniqueName(filename);
        target = target.getChild(newName);
        edsi->setURL(target);
      }

      ifstream is(p_url.getPath().c_str());
      ret = edsi->putDataSet(is);
      is.close();
    }
    else {
      EDSIExporter exporter;
      exporter.setSourceURL(getURL());
      exporter.setTargetURL(target.getParent());
      try {
        // Server to Server copy
        EcceURL result = exporter.xport();
        // getEDSI()->removeResource();
        ret = true;
      }
      catch (IOException& ex) {
        p_msgStack->add(ex.what());
      }
    }
  }
  else {
    p_msgStack->add("UNABLE_TO_MOVE",
                    "Resource to move no longer exists");
  }

  if (ret) {
    if (!p_url.isLocal()) {
      EDSIFactory::renamePoolResource(p_url, target);
      setURL(target);
      clearChildren();
    }
  }

  getEDSI()->setURL(originalUrl);
  return ret;
}



/**
 * Returns the children resources of a resource, if any.  This is
 * the default implementation for a resource.  Other resource
 * types (ie session, mdtask, etc) may have their own.  User is not 
 * responsible for memory management. If failed authentication return NULL.
 * Should use the return value not NULL as a flag to avoid prompting user
 * too many times for password.
 *
 * @return Returns a vector of Resource pointers - the child resources.
 */
vector<Resource*> * Resource::getChildren(bool refresh) 
{
  EcceURL originalUrl = getEDSI()->getURL();

  int idx;

  if ((p_children == 0) || (refresh == true)) {

    // Build property list query
    vector<string> props;
    getEDSI()->describeServerMetaData(props);
    vector<MetaDataRequest> vmdr;
    MetaDataRequest mdr;
    int numProps = props.size();
    for (idx=0; idx<numProps; idx++) {
      mdr.name = props[idx];
      vmdr.push_back(mdr);
    }

    // Get the children and their properties
    EcceURL url = getURL();
    getEDSI()->setURL(url);
    vector<ResourceMetaDataResult> children;
    if (getEDSI()->listCollection(vmdr,children)) {
      clearChildren();
      p_children = new vector<Resource*>;

      int cnt = children.size();
      Resource *resource = 0;
      string name = "";
      for (idx=0; idx<cnt; idx++) {
        name = children[idx].url.getFilePathTail();

        if (name.find("htaccess") != string::npos) continue;

        // Create child resources
        resource = EDSIFactory::getResource(children[idx].url,
                                            children[idx]);  
        if (resource != 0) {
          if (dynamic_cast<TaskJob*>(resource)) {
            resource->updateProps();
          }
          else {
            resource->cacheProps(children[idx].metaData);
          }
          resource->clearChildren();
          p_children->push_back(resource);
        }
      }
      p_access = SUCCEEDED;
    } 
    else {
      // List Collection failed - go ahead and clear/delete children
      // for next attempt - if there is one
      clearChildren();
      p_access = FAILED;
    }
  }

  getEDSI()->setURL(originalUrl);
  return p_children;
}



/**
 * Add a single child Resource to current Resource's children vector.
 *
 * Note: This function won't modify data on server.
 *
 * Return the Resource object.
 * If it already exists, return the copy in children vector.
 * If not, call getResource to generate one and insert it into children vector.
 * If the children vector is not initialized or child url doesn't match
 * the parent one, return 0.
 */
Resource * Resource::addChild(const EcceURL & childUrl) 
{
  Resource * ret;
  if (p_children == 0)
    // Shouldn't be calling this function if children were never loaded.
    ret = 0;
  else {
    if (childUrl.getParent() == getURL()) {
      vector<Resource *>::iterator childItor = p_children->begin();
      for (; childItor != p_children->end(); childItor++) {
        if (childUrl == (*childItor)->getURL()) {
          ret = (*childItor);
          break;
        }
      }
      if (childItor == p_children->end()) {
        ret = EDSIFactory::getResource(childUrl);
        p_children->push_back(ret);
      }
    }
    else
      // child URL is not contained in parent URL
      ret = 0;
  }
  return ret;
}



/**
 * Removes a single child Resource pointer from the vector of children.
 *
 * @param childUrl the url of the child to remove
 */
void Resource::eraseChild(const EcceURL & childUrl)
{
  if (p_children != 0) {
    vector<Resource *>::iterator childItor = p_children->begin();
    for (; childItor != p_children->end(); childItor++) {
      if (childUrl == (*childItor)->getURL()) {
        p_children->erase(childItor);
        break;
      }
    }
  }
}



/**
 * Remove a single child Resource from DAV, the resource pool, and the current 
 * Resource's children vector.
 *
 * @param childUrl the url of the child to remove
 * @return true if Resource removed from DAV
bool Resource::removeChild(const EcceURL & childUrl) 
{
  EcceURL originalUrl = getEDSI()->getURL();

  bool ret = true;

  // child URL should be contained in parent URL
  if (childUrl.getParent() == getURL()) {

    // remove from DAV
    getEDSI()->setURL(childUrl);
    
    if (getEDSI()->removeResource()) {

      cerr << "Before remove from pool\n";
      // remove from pool and children vector

      cerr << "After remove from pool\n";
    }
    else {
      cerr << "\t" << getEDSI()->m_msgStack.getMessage() << "\n";
      ret = false;
    }
  }
  else {
    ret = false;
  }

  getEDSI()->setURL(originalUrl);
  return ret;
}
 */



/**
 * Returns the resource type of Resource based on its registry information.
 *
 * @return resource type based on registry information
 */
ResourceDescriptor::RESOURCETYPE Resource::getResourceType() const
{
  ResourceDescriptor::RESOURCETYPE ret = 
    ResourceDescriptor::RT_UNDEFINED;

  if (p_descriptor != 0) {
    ret = ResourceUtils::stringToResourceType(p_descriptor->getResourceType());
  }
  return ret;
}



/**
 * Returns the content type of Resource based on its registry information.
 *
 * @return content type of ResourceType based on registry information
 */
ResourceDescriptor::CONTENTTYPE Resource::getContentType() const
{
  ResourceDescriptor::CONTENTTYPE ret = 
    ResourceDescriptor::CT_UNDEFINED;

  if (p_descriptor != 0) {
    ret = ResourceUtils::stringToContentType(p_descriptor->getContentType());
  }
  return ret;
}



/**
 * Returns the application type of Resource based on its registry information.
 *
 * @return application type based on registry information
 */
ResourceDescriptor::APPLICATIONTYPE Resource::getApplicationType() const
{
  ResourceDescriptor::APPLICATIONTYPE ret = 
    ResourceDescriptor::AT_UNDEFINED;

  if (p_descriptor != 0) {
    ret = ResourceUtils::stringToApplicationType(p_descriptor->getApplicationType());
  }
  return ret;
}



/**
 * Get ResourceType.
 *
 * @return ResourceType based on registry information
 */
ResourceType *Resource::getDescriptor() const
{
  return p_descriptor;
}



/**
 * Set ResourceType.
 *
 * @param resourceType based on registry information
 */
void Resource::setDescriptor(ResourceType *resourceType)
{
  p_descriptor = resourceType;
}



/**
 * Retrieves metadata for a resource, will search input provider
 * resources for the metadata if requested.  The search for
 * metadata starts with the current resource, and if not found,
 * traverses back along INPUT_PROVIDER relationships until metadata
 * is found.  The search is terminated when the metadata is found
 * or if no further input provider resources are found (ie root resource).
 *
 * @param key name of the metadata property
 * @param searchInputProviders a boolean indicating whether or not
 *        to search INPUT_PROVIDER tasks for the requested
 *        metadata.  If true, the input provider tasks will be
 *        searched for the metadata.  False indicates to only
 *        search current resource.  Default value is false.
 * @param linkName name of link.  The link type, which is INPUT_PROVIDER
 *        in this case, along with the link name uniquely
 *        identifies a relationship between resources.  Default
 *        value is an empty string.
 *
 * @return returns metadata value, empty string is returned
 *         if metadata value was not found.
 */
string Resource::getProp(const string &key,
                         bool searchInputProviders,
                         string linkName) const
{
  string ret = "";

  // Load properties cache if empty
  if (p_properties->empty()) {
    const_cast<Resource*>(this)->updateProps();
  }

  // Try to find property on current resource
  p_properties->findValue(key.c_str(), ret);

  // Continue search if property hasn't been found
  // and search input providers has been requested
  // expensive for a single property
  if (ret.empty() && searchInputProviders) {
    EcceMap *props = 0;
    props = getProps(true, linkName);
    if (props) {
      props->findValue(key.c_str(), ret);
      delete props;
    }
  }

  return ret;
}



/**
 * Retrieves metadata for a resource, will search input provider
 * resources for the metadata if requested.  The search for
 * metadata starts with the current resource, and if not found,
 * traverses back along INPUT_PROVIDER relationships until metadata
 * is found.  The search is terminated when the metadata is found
 * or if no further input provider resources are found (ie root resource).
 * Caller is responsible for memory management.
 *
 * @param searchInputProviders a boolean indicating whether or not
 *        to search INPUT_PROVIDER tasks for the requested
 *        metadata.  If true, the input provider tasks will be
 *        searched for the metadata.  False indicates to only
 *        search current resource.  Default value is false.
 * @param linkName name of link.  The link type, which is INPUT_PROVIDER
 *        in this case, along with the link name uniquely
 *        identifies a relationship between resources.  Default
 *        value is an empty string.
 *
 * @return returns metadata as an EcceMap, but a copy, caller must delete
 */
EcceMap *Resource::getProps(bool searchInputProviders, string linkName) const
{
  // load properties if cache is empty
  if (p_properties->empty()) {
    const_cast<Resource*>(this)->updateProps();
  }

  EcceMap *ret = new EcceMap(*p_properties); // copies resource metadata

  // Continue search if search input providers has been requested
  if (searchInputProviders) {

    Resource *inputProvider = 0;
    Session *session = getSession();
    if (session != 0) {
      inputProvider = getInputProvider(session, linkName);
      while (inputProvider != 0) {
        // Find properties on input provider
        EcceMap *inputProps = inputProvider->getProps();
        
        // Insert any missing properties
        map<string, string, less<string> >::iterator prop;
        for (prop = inputProps->begin(); prop != inputProps->end(); prop++) {
          string key = prop->first;
          if (! ret->contains(key)) {
            ret->insert(key, prop->second);
          }
        }
        
        // Get next input provider
        inputProvider = inputProvider->getInputProvider(session, linkName);
      }
    }
  }

  return ret;
}



/**
 * Adds a string key/value pair to the resource's metadata if the key does not
 * already exist, overwrites the value of key if key does already exist.
 * Saves metadata to server as well as local cache.
 *
 * @param key   the name of the property to store
 * @param value the value of the property to store
 * @return      true if metadata saved to server successfully, false otherwise
 */
bool Resource::addProp(const string &key, const string &value)
{
  vector<MetaDataResult> result(1);
  result[0].name = key;
  result[0].value = value;

  return addProps(result);
}



/**
 * Adds multiple key/value pairs to the resource's metadata.  If the key already
 * exists, the value associated with it is overwritten.  By default, the meta-
 * data is saved to the server as well as creating a local cache.
 *
 * @param results a list of name/value pairs
 * @return        true if metadata saved to server successfully, false otherwise
 *                (this includes when not syncing with server)
 */
bool Resource::addProps(const vector<MetaDataResult> &results)
{
  EcceURL originalUrl = getEDSI()->getURL();

  bool ret = true;

  getEDSI()->setURL(p_url);
  p_msgStack->clear();

  if (!getEDSI()->putMetaData(results)) {
    p_msgStack->add(getEDSI()->m_msgStack);
    ret = false;
  }

  if (ret) {
    cacheProps(results);
  }

  getEDSI()->setURL(originalUrl);
  return ret;
}



/**
 * Overwrites existing cached metadata properties or adds new ones to the
 * cache.  This function is used as opposed to addProp(s) which will perform
 * a JMS message publish in addition to local caching.  Should be used
 * internally only so that outside callers can't mess with the properties
 * cache.
 * @param results the name/value pairs to cache
 */
void Resource::cacheProps(const vector<MetaDataResult> & results)
{
  vector<MetaDataResult>::const_iterator itor;
  for (itor = results.begin(); itor != results.end(); itor++) {
    (*p_properties)[itor->name.c_str()] = itor->value.c_str();
  }
}



/**
 * Refreshes the metadata based on the values currently stored on the server.
 * Clears the locally cached metadata.  Doesn't get VDoc properties, only 
 * properties for a the resource (depth = 0).
 */
void Resource::updateProps()
{
  EcceURL originalUrl = getEDSI()->getURL();
  vector<MetaDataRequest> vmdr;
  ResourceMetaDataResult result;

  // Build property query
  vector<string> props;
  getEDSI()->describeServerMetaData(props);
  MetaDataRequest mdr;
  int numProps = props.size();
  for (int idx=0; idx<numProps; idx++) {
    mdr.name = props[idx];
    vmdr.push_back(mdr);
  }

  // Query properties
  getEDSI()->setURL(p_url);
  if (getEDSI()->getMetaData(vmdr,result, false)) {
    clearProps();
    cacheProps(result.metaData);
    p_access = SUCCEEDED;
  }
  else
    p_access = FAILED;

  getEDSI()->setURL(originalUrl);
}



/**
 * Just clears the prop vector.
 */
void Resource::clearProps()
{
  p_properties->clear();
}



/**
 * Utility function to get creation date of the Resource,
 * caller's responsibility to delete after usage.
 *
 * @return a new TDateTime object, based on metadata
 */
TDateTime * Resource::getCreationDate() const
{
  string dateStr = getProp(VDoc::getEcceNamespace() + ":creationdate");

  TDateTime * ret = NULL;
  if (dateStr != "")
    ret = new TDateTime(dateStr);
  return ret;
}



/**
 * Utility function to get creation date of the Resource,
 * caller's responsibility to delete after usage.
 * 
 * @return a new TDateTime object, based on metadata
 */
TDateTime * Resource::getModifiedDate() const
{
  string dateStr = getProp("DAV:getlastmodified");

  TDateTime * ret = NULL;
  if (dateStr != "")
    ret = new TDateTime(dateStr);
  return ret;
}



/**
 * Utility function to get size of the Resource,
 */
int Resource::getSize() const
{
  int size = 0;
  string sizeStr = getProp("DAV:getcontentlength");

  if (sizeStr != "")
    StringConverter::toInt(sizeStr, size);
  return size;
}



/**
 * Creates a child resource of this parent resource of the given type.
 * 
 * @param name        the name of the child
 * @param childType   the type of resource that this child will be
 *
 * @return            pointer to the newly created resource
 */
Resource *Resource::createChild(string name, ResourceType *childType)
{
  EcceURL originalUrl = getEDSI()->getURL();

  Resource *ret = 0;

  getEDSI()->setURL(p_url);

  EcceURL *childUrl = 0;
  try {
    childUrl =  EDSIFactory::createResource(getEDSI(), name, childType);
  }
  catch (InvalidException& ex) {
    //cout << "EDSIFactory::createResource failed :: " << ex.what() << endl;
    throw;
  }

  if (childUrl != 0) {
    ret = EDSIFactory::getResource(*childUrl);
    if (ret != 0) {
      if (p_children != 0) {
        p_children->push_back(ret);
      }
    }
    delete childUrl;
    childUrl = 0;
  }

  getEDSI()->setURL(originalUrl);
  return ret;
}



/**
 * Creates a child resource of this parent resource of the given type.
 * Combines types into ResourceType and calls the other createChild function.
 *
 * @param name  the name of the child
 * @param rt    resource type
 * @param ct    content type
 * @param at    application (code) type
 * @return      pointer to the newly created resource
 */
Resource *Resource::createChild(string name, 
                                ResourceDescriptor::RESOURCETYPE rt,
                                ResourceDescriptor::CONTENTTYPE ct,
                                ResourceDescriptor::APPLICATIONTYPE at)
{
  Resource *ret = 0;
  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
  ResourceType *resourceType = rd.getResourceType(rt, ct, at);
  if (resourceType != 0) {
    ret = createChild(name, resourceType);
  }
  return ret;
}



/**
 * Creates a child document of this parent resource.
 *
 * @param name  the name of the child
 * @param file  the file to copy
 * @return      pointer to the newly created resource
 */
Resource *Resource::createChild(string name, SFile * file) 
{
  Resource *ret = 0;

  // construct a url based on the parent url + name
  EcceURL url = getURL();
  url = url.getChild(name);

  // get the edsi and set it to our soon-to-copy url
  EDSI *edsi = EDSIFactory::getEDSI(url);

  // check to see if this resource exists on the server already
  if (edsi->exists()) {
    // since a file with that name already exists, we'll change the name of the
    // file being uploaded for hte user to something unique
    url = url.getParent();
    edsi->setURL(url);
    string newName = edsi->uniqueName(name);
    url = url.getChild(newName);
    edsi->setURL(url);
  }

  ifstream ifs(file->path().c_str());
  if (! edsi->putDataSet(ifs))
  {
    p_msgStack->add(edsi->m_msgStack);
  } else {
    ret = EDSIFactory::getResource(url);
  }
  ifs.close();

  return ret;
}



/**
 * Gets name of resource.  This is a convenience function as the name is just
 * the tail of the url for this resource, but this is more readable.
 *
 * @return returns name of resource.
 */
string Resource::getName() const
{
    return p_url.getFilePathTail();
}



/**
 * Returns the url contents as an SFile.
 *
 * @param dest  if null, a temp file is created, otherwise overwrites given file
 *
 * @return      SFile generated from data stream of url contents
 */
SFile *Resource::getDocument(const SFile *dest) 
{
  EcceURL originalUrl = getEDSI()->getURL();

  SFile *sfile = (SFile*)dest;
  if (sfile == 0) {
    sfile = TempStorage::getTempFile();
  }

  getEDSI()->setURL(p_url);
  ofstream ofs(sfile->path().c_str() );
  if (!getEDSI()->getDataSet(ofs)) {
    sfile = 0;
  }
  ofs.close();

  getEDSI()->setURL(originalUrl);
  return sfile;
}



/**
 * Returns the url contents as an istream.
 *
 * @return  istream generated from data stream of url contents
 */
istream *Resource::getDocument() 
{
  EcceURL originalUrl = getEDSI()->getURL();

  istream* ret = 0;
  getEDSI()->setURL(p_url);
  ret = getEDSI()->getDataSet();

  getEDSI()->setURL(originalUrl);
  return ret;
}



/** 
 * Enables user to access the message stack, which contains any 
 * Dav server error messages.
 * 
 * @return returns message stack messages.
 */
string Resource::messages(void) const
{
  return p_msgStack->getMessage();
}



/**
 * Given a metadata property (key), searches all descendants of this url
 * and returns a vector of EcceURLs if they contain substr.
 *
 * @return returns an empty EcceURL vector if no results are found
 */
vector<EcceURL> Resource::descendantSearch(const string& key, 
                                           const string& substr)
{
  EcceURL originalUrl = getEDSI()->getURL();

  getEDSI()->setURL(p_url);
  p_msgStack->clear();

  vector<EcceURL> ret;
  if (! getEDSI()->efind(key, substr, p_url, ret)) {
    p_msgStack->add(getEDSI()->m_msgStack);
  }

  getEDSI()->setURL(originalUrl);
  return ret;
}



/**
 * Returns the EDSI interface for this resource.  IMPORTANT - User should
 * always use this method to access p_edsi, the EDSI data member.  The
 * EDSI will be created upon the first request and cached for
 * subsequent use.
 *
 * @throw InvalidException Indicates that the EDSI interface
 *        creation failed.
 */
EDSI *Resource::getEDSI() const throw(InvalidException)
{
  // Create EDSI
  if (p_edsi == 0) {
    const_cast<Resource*>(this)->p_edsi = EDSIFactory::getEDSI(p_url);
  }

  // Create failed - throw exception
  if (p_edsi == 0) {
    throw InvalidException("Server connection could not be established.",WHERE);
  }

  return p_edsi;
}



/**
 * Retrieves the MIME type for a resource. Checks :mime-type
 * property for backwards compatability, if :mime-type
 * does not exist then DAV:getcontenttype is used
 * 
 * @param resource Url of task resource
 *
 * @return MIME type of task resource 
 */
string Resource::getMimeType(const EcceURL& resource) const
{
  string ret = "";

  Resource *res = EDSIFactory::getResource(resource);
  if (res != 0) {
    string oldPropName = VDoc::getEcceNamespace() + ":mime-type";
    string propName = "DAV:getcontenttype";
    ret = res->getProp(oldPropName);
    if (ret.empty()) {
      ret = res->getProp(propName);
    }
  }
  return ret;
}



/**
 * Clears the cache of children, does not remove them from memory.
 */
void Resource::clearChildren()
{
  if (p_children != 0) {

    /*
    vector<Resource*>::iterator vecIt;
    vecIt = p_children->begin();
    for (vecIt = p_children->begin();
         vecIt != p_children->end();
         vecIt++) {
      if ((*vecIt) != 0) {
        delete (*vecIt);
        (*vecIt) = 0;
      }
    }
    */
    p_children->clear();
    delete p_children;
    p_children = 0;
  }
}


/**
 * Gets the location/URL for a task's session.
 *
 * @return returns EcceURL pointer URI for the task's session.
 *         User is responsible for memory management.
 */
EcceURL * Resource::getSessionId() const
{
  EcceURL *ret = new EcceURL(p_url.getParent());

  return ret;
}


/**
 * Gets a resource's session, if one exists.  Returns a 0 pointer
 * if the resource is not part of a session.
 *
 * @return returns Session pointer. User is responsible
 *         for memory management.
 */
Session *Resource::getSession() const
{
  Session *session = 0;
  EcceURL *sessionId = 0;

  sessionId = getSessionId();
  if (sessionId != 0) {
    session = dynamic_cast<Session*>(EDSIFactory::getResource(*sessionId));
    delete sessionId;
    sessionId = 0;
  }

  return session;
}



/**
 * Return whether has access to current Resource's children.
 *
 * The reason to have this function is that sometimes we don't want to trigger
 * another passprompt again.
 */
bool Resource::hasAccess()
{
  if (p_access == UNKNOWN)
    updateProps();
  return p_access == SUCCEEDED;
}



/**
 * Gets the children vector, perhaps NULL or zero-length.
 *
 * @return vector of children resources, EDSIFactory owns them (do not delete)
 */
vector<Resource*> * Resource::getCachedChildren() 
{
  return p_children;
}



/**
 * Retrieves the default tool for this resource.  If a default tool has
 * not been registered for this resource type then a zero pointer
 * is returned.  User is not responsible for memory management.
 */
ResourceTool *Resource::getDefaultTool()
{

  ResourceTool *ret = 0;
  ResourceType *rt = getDescriptor();

  if (rt->getStateBased()) {
    string result = getProp(VDoc::getEcceNamespace() + ":state");
    if (!result.empty()) {
      ret = rt->getDefaultTool(ResourceUtils::stringToState(result));
    }
  }
  else {
    ret = rt->getDefaultTool();
  }

  return ret;
}



/**
 * @deprecated
 */
int Resource::getSessionIndex() const
{
  string indexStr = getProp(VDoc::getDcQualifierNamespace() +
                            ":taskIndex");
  int ret = -1;
  StringConverter::toInt(indexStr, ret);
  
  return ret;
}



/**
 * @deprecated
 */
bool Resource::canModifyInputProvider()
{
  return true;
}



/**
 * Gets the input providing task. 
 * Caller is responsible for memory management.
 *
 * @param session pointer to Session 
 * @param linkName indicates the name of the INPUT_PROVIDER link, if any.
 * 
 * @return returns pointer to task job that provides input
 *         to the current task. 0 pointer is returned if
 *         an input task does not exist.
 */
Resource * Resource::getInputProvider(Session * session,
                                      const string& linkName) const
{
  Resource * inputProvider = 0;

  // Get input provider task, if it exists
  vector<Resource*> sources = session->getSources(this,
                                                  Session::INPUT_PROVIDER, 
                                                  linkName);
  if (sources.size() > 0) {
    inputProvider = sources[0];
  }
  
  return inputProvider;
}



/**
 * Gets the content type of task providing input to
 * the current task.
 *
 * @param linkName indicates the name of the INPUT_PROVIDER link, if any.
 * 
 * @return returns content type of task providing input
 *         to the current task. An undefined content type
 *         (ResourceDescriptor::CT_UNDEFINED) is returned if
 *         an input task does not exist (i.e. it's either 
 *         the only task or the first task in chain).
 */
ResourceDescriptor::CONTENTTYPE
Resource::getInputProviderType(const string& linkName)
{
  ResourceDescriptor::CONTENTTYPE ret = ResourceDescriptor::CT_UNDEFINED;

  Session *session = getSession();
  if (session != 0) {
    Resource *inputProvider = getInputProvider(session, linkName);
    if (inputProvider != 0) {
      ret = inputProvider->getContentType();
    }
  }
  return ret;
}


bool Resource::isHideInternal()
{
  return p_hideInternal;
}


void Resource::setHideInternal(bool hideInternal)
{
  p_hideInternal = hideInternal;
}


bool Resource::setReviewed(bool flag)
{
  bool ret = true;

  // create a string representation of the given flag
  string flagstr = flag ? "true" : "false";

  // the property name is used twice so we cache it here 
  string prop = VDoc::getEcceNamespace() + ":reviewed";

  // set the reviewed property for the calculation as metadata and sync with DAV
  ret &= addProp(prop, flagstr);

  if (ret) {
    notifyProperty(prop, flagstr);

    // set the date to use as well, blank if we're unsetting reviewed
    string date = "";
    if (flag) {
      TDateTime rdate;
      date = rdate.toString();
    }

    // the property name is used twice so we cache it here
    prop = VDoc::getEcceNamespace() + ":reviewedDate";
  
    // set the reviewed date for the calculation, sync with DAV
    ret &= addProp(prop, date);

    if (ret) {
      notifyProperty(prop, date);
    }
  }

  return ret;
}


void Resource::getDeleteMessage(bool & canDelete, string & message)
{
  canDelete = true;
  if (getResourceType() == ResourceDescriptor::RT_DOCUMENT) {
    message = "Are you sure you want to delete the file \"" + getName() + "\"?";
  }
  else if (getContentType() == ResourceDescriptor::CT_PROJECT) {
    if (getChildren()->size() == 0)
      message = "Project \"" + getName() + "\" is empty.\n\n"
        "Are you sure you want to delete the project?";
    else
      message = "Project \"" + getName() + "\" contains "
        + StringConverter::toString((int)(getChildren()->size())) + 
        " items.\n\nAre you sure you want to delete the project and"
        " its contents?";
  }
  else
    message = "Are you sure you want to delete the file \"" + getName() + "\"?";
}



void Resource::notifyProperty(const string & name, const string & value)
{
  JMSPublisher * publisher = EDSIFactory::getPublisher();
  JMSMessage * msg = publisher->newMessage();
  msg->addProperty("url", p_url.toString());
  msg->addProperty("name", name);
  msg->addProperty("value", value);
  publisher->publish("ecce_url_property", *msg);
  delete msg;
}


void Resource::notifyState(const string & state)
{
  JMSPublisher * publisher = EDSIFactory::getPublisher();
  JMSMessage * msg = publisher->newMessage();
  msg->addProperty("url", p_url.toString());
  msg->addProperty("state", state);
  publisher->publish("ecce_url_state", *msg);
  delete msg;
}

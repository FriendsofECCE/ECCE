/**
 * @file
 *
 * EDSIFactory class is designed to manage Resource object creation, deletion
 * and Resource Pool maintanance. The proper name for the class might be
 * ResourceFactory. 
 * 
 */

#include <iostream>
  using std::cerr;
  using std::endl;

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/JMSPublisher.H"
#include "util/ResourceUtils.H"
#include "util/TDateTime.H"

#include "dsm/NWChemMDModel.H"
#include "dsm/DavCalculation.H"
#include "dsm/DavPropCache.H"
#include "dsm/DavEDSI.H"
#include "dsm/DirDyVTSTTask.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSITraversable.H"
#include "dsm/FileEDSI.H"
#include "dsm/MdTask.H"
#include "dsm/PolyrateTask.H"
#include "dsm/PropertyTask.H"
#include "dsm/IPropCache.H"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"
#include "dsm/VDoc.H"


AuthEventListener     *EDSIFactory::p_aeListener = NULL;
ProgressEventListener *EDSIFactory::p_progressListener = NULL;
JMSPublisher          *EDSIFactory::p_publisher = NULL;
EDSIFactory::PoolMap   EDSIFactory::p_pool;

/**
 * Destructor.
 * Deletes all Resource objects from the pool.
 */
EDSIFactory::~EDSIFactory(){
  PoolIter poolIter;
  for (poolIter = p_pool.begin(); poolIter != p_pool.end(); poolIter++) {
    delete poolIter->second;
    poolIter->second = 0;
  }

  if (p_publisher != NULL) {
    delete p_publisher;
    p_publisher = NULL;
  }
}



/**
 * Get an object representation of a resource on the server.
 * The caller should NOT delete this Resource as it is owned by the factory.
 * If the resource does not yet exist in the pool, the resource object is
 * created and cached in the pool before being returned.
 *
 * @param url the location of the resource on the server
 * @return the pointer to the Resource object
 */
Resource *EDSIFactory::getResource(const EcceURL& url)
{
//    cout << "EDSIFactory::getResource(" << url.toString() << ")" << endl;
//    cout << "\tInitializing ret" << endl;
  Resource *ret = 0;
//    cout << "\tInitializing edsi" << endl;
  EDSI *edsi = getEDSI(url);

  // check if the resource is already contained in the pool
  bool skipPoolCheck = false;
//    cout << "\tChecking the pool for " << url.toString() << endl;
  PoolIter poolIter = p_pool.find(url.toString());
  if (poolIter != p_pool.end()) {
//    cout << "\tFound; returning" << endl;
    return poolIter->second;
  }
  else skipPoolCheck = true;

  // resource was not in pool, continue with object creation

//  cout << "\tNot found; continuing" << endl;

  if (edsi != 0) {
//    cout << "\tNon-null edsi" << endl;

    vector<MetaDataRequest> vmdr;
    ResourceMetaDataResult result;

    // Build property query
    vector<string> props; 
    edsi->describeServerMetaData(props);
    MetaDataRequest mdr;
    int numProps = props.size();
    for (int idx=0; idx<numProps; idx++) {
      mdr.name = props[idx];
      vmdr.push_back(mdr);
    }

    // Query properties, and cache them
    edsi->setURL(url);
    if (edsi->getMetaData(vmdr,result)) {
      ret = getResource(url, result, skipPoolCheck);
      if (ret != 0) {
        ret->cacheProps(result.metaData);
      }
    }
  }

  // clean up memory
  if (edsi != 0) {
    delete edsi;
    edsi = 0;
  }

  return ret;
}



/**
 * Get an object representation of a resource on the server.
 * The caller should NOT delete this Resource as it is owned by the factory.
 * If the resource does not yet exist in the pool, the resource object is
 * created and cached in the pool before being returned.<br>
 * This is the intermediate step in the getResource chain of 3 and only
 * availabe to friends of this factory.
 *
 * @todo The function parameters are a bit redundant since the url of the
 *       resource is already stored in the ResourceMetaDataResult.
 * @param url the location of the resource on the server
 * @param result the resource's metadata and type information
 * @param skipPoolCheck USE WITH CARE - the factory should be the only object
 *        using this parameter, since careless use of this parameter could 
 *        cause an object already in the pool to be recreated
 * @return the pointer to the Resource object
 */
Resource *EDSIFactory::getResource(const EcceURL& url, 
                                   ResourceMetaDataResult result,
                                   bool skipPoolCheck)
{
  Resource *ret = 0;

  // check if the resource is already contained in the pool
  if (!skipPoolCheck) {
    PoolIter poolIter = p_pool.find(url.toString());
    if (poolIter != p_pool.end()) {
      return poolIter->second;
    }
    else skipPoolCheck = true;
  }

  string AT =  VDoc::getEcceNamespace() + ":application";
  string at = "";
  string rt = ResourceUtils::resourceTypeToString(result.resourcetype);
  string ct = result.contenttype;

  for (int i = 0; i < result.metaData.size(); i++) {
    // Application Type
    if (result.metaData[i].name == AT) {
      at = result.metaData[i].value;
    }
  }

  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
  ResourceType *resourceType = rd.getResourceType(rt, ct, at);
  if (resourceType != 0) {
    ret = getResource(url, resourceType, skipPoolCheck);
  }

  return ret;
}



/**
 * Get an object representation of a resource on the server.
 * The caller should NOT delete this Resource as it is owned by the factory.
 * If the resource does not yet exist in the pool, the resource object is
 * created and cached in the pool before being returned.  The pooled resource
 * is always returned.<br>
 * This is the last step in the getResource chain of 3 and only availabe to
 * friends of the factory.  This is where the resource object is created and 
 * cached in the pool.
 *
 * @param url the location of the resource on the server
 * @param resourceType the resource's type information
 * @param skipPoolCheck USE WITH CARE - the factory should be the only object
 *        using this parameter, since careless use of this parameter could 
 *        cause an object already in the pool to be recreated
 * @return the pointer to the Resource object
 */
Resource *EDSIFactory::getResource(const EcceURL& url, 
                                   ResourceType* resourceType,
                                   bool skipPoolCheck)
{
  Resource *ret = 0;

  // check if the resource is already contained in the pool
  if (!skipPoolCheck) {
    PoolIter poolIter = p_pool.find(url.toString());
    if (poolIter != p_pool.end()) {
      return poolIter->second;
    }
    else skipPoolCheck = true;
  }

  if (resourceType != 0) {

    ResourceDescriptor::APPLICATIONTYPE at = 
            ResourceUtils::stringToApplicationType(
            resourceType->getApplicationType());
    ResourceDescriptor::ResourceCategory rc = 
            ResourceUtils::stringToResourceCategory(
            resourceType->getFactoryCategory());

    try {

      switch (rc) {
        case ResourceDescriptor::RC_DOCUMENT:
        case ResourceDescriptor::RC_PROJECT:
          ret = new Resource(url);
          break;
        case ResourceDescriptor::RC_ES_CALCULATION:
          ret = new DavCalculation(url);
          break;
        case ResourceDescriptor::RC_MD_SESSION:
        case ResourceDescriptor::RC_REACTION_SESSION:
        case ResourceDescriptor::RC_CONDENSED_REACTION_SESSION:
          ret = new Session(url);
          break;
        case ResourceDescriptor::RC_MD_TASK:
          ret = new MdTask(url);
          break;
        case ResourceDescriptor::RC_REACTION_TASK:
          if (at == ResourceDescriptor::AT_DRDVTST) {
            ret = new DirDyVTSTTask(url);
          }
          else if (at == ResourceDescriptor::AT_POLYRATE) {
            ret = new PolyrateTask(url);
          }
          break;
        case ResourceDescriptor::RC_CONDENSED_REACTION_TASK:
          if (at == ResourceDescriptor::AT_SOLVATE) {
            ret = new DavCalculation(url);
          }
          else if (at == ResourceDescriptor::AT_METADYNAMICS) {
            ret = new DavCalculation(url);
          }
          break;
        default:
          throw InvalidException("Invalid resource category "
                                 "requested for getResource", WHERE);
          break;
      }
    }
    catch (...) {
      if (ret != 0) {
        delete ret;
      }
      ret = 0;
    }
  }

  if (ret != 0) {
    // Important - initialize Resource's descriptor 
    ret->setDescriptor(resourceType);

    // object created, add to pooled resources
    p_pool[url.toString()] = ret;
  }

  return ret;
}



/**
 * Pool maintenance function.
 * Typically, this function is called during a response to the JMS message 
 * that a Resource was created on the server.  However, it can be called at 
 * any time, but this is not recommended.
 * Determines whether the new resource should be added to the pool by checking
 * for the created Resource's parent.
 *
 * @param url the url of the Resource to possibly add to the pool
 */
void EDSIFactory::createPoolResource(const EcceURL &url)
{
  string parentStr = url.getParent().toString();
  PoolIter poolIter = p_pool.find(parentStr);
  if (poolIter != p_pool.end()) {
    // updates the children vector and calls getResource to create in pool
    poolIter->second->addChild(url);
  }
}



/**
 * Pool maintenance function: Removes resource from the pool.
 *
 * Typically, this function is called during a response to the JMS message 
 * that a Resource was removed from the server.  However, it can be called at
 * any time, but this is not recommended.
 * Simply calls the removeResource function, which clears the pool of the 
 * given resource at url and all its children.
 *
 * This will delete the object that the resource pointer refers to, but it
 * does not remove anything from the server.  It also removes children from
 * the pool.  Also updates the parent's children vector.
 *
 * @param url the url of the resource to remove from the pool
 */
void EDSIFactory::removePoolResource(const EcceURL &url)
{
  string urlStr = url.toString();
  string parentUrlStr = url.getParent().toString();
  PoolIter poolIter;

  // Note that the for loop does not increment automatically.  This is because
  // after STL::map::erase is called the 'poolIter' becomes invalid.  You cannot
  // increment it after that.  As it is written, the statement 
  // p_pool.erase(poolIter++) increments the iterator and returns the previous
  // position, so erase safely gets rid of the desired element and retains
  // a valid iterator pointing to the next element (or end()).

  for (poolIter = p_pool.begin(); poolIter != p_pool.end(); /*no increment*/) {
    if (urlStr == poolIter->first || poolIter->first.find(urlStr+"/") == 0) {
      delete poolIter->second;
      poolIter->second = 0;
      p_pool.erase(poolIter++);
    }
    else {

      // check for parent in order to update children vector
      if (poolIter->first == parentUrlStr) {
        poolIter->second->eraseChild(url);
      }

      // no delete occurred above, need to increment iterator
      poolIter++;
    }
  }
}


/**
 * Pool maintenance function.
 * Typically, this function is called during a response to the JMS message 
 * that a Resource was renamed/moved on the server.  However, it can be called
 * at any time, but this is not recommended.
 * Iterates through the pool, changing the pool keys and Resource URLS as
 * appropriate.
 *
 * @param oldurl the url of the resource that was renamed
 * @param newurl the new url by which to refer to the resource
 */
void EDSIFactory::renamePoolResource(const EcceURL &oldUrl, 
                                     const EcceURL &newUrl)
{
  PoolIter poolIter;
  string oldUrlStr(oldUrl.toString());
  string newUrlStr(newUrl.toString());

  for (poolIter = p_pool.begin(); poolIter != p_pool.end(); /*no increment*/)
  {
    if (poolIter->first == oldUrlStr) {
      // we found the exact resource to change

      // insert the resource at it's new location in the pool
      p_pool[newUrlStr] = poolIter->second;

      // remove the old pool location
      p_pool.erase(poolIter++);

      // update the resource pointer to the new url
      p_pool[newUrlStr]->setURL(newUrl);
    }
    else if (poolIter->first.find(oldUrlStr+"/") == 0) {
      // we found a child of the changing resource
      string newChildUrlStr = poolIter->first;
      newChildUrlStr.replace(0, oldUrlStr.length(), newUrlStr.c_str());

      // insert the resource at it's new location in the pool
      p_pool[newChildUrlStr] = poolIter->second;

      // remove the old pool location
      p_pool.erase(poolIter++);

      // update the resource pointer to the new url
      p_pool[newChildUrlStr]->setURL(newChildUrlStr);
    }
    else {
      // url change does not impact the current iterator's resource
      // we still need to increment the iterator
      poolIter++;
    }
  }
}



/**
 * Pool maintenance function.
 * Typically, this function is called during a response to the JMS message 
 * that a Resource's metadata properties were changed.
 * Simply calls the refreshResourceProperties function, which replaces the 
 * properties cached in the Resource with those retrieved from the server.
 *
 * @param url the url of the server resource that was changed
 */
void EDSIFactory::changePoolResource(const EcceURL &url)
{
  refreshResourceProperties(url);
}



/**
 * Returns the interface to the data server.
 *
 * @param url the url of the resource to interface to
 * @return the pointer to the interface, NULL if an error occurs
 */
EDSI *EDSIFactory::getEDSI(const EcceURL& url)
{
  EDSI *ret = NULL;
  if (url.getProtocol().find("http") == 0) {
    ret = new DavEDSI(url);
  } else if (url.getProtocol().find("file") == 0 ||
      url.getProtocol().empty()) {
    ret = new FileEDSI(url);
  }

  EE_ASSERT(ret,EE_WARNING,"Unrecognized URL - no EDSI returned.");
//  EE_ASSERT(p_aeListener,EE_WARNING,"No AuthEventListener registered");
//  EE_ASSERT(p_progressListener,EE_WARNING,"No ProgressEvent registered");

  if (ret) {
    ret->addAuthEventListener(p_aeListener);
    ret->addProgressEventListener(p_progressListener);
  }
  return ret;
}



/**
 * Returns the virtual document at the given url.
 *
 * @param url the url of the virtual document
 * @return the pointer to the virtual document, 0 if an error occurs
 */
VDoc* EDSIFactory::getVdoc(const EcceURL& url)
{
  VDoc *ret = 0;

  try {
    ret = new VDoc(url);
  }
  catch (...) {
    ret = 0;
  }

  return ret;
}



/**
 * Gets IPropCache based on given parameters.
 * A wrapper to retieve a DavPropCache.
 *
 * @param url location of the IPropCache to retrieve
 * @param storage 
 * @param maxStorageFailure
 * @return the IPropCache
 */
IPropCache *EDSIFactory::getIPropCache(const EcceURL& url, const bool storage, 
                                       const int maxStorageFailure) 
{
  return new DavPropCache(url, storage, maxStorageFailure);
}



/**
 * Adds listener whose responsiblity it is to provide authorization
 * information as needed.  Currently only one listener is supported at
 * a time.
 *
 * @param l the listener
 */
void EDSIFactory::addAuthEventListener(AuthEventListener *l)
{
    p_aeListener = l;
}



/**
 * Remove listener if it has been added.  Equality based on object
 * identity.
 *
 * @param l the listener
 */
void EDSIFactory::removeAuthEventListener(AuthEventListener *l)
{
    if (p_aeListener == l) p_aeListener = NULL;
}



/**
 * Add listener whose responsiblity it is to listen for server 
 * progress updates.  Currently only one listener is supported at a time.
 *
 * @param l the listener
 */
void EDSIFactory::addProgressEventListener(ProgressEventListener *l)
{
    p_progressListener = l;
}



/**
 * Remove listener if it has been added.
 *
 * @param l the listener
 */
void EDSIFactory::removeProgressEventListener(ProgressEventListener *l)
{
    if (p_progressListener == l) p_progressListener = NULL;
}



/**
 * Removes old publisher if it exists and creates a new one with the given
 * application as the source name.
 *
 * @param app_name the name of the app for which we are publishing messages
 */
JMSPublisher * EDSIFactory::getPublisher(const string &app_name)
{
  if (p_publisher == NULL)
    p_publisher = new JMSPublisher(app_name);
  return p_publisher;
}



/**
 * Let the world know a Resource was created at the given url.
 *
 * @param url where the new Resource was created
 */
void EDSIFactory::notifyCreate(const string & url)
{
  JMSMessage *msg = getPublisher()->newMessage();
  msg->addProperty("url", url);
  getPublisher()->publish("ecce_url_created", *msg);
  delete msg;
}



/**
 * Let the world know a Resource has been moved/renamed.
 *
 * @param oldurl where the old Resource used to be
 * @param newurl where the Resource is now
 */
void EDSIFactory::notifyRename(const string & oldurl, const string & newurl)
{
  JMSMessage *msg = getPublisher()->newMessage();
  msg->addProperty("oldurl", oldurl);
  msg->addProperty("newurl", newurl);
  getPublisher()->publish("ecce_url_renamed", *msg);
  delete msg;
}



/**
 * Let the world know a Resource has been removed from the server.
 *
 * @param url where the Resource no longer exists
 */
void EDSIFactory::notifyRemove(const string & url)
{
  JMSMessage *msg = getPublisher()->newMessage();
  msg->addProperty("url", url);
  getPublisher()->publish("ecce_url_removed", *msg);
  delete msg;
}



/**
 * Let the world know a metadata property for a Resource has changed.
 *
 * @param url where the Resource is located that changed
 * @param name the key of the property that has been changed
 * @param value the value associated with the changed property
 */
void EDSIFactory::notifyProperty(const string & url, const string & name, 
                                 const string & value)
{
  JMSMessage *msg = getPublisher()->newMessage();
  msg->addProperty("url", url);
  msg->addProperty("name", name);
  msg->addProperty("value", value);
  getPublisher()->publish("ecce_url_property", *msg);
  delete msg;
}



/**
 * Let the world know the state of the given Resource has changed.
 *
 * @param url where the Resource is located that changed state
 * @param state the new state of the Resource
 */
void EDSIFactory::notifyState(const string & url, const string & state)
{
  JMSMessage *msg = getPublisher()->newMessage();
  msg->addProperty("url", url);
  msg->addProperty("state", state);
  getPublisher()->publish("ecce_url_state", *msg);
  delete msg;
}



/**
 * Create an ecce resource.  This does NOT add the resource to the managed 
 * resource pool.  This may be one of the following
 * resource types:<br>
 * <p>
 * <ul>
 *  <li>Ecce Project</li>
 *  <li>ES Calculation</li>
 *  <li>MD Study</li>
 *  <li>MD Task</li>
 * </ul>
 * </p>
 *
 * @param   interface an EDSI pointer, should be set to the
 *          location of the parent/owning resource.
 * @param   name name of the new resource.
 * @param   resourceType this is the type of resource to create.
 *          The resourceTypes are defined in Ecce's resource
 *          registry (ie ResourceDescriptor.xml)
 * @return  the location of the newly created resource as an EcceURL
 */
EcceURL* EDSIFactory::createResource(EDSI* interface, 
                                     const string& name, 
                                     ResourceType* resourceType)
  throw(InvalidException)
{
  EcceURL *ret = 0;

  if (resourceType != 0) {

    ResourceDescriptor::CONTENTTYPE ct = 
            ResourceUtils::stringToContentType(
            resourceType->getContentType());
    ResourceDescriptor::APPLICATIONTYPE at = 
            ResourceUtils::stringToApplicationType(
            resourceType->getApplicationType());
    ResourceDescriptor::ResourceCategory rc = 
            ResourceUtils::stringToResourceCategory(
            resourceType->getFactoryCategory());

    // Determine if adding resource to a session and if it's allowed. 
    // The EDSI interface should be set to the parent/owning resource.
    bool addToSessionAllowed = false;
    bool addToSession = false;
    ResourceDescriptor::ResourceCategory sessionType = 
            ResourceDescriptor::RC_UNDEFINED;

    EcceURL sessionId = interface->getURL();
    Resource *session = getResource(sessionId);
    if (session != 0) {
      if (session->getDescriptor()->getIsSessionBased()) {
        addToSession = true;
        sessionType = ResourceUtils::stringToResourceCategory(
                session->getDescriptor()->getFactoryCategory());
        vector<ResourceType*> contains = 
                session->getDescriptor()->getContains();
        vector<ResourceType*>::iterator vecIt;
        for (vecIt = contains.begin();
             vecIt != contains.end();
             vecIt++) {
          if ((*(*vecIt)) == (*resourceType)) {
            addToSessionAllowed = true;
            break;
          }
        }
      }
    }
    if ((addToSession) && (!addToSessionAllowed)) {
      throw InvalidException("Adding the resource to session is not allowed.", 
                             WHERE);
    }


    try {

      switch (rc) {
        case ResourceDescriptor::RC_PROJECT:
          ret = createProject(interface, name);
          break;
        case ResourceDescriptor::RC_ES_CALCULATION:
          ret = createCalculation(interface, name, resourceType->getApplicationType());
          break;
        case ResourceDescriptor::RC_MD_SESSION:
          ret = createSession(interface, name);
          break;
        case ResourceDescriptor::RC_REACTION_SESSION:
          ret = createSession(interface, name, 
                              ResourceDescriptor::AT_REACTION_STUDY);
          break;
        case ResourceDescriptor::RC_CONDENSED_REACTION_SESSION:
          ret = createSession(interface, name, 
                              ResourceDescriptor::AT_CONDENSED_REACTION_STUDY);
          break;
        case ResourceDescriptor::RC_MD_TASK:
        case ResourceDescriptor::RC_REACTION_TASK:
        case ResourceDescriptor::RC_CONDENSED_REACTION_TASK:
          ret = createTask(interface, name, ct, at, &sessionId);
          break;
        default:
          throw InvalidException("Invalid resource category "
                                 "requested for createResource", WHERE);
          break;
      }

      if (ret != 0) {
        // Add resource to session, if necessary
        if ((addToSession) && (addToSessionAllowed)) {
          addSessionTask(*ret, sessionId, sessionType);
        }
      }

    }
    catch (...) {
      if (ret != 0) {
        // Rollback create
        interface->setURL(*ret);
        if (interface->exists()) {
          interface->removeResource();
        }
        delete ret;
      }
      ret = 0;
      throw;
    }
  }

  return ret;
}


/**
 * Create a calculation structure and initial metadata info.
 * Rewritten to eliminate returns from middle of function and to always
 * return a zero pointer if an error occurred anywhere along the line - a
 * sort of transaction.
 *
 * @param   interface the interface to the parent/owning node
 * @param   name the name of the new calculation
 * @param   appType the application code string, defined in registry (ResourceDescriptor.xml)
 * @return  the location of the newly created resource, as an EcceURL, will be
 *          0 if an error occurred
 */
EcceURL*
EDSIFactory::createCalculation(EDSI* interface, 
                               const string& name, 
                               string appType)
{
  EE_ASSERT(interface,EE_FATAL,"Unassigned EDSI parameter");

  EDSIMessage myStack;
  EcceURL* ret = NULL;

  //Used to be code to check if its a valid place to put the object.
  // took it out

  // Create Virtual Document
  ret = interface->makeCollection(name);
  if (ret) {
    VDoc *virtualDoc = createVdoc(*ret); 
    if (virtualDoc == 0) {
      interface->m_msgStack.add("UNABLE_TO_COMPLETE_REQUEST", 
                                "Virtual Document could not be created");
      interface->setURL(*ret);
      interface->removeResource();
      delete ret;
      ret = 0;
    }
    else {
      delete virtualDoc;
    }
  }

  if (ret) {

    interface->setURL(*ret);

    vector<MetaDataResult> results(8);
    results[0].name = VDoc::getEcceNamespace() + ":contenttype";
    results[0].value = ResourceUtils::contentTypeToString(
            ResourceDescriptor::CT_CALCULATION);

    results[1].name = VDoc::getEcceNamespace() + ":resourcetype";
    results[1].value = ResourceUtils::resourceTypeToString(
            ResourceDescriptor::RT_VIRTUAL_DOCUMENT);

    results[2].name = VDoc::getEcceNamespace() + ":owner";
    results[2].value = Ecce::serverUser();

    // We keep our own creation data that we can modify so that the
    // imported or old OS calcs can have an accurate date.
    TDateTime now;
    results[3].name = VDoc::getEcceNamespace() + ":creationdate";
    results[3].value = now.toString();
    
    //Set the calculation version 
    results[4].name = VDoc::getEcceNamespace() + ":createdWith";
    results[4].value = Ecce::ecceVersion(); //"ecce version 2.0"; 
    //cout << "current version = " << Ecce::ecceVersion() << endl;
                     
    results[5].name = VDoc::getEcceNamespace() + ":currentVersion";
    results[5].value = Ecce::ecceVersion(); //"ecce version 2.0"; 

    results[6].name = VDoc::getEcceNamespace() + ":application";
    results[6].value = appType;

    // At first reviewed was a checkmark.  if it didn't show, it wasn't 
    // reviewed.  It was requested that it be a text field instead, so it needs
    // to be initialized along with a new calculation.
    results[7].name = VDoc::getEcceNamespace() + ":reviewed";
    results[7].value = "false";


    if (myStack.size() == 0) {
      if (!interface->putMetaData(results))  {
        myStack.add(interface->m_msgStack);
      }
    }

    interface->setURL(*ret);

    vector<MetaDataResult> taskmd(2);

    taskmd[0].name = VDoc::getEcceNamespace() + ":owner";
    taskmd[0].value = Ecce::serverUser();

    taskmd[1].name = VDoc::getEcceNamespace() + ":state";
    taskmd[1].value = ResourceUtils::stateToString(
            ResourceDescriptor::STATE_CREATED);

    if (myStack.size() == 0) {
      if(!interface->putMetaData(taskmd)) {
        myStack.add( interface->m_msgStack);
      }
    }

  } else {
    myStack.add(interface->m_msgStack);
  }


  if (myStack.size() > 0) {
    // some error occured. Set the interface message stack and return
    // a 0 pointer.
    interface->m_msgStack.clear();
    interface->m_msgStack.add(myStack);
    if (ret) {
      delete ret;
      ret = 0;
    }
  }

  return ret;
}



/**
 * Create an ecce project collection.  A 0 pointer is returned if the 
 * project could not be created. 
 *
 * @param   interface the interface to the parent/owning resource
 * @param   name name of the new project
 * @return  the locatino of the new project resource, as an EcceURL, will be 0
 *          if an error occurred
 */
EcceURL* EDSIFactory::createProject(EDSI* interface, const string& name)
{
  EcceURL *ret = 0;
  ret = interface->makeCollection(name);

  if (ret != 0) {
    interface->setURL(*ret);
    vector<MetaDataResult> results(2);

    results[0].name = VDoc::getEcceNamespace() + ":contenttype";
    results[0].value = ResourceUtils::contentTypeToString(
            ResourceDescriptor::CT_PROJECT);

    TDateTime now;
    results[1].name = VDoc::getEcceNamespace() + ":creationdate";
    results[1].value = now.toString();

    if (!interface->putMetaData(results)) {
      interface->setURL(*ret);
      interface->removeResource();
      delete ret;
      ret = 0;
    }
  }

  return ret;
}



/**
 * Creates Session and initializes its metadata.  If a problem is
 * encountered a zero pointer is returned.
 *
 * @param interface connection to DAV server.  Session will be created
 *        as a child of the interface connection URL.
 * @param sessionName name of dav collection, representing the Session.
 * @return pointer to an EcceURL identifying the Session.  Zero pointer 
 *         is returned if there was a problem creating the session.
 */
EcceURL* EDSIFactory::createSession(EDSI* interface, 
                                    const string& sessionName,
                                    ResourceDescriptor::APPLICATIONTYPE appType)
{
  EE_ASSERT(interface,EE_FATAL,"Unassigned EDSI parameter");
  EDSIMessage myStack;
  EcceURL* ret = NULL;

  // Create the Session 
  ret = interface->makeCollection(sessionName);

  if (ret) {

    interface->setURL(*ret);

    vector<MetaDataResult> results(8);
    results[0].name = VDoc::getEcceNamespace() + ":contenttype";
    results[0].value = ResourceUtils::contentTypeToString(
            ResourceDescriptor::CT_SESSION);

    results[1].name = VDoc::getEcceNamespace() + ":resourcetype";
    results[1].value = ResourceUtils::resourceTypeToString(
            ResourceDescriptor::RT_VIRTUAL_DOCUMENT);

    results[2].name = VDoc::getEcceNamespace() + ":application";
    results[2].value = ResourceUtils::applicationTypeToString(appType);

    results[3].name = VDoc::getEcceNamespace() + ":owner";
    results[3].value = Ecce::serverUser();

    TDateTime now;
    results[4].name = VDoc::getEcceNamespace() + ":creationdate";
    results[4].value = now.toString();
    
    results[5].name = VDoc::getEcceNamespace() + ":createdWith";
    results[5].value = Ecce::ecceVersion(); 
                     
    results[6].name = VDoc::getEcceNamespace() + ":currentVersion";
    results[6].value = Ecce::ecceVersion(); 

    results[7].name = "DAV:displayname";
    results[7].value = sessionName; 
                     

    // Store metadata for Session
    if (!interface->putMetaData(results))  {
      myStack.add(interface->m_msgStack);
    }

  } else {
    myStack.add(interface->m_msgStack);
  }


  // Cleanup if an error occurred
  if (myStack.size() > 0) {
    interface->m_msgStack.clear();
    interface->m_msgStack.add(myStack);
    if (ret) {
      delete ret;
      ret = 0;
    }
  }

  return ret;
}



/**
 * Creates a (session) task and initializes its metadata.
 *
 * @param interface interface to parent/owning resource on server
 * @param name name of new task
 * @param taskType what kind of task is needing creation
 * @param appType what kind of session is containing this task
 * @param sessionId the url of the containing session (often same as interface?)
 * @return the location of the created task, as an EcceURL
 */
EcceURL* EDSIFactory::createTask(EDSI* interface, 
                                 const string& name, 
                                 ResourceDescriptor::CONTENTTYPE taskType,
                                 ResourceDescriptor::APPLICATIONTYPE appType,
                                 EcceURL *sessionId)
{
  EE_ASSERT(interface,EE_FATAL,"Unassigned EDSI parameter");

  EDSIMessage myStack;
  EcceURL* ret = 0;
  Session *session = 0;
  bool isValidSession = false;

  // sessionId must contain a valid session URL
  if (sessionId != 0) {
    session = dynamic_cast<Session*>(getResource(*sessionId));
    if (session != 0) {
      isValidSession = true;
    }
  }

  if (isValidSession) {
    // Create Virtual Document
    ret = interface->makeCollection(name,"%d");
    if (ret) {
      VDoc *virtualDoc = createVdoc(*ret); 
      if (virtualDoc == 0) {
        interface->m_msgStack.add("UNABLE_TO_COMPLETE_REQUEST", 
                                  "Virtual Document could not be created");
        interface->setURL(*ret);
        interface->removeResource();
        delete ret;
        ret = 0;
      }
      else {
        delete virtualDoc;
      }
    }
  }

  if (ret) {

    interface->setURL(*ret);

    vector<MetaDataResult> results(9);
    results[0].name = VDoc::getEcceNamespace() + ":contenttype";
    results[0].value = ResourceUtils::contentTypeToString(taskType);

    results[1].name = VDoc::getEcceNamespace() + ":application";
    results[1].value = ResourceUtils::applicationTypeToString(appType);

    results[2].name = VDoc::getEcceNamespace() + ":resourcetype";
    results[2].value = ResourceUtils::resourceTypeToString(
            ResourceDescriptor::RT_VIRTUAL_DOCUMENT);

    results[3].name = VDoc::getEcceNamespace() + ":owner";
    results[3].value = Ecce::serverUser();

    // We keep our own creation data that we can modify so that the
    // imported or old OS calcs can have an accurate date.
    TDateTime now;
    results[4].name = VDoc::getEcceNamespace() + ":creationdate";
    results[4].value = now.toString();
    
    //Set the calculation version 
    results[5].name = VDoc::getEcceNamespace() + ":createdWith";
    results[5].value = Ecce::ecceVersion(); //"ecce version 2.0"; 
                     
    results[6].name = VDoc::getEcceNamespace() + ":currentVersion";
    results[6].value = Ecce::ecceVersion(); //"ecce version 2.0"; 

    results[7].name = "DAV:displayname";
    results[7].value = name;

    // At first reviewed was a checkmark.  if it didn't show, it wasn't 
    // reviewed.  It was requested that it be a text field instead, so it needs
    // to be initialized along with a new calculation.
    results[8].name = VDoc::getEcceNamespace() + ":reviewed";
    results[8].value = "false";
                    

    if (myStack.size() == 0) {
      if (!interface->putMetaData(results))  {
        myStack.add(interface->m_msgStack);
      }
    }

    interface->setURL(*ret);

    vector<MetaDataResult> taskmd(2);

    taskmd[0].name = VDoc::getEcceNamespace() + ":owner";
    taskmd[0].value = Ecce::serverUser();

    taskmd[1].name = VDoc::getEcceNamespace() + ":state";
    taskmd[1].value = ResourceUtils::stateToString(
            ResourceDescriptor::STATE_CREATED);

    if (myStack.size() == 0) {
      if(!interface->putMetaData(taskmd)) {
        myStack.add( interface->m_msgStack);
      }
    }

  } else {
    myStack.add(interface->m_msgStack);
  }


  if (myStack.size() > 0) {
    // some error occured. Set the interface message stack and return
    // a 0 pointer.
    interface->m_msgStack.clear();
    interface->m_msgStack.add(myStack);
    if (ret) {
      delete ret;
      ret = 0;
    }
  }

  return ret;
}



/**
 * Create the virtual document at the given url.
 *
 * @param url the location where the vdoc will be created
 * @return pointer to the vdoc interface, 0 if an error occurred
 */
VDoc* EDSIFactory::createVdoc(const EcceURL& url)
{
  VDoc *ret = 0;

  try {
    ret = new VDoc(url, true);
  }
  catch (...) {
    ret = 0;
  }

  return ret;
}



/**
 * Replaces the latest metadata at the given url with the metadata retrieved
 * from the server, if the Resource exists in the pool.
 *
 * @param url the url of the pool Resource to refresh
 */
void EDSIFactory::refreshResourceProperties(const EcceURL &url)
{
  PoolIter poolIter = p_pool.find(url.toString());
  if (poolIter != p_pool.end()) {
    // what was the application type before the refresh?
    string applicationTypeBefore = poolIter->second->getProp(
            VDoc::getEcceNamespace() + ":application");
    poolIter->second->updateProps();
    // after?
    string applicationTypeAfter = poolIter->second->getProp(
            VDoc::getEcceNamespace() + ":application");
    // did the application type change?  this can only happen with ES calcs
    if (applicationTypeBefore != applicationTypeAfter) {
      // we need to update the ResourceType* stored by our Resource
      string contentType = poolIter->second->getProp(
              VDoc::getEcceNamespace() + ":contenttype");
      string resourceType = poolIter->second->getProp(
              VDoc::getEcceNamespace() + ":resourcetype");
      ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();
      ResourceType *type = rd.getResourceType(resourceType, contentType,
                                              applicationTypeAfter);
      if (type != 0) {
        poolIter->second->setDescriptor(type);
      }
    }
  }
}



/**
 * Adds the given taskId to the given session's model.
 * Don't need to check if the session can contain the task (ie contains 
 * relationship). This check is done in the createResource call.
 *
 * @param taskId url of task to add
 * @param sessionId url of session to add task to (not necessarily the owner?)
 * @param sessionType what kind of session we are dealing with
 */
void
EDSIFactory::addSessionTask(const EcceURL& taskId, 
                            const EcceURL& sessionId,
                            ResourceDescriptor::ResourceCategory sessionType)
{
  if (sessionType == ResourceDescriptor::RC_MD_SESSION) {
    // the logic for adding a session task is done within the Organizer
    // (CalcMgr::createResource)
    // addSessionMdTask(taskId, sessionId);
  }
  else if (sessionType == ResourceDescriptor::RC_REACTION_SESSION) {
    // since we aren't paying attention to selection for linking new reaction
    // study tasks, we can add the task into the session here
    addSessionReactionTask(taskId, sessionId);
  }
  else if (sessionType == ResourceDescriptor::RC_CONDENSED_REACTION_SESSION) {
    // the logic for adding a session task is done within the Organizer
    addSessionCondensedReactionTask(taskId, sessionId);
  }
  else {
    throw InvalidException("Invalid session type selected to add resource", 
                           WHERE);
  }
}


/**
 * Adds given task to the given reaction session.
 *
 * @param taskId url of the task to add
 * @param sessionId url of the containing session model
 * @todo this logic may or may not belong in the factory.  For example, it might
 *       belong in a derived session class's add/create Child method.
 */
void EDSIFactory::addSessionReactionTask(const EcceURL& taskId, 
                                         const EcceURL& sessionId)
{
  Resource *task = 0;
  Session *session = 0;

  // Get task object
  task = getResource(taskId);

  // It may be that something other then a task is being 
  // added to the session, like a project.  If this is 
  // the case then skip
  if (task != 0) {
    // Get tasks resource type
    ResourceType *rt = task->getDescriptor();
      
    session = dynamic_cast<Session*>(getResource(sessionId));

    // Add the new task.
    // Session takes ownership of pointer - no need to delete it
    // after adding to session.

    // Only one DirDyVTST task allowed per session
    // If adding a Polyrate task then link to the DirdyVTST task
    // If the DirdyVTST task does not exist then throw exception
    ResourceDescriptor::APPLICATIONTYPE at =
      ResourceUtils::stringToApplicationType(rt->getApplicationType());
    DirDyVTSTTask *dirdy = 0;
    bool isDirdy = (at == ResourceDescriptor::AT_DRDVTST);
    bool isPolyrate = (at == ResourceDescriptor::AT_POLYRATE);
    vector<Resource *> tasks = session->getMembers();
    for (int i = 0; i < tasks.size(); i++) {
      ResourceType *resType = tasks[i]->getDescriptor();
      ResourceDescriptor::APPLICATIONTYPE appType =
        ResourceUtils::stringToApplicationType(resType->getApplicationType());
      if (appType == ResourceDescriptor::AT_DRDVTST) {
        dirdy = dynamic_cast<DirDyVTSTTask*>(tasks[i]);
        break;
      }
    }

    if ((isDirdy) && (dirdy != 0)) {
      throw InvalidException("Only one DirDyVTST task allowed per reaction "
                             "study", WHERE);
    }
        
    if ((isPolyrate) && (dirdy == 0)) {
      throw InvalidException("DirDyVTST task must be added to reaction study "
                             "before adding a POLYRATE task", WHERE);
    }

    // The user is allowed to create other objects such as projects, but
    // make sure they aren't linked into the session or it will be
    // corrupted
    if (isDirdy) {
      // DirDyVTST task is first session member so no target specified
      session->addMember(task);
      session->saveLinkbase();
    } else if (isPolyrate) {
      // must by a Polyrate task and those always get linked to DirDyVTST
      // tasks
      session->addMemberAsTarget(task, dirdy);
      session->saveLinkbase();
    }
  }
}


/**
 * Adds given task to the given reaction session.
 *
 * @param taskId url of the task to add
 * @param sessionId url of the containing session model
 * @todo this logic may or may not belong in the factory.  For example, it might
 *       belong in a derived session class's add/create Child method.
 */
void EDSIFactory::addSessionCondensedReactionTask(const EcceURL& taskId, 
                                                  const EcceURL& sessionId)
{
  Resource *task = 0;
  Session *session = 0;

  // Get task object
  task = getResource(taskId);

  // It may be that something other then a task is being 
  // added to the session, like a project.  If this is 
  // the case then skip
  if (task != 0) {
    // Get tasks resource type
    ResourceType *rt = task->getDescriptor();
      
    session = dynamic_cast<Session*>(getResource(sessionId));

    // Add the new task.
    // Session takes ownership of pointer - no need to delete it
    // after adding to session.

    // Analogous logic for condensed phase reaction studies as gas phase
    // except Metadynamics tasks can be chained to each other so don't assume
    // they all share the same parent solvate
    ResourceDescriptor::APPLICATIONTYPE at =
      ResourceUtils::stringToApplicationType(rt->getApplicationType());
    DavCalculation *solvate = 0;
    bool isSolvate = (at == ResourceDescriptor::AT_SOLVATE);
    bool isMetadyn = (at == ResourceDescriptor::AT_METADYNAMICS);
    vector<Resource *> tasks = session->getMembers();
    for (int i = 0; i < tasks.size(); i++) {
      ResourceType *resType = tasks[i]->getDescriptor();
      ResourceDescriptor::APPLICATIONTYPE appType =
        ResourceUtils::stringToApplicationType(resType->getApplicationType());
      if (appType == ResourceDescriptor::AT_SOLVATE) {
        solvate = dynamic_cast<DavCalculation*>(tasks[i]);
        break;
      }
    }

    if ((isSolvate) && (solvate != 0)) {
      throw InvalidException("Only one Solvate task allowed per condensed "
                             "phase reaction study", WHERE);
    }
        
    if ((isMetadyn) && (solvate == 0)) {
      throw InvalidException("Solvate task must be added to condensed phase "
                   "reaction study before adding a Metadynamics task", WHERE);
    }

    if (isSolvate) {
      // Solvate task is first session member so no target specified
      session->addMember(task);
      session->saveLinkbase();
    }
  }
}


/**
 * Returns PropertyTask object for url.
 *
 * @deprecated a PropertyTask is a resource, and we should be limiting callers
 *             to only retrieve the base Resource class
 * @param url location of the PropertyTask to retrieve
 * @return the dynamically cast resource to a property task
 */
PropertyTask *EDSIFactory::getPropertyTask(const EcceURL& url)
{
   return dynamic_cast<PropertyTask*>(getResource(url));
}


/**
 * Not really sure how this is used anymore.
 * @deprecated old code?
 */
ITraversable* EDSIFactory::getITraversable(const EcceURL& url)
{
  return new EDSITraversable(url);
}


void EDSIFactory::dumpPool()
{
  PoolIter poolIter;
  cerr << "\n\nPoolStart:\n";
  for (poolIter = p_pool.begin(); poolIter != p_pool.end(); poolIter++)
    cerr << '\t' << poolIter->first << endl;
  cerr << "PoolEnd:\n\n";
}

/**
 * @file
 *
 *
 */

#include <unistd.h>  // access()

#include <iostream>
  using std::cout;
  using std::endl;

#include <string>
  using std::string;

#include <vector>
  using std::vector;

#include <map>
  using std::map;
  using std::multimap;

#include <utility>
  using std::pair;
  using std::make_pair;


#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/util/XMLString.hpp>
  using namespace xercesc;

#include "util/Ecce.H"
#include "util/ParseException.H"
#include "util/InvalidException.H"
#include "util/ResourceUtils.H"
#include "util/StringConverter.H"
#include "util/SFile.H"
#include "util/StringTokenizer.H"

#include "dsm/ITraversable.H"
#include "dsm/VDoc.H"
#include "dsm/ResourceType.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/PropertyView.H"
#include "dsm/AtomView.H"
#include "dsm/ToolPropertyView.H"
#include "dsm/BasicDOMParser.H"



/**
 * Initialize p_singleton to a 0 pointer
 */
ResourceDescriptor *ResourceDescriptor::p_singleton = 0;


/**
 * Constructor
 */
ResourceDescriptor::ResourceDescriptor()
{
  p_doc = 0;
  try {
    parseResourceDescriptorFile();
    loadMdUnits();
    createTools();
    createRmTasks();
    createPropertyViews();
    createToolPropertyViews();
    createAtomViews();
    createResidueViews();
    createResourceTypes();
    loadResourceTypes();
  }
  catch (...) {
    //cout << "Unable to load Resource Descriptor File" << endl;
  }
}


/** 
 * Destructor
 *
 * @todo create a destroy method
*
 */
ResourceDescriptor::~ResourceDescriptor()
{
}


/**
 * Returns reference to the ResourceDescriptor singleton
 */
ResourceDescriptor& ResourceDescriptor::getResourceDescriptor()
{
  if (p_singleton == 0) {
    p_singleton = new ResourceDescriptor();
  }

  return *p_singleton;
}


/**
 * Determines if the indicated resource type is a document.
 */
bool ResourceDescriptor::isDocument(const string& resourceType)
{
  RESOURCETYPE rt =
    ResourceUtils::stringToResourceType(resourceType);

  return (rt == RT_DOCUMENT);
}


/**
 * Determines if the indicated resource type is a collection.
 */
bool ResourceDescriptor::isCollection(const string& resourceType)
{
  RESOURCETYPE rt =
    ResourceUtils::stringToResourceType(resourceType);

  return (rt == RT_COLLECTION);
}


/**
 * Determines if the indicated resource type is a virtual document.
 */
bool ResourceDescriptor::isVirtualDocument(const string& resourceType)
{
  RESOURCETYPE rt =
    ResourceUtils::stringToResourceType(resourceType);

  return (rt == RT_VIRTUAL_DOCUMENT);
}


/**
 * Determines if the indicated content type is an ecce project.
 */
bool ResourceDescriptor::isEcceProject(const string& contentType)
{
  CONTENTTYPE ct =
    ResourceUtils::stringToContentType(contentType);

  return (ct == CT_PROJECT);
}


/**
 * Determines if the indicated content type is an ecce session.
 */
bool ResourceDescriptor::isEcceSession(const string& contentType)
{
  CONTENTTYPE ct =
    ResourceUtils::stringToContentType(contentType);

  return (ct == CT_SESSION);
}


/**
 * Determines if the indicated content type is an ecce calculation.
 */
bool ResourceDescriptor::isEcceCalculation(const string& contentType)
{
  CONTENTTYPE ct =
    ResourceUtils::stringToContentType(contentType);

  return (ct == CT_CALCULATION);
}


/**
 * Determines if the indicated content type is an MD Prepare.
 */
bool ResourceDescriptor::isEcceMdPrepare(const string& contentType)
{
  CONTENTTYPE ct =
    ResourceUtils::stringToContentType(contentType);

  return (ct == CT_MDPREPARE);
}


/**
 * Determines if the indicated content type is an MD Optimize.
 */
bool ResourceDescriptor::isEcceMdOptimize(const string& contentType)
{
  CONTENTTYPE ct =
    ResourceUtils::stringToContentType(contentType);

  return (ct == CT_MDOPTIMIZE);
}


/**
 * Determines if the indicated content type is an MD Energy.
 */
bool ResourceDescriptor::isEcceMdEnergy(const string& contentType)
{
  CONTENTTYPE ct =
    ResourceUtils::stringToContentType(contentType);

  return (ct == CT_MDENERGY);
}


/**
 * Determines if the indicated content type is an MD Dynamics.
 */
bool ResourceDescriptor::isEcceMdDynamics(const string& contentType)
{
  CONTENTTYPE ct =
    ResourceUtils::stringToContentType(contentType);

  return (ct == CT_MDDYNAMICS);
}


/**
 * Returns string value for a resource type.
 */
string ResourceDescriptor::getRtName(const RESOURCETYPE& rt)
{
  return ResourceUtils::resourceTypeToString(rt);
}

/**
 * Returns string value for a content type.
 */
string ResourceDescriptor::getCtName(const CONTENTTYPE& ct)
{
  return ResourceUtils::contentTypeToString(ct);
}


/**
 * Returns string value for an application type.
 */
string ResourceDescriptor::getAtName(const APPLICATIONTYPE& at)
{
  return ResourceUtils::applicationTypeToString(at);
}


/**
 * Returns list of resource's loaded from the resource
 * descriptor file.
 */
vector<ResourceType*> ResourceDescriptor::getResourceTypes()
{
  return p_resources;
}


/**
 * Returns recource identified by the TNode.  The TNode's 
 * resource type, content type, and application type are
 * examined in order to determine the TNode's matching resource.
 * It may be the case that the content type contains
 * a MIME type - which matches an "external resource" 
 * described in the resource descriptor file.  
 * 
 * @param resource a TNode object
 *
 * @return returns a pointer to the resource associated 
 *         with the TNode object.  A 0 pointer is returned
 *         if a match cannot be made.  User is NOT 
 *         responsible for memory management.
 */
ResourceType *ResourceDescriptor::getResourceType(TNode *tnode) 
{
  ResourceType *ret = 0;

  // TNode's resource type
  ResourceDescriptor::RESOURCETYPE tnodeRt = 
    tnode->getResourceType();

  // TNode's content type
  ResourceDescriptor::CONTENTTYPE tnodeCt =
    ResourceUtils::stringToContentType(tnode->getContentType());
  // It may be the case that the content type contains
  // a MIME type (DAV:getcontenttype) - need to check for a 
  // match for these as well.  Convert to lower case.
  string tnodeMimeType; 
  StringConverter::toLower(tnode->getContentType(), tnodeMimeType);

  // TNode's application type
  string appProp = tnode->getProperty(VDoc::getEcceNamespace() + ":application");

  ResourceDescriptor::APPLICATIONTYPE tnodeAt =
    ResourceUtils::stringToApplicationType(appProp);

  // Try and find matching resource
  for (int i = 0; i < p_resources.size(); i++) {

    // Resource's resource type
    ResourceDescriptor::RESOURCETYPE rt = 
      ResourceUtils::stringToResourceType(p_resources[i]->getResourceType());

    // Resource's Content Type
    ResourceDescriptor::CONTENTTYPE ct =
      ResourceUtils::stringToContentType(p_resources[i]->getContentType());
    // It may be the case that the content type contains
    // a MIME type (DAV:getcontenttype) - need to check for a 
    // match for these as well.  Convert to lower case.
    string resourceMimeType; 
    StringConverter::toLower(p_resources[i]->getContentType(), resourceMimeType);

    // Resource's application type
    ResourceDescriptor::APPLICATIONTYPE at = 
      ResourceUtils::stringToApplicationType(p_resources[i]->getApplicationType());

    // Try and find matching resource
    if (tnodeRt == rt &&
        (tnodeCt == ct || tnodeMimeType == resourceMimeType) &&
        tnodeAt == at) {
      ret = p_resources[i];
      break;
    }
  }

  return ret;
}


/**
 * Returns a Resource's registry information.  This method does not account
 * for registered resources that have DAV:getcontenttype (ie MIME types) as
 * their content types.  It also does not account for elsectronic structure
 * codes whose code names are not represented as an enumerated type - should
 * use the string version of this method.
 * 
 * @param resourceRt the resource type
 * @param resourceCt the content type
 * @param resourceAt the application type
 *
 * @return returns a pointer to a ResourceType. A 0 pointer is returned
 *         if a match cannot be made.  User is NOT responsible 
 *         for memory management.
 */
ResourceType*
ResourceDescriptor::getResourceType(ResourceDescriptor::RESOURCETYPE resourceRt,
                                    ResourceDescriptor::CONTENTTYPE resourceCt,
                                    ResourceDescriptor::APPLICATIONTYPE resourceAt)
{
  ResourceType *ret = 0;

  string rt = ResourceUtils::resourceTypeToString(resourceRt);
  string ct = ResourceUtils::contentTypeToString(resourceCt);
  string at = ResourceUtils::applicationTypeToString(resourceAt);

  ret = getResourceType(rt, ct, at);

  return ret;
}


/**
 * Returns a Resource's registry information.  This method does account
 * for registered resources that have DAV:getcontenttype (ie MIME types) as
 * their content types.
 * 
 * @param resourceRt the resource type
 * @param resourceCt the content type
 * @param resourceAt the application type
 *
 * @return returns a pointer to a ResourceType. A 0 pointer is returned
 *         if a match cannot be made.  User is NOT responsible 
 *         for memory management.
 */
ResourceType *ResourceDescriptor::getResourceType(string resourceRt,
                                                  string resourceCt,
                                                  string resourceAt) 
{
  ResourceType *ret = 0;

  // Resource's resource type
  ResourceDescriptor::RESOURCETYPE rt = 
    ResourceUtils::stringToResourceType(resourceRt);

  // Resource's content type
  string ct = resourceCt;

  // Need to use a string for the application type, not
  // all application types are represented as an enumerated
  // type - for example all electronic structure codes.
  string at = resourceAt;

  ResourceDescriptor::RESOURCETYPE regRt;
  string regCt;
  string regAt;

  // Try and find matching resource
  for (int i = 0; i < p_resources.size(); i++) {

    // Registry's resource type
    regRt = ResourceUtils::stringToResourceType(p_resources[i]->
                                                getResourceType());

    // Registry's Content Type
    regCt = p_resources[i]->getContentType();

    // Registry's application type
    regAt = p_resources[i]->getApplicationType();

    // Try and find matching resource
    if ((regRt == rt) &&
        (regCt == ct) && 
        (regAt == at)) {
      ret = p_resources[i];
      break;
    }

  }

  // Did not find a match in the registry
  // Return a file resource by default
  if (ret == 0) {
    ret = getResourceType("document");
  }

  return ret;
}


/**
 * Returns list of resource objects defined by the 
 * resource descriptor file.
 */
vector<ResourceTool*> ResourceDescriptor::getTools()
{
  return p_tools;
}


/**
 * Returns the resource objects with gateway index equals pos
 */
ResourceTool * ResourceDescriptor::getGatewayTool(int pos)
{
  ResourceTool * ret = 0;
  vector<ResourceTool *>::iterator toolItor = p_tools.begin();
  for (; toolItor!= p_tools.end(); toolItor++)
    if ((*toolItor)->getGateway() == pos)
      ret = (*toolItor);

  return ret;
}


/**
 * Gets the resource tool identified by the supplied tool name.
 *
 * @param toolName name of tool
 *
 * @return returns resource tool pointer identified by toolName.
 *         A 0 pointer is returned if a match cannot be made.
 */
ResourceTool *ResourceDescriptor::getTool(const string& toolName)
{
  ResourceTool *ret = 0;

  for (int i = 0; i < p_tools.size(); i++) {
    if (p_tools[i]->getName() == toolName) {
      ret = p_tools[i];
      break;
    }
  }

  return ret;
}


/**
 * Gets the resource tool identified by the supplied tool id.
 *
 * @param id id of tool
 *
 * @return returns resource tool pointer identified by id.
 *         A 0 pointer is returned if a match cannot be made.
 */
ResourceTool *ResourceDescriptor::getTool(int id)
{
  ResourceTool *ret = 0;

  id -= ID_TOOL_BASE;
  if (id >= 0 && id < p_tools.size()) {
    ret = p_tools[id];
  }

  return ret;
}


/**
 * Returns list of run management task objects defined by the 
 * resource descriptor file.
 */
vector<ResourceRmTask*> ResourceDescriptor::getRmTasks()
{
  return p_rmTasks;
}


/**
 * Returns list of of the "canned" property views defined by the 
 * resource descriptor file.
 */
multimap<int, PropertyView*> ResourceDescriptor::getPropertyViews()
{
  return p_propertyViews;
}


/**
 * Returns list of of the tool property views defined by the 
 * resource descriptor file.
 */
multimap<int, ToolPropertyView*> ResourceDescriptor::getToolPropertyViews()
{
  return p_toolPropertyViews;
}


/**
 * Returns ToolPropertyView based on the string name given.
 */
ToolPropertyView *ResourceDescriptor::getToolPropertyView(const string& tool,
                                                      const string& contentType)
{
  ToolPropertyView *ret = 0;

  multimap<int, ToolPropertyView*>::iterator mapIt;
  for (mapIt = p_toolPropertyViews.begin();
       mapIt != p_toolPropertyViews.end();
       mapIt++) {
    if (mapIt->second->getTool() == tool && 
        mapIt->second->getContentType() == contentType) {
      ret = mapIt->second;
      break;
    }
  }
  return ret;
}


/**
 * Returns list of tools enabled by the given resource, in order that they
 * should appear (by integer), along with their corresponding property view
 * (a ToolPropertyView).
 */
multimap<int, pair<ResourceTool*,ToolPropertyView*> >
ResourceDescriptor::getToolPropertyViews(ResourceType* resource)
{
  DOMElement *resourceElement = getResourceElement(resource->getName());

  vector<ResourceTool*> tools = getResourceEnabledTools(resourceElement);
  vector<ResourceTool*>::iterator toolIt;

  ToolPropertyView *tpv;

  multimap<int, pair<ResourceTool*,ToolPropertyView*> > ret;
  pair<ResourceTool*,ToolPropertyView*> pairRT_TPV;

  for (toolIt = tools.begin(); toolIt != tools.end(); toolIt++) {
    tpv = getToolPropertyView((*toolIt)->getName(),resource->getContentType());
    pairRT_TPV = make_pair((*toolIt),tpv);
    ret.insert(make_pair(tpv->getOrder(), pairRT_TPV));
  }

  return ret;
}


/**
 * Gets the property view identified by the supplied name.
 *
 * @param pvName name of the property view
 *
 * @return returns PropertyView pointer identified by pvName.
 *         A 0 pointer is returned if a match cannot be made.
 */
PropertyView *ResourceDescriptor::getPropertyView(const string& pvName)
{
  PropertyView *ret = 0;

  multimap<int, PropertyView*>::iterator mapIt;
  for (mapIt = p_propertyViews.begin();
       mapIt != p_propertyViews.end();
       mapIt++) {
    if (mapIt->second->getName() == pvName) {
      ret = mapIt->second;
    }
  }
  return ret;
}


/**
 * Gets the property view identified by the supplied task id.
 *
 * @param id id of property view
 *
 * @return returns PropertyView pointer identified by id.
 *         A 0 pointer is returned if a match cannot be made.
 */
PropertyView *ResourceDescriptor::getPropertyView(int id)
{
  PropertyView *ret = 0;
  multimap<int, PropertyView*>::iterator mapIt;
  for(mapIt = p_propertyViews.begin();
      mapIt != p_propertyViews.end();
      mapIt++) {
    PropertyView *pv = mapIt->second;
    if (pv->getId() == id) {
      ret = pv;
    }
  }

  return ret;
}


/**
 * Returns list of of the atom views defined in the 
 * resource descriptor file.
 */
multimap<int, AtomView*> ResourceDescriptor::getAtomViews()
{
  return p_atomViews;
}


/**
 * Gets the atom view identified by the supplied name.
 *
 * @param avName name of the atom view
 *
 * @return returns AtomView pointer identified by avName.
 *         A 0 pointer is returned if a match cannot be made.
 */
AtomView *ResourceDescriptor::getAtomView(const string& avName)
{
  AtomView *ret = 0;

  multimap<int, AtomView*>::iterator mapIt;
  for (mapIt = p_atomViews.begin();
       mapIt != p_atomViews.end();
       mapIt++) {
    if (mapIt->second->getName() == avName) {
      ret = mapIt->second;
    }
  }
  return ret;
}


/**
 * Gets the atom view identified by the supplied id.
 *
 * @param id id of atom view
 *
 * @return returns AtomView pointer identified by id.
 *         A 0 pointer is returned if a match cannot be made.
 */
AtomView *ResourceDescriptor::getAtomView(int id)
{
  AtomView *ret = 0;
  multimap<int, AtomView*>::iterator mapIt;
  for(mapIt = p_atomViews.begin();
      mapIt != p_atomViews.end();
      mapIt++) {
    AtomView *av = mapIt->second;
    if (av->getId() == id) {
      ret = av;
    }
  }

  return ret;
}


/**
 * Returns list of of the residue views defined in the 
 * resource descriptor file.
 */
multimap<int, AtomView*> ResourceDescriptor::getResidueViews()
{
  return p_residueViews;
}


/**
 * Gets the residue view identified by the supplied name.
 *
 * @param rvName name of the residue view
 *
 * @return returns AtomView pointer identified by rvName.
 *         A 0 pointer is returned if a match cannot be made.
 */
AtomView *ResourceDescriptor::getResidueView(const string& rvName)
{
  AtomView *ret = 0;

  multimap<int, AtomView*>::iterator mapIt;
  for (mapIt = p_residueViews.begin();
       mapIt != p_residueViews.end();
       mapIt++) {
    if (mapIt->second->getName() == rvName) {
      ret = mapIt->second;
    }
  }
  return ret;
}


/**
 * Gets the residue view identified by the supplied id.
 *
 * @param id id of residue view
 *
 * @return returns AtomView pointer identified by id.
 *         A 0 pointer is returned if a match cannot be made.
 */
AtomView *ResourceDescriptor::getResidueView(int id)
{
  AtomView *ret = 0;
  multimap<int, AtomView*>::iterator mapIt;
  for(mapIt = p_residueViews.begin();
      mapIt != p_residueViews.end();
      mapIt++) {
    AtomView *rv = mapIt->second;
    if (rv->getId() == id) {
      ret = rv;
    }
  }

  return ret;
}


/**
 * Returns list of molecular Dynamics units defined by the 
 * resource descriptor file.
 */
vector<string> ResourceDescriptor::getMdUnits()
{
  return p_mdUnits;
}


/**
 * Parse resource descriptor file.
 * 
 * @throw ParseException indicates parsing failed
 *
 */
void ResourceDescriptor::parseResourceDescriptorFile() throw (ParseException)
{
  string file = Ecce::ecceDataPath();
  file += "/client/config/ResourceDescriptor";
  string ext = ".xml";

  string path = Ecce::ecceHome();
  path += "/";
  path += getenv("ECCE_SYSDIR");
  path += "bin/dirdyed";

  if (!access(path.c_str(), F_OK)) {
    // hardwire resource descriptor if one of the reaction rate exes exists
    ext = "Rxn.xml"; 
  }
  file += ext;

  BasicDOMParser parser;
  p_doc = parser.parse(SFile(file));
}


/**
 * Creates resource objects and saves pointers in the p_resources data 
 * member.  Only identifying data is loaded.
 */
void ResourceDescriptor::createResourceTypes()
{
  // Get <Resource> element list
  XMLCh *tmpStr = XMLString::transcode("ResourceType");
  DOMNodeList *resources = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numResources = 0;
  if (resources != 0) {
    numResources = resources->getLength();
  }


  DOMElement *element = 0;
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;

  // Create resource objects 
  // Load identifying attribute data, this includes
  //   - Name
  //   - Resource Type
  //   - Content Type
  //   - Application Type
  for (int i = 0; i < numResources; i++) {

    string name = "";
    string resourceType = "";
    string contentType = "";
    string applicationType = "";

    // Resource element
    element = (DOMElement*)resources->item(i);

    // Get attributes 
    attributes = element->getAttributes();

    // Get name attribute value
    tmpStr = XMLString::transcode("name");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      name = cptr;
      delete [] cptr;
    }

    // Get resource type attribute value
    tmpStr = XMLString::transcode("resourceType");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      resourceType = cptr;
      delete [] cptr;
    }

    // Get content type attribute value
    tmpStr = XMLString::transcode("contentType");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      contentType = cptr;
      delete [] cptr;
    }

    // Get application type attribute value
    tmpStr = XMLString::transcode("applicationType");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      applicationType = cptr;
      delete [] cptr;
    }

    ResourceType *resource = new ResourceType;
    resource->setName(name);
    resource->setId(i + ID_RESOURCE_BASE);
    resource->setResourceType(resourceType);
    resource->setContentType(contentType);
    resource->setApplicationType(applicationType);
    p_resources.push_back(resource);
  }
}


/**
 * Populate resource objects with data from resource descriptor
 * file.
 */
void ResourceDescriptor::loadResourceTypes()
{
  for (int i = 0; i < p_resources.size(); i++) {
    loadResourceType(p_resources[i]);
  }
}


/**
 * Populate resource object with data from resource
 * descriptor file.
 */
void ResourceDescriptor::loadResourceType(ResourceType *resource)
{
  vector<string> tokens;
  StringTokenizer tokenizer(resource->getContentType());

  // Get content types - could be more then one
  // for mime types sharing a resource registration
  tokens = tokenizer.tokenize(",");

  /*
  cout << "Tokens for :: " << resource->getName() << endl;
  cout << "---------------------------------------" << endl;
  for (int i = 0; i < tokens.size(); i++) { 
    cout << "[" << i << "] : " << tokens[i] << endl;
  }
  cout << "---------------------------------------" << endl;
  cout << endl;
  */

  // Need to account for multiple resource types
  // sharing a single registration 
  if (tokens.size() > 1) {
    for (int i = 0; i < tokens.size(); i++) {
      // For first one, just change in place
      // the existing resource type
      if (i == 0) {
        resource->setContentType(tokens[i]);
      }
      // For the remaining, just add a
      // new resource type to the 
      // end of the list
      else {
        ResourceType *res = new ResourceType();
        (*res) = (*resource);
        res->setContentType(tokens[i]);
        p_resources.push_back(res);
      }
    }
  }

  // Load resource

  DOMElement *resourceElement =
    getResourceElement(resource->getName());

  resource->setSortPriority(getResourcePriority(resourceElement));
  resource->setIcons(getResourceIcons(resourceElement));
  resource->setLabel(getResourceLabel(resourceElement));
  resource->setNameRule(getResourceNameRule(resourceElement));
  resource->setRootTask(getResourceRootTask(resourceElement));
  resource->setStateBased(getResourceStateBased(resourceElement));
  resource->setMenuItem(getResourceMenuItem(resourceElement));
  resource->setDescriptorFile(getResourceDescriptorFile(resourceElement));
  resource->setFactoryCategory(getResourceFactoryCategory(resourceElement));
  resource->setIsExpandable(getResourceIsExpandable(resourceElement));
  resource->setContextPanel(getResourceContextPanel(resourceElement));
  resource->setIsSessionBased(getResourceIsSessionBased(resourceElement));
  resource->setContains(getResourceContains(resourceElement));
  resource->setContainedBy(getResourceContainedBy(resourceElement));
  if (resource->getStateBased()) {
    resource->setDefaultTools(getResourceStateDefaultTools(resourceElement));
    resource->setTools(getResourceStateEnabledTools(resourceElement));
    resource->setRmTasks(getResourceStateEnabledRmTasks(resourceElement));
  }
  else { 
    resource->setDefaultTool(getResourceDefaultTool(resourceElement));
    resource->setTools(getResourceEnabledTools(resourceElement));
    resource->setRmTasks(getResourceEnabledRmTasks(resourceElement));
  }
}


/**
 * Gets the resource identified by the supplied resource name.
 *
 * @param resourceName name of resource
 *
 * @return returns resource pointer identified by resourceName.
 *         A 0 pointer is returned if a match cannot be made.
 */
ResourceType *ResourceDescriptor::getResourceType(const string& resourceName)
{
  ResourceType *ret = 0;

  for (int i = 0; i < p_resources.size(); i++) {
    if (p_resources[i]->getName() == resourceName) {
      ret = p_resources[i];
      break;
    }
  }

  return ret;
}


/**
 * Gets the resource identified by the supplied resource id.
 *
 * @param id id of resource
 *
 * @return returns resource pointer identified by id.
 *         A 0 pointer is returned if a match cannot be made.
 */
ResourceType *ResourceDescriptor::getResourceType(int id)
{
  ResourceType *ret = 0;
  
  id -= ID_RESOURCE_BASE;
  if (id >= 0 && id < p_resources.size()) {
    ret = p_resources[id];
  }

  return ret;
}


/**
 * Gets DOMElement for resource identified by the supplied resource name.
 *
 * @param resourceName name of resource
 *
 * @return returns a DOMElement pointer for the resource.  A 0 pointer
 *         is returned if the resource element could not be found.
 */
DOMElement *ResourceDescriptor::getResourceElement(const string& resourceName)
{
  DOMElement *ret = 0;

  // Get <Resource> element list
  XMLCh *tmpStr = XMLString::transcode("ResourceType");
  DOMNodeList *resources = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numResources = 0;
  if (resources != 0) {
    numResources = resources->getLength();
  }

  DOMElement *element = 0;
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;

  for (int i = 0; i < numResources; i++) {

    // Name attribute
    string name = "";

    // Resource element
    element = (DOMElement*)resources->item(i);

    // Get attributes 
    attributes = element->getAttributes();

    // Get name attribute value
    tmpStr = XMLString::transcode("name");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      name = cptr;
      delete [] cptr;
    }

    if (name == resourceName) {
      ret = element;
      break;
    }
  }

  return ret;
}


/**
 * Get value of sort priority for a resource.
 * Returns lowest possible priority if SortPriority element not found.
 */
int ResourceDescriptor::getResourcePriority(DOMElement *resourceElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("SortPriority");
  DOMNodeList *nodes = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0; 
  if (nodes != 0) {
    numNodes = nodes->getLength();
  }

  if (numNodes > 0) {
    DOMElement* ele = (DOMElement*)nodes->item(0);
    if (ele->hasChildNodes()) {
      DOMNode *child = ele->getFirstChild();
      char *value = XMLString::transcode(child->getNodeValue());
      ret = value;
      delete [] value;
    }
  }

  int retInt = ResourceType::LOWEST_PRIORITY;
  if (ret != "")
    StringConverter::toInt(ret, retInt);

  return retInt;
}


/** 
 * Get list of icons that this resource contains.
 */
map<ResourceDescriptor::IconType, string> 
ResourceDescriptor::getResourceIcons(DOMElement *resourceElement)
{
  map<ResourceDescriptor::IconType, string> ret;
  string iconType;
  string iconName;

  // Get icons element
  XMLCh *tmpStr = XMLString::transcode("Icons");
  DOMNodeList *nodes = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  if (nodes != 0) {

    DOMNodeList *icons = ((DOMElement*)nodes->item(0))->getChildNodes();

    if (icons != 0) {

      for (int i = 0; i < icons->getLength(); i++) {
       
        if (icons->item(i)->getNodeType() == DOMNode::ELEMENT_NODE) {

          DOMElement* ele = (DOMElement*)icons->item(i);

          iconType = "";
          iconName = "";

          // Determine icon type - based on element name
          char *eleName = XMLString::transcode(ele->getNodeName());
          string iconType = eleName;
          delete [] eleName;
          
          // Determine icon name - value of element
          if (ele->hasChildNodes()) {
            DOMNode *child = ele->getFirstChild();
            char *value = XMLString::transcode(child->getNodeValue());
            iconName = value;
            delete [] value;
          }

          // Add to map
          ResourceDescriptor::IconType icon = 
            ResourceUtils::stringToIconType(iconType);
          if (icon != ResourceDescriptor::IT_UNDEFINED) {
            ret.insert(make_pair(icon, iconName));
          }
        }
      }
    }
  }

  return ret;
}


/**
 * Get value of label for a resource.  Returns an empty 
 * string if Label element not found.
 */
string ResourceDescriptor::getResourceLabel(DOMElement *resourceElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("Label");
  DOMNodeList *nodes = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0; 
  if (nodes != 0) {
    numNodes = nodes->getLength();
  }

  if (numNodes > 0) {
    DOMElement* ele = (DOMElement*)nodes->item(0);
    if (ele->hasChildNodes()) {
      DOMNode *child = ele->getFirstChild();
      char *value = XMLString::transcode(child->getNodeValue());
      ret = value;
      delete [] value;
    }
  }
  return ret;
}


/**
 * Get value of name rule for a resource.  Returns an empty 
 * string if NameRule element not found.
 */
string ResourceDescriptor::getResourceNameRule(DOMElement *resourceElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("NameRule");
  DOMNodeList *nodes = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0; 
  if (nodes != 0) {
    numNodes = nodes->getLength();
  }

  if (numNodes > 0) {
    DOMElement* ele = (DOMElement*)nodes->item(0);
    if (ele->hasChildNodes()) {
      DOMNode *child = ele->getFirstChild();
      char *value = XMLString::transcode(child->getNodeValue());
      ret = value;
      delete [] value;
    }
  }
  return ret;
}


/**
 * Get value indicating if the resource is can be the root task.  False is 
 * returned if RootTask element cannot be found.  The value "true" indicates
 * the resource can be the root task (ie first task in a study), any other 
 * value is interpreted as false.
 */
bool ResourceDescriptor::getResourceRootTask(DOMElement *resourceElement)
{
  bool ret = false;
  string retStr = "";

  XMLCh *tmpStr = XMLString::transcode("RootTask");
  DOMNodeList *nodes = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0; 
  if (nodes != 0) {
    numNodes = nodes->getLength();
  }

  if (numNodes > 0) {
    DOMElement* ele = (DOMElement*)nodes->item(0);
    if (ele->hasChildNodes()) {
      DOMNode *child = ele->getFirstChild();
      char *value = XMLString::transcode(child->getNodeValue());
      retStr = value;
      delete [] value;
    }
  }

  if (retStr == "true") {
    ret = true;
  }

  return ret;
}


/**
 * Get value indicating if the resource is state based.  False is 
 * returned if StateBased element not found.  The value "true" indicates
 * the resource is state based, any other value is interpreted as
 * false.
 */
bool ResourceDescriptor::getResourceStateBased(DOMElement *resourceElement)
{
  bool ret = false;
  string retStr = "";

  XMLCh *tmpStr = XMLString::transcode("StateBased");
  DOMNodeList *nodes = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0; 
  if (nodes != 0) {
    numNodes = nodes->getLength();
  }

  if (numNodes > 0) {
    DOMElement* ele = (DOMElement*)nodes->item(0);
    if (ele->hasChildNodes()) {
      DOMNode *child = ele->getFirstChild();
      char *value = XMLString::transcode(child->getNodeValue());
      retStr = value;
      delete [] value;
    }
  }

  if (retStr == "true") {
    ret = true;
  }

  return ret;
}


/**
 * Get value of menu item for a resource.  Returns an empty 
 * string if MenuItem element not found.
 */
string ResourceDescriptor::getResourceMenuItem(DOMElement *resourceElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("MenuItem");
  DOMNodeList *nodes = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0; 
  if (nodes != 0) {
    numNodes = nodes->getLength();
  }

  if (numNodes > 0) {
    DOMElement* ele = (DOMElement*)nodes->item(0);
    if (ele->hasChildNodes()) {
      DOMNode *child = ele->getFirstChild();
      char *value = XMLString::transcode(child->getNodeValue());
      ret = value;
      delete [] value;
    }
  }
  return ret;
}


/**
 * Get value of descriptor file for a resource.  Returns an empty 
 * string if DescriptorFile element not found.
 */
string ResourceDescriptor::getResourceDescriptorFile(DOMElement *resourceElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("DescriptorFile");
  DOMNodeList *nodes = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0; 
  if (nodes != 0) {
    numNodes = nodes->getLength();
  }

  if (numNodes > 0) {
    DOMElement* ele = (DOMElement*)nodes->item(0);
    if (ele->hasChildNodes()) {
      DOMNode *child = ele->getFirstChild();
      char *value = XMLString::transcode(child->getNodeValue());
      ret = value;
      delete [] value;
    }
  }
  return ret;
}


/**
 * Get value of factory category.  Returns an empty 
 * string if FactoryCategory element not found.
 */
string ResourceDescriptor::getResourceFactoryCategory(DOMElement *resourceElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("FactoryCategory");
  DOMNodeList *nodes = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0; 
  if (nodes != 0) {
    numNodes = nodes->getLength();
  }

  if (numNodes > 0) {
    DOMElement* ele = (DOMElement*)nodes->item(0);
    if (ele->hasChildNodes()) {
      DOMNode *child = ele->getFirstChild();
      char *value = XMLString::transcode(child->getNodeValue());
      ret = value;
      delete [] value;
    }
  }
  return ret;
}


/**
 * Get value of IsExpandable element for a resource.  Returns false 
 * if IsExpandable element not found.
 */
bool ResourceDescriptor::getResourceIsExpandable(DOMElement *resourceElement)
{
  bool ret = false;
  string elementVal = "";

  XMLCh *tmpStr = XMLString::transcode("IsExpandable");
  DOMNodeList *nodes = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0; 
  if (nodes != 0) {
    numNodes = nodes->getLength();
  }

  if (numNodes > 0) {
    DOMElement* ele = (DOMElement*)nodes->item(0);
    if (ele->hasChildNodes()) {
      DOMNode *child = ele->getFirstChild();
      char *value = XMLString::transcode(child->getNodeValue());
      elementVal = value;
      delete [] value;
    }
  }

  if (elementVal == "true") {
    ret = true;
  }

  return ret;
}


/**
 * Get value of context panel for a resource.  Returns an empty 
 * string if ContextPanel element not found.
 */
string ResourceDescriptor::getResourceContextPanel(DOMElement *resourceElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("ContextPanel");
  DOMNodeList *nodes = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0; 
  if (nodes != 0) {
    numNodes = nodes->getLength();
  }

  if (numNodes > 0) {
    DOMElement* ele = (DOMElement*)nodes->item(0);
    if (ele->hasChildNodes()) {
      DOMNode *child = ele->getFirstChild();
      char *value = XMLString::transcode(child->getNodeValue());
      ret = value;
      delete [] value;
    }
  }
  return ret;
}


/**
 * Get value of IsSessionBased element for a resource.  Returns false 
 * if IsSessionBased element not found.
 */
bool ResourceDescriptor::getResourceIsSessionBased(DOMElement *resourceElement)
{
  bool ret = false;
  string elementVal = "";

  XMLCh *tmpStr = XMLString::transcode("IsSessionBased");
  DOMNodeList *nodes = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0; 
  if (nodes != 0) {
    numNodes = nodes->getLength();
  }

  if (numNodes > 0) {
    DOMElement* ele = (DOMElement*)nodes->item(0);
    if (ele->hasChildNodes()) {
      DOMNode *child = ele->getFirstChild();
      char *value = XMLString::transcode(child->getNodeValue());
      elementVal = value;
      delete [] value;
    }
  }

  if (elementVal == "true") {
    ret = true;
  }

  return ret;
}


/** 
 * Get list of resources that this resource can contain.
 */
vector<ResourceType*> ResourceDescriptor::getResourceContains(DOMElement *resourceElement)
{
  vector<ResourceType*> ret;

  // Get <Contains> Element
  XMLCh *tmpStr = XMLString::transcode("Contains");
  DOMNodeList *nodeList = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <Resource> elements
  DOMElement *containsElement = (DOMElement*)nodeList->item(0);
  if (containsElement != 0) {
    tmpStr = XMLString::transcode("ContainsResource");
    DOMNodeList *resources = containsElement->getElementsByTagName(tmpStr);
    delete [] tmpStr;

    int numResources = 0;
    if (resources != 0) {
      numResources = resources->getLength();
    }

    DOMElement *element = 0;
    DOMNamedNodeMap *attributes = 0;
    DOMNode *attribute = 0;
    char *cptr = (char*)0;

    // Get resources
    for (int i = 0; i < numResources; i++) {

      string name = "";

      // Tool element
      element = (DOMElement*)resources->item(i);

      // Get attributes 
      attributes = element->getAttributes();

      // Get name attribute value
      tmpStr = XMLString::transcode("name");
      attribute = attributes->getNamedItem(tmpStr);
      delete [] tmpStr;
      if (attribute != 0) {
        cptr = XMLString::transcode(attribute->getNodeValue());
        name = cptr;
        delete [] cptr;
      }

      // Add resource to return vector
      ResourceType *resource = getResourceType(name);
      if (resource != 0) {
        ret.push_back(resource);
      }
    }
  }

  return ret;
}


/**
 * Get list of resources that this resource can be contained by.
 */
vector<ResourceType*> ResourceDescriptor::getResourceContainedBy(DOMElement *resourceElement)
{
  vector<ResourceType*> ret;

  // Get <Contains> Element
  XMLCh *tmpStr = XMLString::transcode("ContainedBy");
  DOMNodeList *nodeList = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <Resource> elements
  DOMElement *containsElement = (DOMElement*)nodeList->item(0);
  if (containsElement != 0) {
    tmpStr = XMLString::transcode("ContainedByResource");
    DOMNodeList *resources = containsElement->getElementsByTagName(tmpStr);
    delete [] tmpStr;

    int numResources = 0;
    if (resources != 0) {
      numResources = resources->getLength();
    }

    DOMElement *element = 0;
    DOMNamedNodeMap *attributes = 0;
    DOMNode *attribute = 0;
    char *cptr = (char*)0;

    // Get resources
    for (int i = 0; i < numResources; i++) {

      string name = "";

      // Tool element
      element = (DOMElement*)resources->item(i);

      // Get attributes 
      attributes = element->getAttributes();

      // Get name attribute value
      tmpStr = XMLString::transcode("name");
      attribute = attributes->getNamedItem(tmpStr);
      delete [] tmpStr;
      if (attribute != 0) {
        cptr = XMLString::transcode(attribute->getNodeValue());
        name = cptr;
        delete [] cptr;
      }

      // Add resource to return vector
      ResourceType *resource = getResourceType(name);
      if (resource != 0) {
        ret.push_back(resource);
      }
    }
  }

  return ret;
}


/**
 * Gets a key,value list, where key is the run state and value is a resource tool.
 * The list represents this resource's default tool for a specific
 * run state.
 */
map<ResourceDescriptor::RUNSTATE, ResourceTool*>
ResourceDescriptor::getResourceStateDefaultTools(DOMElement *resourceElement)
{
  map<ResourceDescriptor::RUNSTATE, ResourceTool*> ret;

  // Get <DefaultTools> Element
  XMLCh *tmpStr = XMLString::transcode("DefaultTools");
  DOMNodeList *nodeList = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <Tool> elements
  DOMElement *dtElement = (DOMElement*)nodeList->item(0);
  tmpStr = XMLString::transcode("Tool");
  DOMNodeList *tools = dtElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numTools = 0;
  if (tools != 0) {
    numTools = tools->getLength();
  }

  DOMElement *element = 0;
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;

  // Get tools
  for (int i = 0; i < numTools; i++) {

    string name = "";

    // Tool element
    element = (DOMElement*)tools->item(i);

    // Get attributes 
    attributes = element->getAttributes();

    // Get name attribute value
    tmpStr = XMLString::transcode("name");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      name = cptr;
      delete [] cptr;
    }

    // Get ResourceTool pointer
    ResourceTool *tool = getTool(name);

    // Get <State> elements
    XMLCh *tmpStr = XMLString::transcode("State");
    DOMNodeList *states = element->getElementsByTagName(tmpStr);
    delete [] tmpStr;

    int numStates = 0;
    if (states != 0) {
      numStates = states->getLength();
    }

    // Get States
    if (tool != 0) {
      for (int j = 0; j < numStates; j++) {

        // Get State 
        string stateName = getStateName((DOMElement*)states->item(j));
        ResourceDescriptor::RUNSTATE rt = 
          ResourceUtils::stringToState(stateName);

        // Add State/Tool (key/val) pair to map
        if (rt != ResourceDescriptor::STATE_ILLEGAL) {
          ret[rt] = tool;
        }
      }
    }
  }

  return ret;
}


/**
 * Gets the default tool for a non state-based resource.  A 0 pointer
 * is returned is a default tool cannot be found.
 */
ResourceTool *
ResourceDescriptor::getResourceDefaultTool(DOMElement *resourceElement)
{
  ResourceTool* ret = 0;

  // Get <DefaultTools> Element
  XMLCh *tmpStr = XMLString::transcode("DefaultTools");
  DOMNodeList *nodeList = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <Tool> elements
  DOMElement *dtElement = (DOMElement*)nodeList->item(0);
  tmpStr = XMLString::transcode("Tool");
  DOMNodeList *tools = dtElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numTools = 0;
  if (tools != 0) {
    numTools = tools->getLength();
  }

  DOMElement *element = 0;
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;

  if (numTools > 0) {

    string name = "";

    // Tool element
    element = (DOMElement*)tools->item(0);

    // Get name attribute 
    attributes = element->getAttributes();

    // Get name attribute value
    tmpStr = XMLString::transcode("name");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      name = cptr;
      delete [] cptr;
    }

    // Get ResourceTool pointer
    ret = getTool(name);

  }

  return ret;
}


/**
 * Gets a key,value list, where key is the run state and value is a resource tool.
 * The list represents this resource's enabled/available tools for a specific
 * run state.
 */
multimap<ResourceDescriptor::RUNSTATE, ResourceTool*>
ResourceDescriptor::getResourceStateEnabledTools(DOMElement *resourceElement)
{
  multimap<ResourceDescriptor::RUNSTATE, ResourceTool*> ret;

  // Get <EnabledTools> Element
  // Resource descriptor file should have a single
  //  <EnabledTools> element for each resource element
  XMLCh *tmpStr = XMLString::transcode("EnabledTools");
  DOMNodeList *nodeList = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <Tool> elements
  DOMElement *etElement = (DOMElement*)nodeList->item(0);
  tmpStr = XMLString::transcode("Tool");
  DOMNodeList *tools = etElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numTools = 0;
  if (tools != 0) {
    numTools = tools->getLength();
  }

  DOMElement *element = 0;
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;

  // Get tools
  for (int i = 0; i < numTools; i++) {

    string name = "";

    // Tool element
    element = (DOMElement*)tools->item(i);

    // Get attributes 
    attributes = element->getAttributes();

    // Get name attribute value
    tmpStr = XMLString::transcode("name");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      name = cptr;
      delete [] cptr;
    }

    // Get ResourceTool pointer
    ResourceTool *tool = getTool(name);

    // Get <State> elements
    XMLCh *tmpStr = XMLString::transcode("State");
    DOMNodeList *states = element->getElementsByTagName(tmpStr);
    delete [] tmpStr;

    int numStates = 0;
    if (states != 0) {
      numStates = states->getLength();
    }

    // Get States
    if (tool != 0) {
      for (int j = 0; j < numStates; j++) {

        // Get State 
        string stateName = getStateName((DOMElement*)states->item(j));
        ResourceDescriptor::RUNSTATE rs = 
          ResourceUtils::stringToState(stateName);

        // Add State/Tool (key/val) pair to map
        if (rs != ResourceDescriptor::STATE_ILLEGAL) {
          ret.insert(make_pair(rs, tool));
        }
      }
    }
  }

  return ret;

}


/**
 * Gets list of the enabled tools for a non state-based resource.
 */
vector<ResourceTool*>
ResourceDescriptor::getResourceEnabledTools(DOMElement *resourceElement)
{
  vector<ResourceTool*> ret;

  // Get <EnabledTools> Element
  // Resource descriptor file should have a single
  //  <EnabledTools> element for each resource element
  XMLCh *tmpStr = XMLString::transcode("EnabledTools");
  DOMNodeList *nodeList = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <Tool> elements
  DOMElement *etElement = (DOMElement*)nodeList->item(0);
  tmpStr = XMLString::transcode("Tool");
  DOMNodeList *tools = etElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numTools = 0;
  if (tools != 0) {
    numTools = tools->getLength();
  }

  DOMElement *element = 0;
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;

  // Get tools
  for (int i = 0; i < numTools; i++) {

    string name = "";

    // Tool element
    element = (DOMElement*)tools->item(i);

    // Get attributes 
    attributes = element->getAttributes();

    // Get name attribute value
    tmpStr = XMLString::transcode("name");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      name = cptr;
      delete [] cptr;
    }

    // Get ResourceTool pointer
    ret.push_back(getTool(name));

  }

  return ret;
}


/**
 * Gets a key,value list, where key is the run state and value is a resource tool.
 * The list represents this resource's enabled/available tools for a specific
 * run state.
 */
multimap<ResourceDescriptor::RUNSTATE, ResourceRmTask*>
ResourceDescriptor::getResourceStateEnabledRmTasks(DOMElement *resourceElement)
{
  multimap<ResourceDescriptor::RUNSTATE, ResourceRmTask*> ret;

  // Get <EnabledRunManagementTasks> Element
  // Resource descriptor file should have a single
  //  <EnabledRunManagementTasks> element for each resource element
  XMLCh *tmpStr = XMLString::transcode("EnabledRunManagementTasks");
  DOMNodeList *nodeList = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <RunManagementTask> elements
  DOMElement *rmtElement = (DOMElement*)nodeList->item(0);
  tmpStr = XMLString::transcode("RunManagementTask");
  DOMNodeList *tasks = rmtElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numTasks = 0;
  if (tasks != 0) {
    numTasks = tasks->getLength();
  }

  DOMElement *element = 0;
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;

  // Get tasks
  for (int i = 0; i < numTasks; i++) {

    string name = "";

    // Tool element
    element = (DOMElement*)tasks->item(i);

    // Get attributes 
    attributes = element->getAttributes();

    // Get name attribute value
    tmpStr = XMLString::transcode("name");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      name = cptr;
      delete [] cptr;
    }

    // Get ResourceTool pointer
    ResourceRmTask *task = getRmTask(name);

    // Get <State> elements
    XMLCh *tmpStr = XMLString::transcode("State");
    DOMNodeList *states = element->getElementsByTagName(tmpStr);
    delete [] tmpStr;

    int numStates = 0;
    if (states != 0) {
      numStates = states->getLength();
    }

    // Get States
    if (task != 0) {
      for (int j = 0; j < numStates; j++) {

        // Get State 
        string stateName = getStateName((DOMElement*)states->item(j));
        ResourceDescriptor::RUNSTATE rs = 
          ResourceUtils::stringToState(stateName);

        // Add State/Tool (key/val) pair to map
        if (rs != ResourceDescriptor::STATE_ILLEGAL) {
          ret.insert(make_pair(rs, task));
        }
      }
    }
  }

  return ret;
}


/**
 * Gets list of enabled run management tasks for a non state-based resource.
 */
vector<ResourceRmTask*>
ResourceDescriptor::getResourceEnabledRmTasks(DOMElement *resourceElement)
{
  vector<ResourceRmTask*> ret;

  // Get <EnabledRunManagementTasks> Element
  // Resource descriptor file should have a single
  //  <EnabledRunManagementTasks> element for each resource element
  XMLCh *tmpStr = XMLString::transcode("EnabledRunManagementTasks");
  DOMNodeList *nodeList = resourceElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <RunManagementTask> elements
  DOMElement *rmtElement = (DOMElement*)nodeList->item(0);
  tmpStr = XMLString::transcode("RunManagementTask");
  DOMNodeList *tasks = rmtElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numTasks = 0;
  if (tasks != 0) {
    numTasks = tasks->getLength();
  }

  DOMElement *element = 0;
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;

  // Get tasks
  for (int i = 0; i < numTasks; i++) {

    string name = "";

    // Tool element
    element = (DOMElement*)tasks->item(i);

    // Get attributes 
    attributes = element->getAttributes();

    // Get name attribute value
    tmpStr = XMLString::transcode("name");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      name = cptr;
      delete [] cptr;
    }

    // Get ResourceTool pointer
    ret.push_back(getRmTask(name));

  }

  return ret;
}


/** 
 * Returns the value of the name attribute for a State element.
 */
string ResourceDescriptor::getStateName(DOMElement *stateElement)
{
  string ret = "";
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;

  if (stateElement != 0) {
    // Get attributes 
    attributes = stateElement->getAttributes();

    // Get name attribute value
    XMLCh *tmpStr = XMLString::transcode("name");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      ret = cptr;
      delete [] cptr;
    }
  }
  return ret;
}


/**
 * Creates ResourceTool objects and saves pointers in the p_tools data 
 * member.  Only identifying data is loaded.
 */
void ResourceDescriptor::createTools()
{
  // Get <Tools> Element
  // Resource descriptor file should have a single
  //  <Tools> element
  XMLCh *tmpStr = XMLString::transcode("Tools");
  DOMNodeList *toolList = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <Tool> elements
  DOMElement *toolsElement = (DOMElement*)toolList->item(0);
  tmpStr = XMLString::transcode("Tool");
  DOMNodeList *tools = toolsElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numTools = 0;
  if (tools != 0) {
    numTools = tools->getLength();
  }

  // Create tool objects 
  // Load identifying attribute data, this includes
  //   - Name
  DOMElement *element = 0;
  for (int i = 0; i < numTools; i++) {
    element = (DOMElement*)tools->item(i);
    ResourceTool *tool = new ResourceTool(element);
    tool->setId(i + ID_TOOL_BASE);
    p_tools.push_back(tool);
  }
}


/**
 * Creates ResourceRmTask objects and saves pointers in the p_rmTasks data 
 * member.  Only identifying data is loaded.
 */
void ResourceDescriptor::createRmTasks()
{
  // Get <RunManagementTasks> Element
  // Resource descriptor file should have a single
  //  <RunManagementTasks> element
  XMLCh *tmpStr = XMLString::transcode("RunManagementTasks");
  DOMNodeList *rmTaskList = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <RunManagementTask> elements
  DOMElement *rmTasksElement = (DOMElement*)rmTaskList->item(0);
  tmpStr = XMLString::transcode("RunManagementTask");
  DOMNodeList *rmTasks = rmTasksElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numRmTasks = 0;
  if (rmTasks != 0) {
    numRmTasks = rmTasks->getLength();
  }
  
  // Create Run Management Task objects 
  DOMElement *element = 0;
  for (int i = 0; i < numRmTasks; i++) {
    element = (DOMElement*)rmTasks->item(i);
    ResourceRmTask *rmTask = new ResourceRmTask(element);
    rmTask->setId(i + ID_RMTASK_BASE);
    p_rmTasks.push_back(rmTask);
  }

}


/**
 * Gets the run management task identified by the supplied task name.
 *
 * @param rmTaskName name of run management task
 *
 * @return returns ResourceRmTask pointer identified by taskName.
 *         A 0 pointer is returned if a match cannot be made.
 */
ResourceRmTask *ResourceDescriptor::getRmTask(const string& rmTaskName)
{
  ResourceRmTask *ret = 0;

  for (int i = 0; i < p_rmTasks.size(); i++) {
    if (p_rmTasks[i]->getName() == rmTaskName) {
      ret = p_rmTasks[i];
      break;
    }
  }

  return ret;
}


/**
 * Gets the run management task identified by the supplied task id.
 *
 * @param id id of run management task
 *
 * @return returns ResourceRmTask pointer identified by id.
 *         A 0 pointer is returned if a match cannot be made.
 */
ResourceRmTask *ResourceDescriptor::getRmTask(int id)
{
  ResourceRmTask *ret = 0;

  id -= ID_RMTASK_BASE;  
  if (id >= 0 && id < p_rmTasks.size()) {
    ret = p_rmTasks[id];
  }

  return ret;
}


/**
 * Creates PropertyView objects and saves pointers in the p_propertyViews data 
 * member.  Only identifying data is loaded.
 */
void ResourceDescriptor::createPropertyViews()
{
  // Get <PropertyViews> Element
  // Resource descriptor file should have a single
  //  <PropertyViews> element
  XMLCh *tmpStr = XMLString::transcode("PropertyViews");
  DOMNodeList *propertyViewList = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <PropertyView> elements
  DOMElement *propertyViewsElement = (DOMElement*)propertyViewList->item(0);
  tmpStr = XMLString::transcode("PropertyView");
  DOMNodeList *propertyViews = propertyViewsElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numPropertyViews = 0;
  if (propertyViews != 0) {
    numPropertyViews = propertyViews->getLength();
  }
  
  // Create PropertyView objects 
  DOMElement *element = 0;
  for (int i = 0; i < numPropertyViews; i++) {
    element = (DOMElement*)propertyViews->item(i);
    PropertyView *propertyView = new PropertyView(element);
    propertyView->setId(i + ID_PROPERTY_VIEW_BASE);
    p_propertyViews.insert(make_pair(propertyView->getOrder(), propertyView));
  }
}


/**
 * Creates ToolPropertyView objects and saves pointers in the 
 * p_toolPropertyViews data member.  Only identifying data is loaded.
 */
void ResourceDescriptor::createToolPropertyViews()
{
  // Get <ToolPropertyViews> Element
  // Resource descriptor file should have a single
  //  <ToolPropertyViews> element
  XMLCh *tmpStr = XMLString::transcode("ToolPropertyViews");
  DOMNodeList *toolPropertyViewList = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <ToolPropertyView> elements
  DOMElement *toolPropertyViewsElement = 
      (DOMElement*)toolPropertyViewList->item(0);
  tmpStr = XMLString::transcode("ToolPropertyView");
  DOMNodeList *toolPropertyViews = 
      toolPropertyViewsElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numToolPropertyViews = 0;
  if (toolPropertyViews != 0) {
    numToolPropertyViews = toolPropertyViews->getLength();
  }

  // Create ToolPropertyView objects
  DOMElement *element = 0;
  for (int i = 0; i < numToolPropertyViews; i++) {
    element = (DOMElement*)toolPropertyViews->item(i);
    ToolPropertyView *toolPropertyView = new ToolPropertyView(element);
    p_toolPropertyViews.insert(make_pair(toolPropertyView->getOrder(), 
                                         toolPropertyView));
  }
}


/**
 * Creates AtomView objects and saves pointers in the p_atomViews data 
 * member.  Only identifying data is loaded.
 */
void ResourceDescriptor::createAtomViews()
{
  // Get <AtomViews> Element
  // Resource descriptor file should have a single
  //  <AtomViews> element
  XMLCh *tmpStr = XMLString::transcode("AtomViews");
  DOMNodeList *atomViewList = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <AtomView> elements
  DOMElement *atomViewsElement = (DOMElement*)atomViewList->item(0);
  tmpStr = XMLString::transcode("AtomView");
  DOMNodeList *atomViews = atomViewsElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numAtomViews = 0;
  if (atomViews != 0) {
    numAtomViews = atomViews->getLength();
  }
  
  // Create AtomView objects 
  DOMElement *element = 0;
  for (int i = 0; i < numAtomViews; i++) {
    element = (DOMElement*)atomViews->item(i);
    AtomView *atomView = new AtomView(element);
    atomView->setId(i + ID_ATOM_VIEW_BASE);
    p_atomViews.insert(make_pair(atomView->getOrder(), atomView));
  }
}


/**
 * Creates AtomView objects representing the residue views, and saves 
 * pointers in the p_residueViews data member.  Only identifying data is loaded.
 */
void ResourceDescriptor::createResidueViews()
{
  // Get <AtomViews> Element
  // Resource descriptor file should have a single
  //  <ResidueViews> element
  XMLCh *tmpStr = XMLString::transcode("ResidueViews");
  DOMNodeList *atomViewList = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <ResidueView> elements
  DOMElement *atomViewsElement = (DOMElement*)atomViewList->item(0);
  tmpStr = XMLString::transcode("ResidueView");
  DOMNodeList *atomViews = atomViewsElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numAtomViews = 0;
  if (atomViews != 0) {
    numAtomViews = atomViews->getLength();
  }
  
  // Create AtomView objects 
  DOMElement *element = 0;
  for (int i = 0; i < numAtomViews; i++) {
    element = (DOMElement*)atomViews->item(i);
    AtomView *atomView = new AtomView(element);
    atomView->setId(i + ID_RESIDUE_VIEW_BASE);
    p_residueViews.insert(make_pair(atomView->getOrder(), atomView));
  }
}

/**
 * Populate molecular dynamics units list with data  
 * from resource descriptor file.
 */
void ResourceDescriptor::loadMdUnits()
{
  // Get <MDUnits> Element
  // Resource descriptor file should have a single
  //  <MDUnits> element for each resource element
  XMLCh *tmpStr = XMLString::transcode("MDUnits");
  DOMNodeList *nodeList = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  // Get <MDUnit> elements
  DOMElement *mduElement = (DOMElement*)nodeList->item(0);
  tmpStr = XMLString::transcode("MDUnit");
  DOMNodeList *units = mduElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numUnits = 0;
  if (units != 0) {
    numUnits = units->getLength();
  }

  // Get units
  for (int i = 0; i < numUnits; i++) {

    // MDUnit element
    DOMElement *unitElement = (DOMElement*)units->item(i);
    if (unitElement->hasChildNodes()) {
      DOMNode *child = unitElement->getFirstChild();
      char *value = XMLString::transcode(child->getNodeValue());
      string unitVal = value;
      delete [] value;
      if (!unitVal.empty()) {
        p_mdUnits.push_back(unitVal);
      }
    }
  }
}


void ResourceDescriptor::getResourceIdRange(int & low, int & high)
{
  low = ID_RESOURCE_BASE;
  high = ID_RESOURCE_BASE + p_resources.size();
}


void ResourceDescriptor::getToolIdRange(int & low, int & high)
{
  low = ID_TOOL_BASE;
  high = ID_TOOL_BASE + p_tools.size();
}


void ResourceDescriptor::getRmTaskIdRange(int & low, int & high)
{
  low = ID_RMTASK_BASE;
  high = ID_RMTASK_BASE + p_rmTasks.size();
}


void ResourceDescriptor::getPropertyViewIdRange(int & low, int & high)
{
  low = ID_PROPERTY_VIEW_BASE;
  high = ID_PROPERTY_VIEW_BASE + p_propertyViews.size();
}


void ResourceDescriptor::getAtomViewIdRange(int & low, int & high)
{
  low = ID_ATOM_VIEW_BASE;
  high = ID_ATOM_VIEW_BASE + p_atomViews.size();
}


void ResourceDescriptor::getResidueViewIdRange(int & low, int & high)
{
  low = ID_RESIDUE_VIEW_BASE;
  high = ID_RESIDUE_VIEW_BASE + p_residueViews.size();
}



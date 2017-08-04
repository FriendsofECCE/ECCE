/**
 * @file 
 *
 *
 */

#include <sstream>
using std::istringstream;
using std::ostringstream;

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <stdexcept>
#include <string>
#include <vector>
using std::string;
using std::vector;
using std::pair;

#include "util/StringConverter.H"
#include "util/Serializable.H"
#include "util/Serializer.H"
#include "util/InvalidException.H"
#include "util/EcceRegexp.H"
#include "util/EcceMap.H"

#include "dsm/NWChemMDModel.H"
#include "dsm/Resource.H"
#include "dsm/MdTask.H"
#include "dsm/ResourceType.H"
#include "dsm/VDoc.H"
#include "dsm/Session.H"
#include "dsm/SessionXMLizer.H"
#include "dsm/DirDyVTSTTask.H"


/**
 * Link class constructor
 */
Session::Link::Link(Resource *source, 
                    Resource *target, 
                    LinkType linkType,
                    string linkName)
{
  this->source = source;
  this->target = target;
  this->linkType = linkType;
  this->linkName = linkName;
}


Session::Link::~Link()
{
}


/**
 * Constructor
 *
 * Load children, load members
 */
Session::Session(const EcceURL & url)
{
  p_url = url;
  p_edsi = 0;
  p_linkbaseLoaded = false;
}


/**
 * Destructor
 *
 * Can't save model here since the session may no longer exist
 */
Session::~Session()
{
}


/*
 * Load the linkbase file and fill in member list and link list.
 *
 * The linkbase is loaded on demand since it is a time consuming process.
 * Before linkbase is loaded, make sure children are loaded first.
 */
void Session::loadLinkbase()
  throw(DavException, ParseException)
{
  p_linkbaseLoaded = true;
  
  p_members.clear();
  p_links.clear();
  
  // First load children here
  getChildren();

  EcceMap props;
  ostringstream os;

  // Set base URL for model, get Linkbase, and perform deserialization
  getEDSI()->setURL(getLinkbaseId());
  if (getEDSI()->exists()) {
    if (getEDSI()->getDataSet(os)) {
      Serializer *ser = new SessionXMLizer();
      try {
        ser->deserialize(os.str(), *this, props);
      }
      catch (ParseException&) {
        delete ser;
        throw;
      }
      delete ser;
    } 
    else {
      throw DavException(getEDSI()->m_msgStack.getMessage(), WHERE);
    }
  }
}


/*
 * Save the linkbase file
 *
 * The linkbase should be saved whenever the member list and link list are changed
 */
void Session::saveLinkbase()
  throw(DavException)
{
  // Set URL for Linkbase and perform serialization
  getEDSI()->setURL(getLinkbaseId());
  if (numMembers() > 0) {
    Serializer *ser = new SessionXMLizer();
    string data;
    EcceMap props;
    ser->serialize(*this, data, props);
    delete ser;
    istringstream is(data);
    if (!getEDSI()->putDataSet(is)) {
      throw DavException(getEDSI()->m_msgStack.getMessage(), WHERE);
    }
  }
  else {
    // Erase the Linkbase file, if it exists
    if (getEDSI()->exists()) {
      if (!getEDSI()->removeResource()) {
        throw DavException(getEDSI()->m_msgStack.getMessage(), WHERE);
      }
    }
  }
}


/*
 * Get child vector.
 *
 * The only thing specialy here is to hide linkbase.xml file if not developer.
 * @todo Consider reload linkbase based on the flag.
 */
vector<Resource*> * Session::getChildren(bool refresh)
{
  vector<Resource*> *ret;

  ret = Resource::getChildren(refresh);

  // Hide the linkbase.xml file if the developer flag is not set
  vector<Resource*>::iterator resItor = ret->begin();
  if (!getenv("ECCE_DEVELOPER")) {
    for (;resItor != ret->end(); resItor++) {
      if ((*resItor)->getName() == "linkbase.xml") {
        ret->erase(resItor);
        break;
      }
    }
  }

  // update member list
  //  if (refresh) {
  //  }

  return ret;
}


/**
 * Creates a new link.
 *
 * A link consists of a source resource, a target resource, a link type
 * and a linkName. Resources being linked must already exist in the member
 * list, resources are added to the member list by using the addMember()
 * operation.  
 *
 * @param source pointer to source resource
 * @param target pointer to target resource
 * @param linkType the type of link, cannot be an UNSPECIFIED_LINKTYPE
 * @param linkName the name of the link
 * @throw InvalidException an exception is thrown if the link
 *                         cannot be created
 */
void Session::addLink(Resource * source,
                      Resource * target, 
                      LinkType linkType,
                      string linkName) throw(InvalidException)
{
  if (!p_linkbaseLoaded)
    loadLinkbase();

  Link * newLink = new Link(source, target, linkType, linkName);
  
  // Make sure source resource exists in member list
  if (!hasMember(source)) {
    throw InvalidException("Source resource does not exist", WHERE);
  }

  // Make sure target resource exists in member list
  if (!hasMember(target)) {
    throw InvalidException("Target resource does not exist", WHERE);
  }

  // Make sure it is a valid linkType
  if (linkType == UNSPECIFIED_LINKTYPE) {
    throw InvalidException("Must specify a link type", WHERE);
  }

  // Make sure it is a valid linkName
  if (!isValidLinkName(linkName)) {
    throw InvalidException("Not a valid link name", WHERE);
  }

  // Make sure the link does not already exist
  if (hasLink(source, target, linkType, linkName)) {
    throw InvalidException("Link has already been created", WHERE);
  }

  // Make sure the link does not create a cycle
  if (containsCycle(newLink)) {
    throw InvalidException("Cannot create link because it will create "
                           "a cycle in the graph", WHERE);
  }

  // Make sure the target resource does not already participate in another
  // link as a target for the link type and link name (i.e. each resource
  // can only have one parent in order to maintain the tree structure.
  if (getSources(target, linkType, linkName).size() > 0) {
    throw InvalidException("Each Resource can have at most one source",
                           WHERE);
  }

  // Determine if link is allowed for the link type

  // No longer needed, not sure if we need to keep.
  // INPUT_PROVIDER
  //   if (linkType == INPUT_PROVIDER) {
  //     if (!target->canModifyInputProvider()) {
  //       throw InvalidException("Target task cannot be altered to accept "
  //                              "input from source task", WHERE);
  //     }
  //   }
  // PARENT_VERSION
  // PRIMARY_PATH


  // New link can be added
  p_links.push_back(newLink);

}


/**
 * Removes a link.
 *
 * The resources themselves are not removed, only their association. 
 * 
 * @param source resource to unlink from target resource
 * @param target resource to unlink from source resource
 * @param linkType the type of link, cannot be an UNSPECIFIED_LINKTYPE
 * @param linkName the name of the link
 * @throw InvalidException an exception is thrown if the link cannot 
 *                         be removed.
 */
void Session::removeLink(Resource * source,
                         Resource * target,
                         LinkType linkType,
                         string linkName) throw(InvalidException)
{
  if (!p_linkbaseLoaded)
    loadLinkbase();

  vector<Link *>::iterator linkIt  = p_links.begin();

  for (; linkIt != p_links.end(); linkIt++)
    if ((*linkIt)->source == source &&
        (*linkIt)->target == target &&
        (*linkIt)->linkType == linkType && 
        (*linkIt)->linkName == linkName)
      break;

  // Make sure the link exists
  if (linkIt == p_links.end()) {
    throw InvalidException("Link has not been created", WHERE);
  }

  // Determine if link removal is allowed for the link type

  // No longer needed, not sure if we need to keep.
  // INPUT_PROVIDER
  //   if (linkType == INPUT_PROVIDER) {
  //     if (!target->canModifyInputProvider()) {
  //       throw InvalidException("Cannot remove link, target task has "
  //                              "already applied input provided by the "
  //                              "source task", WHERE);
  //     }
  //   }
  // PARENT_VERSION
  // PRIMARY_PATH


  // The link can be removed
  p_links.erase(linkIt);
  
  // @todo delete the link obj here?
}


/**
 * Creates a new Branch. A link consists of a source resource, a target
 * resource, a link type, and a linkName. Resources being linked must
 * already exist in the member list, resources are added to the member
 * list by using the addMember() operation.  
 * Unlike the link operation, the branch requires that the target resource
 * not be a member of any existing link for the specified link type.
 * Branching allows the caller to indicate that the new branch
 * (i.e. the new link and connected links of the specified link
 * type - this may be a path up to the tree root) be the primary path.
 * Neither the new link or primary path will be created if a problem is
 * encountered.  
 *
 * @param source pointer to source resource
 * @param target pointer to target resource
 * @param linkType the type of link, cannot be an UNSPECIFIED_LINKTYPE
 * @param linkName the name of the link
 * @param isPrimaryPath indicates if the new branch should be
 *        the primary path (i.e. set of links with the link type
 *        of PRIMARY_PATH)
 * @throw InvalidException an exception is thrown if the link or the
 *                         PRIMARY_PATH cannot be created.  
 */
void Session::branch(Resource *source,
                     Resource *target,
                     LinkType linkType,
                     string linkName,
                     bool isPrimaryPath) throw(InvalidException)
{
  if (!p_linkbaseLoaded)
    loadLinkbase();

  // Used to restore original links if an exception is caught
  // Is this deep copy??
  vector<Link *> origLinks(p_links);

  // Make sure the target task has not been linked to other 
  // tasks.
  if ((getSources(target, linkType, linkName).size() > 0) || 
      (getTargets(target, linkType, linkName).size() > 0)) {
    throw InvalidException("Cannot branch to target, the target has "
                           "already been linked with another task", WHERE);
  }

  // Go ahead and create the link
  try {
    addLink(source, target, linkType, linkName);
  }
  catch (InvalidException& ex) {
    string reason = ex.what();
    throw InvalidException("Branch failed - " + reason, WHERE);
  }

  // Make branch the primary path
  try {
    if (isPrimaryPath) {
      // Remove existing primary path
      removeLinks(PRIMARY_PATH);
      // Add PRIMARY_PATH link for new link
      addLink(source, target, PRIMARY_PATH);

      // Traverse up the tree adding PRIMARY_PATH
      // links until the root task for the link type 
      // is found.
      Resource *targetRes = source;
      vector<Resource *> sourceRes = getSources(targetRes, linkType, linkName);
      while (sourceRes.size() > 0) {
        addLink(sourceRes[0], targetRes, PRIMARY_PATH);
        targetRes = sourceRes[0];
        sourceRes = getSources(targetRes, linkType, linkName);
      }
    }
  }
  catch (InvalidException& ex) {
    // Restore original links
    p_links = origLinks;
    string reason = ex.what();
    throw InvalidException("Branch failed - " + reason, WHERE);
  }

}


/**
 * Reorders the source and target members of a link - target 
 * becomes source and source becomes target.  Links that 
 * connect to source will connect to the target after the 
 * swap.  Likewise, links from the target will be 
 * connected to the source after the swap.  The swap will
 * only affect the links with the type of link defined by
 * linkType and linkName.
 * 
 * @param source pointer to source task
 * @param target pointer to target task
 * @param linkType type of link
 * @param linkName the name of the link
 * @throw InvalidException an exception is thrown if the 
 *                         swap cannot be performed
 */
void Session::swap(Resource * source, 
                   Resource * target,
                   LinkType linkType,
                   string linkName) throw(InvalidException)
{
  if (!p_linkbaseLoaded)
    loadLinkbase();

  Resource *tempResource = 0;
  
  // Link must exist
  vector<Link *>::iterator origLinkIt  = p_links.begin();

  for (; origLinkIt != p_links.end(); origLinkIt++)
    if ((*origLinkIt)->source == source &&
        (*origLinkIt)->target == target &&
        (*origLinkIt)->linkType == linkType && 
        (*origLinkIt)->linkName == linkName)
      break;

  if (origLinkIt == p_links.end()) {
    throw InvalidException("Link does not exist", WHERE);
  }

  // Determine if swap is allowed for the link type
  // INPUT_PROVIDER
  //   if (linkType == INPUT_PROVIDER) {
  //     if ( (!source->canModifyInputProvider()) ||
  //          (!target->canModifyInputProvider()) ) {
  //       throw InvalidException("Swap unallowed, because source and/or target " 
  //                              "tasks cannot be altered", WHERE);
  //     }
  //   }
  // PARENT_VERSION
  // PRIMARY_PATH

  // Begin - Swap
  // Modify links that connect to source - 
  // to connect to target
  vector<Link *>::iterator linkIt = p_links.begin();
  while (linkIt != p_links.end()) {
    if ( ((*linkIt)->target == source) &&
         ((*linkIt)->linkType == linkType) &&
         ((*linkIt)->linkName == linkName) ) {
      (*linkIt)->target = target;
    }
    linkIt++;
  }
        
  // Modify links from target to be from source
  linkIt = p_links.begin();
  while (linkIt != p_links.end()) {
    if ( ((*linkIt)->source == target) &&
         ((*linkIt)->linkType == linkType) &&
         ((*linkIt)->linkName == linkName) ) {
      (*linkIt)->source = source;
    }
    linkIt++;
  }

  // Reorder source & target
  tempResource = (*origLinkIt)->source;
  (*origLinkIt)->source = (*origLinkIt)->target;
  (*origLinkIt)->target = tempResource;
  // End - Swap
}


/**
 * Determines the source resources of a resource for the specified 
 * link type and link Name. That is, resources that participate in a
 * link as a source with the provided resource as its target.
 * The provided resource must have been previously added to the model.
 * If the link type is not specified (i.e. an UNSPECIFIED_LINKTYPE),
 * then all source resources will be returned without regard to the type 
 * and name of link, this is the default behavior.
 *
 * @param res pointer to resource for which source resources are determined.
 * @param linkType only include source resources that are linked
 *                    with linkType.
 * @param linkName only include source resources that are linked
 *                    with linkName.
 * @return vector of resource pointers sources.
 */
vector<Resource *> Session::getSources(const Resource * res,
                                       LinkType linkType,
                                       const string & linkName) const
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  vector<Resource*> ret;

  // Make sure task has been added to model
  if (hasMember(res)) {
    vector<Link *>::const_iterator linkIt = p_links.begin();

    if (linkType == UNSPECIFIED_LINKTYPE) {
      while (linkIt != p_links.end()) {
        if ((*linkIt)->target == res) {
          if (!hasMember(ret, (*linkIt)->source)) {
            ret.push_back((*linkIt)->source);
          }
        }
        linkIt++;
      }
    }
    else {
      while (linkIt != p_links.end()) {
        if (((*linkIt)->target == res) &&
            ((*linkIt)->linkType == linkType) &&
            ((*linkIt)->linkName == linkName)) {
          if (!hasMember(ret, (*linkIt)->source)) {
            ret.push_back((*linkIt)->source);
          }
        }
        linkIt++;
      }
    }
  }

  return ret;
}


/**
 * Determines target resources for a resource.  That is, resources that 
 * participate in a link as a target with the provided resource as its
 * source - for a specific link type and link name.
 * The provided resource must have been previously added to the model.
 * If the link type is not specified (i.e. an UNSPECIFIED_LINKTYPE),
 * then all target resources will be returned without regard to the type
 * of link or link name, this is the default behavior.
 *
 * @param res pointer to resource for which target resources are determined.
 * @param linkType only include target resources that are linked
 *                    with linkType.
 * @param linkName the link names to include in the search for targets.
 * @return vector of resource pointers to targets.
 */
vector<Resource *> Session::getTargets(const Resource * res,
                                       LinkType linkType,
                                       const string & linkName) const
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  vector<Resource *> ret;

  // Make sure resource has been added to model
  if (hasMember(res)) {
    vector<Link *>::const_iterator linkIt = p_links.begin();

    if (linkType == UNSPECIFIED_LINKTYPE) {
      while (linkIt != p_links.end()) {
        if ((*linkIt)->source == res) {
          if (!hasMember(ret, (*linkIt)->target)) {
            ret.push_back((*linkIt)->target);
          }
        }
        linkIt++;
      }
    }
    else {
      while (linkIt != p_links.end()) {
        // includes special STTR reaction study logic to recognize the
        // transition state as the "true" parent of the dirdyvtst task
        // while ignoring the other reactants and products as parents
        if (((*linkIt)->source == res) &&
            ((*linkIt)->linkType == linkType) &&
            (((*linkIt)->linkName == linkName) ||
             (linkType == Session::PRIMARY_PATH) ||
             ((*linkIt)->linkName == "Transition-State"))) {
          if (!hasMember(ret, (*linkIt)->target)) {
            ret.push_back((*linkIt)->target);
          }
        }
        linkIt++;
      }
    }
  }
  return ret;
}


vector<Resource *> Session::getReactionTargets(const Resource * res,
                                               LinkType linkType) const
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  vector<Resource *> ret;

  // Make sure resource has been added to model
  if (hasMember(res)) {
    vector<Link *>::const_iterator linkIt = p_links.begin();

    if (linkType == UNSPECIFIED_LINKTYPE) {
      while (linkIt != p_links.end()) {
        if ((*linkIt)->source == res) {
          if (!hasMember(ret, (*linkIt)->target)) {
            ret.push_back((*linkIt)->target);
          }
        }
        linkIt++;
      }
    }
    else {
      while (linkIt != p_links.end()) {
        if (((*linkIt)->source == res) &&
            ((*linkIt)->linkType == linkType)) {
          if (!hasMember(ret, (*linkIt)->target)) {
            ret.push_back((*linkIt)->target);
          }
        }
        linkIt++;
      }
    }
  }
  return ret;
}


/**
 * Returns set of resources being managed
 */
vector<Resource *> Session::getMembers() const
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  return p_members;
}


/**
 * Finds root resources for the session. That is, resources that either
 * do not participate in a link or only participate in links
 * for which they are a source. If the linkType is not specified 
 * (i.e. UNSPECIFIED_LINKTYPE) - then all root tasks will be returned
 * without regard to the link type or link name.
 *
 * @param linkType the types of links to include in the search
 *        for root resources.
 * @param linkName the link names to include in the search
 *        for root resources.
 * @return a vector of resource pointers to root resources.
 */
vector<Resource *> Session::getRootMembers(LinkType linkType,
                                           const string & linkName) const
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  vector<Resource *> ret(p_members);
  
  vector<Link *>::const_iterator linkIt = p_links.begin();

  for (; linkIt != p_links.end(); linkIt++) {
    if (linkType != UNSPECIFIED_LINKTYPE &&
        (linkType != (*linkIt)->linkType ||
         linkName != (*linkIt)->linkName))
      break;

    vector<Resource*>::iterator resIt;
    for (resIt = ret.begin(); resIt != ret.end(); resIt++)
      if ((*resIt) == (*linkIt)->target)
        break;
    
    if (resIt != ret.end())
      ret.erase(resIt);
  }

  /* PRODUCED ODD RESULTS
  vector<Resource *> ret(p_members);
  vector<Resource *>::iterator resIt;
  vector<Link *>::const_iterator linkIt = p_links.begin();
  for (linkIt = p_links.begin(); linkIt != p_links.end(); linkIt++) {
    if ((resIt = find(ret.begin(), ret.end(), (*linkIt)->target)) != ret.end())
    {
      ret.erase(resIt);
    }
    if (linkType != UNSPECIFIED_LINKTYPE
            && ((*linkIt)->linkType != linkType
                    || (*linkIt)->linkName != linkName)
            && ((resIt = find(ret.begin(), ret.end(), (*linkIt)->source)) !=
                    ret.end())) {
      ret.erase(resIt);
    }
  }
  */

  /* PRODUCED ODD RESULTS
  vector<Resource *> ret;
  set<Resource *> sources;
  set<Resource *> targets;

  vector<Link *>::const_iterator linkIt = p_links.begin();
  for (linkIt = p_links.begin(); linkIt != p_links.end(); linkIt++) {
    if (linkType == UNSPECIFIED_LINKTYPE) {
      sources.insert((*linkIt)->source);
      targets.insert((*linkIt)->target);
    } else {
      if ((*linkIt)->linkType == linkType && (*linkIt)->linkName == linkName) {
        sources.insert((*linkIt)->source);
        targets.insert((*linkIt)->target);
      }
    }
  }

  set<Resource *>::iterator sourceIt;
  set<Resource *>::iterator targetIt;
  for (sourceIt = sources.begin(); sourceIt != sources.end(); sourceIt++) {
    if ((targetIt = targets.find(*sourceIt)) != targets.end()) {
      targets.erase(targetIt);
    }
  }

  for (targetIt = targets.begin(); targetIt != targets.end(); targetIt++) {
    ret.push_back(*targetIt);
  }
  */

  return ret;
}


/**
 * Finds leaf resource for the model.  That is, resources that only
 * participate in a link for which they are a target.
 * If the linkType is not specified (i.e. UNSPECIFIED_LINKTYPE),
 * then all leaf resources will be returned without regard to 
 * the link type and link name.
 *
 * @param linkType the types of links to include in the search
 *        for leaf tasks.
 * @param linkName the link name to include in the search
 *        for leaf tasks.
 * @return a vector of task pointers to leaf tasks.
 */
vector<Resource *> Session::getLeafMembers(LinkType linkType,
                                           const string & linkName) const
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();
  
  /*
  // for each task, determing if it is a leaf task
  // pay no attention to the link type or link name
  vector<Link *>::const_iterator linkIt;
  for (linkIt = p_links.begin(); linkIt != p_links.end(); linkIt++) {
    Resource * source = (*linkIt)->source;
    vector<Resource*>::iterator resIt;
    for (resIt = ret.begin(); resIt != ret.end(); resIt++) {
      if ((*resIt) == source) {
        ret.erase(resIt);
        break;
      }
    }
  }

  if (linkType != UNSPECIFIED_LINKTYPE) {
    // we now have all leaf tasks, we just need to find the ones that were
    // linked to by the specified type of link or name
    for (linkIt = p_links.begin(); linkIt != p_links.end(); linkIt++) {
      vector<Resource *>::iterator resIt;
      for (resIt = ret.begin(); resIt != ret.end(); resIt++) {
        if ((*linkIt)->target == (*resIt)) {
          if ((*linkIt)->linkType != linkType || 
              (*linkIt)->linkName != linkName)
            ret.erase(resIt);
          break;
        }
      }
    }
  }
  */

  // TODO
  // this logic assumes that all paths, whether PRIMARY or INPUT_PROVIDER
  // are always from root to leaf, such that when we erase a "source" it is
  // guaranteed to be a "source" for all paths which may not always be the case
  vector<Resource *> leaves(p_members);
  vector<Resource *>::iterator resIt;
  vector<Link *>::const_iterator linkIt = p_links.begin();
  for (linkIt = p_links.begin(); linkIt != p_links.end(); linkIt++) {
    if ((resIt = find(leaves.begin(), leaves.end(), (*linkIt)->source)) !=
            leaves.end())
    {
      leaves.erase(resIt);
    }
  }
  // we now have all leaf tasks, we just need to find the ones that were
  // linked to by the specified type of link or name
  
  vector<Resource *> ret;
  if (linkType != UNSPECIFIED_LINKTYPE)
  {
    for (linkIt = p_links.begin(); linkIt != p_links.end(); linkIt++) {
      if ((resIt = find(leaves.begin(), leaves.end(), (*linkIt)->target)) !=
              leaves.end())
      {
        if ((*linkIt)->linkType == linkType && (*linkIt)->linkName == linkName)
        {
          ret.push_back(*resIt);
        }
      }
    }
  }
  else {
    ret = leaves;
  }

  return ret;
}


/**
 * Returns set of links being managed
 */
vector<Session::Link *> Session::getLinks() const
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  return p_links;
}


void Session::getDeleteMessage(bool & canDelete, string & message)
{
  canDelete = true;
  message = "Study \"" + getName() + "\" contains "
    + StringConverter::toString(numMembers()) + 
    " tasks.\n\nAre you sure you want to delete this study and its tasks?";
  //   cerr << getDescriptor()->getName() << endl;
  //   cerr << getResourceType() << endl;
  //   cerr << getContentType() << endl;
  //   cerr << message << endl;
}


/**
 * Number of members being managed by the task model.
 *
 * @return number of members
 */
int Session::numMembers() const
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  return p_members.size();
}


/**
 * Clear the member list and link list without delete the resource
 */
void Session::clearMembers()
{
  p_members.clear();
  p_links.clear();
}


/** 
 * Adds a new member to the Session.  Returns true if the
 * member was successfully added to the Session.
 */
bool Session::addMember(Resource * res)
{
  bool ret = false;

  if (res != 0) {
    if (res->addProp(VDoc::getDcQualifierNamespace() + ":taskIndex",
                     StringConverter::toString(numMembers()))) {
      p_members.push_back(res);
      ret = true;
    }
  }

  return ret;
}


/**
 * Removes a member and any associated links from Session.
 * Returns true if the task was successfully removed from the model.
 *
 * @param pointer to task to be removed
 */
bool Session::removeMember(Resource * res)
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  bool ret = false;

  vector<Resource*>::iterator resIt = p_members.begin();
  for (; resIt != p_members.end(); resIt++)
    if ((*resIt) == res)
      break;
  if (resIt != p_members.end()) {
    removeLinks(res);
    p_members.erase(resIt);
    ret = true;
  }

  return ret;
}


/**
 * Converts a string to a LinkType, an UNSPECIFIED_LINKTYPE
 * will be returned if the conversion fails.
 * 
 * @param string to convert
 * @return LinkType, UNSPECIFIED_LINKTYPE is returned if the
 *         conversion fails
 */
Session::LinkType Session::stringToLinkType(const string& linkType)
{
  LinkType ret = UNSPECIFIED_LINKTYPE;

  if (linkType == "input_provider") {
    ret = INPUT_PROVIDER;
  }
  if (linkType == "version") {
    ret = VERSION;
  }
  if (linkType == "primary_path") {
    ret = PRIMARY_PATH;
  }

  return ret;
}


/**
 * Converts a LinkType to a string, an empty string
 * is returned for an UNSPECIFIED_LINKTYPE.
 *
 * @param LinkType to convert
 * @return string, empty string is returned for
 *         an UNSPECIFIED_LINKTYPE
 */
string Session::linkTypeToString(LinkType linkType)
{
  string ret = "";

  switch (linkType) {
  case INPUT_PROVIDER:
    ret = "input_provider";
    break;
  case VERSION:
    ret = "version";
    break;
  case PRIMARY_PATH:
    ret = "primary_path";
    break;
  default:
    break;
  }

  return ret;
}


/**
 * Determined the number of links that have a specified
 * link type and link name.
 *
 * @param linkType the link types
 * @param linkName the link names, if not specified an empty string 
 *        will be used.
 * @return a count of the number of links for a specified 
 *         link type
 */
int Session::numLinks(LinkType linkType, string linkName)
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  int ret = 0;
  for (size_t i = 0; i < p_links.size(); i++) {
    if ((p_links[i]->linkType == linkType) &&
        (p_links[i]->linkName == linkName)) {
      ret++;
    }
  }
  return ret;
}


/**
 * Removes all links between a source and target resource.
 *
 * @param source pointer to source task
 * @param target pointer to target task
 */
void Session::removeLinks(Resource *source, Resource *target)
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  vector<Link *>::iterator linkIt = p_links.begin();
  while (linkIt != p_links.end()) {
    if ( ((*linkIt)->source == source) &&
         ((*linkIt)->target == target) ) {
      linkIt = p_links.erase(linkIt);
    }
    else {
      linkIt++;
    }
  }
}


/**
 * Removes links containing the provided resource with the 
 * specified link type and linkName.  If the link type is not 
 * specified (i.e. UNSPECIFIED_LINKTYPE) then all links
 * will be removed without regard to the type of link,
 * this is the default behavior.
 *
 * @param res pointer to the resource
 * @param linkType type of link to remove
 */ 
void Session::removeLinks(Resource * res,
                          LinkType linkType,
                          string linkName)
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  vector<Link *>::iterator linkIt = p_links.begin();

  if (linkType == UNSPECIFIED_LINKTYPE) {
    while (linkIt != p_links.end()) {
      if ((*linkIt)->source == res || (*linkIt)->target == res) {
        linkIt = p_links.erase(linkIt);
      }
      else {
        linkIt++;
      }
    }
  }
  else {
    while (linkIt != p_links.end()) {
      if (((*linkIt)->source == res || (*linkIt)->target == res) && 
          (*linkIt)->linkType == linkType &&
          (*linkIt)->linkName == linkName) { 
        linkIt = p_links.erase(linkIt);
      }
      else {
        linkIt++;
      }
    }
  }
}


/**
 * Removes all links having the specified link type.
 *
 * @param linkType type of links to remove
 */ 
void Session::removeLinks(LinkType linkType)
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  vector<Link *>::iterator linkIt = p_links.begin();
  while (linkIt != p_links.end()) {
    // includes special reaction study logic not to remove the primary path
    // links for the tasks going into the DirDyVTST tasks
    if ((*linkIt)->linkType == linkType &&
        (*linkIt)->target->getApplicationType() !=
                    ResourceDescriptor::AT_DRDVTST) {
      linkIt = p_links.erase(linkIt);
    }
    else {
      linkIt++;
    }
  }
}


/**
 * Determines if the introduction of a new link
 * to the model will create a cycle.
 *
 * @param newLink link to add to the model
 * @return returns true if the new link will introduce a 
 *         cycle in the graph
 */
bool Session::containsCycle(Link * newLink)
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  bool containsCycle = false;
  vector<Link *> origLinks = p_links;

  // Add new link and determine if cycle is introduced.
  // Its assumed that the values of the new link
  // are valid, the tasks exist in the model and the
  // link type is not UNSPECIFIED_LINKTYPE
  p_links.push_back(newLink);


  // Should be able to repeatedly remove links 
  // containing root tasks until their are no
  // longer any links.  If links still exist 
  // but there are no root tasks then a cycle
  // was introduced with the new link. 
  // A zero in-degree sort algorithm for a DAG.
  while (numLinks(newLink->linkType, newLink->linkName) > 0) {
    vector<Resource *> roots =
      getRootMembers(newLink->linkType, newLink->linkName);
    if (!roots.empty()) {
      for (size_t i = 0; i < roots.size(); i++) {
        removeLinks(roots[i], newLink->linkType, newLink->linkName);
      }
    }
    else {
      containsCycle = true;
      break;
    }
  }

  // Restore links
  p_links = origLinks;

  return containsCycle;
}


/**
 * Performs search for a resource in member list
 *
 * @param res pointer to resource to search for
 * @return returns a vector<Resource*> iterator (p_member), if search fails
 *         then an end() iterator value is returned.
 */
bool Session::hasMember(const Resource * res) const
{
  return hasMember(p_members, res);
}


/**
 * Performs search for a resource in any vector.
 *
 * @param resource vector to search in
 * @param resource pointer to search for
 * @return returns a vector<Resource*> iterator (p_member), if search fails
 *         then an end() iterator value is returned.
 */
bool Session::hasMember(vector<Resource*> resources,
                        const Resource * const res)
{
  /*
  vector<Resource*>::iterator resIt = resources.begin();
  for (; resIt != resources.end(); resIt++) {
    if ((*resIt) == res)
      break;
  }
  return resIt != resources.end();
  */
  return find(resources.begin(), resources.end(), res) != resources.end();
}


/**
 * Performs search for a link.  The link is identified by a 
 * source resource, target resource, link type, and link name.
 *
 * @param source pointer to source resource link contains
 * @param target pointer to target resource link contains
 * @param linkType type of link
 * @param linkName name of link
 * @return returns a vector<Link> iterator (p_links), if search fails
 *         then an end() iterator value is returned.
 */
bool Session::hasLink(Resource * source,
                      Resource * target,
                      LinkType linkType,
                      string linkName)
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  vector<Link *>::iterator linkIt  = p_links.begin();

  for (; linkIt != p_links.end(); linkIt++)
    if ((*linkIt)->source == source &&
        (*linkIt)->target == target &&
        (*linkIt)->linkType == linkType && 
        (*linkIt)->linkName == linkName)
      break;

  return linkIt != p_links.end();;
}


bool Session::isValidLinkName(const string& linkName)
{
  bool ret = false;
  EcceRegexp regExp("[A-Za-z0-9._-]*");

  int foundIdx;
  int length;
  string match = regExp.match(linkName, foundIdx, length);

  if (match == linkName) {
    ret = true;
  }

  return ret;
}


/**
 * Get URL for Linkbase
 * 
 * @return an EcceURL - address of Session's Linkbase resource
 *         which identifies the Tasks and their 
 *         relationships within the session.
 */
EcceURL Session::getLinkbaseId() const
{
  return p_url.getChild("linkbase.xml");
}


void Session::addMemberAsTarget(Resource * target, Resource * source)
  throw(InvalidException)
{
  if (!p_linkbaseLoaded)
    (const_cast<Session *>(this))->loadLinkbase();

  // If the source "task" is this session, then it won't be found in the model
  // since it is the model.  In any case, if the source task isn't found,
  // the new task will be appended to the end of the primary path
  if (source == 0) {
    // source is this session
    // target might need to be a root task
    if (numMembers() == 0) {
      if (target->getDescriptor()->getRootTask()) {
        addMember(target);
      }
      else {
        throw InvalidException("The task cannot be the first task created "
                               "within a study.", WHERE);
      }
    }
    else {
      if (numMembers() == 1) {
        // target is not a root task, but no links yet exist in the model
        source = p_members[0];
      }
      else {
        // source is leaf task of primary path
        source = getLeafMembers(PRIMARY_PATH)[0];
      }
      addMember(target);
      branch(source, target, INPUT_PROVIDER);
    }
  }
  else {
    addMember(target);
    branch(source, target, INPUT_PROVIDER);
  }

  saveLinkbase();

  // MD only logic:
  // Try to initialize model from previous task in study, if there is one.
  // Don't attempt if current task or input task is a prepare task.
  if (target->getApplicationType()==ResourceDescriptor::AT_MDSTUDY &&
      target->getContentType()!=ResourceDescriptor::CT_MDPREPARE) {
    MdTask *currentTask = dynamic_cast<MdTask*>(target);
    if (currentTask) {
      // Get input provider, if there is one
      MdTask *inputTask = dynamic_cast<MdTask*>(currentTask->getInputProvider(this));
      if (inputTask) {
        if (inputTask->getContentType() != ResourceDescriptor::CT_MDPREPARE) {
          NWChemMDModel inputModel(currentTask->getContentType());
          inputTask->getTaskModel(inputModel);
          currentTask->setTaskModel(&inputModel);
        }
      }
    }
  } else if (target->getApplicationType() ==
                     ResourceDescriptor::AT_METADYNAMICS) {
    DavCalculation *currentTask = dynamic_cast<DavCalculation*>(target);
    if (currentTask) {
      DavCalculation *inputTask = dynamic_cast<DavCalculation*>(currentTask->getInputProvider(this));
      if (inputTask) {
        if (inputTask->getApplicationType() ==
                       ResourceDescriptor::AT_METADYNAMICS) {
          Fragment *frag = inputTask->fragment();
          MetaPotentialModel metaModel(*frag);
          inputTask->getMetaPotentialModel(metaModel);
          currentTask->setMetaPotentialModel(&metaModel);

          QMMMModel qmmmModel(*frag);
          inputTask->getQMMMModel(qmmmModel);
          currentTask->setQMMMModel(&qmmmModel);

          // detail dialog settings
          currentTask->guiparams(inputTask->guiparams());

          // theory, runtype, multiplicity
          currentTask->theory(inputTask->theory());
          currentTask->runtype(inputTask->runtype());
          currentTask->spinMultiplicity(inputTask->spinMultiplicity());
        }
      }
    }
  }
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
Resource * Session::addChild(const EcceURL & childUrl) {
  getChildren(true);
  // Force reload linkbase
  loadLinkbase();
  return EDSIFactory::getResource(childUrl);
}


void Session::eraseChild(const EcceURL & childUrl) {
  getChildren(true);
  // Force reload linkbase
  loadLinkbase();
}


/**
 * Moves a Session to a different location on the Dav server.
 * Need to reload the member list to update isPartOf prop.
 *
 * @param target New location of Session.
 *
 * @return Returns true if resource was moved, otherwise fails.
 */
bool Session::move(EcceURL& target) {
  bool ret = Resource::move(target);
  if (ret)
    loadLinkbase();
  return ret;
}


/**
 * Create a new child in session.  Added this to force a 
 * refresh of the session before adding the new child.  This
 * is what the organizer uses, and was necessary to add the
 * refresh to avoid sync issues when more then on app
 * is working with the same session and the session's 
 * are maintained in their separate pools.
 *
 * @param name name of the new resource to create and add to session
 * @param childType pointer to the type of resource of child
 *
 * @return returns a pointer to the new child
 */
Resource *Session::createChild(string name, ResourceType *childType) {

  // Force a refresh of the session - other apps (e.g. nwdirdy editor) 
  // may have modified it.
  getChildren(true);
  loadLinkbase();

  // Call default implementation
  return Resource::createChild(name, childType);

}


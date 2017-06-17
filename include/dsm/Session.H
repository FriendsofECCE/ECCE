/**
 * @file 
 *
 */

#ifndef SESSION_HH
#define SESSION_HH


#include <stdexcept>
#include <string>
#include <vector>

using std::string;
using std::vector;
using std::pair;


#include "util/Serializable.H"
#include "util/InvalidException.H"
#include "dsm/Resource.H"
#include "dsm/EDSIFactory.H"


/** 
 * Class for managing a Session's resources and  
 * relationships that exist between them. Resource
 * relationships are represented as links.  Each
 * link is composed of a source resource, a target resource, 
 * the type of link, and the link name. The set 
 * of links for a specific link type and link name will 
 * define one or more trees, where a tree is a 
 * directed acyclic graph such that each 
 * node (i.e. resource) has at most one parent.
 */
class Session : public Resource, public Serializable
{
public:

  friend class EDSIFactory;

  /** 
   * Indicates the type of relationship
   * existing between resources.
   */
  enum LinkType {
    INPUT_PROVIDER,        /**< The source resource provides input to 
                              the target resource */
    VERSION,               /**< The target resource was derived from the
                              source resource */
    PRIMARY_PATH,          /**< A link that belongs to the primary
                              path of a graph. */
    UNSPECIFIED_LINKTYPE   /**< The type of link was not specified */
  };


  /**
   * Represents a Link.  Link navigation is
   * from the source resource to the target resource.
   * The LinkType along with the linkName 
   * identifies the relationship that exists 
   * between the source and target resources.  The 
   * linkName is not mandatory (i.e. it can be
   * an empty string).  However, the linkName
   * becomes necessary in order to distinguish 
   * multiple links with the same linkType between
   * the same source and target resource.  The linkName
   * is also necessary when a target resource has 
   * multiple source resources each connected with the 
   * same linkType, in order to avoid a resource having
   * more than one parent resource.
   */
  class Link {
  public:
    Link(Resource *source, Resource *target,
         LinkType linkType, string linkName);
    ~Link();
    Resource *source;
    Resource *target;
    LinkType linkType;
    string linkName;
  };


  virtual ~Session();

  virtual Resource * addChild(const EcceURL & childUrl);

  virtual void eraseChild(const EcceURL & childUrl);

  virtual bool move(EcceURL& target);

  virtual Resource * createChild(string name, ResourceType *childType);

  virtual EcceURL getLinkbaseId() const;

  virtual void saveLinkbase() throw(DavException);

  virtual vector<Resource *> * getChildren(bool refresh = false);

  virtual void addLink(Resource * source, Resource * target, 
                       LinkType linkType, string linkName = "")
    throw(InvalidException);
  
  virtual void removeLink(Resource * source, Resource * target, 
                  LinkType linkType, string linkName = "")
    throw(InvalidException);
  
  virtual void branch(Resource * source, Resource * target,
              LinkType linkType, string linkName = "",
              bool isPrimaryPath = true)
    throw(InvalidException);

  virtual void swap(Resource * source, Resource * target, 
            LinkType linkType, string linkName = "")
    throw(InvalidException);

  virtual bool addMember(Resource * res);

  virtual void addMemberAsTarget(Resource * target, Resource * source = 0)
    throw(InvalidException);
  
  virtual bool removeMember(Resource * res);
  
  virtual vector<Resource *> getSources(const Resource * res,
                                        LinkType= UNSPECIFIED_LINKTYPE,
                                        const string & linkName="") const;
  
  virtual vector<Resource *> getTargets(const Resource * res,
                                        LinkType = UNSPECIFIED_LINKTYPE,
                                        const string & linkName="") const;

  virtual vector<Resource *> getReactionTargets(const Resource * res,
                                         LinkType = UNSPECIFIED_LINKTYPE) const;
  
  virtual vector<Resource *> getMembers() const; 
  
  virtual int numMembers() const;

  virtual vector<Resource *> getRootMembers(LinkType = UNSPECIFIED_LINKTYPE,
                                            const string & linkName="") const;
  
  virtual vector<Resource *> getLeafMembers(LinkType = UNSPECIFIED_LINKTYPE,
                                            const string & linkName="") const;
  
  virtual vector<Link *> getLinks() const;
  
  
  virtual void getDeleteMessage(bool & canDelete, string & message);

  virtual void loadLinkbase() throw(DavException, ParseException);

  static LinkType stringToLinkType(const string & linkType);

  static string linkTypeToString(LinkType linkType);

  static bool hasMember(vector<Resource *> resources,
                        const Resource * res);


protected:

  Session(const EcceURL & url);

  void clearMembers();

  int numLinks(LinkType linkType, string linkName = "");

  void removeLinks(Resource * source, Resource * target);

  void removeLinks(Resource * res,
                   LinkType = UNSPECIFIED_LINKTYPE,
                   string linkName = "");

  void removeLinks(LinkType linkType);

  bool containsCycle(Link * newLink);

  bool hasMember(const Resource * const res) const;

  bool hasLink(Resource * source, Resource * target,
               LinkType linkType, string linkName = "");

  bool isValidLinkName(const string& linkName);
  
  EcceURL p_baseUrl;

  vector<Resource *> p_members;

  vector<Link *> p_links;

  bool p_linkbaseLoaded;
};


#endif 

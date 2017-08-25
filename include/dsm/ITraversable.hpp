//////////////////////////////////////////////////////////////////////////
// Desgin
//   Applications should not delete TNode objects.  These are managed
//   completely through the ITraversable interface.
//////////////////////////////////////////////////////////////////////////
#ifndef ITRAVERSABLE_HH
#define ITRAVERSABLE_HH

#include <vector>
#include <string>
  using std::string;
  using std::vector;

#include "util/EcceURL.H"
#include "util/EcceMap.H"
#include "dsm/ResourceDescriptor.H"

class EDSI;
class TNode;

// For sorting TNode objects
#include <algorithm>



/**
 * EDSI-based implementation for sources of Hierarchical objects
 */
class ITraversable {
  public:
    ITraversable(const EcceURL& url);
    virtual ~ITraversable();

       // access to underlying EDSI for operations not supported directly
    EDSI * getEDSI() const;

    virtual string getRoot() = 0;
    virtual string getType() = 0;
    virtual vector<TNode*> *getChildren(TNode *node) = 0;
    virtual bool updateProperties(TNode *node) = 0;
    virtual bool updateList(TNode *node,vector<TNode*>& newChildren) = 0;
    virtual vector<string> getPropertyList() = 0;

    //virtual istream& get(TNode *node) = 0;
    virtual EcceURL *setName(TNode *node, const string& name) = 0;
    virtual EcceURL *move(TNode *node, TNode *newparent) = 0;
    virtual bool remove(TNode *node) = 0;
    virtual EcceURL *makeCollection(TNode *node, 
        const char *base="Untitled", const char *contenttype="") = 0;

    // Each operation should set "exception" messages here - we don't
    // wish to get into using exceptions at this point....
    void setLastMessage(const string& msg);
    string getLastMessage();

  protected:
    string p_lastMsg;
    EcceURL p_url;
    EDSI   *p_edsi;
};

/**
 * Container for information about what an ITraversable traverses!
 */
class TNode {
  public:
    TNode(string name = "", string contentType = "",
	  ResourceDescriptor::RESOURCETYPE resourceType = ResourceDescriptor::RT_DOCUMENT,
          const EcceURL & url = "");

    TNode(const TNode& rhs);
    virtual ~TNode();

    bool operator==(const TNode& rhs) const;
    TNode& operator=(const TNode& rhs);

  //    string getURL() const;
    EcceURL getUrl() const;
    string getName() const;
    string getContentType() const;
    ResourceDescriptor::RESOURCETYPE   getResourceType() const;
    EcceMap *getProperties() const;
    int getnumChildren() const;
    vector<TNode*> *getChildren() const;

  //    void setURL(const string& value);
    void setUrl(const EcceURL& value);
    void setName(const string& value);
    void setContentType(const string& value);
    void setResourceType(ResourceDescriptor::RESOURCETYPE value);
    void setChildren(const vector<TNode*>& value);
    void removeChild(TNode *child);
    virtual void removeChildren();
    void addChild(TNode *child);
    bool isChild(TNode *node) const;
    void addProperty(const string& key, const string& value);
    string getProperty(const string& key);
    void clearProperties();

    // User Data ala widget user data.
    void setUserData(void *ptr);
    void *getUserData() const;

    void setSessionIndex(int i);
    int getSessionIndex() const;

  protected:

    void   *p_userData;
    EcceURL p_url;
    string p_name;
    string p_contenttype;
    ResourceDescriptor::RESOURCETYPE  p_resourcetype;
    EcceMap *p_properties;
    vector<TNode*> p_children;

    bool p_recursiveDelete;
    int p_sessionIndex;
};

/*
struct TNodeSort : public std::binary_function<TNode *,TNode *, bool> {
  bool operator()(TNode *lhs, TNode *rhs) {
    return lhs->getName() < rhs->getName();
  }
};
*/

#endif

#include <iostream>
using namespace std;

#include "dsm/EDSI.H"
#include "dsm/VDoc.H"
#include "dsm/EDSIFactory.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"
#include "dsm/EDSITraversable.H"

EDSITraversable::EDSITraversable(const EcceURL& url) : ITraversable(url)
{
  p_edsi = EDSIFactory::getEDSI(url);
}

EDSITraversable::~EDSITraversable()
{
   if (p_edsi) { delete p_edsi; p_edsi = 0; }
}

bool EDSITraversable::operator==(const EDSITraversable& rhs) const
{ 
  return this == &rhs; 
}

/////////////////////////////////////////////////////////////////////////////
// Description
//   Return the name of the root of this ITraversable object.  This is
//   the path from which all subsequent urls are derived.
/////////////////////////////////////////////////////////////////////////////
string EDSITraversable::getRoot()
{
  return p_url.toString();
}

string EDSITraversable::getType()
{
  return "EDSITraversable";
}

vector<string> EDSITraversable::getPropertyList()
{
  setLastMessage("");
  vector<string> ret;
  if (!p_edsi->describeServerMetaData(ret)) {
    setLastMessage(p_edsi->m_msgStack.getMessage().c_str());
  }

  return ret;
}

/**
 * Get the collection and the standard properties as defined by
 * the EDSIFactory for lack of a better place to put this.
 * Based on whether parent is a session, update the index of the child
 * nodes.
 */
vector<TNode*> *EDSITraversable::getChildren(TNode *parent)
{
  setLastMessage("");

  vector<TNode*> *ret = new vector<TNode*>;
  int idx;

  // What properties are we interested in for this type of URL?
#define specifyProperties
#ifdef specifyProperties
  vector<string> props = getPropertyList();
  vector<MetaDataRequest> vmdr;
  MetaDataRequest mdr;
  int numProps = props.size();
  for (idx=0; idx<numProps; idx++) {
    mdr.name = props[idx];
    vmdr.push_back(mdr);
  }

#endif

  // Get the children and their properties
  EcceURL url = parent->getUrl();
  p_edsi->setURL(url);
  vector<ResourceMetaDataResult> children;
#ifdef specifyProperties
  if (p_edsi->listCollection(vmdr,children)) {
#else
  if (p_edsi->listCollection(children)) {
#endif
    int cnt = children.size();
    TNode *node;
    string name;
    for (idx=0; idx<cnt; idx++) {
      name = children[idx].url.getFilePathTail();

#ifdef INSTALL
      if (name.find("htaccess") != string::npos) continue;
#endif

      node = createTNode(name, children[idx].contenttype,
                         children[idx].resourcetype,
                         children[idx].url);
      int numMD = children[idx].metaData.size();
      for (int mdidx=0; mdidx<numMD; mdidx++) {
        node->addProperty(children[idx].metaData[mdidx].name.c_str(),
            children[idx].metaData[mdidx].value.c_str());
      }
      ret->push_back(node);
    }
  } else {
    setLastMessage(p_edsi->m_msgStack.getMessage().c_str());
  }
  
  ResourceType * resource = ResourceDescriptor::getResourceDescriptor()
    .getResourceType(parent);
  if (resource && resource->getIsSessionBased()) {
    Session * session = dynamic_cast<Session*>(EDSIFactory::getResource(parent->getUrl()));
    vector<Resource *> tasks = session->getMembers();
    int i=0;
    for (; i<tasks.size(); i++) {
      int j=0;
      for (; j<ret->size(); j++) {
        if ((*ret)[j]->getUrl() == tasks[i]->getURL()) {
          (*ret)[j]->setSessionIndex(i);
          break;
          }
      }
      if (j == ret->size())
        cerr << "Task is located outside parent, what do we do?\n";
    }
  }
  return ret;
}


/**
 * Get the collection contents and add any new children.
 */
bool EDSITraversable::updateList(TNode *parent,vector<TNode*>& newChildren) 
{
  setLastMessage("");
  bool ret = false;

  // Get the children and their properties
  p_edsi->setURL(parent->getUrl());
  vector<ResourceMetaDataResult> children;
  if (p_edsi->listCollection(children)) {
    ret = true;

    // Get list of existing children
    vector<TNode*> *curChildren = parent->getChildren();
    int curCnt = curChildren->size();

    int cnt = children.size();
    TNode *node;
    for (int idx=0; idx<cnt; idx++) {
      EcceURL childUrl = children[idx].url;

      // See if this chlid already exists.  Might want to make this into
      // a temporary hash later to improve performance
      bool alreadyHaveIt = false;
      for (int cdx=0; cdx<curCnt; cdx++) {
         if (childUrl == (*curChildren)[cdx]->getUrl()) {
           alreadyHaveIt = true;
           break;
         }
      }
      if (!alreadyHaveIt) {

        node = createTNode(children[idx].url.getFilePathTail(),
                           children[idx].contenttype,
                           children[idx].resourcetype, childUrl);
        int numMD = children[idx].metaData.size();
        for (int mdidx=0; mdidx<numMD; mdidx++) {
          node->addProperty(children[idx].metaData[mdidx].name.c_str(),
              children[idx].metaData[mdidx].value.c_str());
        }
        newChildren.push_back(node);
      }
    }
    delete curChildren;
  } else {
    setLastMessage(p_edsi->m_msgStack.getMessage().c_str());
  }

  return ret;
}

////////////////////////////////////////////////////////////////////////////
// Description
//   Clear existing properties and get new ones.
////////////////////////////////////////////////////////////////////////////
bool EDSITraversable::updateProperties(TNode *node) 
{
  setLastMessage("");
  bool ret = true;

  vector<MetaDataRequest> vmdr;
  ResourceMetaDataResult result;
#ifdef specifyProperties
  // What properties are we interested in for this type of URL?
  vector<string> props = getPropertyList();
  MetaDataRequest mdr;
  int numProps = props.size();
  for (int idx=0; idx<numProps; idx++) {
    mdr.name = props[idx];
    vmdr.push_back(mdr);
  }
#endif
  EcceURL url = node->getUrl();
  //cout << "setting url before " << p_edsi->getURL().toString() << endl;
  p_edsi->setURL(url);
  //cout << "setting url after " << p_edsi->getURL().toString() << endl;
  if (p_edsi->getMetaData(vmdr,result)) {
    node->clearProperties();

    node->setUrl(result.url);
    node->setName(result.url.getFilePathTail().c_str());
    node->setResourceType(result.resourcetype);
    node->setContentType(result.contenttype.c_str());
    int numMD = result.metaData.size();
    for (int mdidx=0; mdidx<numMD; mdidx++) {
      //cout << result.metaData[mdidx].name << " " << result.metaData[mdidx].value << endl;
      node->addProperty(result.metaData[mdidx].name.c_str(),
          result.metaData[mdidx].value.c_str());
    }
  } else {
    ret = false;
    setLastMessage(p_edsi->m_msgStack.getMessage().c_str());
  }
  return ret;

}

EcceURL *EDSITraversable::setName(TNode *node, const string& name)
{
  setLastMessage("");

  EcceURL *ret = NULL;
  EcceURL url = node->getUrl();
  string urlstr = url.toString();
  p_edsi->setURL(url);
  string newfile = url.getFilePathRoot();
  newfile += "/" + name;

  // If it had a trailing slash in the old name, make sure it has one in
  // the new name.  This does indicate collection vs file and some string
  // matching functions get messed up if we are not consistent about
  // this.
  if (urlstr[urlstr.size()-1] == '/') newfile.append("/");

  ret = new EcceURL(url.getProtocol().c_str(),url.getHost().c_str(), 
      url.getPort(), newfile.c_str());
  if (!p_edsi->moveResource(*ret)) {
    delete ret;
    ret = NULL;
    setLastMessage(p_edsi->m_msgStack.getMessage().c_str());
  }

  return ret;
}

EcceURL *EDSITraversable::move(TNode *node, TNode *newparent)
{
  //cout << "moving from url " << node->getURL() << endl;
  //cout << "moving to url " << newparent->getURL() << endl;
  setLastMessage("");

  EcceURL *ret = NULL;
  EcceURL url = node->getUrl();
  string urlstr = url.toString();
  p_edsi->setURL(url);

  // Determine new url from parent
  EcceURL purl = newparent->getUrl();

  // If it had a trailing slash in the old name, make sure it has one in
  // the new name.  This does indicate collection vs file and some string
  // matching functions get messed up if we are not consistent about
  // this.
  string name = node->getName();
  if (urlstr[urlstr.size()-1] == '/') name.append("/");

  purl = purl.getChild(name.c_str());

  ret = new EcceURL(purl);
  if (!p_edsi->moveResource(*ret, EDSI::SORTOF)) {
    delete ret;
    ret = NULL;
    setLastMessage(p_edsi->m_msgStack.getMessage().c_str());
  }

  return ret;
}

bool EDSITraversable::remove(TNode *node)
{
  setLastMessage("");
  bool ret = false;
  EcceURL parent = node->getUrl();
  p_edsi->setURL(parent);
  ret = p_edsi->removeResource();
  return ret;
}

EcceURL *EDSITraversable::makeCollection(TNode *node, const char *base,
                                         const char *contenttype)
{
  setLastMessage("");

  EcceURL *ret = NULL;
  EcceURL parent = node->getUrl();
  p_edsi->setURL(parent);
  ret = p_edsi->makeCollection(base);
  if (ret) {
    p_edsi->setURL(*ret);
    if (contenttype && strlen(contenttype) > 0) {
      vector<MetaDataResult> results(1);
      results[0].name = VDoc::getEcceNamespace() + ":contenttype";
      results[0].value = contenttype;
      if (!p_edsi->putMetaData(results)) {
        setLastMessage(p_edsi->m_msgStack.getMessage().c_str());
      }
    }
  } else {
    setLastMessage(p_edsi->m_msgStack.getMessage().c_str());
  }
  return ret;
}


TNode * EDSITraversable::createTNode(string name,
                                     string contentType,
                                     ResourceDescriptor::RESOURCETYPE resourceType,
                                     const EcceURL & url)
{
  return new TNode(name, contentType, resourceType, url);
}

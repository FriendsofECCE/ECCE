#include <iostream>
using std::cout;
using std::endl;

#include "dsm/ITraversable.H"
#include "dsm/EDSI.H"
#include "dsm/ResourceDescriptor.H"
#include "util/EcceURL.H"


ITraversable::ITraversable(const EcceURL& url)
{
  p_edsi = 0;
  p_url = url;
}


ITraversable::~ITraversable()
{
  if (p_edsi) { delete p_edsi; p_edsi = 0; }
}


void ITraversable::setLastMessage(const string& msg)
{
  p_lastMsg = msg;
}


string ITraversable::getLastMessage()
{
  return p_lastMsg;
}


EDSI * ITraversable::getEDSI() const
{
  return p_edsi;
}


TNode::TNode(string name, string contentType,
	     ResourceDescriptor::RESOURCETYPE resourceType, const EcceURL & url)
{
  p_properties = new EcceMap();
  p_userData = NULL;

  p_name = name;
  p_contenttype = contentType;
  p_resourcetype = resourceType;
  p_url = url;

  // This flag is here because WxEdsiTreeItemData which is a subclass of TNode
  // and wxTreeItemData requires to turn off the recursive deletion since
  // wxTreeCtrl will auto delete all the wxTreeItemData. 
  p_recursiveDelete = true;
  // A TNode in a non-session collection will always has an index of -1.
  p_sessionIndex = -1;
}


TNode::~TNode() 
{
  if (p_properties) {
    delete p_properties;
    p_properties = NULL;
  }

  if (p_recursiveDelete) {
    removeChildren();
  }
  else {
    p_children.clear();
  }

  p_userData = 0;
}


TNode::TNode(const TNode &rhs) 
{
  p_properties = new EcceMap;
  *this = rhs;
}


TNode& TNode::operator=(const TNode& rhs)
{
  if (this != &rhs) {
    p_url = rhs.getUrl();
    p_name = rhs.getName();
    p_contenttype = rhs.getContentType();
    p_resourcetype = rhs.getResourceType();
    vector<TNode*> *tmp = rhs.getChildren();
    p_children = *tmp;
    delete tmp;
    p_userData = rhs.getUserData();
  }
  return *this;
}


bool TNode::operator==(const TNode& rhs) const
{
  return this == &rhs;
}


/**
 * @deprecated Should avoid using string URLs, use EcceURL instead.
void TNode::setURL(const string& value)
{
  p_url = value;
}
*/


void TNode::setUrl(const EcceURL& value)
{
  p_url = value;
}


void TNode::setName(const string& value)
{
  p_name = value;
}


void TNode::setResourceType(ResourceDescriptor::RESOURCETYPE type)
{
  p_resourcetype = type;
}


void TNode::setContentType(const string& type)
{
  p_contenttype = type;
}


void TNode::setChildren(const vector<TNode*>& value)
{
  p_children = value;
}


void TNode::removeChildren()
{
  vector<TNode*>::iterator itor = p_children.begin();
  for (;itor != p_children.end(); itor++) {
    delete (*itor);
  }
  p_children.clear();
}


void TNode::removeChild(TNode *child)
{
  vector<TNode*>::iterator it = p_children.begin();
  while (it != p_children.end()) {
    if (*it == child) {
      p_children.erase(it);
      break;
    }
    it++;
  }
}


void TNode::addChild(TNode *child)
{
  p_children.push_back(child);
}


void TNode::clearProperties()
{
  p_properties->clear();
}


void TNode::addProperty(const string& key, const string& value)
{
  (*p_properties)[key.c_str()] = value.c_str();
}


string TNode::getProperty(const string& key) 
{
  string ret;
  p_properties->findValue(key.c_str(), ret);
  return ret;
}


EcceMap *TNode::getProperties() const
{
  // Make a copy of the map
  // caller needs to delete this pointer
  EcceMap *ret = new EcceMap(*p_properties);
  return ret;
}


/**
 * @deprecated Should avoid using string URLs, use EcceURL instead.
string TNode::getURL() const
{
  return p_url.toString();
}
*/

EcceURL TNode::getUrl() const
{
  return p_url;
}


string TNode::getName() const
{
  return p_name;
}


string TNode::getContentType() const
{
  return p_contenttype;
}


ResourceDescriptor::RESOURCETYPE TNode::getResourceType() const
{
  return p_resourcetype;
}


vector<TNode*> *TNode::getChildren() const
{ 
  vector<TNode*> *ret = new vector<TNode*>(p_children.size());
  *ret = p_children;
  return ret; 
}


int TNode::getnumChildren() const
{
  return p_children.size();
}


bool TNode::isChild(TNode *node) const
{
  bool ret = false;
  vector<TNode*>::const_iterator it;
  for (it = p_children.begin(); it!=p_children.end(); it++) {
    if (node == (*it)) {
      ret = true;
      break;
    }
  }
  return ret;
}


void TNode::setUserData(void *data)
{
  p_userData = data;
}


void *TNode::getUserData() const
{
  return p_userData;
} 


void TNode::setSessionIndex(int i)
{
  p_sessionIndex = i;
}


int TNode::getSessionIndex() const
{
  return p_sessionIndex;
}

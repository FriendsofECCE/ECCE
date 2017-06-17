/**
* @file
*
*
*/

#include <string>
  using std::string;

#include <vector>
  using std::vector;

#include <utility>
  using std::make_pair;

#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/util/XMLString.hpp>
  using namespace xercesc;

#include"util/STLUtil.H"
#include"util/NotImplementedException.H"

#include "dsm/AtomView.H"


/**
 * Constructor.
 */
AtomView::AtomView(DOMElement *avElement)
{
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;

  // Get attributes
  attributes = avElement->getAttributes();

  // Get name attribute value
  p_name = "";
  XMLCh *tmpStr = XMLString::transcode("name");
  attribute = attributes->getNamedItem(tmpStr);
  delete [] tmpStr;
  if (attribute != 0) {
    cptr = XMLString::transcode(attribute->getNodeValue());
    p_name = cptr;
    delete [] cptr;
  }

  // Get Menu Item
  p_menuItem = loadMenuItem(avElement);

  // Get Order
  p_order = loadOrder(avElement);

  // Load resource properties
  p_properties = loadProperties(avElement);
}

AtomView::AtomView(const AtomView& rhs)
{
   NOTIMPLEMENTEDEXCEPTION(0,"AtomView copy constructor");
}

/**
 * Destructor
 */
AtomView::~AtomView()
{
  cleanup();
}


void AtomView::cleanup()
{
  flushProperties();
}


void AtomView::flushProperties()
{
  multimap<int, AtomProperty*>::iterator mapIt;
  for (mapIt = p_properties.begin(); 
       mapIt != p_properties.end();
       mapIt++) {
    if (mapIt->second != 0) {
      delete mapIt->second;
      mapIt->second = 0;
    }
  }
  p_properties.clear();
}


string AtomView::getName()
{
  return p_name;
}


int AtomView::getId()
{
  return p_id;
}


string AtomView::getMenuItem()
{
  return p_menuItem;
}


string AtomView::getToolTip()
{
  return p_toolTip;
}


multimap<int, AtomProperty*> AtomView::getProperties() 
{
  return p_properties;
}


multimap<int, AtomProperty*> AtomView::getDisplayedProperties() 
{
  multimap<int, AtomProperty*> properties;
  multimap<int, AtomProperty*>::iterator mapIt;

if (p_properties.size() != 0) {

  for (mapIt = p_properties.begin();
       mapIt != p_properties.end();
       mapIt++) {
    if (mapIt->second->getDisplay()) {
      const int key = mapIt->first;
      AtomProperty* val = mapIt->second;
      properties.insert(make_pair(key, val));
    }
  }
}
  return properties;
}


int AtomView::getOrder()
{
  return p_order;
}


void AtomView::setName(const string& name)
{
  p_name = name;
}


void AtomView::setId(int id)
{
  p_id = id;
}


void AtomView::setMenuItem(const string& menuItem)
{
  p_menuItem = menuItem;
}


void AtomView::setToolTip(const string& toolTip)
{
  p_toolTip = toolTip;
}


void AtomView::setOrder(const int& order)
{
  p_order = order;
}


void AtomView::setProperties(multimap<int, AtomProperty*> properties)
{
  flushProperties();
  p_properties = properties;

}


/**
 * Get value of <MenuItem> for an Atom View.  Returns an empty
 * string if menuItem element not found.
 */
string AtomView::loadMenuItem(DOMElement *avElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("MenuItem");
  DOMNodeList *nodes = avElement->getElementsByTagName(tmpStr);
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
 * Get value of <ToolTip> for an Atom View.  Returns an empty
 * string if toolTip element not found.
 */
string AtomView::loadToolTip(DOMElement *avElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("ToolTip");
  DOMNodeList *nodes = avElement->getElementsByTagName(tmpStr);
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
 * Get value of <Order> for a Property View.  Returns 0
 * if the order is missing.
 */
int AtomView::loadOrder(DOMElement *avElement)
{
  int ret = 0;
  string orderVal = "";

  XMLCh *tmpStr = XMLString::transcode("Order");
  DOMNodeList *nodes = avElement->getElementsByTagName(tmpStr);
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
      orderVal = value;
      delete [] value;
    }
  }

  STLUtil::stoi(orderVal, ret);
  return ret;
}


/** 
 * Get the resource properties that are a part of this 
 * property view.
 */
multimap<int, AtomProperty*> AtomView::loadProperties(DOMElement *avElement)
{
  multimap<int, AtomProperty*> ret;

  XMLCh *tmpStr = XMLString::transcode("AtomProperty");
  DOMNodeList *nodes = avElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0;
  if (nodes != 0) {
    numNodes = nodes->getLength();
  }

  for (int i = 0; i < numNodes; i++) {
    DOMElement* ele = (DOMElement*)nodes->item(i);
    AtomProperty *resourceProperty = new AtomProperty(ele);
    ret.insert(make_pair(resourceProperty->getOrder(), resourceProperty));
  }

  return ret;
}








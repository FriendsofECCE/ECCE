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

#include "dsm/PropertyView.H"



/**
 * Constructor.
 */
PropertyView::PropertyView(DOMElement *pvElement)
{
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;

  // Get attributes
  attributes = pvElement->getAttributes();

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
  p_menuItem = loadMenuItem(pvElement);

  // Get Order
  p_order = loadOrder(pvElement);

  // Load resource properties
  p_properties = loadProperties(pvElement);

}


/**
 * Destructor
 */
PropertyView::~PropertyView()
{
  cleanup();
}


void PropertyView::cleanup()
{
  flushProperties();
}


void PropertyView::flushProperties()
{
  multimap<int, ResourceProperty*>::iterator mapIt;
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


string PropertyView::getName()
{
  return p_name;
}


int PropertyView::getId()
{
  return p_id;
}


string PropertyView::getMenuItem()
{
  return p_menuItem;
}

multimap<int, ResourceProperty*> PropertyView::getProperties() 
{
  return p_properties;
}


multimap<int, ResourceProperty*> PropertyView::getDisplayedProperties() 
{
  multimap<int, ResourceProperty*> properties;
  multimap<int, ResourceProperty*>::iterator mapIt;

  for (mapIt = p_properties.begin();
       mapIt != p_properties.end();
       mapIt++) {
    if (mapIt->second->getDisplay()) {
      const int key = mapIt->first;
      ResourceProperty* val = mapIt->second;
      properties.insert(make_pair(key, val));
    }
  }

  return properties;
}


int PropertyView::getOrder()
{
  return p_order;
}


void PropertyView::setName(const string& name)
{
  p_name = name;
}


void PropertyView::setId(int id)
{
  p_id = id;
}


void PropertyView::setMenuItem(const string& menuItem)
{
  p_menuItem = menuItem;
}


void PropertyView::setOrder(const int& order)
{
  p_order = order;
}


void PropertyView::setProperties(multimap<int, ResourceProperty*> properties)
{
  flushProperties();
  p_properties = properties;

}


/**
 * Get value of <MenuItem> for a Property View.  Returns an empty
 * string if menuItem element not found.
 */
string PropertyView::loadMenuItem(DOMElement *pvElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("MenuItem");
  DOMNodeList *nodes = pvElement->getElementsByTagName(tmpStr);
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
int PropertyView::loadOrder(DOMElement *pvElement)
{
  int ret = 0;
  string orderVal = "";

  XMLCh *tmpStr = XMLString::transcode("Order");
  DOMNodeList *nodes = pvElement->getElementsByTagName(tmpStr);
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
multimap<int, ResourceProperty*> PropertyView::loadProperties(DOMElement *pvElement)
{
  multimap<int, ResourceProperty*> ret;

  XMLCh *tmpStr = XMLString::transcode("Property");
  DOMNodeList *nodes = pvElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0;
  if (nodes != 0) {
    numNodes = nodes->getLength();
  }

  for (int i = 0; i < numNodes; i++) {
    DOMElement* ele = (DOMElement*)nodes->item(i);
    ResourceProperty *resourceProperty = new ResourceProperty(ele);
    ret.insert(make_pair(resourceProperty->getOrder(), resourceProperty));
  }

  return ret;
}








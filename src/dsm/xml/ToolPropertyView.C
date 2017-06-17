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

#include "dsm/ToolPropertyView.H"



/**
 * Constructor.
 */
ToolPropertyView::ToolPropertyView(DOMElement *pvElement)
{
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;
  XMLCh *tmpStr;

  // Get attributes
  attributes = pvElement->getAttributes();

  // Get contentType attribute value
  p_contentType = "";
  tmpStr = XMLString::transcode("contentType");
  attribute = attributes->getNamedItem(tmpStr);
  if (attribute != 0) {
    cptr = XMLString::transcode(attribute->getNodeValue());
    p_contentType = cptr;
  }

  // Get tool attribute value
  p_tool = "";
  tmpStr = XMLString::transcode("tool");
  attribute = attributes->getNamedItem(tmpStr);
  if (attribute != 0) {
    cptr = XMLString::transcode(attribute->getNodeValue());
    p_tool = cptr;
  }

  delete [] cptr;
  delete [] tmpStr;

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
ToolPropertyView::~ToolPropertyView()
{
  cleanup();
}


void ToolPropertyView::cleanup()
{
  flushProperties();
}


void ToolPropertyView::flushProperties()
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


string ToolPropertyView::getTool()
{
  return p_tool;
}


string ToolPropertyView::getContentType()
{
  return p_contentType;
}


string ToolPropertyView::getMenuItem()
{
  return p_menuItem;
}


multimap<int, ResourceProperty*> ToolPropertyView::getProperties() 
{
  return p_properties;
}


multimap<int, ResourceProperty*> ToolPropertyView::getDisplayedProperties() 
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


int ToolPropertyView::getOrder()
{
  return p_order;
}


void ToolPropertyView::setTool(const string& tool)
{
  p_tool = tool;
}


void ToolPropertyView::setContentType(const string& contentType)
{
  p_contentType = contentType;
}


void ToolPropertyView::setMenuItem(const string& menuItem)
{
  p_menuItem = menuItem;
}


void ToolPropertyView::setOrder(const int& order)
{
  p_order = order;
}


void ToolPropertyView::setProperties(multimap<int, ResourceProperty*> properties)
{
  flushProperties();
  p_properties = properties;

}


/**
 * Get value of <MenuItem> for a Property View.  Returns an empty
 * string if menuItem element not found.
 */
string ToolPropertyView::loadMenuItem(DOMElement *pvElement)
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
int ToolPropertyView::loadOrder(DOMElement *pvElement)
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
multimap<int, ResourceProperty*> ToolPropertyView::loadProperties(DOMElement *pvElement)
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








/**
* @file
*
*
*/

#include <string>
  using std::string;

#include <vector>
  using std::vector;

#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/util/XMLString.hpp>
  using namespace xercesc;


#include "util/StringConverter.H"
#include "util/STLUtil.H"


#include "dsm/GridProperty.H"



/**
 * Constructor.  
 */
GridProperty::GridProperty()
{
  p_name = "";
  p_order = 0;
  p_label = "";
  p_display = false;
  p_isReadOnly = false;
  p_type = GridProperty::PT_UNDEFINED;
}


/**
 * Constructor.  
 */
GridProperty::GridProperty(GridProperty::PropertyType type)
{
  p_name = "";
  p_order = 0;
  p_label = "";
  p_display = false;
  p_isReadOnly = false;
  p_type = type;
}


/**
 * Constructor.  
 */
GridProperty::GridProperty(DOMElement *propertyElement)
{
  p_label = loadLabel(propertyElement);
  p_name = loadName(propertyElement);
  p_order = loadOrder(propertyElement);
  p_display = loadDisplay(propertyElement);
  p_type = loadType(propertyElement);
  p_isReadOnly = loadReadOnly(propertyElement);
}


bool GridProperty::operator==(const GridProperty& rhs)
{
  return this->getName() == rhs.getName();
}


/**
 * Destructor
 */
GridProperty::~GridProperty()
{}


string GridProperty::getName() const
{
  return p_name;
}


int GridProperty::getOrder() const
{
  return p_order;
}


string GridProperty::getLabel() const
{
  return p_label;
}


bool GridProperty::getDisplay() const
{
  return p_display;
}


bool GridProperty::getReadOnly() const
{
  return p_isReadOnly;
}


GridProperty::PropertyType GridProperty::getType() const
{
  return p_type;
}


void GridProperty::setName(const string& name)
{
  p_name = name;
}


void GridProperty::setOrder(const int& order)
{
  p_order = order;
}


void GridProperty::setLabel(const string& label)
{
  p_label = label;
}


void GridProperty::setDisplay(const bool& isDisplayed)
{
  p_display = isDisplayed;
}


void GridProperty::setReadOnly(const bool& isReadOnly)
{
  p_isReadOnly = isReadOnly;
}


void GridProperty::setType(GridProperty::PropertyType type)
{
  p_type = type;
}


/**
 * Converts a a PropertyType to its cooresponding string value
 */
string GridProperty::propertyTypeToStr(GridProperty::PropertyType type)
{
  string ret = "";

  switch (type) {
    case PT_CDATA:
      ret = "cdata";
      break;
    case PT_DATE_TIME:
      ret = "datetime";
      break;
    case PT_CHECK:
      ret = "check";
      break;
    case PT_ICON:
      ret = "icon";
      break;
    case PT_RESOURCE_ICON:
      ret = "resourceicon";
      break;
    case PT_RUNSTATE_ICON:
      ret = "runstateicon";
      break;
    case PT_NUMERIC:
      ret = "numeric";
      break;
    case PT_FLOAT:
      ret = "float";
      break;
    case PT_UNDEFINED:
      ret = "undefined";
      break;
  }
  return ret;
}


/**
 * Converts a string to a PropertyType, if the string value is not recognized
 * then PT_UNDEFINED will be returned.
 */
GridProperty::PropertyType GridProperty::strToPropertyType(const string& pt)
{
  GridProperty::PropertyType ret;

  string lstr = "";
  if (StringConverter::toLower(pt, lstr)) {
    if (lstr == "cdata") {
      ret = GridProperty::PT_CDATA;
    }
    else if (lstr == "datetime") {
      ret = GridProperty::PT_DATE_TIME;
    }
    else if (lstr == "check") {
      ret = GridProperty::PT_CHECK;
    }
    else if (lstr == "icon") {
      ret = GridProperty::PT_ICON;
    }
    else if (lstr == "resourceicon") {
      ret = GridProperty::PT_RESOURCE_ICON;
    }
    else if (lstr == "runstateicon") {
      ret = GridProperty::PT_RUNSTATE_ICON;
    }
    else if (lstr == "numeric") {
      ret = GridProperty::PT_NUMERIC;
    }
    else if (lstr == "float") {
      ret = GridProperty::PT_FLOAT;
    }
    else {
      ret = GridProperty::PT_UNDEFINED;
    }
  }
  return ret;
}



/**
 * Get value of <Label> for a Resource Property.
 */
string GridProperty::loadLabel(DOMElement *propertyElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("Label");
  DOMNodeList *nodes = propertyElement->getElementsByTagName(tmpStr);
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
 * Get value of <Name> for a Resource Property.
 */
string GridProperty::loadName(DOMElement *propertyElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("Name");
  DOMNodeList *nodes = propertyElement->getElementsByTagName(tmpStr);
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
 * Get value of <Order> for a Resource Property.  Returns 0
 * if the order value is missing or cannot be evaluated.
 */
int GridProperty::loadOrder(DOMElement *propertyElement)
{
  int ret = 0;
  string orderVal = "";

  XMLCh *tmpStr = XMLString::transcode("Order");
  DOMNodeList *nodes = propertyElement->getElementsByTagName(tmpStr);
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
 * Get value of <Display> for a Resource Property.  Returns false
 * if the display value is not recognized, values of "true" or "false"
 * are expected.
 */
bool GridProperty::loadDisplay(DOMElement *propertyElement)
{
  bool ret = false;
  string displayVal = "";

  XMLCh *tmpStr = XMLString::transcode("Display");
  DOMNodeList *nodes = propertyElement->getElementsByTagName(tmpStr);
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
      displayVal = value;
      delete [] value;
    }
  }

  if (displayVal == "true") {
    ret = true;
  }

  return ret;
}


/**
 * Get value of <IsReadOnly> for a Resource Property.  Returns false
 * if the read only value is not recognized, values of "true" or "false"
 * are expected.
 */
bool GridProperty::loadReadOnly(DOMElement *propertyElement)
{
  bool ret = false;
  string readOnlyVal = "";

  XMLCh *tmpStr = XMLString::transcode("IsReadOnly");
  DOMNodeList *nodes = propertyElement->getElementsByTagName(tmpStr);
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
      readOnlyVal = value;
      delete [] value;
    }
  }

  if (readOnlyVal == "true") {
    ret = true;
  }

  return ret;
}


/**
 * Get value of <Type> for a Resource Property.  Returns PT_UNDEFINED
 * if the type cannot be determined.
 */
GridProperty::PropertyType GridProperty::loadType(DOMElement *propertyElement)
{
  GridProperty::PropertyType ret;
  string typeVal = "";

  XMLCh *tmpStr = XMLString::transcode("Type");
  DOMNodeList *nodes = propertyElement->getElementsByTagName(tmpStr);
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
      typeVal = value;
      delete [] value;
    }
  }
  ret = strToPropertyType(typeVal);

  return ret;
}



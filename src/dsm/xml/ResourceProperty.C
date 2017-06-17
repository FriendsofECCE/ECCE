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

#include "dsm/ResourceProperty.H"



/**
 * Constructor.  
 */
ResourceProperty::ResourceProperty()
{
  p_nameSpace = "";
}


/**
 * Constructor.  
 */
ResourceProperty::ResourceProperty(DOMElement *propertyElement) : 
                  GridProperty(propertyElement)
{
  p_nameSpace = loadNameSpace(propertyElement);
}


bool ResourceProperty::operator==(const ResourceProperty& rhs)
{
  return this->getQName() == rhs.getQName();
}


/**
 * Destructor
 */
ResourceProperty::~ResourceProperty()
{}


string ResourceProperty::getNameSpace() const
{
  return p_nameSpace;
}


string ResourceProperty::getQName() const
{
  return p_nameSpace + ":" + p_name;
}


void ResourceProperty::setNameSpace(const string& nameSpace)
{
  p_nameSpace = nameSpace;
}


/**
 * Get value of <NameSpace> for a Resource Property.
 */
string ResourceProperty::loadNameSpace(DOMElement *propertyElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("Namespace");
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



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

#include "dsm/ResourceTool.H"



/**
 * Constructor.  Only ResourceDescriptor can
 * create ResourceTool objects.
 */
ResourceTool::ResourceTool(DOMElement *toolElement)
{
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;

  // Get attributes
  attributes = toolElement->getAttributes();

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

  // Get icon 
  p_icon = loadIcon(toolElement);

  // Get Label
  p_label = loadLabel(toolElement);

  // Get Menu Item
  p_menuItem = loadMenuItem(toolElement);

  // Get Invoke Arg
  p_invokeArg = loadInvokeArg(toolElement);

  // Get Location in Gateway 
  p_gateway = loadGateway(toolElement);;

  p_id = -1;
}


/**
 * Destructor
 */
ResourceTool::~ResourceTool()
{}


string ResourceTool::getName()
{
  return p_name;
}


int ResourceTool::getId()
{
  return p_id;
}


string ResourceTool::getIcon()
{
  return p_icon;
}


string ResourceTool::getLabel()
{
  return p_label;
}


string ResourceTool::getMenuItem()
{
  return p_menuItem;
}


string ResourceTool::getInvokeArg()
{
  return p_invokeArg;
}


int ResourceTool::getGateway()
{
  return p_gateway;
}


void ResourceTool::setName(const string& name)
{
  p_name = name;
}


void ResourceTool::setId(int id)
{
  p_id = id;
}


void ResourceTool::setIcon(const string& icon)
{
  p_icon = icon;
}


void ResourceTool::setLabel(const string& label)
{
  p_label = label;
}


void ResourceTool::setMenuItem(const string& menuItem)
{
  p_menuItem = menuItem;
}


void ResourceTool::setInvokeArg(const string& invokeArg)
{
  p_invokeArg = invokeArg;
}


void ResourceTool::setGateway(int gateway)
{
  p_gateway = gateway;
}


/**
 * Get value of <MenuItem> for a resource tool.  Returns an empty
 * string if MenuItem element not found.
 */
string ResourceTool::loadMenuItem(DOMElement *toolElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("MenuItem");
  DOMNodeList *nodes = toolElement->getElementsByTagName(tmpStr);
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
 * Get value of <Label> for a resource tool.  Returns an empty
 * string if Label element not found.
 */
string ResourceTool::loadLabel(DOMElement *toolElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("Label");
  DOMNodeList *nodes = toolElement->getElementsByTagName(tmpStr);
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
 * Get value of <Icon> for a resource tool.  Returns an empty
 * string if Icon element not found.
 */
string ResourceTool::loadIcon(DOMElement *toolElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("Icon");
  DOMNodeList *nodes = toolElement->getElementsByTagName(tmpStr);
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
 * Get value of <InvokeArg> for a resource tool.  Returns an empty
 * string if InvokeArg element not found.
 */
string ResourceTool::loadInvokeArg(DOMElement *toolElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("InvokeArg");
  DOMNodeList *nodes = toolElement->getElementsByTagName(tmpStr);
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
 * Get value of <Gateway> for a resource tool.  Returns an empty
 * -1 if Gateway element not found.
 */
int ResourceTool::loadGateway(DOMElement *toolElement)
{
  string ret = "";
  int retInt = -1;

  XMLCh *tmpStr = XMLString::transcode("Gateway");
  DOMNodeList *nodes = toolElement->getElementsByTagName(tmpStr);
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

  if (ret != "")
    StringConverter::toInt(ret, retInt);

  return retInt;
}

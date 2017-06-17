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

#include "dsm/ResourceRmTask.H"



/**
 * Constructor. Constructs the object with a <RunManagementTask>
 * element - see the ResourceDescriptor.xml file.
 */
ResourceRmTask::ResourceRmTask(DOMElement *rmTaskElement)
{
  DOMNamedNodeMap *attributes = 0;
  DOMNode *attribute = 0;
  char *cptr = (char*)0;

  // Get attributes
  attributes = rmTaskElement->getAttributes();

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

  // Get the Menu Item
  p_menuItem = loadMenuItem(rmTaskElement);

  p_id = -1;
}


/**
 * Destructor
 */
ResourceRmTask::~ResourceRmTask()
{}


string ResourceRmTask::getName()
{
  return p_name;
}


int ResourceRmTask::getId()
{
  return p_id;
}


string ResourceRmTask::getMenuItem()
{
  return p_menuItem;
}


void ResourceRmTask::setName(const string& name)
{
  p_name = name;
}


void ResourceRmTask::setId(int id)
{
  p_id = id;
}


void ResourceRmTask::setMenuItem(const string& menuItem)
{
  p_menuItem = menuItem;
}


/**
 * Get value of <MenuItem> for a run management task.  Returns an empty
 * string if menuItem element not found.
 */
string ResourceRmTask::loadMenuItem(DOMElement *rmTaskElement)
{
  string ret = "";

  XMLCh *tmpStr = XMLString::transcode("MenuItem");
  DOMNodeList *nodes = rmTaskElement->getElementsByTagName(tmpStr);
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




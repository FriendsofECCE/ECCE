#include <stdio.h>

#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>

#include "util/Serializable.H"
#include "dsm/XMLSerializer.H"

using std::string;

string XMLSerializer::xmlHeader() const
{
   return "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\" ?>";
}

string XMLSerializer::getAsString(const XMLCh *data) const
{
   string ret;
   if (data) {
      char *cptr = XMLString::transcode(data);
      if (cptr) {
         ret = cptr;
         delete [] cptr;
      }

   }
   return ret;
}

DOMElement* XMLSerializer::getNextElementFromTag(const DOMElement *root,
                                                 const string tag) const
{
  DOMElement* ret = 0;
  XMLCh *tagName = XMLString::transcode(tag.c_str());
  if (root != (DOMElement*)0) {
    DOMNodeList *nodeList = root->getElementsByTagName(tagName);
    delete [] tagName;
    if (nodeList->getLength() > 0) {
      DOMNode *node = nodeList->item(0);
      if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
        ret = (DOMElement*)node;
      }
    }
  } else {
    delete [] tagName;
  }
  return ret;
}

vector<DOMElement*> XMLSerializer::getNextElementsFromTag(const DOMElement *root,
                                                          const string tag) const
{
  vector<DOMElement*> ret;
  DOMElement* item = 0;
  XMLCh *tagName = XMLString::transcode(tag.c_str());
  if (root != (DOMElement*)0) {
    DOMNodeList *nodeList = root->getElementsByTagName(tagName);
    delete [] tagName;
    int idx;
    for (idx = 0; idx<nodeList->getLength(); idx++) {
      DOMNode *node = nodeList->item(idx);
      if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
        item = (DOMElement*)node;
        ret.push_back(item);
      }
    }
  } else {
    delete [] tagName;
  }
  return ret;
}

string XMLSerializer::getTagValue(const DOMElement *root, const string tag) const
{
  string ret;
  XMLCh *tagName = XMLString::transcode(tag.c_str());
  DOMNodeList *nodeList = root->getElementsByTagName(tagName);
  delete [] tagName;
  if (nodeList->getLength() > 0) {
    DOMNode *node = nodeList->item(0);
    if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
      DOMNode *child = node->getFirstChild();
      if (child && child->getNodeType() == DOMNode::TEXT_NODE) {
        ret = getAsString(child->getNodeValue());
      }
    }
  }
  return ret;
}

string XMLSerializer::getChildTagValue(const DOMElement *root, const string parent, const string child) const
{
  string ret;
  XMLCh *tagName = XMLString::transcode(parent.c_str());
  DOMNodeList *nodeList = root->getElementsByTagName(tagName);
  delete [] tagName;
  string localvalue;
  if (nodeList->getLength() > 0) {
    DOMNode *node = nodeList->item(0);
    if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
      DOMNodeList *childList = node->getChildNodes();
      int numChildren = childList->getLength();
      for (int ichild = 0; ichild < numChildren; ichild++) {
        node = childList->item(ichild);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
          localvalue = getAsString(node->getNodeName());
          if (getAsString(node->getNodeName()) == child) {
            DOMNode *childNode = node->getFirstChild();
            if (childNode && childNode->getNodeType() == DOMNode::TEXT_NODE) {
              ret = getAsString(childNode->getNodeValue());
            }
          }
        }
      }
    }
  }
  return ret;
}

std::string XMLSerializer::getAttribute(const DOMElement& el, 
                                        const char *key) const
{
    string ret;
    char *cptr = 0;
    XMLCh *tmpStr = XMLString::transcode(key);
    DOMNamedNodeMap *attributes = el.getAttributes();
    DOMNode *attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;

    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      ret = cptr;
      delete [] cptr;
    }
    return ret;
}

std::string XMLSerializer::writeDoubleAsString(double value)
{
  char buf[120];
  sprintf(buf,"%lf",value);
  string ret(buf);
  return ret;
  
}

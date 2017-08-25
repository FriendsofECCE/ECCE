#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/util/XMLString.hpp>
using namespace xercesc;

#include "dsm/JCode.H"
#include "dsm/SummaryIterator.H"


SummaryIterator::SummaryIterator()
{
  p_elements = 0;
  p_nextItem = 0;
}

SummaryIterator::SummaryIterator(const DOMNodeList& nodes)
{
  p_elements = (DOMNodeList *) &nodes;
  p_nextItem = 0;
}

SummaryIterator::SummaryIterator(const SummaryIterator& rhs)
{
  p_elements = rhs.p_elements;
  p_nextItem = 0;
}

SummaryIterator::~SummaryIterator()
{
  // Xerces owns the NodeList and will clean it up
  p_elements = 0;
}

bool SummaryIterator::next
(
  string& topLabel, 
  bool& toggleSet,
  bool& noDefault,
  vector<string>& keys, 
  vector<string>& labels
)
{
  bool ret = false;

  toggleSet = false;
  noDefault = false;

  int len = p_elements->getLength();
  if (len > 0 && p_nextItem < len) {
    ret = true;
    DOMElement *summary = (DOMElement *) p_elements->item(p_nextItem++);

    // Dig out topLabel attribute
    DOMNamedNodeMap *attributes = summary->getAttributes();

    char *cptr = 0;
    XMLCh *tmpStr = XMLString::transcode("topLabel");
    DOMNode *attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;

    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      topLabel = cptr;
      delete [] cptr;
    } else {
      topLabel = "";
    }

    cptr = 0;
    tmpStr = XMLString::transcode("toggleSet");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      if (strcmp(cptr, "true") == 0)
        toggleSet = true;
      delete [] cptr;
    }

    cptr = 0;
    tmpStr = XMLString::transcode("noDefault");
    attribute = attributes->getNamedItem(tmpStr);
    delete [] tmpStr;
    if (attribute != 0) {
      cptr = XMLString::transcode(attribute->getNodeValue());
      if (strcmp(cptr, "true") == 0)
        noDefault = true;
      delete [] cptr;
    }

    // Then parse item elements
    tmpStr = XMLString::transcode("item");
    DOMNodeList *nodes = summary->getElementsByTagName(tmpStr);
    delete [] tmpStr;

    int numItems = 0;
    if (nodes != 0) numItems = nodes->getLength();
  
    // Assumes text node is first child
    if (numItems > 0) {
      DOMElement *ele = 0;
      for (int idx=0; idx<numItems; idx++) {
        ele = (DOMElement *) nodes->item(idx);
        attributes = ele->getAttributes();
        tmpStr = XMLString::transcode("key");
        attribute = attributes->getNamedItem(tmpStr);
        delete [] tmpStr;

        if (attribute != 0) {
          cptr = XMLString::transcode(attribute->getNodeValue());
          keys.push_back(cptr);
          delete [] cptr;
        }
        tmpStr = XMLString::transcode("label");
        attribute = attributes->getNamedItem(tmpStr);
        delete [] tmpStr;

        if (attribute != 0) {
          cptr = XMLString::transcode(attribute->getNodeValue());
          labels.push_back(cptr);
          delete [] cptr;
        }
      }
    }
  }
  return ret;
}

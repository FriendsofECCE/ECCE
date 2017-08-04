#include <stdlib.h> // atoi
#include <stdio.h> // sprintf
#include <string.h>
#include <string>
#include <vector>

using std::string;
using std::vector;


#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
using namespace xercesc;

#include "dsm/PropFactory.H"
#include "dsm/PropertyDoc.H" 
#include "dsm/BasicDOMParser.H"


/**
 *
 *  Constructor uses a property reference to define the 
 *  characteristics of the PropertyDoc.  A property reference
 *  state is retained so to ensure that modifiers stick to the
 *  appropriate rules when building/modifying the PropertyDoc.
 *
**/

PropertyDoc::PropertyDoc(const Property_Ref* ref) {
  p_ref = ref;
  if (!createContainer()) 
    p_ref = NULL;
}

PropertyDoc::PropertyDoc(char* doc, const bool& logErrors) {
//NEED reference to be generated!!!  p_ref = ref;
//THIS is depedent on a schema I'm kind of taking a short cut right now, 
//but we need to do validation!
//
  if (parseXmlDoc(doc, logErrors))
    p_ref = PropFactory::getPropRef(name());
  else 
    p_ref = NULL;
   
}


PropertyDoc::PropertyDoc(const char* doc, const bool& logErrors) {
//NEED reference to be generated!!!  p_ref = ref;
//THIS is depedent on a schema I'm kind of taking a short cut right now,
//but we need to do validation!
//
  if (parseXmlDoc(doc, logErrors))
    p_ref = PropFactory::getPropRef(name());
  else
    p_ref = NULL;

}


PropertyDoc::PropertyDoc() {
  p_container = 0;
}


PropertyDoc::~PropertyDoc() {
  if (p_container != 0) p_container->release();
  p_container = 0;
}

/**
 * Parse a new document.  Will release current document, if any, and
 * update the property reference.
 */
void PropertyDoc::parse(char* doc, const bool& logErrors)
{
  if (p_container != 0) {
    p_container->release();
    p_container = 0;
  }
  if (parseXmlDoc(doc, logErrors)) {
    p_ref = PropFactory::getPropRef(name());
  }
  else {
    p_ref = NULL;
  }
}


/**
 * Parse a new document.  Will release current document, if any, and
 * update the property reference.
 */
void PropertyDoc::parse(const char* doc, const bool& logErrors)
{
  if (p_container != 0) {
    p_container->release();
    p_container = 0;
  }
  if (parseXmlDoc(doc, logErrors)) {
    p_ref = PropFactory::getPropRef(name());
  }
  else {
    p_ref = NULL;
  }
}



/**
 *
 *  createContainer() uses the Property_Ref property identity to construct
 *  a shell Property XML document.  The step element creation has been 
 *  separated out to add more versatility to Time Series Property document
 *  implementations.
 *
**/
bool PropertyDoc::createContainer() {
  bool ret = true;
  XMLCh *xStr = XMLString::transcode("Core");
  DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(xStr);
  delete [] xStr;

  xStr = XMLString::transcode(p_ref->storagetype().c_str());
  p_container = impl->createDocument( 0, xStr, 0);
  delete [] xStr;

  p_container->setStandalone(true);
  // p_container->setXmlStandalone(true); // Xerces 3.x

  DOMElement *rootElement = p_container->getDocumentElement();
  //Set name
  XMLCh *attribName = XMLString::transcode("name");
  XMLCh *attribValue = XMLString::transcode(p_ref->short_name.c_str());
  rootElement->setAttribute(attribName, attribValue);
  delete [] attribName;
  delete [] attribValue;

  // Go ahead and set Units since we know this from PropFactory.
  string units = "NA";
  // units are defined in the 
  // property parse scripts.  Still go ahead and set the
  // units to a default value of "NA"
  attribName = XMLString::transcode("units");
  attribValue = XMLString::transcode(units.c_str());
  rootElement->setAttribute(attribName, attribValue);

  if (!p_container->hasChildNodes())
    ret = false;
  return ret;
}
/**
 *
 *  Is the document empty ?
 *
 *
**/

bool PropertyDoc::empty() 
{
  bool ret;
  if (p_container == 0) {
    ret = true;
  }
  else {
    ret = (!p_container->hasChildNodes());
  }
  return ret;
}
/**
 *
 * used to parse an existing property XML document.  We
 * need to have some schema validation eventually. 
 *
**/
bool PropertyDoc::parseXmlDoc(char* docString, const bool& logErrors) {
   BasicDOMParser parser;
   bool ret = false;
   string currentHref;
   const char * gMemBufId = "Calculation Property";
   MemBufInputSource* memBufIS = new MemBufInputSource (
                                           ( const XMLByte*)docString,
                                             strlen(docString),
                                             gMemBufId, false );
   p_container = parser.parse(*memBufIS, logErrors);
   delete memBufIS;
   if ( p_container != 0 && p_container->hasChildNodes() ) {
     ret = true;
   }
   return ret;
}

/**
 *
 * used to parse an existing property XML document.  We
 * need to have some schema validation eventually.
 *
**/
bool PropertyDoc::parseXmlDoc(const char* docString, const bool& logErrors) {
   BasicDOMParser parser;
   bool ret = false;
   string currentHref;
   const char * gMemBufId = "Calculation Property";
   MemBufInputSource* memBufIS = new MemBufInputSource (
                                           ( const XMLByte*)docString,
                                             strlen(docString),
                                             gMemBufId, false );
   p_container = parser.parse(*memBufIS, logErrors);
   delete memBufIS;
   if ( p_container != 0 && p_container->hasChildNodes() ) {
     ret = true;
   }
   return ret;
}


/**
 *
 * creates a step element which is a child of root element.
 *
**/
void PropertyDoc::createStep(unsigned long step) 
{
  XMLCh *type = XMLString::transcode("step");
  createDataPartition(step, type);
}

/**
 *
 * creates a matrix element which is a child of root element.
 *
**/
void PropertyDoc::createMatrix(unsigned long matrix)
{
  XMLCh *type = XMLString::transcode("matrix");
  createDataPartition(matrix, type);
  delete [] type;
}

void PropertyDoc::createDataPartition(unsigned long partition, XMLCh* type)
{
   switch(p_ref->classType)
   {
     case TProperty::PROPVALUE         :
     case TProperty::PROPSTRING        :
     case TProperty::PROPVECTOR        :
     case TProperty::PROPVECSTRING     :
     case TProperty::PROPTABLE         : break;
     case TProperty::PROPVECTABLE      : //We include VECTABLE to separate blocks of data later
     case TProperty::PROPTSTABLE       :
     case TProperty::PROPTSVECTABLE    :
     case TProperty::PROPTSVECSTRING   :
     case TProperty::PROPTSVECTOR      :
     case TProperty::PROPTSVECVECTOR   :
     {

       DOMElement *rootElement = p_container->getDocumentElement();
       char buf[80];
       sprintf(buf, "%lu", partition);
       XMLCh *attribName = XMLString::transcode("number");
       XMLCh *attribValue = XMLString::transcode(buf);

       DOMElement *valElement = p_container->createElement(type);
       valElement->setAttribute(attribName, attribValue);
       rootElement->appendChild(valElement);
       delete [] attribName;
       delete [] attribValue;
       break;
     }
     case TProperty::PROPVECGRID       :
     case TProperty::SUPER             :
       break;
   }
}


/**
 *
 * Provides the property reference by which the document rules 
 * are based upon.
 *
**/
const Property_Ref* PropertyDoc::reference(void) {
  return p_ref;
}


/**
 *
 * Returns the Property DOMDocument. 
 *
**/
DOMDocument* PropertyDoc::container() {
  return p_container;
}

/**
 *
 * Returns the last step contained in the DOMDocument.
 * This only pertains to time series properties.  If no steps
 * are found in the document a "0" value is returned.
 *
**/
unsigned long PropertyDoc::lastStep(void) 
{
  XMLCh *xStr = XMLString::transcode("step");
  DOMNodeList *steps = p_container->getElementsByTagName(xStr);
  delete [] xStr;
  unsigned long numSteps = steps->getLength();
  return numSteps;
}

/**
 *
 * Returns the last matrix contained in the DOMDocument.
 * This only pertains to time series properties.  If no steps
 * are found in the document a "0" value is returned.
 *
**/
unsigned long PropertyDoc::lastMatrix(void)
{
  XMLCh *xStr = XMLString::transcode("matrix");
  DOMNodeList *matrices = p_container->getElementsByTagName(xStr);
  delete [] xStr;
  unsigned long numMatrices = matrices->getLength();
  return numMatrices;
}


/**
 *
 * In time series properties values are stored  in a <step> 
 * element.  In non-time series properies e.g. PropValue
 * values are stored at the root element level.  This function
 * returns the element that they need to store values or 
 * other information local to the value.
 *
**/
DOMElement  *PropertyDoc::valueElement(void) 
{
  DOMElement *ret = p_container->getDocumentElement();
  switch(p_ref->classType)
  {
    case TProperty::PROPVALUE         :
    case TProperty::PROPSTRING        :
    case TProperty::PROPVECTOR        :
    case TProperty::PROPVECSTRING     :
    case TProperty::PROPTABLE         : break;
    case TProperty::PROPVECTABLE      : 
    {
      XMLCh *xStr = XMLString::transcode("matrix");
      DOMNodeList *matrices = p_container->getElementsByTagName(xStr);
      delete [] xStr;
      int numMatrices = matrices->getLength();
      if (numMatrices > 0) 
      { 
        numMatrices--;
        DOMNode *domnode = matrices->item(numMatrices);
        ret = (DOMElement*) domnode;
      }
      break;
    } 
    case TProperty::PROPTSTABLE       :
    case TProperty::PROPTSVECTABLE    :
    case TProperty::PROPTSVECSTRING   :
    case TProperty::PROPTSVECTOR      :
    case TProperty::PROPTSVECVECTOR   :
    {
      XMLCh *xStr = XMLString::transcode("step");
      DOMNodeList *steps = p_container->getElementsByTagName(xStr);
      delete [] xStr;
      int numSteps = steps->getLength();
      if (numSteps > 0) { 
        numSteps--;
        DOMNode *domnode = steps->item(numSteps);
        ret = (DOMElement*) domnode;
        break;
      }
    }
    case TProperty::PROPVECGRID       : 
    case TProperty::SUPER             :
      break;
  }
  return ret;
}

/**
 *
 *  Sets the vectorLabel attribute on the root document element. 
 *
 *
**/
void PropertyDoc::setVectorLabel(void) 
{
  XMLCh *attribName = XMLString::transcode("vectorLabel");
  XMLCh *attribValue = XMLString::transcode(p_ref->dimension(3).c_str());
  DOMElement *rootElement = p_container->getDocumentElement();
  rootElement->setAttribute(attribName, attribValue);
  delete [] attribName;
  delete [] attribValue;
}

/**
 *
 *  Sets the columnLabel attribute on the root document element. 
 *
 *
**/
void PropertyDoc::setColumnLabel(void) 
{
  XMLCh *attribName = XMLString::transcode("columnLabel");
  XMLCh *attribValue = XMLString::transcode(p_ref->dimension(2).c_str());
  DOMElement *rootElement = p_container->getDocumentElement();
  rootElement->setAttribute(attribName, attribValue);
  delete [] attribName;
  delete [] attribValue;
}

/**
 *
 *  Sets the rowLabel attribute on the root document element.
 *
 *
**/
void PropertyDoc::setRowLabel(void)
{
  XMLCh *attribName = XMLString::transcode("rowLabel");
  XMLCh *attribValue = XMLString::transcode(p_ref->dimension(1).c_str());
  DOMElement *rootElement = p_container->getDocumentElement();
  rootElement->setAttribute(attribName, attribValue);
  delete [] attribName;
  delete [] attribValue;
}


/**
 *
 *  Sets the vectorLabels attribute on the "value" element.
 *  The valueElement is a derived element which is either the
 *  last step of a time series document or the root element of 
 *  a non-time series document.
 *
**/
void PropertyDoc::vectorLabels(const char* labels) {
  XMLCh *attribName = XMLString::transcode("vectorLabels");
  XMLCh *attribValue = XMLString::transcode(labels);
  DOMElement *element = p_container->getDocumentElement();
  element->setAttribute(attribName, attribValue);
  delete [] attribName;
  delete [] attribValue;
}

/**
 *
 *  Sets the columnLabels attribute on the "value" element.
 *  The value Element is a derived element which is either the
 *  last step of a time series document or the root element of 
 *  a non-time series document.
 *
**/
void PropertyDoc::columnLabels(const char* labels) {
  XMLCh *attribName = XMLString::transcode("columnLabels");
  XMLCh *attribValue = XMLString::transcode(labels);
  DOMElement *element = p_container->getDocumentElement();;
  element->setAttribute(attribName, attribValue);
  delete [] attribName;
  delete [] attribValue;
}

/**
 *
 *  Sets the rowLabels attribute on the "value" element.
 *  The value Element is a derived element which is either the
 *  last step of a time series document or the root element of
 *  a non-time series document.
 *
**/
void PropertyDoc::rowLabels(const char* labels) {
  XMLCh *attribName = XMLString::transcode("rowLabels");
  XMLCh *attribValue = XMLString::transcode(labels);
  DOMElement *element = p_container->getDocumentElement();
  element->setAttribute(attribName, attribValue);
  delete [] attribName;
  delete [] attribValue;
}


/**
 *
 *  Sets the vectors attribute on the "value" element.
 *  The valueElement is a derived element which is either the
 *  last step of a time series document or the root element of
 *  a non-time series document.
 *
**/
void PropertyDoc::vectors(const char* size) {
  XMLCh *attribName = XMLString::transcode("vectors");
  XMLCh *attribValue = XMLString::transcode(size);
  DOMElement *element = p_container->getDocumentElement();
  element->setAttribute(attribName, attribValue);
  setVectorLabel();
  delete [] attribName;
  delete [] attribValue;
}

/**
 *
 *  Sets the units attribute on the "value" element.
 *  The valueElement is a derived element which is either the
 *  last step of a time series document or the root element of
 *  a non-time series document.
 *
**/
void PropertyDoc::units(const char* unit) {
  XMLCh *attribName = XMLString::transcode("units");
  XMLCh *attribValue = XMLString::transcode(unit);
  DOMElement *element = p_container->getDocumentElement();
  element->setAttribute(attribName, attribValue);
  delete [] attribName;
  delete [] attribValue;
}

/**
 *
 *  Sets the rows attribute on the "value" element.
 *  The valueElement is a derived element which is either the
 *  last step of a time series document or the root element of
 *  a non-time series document.
 *
**/
void PropertyDoc::rows(const char* size) {
  XMLCh *attribName = XMLString::transcode("rows");
  XMLCh *attribValue = XMLString::transcode(size);
  DOMElement *element = p_container->getDocumentElement();
  element->setAttribute(attribName, attribValue);
  setRowLabel();
  delete [] attribName;
  delete [] attribValue;
}

/**
 *
 *  Sets the columns attribute on the "value" element.
 *  The valueElement is a derived element which is either the
 *  last step of a time series document or the root element of
 *  a non-time series document.
 *
**/
void PropertyDoc::columns(const char* size) {
  XMLCh *attribName = XMLString::transcode("columns");
  XMLCh *attribValue = XMLString::transcode(size);
  DOMElement *element = p_container->getDocumentElement();
  element->setAttribute(attribName, attribValue);
  setColumnLabel();
  delete [] attribName;
  delete [] attribValue;
}

/**
 *
 *  Sets the value text node and assigns this to the "value" element. 
 *
**/
void PropertyDoc::values(const char* valueString) {
  XMLCh *domvalue = XMLString::transcode(valueString);
  DOMElement *element = valueElement();
  DOMText *entryVal =  p_container->createTextNode(domvalue);
  element->appendChild(entryVal);
  delete [] domvalue;
}

char* PropertyDoc::attribute(const char* name, DOMElement& element) 
{
  char* ret = NULL;
  char* attributeName = NULL;
  DOMNamedNodeMap *attributes = element.getAttributes();
  int attrCount = attributes->getLength();
  DOMNode *attribute = 0;

  // Now parse others
  for (int idx = 0; idx < attrCount; idx++) {
    attribute = attributes->item(idx);
    attributeName = XMLString::transcode(attribute->getNodeName());
    if (strcmp(attributeName, name) == 0) {
      ret = XMLString::transcode(attribute->getNodeValue());
      break;
    }
    delete [] attributeName;
  }
  return ret;
}

char* PropertyDoc::vectorLabels(void) 
{
  char* ret = NULL; 
  const char* aname = "vectorLabels";
  DOMElement *rootElement = p_container->getDocumentElement();
  ret = attribute(aname, *rootElement);
  return ret;
}

char* PropertyDoc::units(void)
{
  char* ret = NULL;
  const char* aname = "units";
  DOMElement *rootElement = p_container->getDocumentElement();
  ret = attribute(aname, *rootElement);
  return ret;
}

char* PropertyDoc::name(void)
{
  char* ret = NULL;
  const char* aname = "name";
  DOMElement *rootElement = p_container->getDocumentElement();
  ret = attribute(aname, *rootElement);
  return ret;
}

char* PropertyDoc::columnLabels(void)
{
  char* ret = NULL;
  const char* aname = "columnLabels";
  DOMElement *rootElement = p_container->getDocumentElement();
  ret = attribute(aname, *rootElement);
  return ret;
}

char* PropertyDoc::rowLabels(void)
{
  char* ret = NULL;
  const char* aname = "rowLabels";
  DOMElement *rootElement = p_container->getDocumentElement();
  ret = attribute(aname, *rootElement);
  return ret;
}

char* PropertyDoc::vectors(void)
{
  char* ret = NULL;
  const char* aname = "vectors";
  DOMElement *rootElement = p_container->getDocumentElement();
  ret = attribute(aname, *rootElement);
  return ret;
}

char* PropertyDoc::rows(void)
{
  char*  ret = NULL;
  const char* aname = "rows";
  DOMElement *rootElement = p_container->getDocumentElement();
  ret = attribute(aname, *rootElement);
  return ret;
}

char* PropertyDoc::columns(void)
{
  char* ret = NULL;
  const char* aname = "columns";
  DOMElement *rootElement = p_container->getDocumentElement();
  ret = attribute(aname, *rootElement);
  return ret;
}

char* PropertyDoc::vectorLabel(void)
{
  char* ret = NULL;
  const char* aname = "vectorLabel";
  DOMElement *rootElement = p_container->getDocumentElement();
  ret = attribute(aname, *rootElement);
  return ret;
}

char* PropertyDoc::rowLabel(void)
{
  char* ret = NULL;
  const char* aname = "rowLabel";
  DOMElement *rootElement = p_container->getDocumentElement();
  ret = attribute(aname, *rootElement);
  return ret;
}

char* PropertyDoc::columnLabel(void)
{
  char* ret = NULL;
  const char* aname = "columnLabel";
  DOMElement *rootElement = p_container->getDocumentElement();
  ret = attribute(aname, *rootElement);
  return ret;
}
char* PropertyDoc::values(unsigned long index) 
{
  char* ret = NULL;
  DOMElement *rootElement = p_container->getDocumentElement();
   switch(p_ref->classType)
   {
     case TProperty::PROPVALUE         :
     case TProperty::PROPSTRING        :
     case TProperty::PROPVECTOR        :
     case TProperty::PROPVECSTRING     :
     case TProperty::PROPTABLE         :
     {
       DOMNode *child = rootElement->getFirstChild();
       ret = XMLString::transcode(child->getNodeValue());
       break;
     }
     case TProperty::PROPVECTABLE      : 
     {
        XMLCh *xStr = XMLString::transcode("matrix");
        DOMNodeList *matrices = p_container->getElementsByTagName(xStr);
        delete [] xStr;
        int numMatrices = matrices->getLength();
        if (index < numMatrices) {
          DOMElement *matrixElement = (DOMElement*) matrices->item(index);
          if (matrixElement->hasChildNodes()) {
            DOMNode *child = matrixElement->getFirstChild();
            if (child->getNodeType() == DOMNode::TEXT_NODE)
              ret = XMLString::transcode(child->getNodeValue());
          }
        }
        break;
     }
     case TProperty::PROPTSTABLE       :
     case TProperty::PROPTSVECTABLE    :
     case TProperty::PROPTSVECSTRING   :
     {
       XMLCh *xStr = XMLString::transcode("step");
       DOMNodeList *steps = p_container->getElementsByTagName(xStr);
       delete [] xStr;
       int numSteps = steps->getLength();
       if (index < numSteps) {
         DOMElement *stepElement = (DOMElement*) steps->item(index);
         if (stepElement->hasChildNodes()) {
           DOMNode *child = stepElement->getFirstChild();
           if (child->getNodeType() == DOMNode::TEXT_NODE)
             ret = XMLString::transcode(child->getNodeValue());
         }
       }
       break;
     }
     case TProperty::PROPTSVECTOR      :
     case TProperty::PROPTSVECVECTOR   : 
     {
        XMLCh *xStr = XMLString::transcode("step");
        DOMNodeList *steps = p_container->getElementsByTagName(xStr);
        delete [] xStr;
        int numSteps = steps->getLength();
        DOMElement *stepElement = 0;
        if (index < numSteps) {
          stepElement = (DOMElement*) steps->item(index);
          if (stepElement->hasChildNodes()) {
            DOMNode *child = stepElement->getFirstChild();
            if (child->getNodeType() == DOMNode::TEXT_NODE)
              ret = XMLString::transcode(child->getNodeValue());
          } 
        } 
        break;
     }
     case TProperty::PROPVECGRID       : 
     case TProperty::SUPER             :
        break;
  }
  return ret;
}

vector<double> PropertyDoc::doubleVector()
{
  vector<double> ret;
  char *value;
  if (p_ref->classType == TProperty::PROPTSVECTOR)
  {
    XMLCh *xStr = XMLString::transcode("step");
    DOMNodeList *steps = p_container->getElementsByTagName(xStr);
    delete [] xStr;
    int numSteps = steps->getLength();
    DOMElement *stepElement = 0;
    DOMNode *child = 0;
    for (int index = 0; index < numSteps; index++) {   
      stepElement = (DOMElement*) steps->item(index);
      if (stepElement->hasChildNodes()) {
        child = stepElement->getFirstChild();
        value = XMLString::transcode(child->getNodeValue());
        ret.push_back(atof(value));
        delete [] value;
      }
    }
  }
  return ret;
}

void PropertyDoc::parseValues(char* values, vector<double>& dvec) 
{
  char* token;
  token = strtok(values," \n\r"); 
  while (token) { 
    dvec.push_back(atof(token)); 
    token = strtok(NULL," \n\r"); 
  } 
}

vector<vector<double> >* PropertyDoc::doubleVecTable() 
{
  vector<vector<double> >* ret = NULL; 
  string type; 
  if ((p_ref->classType == TProperty::PROPTSVECTABLE) || 
      (p_ref->classType == TProperty::PROPVECTABLE)) 
  {
    if (p_ref->classType == TProperty::PROPTSVECTABLE)
      type = "step";
    else
      type = "matrix"; 
    vector<double> tableValues;
    XMLCh *xStr = XMLString::transcode(type.c_str());
    DOMNodeList *dataNodes = p_container->getElementsByTagName(xStr);
    delete [] xStr;
    int numNodes = dataNodes->getLength();
    if (numNodes > 0) 
    {
      ret = new vector<vector<double> >; 
      DOMElement *dataElement = 0;
      DOMNode *child = 0;
      char* values;
      for (int index = 0; index < numNodes; index++) {
        tableValues.clear();
        dataElement = (DOMElement*) dataNodes->item(index);
        if (dataElement->hasChildNodes()) {
          child = dataElement->getFirstChild();
          values = XMLString::transcode(child->getNodeValue());
          parseValues(values, tableValues); 
          delete [] values;
          ret->push_back(tableValues); 
        }
      }
    }
  }
  return ret;
}


vector<double> PropertyDoc::doubleVector(unsigned long dataIndex)
{
  vector<double> ret;
  string type;
  if ((p_ref->classType == TProperty::PROPTSVECTABLE) ||
      (p_ref->classType == TProperty::PROPVECTABLE))
  {
    if (p_ref->classType == TProperty::PROPTSVECTABLE)
      type = "step";
    else
      type = "matrix";
    XMLCh *xStr = XMLString::transcode(type.c_str());
    DOMNodeList *dataNodes = p_container->getElementsByTagName(xStr);
    delete [] xStr;
    int numNodes = dataNodes->getLength();
    if ((numNodes > 0) && ( dataIndex < numNodes))
    {
      DOMElement *dataElement = 0;
      DOMNode *child = 0;
      char* values;
      dataElement = (DOMElement*) dataNodes->item(dataIndex);
      if (dataElement->hasChildNodes()) {
        child = dataElement->getFirstChild();
        values = XMLString::transcode(child->getNodeValue());
        parseValues(values, ret);
        delete [] values;
      }
    }
  }
  return ret;
}

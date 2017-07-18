#include <stdio.h>
#include <limits.h>
#include <float.h>

#include <strstream>
  using std::istrstream;
  using std::ostrstream;

#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeFilter.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>

#include <string>

using std::string;

#include "util/StringConverter.H"
#include "util/ErrMsg.H"
#include "tdat/FFDesc.H"
#include "tdat/FFCategoryDesc.H"
#include "tdat/FFParameterDesc.H"

#include "dsm/BasicDOMParser.H"
#include "dsm/FFDescXMLSerializer.H"


FFDescXMLSerializer::FFDescXMLSerializer() : XMLSerializer()
{
}

FFDescXMLSerializer::~FFDescXMLSerializer()
{
}

string FFDescXMLSerializer::mimetype() const
{
   return "text/xml/ecce-ffdesc";
}

/**
 * Its the callers responsibility to make sure the ESPConstraintModel
 * has its fragment assigned.  Its also the callers responsibility to
 * clear any existing constrains from "object".
 */
void FFDescXMLSerializer::deserialize(const string& data,
                       Serializable& object, EcceMap& properties)
                       throw (ParseException)
{
   // NOTE: For now at least, we aren't interested in any properties.
   FFDesc *dobject = dynamic_cast<FFDesc*>(&object);

   if (dobject) {

      // Parse the XML document
      BasicDOMParser parser;
      DOMDocument *doc = 0;

      // This can throw a ParseException
      istrstream is(data.c_str());
      doc = parser.parse(is);

      DOMElement *root = doc->getDocumentElement();
      string ffname = getAttribute(*root,"name");
      dobject->setName( ffname );

      XMLCh *name = XMLString::transcode("CategoryDescriptor");
      DOMNodeList *categories = root->getElementsByTagName(name);
      delete [] name;

      DOMElement *cEle = 0;
      DOMElement *child = 0;
      string elname;

      int numCategories = categories->getLength();

      for (int idx=0; idx<numCategories; idx++) {
         cEle = (DOMElement *) categories->item(idx);
         string catname = getAttribute(*cEle,"type");
         FFCategoryDesc *cdesc = new FFCategoryDesc( );
         cdesc->setName( catname );

         // Parse category simple elements (name, label, formula, order);
         DOMNodeList *cfields = cEle->getChildNodes();
         string value;
         int numCFields = cfields->getLength();
         for (int cfdx=0; cfdx<numCFields; cfdx++) {
            DOMNode *catnode = cfields->item(cfdx);
            if (catnode->getNodeType() == DOMNode::ELEMENT_NODE) {
               elname = getAsString(catnode->getNodeName());
               child = (DOMElement*)catnode;

               // Assuming text node is first child
               DOMNode *text = child->getFirstChild();
               value = "";
               if (text) value = getAsString(text->getNodeValue());
               if (elname == "Label") {
                  cdesc->setLabel(value);
               } else if (elname == "Name") {
                  cdesc->setName(value);
               } else if (elname == "Formula") {
                  cdesc->setFormula(value);
               } else if (elname == "MultiLine") {
                  cdesc->setMultiLine(value=="true");
               } else if (elname == "Classes") {
                  string tmp = getAttribute(*child,"num");
                  cdesc->setNumAtoms(atoi(tmp.c_str()));
                  tmp = getAttribute(*child,"charlimit");
                  cdesc->setMaxAtomLength(atoi(tmp.c_str()));
                  // Get bitmasks of valid combinations
                  DOMNodeList *valCombos = child->getChildNodes();
                  int numValCombos = valCombos->getLength();
                  vector<vector<bool> > rules;
                  for (int vdx=0; vdx<numValCombos; vdx++) {
                     DOMNode *vnode = valCombos->item(vdx);
                     if (vnode->getNodeType() == DOMNode::ELEMENT_NODE) {
                       string vname = getAsString(vnode->getNodeName());
                       if (vname == "ValidCombination") {
                         DOMNode *rulestext = vnode->getFirstChild();
                         if (rulestext) {
                            string rulesstr = getAsString(rulestext->getNodeValue());
                            // At this point we should have a string that is a
                            // bit mask.  There should be a bit for 'num'
                            // entries.
                            vector<bool> bmask;
                            int numAtoms = cdesc->getNumAtoms();
                            for (int bdx=0; 
                                 bdx<numAtoms && bdx<rulesstr.size();
                                 bdx++) {
                               if (rulesstr[bdx] == '0') {
                                  bmask.push_back(false);
                               } else if (rulesstr[bdx] == '1') {
                                  bmask.push_back(true);
                               } else {
                                  throw ParseException(ParseException::WARNING,"Atom rules bitmask contains illegal characters; expectin 0s and 1s", WHERE);
                               }
                            }
                            if (bmask.size() != numAtoms) {
                                  throw ParseException(ParseException::WARNING,rulesstr + ": number of bits in bit mask incorrect.", WHERE);
                            }
                            rules.push_back(bmask);
                         }
                       }
                     }
                  }
                  cdesc->setAtomRules(rules);

               } else if (elname == "Parameters") {
                  // do nothing - handle parameters later
               } else if (elname == "DisplayOrder") {
                  vector<int> order;
                  if (!StringConverter::toIntList( value, order )) {
                     string msg = "Display Order unparsable " + value;
                     throw ParseException(ParseException::WARNING, msg, WHERE);
                  }
                  cdesc->defineDisplayOrder(order);
               } else {
                  string msg = "Unrecognized Category node ";
                  msg += getAsString(name);
                  throw ParseException(ParseException::WARNING, msg, WHERE);
               }
            }
         }

         // Parse parameter objects
         name = XMLString::transcode("Parameter");
         DOMNodeList *params = cEle->getElementsByTagName(name);
         delete [] name;

         // Parse elements within each Parameter
         int numParams = params->getLength();
         DOMElement *pEle = 0;
         for (int pdx=0; pdx<numParams; pdx++) {
            pEle = (DOMElement *) params->item(pdx);

            FFParameterDesc *paramDesc = 0;
            DOMNodeList *fields = pEle->getChildNodes();
            int numFields = fields->getLength();
            int fdx;
            //
            // This loop digs out the type so we can create our object
            //
            for (fdx=0; fdx<numFields; fdx++) {
               DOMNode *node = fields->item(fdx);
               if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
                  elname = getAsString(node->getNodeName());
                  child = (DOMElement*)node;

                  // Assuming text node is first child
                  DOMNode *text = child->getFirstChild();
                  value = "";
                  if (text) value = getAsString(text->getNodeValue());
                  if (elname == "Type") {
                     if (value == "float")
                       paramDesc = new FFParameterDesc(FFParameterDesc::eFLOAT);
                     else if (value == "int")
                       paramDesc = new FFParameterDesc(FFParameterDesc::eINT);
                     else if (value == "element")
                       paramDesc = new FFParameterDesc(FFParameterDesc::eELEMENT);
                     else if (value == "boolean")
                       paramDesc = new FFParameterDesc(FFParameterDesc::eBOOL);
                     break;
                  }
               }
            }
            if (paramDesc == 0) {
               throw ParseException(ParseException::FATAL,
                                    "No type specified", WHERE);
            }

            //
            // If we got here, we have a Paremeter so dig out the 
            // remaining data.
            //
            for (fdx=0; fdx<numFields; fdx++) {
               DOMNode *node = fields->item(fdx);
               if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
                  elname = getAsString(node->getNodeName());
                  child = (DOMElement*)node;

                  // Assuming text node is first child
                  DOMNode *text = child->getFirstChild();
                  value = "";
                  if (text) value = getAsString(text->getNodeValue());
                  if (elname == "Label") {
                     paramDesc->setLabel(value);
                  } else if (elname == "Type") {
                     // do nothing since we already processed this
                  } else if (elname == "Minimum") {
                     setParamMinimum(*paramDesc,value);
                  } else if (elname == "Maximum") {
                     setParamMaximum(*paramDesc,value);
                  } else if (elname == "Default") {
                     paramDesc->setDefaultValue(value);
                  } else if (elname == "Units") {
                    string unitclass = getAttribute(*child,"class");
                     paramDesc->setUnitClass(unitclass);
                     paramDesc->setDefaultUnit(value);
                  } else {
                    string msg = "Unregconized Parameter node ";
                    msg += getAsString(name);
                    throw ParseException(ParseException::WARNING,msg, WHERE);
                  }
               }
            }


            // TODO should we change CateogryDesc to use pointers 
            // so that we have consistency with FFDesc
            cdesc->addParamDesc(*paramDesc);
            delete paramDesc;
         }
         dobject->addCategory(cdesc);
      }
      doc->release();
   } else {
      string msg = "Dynamic cast to FFDesc failed";
      throw ParseException(ParseException::WARNING, msg, WHERE);
   }

}

/**
 * Do nothing.
 */
void FFDescXMLSerializer::serialize(const Serializable& object,
                     std::string& data, EcceMap& properties)
{
}


// Parse numeric value from string and assign to the appropriate 
// data method.
void FFDescXMLSerializer::setParamMinimum(FFParameterDesc& paramDesc, 
                                         const string& value)
{
   if (paramDesc.getType() == FFParameterDesc::eINT) {
      int v;
      if (value == "GEZero") {
        v = 0;
      } else if (value == "GTZero") {
        v = 1;
      } else if (value == "MinusInfinity") {
        v = -(INT_MAX-1);
      } else if (value == "Infinity") {
        EE_ASSERT(0, EE_FATAL,
            "Cannot use infinity as lower bound");
      } else if (value == "LEZero" || value == "LTZero") {
        EE_ASSERT(0, EE_FATAL,
            "Cannot use less than comparison as lower bound");
      } else {
         StringConverter::toInt(value,v);
      }
       paramDesc.setMinInt(v);
   } else if (paramDesc.getType() == FFParameterDesc::eFLOAT) {
      double v;
      if (value == "GEZero") {
        v = 0.0;
      } else if (value == "GTZero") {
        double epsilon = 1.0;
        while (epsilon/2.0 > 0.0) {
          epsilon /= 2.0;
        }
        v = epsilon;
      } else if (value == "MinusInfinity") {
        v = -DBL_MAX;
      } else if (value == "Infinity") {
        EE_ASSERT(0, EE_FATAL,
            "Cannot use infinity as lower bound");
      } else if (value == "LEZero" || value == "LTZero") {
        EE_ASSERT(0, EE_FATAL,
            "Cannot use less than comparison as lower bound");
      } else {
         StringConverter::toDouble(value,v);
      }
      paramDesc.setMinFloat((float)v);
   }
}

void FFDescXMLSerializer::setParamMaximum(FFParameterDesc& paramDesc, 
                                         const string& value)
{
   if (paramDesc.getType() == FFParameterDesc::eINT) {
      int v;
      if (value == "LEZero") {
        v = 0;
      } else if (value == "LTZero") {
        v = -1;
      } else if (value == "Infinity") {
        v = INT_MAX;
      } else if (value == "MinusInfinity") {
        EE_ASSERT(0, EE_FATAL,
            "Cannot use minus infinity as lower bound");
      } else if (value == "GEZero" || value == "GTZero") {
        EE_ASSERT(0, EE_FATAL,
            "Cannot use greater than comparison as lower bound");
      }
      if (StringConverter::toInt(value,v)) {
         paramDesc.setMinInt(v);
      }
      if (StringConverter::toInt(value,v)) {
         paramDesc.setMaxInt(v);
      }
   } else if (paramDesc.getType() == FFParameterDesc::eFLOAT) {
      double v;
      if (value == "LEZero") {
        v = 0.0;
      } else if (value == "LTZero") {
        double epsilon = 1.0;
        while (epsilon/2.0 > 0.0) {
          epsilon /= 2.0;
        }
        v = -epsilon;
      } else if (value == "Infinity") {
        v = DBL_MAX;
      } else if (value == "MinusInfinity") {
        EE_ASSERT(0, EE_FATAL,
            "Cannot use minus infinity as lower bound");
      } else if (value == "LEZero" || value == "LTZero") {
        EE_ASSERT(0, EE_FATAL,
            "Cannot use less than comparison as lower bound");
      }
      if (StringConverter::toDouble(value,v)) {
         paramDesc.setMaxFloat((float)v);
      }
   }
}

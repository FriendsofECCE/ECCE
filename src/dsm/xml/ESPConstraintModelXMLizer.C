#include <stdio.h>

#include <iostream>
  using std::cerr;
  using std::endl;
  using std::ends;
#include <strstream>
  using std::istrstream;
  using std::ostrstream;

#include <xercesc/util/XMLString.hpp>
//#include <xercesc/dom/DOMTreeWalker.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeFilter.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>

#include <string>

using std::string;

#include "util/EcceMap.H"
#include "util/StringTokenizer.H"
#include "tdat/TAtm.H"
#include "tdat/Fragment.H"
#include "tdat/ESPConstraintModel.H"
#include "dsm/BasicDOMParser.H"
#include "dsm/ESPConstraintModelXMLizer.H"

const string ESPConstraintModelXMLizer::ROOTELEMENT = "ESPModel";

ESPConstraintModelXMLizer::ESPConstraintModelXMLizer() : XMLSerializer()
{
}

ESPConstraintModelXMLizer::~ESPConstraintModelXMLizer()
{
}

string ESPConstraintModelXMLizer::mimetype() const
{
   return "text/xml/ecce-espmodel";
}

/**
 * Its the callers responsibility to make sure the ESPConstraintModel
 * has its fragment assigned.  Its also the callers responsibility to
 * clear any existing constrains from "object".
 */
void ESPConstraintModelXMLizer::deserialize(const string& data,
                       Serializable& object, EcceMap& properties)
                       throw (ParseException)
{
   // NOTE: For now at least, we aren't interested in any properties.
   ESPConstraintModel *dobject = dynamic_cast<ESPConstraintModel*>(&object);

   if (dobject) {

      // Parse the XML document
      BasicDOMParser parser;
      DOMDocument *doc = 0;

      // This can throw a ParseException
      istrstream is(data.c_str());
      doc = parser.parse(is);

      /* I had thought of using a tree walker but...
         DOMNode *root = doc->getDocumentElement();
         int whattoshow = DOMNodeFilter::SHOW_ALL;
         bool expandReferences = false;

         DOMTreeWalker *walker = doc->createTreeWalker(root, whattoshow, 
         (DOMNodeFilter*)0, expandReferences);
         walker->release();
       */
      DOMElement *root = doc->getDocumentElement();
      XMLCh *name = XMLString::transcode("Constraint");
      DOMNodeList *constraints = root->getElementsByTagName(name);
      delete [] name;

      const Fragment& frag = dobject->getFragment();
      int numConstraints = constraints->getLength();
      DOMElement *cEle = 0;
      DOMElement *child = 0;
      DOMNode *node = 0;
      string elname, value;
      float fval;
      for (int idx=0; idx<numConstraints; idx++) {
         cEle = (DOMElement *) constraints->item(idx);
         ESPConstraint *espconstraint = new ESPConstraint();
         DOMNodeList *children = cEle->getChildNodes();
         int numChildren = children->getLength();
         for (int cdx=0; cdx<numChildren; cdx++) {
            node = children->item(cdx);
            if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
               elname = getAsString(node->getNodeName());
               child = (DOMElement*)node;
               if (child) {
                  // Assuming text node is first child
                  DOMNode *text = child->getFirstChild();
                  value = "";
                  if (text) value = getAsString(text->getNodeValue());
                  if (elname == "Annotation") {
                     espconstraint->annotation(value);
                  } else if (elname == "Type") {
                     espconstraint->type(ESPConstraint::labelToType(value));
                  } else if (elname == "Charge") {
                     sscanf(value.c_str(),"%f",&fval);
                     espconstraint->charge(fval);
                  } else if (elname == "Group1") {
                     espconstraint->group1(groupStrToVec(frag,value));
                  } else if (elname == "Group2") {
                     espconstraint->group2(groupStrToVec(frag,value));
                  } else {
                     cerr << "Unregconized node " << name << " " << value << endl;
                  }
               }
            }
         }
         //??children->release();
         dobject->add(espconstraint);
      }
      doc->release();
   } else {
      cerr << "Dynamic cast to ESPConstraintModel failed" << endl;
   }


}

void ESPConstraintModelXMLizer::serialize(const Serializable& object,
                        string& data, EcceMap& properties)

{
   data = "";

   const ESPConstraintModel *dobject = dynamic_cast<const ESPConstraintModel*>(&object);
   if (dobject) {
      ostrstream os;
      os << xmlHeader() << endl;

      os << "<" << ROOTELEMENT << ">" << endl;

      // Add consraints
      int numConstraints = dobject->size();
      for (int idx=0; idx<numConstraints; idx++ ) {
         ESPConstraint *constraint = dobject->get(idx);
         os << "<Constraint>" << endl;
         os << "<Type>" << 
            ESPConstraint::typeToLabel(constraint->type()) << 
            "</Type>\n";
         os << "<Group1>" << groupToStr(constraint->group1()) << "</Group1>\n";
         if (constraint->group2Applies()) {
            os << "<Group2>" << groupToStr(constraint->group2()) << "</Group2>\n";
         }
         if (constraint->chargeApplies()) {
            os << "<Charge>" << constraint->charge() << "</Charge>\n";
         }
         os << "<Annotation>" << constraint->annotation() << "</Annotation>\n";
         os << "</Constraint>" << endl;
      }

      // Add radius specifications (TODO when added to model)

      os << "</" << ROOTELEMENT << ">" << endl;
      os << ends;

      data = os.str();
   } else {
      cerr << "Dynamic cast to ESPConstraintModel failed" << endl;
   }
}

/**
 * Creates a space separated string of atom indices that represent
 * the indices of the atoms in the group.
 */
string ESPConstraintModelXMLizer::groupToStr(const vector<TAtm*>& g) const
{
   string ret;
   ostrstream os;
   int numAtoms = g.size();
   for (int idx=0; idx< numAtoms; idx++) {
      os << g[idx]->index() << " ";
   }
   os << ends;

   // Now convert to string
   ret = os.str();

   return ret;
}

vector<TAtm*> ESPConstraintModelXMLizer::groupStrToVec(const Fragment& frag, 
                                                      const string& str) const
{
   vector<TAtm*> ret;
   StringTokenizer tizer(str);
   int atom;
   vector<string> tokens = tizer.tokenize(" \t\n");
   int numAtoms = frag.numAtoms();
   for (int idx=0; idx<tokens.size(); idx++) {
      sscanf(tokens[idx].c_str(),"%d",&atom);
      if (atom < numAtoms) ret.push_back(frag.atomRef(atom));
   }
   return ret;
}

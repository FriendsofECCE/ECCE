/**
* @file
*
*
*/
        

#include <iostream>
  using std::cerr;
  using std::endl;
  using std::ends;
#include <strstream>
  using std::istrstream;
  using std::ostrstream;

#include <stdio.h>

#include <xercesc/util/XMLString.hpp>
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
#include "tdat/GeomConstraintModel.H"
#include "tdat/GeomConstraint.H"
#include "tdat/GeomRestraint.H"
#include "dsm/BasicDOMParser.H"
#include "dsm/GeomConstraintModelXMLizer.H"




/**
 * Root element of the geometry constraints xml document
 */
const string GeomConstraintModelXMLizer::ROOTELEMENT = "GeomConstraintModel";



GeomConstraintModelXMLizer::GeomConstraintModelXMLizer() : XMLSerializer()
{
}


GeomConstraintModelXMLizer::~GeomConstraintModelXMLizer()
{
}


/** 
 * Returns MIME type for geometry constraint resources.
 * @todo This is currently not used, should probably 
 *       change to use this when retrieving constraints.
 */
string GeomConstraintModelXMLizer::mimetype() const
{
   return "chemical/x-geom-constraint";
}


/**
 * Transfer constraints from xml to a GeomConstraintModel object.
 * 
 * @param data string containing xml data
 * @param object A reference to a GeomConstraintModel object
 * @param properties A reference to an EcceMap of propertie, currently
 *                   properties are ignored.
 * @throw xerces may throw a ParseException
 */
void GeomConstraintModelXMLizer::deserialize(const string& data,
                       Serializable& object, EcceMap& properties)
                       throw (ParseException)
{

   // NOTE: For now at least, we aren't interested in any properties.
   GeomConstraintModel *dobject = dynamic_cast<GeomConstraintModel*>(&object);

   if (dobject) {

      // Parse the XML document
      BasicDOMParser parser;
      DOMDocument *doc = 0;
      // This can throw a ParseException
      istrstream is(data.c_str());
      doc = parser.parse(is);

      // Get Root element
      DOMElement *root = doc->getDocumentElement();

      // Get Constraint elements
      XMLCh *constraintName = XMLString::transcode("Constraint");
      DOMNodeList *constraints = root->getElementsByTagName(constraintName);
      delete [] constraintName;
      deserializeConstraints(dobject, constraints);

      // Get Restraint elements
      XMLCh *restraintName = XMLString::transcode("Restraint");
      DOMNodeList *restraints = root->getElementsByTagName(restraintName);
      delete [] restraintName;
      deserializeConstraints(dobject, restraints);

      doc->release();
   } 
   else {
      cerr << "Dynamic cast to GeomConstraintModel failed" << endl;
   }
}


/**
 * Deserializes a list of constraints.  Used by deserialize method to 
 * separate the deserialization of restraints and constraints.
 * 
 * @param dobject pointer to a GeomConstraintmodel
 * @param constraints a DOMNodeList of constraint/restraint pointers.
 */
void GeomConstraintModelXMLizer::deserializeConstraints(
                                   GeomConstraintModel *dobject, 
                                   DOMNodeList *constraints)
{
  const Fragment& frag = dobject->getFragment();
  int numConstraints = constraints->getLength();
  DOMElement *cEle = 0;
  DOMElement *child = 0;
  DOMNode *node = 0;
  string elname, value;
  for (int idx=0; idx<numConstraints; idx++) {
    bool isRestraint = false;
    vector<int> atomIndices;
    double forceConstant = 0.0;
    double constraintValue = 0.0;
    cEle = (DOMElement *) constraints->item(idx);
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
             if (text) {
               value = getAsString(text->getNodeValue());
             }
             if (elname == "Group") {
                atomIndices = groupStrToVec(frag,value);
             } 
             else if (elname == "ForceConstant") {
                sscanf(value.c_str(),"%lf",&forceConstant);
                isRestraint = true;
             } 
             else if (elname == "Value") {
                sscanf(value.c_str(),"%lf",&constraintValue);
             }
             else {
                cerr << "Unregconized node " << elname << " " << value << endl;
             }
          }
       }
    }

    // Add Constraint/Restraint to Model
    vector<TAtm *> *atoms = dobject->indicesToAtoms(atomIndices);
    int numAtoms = atoms->size();
    if ( (numAtoms >= 1) && (numAtoms <= 4) ) {

      // Fixed Atoms
      if (numAtoms == 1) {
        GeomConstraint *geomconstraint = new GeomConstraint((*atoms)[0]); 
        try {
          dobject->addConstraint(geomconstraint);
        }
        catch (InvalidException& ex) {
          cerr << "Add Fixed Atom Constraint Failed :: " << ex.what() << endl;
          delete geomconstraint;
        }
      }

      // Bond Constraint/Restraint
      if (numAtoms == 2) {
        if (!isRestraint) {
          GeomConstraint *geomconstraint = new GeomConstraint((*atoms)[0], (*atoms)[1]); 
          geomconstraint->setConstraintValue(constraintValue);
          try {
            dobject->addConstraint(geomconstraint);
          }
          catch (InvalidException& ex) {
            cerr << "Add Bond Constraint Failed :: " << ex.what() << endl;
            delete geomconstraint;
          }
        }
        else {
          GeomRestraint *geomrestraint = new GeomRestraint((*atoms)[0], (*atoms)[1]); 
          geomrestraint->setRestraintValue(forceConstant);
          geomrestraint->setConstraintValue(constraintValue);
          try {
            dobject->addConstraint(geomrestraint);
          }
          catch (InvalidException& ex) {
            cerr << "Add Bond Restraint Failed :: " << ex.what() << endl;
            delete geomrestraint;
          }
        }
      }

      // Angle Constraint/Restraint
      if (numAtoms == 3) {
        if (!isRestraint) {
          GeomConstraint *geomconstraint = new GeomConstraint((*atoms)[0], (*atoms)[1], (*atoms)[2]); 
          geomconstraint->setConstraintValue(constraintValue);
          try {
            dobject->addConstraint(geomconstraint);
          }
          catch (InvalidException& ex) {
            cerr << "Add Angle Constraint Failed :: " << ex.what() << endl;
            delete geomconstraint;
          }
        }
        else {
          GeomRestraint *geomrestraint = new GeomRestraint((*atoms)[0], (*atoms)[1], (*atoms)[2]); 
          geomrestraint->setRestraintValue(forceConstant);
          geomrestraint->setConstraintValue(constraintValue);
          try {
            dobject->addConstraint(geomrestraint);
          }
          catch (InvalidException& ex) {
            cerr << "Add Angle Restraint Failed :: " << ex.what() << endl;
            delete geomrestraint;
          }
        }
      }

      // Torsion Constraint/Restraint
      if (numAtoms == 4) {
        if (!isRestraint) {
          GeomConstraint *geomconstraint = new GeomConstraint((*atoms)[0], (*atoms)[1], (*atoms)[2], (*atoms)[3]); 
          geomconstraint->setConstraintValue(constraintValue);
          try {
            dobject->addConstraint(geomconstraint);
          }
          catch (InvalidException& ex) {
            cerr << "Add Torsion Constraint Failed :: " << ex.what() << endl;
            delete geomconstraint;
          }
        }
        else {
          GeomRestraint *geomrestraint = new GeomRestraint((*atoms)[0], (*atoms)[1], (*atoms)[2], (*atoms)[3]); 
          geomrestraint->setRestraintValue(forceConstant);
          geomrestraint->setConstraintValue(constraintValue);
          try {
            dobject->addConstraint(geomrestraint);
          }
          catch (InvalidException& ex) {
            cerr << "Add Torsion Restraint Failed :: " << ex.what() << endl;
            delete geomrestraint;
          }
        }
      }
    }
    else {
      cerr << "Incorrect number of atom indices for a Constraint/Restraint " << endl;
    }
    delete atoms;
  }
}



/**
 * Transfer constraints from a GeomConstraintModel object to xml.
 * 
 * @param object A reference to a GeomConstraintModel object
 * @param data reference to string containing xml data 
 * @param properties A reference to an EcceMap of propertie, currently
 *                   properties are ignored.
 */
void GeomConstraintModelXMLizer::serialize(const Serializable& object,
                        string& data, EcceMap& properties)

{
   data = "";
   const GeomConstraintModel *dobject = 
             dynamic_cast<const GeomConstraintModel*>(&object);

   if (dobject) {

      // Begin serialization
      ostrstream os;
      os << xmlHeader() << endl << endl;
      os << "<" << ROOTELEMENT << ">" << endl << endl;

      // Get/Add Fixed Constraints
      vector<GeomConstraint *> *atomConstraints = dobject->getAtomConstraints();
      if (atomConstraints->size() > 0) {
        os << serializeConstraints(atomConstraints) << endl;
      }
      delete atomConstraints;

      // Get/Add Bond Constraints
      vector<GeomConstraint *> *bondConstraints = dobject->getBondConstraints();
      if (bondConstraints->size() > 0) {
        os << serializeConstraints(bondConstraints) << endl;
      }
      delete bondConstraints;

      // Get/Add Angle Constraints
      vector<GeomConstraint *> *angleConstraints = dobject->getAngleConstraints();
      if (angleConstraints->size() > 0) {
        os << serializeConstraints(angleConstraints) << endl;
      }
      delete angleConstraints;

      // Get/Add Torsion Constraints
      vector<GeomConstraint *> *torsionConstraints = dobject->getTorsionConstraints();
      if (torsionConstraints->size() > 0) {
        os << serializeConstraints(torsionConstraints) << endl;
      }
      delete torsionConstraints;
      
      // End serialization
      os << "</" << ROOTELEMENT << ">" << endl;
      os << ends;
      data = os.str();
   } 
   else {
      cerr << "Dynamic cast to GeomConstraintModel failed" << endl;
   }
}


/**
 * Serializes a list of constraints.  Used by serialize method to 
 * separate the serialization of constraints types.
 * 
 * @param constraints vector of GeomConstraint pointers 
 *
 * @return Returns xml for list of constraints.
 */
string GeomConstraintModelXMLizer::serializeConstraints(vector<GeomConstraint *> *constraints)
{
  string ret = "";
  ostrstream os;
  int numConstraints = constraints->size();
  for (int idx=0; idx<numConstraints; idx++ ) {
    
    GeomConstraint *constraint = (*constraints)[idx];

    // Get Type of Constraint
    string constraintType = typeToStr(*constraint); 

    // Get space separated list of Atom indices
    vector<TAtm *> *atoms = constraint->getAtoms();
    string atomIndices = groupToStr(*atoms);

    // Get Constraint Value
    double constraintValue = constraint->getConstraintValue();

    // Determine if it is a Constraint or a Restraint
    string elemName = "";
    double forceConstant = 0.0;
    GeomRestraint *restraint;
    restraint = dynamic_cast<GeomRestraint*>(constraint);
    bool isRestraint = false;
    if (restraint != 0) {
      elemName = "Restraint";
      forceConstant = restraint->getRestraintValue();
      isRestraint = true;
    }
    else {
      elemName = "Constraint";
    }
    
    // Serialize the Constraint
    os << "<" << elemName << " type=\"" << constraintType << "\">" << endl;
    os << "  <Group>";
    os << atomIndices;
    os << "  </Group> " << endl;
    
    if (constraintType != "Fixed") {
      os << "  <Value>";
      os << constraintValue;
      os << "  </Value>" << endl;
    }

    if (isRestraint) {
      os << "  <ForceConstant>";
      os << forceConstant;
      os << "  </ForceConstant>" << endl;
    }

    os << "</" << elemName << ">" << endl << endl;

  }

  // Get and return serialization results
  os << ends;
  ret = os.str();
  return ret;
}


/**
 * Creates constraint type string.  The constraint type is determined 
 * by the number of atoms involved in the constraint.  
 * <ol>
 *   <li>Fixed Atom</li>
 *   <li>Bond</li>
 *   <li>Angle</li>
 *   <li>Torsion</li>
 * </ol>
 *
 * @return The string "Unknown" is returned if the constraint type 
 *         cannot be determined based on the number of atoms.
 */
string GeomConstraintModelXMLizer::typeToStr(const GeomConstraint& constraint) const
{
   string ret = "";
   int numAtoms = constraint.getNumAtoms();
   switch (numAtoms) {
     case 1  :
       ret = "Fixed";
       break;
     case 2  :
       ret = "Bond";
       break;
     case 3  :
       ret = "Angle";
       break;
     case 4  :
       ret = "Torsion";
       break;
     default :
       ret = "Unknown";
   }
   return ret;
}


/**
 * Creates a space separated string of atom indices that represent
 * the indices of the atoms in the group.
 *
 * @param g reference to a vector of TAtm pointers
 *
 * @return returns a space separated list of atom indices.
 */
string GeomConstraintModelXMLizer::groupToStr(const vector<TAtm*>& g) const
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


/**
 * Creates a vector of indices (int) that represent indices 
 * of a space separated string of indices.  Each index is 
 * checked to ensure that it is valid by making sure that
 * it is less than the total number of indices in the 
 * fragment.
 *
 * @param frag reference to a Fragment, used to determine total number
 *             of atom indices.
 * @param str string of space separated list of indices.
 *
 * @return returns a vector of indices created from a space
 *         separated list of indices.  If the atom index is greater
 *         then the total number of atoms in the fragment - it
 *         is not included in the return vector.
 */
vector<int> GeomConstraintModelXMLizer::groupStrToVec(const Fragment& frag, 
                                                      const string& str) const
{
   vector<int> ret;
   StringTokenizer tizer(str);
   int atom;
   vector<string> tokens = tizer.tokenize(" \t\n");
   int numAtoms = frag.numAtoms();
   for (int idx=0; idx<tokens.size(); idx++) {
      sscanf(tokens[idx].c_str(),"%d",&atom);
      if (atom < numAtoms) ret.push_back(atom);
   }
   return ret;
}

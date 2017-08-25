/**
* @file
*
*
*/

#include <iostream>
  using std::cerr;
  using std::endl;
  using std::ends;
  using std::cout;
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
#include "tdat/MetaPotentialModel.H"
#include "tdat/MetaPotential.H"
#include "tdat/GeomRestraint.H"
#include "dsm/BasicDOMParser.H"
#include "dsm/MetaPotentialModelXMLizer.H"


/**
 * Root element of the meta-potentials xml document
 */
const string MetaPotentialModelXMLizer::ROOTELEMENT = "MetaPotentialModel";


MetaPotentialModelXMLizer::MetaPotentialModelXMLizer() : XMLSerializer()
{
}


MetaPotentialModelXMLizer::~MetaPotentialModelXMLizer()
{
}


/** 
 * Returns MIME type for meta-potential resources.
 * @todo This is currently not used, should probably 
 *       change to use this when retrieving potentials.
 */
string MetaPotentialModelXMLizer::mimetype() const
{
   return "chemical/x-meta-potential";
}


/**
 * Transfer potentials from xml to a MetaPotentialModel object.
 * 
 * @param data string containing xml data
 * @param object A reference to a MetaPotentialModel object
 * @param properties A reference to an EcceMap of propertie, currently
 *                   properties are ignored.
 * @throw xerces may throw a ParseException
 */
void MetaPotentialModelXMLizer::deserialize(const string& data,
                       Serializable& object, EcceMap& properties)
                       throw (ParseException)
{

   // NOTE: For now at least, we aren't interested in any properties.
   MetaPotentialModel *dobject = dynamic_cast<MetaPotentialModel*>(&object);

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
      XMLCh *potentialName = XMLString::transcode("MetaPotential");
      DOMNodeList *potentials = root->getElementsByTagName(potentialName);
      delete [] potentialName;
      deserializePotentials(dobject, potentials);

      doc->release();
   } 
   else {
      cerr << "Dynamic cast to MetaPotentialModel failed" << endl;
   }
}


/**
 * Deserializes a list of potentials.  Used by deserialize method to 
 * separate the deserialization of potentials.
 * 
 * @param dobject pointer to a MetaPotentialmodel
 * @param potentials a DOMNodeList of potential pointers.
 */
void MetaPotentialModelXMLizer::deserializePotentials(
                                   MetaPotentialModel *dobject, 
                                   DOMNodeList *potentials)
{
  const Fragment& frag = dobject->getFragment();
  int numPotentials = potentials->getLength();
  DOMElement *cEle = 0;
  DOMElement *child = 0;
  DOMNode *node = 0;
  string elname, value;
  for (int idx=0; idx<numPotentials; idx++) {
    vector<int> atomIndices;
    vector<double> potentialParams;
    cEle = (DOMElement *) potentials->item(idx);
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
             else if (elname == "Values") {
                potentialParams = rGroupStrToVec(value);
             }
             else {
                cerr << "Unregcognized node " << elname << " " << value << endl;
             }
          }
       }
    }

    // Add MetaPotential to Model
    int numAtoms = atomIndices.size();
    int numValues = potentialParams.size();
    if (numAtoms == 2 || numAtoms == 3) {

      // 2 or 3-site potential
      MetaPotential *metapotential = new MetaPotential(numAtoms,
          &atomIndices, numValues, &potentialParams); 
      try {
        dobject->addPotential(metapotential);
      }
      catch (InvalidException& ex) {
        cerr << "Add meta-Potential Failed :: " << ex.what() << endl;
        delete metapotential;
      }

    }
    else {
      cerr << "Incorrect number of atom indices for a meta-Potential " << endl;
    }
  }
}



/**
 * Transfer potentials from a MetaPotentialModel object to xml.
 * 
 * @param object A reference to a MetaPotentialModel object
 * @param data reference to string containing xml data 
 * @param properties A reference to an EcceMap of propertie, currently
 *                   properties are ignored.
 */
void MetaPotentialModelXMLizer::serialize(const Serializable& object,
                        string& data, EcceMap& properties)

{
   data = "";
   const MetaPotentialModel *dobject = 
             dynamic_cast<const MetaPotentialModel*>(&object);

   if (dobject) {

      // Begin serialization
      ostrstream os;
      os << xmlHeader() << endl << endl;
      os << "<" << ROOTELEMENT << ">" << endl << endl;
      
      // Get and Add meta-Potentials
      vector<MetaPotential*> *twoSites = dobject->get2SitePotentials();
      if (twoSites->size() > 0) {
        os << serializePotentials(twoSites)<<endl;
      }
      delete twoSites;

      vector<MetaPotential*> *threeSites = dobject->get3SitePotentials();
      if (threeSites->size() > 0) {
        os << serializePotentials(threeSites)<<endl;
      }
      delete threeSites;

      // End serialization
      os << "</" << ROOTELEMENT << ">" << endl;
      os << ends;
      data = os.str();
   } 
   else {
      cerr << "Dynamic cast to MetaPotentialModel failed" << endl;
   }
}


/**
 * Serializes a list of potentials.  Used by serialize method to 
 * separate the serialization of potentials types.
 * 
 * @param potentials vector of MetaPotential pointers 
 *
 * @return Returns xml for list of potentials.
 */
string MetaPotentialModelXMLizer::serializePotentials(vector<MetaPotential *> *potentials)
{
  string ret = "";
  ostrstream os;
  int numPotentials = potentials->size();
  for (int idx=0; idx<numPotentials; idx++ ) {
    
    MetaPotential *potential = (*potentials)[idx];

    // Get Type of Constraint
    string potentialType = typeToStr(*potential); 

    // Get space separated list of Atom indices
    vector<int> *atoms = potential->getAtoms();
    string atomIndices = groupToStr(*atoms);
    delete atoms;

    // Get space separated list of potential parameters
    vector<double> *params = potential->getParams();
    string potentialValues = rGroupToStr(*params);
    delete params;

    // Serialize the Potential
    os << "<MetaPotential type=\"" << potentialType << "\">" << endl;
    os << "  <Group>";
    os << atomIndices;
    os << "  </Group> " << endl;
    
    os << "  <Values>";
    os << potentialValues;
    os << "  </Values>" << endl;
    os << "</MetaPotential>" << endl;
  }

  // Get and return serialization results
  os << ends;
  ret = os.str();
  return ret;
}


/**
 * Creates potential type string.  The potential type is determined 
 * by the number of atoms involved in the potential.  
 * <ol>
 *   <li>Fixed Atom</li>
 *   <li>Bond</li>
 *   <li>Angle</li>
 *   <li>Torsion</li>
 * </ol>
 *
 * @return The string "Unknown" is returned if the potential type 
 *         cannot be determined based on the number of atoms.
 */
string MetaPotentialModelXMLizer::typeToStr(const MetaPotential& potential) const
{
   string ret = "";
   int numAtoms = potential.getNumAtoms();
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
string MetaPotentialModelXMLizer::groupToStr(const vector<int>& g) const
{
   string ret;
   ostrstream os;
   int numAtoms = g.size();
   for (int idx=0; idx< numAtoms; idx++) {
      // convert to 1-based indexing
      os << g[idx]+1 << " ";
   }
   os << ends;

   // Now convert to string
   ret = os.str();

   return ret;
}


/**
 * Creates a space separated string of real values from a vector of doubles
 *
 * @param r reference to a vector of double precision values
 *
 * @return returns a space separated list of atom indices.
 */
string MetaPotentialModelXMLizer::rGroupToStr(const vector<double>& r) const
{
   string ret;
   ostrstream os;
   int numR = r.size();
   for (int idx=0; idx< numR; idx++) {
      os << r[idx] << " ";
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
vector<int> MetaPotentialModelXMLizer::groupStrToVec(const Fragment& frag, 
                                                      const string& str) const
{
   vector<int> ret;
   StringTokenizer tizer(str);
   int atom;
   vector<string> tokens = tizer.tokenize(" \t\n");
   int numAtoms = frag.numAtoms();
   for (int idx=0; idx<tokens.size(); idx++) {
      sscanf(tokens[idx].c_str(),"%d",&atom);
      // convert to 0-based indexing
      if (atom < numAtoms) ret.push_back(atom-1);
   }
   return ret;
}


/**
 * Creates a vector of double precision numbers from a space separate string of
 * values.
 *
 * @param str string of space separated double precision numbers.
 *
 * @return returns a vector of doubles created from a space
 *         separated list of numbers.
 */
vector<double> MetaPotentialModelXMLizer::rGroupStrToVec(const string& str) const
{
   vector<double> ret;
   StringTokenizer tizer(str);
   vector<string> tokens = tizer.tokenize(" \t\n");
   double rval;
   for (int idx=0; idx<tokens.size(); idx++) {
      sscanf(tokens[idx].c_str(),"%lf",&rval);
      ret.push_back(rval);
   }
   return ret;
}

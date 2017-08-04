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
#include "util/STLUtil.H"
#include "tdat/TAtm.H"
#include "tdat/Fragment.H"
#include "tdat/QMMMModel.H"
#include "tdat/QMMMParams.H"
#include "dsm/BasicDOMParser.H"
#include "dsm/QMMMModelXMLizer.H"




/**
 * Root element of the meta-potentials xml document
 */
const string QMMMModelXMLizer::ROOTELEMENT = "QMMMModel";



QMMMModelXMLizer::QMMMModelXMLizer() : XMLSerializer()
{
}


QMMMModelXMLizer::~QMMMModelXMLizer()
{
}


/** 
 * Returns MIME type for QMMM resources.
 * @todo This is currently not used, should probably 
 *       change to use this when retrieving QMMM parameters.
 */
string QMMMModelXMLizer::mimetype() const
{
   return "chemical/x-qmmm-params";
}


/**
 * Transfer potentials from xml to a QMMMModel object.
 * 
 * @param data string containing xml data
 * @param object A reference to a QMMMModel object
 * @param properties A reference to an EcceMap of propertie, currently
 *                   properties are ignored.
 * @throw xerces may throw a ParseException
 */
void QMMMModelXMLizer::deserialize(const string& data,
                       Serializable& object, EcceMap& properties)
                       throw (ParseException)
{

   // NOTE: For now at least, we aren't interested in any properties.
   QMMMModel *dobject = dynamic_cast<QMMMModel*>(&object);

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
      XMLCh *QMMMName = XMLString::transcode("QMMMParams");
      DOMNodeList *QMMMTerms = root->getElementsByTagName(QMMMName);
      delete [] QMMMName;
      deserializeQMMMTerms(dobject, QMMMTerms);

      doc->release();
   } 
   else {
      cerr << " Dynamic cast to QMMMModel failed" << endl;
   }
}


/**
 * Deserializes a list of QMMM terms.  Used by deserialize method to 
 * separate the deserialization of QMMM terms.
 * 
 * @param dobject pointer to a QMMMModel
 * @param potentials a DOMNodeList of potential pointers.
 */
void QMMMModelXMLizer::deserializeQMMMTerms(
                                   QMMMModel *dobject, 
                                   DOMNodeList *QMMMTerms)
{
  int numQMMMTerms = QMMMTerms->getLength();
  DOMElement *cEle = 0;
  DOMElement *child = 0;
  DOMNode *node = 0;
  string elname, value;
  for (int idx=0; idx<numQMMMTerms; idx++) {
    string type;
    vector<string> atomTypes;
    vector<double> potentialParams;
    cEle = (DOMElement *) QMMMTerms->item(idx);
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
             if (elname == "Type") {
               type = value;
             }
             else if (elname == "Atoms") {
                atomTypes = atomStrToVec(value);
             } 
             else if (elname == "Values") {
                potentialParams = paramStrToVec(value);
             }
             else {
                cerr << "Unrecognized node " << elname << " " << value << endl;
             }
          }
       }
    }
    STLUtil::stripLeadingAndTrailingWhiteSpace(type);

    // Add QMMMParams to Model
    int numAtoms = atomTypes.size();
    int numParams = potentialParams.size();

    // Do some quick consistency checks
    if (type == "su_vdw" && numAtoms != 1) {
      cerr << "Incorrect number of atoms for solute atom " << endl;
    }
    if (type == "sv_vdw" && numAtoms != 1) {
      cerr << "Incorrect number of atoms for solute atom " << endl;
    }
    if (type == "bond" && numAtoms != 2) {
    }
    if (type == "angle" && numAtoms != 3) {
    }
    // Don't recover parameters for particular bonds and angles. These will be
    // reset by GUI.
    QMMMParams *params;
    if (type != "bond_set" && type != "angle_set" && type != "shake") {
      params = new QMMMParams(type,numAtoms,&atomTypes,numParams,&potentialParams);
    } else {
      vector<int> atomIndices;
      int i;
      for (i=0; i<atomTypes.size(); i++) {
        atomIndices.push_back(atoi(atomTypes[i].c_str())-1);
      }
      params = new QMMMParams(type,numAtoms,&atomIndices,numParams,&potentialParams);
    }
    try {
      dobject->addQMMMTerm(params);
    } catch (InvalidException &ex) {
      cerr << "Add QMMM Params Failed :: " << ex.what() << endl;
      delete params;
    }
  }
}



/**
 * Transfer potentials from a QMMMModel object to xml.
 * 
 * @param object A reference to a QMMMModel object
 * @param data reference to string containing xml data 
 * @param properties A reference to an EcceMap of property, currently
 *                   properties are ignored.
 */
void QMMMModelXMLizer::serialize(const Serializable& object,
                        string& data, EcceMap& properties)

{
   data = "";
   const QMMMModel *dobject = 
             dynamic_cast<const QMMMModel*>(&object);
   string type;

   if (dobject) {

      // Begin serialization
      ostrstream os;
      os << xmlHeader() << endl << endl;
      os << "<" << ROOTELEMENT << ">" << endl << endl;
      
      // Get and Add QMMM terms
      // Get/Add solute atoms non-bonding terms

      type = "su_vdw";
      vector<QMMMParams*> *suVDWTerms = dobject->getQMMMTerms(type);
      if (suVDWTerms->size() > 0) {
        os << serializeQMMMTerms(suVDWTerms)<<endl;
      }
      delete suVDWTerms;

      type = "sv_vdw";
      vector<QMMMParams*> *svVDWTerms = dobject->getQMMMTerms(type);
      if (svVDWTerms->size() > 0) {
        os << serializeQMMMTerms(svVDWTerms)<<endl;
      }
      delete svVDWTerms;

      type = "pseud_pot";
      vector<QMMMParams*> *pseudPTerms = dobject->getQMMMTerms(type);
      if (pseudPTerms->size() > 0) {
        os << serializeQMMMTerms(pseudPTerms)<<endl;
      }
      delete pseudPTerms;

      type = "bond";
      vector<QMMMParams*> *BondTerms = dobject->getQMMMTerms(type);
      if (BondTerms->size() > 0) {
        os << serializeQMMMTerms(BondTerms)<<endl;
      }
      delete BondTerms;

      type = "angle";
      vector<QMMMParams*> *AngleTerms = dobject->getQMMMTerms(type);
      if (AngleTerms->size() > 0) {
        os << serializeQMMMTerms(AngleTerms)<<endl;
      }
      delete AngleTerms;

      type = "bond_set";
      BondTerms = dobject->getQMMMTerms(type);
      if (BondTerms->size() > 0) {
        os << serializeQMMMTerms(BondTerms)<<endl;
      }
      delete BondTerms;

      type = "angle_set";
      AngleTerms = dobject->getQMMMTerms(type);
      if (AngleTerms->size() > 0) {
        os << serializeQMMMTerms(AngleTerms)<<endl;
      }
      delete AngleTerms;

      type = "shake";
      vector<QMMMParams*> *shakeTerms = dobject->getQMMMTerms(type);
      if (shakeTerms->size() > 0) {
        os << serializeQMMMTerms(shakeTerms)<<endl;
      }
      delete shakeTerms;

      // End serialization
      os << "</" << ROOTELEMENT << ">" << endl;
      os << ends;
      data = os.str();
   } 
   else {
      cerr << " Dynamic cast to QMMMModel failed" << endl;
   }
}


/**
 * Serializes a list of QMMM terms.  Used by serialize method to 
 * separate the serialization of QMMM term types.
 * 
 * @param params: vector of QMMMParams pointers 
 *
 * @return Returns xml for list of potentials.
 */
string QMMMModelXMLizer::serializeQMMMTerms(vector<QMMMParams *> *params)
{
  string ret = "";
  ostrstream os;
  int numParams = params->size();
  for (int idx=0; idx<numParams; idx++ ) {
    
    QMMMParams *param = (*params)[idx];
    
    if (param->isZeroParams()) continue;

    // Get type of QMMM term 
    string type = param->getQMMMType(); 

    string atomStr;
    string potentialValues;
    int i, natoms;
    if (type != "bond_set" && type != "angle_set" && type != "shake") {
      // Get space separated list of Atom types
      vector<string> *atoms = param->getQMMMPotentials();
      natoms = param->getNumAtoms();
      atomStr = (*atoms)[0].c_str();
      for (i=1; i<natoms; i++) {
        atomStr.append(" ");
        atomStr.append((*atoms)[i].c_str());
      }
      delete atoms;
      // Get space separated list of potential parameters
      vector<double> *params = param->getParams();
      potentialValues = paramsToStr(*params);
      delete params;
    } else {
      //Get space separated list of Atom indices
      vector<int> *atm_idx = param->getAtomIndices();
      natoms = param->getNumAtoms();
      char buf[128];
      sprintf(buf,"%d",(*atm_idx)[0]+1);
      atomStr = buf;
      for (i=1; i<natoms; i++) {
        atomStr.append(" ");
        sprintf(buf,"%d",(*atm_idx)[i]+1);
        atomStr.append(buf);
      }
      delete atm_idx;
      // Get space separated list of potential parameters
      QMMMParams *parent = param->getQMMMParams();
      if (parent != (QMMMParams*)0) {
        vector<double> *params = parent->getParams();
        potentialValues = paramsToStr(*params);
        delete params;
      } else {
        // Get space separated list of potential parameters
        vector<double> *params = param->getParams();
        potentialValues = paramsToStr(*params);
        delete params;
      }
    }


    // Serialize the Potential
    os << "<QMMMParams>" << endl;
    os << "  <Type>";
    os << type;
    os << "  </Type> " << endl;
    os << "  <Atoms>";
    os << atomStr;
    os << "  </Atoms> " << endl;
    
    os << "  <Values>";
    os << potentialValues;
    os << "  </Values>" << endl;
    os << "</QMMMParams>" << endl;
  }

  // Get and return serialization results
  os << ends;
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
string QMMMModelXMLizer::paramsToStr(const vector<double>& r) const
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
 * Creates a vector of space-separated atom types from a vector of atom
 * type strings. 
 *
 * @param str string of space separated list of atom types.
 *
 * @return returns a vector of indices created from a space
 *         separated list of indices.  If the atom index is greater
 *         then the total number of atoms in the fragment - it
 *         is not included in the return vector.
 */
vector<string> QMMMModelXMLizer::atomStrToVec(const string& str) const
{
   vector<string> ret;
   StringTokenizer tizer(str);
   string atom;
   vector<string> tokens = tizer.tokenize(" \t\n");
   for (int idx=0; idx<tokens.size(); idx++) {
     atom.clear();
     STLUtil::stripLeadingAndTrailingWhiteSpace(tokens[idx]);
     atom = tokens[idx];
     int size = atom.size();
     for (int j=size; j<4; j++) {
       atom.append(" ");
     }
     ret.push_back(atom);
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
vector<double> QMMMModelXMLizer::paramStrToVec(const string& str) const
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

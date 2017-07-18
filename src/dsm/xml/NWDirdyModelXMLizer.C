/**
 * @file
 *
 *
 */

#include <iostream>
  using std::cout;
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
#include "tdat/NWDirdyModel.H"
#include "dsm/BasicDOMParser.H"
#include "dsm/NWDirdyModelXMLizer.H"
#include "dsm/Resource.H"
#include "dsm/MdTask.H"



const string NWDirdyModelXMLizer::ROOTELEMENT = "NWDirdyModel";



NWDirdyModelXMLizer::NWDirdyModelXMLizer() : XMLSerializer()
{}


NWDirdyModelXMLizer::~NWDirdyModelXMLizer()
{}


string NWDirdyModelXMLizer::mimetype() const
{
   return "text/xml/ecce-nwdirdymodel";
}


void NWDirdyModelXMLizer::deserialize(const string& data,
                       Serializable& object, EcceMap& properties)
                       throw (ParseException)
{
   // NOTE: For now at least, we aren't interested in any properties.
   NWDirdyModel *dobject = dynamic_cast<NWDirdyModel*>(&object);

   if (dobject) {

      // Parse the XML document
      BasicDOMParser parser;
      DOMDocument *doc = 0;

      // This can throw a ParseException
      istrstream is(data.c_str());
      doc = parser.parse(is);

      string value;

      DOMElement *root = doc->getDocumentElement();

      // Perform deserialization - get element values 
      value =  getTagValue(root, "AutoSymmetry");
      if (value.length() > 0) {
        dobject->setAutoSym((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "SymmetryTolerance");
      if (value.length() > 0) {
        dobject->setSymmetryTol(atof(value.c_str()));
      }
      value =  getTagValue(root, "FragmentSeparation");
      if (value.length() > 0) {
        dobject->setFragmentSep(atof(value.c_str()));
      }
      value =  getTagValue(root, "AutoZMatrix");
      if (value.length() > 0) {
        dobject->setAutoZ((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "UseSinglePoint");
      if (value.length() > 0) {
        dobject->setUseSP((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "GeneralCategory");
      if (value.length() > 0) {
        dobject->setCategory(value);
      }
      value =  getTagValue(root, "GeneralTheory");
      if (value.length() > 0) {
        dobject->setTheory(value);
      }
      value =  getTagValue(root, "SinglePointCategory");
      if (value.length() > 0) {
        dobject->setSPCategory(value);
      }
      value =  getTagValue(root, "SinglePointTheory");
      if (value.length() > 0) {
        dobject->setSPTheory(value);
      }
      value =  getTagValue(root, "GeneralExpCoeff");
      if (value.length() > 0) {
        dobject->setExpCoeff((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "SinglePointExpCoeff");
      if (value.length() > 0) {
        dobject->setSPExpCoeff((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "MemoryFlag");
      if (value.length() > 0) {
        dobject->setMemoryFlag((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "Memory");
      if (value.length() > 0) {
        dobject->setMemory(atoi(value.c_str()));
      }
      value =  getTagValue(root, "DiskFlag");
      if (value.length() > 0) {
        dobject->setDiskFlag((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "Disk");
      if (value.length() > 0) {
        dobject->setDisk(atoi(value.c_str()));
      }
      value =  getTagValue(root, "MEPIncrement");
      if (value.length() > 0) {
        dobject->setMEPIncrement(atof(value.c_str()));
      }
      value =  getTagValue(root, "SaveIncrement");
      if (value.length() > 0) {
        dobject->setSaveIncrement(atof(value.c_str()));
      }
      value =  getTagValue(root, "HessianIncrement");
      if (value.length() > 0) {
        dobject->setHessianIncrement(atof(value.c_str()));
      }
      value =  getTagValue(root, "ReducedMass");
      if (value.length() > 0) {
        dobject->setReducedMass(atof(value.c_str()));
      }
      value =  getTagValue(root, "TSTOnly");
      if (value.length() > 0) {
        dobject->setTSTOnly((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "LowerSLimit");
      if (value.length() > 0) {
        dobject->setLowerSLimit(atof(value.c_str()));
      }
      value =  getTagValue(root, "HigherSLimit");
      if (value.length() > 0) {
        dobject->setHigherSLimit(atof(value.c_str()));
      }
      value =  getTagValue(root, "SwitchSign");
      if (value.length() > 0) {
        dobject->setSwitchSign((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "IntegrationOpt");
      if (value.length() > 0) {
        dobject->setIntegrationOpt(atoi(value.c_str()));
      }
      value =  getTagValue(root, "PrintFlag");
      if (value.length() > 0) {
        dobject->setPrintFlag((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "LinearTS");
      if (value.length() > 0) {
        dobject->setLinearTS((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "LinearProd1");
      if (value.length() > 0) {
        dobject->setLinearProd1((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "LinearProd2");
      if (value.length() > 0) {
        dobject->setLinearProd2((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "LinearReact1");
      if (value.length() > 0) {
        dobject->setLinearReact1((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "LinearReact2");
      if (value.length() > 0) {
        dobject->setLinearReact2((bool)atoi(value.c_str()));
      }
      value =  getTagValue(root, "NumAtomsTS");
      if (value.length() > 0) {
        dobject->setNumAtomsTS(atoi(value.c_str()));
      }
      value =  getTagValue(root, "NumAtomsProd1");
      if (value.length() > 0) {
        dobject->setNumAtomsProd1(atoi(value.c_str()));
      }
      value =  getTagValue(root, "NumAtomsProd2");
      if (value.length() > 0) {
        dobject->setNumAtomsProd2(atoi(value.c_str()));
      }
      value =  getTagValue(root, "NumAtomsReact1");
      if (value.length() > 0) {
        dobject->setNumAtomsReact1(atoi(value.c_str()));
      }
      value =  getTagValue(root, "NumAtomsReact2");
      if (value.length() > 0) {
        dobject->setNumAtomsReact2(atoi(value.c_str()));
      }
      
      doc->release();

   } else {
      cerr << "Dynamic cast to NWDirdyModel failed" << endl;
   }
}

void NWDirdyModelXMLizer::serialize(const Serializable& object,
                        string& data, EcceMap& properties)

{
   data = "";

   const NWDirdyModel *dobject = dynamic_cast<const NWDirdyModel*>(&object);

   if (dobject) {
      ostrstream os;
      os << xmlHeader() << endl;
      os << "<" << ROOTELEMENT << ">" << endl;

      // Perform serialization - set element values
      os << "<AutoSymmetry>" << dobject->getAutoSym()
         << "</AutoSymmetry>" << endl;

      // always export symmetry tolerance because the ECCE default is
      // different than the NWChem default
      os << "<SymmetryTolerance>" << dobject->getSymmetryTol()
         << "</SymmetryTolerance>" << endl;

      if (dobject->getFragmentSep() != dobject->FRAGSEP) {
        os << "<FragmentSeparation>" << dobject->getFragmentSep()
           << "</FragmentSeparation>" << endl;
      }

      if (dobject->getAutoZ() != dobject->AUTOZ) {
        os << "<AutoZMatrix>" << dobject->getAutoZ()
           << "</AutoZMatrix>" << endl;
      }

      os << "<UseSinglePoint>" << dobject->getUseSP()
         << "</UseSinglePoint>" << endl;

      os << "<GeneralCategory>" << dobject->getCategory()
         << "</GeneralCategory>" << endl;

      os << "<GeneralTheory>" << dobject->getTheory()
         << "</GeneralTheory>" << endl;

      os << "<SinglePointCategory>" << dobject->getSPCategory()
         << "</SinglePointCategory>" << endl;

      os << "<SinglePointTheory>" << dobject->getSPTheory()
         << "</SinglePointTheory>" << endl;

      os << "<GeneralExpCoeff>" << dobject->getExpCoeff()
         << "</GeneralExpCoeff>" << endl;

      os << "<SinglePointExpCoeff>" << dobject->getSPExpCoeff()
         << "</SinglePointExpCoeff>" << endl;

      if (dobject->getMemoryFlag() != dobject->MEMORYFLAG) {
        os << "<MemoryFlag>" << dobject->getMemoryFlag()
           << "</MemoryFlag>" << endl;
      }

      if (dobject->getMemory() != dobject->MEMORY) {
        os << "<Memory>" << dobject->getMemory()
           << "</Memory>" << endl;
      }

      if (dobject->getDiskFlag() != dobject->DISKFLAG) {
        os << "<DiskFlag>" << dobject->getDiskFlag()
           << "</DiskFlag>" << endl;
      }

      if (dobject->getDisk() != dobject->DISK) {
        os << "<Disk>" << dobject->getDisk()
           << "</Disk>" << endl;
      }

      if (dobject->getMEPIncrement() != dobject->MEPINC) {
        os << "<MEPIncrement>" << dobject->getMEPIncrement()
           << "</MEPIncrement>" << endl;
      }

      if (dobject->getSaveIncrement() != dobject->SAVEINC) {
        os << "<SaveIncrement>" << dobject->getSaveIncrement()
           << "</SaveIncrement>" << endl;
      }

      if (dobject->getHessianIncrement() != dobject->HESSINC) {
        os << "<HessianIncrement>" << dobject->getHessianIncrement()
           << "</HessianIncrement>" << endl;
      }

      if (dobject->getReducedMass() != dobject->RMASS) {
        os << "<ReducedMass>" << dobject->getReducedMass()
           << "</ReducedMass>" << endl;
      }

      if (dobject->getTSTOnly() != dobject->TSTONLY) {
        os << "<TSTOnly>" << dobject->getTSTOnly()
           << "</TSTOnly>" << endl;
      }

      if (dobject->getLowerSLimit() != dobject->SLOW) {
        os << "<LowerSLimit>" << dobject->getLowerSLimit()
           << "</LowerSLimit>" << endl;
      }

      if (dobject->getHigherSLimit() != dobject->SHIGH) {
        os << "<HigherSLimit>" << dobject->getHigherSLimit()
           << "</HigherSLimit>" << endl;
      }

      os << "<SwitchSign>" << dobject->getSwitchSign()
         << "</SwitchSign>" << endl;

      if (dobject->getIntegrationOpt() != dobject->INTOPT) {
        os << "<IntegrationOpt>" << dobject->getIntegrationOpt()
           << "</IntegrationOpt>" << endl;
      }

      os << "<PrintFlag>" << dobject->getPrintFlag()
         << "</PrintFlag>" << endl;

      os << "<LinearTS>" << dobject->getLinearTS()
         << "</LinearTS>" << endl;

      os << "<LinearProd1>" << dobject->getLinearProd1()
         << "</LinearProd1>" << endl;

      os << "<LinearProd2>" << dobject->getLinearProd2()
         << "</LinearProd2>" << endl;

      os << "<LinearReact1>" << dobject->getLinearReact1()
         << "</LinearReact1>" << endl;

      os << "<LinearReact2>" << dobject->getLinearReact2()
         << "</LinearReact2>" << endl;

      os << "<NumAtomsTS>" << dobject->getNumAtomsTS()
         << "</NumAtomsTS>" << endl;

      os << "<NumAtomsProd1>" << dobject->getNumAtomsProd1()
         << "</NumAtomsProd1>" << endl;

      os << "<NumAtomsProd2>" << dobject->getNumAtomsProd2()
         << "</NumAtomsProd2>" << endl;

      os << "<NumAtomsReact1>" << dobject->getNumAtomsReact1()
         << "</NumAtomsReact1>" << endl;

      os << "<NumAtomsReact2>" << dobject->getNumAtomsReact2()
         << "</NumAtomsReact2>" << endl;

      os << "</" << ROOTELEMENT << ">" << endl;
      os << ends;

      data = os.str();

   } else {
      cerr << "Dynamic cast to NWDirdyModel failed" << endl;
   }
}

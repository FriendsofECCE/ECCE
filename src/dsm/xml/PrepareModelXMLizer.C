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
#include "util/FFConfigData.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"
#include "tdat/TAtm.H"
#include "tdat/Fragment.H"
#include "tdat/PrepareModel.H"
#include "dsm/BasicDOMParser.H"
#include "dsm/PrepareModelXMLizer.H"
#include "dsm/Resource.H"
#include "dsm/MdTask.H"

const string PrepareModelXMLizer::ROOTELEMENT = "PrepareModel";

PrepareModelXMLizer::PrepareModelXMLizer() : XMLSerializer()
{
}

PrepareModelXMLizer::~PrepareModelXMLizer()
{
}

string PrepareModelXMLizer::mimetype() const
{
   return "text/xml/ecce-preparemodel";
}

/**
 * Its the callers responsibility to make sure the PrepareModel
 * has its fragment assigned.  Its also the callers responsibility to
 * clear any existing constrains from "object".
 */
void PrepareModelXMLizer::deserialize(const string& data,
                       Serializable& object, EcceMap& properties)
                       throw (ParseException)
{
   // NOTE: For now at least, we aren't interested in any properties.
   PrepareModel *dobject = dynamic_cast<PrepareModel*>(&object);

   if (dobject) {

      // Parse the XML document
      BasicDOMParser parser;
      DOMDocument *doc = 0;

      // This can throw a ParseException
      istrstream is(data.c_str());
      doc = parser.parse(is);

      string elname, value;
      DOMElement *root = doc->getDocumentElement();
      
      // Is this first task?
      value = getTagValue(root, "IsFirstTask");
      if (value.length()>0) {
         dobject->setIsFirstTask((bool)atoi(value.c_str()));
      }

      // Get PDBSource 
      value = getTagValue(root, "PDBSource");
      if (value.length()>0) {
         dobject->setPDBSource(value.c_str());
      }

      // Get default Histidine residue
      value = getTagValue(root, "Histidine");
      if (value == "HIE") {
        dobject->setDefaultHistidine(PrepareModel::HIE);
      } else if (value == "HID") {
        dobject->setDefaultHistidine(PrepareModel::HID);
      } else if (value == "HIP") {
        dobject->setDefaultHistidine(PrepareModel::HIP);
      }

      //Get solvent settings
      value = getTagValue(root, "SetSolvent");
      if (value.length() > 0) {
        dobject->setSetSolvent((bool)atoi(value.c_str()));
      }
      value = getTagValue(root, "SolventOption");
      if (value.length() > 0) {
        dobject->setSolventOption(atoi(value.c_str()));
      }
      value = getTagValue(root, "SolventName");
      if (value.length() > 0) {
        dobject->setSolventName(value);
      }
      value = getTagValue(root, "SolventModel");
      if (value.length() > 0) {
        dobject->setSolventModel(value);
      }

      // Get add Links flag
      value = getTagValue(root, "AddLinks");
      int ival = atoi(value.c_str());
      if (ival != 0) {
        dobject->setAddLinks(true);
      } else {
        dobject->setAddLinks(false);
      }

      // Get changed topology flag
      value = getTagValue(root, "ChangedTopology");
      if (value.length()>0) {
        dobject->setChangedTopology((bool)atoi(value.c_str()));
      }

      // Get Commands directly, do not retrieve CoordCommands first
      XMLCh *commandName = XMLString::transcode("Command");
      DOMNodeList *commands = root->getElementsByTagName(commandName);
      delete [] commandName;

      // Restore commands
      int numCommands = commands->getLength();
      dobject->clearCmdList();
      for (int idx=0; idx<numCommands; idx++) {
        DOMNode *node = commands->item(idx);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
          DOMNode *child = node->getFirstChild();
          if (child->getNodeType() == DOMNode::TEXT_NODE) {
            value = getAsString(child->getNodeValue());
            dobject->appendCmdList(value);
          }
        }
      }

      // Get PmfSelections directly, do not retrieve PmfSelections first
      XMLCh * pmfSelectionName = XMLString::transcode("PmfSelection");
      DOMNodeList* pmfSelections = root->getElementsByTagName(pmfSelectionName);
      delete [] pmfSelectionName;

      // Restore pmf selections
      int numPmfSelections = pmfSelections->getLength();
      dobject->clearPmfSelection();
      for (int idx = 0; idx < numPmfSelections; idx++) {
        DOMNode * node = pmfSelections->item(idx);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
          DOMNode * child = node->getFirstChild();
          if (child->getNodeType() == DOMNode::TEXT_NODE) {
            value = getAsString(child->getNodeValue());
            StringTokenizer t(value, " ");
            t.next(); // throw away "select"
            int selInt;
            StringConverter::toInt(t.next(), selInt);
            string selString = t.next();
            dobject->addPmfSelection(selString, selInt);
          }
        }
      }
      
      // Get PmfDirectives directly, do not retrieve PmfDirectives first
      XMLCh *pmfDirectiveName = XMLString::transcode("PmfDirective");
      DOMNodeList* pmfDirectives = root->getElementsByTagName(pmfDirectiveName);
      delete [] pmfDirectiveName;
      
      // Restore pmf directives
      int numPmfDirectives = pmfDirectives->getLength();
      dobject->clearPmfCmdList();
      for (int idx = 0; idx < numPmfDirectives; idx++) {
        DOMNode * node = pmfDirectives->item(idx);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
          DOMNode * child = node->getFirstChild();
          if (child->getNodeType() == DOMNode::TEXT_NODE) {
            value = getAsString(child->getNodeValue());
            dobject->appendPmfCmdList(value);
          }
        }
      }

      // Get Links directly, do not retrieve AtomLinks first
      XMLCh *linkName = XMLString::transcode("Link");
      DOMNodeList *links = root->getElementsByTagName(linkName);
      delete [] linkName;

      // Restore links
      int numLinks = links->getLength();
      dobject->clearLinkList();
      for (int idx=0; idx<numLinks; idx++) {
        DOMNode *node = links->item(idx);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
          DOMNode *child = node->getFirstChild();
          if (child->getNodeType() == DOMNode::TEXT_NODE) {
            value = getAsString(child->getNodeValue());
            dobject->appendLinkList(value);
          }
        }
      }

      // Get Ions directly, do not retrieve CounterIons first
      XMLCh *ionName = XMLString::transcode("Ion");
      DOMNodeList *ions = root->getElementsByTagName(ionName);
      delete [] ionName;

      // Restore ions
      value = getTagValue(root, "SetCounterIons");
      if (value.length() > 0) {
        dobject->setSetIons((bool)atoi(value.c_str()));
      }
      int numIons = ions->getLength();
      dobject->clearCounterIonList();
      for (int idx=0; idx<numIons; idx++) {
        DOMNode *node = ions->item(idx);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
          DOMNode *child = node->getFirstChild();
          if (child->getNodeType() == DOMNode::TEXT_NODE) {
            value = getAsString(child->getNodeValue());
            dobject->appendCounterIonList(value);
          }
        }
      }
      value = getTagValue(root, "CounterIonScale");
      if (value.length()>0) {
        dobject->setCounterIonScale(atof(value.c_str()));
      }
      value = getTagValue(root, "CounterIonOption");
      if (value.length()>0) {
        dobject->setCounterIonOpt(value.c_str());
      }
      value = getTagValue(root, "CounterIonNumber");
      if (value.length() > 0) {
        dobject->setCINumber(atoi(value.c_str()));
      }
      value = getTagValue(root, "CounterIonGridSize");
      if (value.length()>0) {
        dobject->setCIGridSize(atoi(value.c_str()));
      }
      value = getTagValue(root, "MinimumAtomIonDistance");
      if (value.length()>0) {
        dobject->setMinAtomCIDis(atof(value.c_str()));
      }
      // Restore system fractions
      value = getTagValue(root, "UseFractions");
      if (value.length()>0) {
        dobject->setUseFractions((bool)atoi(value.c_str()));
      }
      value = getTagValue(root, "Fraction1");
      if (value.length()>0) {
        dobject->setFraction1(atoi(value.c_str()));
      }
      value = getTagValue(root, "Fraction2");
      if (value.length()>0) {
        dobject->setFraction2(atoi(value.c_str()));
      }
      value = getTagValue(root, "Fraction3");
      if (value.length()>0) {
        dobject->setFraction3(atoi(value.c_str()));
      }
      value = getTagValue(root, "Fraction4");
      if (value.length()>0) {
        dobject->setFraction4(atoi(value.c_str()));
      }
      value = getTagValue(root, "Fraction5");
      if (value.length()>0) {
        dobject->setFraction5(atoi(value.c_str()));
      }
      value = getTagValue(root, "Fraction6");
      if (value.length()>0) {
        dobject->setFraction6(atoi(value.c_str()));
      }
      value = getTagValue(root, "Fraction7");
      if (value.length()>0) {
        dobject->setFraction7(atoi(value.c_str()));
      }
      value = getTagValue(root, "Fraction8");
      if (value.length()>0) {
        dobject->setFraction8(atoi(value.c_str()));
      }
      value = getTagValue(root, "Fraction9");
      if (value.length()>0) {
        dobject->setFraction9(atoi(value.c_str()));
      }
      value = getTagValue(root, "Fraction10");
      if (value.length()>0) {
        dobject->setFraction10(atoi(value.c_str()));
      }

      // Restore coordinate panel settings
      // Translate panel
      value = getTagValue(root, "TranslateOption");
      if (value.length()>0) {
        dobject->setTranslateOption(atoi(value.c_str()));
      }
      value = getTagValue(root, "TranslateX");
      if (value.length()>0) {
        dobject->setTranslateX(atof(value.c_str()));
      }
      value = getTagValue(root, "TranslateY");
      if (value.length()>0) {
        dobject->setTranslateY(atof(value.c_str()));
      }
      value = getTagValue(root, "TranslateZ");
      if (value.length()>0) {
        dobject->setTranslateZ(atof(value.c_str()));
      }

      // Rotate panel
      value = getTagValue(root, "OrientOption");
      if (value.length()>0) {
        dobject->setOrientOption(atoi(value.c_str()));
      }
      value = getTagValue(root, "RotationAngle");
      if (value.length()>0) {
        dobject->setAngle(atof(value.c_str()));
      }
      value = getTagValue(root, "OrientX");
      if (value.length()>0) {
        dobject->setOrientX(atof(value.c_str()));
      }
      value = getTagValue(root, "OrientY");
      if (value.length()>0) {
        dobject->setOrientY(atof(value.c_str()));
      }
      value = getTagValue(root, "OrientZ");
      if (value.length()>0) {
        dobject->setOrientZ(atof(value.c_str()));
      }
      // Solvate panel
      value = getTagValue(root, "BoundaryCondition");
      if (value.length()>0) {
        dobject->setBoundaryCondition(atoi(value.c_str()));
      }
      value = getTagValue(root, "GeometryOption");
      if (value.length()>0) {
        dobject->setGeometryOption(atoi(value.c_str()));
      }
      value = getTagValue(root, "MinSluSluSeparation");
      if (value.length()>0) {
        dobject->setMinSluSluDist(atof(value.c_str()));
      }
      value = getTagValue(root, "MinSluSlvSeparation");
      if (value.length()>0) {
        dobject->setMinSluSlvDist(atof(value.c_str()));
      }
      value = getTagValue(root, "ExpandVolume");
      if (value.length()>0) {
        dobject->setExpandVolPar(atof(value.c_str()));
      }
      value = getTagValue(root, "EnvelopeVolume");
      if (value.length()>0) {
        dobject->setEnvelopePar(atof(value.c_str()));
      }
      // Remove panel
      value = getTagValue(root, "RemoveOption");
      if (value.length()>0) {
        dobject->setRemoveOption(atoi(value.c_str()));
      }
      value = getTagValue(root, "RemoveXRange");
      if (value.length()>0) {
        dobject->setRemoveXRange((bool)atoi(value.c_str()));
      }
      value = getTagValue(root, "RemoveYRange");
      if (value.length()>0) {
        dobject->setRemoveYRange((bool)atoi(value.c_str()));
      }
      value = getTagValue(root, "RemoveZRange");
      if (value.length()>0) {
        dobject->setRemoveZRange((bool)atoi(value.c_str()));
      }
      // Modify cell panel
      value = getTagValue(root, "RepeatOption");
      if (value.length()>0) {
        dobject->setRepeatOption(atoi(value.c_str()));
      }
      value = getTagValue(root, "XRepeat");
      if (value.length()>0) {
        dobject->setXRepeat(atoi(value.c_str()));
      }
      value = getTagValue(root, "YRepeat");
      if (value.length()>0) {
        dobject->setYRepeat(atoi(value.c_str()));
      }
      value = getTagValue(root, "ZRepeat");
      if (value.length()>0) {
        dobject->setZRepeat(atoi(value.c_str()));
      }
      value = getTagValue(root, "MinimumRepeatSeparation");
      if (value.length()>0) {
        dobject->setMinRepeatSeparation(atof(value.c_str()));
      }
      value = getTagValue(root, "RandomX");
      if (value.length()>0) {
        dobject->setRandomX((bool)atoi(value.c_str()));
      }
      value = getTagValue(root, "RandomY");
      if (value.length()>0) {
        dobject->setRandomY((bool)atoi(value.c_str()));
      }
      value = getTagValue(root, "RandomZ");
      if (value.length()>0) {
        dobject->setRandomZ((bool)atoi(value.c_str()));
      }
      value = getTagValue(root, "InvertZCopy");
      if (value.length()>0) {
        dobject->setInvertZCopy((bool)atoi(value.c_str()));
      }
      value = getTagValue(root, "UseZBoxDimension");
      if (value.length()>0) {
        dobject->setUseZBoxDim((bool)atoi(value.c_str()));
      }
      value = getTagValue(root, "SkipX");
      if (value.length()>0) {
        dobject->setSkipX(atoi(value.c_str()));
      }
      value = getTagValue(root, "SkipY");
      if (value.length()>0) {
        dobject->setSkipY(atoi(value.c_str()));
      }
      value = getTagValue(root, "SkipZ");
      if (value.length()>0) {
        dobject->setSkipZ(atoi(value.c_str()));
      }

      // Restore details settings
      value = getTagValue(root, "UseMemory");
      if (value.length()>0) {
        dobject->setUseMemory((bool)atoi(value.c_str()));
      }
      value = getTagValue(root, "HeapMemory");
      if (value.length()>0) {
        dobject->setHeapMemory(atoi(value.c_str()));
      }
      value = getTagValue(root, "StackMemory");
      if (value.length()>0) {
        dobject->setStackMemory(atoi(value.c_str()));
      }
      value = getTagValue(root, "GlobalMemory");
      if (value.length()>0) {
        dobject->setGlobalMemory(atoi(value.c_str()));
      }
      value = getTagValue(root, "PrintLevel");
      if (value.length()>0) {
        dobject->setPrintLevel(atoi(value.c_str()));
      }

      doc->release();

   } else {
      cerr << "Dynamic cast to PrepareModel failed" << endl;
   }
}

void PrepareModelXMLizer::serialize(const Serializable& object,
                        string& data, EcceMap& properties)

{
   data = "";

   const PrepareModel *dobject = dynamic_cast<const PrepareModel*>(&object);
   MdTask *mdTask = 0;

   if (dobject) {
      ostrstream os;
      os << xmlHeader() << endl;

      os << "<" << ROOTELEMENT << ">" << endl;
      os << "<IsFirstTask>" <<dobject->getIsFirstTask()<< "</IsFirstTask>"<< endl;
      // Add PDB settings
      if (dobject->getPDBSource().length() > 0) {
        os << "<PDBSource>" <<dobject->getPDBSource()<< "</PDBSource>"<< endl;
      }

      // System Name
      try {
        mdTask = dynamic_cast<MdTask*>(EDSIFactory::getResource(dobject->getUrl()));
        if (mdTask != 0) {
          string systemName = mdTask->getSystemName();
          if (systemName.length() > 0) {
            os << "<SystemName>" << systemName << "</SystemName>"<< endl;
          }
          mdTask = 0;
        }
      }
      catch (...) {
        mdTask = 0;
      }

      // Calc Name
      try {
        mdTask = dynamic_cast<MdTask*>(EDSIFactory::getResource(dobject->getUrl()));
        if (mdTask != 0) {
          string calcName = mdTask->getCalcName();
          if (calcName.length() > 0) {
            os << "<CalcId>" << calcName << "</CalcId>"<< endl;
          }
          mdTask = 0;
        }
      }
      catch (...) {
        mdTask = 0;
      }

      // Output Fragment Name
      try {
        mdTask = dynamic_cast<MdTask*>(EDSIFactory::getResource(dobject->getUrl()));
        if (mdTask != 0) {
          string outputFragmentName = mdTask->getOutputFragmentName();
          if (outputFragmentName.length() > 0) {
            os << "<OutputFragmentName>" << outputFragmentName << "</OutputFragmentName>"<< endl;
          }
          mdTask = 0;
        }
      }
      catch (...) {
        mdTask = 0;
      }

      // Add changed Topology flag
      if (dobject->getChangedTopology()) {
        os << "<ChangedTopology>1</ChangedTopology>" << endl;
      } else {
        os << "<ChangedTopology>0</ChangedTopology>" << endl;
      }
      // Add default Histidine setting
      os << "<Histidine>";
      if (dobject->getDefaultHistidine() == PrepareModel::HID) {
        os << "HID";
      } else if (dobject->getDefaultHistidine() == PrepareModel::HIE) {
        os << "HIE";
      } else if (dobject->getDefaultHistidine() == PrepareModel::HIP) {
        os << "HIP";
      }
      os << "</Histidine>" << endl;
      // Add solvent settings
      os << "<SetSolvent>" << dobject->getSetSolvent()
         << "</SetSolvent>" <<endl;
      os << "<SolventOption>"<< dobject->getSolventOption()
         << "</SolventOption>" << endl;
      os << "<SolventName>"<< dobject->getSolventName()
         << "</SolventName>" << endl;
      os << "<SolventModel>"<< dobject->getSolventModel()
         << "</SolventModel>" << endl;

      // Add counter ion settings
      os << "<SetCounterIons>" << dobject->getSetIons()
         << "</SetCounterIons>" <<endl;
      vector<string> ions = dobject->getCounterIonList();
      int numIons = ions.size();
      if (numIons > 0) {
        os << "<CounterIons>" << endl;
          for (int idx=0; idx<numIons; idx++ ) {
            os << "  <Ion>";
            os << ions[idx];
            os << "</Ion>" << endl;
          }
        os << "</CounterIons>" << endl;
      }

      if (dobject->getCounterIonOpt() != dobject->IONOPTION) {
        os << "<CounterIonOption>" << dobject->getCounterIonOpt()
           << "</CounterIonOption>" << endl;
      }
      if (dobject->getCINumber() != dobject->CINUM) {
        os << "<CounterIonNumber>" << dobject->getCINumber()
           << "</CounterIonNumber>" << endl;
      }
      if (dobject->getCounterIonScale() != dobject->CISCALE) {
        os << "<CounterIonScale>"
           << writeDoubleAsString(dobject->getCounterIonScale())
           << "</CounterIonScale>"<< endl;
      }
      if (dobject->getCIGridSize() != dobject->CIGRID ||
          dobject->getMinAtomCIDis() != dobject->CIMINDIS) {
        os << "<CounterIonGridSize>" <<dobject->getCIGridSize()
           << "</CounterIonGridSize>"<< endl;
        os << "<MinimumAtomIonDistance>"
           << writeDoubleAsString(dobject->getMinAtomCIDis())
           << "</MinimumAtomIonDistance>"<< endl;
      }

      // Fractions settings
      os << "<UseFractions>" <<dobject->getUseFractions()
         << "</UseFractions>"<<endl;
      if (dobject->getFraction1() != dobject->FRAC1) {
        os << "<Fraction1>" <<dobject->getFraction1()
           << "</Fraction1>"<<endl;
      }
      if (dobject->getFraction2() != dobject->FRAC2) {
        os << "<Fraction2>" <<dobject->getFraction2()
           << "</Fraction2>"<<endl;
      }
      if (dobject->getFraction3() != dobject->FRAC3) {
        os << "<Fraction3>" <<dobject->getFraction3()
           << "</Fraction3>"<<endl;
      }
      if (dobject->getFraction4() != dobject->FRAC4) {
        os << "<Fraction4>" <<dobject->getFraction4()
           << "</Fraction4>"<<endl;
      }
      if (dobject->getFraction5() != dobject->FRAC5) {
        os << "<Fraction5>" <<dobject->getFraction5()
           << "</Fraction5>"<<endl;
      }
      if (dobject->getFraction6() != dobject->FRAC6) {
        os << "<Fraction6>" <<dobject->getFraction6()
           << "</Fraction6>"<<endl;
      }
      if (dobject->getFraction7() != dobject->FRAC7) {
        os << "<Fraction7>" <<dobject->getFraction7()
           << "</Fraction7>"<<endl;
      }
      if (dobject->getFraction8() != dobject->FRAC8) {
        os << "<Fraction8>" <<dobject->getFraction8()
           << "</Fraction8>"<<endl;
      }
      if (dobject->getFraction9() != dobject->FRAC9) {
        os << "<Fraction9>" <<dobject->getFraction9()
           << "</Fraction9>"<<endl;
      }
      if (dobject->getFraction10() != dobject->FRAC10) {
        os << "<Fraction10>" <<dobject->getFraction10()
           << "</Fraction10>"<<endl;
      }

      // Add add links flag
      if (dobject->getAddLinks()) {
        os << "<AddLinks>1</AddLinks>" << endl;
      } else {
        os << "<AddLinks>0</AddLinks>" << endl;
      }

      // Add Links
      vector<string> links = dobject->getLinkList();
      int numLinks = links.size();
      if (numLinks > 0) {
        os << "<AtomLinks>" << endl;
          for (int idx=0; idx<numLinks; idx++ ) {
            os << "  <Link>";
            os << links[idx];
            os << "</Link>" << endl;
          }
        os << "</AtomLinks>" << endl;
      }

      // Add coordinate panel settings
      // Translate panel
      if (dobject->getTranslateOption() != dobject->TRNSOPTION) {
        os << "<TranslateOption>" << dobject->getTranslateOption()
           << "</TranslateOption>" << endl;
      }
      if (dobject->getTranslateX() != dobject->TRNSX) {
        os << "<TranslateX>" << dobject->getTranslateX()
           << "</TranslateX>" << endl;
      }
      if (dobject->getTranslateY() != dobject->TRNSY) {
        os << "<TranslateY>" << dobject->getTranslateY()
           << "</TranslateY>" << endl;
      }
      if (dobject->getTranslateZ() != dobject->TRNSZ) {
        os << "<TranslateZ>" << dobject->getTranslateZ()
           << "</TranslateZ>" << endl;
      }
      // Rotate panel
      if (dobject->getOrientOption() != dobject->ORNTOPTION) {
        os << "<OrientOption>" << dobject->getOrientOption()
           << "</OrientOption>" << endl;
      }
      if (dobject->getAngle() != dobject->ANGLE) {
        os << "<RotationAngle>" << dobject->getAngle()
           << "</RotationAngle>" << endl;
      }
      if (dobject->getOrientX() != dobject->ORIENTX) {
        os << "<OrientX>" << dobject->getOrientX()
           << "</OrientX>" << endl;
      }
      if (dobject->getOrientY() != dobject->ORIENTY) {
        os << "<OrientY>" << dobject->getOrientY()
           << "</OrientY>" << endl;
      }
      if (dobject->getOrientZ() != dobject->ORIENTZ) {
        os << "<OrientZ>" << dobject->getOrientZ()
           << "</OrientZ>" << endl;
      }
      // Solvate panel
      if (dobject->getBoundaryCondition() != dobject->BCOPT) {
        os << "<BoundaryCondition>" << dobject->getBoundaryCondition()
           << "</BoundaryCondition>" << endl;
      }
      if (dobject->getGeometryOption() != dobject->GEOMOPT) {
        os << "<GeometryOption>" << dobject->getGeometryOption()
           << "</GeometryOption>" << endl;
      }
      if (dobject->getMinSluSluDist() != dobject->MINSEPARATION) {
        os << "<MinSluSluSeparation>" << dobject->getMinSluSluDist()
           << "</MinSluSluSeparation>" << endl;
      }
      if (dobject->getMinSluSlvDist() != dobject->TOUCH) {
        os << "<MinSluSlvSeparation>" << dobject->getMinSluSlvDist()
           << "</MinSluSlvSeparation>" << endl;
      }
      if (dobject->getExpandVolPar() != dobject->EXPANDVOL) {
        os << "<ExpandVolume>" << dobject->getExpandVolPar()
           << "</ExpandVolume>" << endl;
      }
      if (dobject->getEnvelopePar() != dobject->SOLVENTTOL) {
        os << "<EnvelopeVolume>" << dobject->getEnvelopePar()
           << "</EnvelopeVolume>" << endl;
      }
      // Remove panel
      if (dobject->getRemoveOption() != dobject->RMVOPT) {
        os << "<RemoveOption>" << dobject->getRemoveOption()
           << "</RemoveOption>" << endl;
      }
      if (dobject->getRemoveXRange() != dobject->XRANGE) {
        os << "<RemoveXRange>" << dobject->getRemoveXRange()
           << "</RemoveXRange>" << endl;
      }
      if (dobject->getRemoveYRange() != dobject->YRANGE) {
        os << "<RemoveYRange>" << dobject->getRemoveYRange()
           << "</RemoveYRange>" << endl;
      }
      if (dobject->getRemoveZRange() != dobject->ZRANGE) {
        os << "<RemoveZRange>" << dobject->getRemoveZRange()
           << "</RemoveZRange>" << endl;
      }
      // Modify cell panel
      if (dobject->getRepeatOption() != dobject->REPOPTS) {
        os << "<RepeatOption>" << dobject->getRepeatOption()
           << "</RepeatOption>" << endl;
      }
      if (dobject->getXRepeat() != dobject->INDMODCELL) {
        os << "<XRepeat>" << dobject->getXRepeat()
           << "</XRepeat>" << endl;
      }
      if (dobject->getYRepeat() != dobject->INDMODCELL) {
        os << "<YRepeat>" << dobject->getYRepeat()
           << "</YRepeat>" << endl;
      }
      if (dobject->getZRepeat() != dobject->INDMODCELL) {
        os << "<ZRepeat>" << dobject->getZRepeat()
           << "</ZRepeat>" << endl;
      }
      if (dobject->getMinRepeatSeparation() != dobject->MCLLMINSEP) {
        os << "<MinimumRepeatSeparation>" << dobject->getMinRepeatSeparation()
           << "</MinimumRepeatSeparation>" << endl;
      }
      if (dobject->getRandomX() != dobject->RANDXYZ) {
        os << "<RandomX>" << dobject->getRandomX()
           << "</RandomX>" << endl;
      }
      if (dobject->getRandomY() != dobject->RANDXYZ) {
        os << "<RandomY>" << dobject->getRandomY()
           << "</RandomY>" << endl;
      }
      if (dobject->getRandomZ() != dobject->RANDXYZ) {
        os << "<RandomZ>" << dobject->getRandomZ()
           << "</RandomZ>" << endl;
      }
      if (dobject->getInvertZCopy() != dobject->INVCOPY) {
        os << "<InvertZCopy>" << dobject->getInvertZCopy()
           << "</InvertZCopy>" << endl;
      }
      if (dobject->getUseZBoxDim() != dobject->USEZBOX) {
        os << "<UseZBoxDimension>" << dobject->getUseZBoxDim()
           << "</UseZBoxDimension>" << endl;
      }
      if (dobject->getSkipX() != dobject->INDMODCELL) {
        os << "<SkipX>" << dobject->getSkipX()
           << "</SkipX>" << endl;
      }
      if (dobject->getSkipY() != dobject->INDMODCELL) {
        os << "<SkipY>" << dobject->getSkipY()
           << "</SkipY>" << endl;
      }
      if (dobject->getSkipZ() != dobject->INDMODCELL) {
        os << "<SkipZ>" << dobject->getSkipZ()
           << "</SkipZ>" << endl;
      }

      // Add Commands
      vector<string> commands = dobject->getCmdList();
      int numCommands = commands.size();
      if (numCommands > 0) {
        os << "<CoordCommands>" << endl;
          for (int idx=0; idx<numCommands; idx++ ) {
            os << "  <Command>";
            os << commands[idx];
            os << "</Command>" << endl;
          }
        os << "</CoordCommands>" << endl;
      }

      // Add PMF selections
      map<string,int> selections = dobject->getPmfSelection();
      if (selections.size() > 0) {
        os << "<PmfSelections>" << endl;
        map<string,int>::iterator selIter = selections.begin();
        for (; selIter != selections.end(); selIter++) {
          os << "  <PmfSelection>";
          os << "select " << selIter->second << " " << selIter->first;
          os << "</PmfSelection>" << endl;
        }
        os << "</PmfSelections>" << endl;
      }

      // Add PMF directives
      vector<string> pmfList = dobject->getPmfList();
      int numPmf = pmfList.size();
      if (numPmf > 0) {
        os << "<PmfDirectives>" << endl;
        for (int idx = 0; idx < numPmf; idx++) {
          os << "  <PmfDirective>";
          os << pmfList[idx];
          os << "</PmfDirective>" << endl;
        }
        os << "</PmfDirectives>" << endl;
      }

      // Details settings
      os << "<UseMemory>" <<dobject->getUseMemory()
         << "</UseMemory>"<<endl;
      os << "<HeapMemory>" <<dobject->getHeapMemory()
         << "</HeapMemory>"<<endl;
      os << "<StackMemory>" <<dobject->getStackMemory()
         << "</StackMemory>"<<endl;
      os << "<GlobalMemory>" <<dobject->getGlobalMemory()
         << "</GlobalMemory>"<<endl;
      if (dobject->getPrintLevel() != dobject->PRINTLEVEL) {
        os << "<PrintLevel>" <<dobject->getPrintLevel()
           << "</PrintLevel>"<<endl;
      }

      vector<string> files = FFConfigData::getFFFiles();
      os << "<ForceFieldFiles>" << endl;
        for (int idx=0; idx<files.size(); idx++ ) {
          os << "  <File>";
          os << files[idx];
          os << "</File>" << endl;
        }
      os << "</ForceFieldFiles>" << endl;

      vector<string> dirs = FFConfigData::getSegDirs();
      os << "<ForceFieldDirectories>" << endl;
        for (int idx=0; idx<dirs.size(); idx++ ) {
          os << "  <Directory>";
          os << dirs[idx];
          os << "</Directory>" << endl;
        }
      os << "</ForceFieldDirectories>" << endl;

      os << "</" << ROOTELEMENT << ">" << endl;
      os << ends;

      data = os.str();
   } else {
      cerr << "Dynamic cast to PrepareModel failed" << endl;
   }
}

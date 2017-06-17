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
#include "tdat/PolyrateModel.H"
#include "dsm/BasicDOMParser.H"
#include "dsm/PolyrateModelXMLizer.H"
#include "dsm/Resource.H"
#include "dsm/MdTask.H"



const string PolyrateModelXMLizer::ROOTELEMENT = "PolyrateModel";



PolyrateModelXMLizer::PolyrateModelXMLizer() : XMLSerializer()
{}


PolyrateModelXMLizer::~PolyrateModelXMLizer()
{}


string PolyrateModelXMLizer::mimetype() const
{
   return "text/xml/ecce-polyratemodel";
}


void PolyrateModelXMLizer::deserialize(const string& data,
                       Serializable& object, EcceMap& properties)
                       throw (ParseException)
{
   // NOTE: For now at least, we aren't interested in any properties.
   PolyrateModel *dobject = dynamic_cast<PolyrateModel*>(&object);

   if (dobject) {

      // Parse the XML document
      BasicDOMParser parser;
      DOMDocument *doc = 0;

      // This can throw a ParseException
      istrstream is(data.c_str());
      doc = parser.parse(is);

      string value;

      DOMElement *root = doc->getDocumentElement();

      // Perform deserialization, get values
      value = getTagValue(root, "ForwardReverseOption");
      if (value.length() > 0) {
        dobject->setFrwrdRevOpt((bool)atoi(value.c_str()));
      }
      value = getTagValue(root, "ReverseReactionScale");
      if (value.length() > 0) {
        dobject->setReverseRxScaleExp(atoi(value.c_str()));
      }
      value = getTagValue(root, "ForwardSymmetryFactor");
      if (value.length() > 0) {
        dobject->setForwardSymmetryFactor(atoi(value.c_str()));
      }
      value = getTagValue(root, "ReverseSymmetryFactor");
      if (value.length() > 0) {
        dobject->setReverseSymmetryFactor(atoi(value.c_str()));
      }
      value = getTagValue(root, "DetailedAnalysis");
      if (value.length() > 0) {
        dobject->setAnalysisFlag((bool)atoi(value.c_str()));
      }
      value = getTagValue(root, "IndicateSaddle");
      if (value.length() > 0) {
        dobject->setSaddleFlag((bool)atoi(value.c_str()));
      }
      value = getTagValue(root, "FrequencyScale");
      if (value.length() > 0) {
        dobject->setFrequencyScale(atof(value.c_str()));
      }
      int i;
      string component;
      string compstr;
      for (i = 0; i<5; i++) {
        if (i == 0) {
          component = "react1";
          compstr = "Reactant1";
        } else if (i == 1) {
          component = "react2";
          compstr = "Reactant2";
        } else if (i == 2) {
          component = "transition";
          compstr = "TransitionState";
        } else if (i == 3) {
          component = "prod1";
          compstr = "Product1";
        } else if (i == 4) {
          component = "prod2";
          compstr = "Product2";
        }
        value = getChildTagValue(root,compstr.c_str(),"Degeneracy1");
        if (value.length() > 0) {
          dobject->setDegeneracy1(component,atoi(value.c_str()));
        }
        value = getChildTagValue(root,compstr.c_str(),"ElectronicEnergy1");
        if (value.length() > 0) {
          dobject->setElectronicEnergy1(component,atof(value.c_str()));
        }
        value = getChildTagValue(root,compstr.c_str(),"Degeneracy2");
        if (value.length() > 0) {
          dobject->setDegeneracy2(component,atoi(value.c_str()));
        }
        value = getChildTagValue(root,compstr.c_str(),"ElectronicEnergy2");
        if (value.length() > 0) {
          dobject->setElectronicEnergy2(component,atof(value.c_str()));
        }
        value = getChildTagValue(root,compstr.c_str(),"Degeneracy3");
        if (value.length() > 0) {
          dobject->setDegeneracy3(component,atoi(value.c_str()));
        }
        value = getChildTagValue(root,compstr.c_str(),"ElectronicEnergy3");
        if (value.length() > 0) {
          dobject->setElectronicEnergy3(component,atof(value.c_str()));
        }
        value = getChildTagValue(root,compstr.c_str(), "LinearStatus");
        if (value.length() > 0) {
          dobject->setLinearStatus(component,(bool)atoi(value.c_str()));
          if ((bool)atoi(value.c_str())) {
            value = getChildTagValue(root,compstr.c_str(), "ComponentAxis");
            if (value.length() > 0) {
              dobject->setComponentAxis(component, *(value.c_str()));
            }
          }
        }
        DOMElement *cmpRoot = getNextElementFromTag(root, compstr.c_str()); 
        DOMElement *listRoot = getNextElementFromTag(cmpRoot, "TorsionList");
        vector<DOMElement*> torsions = getNextElementsFromTag(listRoot, "Torsion");
        for (int i=0; i<(int)torsions.size(); i++) {
          value = getTagValue(torsions[i], "Command");
          int mode, dmin, sym, ngrp;
          string scheme, level;
          if (value.length() > 0) {
            dobject->translateTorsion(value,mode,dmin,sym,scheme,level,ngrp);
            dobject->addMode(component,mode);
            dobject->setTorsion(component,mode,value);
            dobject->setAnharmonicity(component,mode,"tor");
          }
          XMLCh *optionName = XMLString::transcode("Option");
          DOMNodeList *opts = torsions[i]->getElementsByTagName(optionName);
          delete [] optionName;

          // Restore mode options
          int numOpts = opts->getLength();
          vector<string> optList;
//          if (numOpts > 0) {
//            dobject->clearOpts();
//          }
          for (int idx=0; idx<numOpts; idx++) {
            DOMNode *node = opts->item(idx);
            if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
              DOMNode *child = node->getFirstChild();
              if (child->getNodeType() == DOMNode::TEXT_NODE) {
                dobject->addTorsionOpt(component,mode,getAsString(child->getNodeValue()).c_str());
              }
            }
          }
        }
        DOMElement *stretchRoot = getNextElementFromTag(cmpRoot, "StretchList");
        // Get Commands directly, do not retrieve CoordCommands first
        XMLCh *stretchName = XMLString::transcode("Stretch");
        if (stretchRoot) {
          DOMNodeList *stretches = stretchRoot->getElementsByTagName(stretchName);

          // Restore stretch modes
          int numStretch = stretches->getLength();
          for (int idx=0; idx<numStretch; idx++) {
            DOMNode *node = stretches->item(idx);
            if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
              DOMNode *child = node->getFirstChild();
              if (child->getNodeType() == DOMNode::TEXT_NODE) {
                int mode = atoi(getAsString(child->getNodeValue()).c_str());
                dobject->addMode(component,mode);
                dobject->setAnharmonicity(component,mode,"morse");
              }
            }
          }
        }
        delete [] stretchName;
      }

      // Get Commands directly, do not retrieve CoordCommands first
      XMLCh *temperatureName = XMLString::transcode("Temperature");
      DOMNodeList *temps = root->getElementsByTagName(temperatureName);
      delete [] temperatureName;

      // Restore temperature list
      int numTemp = temps->getLength();
      vector<double> tempList;
      if (numTemp > 0) {
        dobject->clearTemperature();
      }
      for (int idx=0; idx<numTemp; idx++) {
        DOMNode *node = temps->item(idx);
        if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
          DOMNode *child = node->getFirstChild();
          if (child->getNodeType() == DOMNode::TEXT_NODE) {
            dobject->addTemperature(strtod(getAsString(child->getNodeValue()).c_str(), NULL));
          }
        }
      }

      doc->release();

   } else {
      cerr << "Dynamic cast to PolyrateModel failed" << endl;
   }
}

void PolyrateModelXMLizer::serialize(const Serializable& object,
                        string& data, EcceMap& properties)

{
   data = "";

   const PolyrateModel *dobject = dynamic_cast<const PolyrateModel*>(&object);

   if (dobject) {
      ostrstream os;
      os << xmlHeader() << endl;
      os << "<" << ROOTELEMENT << ">" << endl;

      // Perform serialization, set element values
      os << "<ForwardReverseOption>"<<dobject->getFrwrdRevOpt()
         << "</ForwardReverseOption>" <<endl;
      if (dobject->getReverseRxScaleExp() != dobject->RVRXEXP) {
        os << "<ReverseReactionScale>"<<dobject->getReverseRxScaleExp()
           << "</ReverseReactionScale>" <<endl;
      }
      if (dobject->getForwardSymmetryFactor() != dobject->FRWSYM) {
        os << "<ForwardSymmetryFactor>"<<dobject->getForwardSymmetryFactor()
           << "</ForwardSymmetryFactor>" <<endl;
      }
      if (dobject->getReverseSymmetryFactor() != dobject->REVSYM) {
        os << "<ReverseSymmetryFactor>"<<dobject->getReverseSymmetryFactor()
           << "</ReverseSymmetryFactor>" <<endl;
      }
      os << "<DetailedAnalysis>"<<dobject->getAnalysisFlag()
         << "</DetailedAnalysis>" <<endl;
      if (dobject->getSaddleFlag() != dobject->SADDLE) {
        os << "<IndicateSaddle>"<<dobject->getSaddleFlag()
           << "</IndicateSaddle>" <<endl;
      }
      if (dobject->getFrequencyScale() != dobject->FREQSCAL) {
        os << "<FrequencyScale>"<<dobject->getFrequencyScale()
           << "</FrequencyScale>" <<endl;
      }

      int i;
      string component;
      string compstr;
      for (i = 0; i<5; i++) {
        if (i == 0) {
          component = "react1";
          compstr = "Reactant1";
        } else if (i == 1) {
          component = "react2";
          compstr = "Reactant2";
        } else if (i == 2) {
          component = "transition";
          compstr = "TransitionState";
        } else if (i == 3) {
          component = "prod1";
          compstr = "Product1";
        } else if (i == 4) {
          component = "prod2";
          compstr = "Product2";
        }
        if (dobject->getComponentStatus(component)) {
          os << "<"<<compstr<<">"<<endl;
          os << "  <Degeneracy1>"<<dobject->getDegeneracy1(component)
             << "</Degeneracy1>"<<endl;
          os << "  <ElectronicEnergy1>"<<dobject->getElectronicEnergy1(component)
             << "</ElectronicEnergy1>"<<endl;
          if (dobject->getDegeneracy2(component) != dobject->DEG2) {
            os << "  <Degeneracy2>"<<dobject->getDegeneracy2(component)
              << "</Degeneracy2>"<<endl;
            os << "  <ElectronicEnergy2>"<<dobject->getElectronicEnergy2(component)
              << "</ElectronicEnergy2>"<<endl;
          }
          if (dobject->getDegeneracy3(component) != dobject->DEG3) {
            os << "  <Degeneracy3>"<<dobject->getDegeneracy3(component)
              << "</Degeneracy3>"<<endl;
            os << "  <ElectronicEnergy3>"<<dobject->getElectronicEnergy3(component)
              << "</ElectronicEnergy3>"<<endl;
          }
          if (dobject->getLinearStatus(component) &&
              dobject->getComponentAxis(component) != dobject->DEFAX) {
            os << "  <LinearStatus>"<<dobject->getLinearStatus(component)
               << "</LinearStatus>"<<endl;
            os << "  <ComponentAxis>"<<dobject->getComponentAxis(component)
               << "</ComponentAxis>"<<endl;
          }
          int numModes = dobject->getNumModes(component);
          int j, k, mode;
          if (dobject->getNumTorsionModes(component) > 0) {
            os << "  <TorsionList>"<<endl;
            for (j=0; j<numModes; j++) {
              mode = dobject->getMode(component,j);
              if (dobject->getAnharmonicity(component,mode) == "tor") {
                string cmd = dobject->getTorsion(component,mode);
                if (cmd != "") {
                  os << "    <Torsion>"<<endl;
                  os << "      <Command>" <<cmd<< "</Command>"<<endl;
                  vector<string> opts = dobject->getTorsionOpts(component,mode);
                  if (opts.size() > 0) {
                    os << "      <TorsionOpts>"<<endl;
                    for (k=0; k<(int)opts.size(); k++) {
                      os << "        <Option>"<<opts[k]<<"</Option>"<<endl;
                    }
                    os << "      </TorsionOpts>"<<endl;
                  }
                  os << "    </Torsion>"<<endl;
                }
              }
            }
            os << "  </TorsionList>"<<endl;
          }
          if (dobject->getNumStretchModes(component) > 0) {
            os << "  <StretchList>"<<endl;
            for (j=0; j<numModes; j++) {
              mode = dobject->getMode(component,j);
              if (dobject->getAnharmonicity(component,mode) == "morse") {
                os << "    <Stretch>" <<mode<< "</Stretch>"<<endl;
              }
            }
            os << "  </StretchList>"<<endl;
          }
          os << "</"<<compstr<<">"<<endl;
        }
      }
      int numTemp = dobject->getTemperatureListSize();
      if (numTemp > 0) {
        os << "<TemperatureList>" << endl;
        char buf[32];
        for (int idx = 0; idx < numTemp; idx++) {
          sprintf(buf, "%18.6f", dobject->getTemperature(idx));
          os << "  <Temperature>"<<buf<<"</Temperature>"<<endl;
        }
        os << "</TemperatureList>" << endl;
      }

      os << "</" << ROOTELEMENT << ">" << endl;
      os << ends;

      data = os.str();

   } else {
      cerr << "Dynamic cast to PolyrateModel failed" << endl;
   }
}

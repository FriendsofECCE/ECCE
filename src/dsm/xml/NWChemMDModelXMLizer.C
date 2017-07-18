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
#include "util/StringTokenizer.H"

#include "tdat/TAtm.H"
#include "tdat/Fragment.H"

#include "dsm/NWChemMDModel.H"
#include "dsm/BasicDOMParser.H"
#include "dsm/NWChemMDModelXMLizer.H"
#include "dsm/Resource.H"
#include "dsm/MdTask.H"

const string NWChemMDModelXMLizer::ROOTELEMENT = "NWChemMDModel";

NWChemMDModelXMLizer::NWChemMDModelXMLizer() : XMLSerializer()
{
}

NWChemMDModelXMLizer::~NWChemMDModelXMLizer()
{
}

string NWChemMDModelXMLizer::mimetype() const
{
   return "text/xml/ecce-preparemodel";
}

/**
 * Its the callers responsibility to make sure the NWChemMDModel
 * has its fragment assigned.  Its also the callers responsibility to
 * clear any existing constrains from "object".
 */
void NWChemMDModelXMLizer::deserialize(const string& data,
                       Serializable& object, EcceMap& properties)
                       throw (ParseException)
{
   // NOTE: For now at least, we aren't interested in any properties.
   NWChemMDModel *dobject = dynamic_cast<NWChemMDModel*>(&object);

   if (dobject) {

      // Parse the XML document
      BasicDOMParser parser;
      DOMDocument *doc = 0;

      // This can throw a ParseException
      istrstream is(data.c_str());
      doc = parser.parse(is);

      string elname, value;
      DOMElement *root = doc->getDocumentElement();

      // Get Interaction model and associated parameters, if it exists
      InteractionModel *imodel = dobject->getInteractionModel();
      if (imodel) {
        XMLCh *tagName = XMLString::transcode("Interaction");
        DOMNodeList *nodeList = root->getElementsByTagName(tagName);
        delete [] tagName;
        DOMElement *iroot = (DOMElement*)nodeList->item(0);
        if (iroot) {
          value = getTagValue(iroot, "Touched");
          if (value.length() > 0) {
            imodel->setTouched((bool)atoi(value.c_str()));
          }
          value = getTagValue(iroot, "ParameterSet");
          if (value.length() > 0) {
            imodel->setParameterSet(atoi(value.c_str()));
          }
          value = getTagValue(iroot, "FirstPerturbationSet");
          if (value.length() > 0) {
            imodel->setFirstPerturbationSet((bool)atoi(value.c_str()));
          }
          value = getTagValue(iroot, "SecondPerturbationSet");
          if (value.length() > 0) {
            imodel->setSecondPerturbationSet((bool)atoi(value.c_str()));
          }
          value = getTagValue(iroot, "InteractionOption");
          if (value.length() > 0) {
            imodel->setInteractionOption(atoi(value.c_str()));
          }
          value = getTagValue(iroot, "GridDimensions");
          if (value.length() > 0) {
            imodel->setGridDimensions(atoi(value.c_str()));
          }
          value = getTagValue(iroot, "EwaldConvergence");
          if (value.length() > 0) {
            imodel->setEwaldConvergence(atof(value.c_str()));
          }
          value = getTagValue(iroot, "BSplineOrder");
          if (value.length() > 0) {
            imodel->setBSplineOrder(atoi(value.c_str()));
          }
          value = getTagValue(iroot, "AdvancedFFT");
          if (value.length() > 0) {
            imodel->setAdvancedFFT((bool)atoi(value.c_str()));
          }
          value = getTagValue(iroot, "FFTNodes");
          if (value.length() > 0) {
            imodel->setFFTNodes(atoi(value.c_str()));
          }
          value = getTagValue(iroot, "FFTOption");
          if (value.length() > 0) {
            imodel->setFFTOption(atoi(value.c_str()));
          }
          value = getTagValue(iroot, "ChargeGridOption");
          if (value.length() > 0) {
            imodel->setChargeGridOption((bool)atoi(value.c_str()));
          }
          value = getTagValue(iroot, "MaxPolarizationIterations");
          if (value.length() > 0) {
            imodel->setMaximumPolarIter(atoi(value.c_str()));
          }
          value = getTagValue(iroot, "PolarizationTolerance");
          if (value.length() > 0) {
            imodel->setPolarizationTolerance(atof(value.c_str()));
          }
          value = getTagValue(iroot, "FirstOrderOption");
          if (value.length() > 0) {
            imodel->setFirstOrderOption((bool)atoi(value.c_str()));
          }
          value = getTagValue(iroot, "CutoffOption");
          if (value.length() > 0) {
            imodel->setCutoffOption(atoi(value.c_str()));
          }
          value = getTagValue(iroot, "ShortRangeCutoff");
          if (value.length() > 0) {
            imodel->setSRCutoff(atof(value.c_str()));
          }
          value = getTagValue(iroot, "LongRangeCutoff");
          if (value.length() > 0) {
            imodel->setLRCutoff(atof(value.c_str()));
          }
          value = getTagValue(iroot, "ShortListUpdateFrequency");
          if (value.length() > 0) {
            imodel->setShortListUpdateFreq(atoi(value.c_str()));
          }
          value = getTagValue(iroot, "LongListUpdateFrequency");
          if (value.length() > 0) {
            imodel->setLongListUpdateFreq(atoi(value.c_str()));
          }
        }
      }
      // Get Constraint model and associated parameters, if it exists
      ConstraintModel *cmodel = dobject->getConstraintModel();
      if (cmodel) {
        XMLCh *tagName = XMLString::transcode("Constraints");
        DOMNodeList *nodeList = root->getElementsByTagName(tagName);
        delete [] tagName;
        DOMElement *croot = (DOMElement*)nodeList->item(0);
        if (croot) {
          value = getTagValue(croot, "UseSHAKE");
          if (value.length() > 0) {
            cmodel->setUseSHAKE((bool)atoi(value.c_str()));
          }
          value = getTagValue(croot, "MaxSolventIter");
          if (value.length() > 0) {
            cmodel->setMaxSolventIter(atoi(value.c_str()));
          }
          value = getTagValue(croot, "SolventTol");
          if (value.length() > 0) {
            cmodel->setSolventTol(atof(value.c_str()));
          }
          value = getTagValue(croot, "MaxSoluteIter");
          if (value.length() > 0) {
            cmodel->setMaxSoluteIter(atoi(value.c_str()));
          }
          value = getTagValue(croot, "SoluteTol");
          if (value.length() > 0) {
            cmodel->setSoluteTol(atof(value.c_str()));
          }
          value = getTagValue(croot, "SHAKEOption");
          if (value.length() > 0) {
            cmodel->setSHAKEOpt(atoi(value.c_str()));
          }
          value = getTagValue(croot, "FixOption");
          if (value.length() > 0) {
            cmodel->setFixOption(atoi(value.c_str()));
          }
          value = getTagValue(croot, "SelectOption");
          if (value.length() > 0) {
            cmodel->setSelectOption(atoi(value.c_str()));
          }
          value = getTagValue(croot, "HeavyAtomsOnly");
          if (value.length() > 0) {
            cmodel->setHeavyAtomsOnly((bool)atoi(value.c_str()));
          }
          value = getTagValue(croot, "PermanentOption");
          if (value.length() > 0) {
            cmodel->setPermanentOption((bool)atoi(value.c_str()));
          }

          // Get Commands directly, do not retrieve CoordCommands first
          XMLCh *commandName = XMLString::transcode("Command");
          DOMNodeList *commands = croot->getElementsByTagName(commandName);
          delete [] commandName;

          // Restore commands
          int numFixAtoms = commands->getLength();
          cmodel->clearFixAtomCmd();
          for (int idx=0; idx<numFixAtoms; idx++) {
            DOMNode *node = commands->item(idx);
            if (node->getNodeType() == DOMNode::ELEMENT_NODE) {
              DOMNode *child = node->getFirstChild();
              if (child->getNodeType() == DOMNode::TEXT_NODE) {
                value = getAsString(child->getNodeValue());
                cmodel->appendFixAtomCmd(value);
              }
            }
          }
        }
      }

      // Get Optimize model and associated parameters, if it exists
      OptimizeModel *omodel = dobject->getOptimizeModel();
      if (omodel) {
        XMLCh *tagName = XMLString::transcode("Optimize");
        DOMNodeList *nodeList = root->getElementsByTagName(tagName);
        delete [] tagName;
        DOMElement *oroot = (DOMElement*)nodeList->item(0);
        if (oroot) {
          value = getTagValue(oroot, "UseSD");
          if (value.length() > 0) {
            omodel->setUseSD((bool)atoi(value.c_str()));
          }
          value = getTagValue(oroot, "SDMaxIterations");
          if (value.length() > 0) {
            omodel->setSDMaxIterations(atoi(value.c_str()));
          }
          value = getTagValue(oroot, "SDInitialStepSize");
          if (value.length() > 0) {
            omodel->setSDInitialStepSize(atof(value.c_str()));
          }
          value = getTagValue(oroot, "SDTolerance");
          if (value.length() > 0) {
            omodel->setSDTolerance(atof(value.c_str()));
          }
          value = getTagValue(oroot, "SDMaxStepSize");
          if (value.length() > 0) {
            omodel->setSDMaxStepSize(atof(value.c_str()));
          }
          value = getTagValue(oroot, "UseCG");
          if (value.length() > 0) {
            omodel->setUseCG((bool)atoi(value.c_str()));
          }
          value = getTagValue(oroot, "CGMaxIterations");
          if (value.length() > 0) {
            omodel->setCGMaxIterations(atoi(value.c_str()));
          }
          value = getTagValue(oroot, "CGInitialSearchInterval");
          if (value.length() > 0) {
            omodel->setCGInitialSearchInterval(atof(value.c_str()));
          }
          value = getTagValue(oroot, "CGThresholdSize");
          if (value.length() > 0) {
            omodel->setCGThresholdSize(atof(value.c_str()));
          }
          value = getTagValue(oroot, "CGRefreshCycle");
          if (value.length() > 0) {
            omodel->setCGRefreshCycle(atoi(value.c_str()));
          }
        }
      }

      // Get Control model and associated parameters, if it exists
      ControlModel *cntrlmodel = dobject->getControlModel();
      if (cntrlmodel) {
        XMLCh *tagName = XMLString::transcode("Control");
        DOMNodeList *nodeList = root->getElementsByTagName(tagName);
        delete [] tagName;
        DOMElement *croot = (DOMElement*)nodeList->item(0);
        if (croot) {
          value = getTagValue(croot, "LoadBalance");
          if (value.length() > 0) {
            cntrlmodel->setLoadBalance((bool)atoi(value.c_str()));
          }
          value = getTagValue(croot, "BalanceCellSize");
          if (value.length() > 0) {
            cntrlmodel->setBalanceCellSize((bool)atoi(value.c_str()));
          }
          value = getTagValue(croot, "BalanceZOnly");
          if (value.length() > 0) {
            cntrlmodel->setBalanceZOnly((bool)atoi(value.c_str()));
          }
          value = getTagValue(croot, "RedistributePairs");
          if (value.length() > 0) {
            cntrlmodel->setRedistributePairs((bool)atoi(value.c_str()));
          }
          value = getTagValue(croot, "CouplingStrength");
          if (value.length() > 0) {
            cntrlmodel->setCouplingStrength(atof(value.c_str()));
          }
          value = getTagValue(croot, "AdjustFrequency");
          if (value.length() > 0) {
            cntrlmodel->setAdjustFrequency(atoi(value.c_str()));
          }
          value = getTagValue(croot, "SynchOption");
          if (value.length() > 0) {
            cntrlmodel->setSynchOption(atoi(value.c_str()));
          }
          value = getTagValue(croot, "AdvancedConfigOpts");
          if (value.length() > 0) {
            cntrlmodel->setAdvancedConfigOpt((bool)atoi(value.c_str()));
          }
          value = getTagValue(croot, "ProcessorConfig");
          if (value.length() > 0) {
            cntrlmodel->setProcessorConfig((bool)atoi(value.c_str()));
          }
          value = getTagValue(croot, "XProc");
          if (value.length() > 0) {
            cntrlmodel->setXProc(atoi(value.c_str()));
          }
          value = getTagValue(croot, "YProc");
          if (value.length() > 0) {
            cntrlmodel->setYProc(atoi(value.c_str()));
          }
          value = getTagValue(croot, "ZProc");
          if (value.length() > 0) {
            cntrlmodel->setZProc(atoi(value.c_str()));
          }
          value = getTagValue(croot, "CellConfig");
          if (value.length() > 0) {
            cntrlmodel->setCellConfig((bool)atoi(value.c_str()));
          }
          value = getTagValue(croot, "XCell");
          if (value.length() > 0) {
            cntrlmodel->setXCell(atoi(value.c_str()));
          }
          value = getTagValue(croot, "YCell");
          if (value.length() > 0) {
            cntrlmodel->setYCell(atoi(value.c_str()));
          }
          value = getTagValue(croot, "ZCell");
          if (value.length() > 0) {
            cntrlmodel->setZCell(atoi(value.c_str()));
          }
          value = getTagValue(croot, "UseMaxSolvent");
          if (value.length() > 0) {
            cntrlmodel->setUseMaxSolvent((bool)atoi(value.c_str()));
          }
          value = getTagValue(croot, "MaxSolvent");
          if (value.length() > 0) {
            cntrlmodel->setMaxSolvent(atoi(value.c_str()));
          }
          value = getTagValue(croot, "UseMaxSolute");
          if (value.length() > 0) {
            cntrlmodel->setUseMaxSolute((bool)atoi(value.c_str()));
          }
          value = getTagValue(croot, "MaxSolute");
          if (value.length() > 0) {
            cntrlmodel->setMaxSolute(atoi(value.c_str()));
          }
          value = getTagValue(croot, "PackOption");
          if (value.length() > 0) {
            cntrlmodel->setPackOption((bool)atoi(value.c_str()));
          }
        }
      }

      // Get Dynamics model and associated parameters, if it exists
      DynamicsModel *dmodel = dobject->getDynamicsModel();
      if (dmodel) {
        XMLCh *tagName = XMLString::transcode("Dynamics");
        DOMNodeList *nodeList = root->getElementsByTagName(tagName);
        delete [] tagName;
        DOMElement *droot = (DOMElement*)nodeList->item(0);
        if (droot) {
          value = getTagValue(droot, "ResumeOpt");
          if (value.length() > 0) {
            dmodel->setResumeOpt((bool)atoi(value.c_str()));
          }
          value = getTagValue(droot, "IntegrationOpts");
          if (value.length() > 0) {
            dmodel->setIntegrationOpts(atoi(value.c_str()));
          }
          value = getTagValue(droot, "EquilibrationSteps");
          if (value.length() > 0) {
            dmodel->setEquilibrationSteps(atoi(value.c_str()));
          }
          value = getTagValue(droot, "DataSteps");
          if (value.length() > 0) {
            dmodel->setDataSteps(atoi(value.c_str()));
          }
          value = getTagValue(droot, "TimeStep");
          if (value.length() > 0) {
            dmodel->setTimeStep(atof(value.c_str()));
          }
          value = getTagValue(droot, "InitialTime");
          if (value.length() > 0) {
            dmodel->setInitialTime(atof(value.c_str()));
          }
          value = getTagValue(droot, "RecenterSystem");
          if (value.length() > 0) {
            dmodel->setRecenterSystem((bool)atoi(value.c_str()));
          }
          value = getTagValue(droot, "RecenterFreq");
          if (value.length() > 0) {
            dmodel->setRecenterFreq(atoi(value.c_str()));
          }
          value = getTagValue(droot, "RecenterOpts");
          if (value.length() > 0) {
            dmodel->setRecenterOpts(atoi(value.c_str()));
          }
          value = getTagValue(droot, "UseFractions");
          if (value.length() > 0) {
            dmodel->setUseFractions((bool)atoi(value.c_str()));
          }
          value = getTagValue(droot, "Fraction1");
          if (value.length() > 0) {
            dmodel->setFraction1(atoi(value.c_str()));
          }
          value = getTagValue(droot, "Fraction2");
          if (value.length() > 0) {
            dmodel->setFraction2(atoi(value.c_str()));
          }
          value = getTagValue(droot, "Fraction3");
          if (value.length() > 0) {
            dmodel->setFraction3(atoi(value.c_str()));
          }
          value = getTagValue(droot, "Fraction4");
          if (value.length() > 0) {
            dmodel->setFraction4(atoi(value.c_str()));
          }
          value = getTagValue(droot, "Fraction5");
          if (value.length() > 0) {
            dmodel->setFraction5(atoi(value.c_str()));
          }
          value = getTagValue(droot, "RemoveCM");
          if (value.length() > 0) {
            dmodel->setRemoveCM((bool)atoi(value.c_str()));
          }
          value = getTagValue(droot, "RemoveCMFreq");
          if (value.length() > 0) {
            dmodel->setRemoveCMFreq(atoi(value.c_str()));
          }
          value = getTagValue(droot, "UseNVT");
          if (value.length() > 0) {
            dmodel->setUseNVT((bool)atoi(value.c_str()));
          }
          value = getTagValue(droot, "NVTTemperature");
          if (value.length() > 0) {
            dmodel->setNVTTemperature(atof(value.c_str()));
          }
          value = getTagValue(droot, "UseDoubleRelaxation");
          if (value.length() > 0) {
            dmodel->setUseDoubleRelaxation((bool)atoi(value.c_str()));
          }
          value = getTagValue(droot, "SolventRxTime");
          if (value.length() > 0) {
            dmodel->setSolventRxTime(atof(value.c_str()));
          }
          value = getTagValue(droot, "SoluteRxTime");
          if (value.length() > 0) {
            dmodel->setSoluteRxTime(atof(value.c_str()));
          }
          value = getTagValue(droot, "UseAnnealing");
          if (value.length() > 0) {
            dmodel->setUseAnnealing((bool)atoi(value.c_str()));
          }
          value = getTagValue(droot, "FinalAnnealTemp");
          if (value.length() > 0) {
            dmodel->setFinalAnnealTemp(atof(value.c_str()));
          }
          value = getTagValue(droot, "InitAnnealTime");
          if (value.length() > 0) {
            dmodel->setInitAnnealTime(atof(value.c_str()));
          }
          value = getTagValue(droot, "FinalAnnealTime");
          if (value.length() > 0) {
            dmodel->setFinalAnnealTime(atof(value.c_str()));
          }
          value = getTagValue(droot, "UseNPT");
          if (value.length() > 0) {
            dmodel->setUseNPT((bool)atoi(value.c_str()));
          }
          value = getTagValue(droot, "NPTPressure");
          if (value.length() > 0) {
            dmodel->setNPTPressure(atof(value.c_str()));
          }
          value = getTagValue(droot, "NPTRxTime");
          if (value.length() > 0) {
            dmodel->setNPTRxTime(atof(value.c_str()));
          }
          value = getTagValue(droot, "Compressibility");
          if (value.length() > 0) {
            dmodel->setCompressibility(atof(value.c_str()));
          }
          value = getTagValue(droot, "VolumeAdjustAlg");
          if (value.length() > 0) {
            dmodel->setVolumeAdjustAlg(atoi(value.c_str()));
          }
          value = getTagValue(droot, "UseVelRetherm");
          if (value.length() > 0) {
            dmodel->setUseVelRetherm((bool)atoi(value.c_str()));
          }
          value = getTagValue(droot, "RethermFreq");
          if (value.length() > 0) {
            dmodel->setRethermFreq(atoi(value.c_str()));
          }
          value = getTagValue(droot, "RethermTemp");
          if (value.length() > 0) {
            dmodel->setRethermTemp(atof(value.c_str()));
          }
          value = getTagValue(droot, "RethermFrac");
          if (value.length() > 0) {
            dmodel->setRethermFrac(atof(value.c_str()));
          }
          value = getTagValue(droot, "RethermOpt");
          if (value.length() > 0) {
            dmodel->setRethermOpts(atoi(value.c_str()));
          }
        }
      }

      // Get Thermodynamics model and associated parameters, if it exists
      ThermodynamicsModel *tdmodel = dobject->getThermodynamicsModel();
      if (tdmodel) {
        XMLCh *tagName = XMLString::transcode("Thermodynamics");
        DOMNodeList *nodeList = root->getElementsByTagName(tagName);
        delete [] tagName;
        DOMElement *tdroot = (DOMElement*)nodeList->item(0);
        if (tdroot) {
          value = getTagValue(tdroot, "UsePmf");
          if (value.length() > 0) {
            tdmodel->setUsePmf((bool)atoi(value.c_str()));
          }
          value = getTagValue(tdroot, "PmfOpt");
          if (value.length() > 0) {
            tdmodel->setPmfOpt(atoi(value.c_str()));
          }
          value = getTagValue(tdroot, "PmfScale");
          if (value.length() > 0) {
            tdmodel->setPmfScale(atof(value.c_str()));
          }
          value = getTagValue(tdroot, "PmfEquilharm");
          if (value.length() > 0) {
            tdmodel->setPmfEquilharm(atoi(value.c_str()));
          }
          value = getTagValue(tdroot, "UseTherm");
          if (value.length() > 0) {
            tdmodel->setUseThermoIntegration((bool)atoi(value.c_str()));
          }
          value = getTagValue(tdroot, "ThermInitialConditionsOpt");
          if (value.length() > 0) {
            tdmodel->setInitialConditionsOpt(atoi(value.c_str()));
          }
          value = getTagValue(tdroot, "ThermDirectionOpt");
          if (value.length() > 0) {
            tdmodel->setDirectionOpt(atoi(value.c_str()));
          }
          value = getTagValue(tdroot, "ThermEnsembleNumber");
          if (value.length() > 0) {
            tdmodel->setEnsembleNumber(atoi(value.c_str()));
          }
          value = getTagValue(tdroot, "ThermEnsembleMax");
          if (value.length() > 0) {
            tdmodel->setEnsembleMax(atoi(value.c_str()));
          }
          value = getTagValue(tdroot, "ThermErrorMax");
          if (value.length() > 0) {
            tdmodel->setErrorMax(atof(value.c_str()));
          }
          value = getTagValue(tdroot, "ThermDriftMax");
          if (value.length() > 0) {
            tdmodel->setDriftMax(atof(value.c_str()));
          }
        } else {
          tdmodel->reset();
        }
      }

       // Get Files model and associated parameters, if it exists
      FilesModel *fmodel = dobject->getFilesModel();
      if (fmodel) {
        XMLCh *tagName = XMLString::transcode("Files");
        DOMNodeList *nodeList = root->getElementsByTagName(tagName);
        delete [] tagName;
        DOMElement *froot = (DOMElement*)nodeList->item(0);
        if (froot) {
          value = getTagValue(froot, "RestartFreq");
          if (value.length() > 0) {
            fmodel->setRestartFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "KeepRestart");
          if (value.length() > 0) {
            fmodel->setKeepRestart((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "StoreTrajectory");
          if (value.length() > 0) {
            fmodel->setTrajectoryStorage(atoi(value.c_str()));
          }
          value = getTagValue(froot, "UseBatches");
          if (value.length() > 0) {
            fmodel->setUseBatches((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "BatchSize");
          if (value.length() > 0) {
            fmodel->setBatchSize(atoi(value.c_str()));
          }
          value = getTagValue(froot, "CoordinatesSolutePrint");
          if (value.length() > 0) {
            fmodel->setCoordsSolutePrint((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "CoordinatesSoluteFreq");
          if (value.length() > 0) {
            fmodel->setCoordsSoluteFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "CoordinatesSolventPrint");
          if (value.length() > 0) {
            fmodel->setCoordsSolventPrint((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "CoordinatesSolventFreq");
          if (value.length() > 0) {
            fmodel->setCoordsSolventFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "VelocitiesSolutePrint");
          if (value.length() > 0) {
            fmodel->setVelocitySolutePrint((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "VelocitiesSoluteFreq");
          if (value.length() > 0) {
            fmodel->setVelocitySoluteFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "VelocitiesSolventPrint");
          if (value.length() > 0) {
            fmodel->setVelocitySolventPrint((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "VelocitiesSolventFreq");
          if (value.length() > 0) {
            fmodel->setVelocitySolventFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "ForcesSolutePrint");
          if (value.length() > 0) {
            fmodel->setForcesSolutePrint((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "ForcesSoluteFreq");
          if (value.length() > 0) {
            fmodel->setForcesSoluteFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "ForcesSolventPrint");
          if (value.length() > 0) {
            fmodel->setForcesSolventPrint((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "ForcesSolventFreq");
          if (value.length() > 0) {
            fmodel->setForcesSolventFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "WritePolarization");
          if (value.length() > 0) {
            fmodel->setWritePolarization((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "PolarizationOpt");
          if (value.length() > 0) {
            fmodel->setPolarizationOpt(atoi(value.c_str()));
          }
          value = getTagValue(froot, "PolarizationSoluteFreq");
          if (value.length() > 0) {
            fmodel->setPolarizationSoluteFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "PolarizationSolventFreq");
          if (value.length() > 0) {
            fmodel->setPolarizationSolventFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "AverageProperties");
          if (value.length() > 0) {
            fmodel->setAvgProperties((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "PropertyFreq");
          if (value.length() > 0) {
            fmodel->setPropertyFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "PrintMD");
          if (value.length() > 0) {
            fmodel->setPrintMD((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "PrintMDFreq");
          if (value.length() > 0) {
            fmodel->setPrintMDFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "SolventEnergy");
          if (value.length() > 0) {
            fmodel->setSolventEnergy((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "SoluteEnergy");
          if (value.length() > 0) {
            fmodel->setSoluteEnergy((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "PrintStat");
          if (value.length() > 0) {
            fmodel->setPrintStat((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "StatFreq");
          if (value.length() > 0) {
            fmodel->setStatFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "SoluteBonds");
          if (value.length() > 0) {
            fmodel->setSoluteBonds((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "BondFreq");
          if (value.length() > 0) {
            fmodel->setBondFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "SoluteForces");
          if (value.length() > 0) {
            fmodel->setSoluteForces((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "ForceFreq");
          if (value.length() > 0) {
            fmodel->setForceFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "PmfData");
          if (value.length() > 0) {
            fmodel->setPmfData((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "PmfFreq");
          if (value.length() > 0) {
            fmodel->setPmfFreq(atoi(value.c_str()));
          }
          value = getTagValue(froot, "PrintTop");
          if (value.length() > 0) {
            fmodel->setPrintTop((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "NbondPar");
          if (value.length() > 0) {
            fmodel->setNbondPar((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "SolventBondPar");
          if (value.length() > 0) {
            fmodel->setSolventBondPar((bool)atoi(value.c_str()));
          }
          value = getTagValue(froot, "SoluteBondPar");
          if (value.length() > 0) {
            fmodel->setSoluteBondPar((bool)atoi(value.c_str()));
          }
        }
      }
      
      doc->release();
   } else {
      cerr << "Dynamic cast to NWChemMDModel failed" << endl;
   }
}

void NWChemMDModelXMLizer::serialize(const Serializable& object,
                        string& data, EcceMap& properties)
{
   data = "";

   const NWChemMDModel *dobject = dynamic_cast<const NWChemMDModel*>(&object);
   MdTask *mdTask = 0;

   if (dobject) {
      ostrstream os;
      os << xmlHeader() << endl;

      os << "<" << ROOTELEMENT << ">" << endl;

      // System Name
      try {
        mdTask = dynamic_cast<MdTask*>(EDSIFactory::getResource((dobject->getUrl())));
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

      // Calc Id
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

      // MD Output File Name
      try {
        mdTask = dynamic_cast<MdTask*>(EDSIFactory::getResource(dobject->getUrl()));
        if (mdTask != 0) {
          string mdOutputName = mdTask->getMdOutputName();
          if (mdOutputName.length() > 0) {
            os << "<MdOutputName>" << mdOutputName << "</MdOutputName>"<< endl;
          }
          mdTask = 0;
        }
      }
      catch (...) {
        mdTask = 0;
      }

      if (dobject->getInteractionModel()) {
        os << "  <Interaction>" << endl;
        dumpInteraction(dobject->getInteractionModel(), os);
        os << "  </Interaction>" << endl;
      }
      if (dobject->getConstraintModel()) {
        os << "  <Constraints>" << endl;
        dumpConstraint(dobject->getConstraintModel(), os);
        os << "  </Constraints>" << endl;
      }
      if (dobject->getOptimizeModel()) {
        os << "  <Optimize>" << endl;
        dumpOptimize(dobject->getOptimizeModel(), os);
        os << "  </Optimize>" << endl;
      }
      if (dobject->getControlModel()) {
        os << "  <Control>" << endl;
        dumpControl(dobject->getControlModel(), os);
        os << "  </Control>" << endl;
      }
      if (dobject->getDynamicsModel()) {
        os << "  <Dynamics>" << endl;
        dumpDynamics(dobject->getDynamicsModel(), os);
        os << "  </Dynamics>" << endl;
      }
      if (dobject->getThermodynamicsModel()) {
        os << "  <Thermodynamics>" << endl;
        dumpThermodynamics(dobject->getThermodynamicsModel(), os);
        os << "  </Thermodynamics>" << endl;
      }
      if (dobject->getFilesModel()) {
        os << "  <Files>" << endl;
        dumpFiles(dobject->getFilesModel(), os);
        os << "  </Files>" << endl;
      }
      os << "</" << ROOTELEMENT << ">" << endl;
      os << ends;

      data = os.str();
   } else {
      cerr << "Dynamic cast to NWChemMDModel failed" << endl;
   }
}

void NWChemMDModelXMLizer::dumpInteraction(InteractionModel *model, ostream &os)
{
  os <<"    <Touched>1</Touched>"<<endl;
  if (model->getParameterSet() != model->PARAMSET) {
    os << "    <ParameterSet>"<< model->getParameterSet()
       << "</ParameterSet>" << endl;
  }
  os << "    <FirstPerturbationSet>" << model->getFirstPerturbationSet()
     << "</FirstPerturbationSet>" << endl;
  os << "    <SecondPerturbationSet>" << model->getSecondPerturbationSet()
     << "</SecondPerturbationSet>" << endl;
  os << "    <InteractionOption>" << model->getInteractionOption()
     << "</InteractionOption>" << endl;
  os << "    <GridDimensions>" << model->getGridDimensions()
     << "</GridDimensions>" << endl;
  if (model->getEwaldConvergence() != model->EWALDCONVRG) {
    os << "    <EwaldConvergence>" << model->getEwaldConvergence()
       << "</EwaldConvergence>" << endl;
  }
  if (model->getBSplineOrder() != model->BSPLINEORDER) {
    os << "    <BSplineOrder>" << model->getBSplineOrder()
       << "</BSplineOrder>" << endl;
  }
  os << "    <AdvancedFFT>" << model->getAdvancedFFT()
     << "</AdvancedFFT>" << endl;
  if (model->getFFTNodes() != model->NFFT) {
    os << "    <FFTNodes>" << model->getFFTNodes()
       << "</FFTNodes>" << endl;
  }
  if (model->getFFTOption() != model->FFTOPTION) {
    os << "    <FFTOption>" << model->getFFTOption()
       << "</FFTOption>" << endl;
  }
  if (model->getChargeGridOption() != model->USESOLVENTONLY) {
    os << "    <ChargeGridOption>" << model->getChargeGridOption()
       << "</ChargeGridOption>" << endl;
  }
  if (model->getMaximumPolarIter() != model->MAXPOLITERATION) {
    os << "    <MaxPolarizationIterations>" << model->getMaximumPolarIter()
       << "</MaxPolarizationIterations>" << endl;
  }
  os << "    <PolarizationTolerance>" << model->getPolarizationTolerance()
     << "</PolarizationTolerance>" << endl;
  os << "    <FirstOrderOption>" << model->getFirstOrderOption()
     << "</FirstOrderOption>" << endl;
  int opt = model->getCutoffOption();
  os << "    <CutoffOption>" << opt
     << "</CutoffOption>" << endl;
  if (model->getSRCutoff() != model->SHORTCUT ||
      opt != model->CUTOFFOPTION) {
    os << "    <ShortRangeCutoff>" << model->getSRCutoff()
       << "</ShortRangeCutoff>" << endl;
  }
  if (model->getSRCutoff() != model->LONGCUT ||
      opt != model->CUTOFFOPTION) {
    os << "    <LongRangeCutoff>" << model->getLRCutoff()
       << "</LongRangeCutoff>" << endl;
  }
  if (model->getLongListUpdateFreq() != model->SHORTFREQ) {
    os << "    <ShortListUpdateFrequency>" << model->getShortListUpdateFreq()
       << "</ShortListUpdateFrequency>" << endl;
  }
  if (model->getLongListUpdateFreq() != model->LONGFREQ) {
    os << "    <LongListUpdateFrequency>" << model->getLongListUpdateFreq()
       << "</LongListUpdateFrequency>" << endl;
  }
}

void NWChemMDModelXMLizer::dumpConstraint(ConstraintModel *model, ostream &os)
{
  os << "    <UseSHAKE>" << model->getUseSHAKE()
     << "</UseSHAKE>" << endl;
  if (model->getMaxSolventIter() != model->MAXSLVITER ||
      model->getMaxSoluteIter() != model->MAXSLUITER) {
    os << "    <MaxSolventIter>" << model->getMaxSolventIter()
       << "</MaxSolventIter>" << endl;
  }
  if (model->getSolventTol() != model->SLVTOL ||
      model->getSoluteTol() != model->SLUTOL) {
    os << "    <SolventTol>" << model->getSolventTol()
       << "</SolventTol>" << endl;
  }
  if (model->getMaxSoluteIter() != model->MAXSLUITER) {
    os << "    <MaxSoluteIter>" << model->getMaxSoluteIter()
       << "</MaxSoluteIter>" << endl;
  }
  if (model->getSoluteTol() != model->SLUTOL) {
    os << "    <SoluteTol>" << model->getSoluteTol()
       << "</SoluteTol>" << endl;
  }
  if (model->getSHAKEOpt() != model->SHAKEOPT) {
    os << "    <SHAKEOption>" << model->getSHAKEOpt()
       << "</SHAKEOption>" << endl;
  }
  if (model->getFixAtomRadius() != model->RADIUS) {
    os << "    <FixAtomRadius>" << model->getFixAtomRadius()
       << "</FixAtomRadius>" << endl;
  }

  if (model->getFixOption() != model->FIXOPTION) {
    os << "    <FixOption>" << model->getFixOption()
       << "</FixOption>" << endl;
  }

  if (model->getSelectOption() != model->SELECTOPTION) {
    os << "    <SelectOption>" << model->getSelectOption()
       << "</SelectOption>" << endl;
  }

  if (model->getPermanentOption() != model->PERMANENT) {
    os << "    <PermanentOption>" << model->getPermanentOption()
       << "</PermanentOption>" << endl;
  }

  if (model->getHeavyAtomsOnly() != model->HEAVYONLY) {
    os << "    <HeavyAtomsOnly>" << model->getHeavyAtomsOnly()
       << "</HeavyAtomsOnly>" << endl;
  }

  // Add Fix Atom Commands
  vector<string> fixAtomCmds = model->getFixAtomCmds();
  int numCmds = fixAtomCmds.size();
  if (numCmds > 0) {
    os << "    <FixAtomCmds>" << endl;
    for (int idx=0; idx<numCmds; idx++ ) {
      os << "      <Command>";
      os << fixAtomCmds[idx];
      os << "</Command>" << endl;
    }
    os << "    </FixAtomCmds>" << endl;
  }
}

void NWChemMDModelXMLizer::dumpOptimize(OptimizeModel *model, ostream &os)
{
  os << "    <UseSD>" << (int)model->getUseSD()
     << "</UseSD>" << endl;
  os << "    <SDMaxIterations>" << model->getSDMaxIterations()
     << "</SDMaxIterations>" << endl;
  if (model->getSDInitialStepSize() != model->SDISTEPSIZE) {
    os << "    <SDInitialStepSize>" << model->getSDInitialStepSize()
       << "</SDInitialStepSize>" << endl;
  }
  if (model->getSDTolerance() != model->SDTOLERANCE) {
    os << "    <SDTolerance>" << model->getSDTolerance()
       << "</SDTolerance>" << endl;
  }
  if (model->getSDMaxStepSize() != model->SDMAXSTEPSIZE) {
    os << "    <SDMaxStepSize>" << model->getSDMaxStepSize()
       << "</SDMaxStepSize>" << endl;
  }
  os << "    <UseCG>" << (int)model->getUseCG()
     << "</UseCG>" << endl;
  os << "    <CGMaxIterations>" << model->getCGMaxIterations()
     << "</CGMaxIterations>" << endl;
  if (model->getCGInitialSearchInterval() != model->CGSEARCHINT) {
    os << "    <CGInitialSearchInterval>" << model->getCGInitialSearchInterval()
       << "</CGInitialSearchInterval>" << endl;
  }
  if (model->getCGThresholdSize() != model->CGTHRESHOLD) {
    os << "    <CGThresholdSize>" << model->getCGThresholdSize()
       << "</CGThresholdSize>" << endl;
  }
  if (model->getCGRefreshCycle() != model->CGREFRESHINT) {
    os << "    <CGRefreshCycle>" << model->getCGRefreshCycle()
       << "</CGRefreshCycle>" << endl;
  }
}

void NWChemMDModelXMLizer::dumpControl(ControlModel *model, ostream &os)
{
  os << "    <LoadBalance>"<< model->getLoadBalance()
     << "</LoadBalance>" << endl;
  os << "    <BalanceCellSize>"<< model->getBalanceCellSize()
     << "</BalanceCellSize>" << endl;
  os << "    <BalanceZOnly>"<< model->getBalanceZOnly()
     << "</BalanceZOnly>" << endl;
  os << "    <RedistributePairs>"<< model->getRedistributePairs()
     << "</RedistributePairs>" << endl;
  os << "    <CouplingStrength>"<< model->getCouplingStrength()
     << "</CouplingStrength>" << endl;
  os << "    <AdjustFrequency>"<< model->getAdjustFrequency()
       << "</AdjustFrequency>" << endl;
  if (model->getRedistributePairs()) {
    if (model->getSynchOption() != model->SYNCHOPT) {
      os << "    <SynchOption>"<< model->getSynchOption()
         << "</SynchOption>" << endl;
    }
  }
  os << "    <AdvancedConfigOpts>"<< model->getAdvancedConfigOpt()
     << "</AdvancedConfigOpts>" << endl;
  os << "    <ProcessorConfig>"<< model->getProcessorConfig()
     << "</ProcessorConfig>" << endl;
  os << "    <XProc>"<< model->getXProc()
     << "</XProc>" << endl;
  os << "    <YProc>"<< model->getYProc()
     << "</YProc>" << endl;
  os << "    <ZProc>"<< model->getZProc()
     << "</ZProc>" << endl;
  os << "    <CellConfig>"<< model->getCellConfig()
     << "</CellConfig>" << endl;
  os << "    <XCell>"<< model->getXCell()
     << "</XCell>" << endl;
  os << "    <YCell>"<< model->getYCell()
     << "</YCell>" << endl;
  os << "    <ZCell>"<< model->getZCell()
     << "</ZCell>" << endl;
  os << "    <UseMaxSolvent>"<< model->getUseMaxSolvent()
     << "</UseMaxSolvent>" << endl;
  os << "    <MaxSolvent>"<< model->getMaxSolvent()
     << "</MaxSolvent>" << endl;
  os << "    <UseMaxSolute>"<< model->getUseMaxSolute()
     << "</UseMaxSolute>" << endl;
  os << "    <MaxSolute>"<< model->getMaxSolute()
     << "</MaxSolute>" << endl;
  if (model->getPackOption() != model->PACK) {
    os << "    <PackOption>"<< model->getPackOption()
       << "</PackOption>" << endl;
  }
}

void NWChemMDModelXMLizer::dumpDynamics(DynamicsModel *model, ostream &os)
{
  os << "    <ResumeOpt> "<< model->getResumeOpt()
     << "</ResumeOpt>" << endl;
  os << "    <IntegrationOpts> "<< model->getIntegrationOpts()
     << "</IntegrationOpts>" << endl;
  os << "    <EquilibrationSteps> "<< model->getEquilibrationSteps()
     << "</EquilibrationSteps>" << endl;
  os << "    <TimeStep> "<< model->getTimeStep()
     << "</TimeStep>" << endl;
  os << "    <DataSteps> "<< model->getDataSteps()
       << "</DataSteps>" << endl;
  if (model->getInitialTime() != model->INITTIME ) {
    os << "    <InitialTime> "<< model->getInitialTime()
       << "</InitialTime>" << endl;
  }
  if (model->getRecenterSystem() != model->RECENTER ) {
    os << "    <RecenterSystem> "<< model->getRecenterSystem()
       << "</RecenterSystem>" << endl;
  }
  if (model->getRecenterFreq() != model->RECENTERFREQ ||
      model->getRecenterSystem()) {
    os << "    <RecenterFreq> "<< model->getRecenterFreq()
       << "</RecenterFreq>" << endl;
  }
  if (model->getRecenterOpts() != model->RECENTEROPT ) {
    os << "    <RecenterOpts> "<< model->getRecenterOpts()
       << "</RecenterOpts>" << endl;
  }
  if (model->getUseFractions() != model->USEFRAC ) {
    os << "    <UseFractions> "<< model->getUseFractions()
       << "</UseFractions>" << endl;
  }
  if (model->getFraction1() != model->FRAC1 ) {
    os << "    <Fraction1> "<< model->getFraction1()
       << "</Fraction1>" << endl;
  }
  if (model->getFraction2() != model->FRAC2 ) {
    os << "    <Fraction2> "<< model->getFraction2()
       << "</Fraction2>" << endl;
  }
  if (model->getFraction3() != model->FRAC3 ) {
    os << "    <Fraction3> "<< model->getFraction3()
       << "</Fraction3>" << endl;
  }
  if (model->getFraction4() != model->FRAC4 ) {
    os << "    <Fraction4> "<< model->getFraction4()
       << "</Fraction4>" << endl;
  }
  if (model->getFraction5() != model->FRAC5 ) {
    os << "    <Fraction5> "<< model->getFraction5()
       << "</Fraction5>" << endl;
  }
  os << "    <RemoveCM> "<< model->getRemoveCM()
     << "</RemoveCM>" << endl;
  if (model->getRemoveCMFreq() != model->REMOVECMFREQ ||
      model->getRemoveCM() ) {
    os << "    <RemoveCMFreq> "<< model->getRemoveCMFreq()
       << "</RemoveCMFreq>" << endl;
  }
  os << "    <UseNVT> "<< model->getUseNVT()
     << "</UseNVT>" << endl;
  os << "    <NVTTemperature> "<< model->getNVTTemperature()
     << "</NVTTemperature>" << endl;
  os << "    <UseDoubleRelaxation> "<< model->getUseDoubleRelaxation()
     << "</UseDoubleRelaxation>" << endl;
  bool use2Rx = model->getUseDoubleRelaxation();
  if (model->getSolventRxTime() != model->SLVRXTIME || use2Rx ) {
    os << "    <SolventRxTime> "<< model->getSolventRxTime()
       << "</SolventRxTime>" << endl;
  }
  if (model->getSoluteRxTime() != model->SLURXTIME  || use2Rx) {
    os << "    <SoluteRxTime> "<< model->getSoluteRxTime()
       << "</SoluteRxTime>" << endl;
  }
  os << "    <UseAnnealing> "<< model->getUseAnnealing()
     << "</UseAnnealing>" << endl;
  bool useAnnl = model->getUseAnnealing();
  if (model->getFinalAnnealTemp() != model->FINALTEMP || useAnnl) {
    os << "    <FinalAnnealTemp> "<< model->getFinalAnnealTemp()
       << "</FinalAnnealTemp>" << endl;
  }
  if (model->getInitAnnealTime() != model->INITANNEALTIME || useAnnl) {
    os << "    <InitAnnealTime> "<< model->getInitAnnealTime()
       << "</InitAnnealTime>" << endl;
  }
  if (model->getFinalAnnealTime() != model->FINALANNEALTIME || useAnnl) {
    os << "    <FinalAnnealTime> "<< model->getFinalAnnealTime()
       << "</FinalAnnealTime>" << endl;
  }
  os << "    <UseNPT> "<< model->getUseNPT()
     << "</UseNPT>" << endl;
  if (model->getNPTPressure() != model->NPTPRSSR ) {
    os << "    <NPTPressure> "<< model->getNPTPressure()
       << "</NPTPressure>" << endl;
  }
  if (model->getNPTRxTime() != model->NPTRXTIME ) {
    os << "    <NPTRxTime> "<< model->getNPTRxTime()
       << "</NPTRxTime>" << endl;
  }
  if (model->getCompressibility() != model->COMPRESSIBILITY ) {
    os << "    <Compressibility> "<< model->getCompressibility()
       << "</Compressibility>" << endl;
  }
  if (model->getVolumeAdjustAlg() != model->VOLADJUSTALG ) {
    os << "    <VolumeAdjustAlg> "<< model->getVolumeAdjustAlg()
       << "</VolumeAdjustAlg>" << endl;
  }
  os << "    <UseVelRetherm> "<< model->getUseVelRetherm()
     << "</UseVelRetherm>" << endl;
  bool useTherm = model->getUseVelRetherm();
  if (model->getRethermFreq() != model->RETHERMFREQ || useTherm) {
    os << "    <RethermFreq> "<< model->getRethermFreq()
       << "</RethermFreq>" << endl;
  }
  if (model->getRethermTemp() != model->RETHERMTEMP || useTherm) {
    os << "    <RethermTemp> "<< model->getRethermTemp()
       << "</RethermTemp>" << endl;
  }
  if (model->getRethermFrac() != model->RETHERMFRAC ) {
    os << "    <RethermFrac> "<< model->getRethermFrac()
       << "</RethermFrac>" << endl;
  }
  if (model->getRethermOpts() != model->RETHERMOPT ) {
    os << "    <RethermOpt> "<< model->getRethermOpts()
       << "</RethermOpt>" << endl;
  }
}


void NWChemMDModelXMLizer::dumpThermodynamics(ThermodynamicsModel *model, ostream &os)
{
  bool usePmf = model->getUsePmf();
  os << "    <UsePmf>" << usePmf
     << "</UsePmf>" << endl;
  if (model->getPmfOpt() != model->PMFOPT || usePmf) {
    os << "    <PmfOpt>" << model->getPmfOpt()
       << "</PmfOpt>" << endl;
  }
  if (model->getPmfScale() != model->PMFSCALE || usePmf) {
    os << "    <PmfScale>" << model->getPmfScale()
       << "</PmfScale>" << endl;
  }
  if (model->getPmfEquilharm() != model->PMFEQUILHARM || usePmf) {
    os << "    <PmfEquilharm>" << model->getPmfEquilharm() 
       << "</PmfEquilharm>" << endl;
  }
  bool useTherm = model->getUseThermoIntegration();
  os << "    <UseTherm>" << useTherm
     << "</UseTherm>" << endl;
  if (model->getInitialConditionsOpt() != model->INITIALCONDITIONSOPTION ||
      useTherm) {
    os << "    <ThermInitialConditionsOpt>"<<model->getInitialConditionsOpt()
       << "</ThermInitialConditionsOpt>" << endl;
  }
  if (model->getDirectionOpt() != model->DIRECTIONOPTION || useTherm) {
    os << "    <ThermDirectionOpt>" << model->getDirectionOpt()
       << "</ThermDirectionOpt>" << endl;
  }
  if (model->getEnsembleNumber() != model->ENSEMBLESNUM || useTherm) {
    os << "    <ThermEnsembleNumber>" << model->getEnsembleNumber()
       << "</ThermEnsembleNumber>" << endl;
  }
  if (model->getEnsembleMax() != model->ENSEMBLESMAX || useTherm) {
    os << "    <ThermEnsembleMax>" << model->getEnsembleMax()
       << "</ThermEnsembleMax>" << endl;
  }
  if (model->getErrorMax() != model->ERRORMAX || useTherm) {
    os << "    <ThermErrorMax>" << model->getErrorMax()
       << "</ThermErrorMax>" << endl;
  }
  if (model->getDriftMax() != model->DRIFTMAX || useTherm) {
    os << "    <ThermDriftMax>" << model->getDriftMax()
       << "</ThermDriftMax>" << endl;
  }
}


void NWChemMDModelXMLizer::dumpFiles(FilesModel *model, ostream &os)
{
  os << "    <RestartFreq> "<<model->getRestartFreq()
     << "</RestartFreq>" << endl;
  os << "    <KeepRestart> "<<model->getKeepRestart()
     << "</KeepRestart>" << endl;
  os << "    <WriteTrajectory> "<<model->getWriteTrajectory()
     << "</WriteTrajectory>" << endl;
  os << "    <StoreTrajectory> "<<model->getTrajectoryStorage()
     << "</StoreTrajectory>" << endl;
  os << "    <UseBatches> "<<model->getUseBatches()
     << "</UseBatches>" << endl;
  os << "    <BatchSize> "<<model->getBatchSize()
     << "</BatchSize>" << endl;

  os << "    <CoordinatesSolutePrint> "<<model->getCoordsSolutePrint()
     << "</CoordinatesSolutePrint>" << endl;
  os << "    <CoordinatesSoluteFreq> "<<model->getCoordsSoluteFreq()
     << "</CoordinatesSoluteFreq>" << endl;
  os << "    <CoordinatesSolventPrint> "<<model->getCoordsSolventPrint()
     << "</CoordinatesSolventPrint>" << endl;
  os << "    <CoordinatesSolventFreq> "<<model->getCoordsSolventFreq()
     << "</CoordinatesSolventFreq>" << endl;

  os << "    <VelocitiesSolutePrint> "<<model->getVelocitySolutePrint()
     << "</VelocitiesSolutePrint>" << endl;
  os << "    <VelocitiesSoluteFreq> "<<model->getVelocitySoluteFreq()
     << "</VelocitiesSoluteFreq>" << endl;
  os << "    <VelocitiesSolventPrint> "<<model->getVelocitySolventPrint()
     << "</VelocitiesSolventPrint>" << endl;
  os << "    <VelocitiesSolventFreq> "<<model->getVelocitySolventFreq()
     << "</VelocitiesSolventFreq>" << endl;

  os << "    <ForcesSolutePrint> "<<model->getForcesSolutePrint()
     << "</ForcesSolutePrint>" << endl;
  os << "    <ForcesSoluteFreq> "<<model->getForcesSoluteFreq()
     << "</ForcesSoluteFreq>" << endl;
  os << "    <ForcesSolventPrint> "<<model->getForcesSolventPrint()
     << "</ForcesSolventPrint>" << endl;
  os << "    <ForcesSolventFreq> "<<model->getForcesSolventFreq()
     << "</ForcesSolventFreq>" << endl;

  os << "    <WritePolarization> "<<model->getWritePolarization()
     << "</WritePolarization>" << endl;
  os << "    <PolarizationOpt> "<<model->getPolarizationOpt()
     << "</PolarizationOpt>" << endl;
  os << "    <PolarizationSoluteFreq> "<<model->getPolarizationSoluteFreq()
     << "</PolarizationSoluteFreq>" << endl;
  os << "    <PolarizationSolventFreq> "<<model->getPolarizationSolventFreq()
     << "</PolarizationSolventFreq>" << endl;

  os << "    <AverageProperties> "<<model->getAvgProperties()
     << "</AverageProperties>" << endl;
  os << "    <PropertyFreq> "<<model->getPropertyFreq()
     << "</PropertyFreq>" << endl;

  os << "    <PrintMD> "<<model->getPrintMD()
     << "</PrintMD>" << endl;
  os << "    <PrintMDFreq> "<<model->getPrintMDFreq()
     << "</PrintMDFreq>" << endl;
  os << "    <SolventEnergy> "<<model->getSolventEnergy()
     << "</SolventEnergy>" << endl;
  os << "    <SoluteEnergy> "<<model->getSoluteEnergy()
     << "</SoluteEnergy>" << endl;
  os << "    <PrintStat> "<<model->getPrintStat()
     << "</PrintStat>" << endl;
  os << "    <StatFreq> "<<model->getStatFreq()
     << "</StatFreq>" << endl;
  os << "    <SoluteBonds> "<<model->getSoluteBonds()
     << "</SoluteBonds>" << endl;
  os << "    <BondFreq> "<<model->getBondFreq()
     << "</BondFreq>" << endl;
  os << "    <SoluteForces> "<<model->getSoluteForces()
     << "</SoluteForces>" << endl;
  os << "    <ForceFreq> "<<model->getForceFreq()
     << "</ForceFreq>" << endl;
  os << "    <PmfData> "<<model->getPmfData()
     << "</PmfData>" << endl;
  os << "    <PmfFreq> "<<model->getPmfFreq()
     << "</PmfFreq>" << endl;
  os << "    <PrintTop> "<<model->getPrintTop()
     << "</PrintTop>" << endl;
  os << "    <NbondPar> "<<model->getNbondPar()
     << "</NbondPar>" << endl;
  os << "    <SolventBondPar> "<<model->getSolventBondPar()
     << "</SolventBondPar>" << endl;
  os << "    <SoluteBondPar> "<<model->getSoluteBondPar()
     << "</SoluteBondPar>" << endl;
}

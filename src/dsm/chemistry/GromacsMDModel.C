/**
 * @file
 */

#include "dsm/GromacsMDModel.H"


GromacsMDModel::GromacsMDModel() : MDCompositeModel()
{
   setTitle("GROMACS MD");
   setUrl("");
}

GromacsMDModel::GromacsMDModel(const vector<MDCompositeModel::GUIPanel>& panels)
   : MDCompositeModel(panels)
{
   setTitle("GROMACS MD");
   setUrl("");
   setGromacsDefaults();
}

GromacsMDModel::GromacsMDModel(const ResourceDescriptor::CONTENTTYPE task)
   : MDCompositeModel(panelsFor(task))
{
   setTitle("GROMACS MD");
   setUrl("");
   setGromacsDefaults();
}

GromacsMDModel::~GromacsMDModel()
{
}

string GromacsMDModel::codeName() const
{
   return "GROMACS";
}

void GromacsMDModel::applyDefaults()
{
   setGromacsDefaults();
}

/**
 * Starting values for a GROMACS task where NWChem's would not be a sensible
 * first try: PME rather than a plain cutoff, a 2 fs step (bonds to hydrogen
 * are constrained), steepest descent with a force tolerance grompp
 * understands, a thermostat on, and some steps to run.  A value the XML
 * writer leaves out when it equals NWChem's default is not set here, since
 * a task would then reload with the wrong value (the writer writes every
 * value for a GROMACS task; see NWChemMDModelXMLizer::serialize).
 */
void GromacsMDModel::setGromacsDefaults()
{
   if (p_interactionModel) {
      p_interactionModel->setInteractionOption(0);
   }
   if (p_optimizeModel) {
      p_optimizeModel->setUseCG(false);
      p_optimizeModel->setSDMaxIterations(500);
      p_optimizeModel->setSDInitialStepSize(0.01);
      p_optimizeModel->setSDTolerance(100.0);
   }
   if (p_dynamicsModel) {
      p_dynamicsModel->setTimeStep(0.002);
      p_dynamicsModel->setDataSteps(5000);
      p_dynamicsModel->setUseNVT(true);
      p_dynamicsModel->setNVTTemperature(300.0);
      p_dynamicsModel->setRemoveCM(true);
   }
}

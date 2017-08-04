/**
 * @file
 *
 *
 */

#include "util/NotImplementedException.H"
#include "util/StringTokenizer.H"
#include "util/STLUtil.H"

#include "dsm/NWChemMDModel.H"


/**
 * Constructor
 */
NWChemMDModel::NWChemMDModel() : ITaskModel()
{
   setTitle("NWChem MD");
   setUrl("");
   p_interactionModel = 0;
   p_constraintModel = 0;
   p_controlModel = 0;
   p_dynamicsModel = 0;
   p_thermoModel = 0;
   p_optimizeModel = 0;
   p_filesModel = 0;
}

/**
 * Constructor
 */
NWChemMDModel::NWChemMDModel(vector<NWChemMDModel::GUIPanel> panels) : ITaskModel()
{
   setTitle("NWChem MD");
   setUrl("");

   p_interactionModel = 0;
   p_constraintModel = 0;
   p_controlModel = 0;
   p_dynamicsModel = 0;
   p_thermoModel = 0;
   p_optimizeModel = 0;
   p_filesModel = 0;

   int pmax = panels.size();
   for (int i=0; i<pmax; i++) {
     if (panels[i] == NWChemMDModel::INTERACTION)
       p_interactionModel = new InteractionModel();
     if (panels[i] == NWChemMDModel::CONSTRAINT)
       p_constraintModel = new ConstraintModel();
     if (panels[i] == NWChemMDModel::CONTROL)
       p_controlModel = new ControlModel();
     if (panels[i] == NWChemMDModel::OPTIMIZE)
       p_optimizeModel = new OptimizeModel();
     if (panels[i] == NWChemMDModel::DYNAMICS)
       p_dynamicsModel = new DynamicsModel();
     if (panels[i] == NWChemMDModel::THERMODYNAMICS)
       p_thermoModel = new ThermodynamicsModel();
     if (panels[i] == NWChemMDModel::FILES)
       p_filesModel = new FilesModel();
   }
}

/**
 * Constructor
 */
NWChemMDModel::NWChemMDModel(const ResourceDescriptor::CONTENTTYPE task) : ITaskModel()
{
  vector<NWChemMDModel::GUIPanel> panels;
  panels.push_back(NWChemMDModel::INTERACTION);
  panels.push_back(NWChemMDModel::CONSTRAINT);
  panels.push_back(NWChemMDModel::CONTROL);
  panels.push_back(NWChemMDModel::FILES);
  if (task == ResourceDescriptor::CT_MDOPTIMIZE) {
    panels.push_back(NWChemMDModel::OPTIMIZE);
    panels.push_back(NWChemMDModel::THERMODYNAMICS);
  } else if (task == ResourceDescriptor::CT_MDDYNAMICS) {
    panels.push_back(NWChemMDModel::DYNAMICS);
    panels.push_back(NWChemMDModel::THERMODYNAMICS);
  }

  setTitle("NWChem MD");
  setUrl("");

  p_interactionModel = 0;
  p_constraintModel = 0;
  p_controlModel = 0;
  p_dynamicsModel = 0;
  p_thermoModel = 0;
  p_optimizeModel = 0;
  p_filesModel = 0;

  int pmax = panels.size();
  for (int i=0; i<pmax; i++) {
    if (panels[i] == NWChemMDModel::INTERACTION)
      p_interactionModel = new InteractionModel();
    if (panels[i] == NWChemMDModel::CONSTRAINT)
      p_constraintModel = new ConstraintModel();
    if (panels[i] == NWChemMDModel::CONTROL)
      p_controlModel = new ControlModel();
    if (panels[i] == NWChemMDModel::OPTIMIZE)
      p_optimizeModel = new OptimizeModel();
    if (panels[i] == NWChemMDModel::DYNAMICS)
      p_dynamicsModel = new DynamicsModel();
    if (panels[i] == NWChemMDModel::THERMODYNAMICS)
      p_thermoModel = new ThermodynamicsModel();
    if (panels[i] == NWChemMDModel::FILES)
      p_filesModel = new FilesModel();
  }
}

/**
 * Constructor
 */
NWChemMDModel::NWChemMDModel(const string& url, const string& name) : ITaskModel()
{
   setTitle(name);
   setUrl(url);
   p_interactionModel = 0;
   p_constraintModel = 0;
   p_controlModel = 0;
   p_dynamicsModel = 0;
   p_thermoModel = 0;
   p_optimizeModel = 0;
   p_filesModel = 0;
}

/**
 * Destructor
 */
NWChemMDModel::~NWChemMDModel()
{
   if (p_interactionModel) delete p_interactionModel;
   if (p_constraintModel) delete p_constraintModel;
   if (p_controlModel) delete p_controlModel;
   if (p_optimizeModel) delete p_optimizeModel;
   if (p_dynamicsModel) delete p_dynamicsModel;
   if (p_thermoModel) delete p_thermoModel;
   if (p_filesModel) delete p_filesModel;

}

/**
 * Modifiers
 */
void NWChemMDModel::setInteractionModel(InteractionModel *model)
{
  p_interactionModel = model;
}

void NWChemMDModel::setConstraintModel(ConstraintModel *model)
{
  p_constraintModel = model;
}

void NWChemMDModel::setOptimizeModel(OptimizeModel *model)
{
  p_optimizeModel = model;
}

void NWChemMDModel::setControlModel(ControlModel *model)
{
  p_controlModel = model;
}

void NWChemMDModel::setDynamicsModel(DynamicsModel *model)
{
  p_dynamicsModel = model;
}

void NWChemMDModel::setThermodynamicsModel(ThermodynamicsModel *model)
{
  p_thermoModel = model;
}

void NWChemMDModel::setFilesModel(FilesModel *model)
{
  p_filesModel = model;
}

/**
 * Accessors
 */
InteractionModel* NWChemMDModel::getInteractionModel() const
{
  return p_interactionModel;
}

ConstraintModel* NWChemMDModel::getConstraintModel() const
{
  return p_constraintModel;
}

OptimizeModel* NWChemMDModel::getOptimizeModel() const
{
  return p_optimizeModel;
}

ControlModel* NWChemMDModel::getControlModel() const
{
  return p_controlModel;
}

DynamicsModel* NWChemMDModel::getDynamicsModel() const
{
  return p_dynamicsModel;
}

ThermodynamicsModel* NWChemMDModel::getThermodynamicsModel() const
{
  return p_thermoModel;
}

FilesModel* NWChemMDModel::getFilesModel() const
{
  return p_filesModel;
}

void NWChemMDModel::generateInputFile() throw(InvalidException)
{
   throw NotImplementedException("generateInputFile not implemented",WHERE);
}

void NWChemMDModel::run() throw(InvalidException)
{
   throw NotImplementedException("run not implemented",WHERE);
}

/**
 * reset all models to defaults
 */
void NWChemMDModel::reset()
{
  if (p_interactionModel) {
    p_interactionModel->reset();
  }
  if (p_constraintModel) {
    p_constraintModel->reset();
  }
  if (p_controlModel) {
    p_controlModel->reset();
  }
  if (p_dynamicsModel) {
    p_dynamicsModel->reset();
  }
  if (p_optimizeModel) {
    p_optimizeModel->reset();
  }
  if (p_filesModel) {
    p_filesModel->reset();
  }
}

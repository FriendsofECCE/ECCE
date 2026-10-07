/**
 * @file
 */

#include "util/NotImplementedException.H"

#include "dsm/MDCompositeModel.H"


MDCompositeModel::MDCompositeModel() : ITaskModel()
{
   p_interactionModel = 0;
   p_constraintModel = 0;
   p_controlModel = 0;
   p_dynamicsModel = 0;
   p_thermoModel = 0;
   p_optimizeModel = 0;
   p_filesModel = 0;
}

MDCompositeModel::MDCompositeModel(const vector<GUIPanel>& panels) : ITaskModel()
{
   p_interactionModel = 0;
   p_constraintModel = 0;
   p_controlModel = 0;
   p_dynamicsModel = 0;
   p_thermoModel = 0;
   p_optimizeModel = 0;
   p_filesModel = 0;
   createModels(panels);
}

MDCompositeModel::~MDCompositeModel()
{
   if (p_interactionModel) delete p_interactionModel;
   if (p_constraintModel) delete p_constraintModel;
   if (p_controlModel) delete p_controlModel;
   if (p_optimizeModel) delete p_optimizeModel;
   if (p_dynamicsModel) delete p_dynamicsModel;
   if (p_thermoModel) delete p_thermoModel;
   if (p_filesModel) delete p_filesModel;
}

void MDCompositeModel::createModels(const vector<GUIPanel>& panels)
{
   int pmax = panels.size();
   for (int i=0; i<pmax; i++) {
     if (panels[i] == INTERACTION)
       p_interactionModel = new InteractionModel();
     if (panels[i] == CONSTRAINT)
       p_constraintModel = new ConstraintModel();
     if (panels[i] == CONTROL)
       p_controlModel = new ControlModel();
     if (panels[i] == OPTIMIZE)
       p_optimizeModel = new OptimizeModel();
     if (panels[i] == DYNAMICS)
       p_dynamicsModel = new DynamicsModel();
     if (panels[i] == THERMODYNAMICS)
       p_thermoModel = new ThermodynamicsModel();
     if (panels[i] == FILES)
       p_filesModel = new FilesModel();
   }
}

vector<MDCompositeModel::GUIPanel>
MDCompositeModel::panelsFor(ResourceDescriptor::CONTENTTYPE task)
{
  vector<GUIPanel> panels;
  panels.push_back(INTERACTION);
  panels.push_back(CONSTRAINT);
  panels.push_back(CONTROL);
  panels.push_back(FILES);
  if (task == ResourceDescriptor::CT_MDOPTIMIZE) {
    panels.push_back(OPTIMIZE);
    panels.push_back(THERMODYNAMICS);
  } else if (task == ResourceDescriptor::CT_MDDYNAMICS ||
             task == ResourceDescriptor::CT_MDEQUILIBRATE) {
    panels.push_back(DYNAMICS);
    panels.push_back(THERMODYNAMICS);
  }
  return panels;
}

void MDCompositeModel::setInteractionModel(InteractionModel *model)
{
  p_interactionModel = model;
}

void MDCompositeModel::setConstraintModel(ConstraintModel *model)
{
  p_constraintModel = model;
}

void MDCompositeModel::setOptimizeModel(OptimizeModel *model)
{
  p_optimizeModel = model;
}

void MDCompositeModel::setControlModel(ControlModel *model)
{
  p_controlModel = model;
}

void MDCompositeModel::setDynamicsModel(DynamicsModel *model)
{
  p_dynamicsModel = model;
}

void MDCompositeModel::setThermodynamicsModel(ThermodynamicsModel *model)
{
  p_thermoModel = model;
}

void MDCompositeModel::setFilesModel(FilesModel *model)
{
  p_filesModel = model;
}

InteractionModel* MDCompositeModel::getInteractionModel() const
{
  return p_interactionModel;
}

ConstraintModel* MDCompositeModel::getConstraintModel() const
{
  return p_constraintModel;
}

OptimizeModel* MDCompositeModel::getOptimizeModel() const
{
  return p_optimizeModel;
}

ControlModel* MDCompositeModel::getControlModel() const
{
  return p_controlModel;
}

DynamicsModel* MDCompositeModel::getDynamicsModel() const
{
  return p_dynamicsModel;
}

ThermodynamicsModel* MDCompositeModel::getThermodynamicsModel() const
{
  return p_thermoModel;
}

FilesModel* MDCompositeModel::getFilesModel() const
{
  return p_filesModel;
}

void MDCompositeModel::generateInputFile()
{
   throw NotImplementedException("generateInputFile not implemented",WHERE);
}

void MDCompositeModel::run()
{
   throw NotImplementedException("run not implemented",WHERE);
}

/**
 * reset all models to defaults (the thermodynamics model has never been
 * part of this; kept as it was)
 */
void MDCompositeModel::reset()
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
  applyDefaults();
}

void MDCompositeModel::applyDefaults()
{
}

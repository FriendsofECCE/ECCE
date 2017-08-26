/**
 * @file
 *
 *
 */
#ifndef NWCHEMMDMODEL_HH
#define NWCHEMMDMODEL_HH

#include <string>
#include <vector>

using std::string;
using std::vector;

#include "util/IOException.hpp"

#include "tdat/ITaskModel.hpp"
#include "tdat/InteractionModel.hpp"
#include "tdat/ConstraintModel.hpp"
#include "tdat/OptimizeModel.hpp"
#include "tdat/DynamicsModel.hpp"
#include "tdat/ThermodynamicsModel.hpp"
#include "tdat/ControlModel.hpp"
#include "tdat/FilesModel.hpp"

#include "dsm/ResourceDescriptor.hpp"


/**
 * Model for implementing NWChem MD including theories energy, opt...
 */
class NWChemMDModel : public ITaskModel
{
   public:
      enum GUIPanel {INTERACTION, CONSTRAINT, DYNAMICS, THERMODYNAMICS, 
                     OPTIMIZE, CONTROL, FILES};
      /* Constructors */
      NWChemMDModel();
      NWChemMDModel(vector<NWChemMDModel::GUIPanel>);
      NWChemMDModel(const string& url, const string& name);
      NWChemMDModel(const ResourceDescriptor::CONTENTTYPE task);

      virtual ~NWChemMDModel();

      /* Task Model virtuals */
      virtual void generateInputFile() throw(InvalidException);
      virtual void run() throw(InvalidException);

      /* Set pointers for models */
      void setInteractionModel(InteractionModel *model);
      void setConstraintModel(ConstraintModel *model);
      void setOptimizeModel(OptimizeModel *model);
      void setControlModel(ControlModel *model);
      void setDynamicsModel(DynamicsModel *model);
      void setThermodynamicsModel(ThermodynamicsModel *model);
      void setFilesModel(FilesModel *model);

      /* Get pointers for models */
      InteractionModel* getInteractionModel() const;
      ConstraintModel* getConstraintModel() const;
      OptimizeModel* getOptimizeModel() const;
      ControlModel* getControlModel() const;
      DynamicsModel* getDynamicsModel() const;
      ThermodynamicsModel* getThermodynamicsModel() const;
      FilesModel* getFilesModel() const;

      /**
       * Reset all model values back to their default values
       */
      void reset();

   protected:
      InteractionModel    *p_interactionModel;
      ConstraintModel     *p_constraintModel;
      OptimizeModel       *p_optimizeModel;
      ControlModel        *p_controlModel;
      DynamicsModel       *p_dynamicsModel;
      ThermodynamicsModel *p_thermoModel;
      FilesModel          *p_filesModel;

   private:

};
#endif

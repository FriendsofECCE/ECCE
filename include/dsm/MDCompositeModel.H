/**
 * @file
 *
 * The part of an MD task model that does not depend on the MD code: the
 * seven generic sub-models the editor panels bind to.
 */
#ifndef MDCOMPOSITEMODEL_HH
#define MDCOMPOSITEMODEL_HH

#include <string>
#include <vector>

using std::string;
using std::vector;

#include "tdat/ITaskModel.H"
#include "tdat/InteractionModel.H"
#include "tdat/ConstraintModel.H"
#include "tdat/OptimizeModel.H"
#include "tdat/DynamicsModel.H"
#include "tdat/ThermodynamicsModel.H"
#include "tdat/ControlModel.H"
#include "tdat/FilesModel.H"

#include "dsm/ResourceDescriptor.H"

/**
 * Base of NWChemMDModel and GromacsMDModel: owns the sub-models and the
 * accessors MDEdBase and the MD editors use, so the editors need not know
 * which code a task belongs to.  A subclass names its code and may adjust
 * defaults; the sub-models and their XML form are shared.
 */
class MDCompositeModel : public ITaskModel
{
   public:
      enum GUIPanel {INTERACTION, CONSTRAINT, DYNAMICS, THERMODYNAMICS,
                     OPTIMIZE, CONTROL, FILES};

      virtual ~MDCompositeModel();

      /** Short code name used in titles, e.g. "NWChem" or "GROMACS". */
      virtual string codeName() const = 0;

      /* Task Model virtuals */
      virtual void generateInputFile();
      virtual void run();

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
      /** Called at the end of reset(): a code's own starting values. */
      virtual void applyDefaults();

      MDCompositeModel();
      MDCompositeModel(const vector<GUIPanel>& panels);

      /** Panels a task of the given content type shows. */
      static vector<GUIPanel> panelsFor(ResourceDescriptor::CONTENTTYPE task);

      void createModels(const vector<GUIPanel>& panels);

      InteractionModel    *p_interactionModel;
      ConstraintModel     *p_constraintModel;
      OptimizeModel       *p_optimizeModel;
      ControlModel        *p_controlModel;
      DynamicsModel       *p_dynamicsModel;
      ThermodynamicsModel *p_thermoModel;
      FilesModel          *p_filesModel;
};
#endif

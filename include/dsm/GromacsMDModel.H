/**
 * @file
 */
#ifndef GROMACSMDMODEL_HH
#define GROMACSMDMODEL_HH

#include "dsm/MDCompositeModel.H"

/**
 * MD task model for GROMACS.  It carries the same sub-models as NWChem's
 * (and so the same XML form and the same editor panels); what differs is
 * the code it names, the defaults it starts from, and which of the values
 * mean anything to the .mdp generator (scripts/parsers/md.gmxtask).
 */
class GromacsMDModel : public MDCompositeModel
{
   public:
      GromacsMDModel();
      GromacsMDModel(const vector<MDCompositeModel::GUIPanel>& panels);
      GromacsMDModel(const ResourceDescriptor::CONTENTTYPE task);
      virtual ~GromacsMDModel();

      virtual string codeName() const;

   protected:
      virtual void applyDefaults();

   private:
      void setGromacsDefaults();
};
#endif

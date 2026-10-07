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

#include "util/IOException.H"

#include "dsm/MDCompositeModel.H"


/**
 * Model for implementing NWChem MD including theories energy, opt...
 */
class NWChemMDModel : public MDCompositeModel
{
   public:
      /* Constructors */
      NWChemMDModel();
      NWChemMDModel(vector<NWChemMDModel::GUIPanel>);
      NWChemMDModel(const string& url, const string& name);
      NWChemMDModel(const ResourceDescriptor::CONTENTTYPE task);

      virtual ~NWChemMDModel();

      virtual string codeName() const;
};
#endif

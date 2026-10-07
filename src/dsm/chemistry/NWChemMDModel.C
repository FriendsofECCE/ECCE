/**
 * @file
 *
 *
 */

#include "dsm/NWChemMDModel.H"


NWChemMDModel::NWChemMDModel() : MDCompositeModel()
{
   setTitle("NWChem MD");
   setUrl("");
}

NWChemMDModel::NWChemMDModel(vector<NWChemMDModel::GUIPanel> panels)
   : MDCompositeModel(panels)
{
   setTitle("NWChem MD");
   setUrl("");
}

NWChemMDModel::NWChemMDModel(const ResourceDescriptor::CONTENTTYPE task)
   : MDCompositeModel(panelsFor(task))
{
  setTitle("NWChem MD");
  setUrl("");
}

NWChemMDModel::NWChemMDModel(const string& url, const string& name)
   : MDCompositeModel()
{
   setTitle(name);
   setUrl(url);
}

NWChemMDModel::~NWChemMDModel()
{
}

string NWChemMDModel::codeName() const
{
   return "NWChem";
}

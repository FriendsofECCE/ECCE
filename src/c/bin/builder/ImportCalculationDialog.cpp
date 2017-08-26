
#include "util/ResourceUtils.hpp"
#include "util/STLUtil.hpp"

#include "dsm/IPropCalculation.hpp"
#include "dsm/ResourceType.hpp"

#include "CalculationFactory.hpp"
#include "ImportCalculationDialog.hpp"


ImportCalculationDialog::ImportCalculationDialog()
  : OpenCalculationDialog()
{
}


ImportCalculationDialog::ImportCalculationDialog(wxWindow *parent)
  : OpenCalculationDialog()
{
  Create(parent);
  restoreSettings();
}
    

ImportCalculationDialog::~ImportCalculationDialog()
{
  saveSettings();
}


bool ImportCalculationDialog::Create(wxWindow *parent)
{
  if (!OpenCalculationDialog::Create(parent)) {
    return false;
  }

  SetName("ImportCalculationDialog");
  SetMessage("Load Chemical System into Current Context");

  return true;
}


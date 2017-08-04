
#include "util/ResourceUtils.H"
#include "util/STLUtil.H"

#include "dsm/IPropCalculation.H"
#include "dsm/ResourceType.H"

#include "CalculationFactory.H"
#include "ImportCalculationDialog.H"


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


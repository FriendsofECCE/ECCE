
#include "util/ResourceUtils.H"
#include "util/STLUtil.H"

#include "dsm/IPropCalculation.H"
#include "dsm/ResourceType.H"

#include "CalculationFactory.H"
#include "OpenCalculationDialog.H"


OpenCalculationDialog::OpenCalculationDialog()
  : ewxGenericFileDialog()
{
}


OpenCalculationDialog::OpenCalculationDialog(wxWindow *parent)
  : ewxGenericFileDialog()
{
  Create(parent);
  restoreSettings();
}
    

OpenCalculationDialog::~OpenCalculationDialog()
{
  saveSettings();
}


bool OpenCalculationDialog::Create(wxWindow *parent)
{
  map<string,string> filters;
  map<string,string>::iterator filtersIt;
  string currentWildcard;
  string filter_string;
  
  PropCalcInfoMap infoMap = CalculationInfo::GetInfo();
  PropCalcInfoMapIter infoIt;
  for (infoIt = infoMap.begin(); infoIt != infoMap.end(); ++infoIt) {
    const CalculationInfo *info = infoIt->second;
    if (info->GetOpenMask() != "") {
      filters[info->GetLabel()] = info->GetOpenMask();
    }
  }

  // convert filter vectors into single string that dialog expects
  // insert the "all types" first
  filter_string = "All Supported Types|"+CalculationFactory::openMask()+"|";
  for (filtersIt = filters.begin(); filtersIt != filters.end(); ++filtersIt) {
    filter_string.append(filtersIt->first + "|");
    filter_string.append(filtersIt->second + "|");
  }
  filter_string.erase(filter_string.end() - 1); // remove last "|"

  if (!ewxGenericFileDialog::Create(parent,
          "Open in New Context",
          wxEmptyString, wxEmptyString, filter_string,
          wxFD_OPEN|wxFD_FILE_MUST_EXIST)) {
    return false;
  }

  SetName("OpenCalculationDialog");

  return true;
}


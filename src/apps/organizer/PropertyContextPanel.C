/**
 * @file
 *
 *
 */


#ifndef WX_PRECOMP
#include "wx/wx.h"
#include "wx/generic/grid.h"
#endif


#include <iostream>
  using std::cerr;
  using std::cout;
  using std::endl;
     

#include "util/NullPointerException.H"

#include "dsm/EDSI.H"
#include "dsm/DavEDSI.H"
#include "dsm/EDSIFactory.H"

#include "wxgui/ewxPanel.H"
#include "wxgui/WxResourceTreeItemData.H"
#include "wxgui/GridModel.H"
#include "wxgui/WxGridView.H"
#include "wxgui/GridElement.H"

#include "PropertyContextPanel.H"

class CalcMgr;

/**
 * Constructor
 */
PropertyContextPanel::PropertyContextPanel(CalcMgr * calcMgr,
                                           WxResourceTreeItemData *propertyData,
                                           wxWindow* parent,
                                           wxWindowID id,
                                           const wxPoint& pos,
                                           const wxSize& size,
                                           long style):
  PropertyContextPanelGUI(parent, id, pos, size, style),
  ContextPanelInterface(calcMgr)
{
  p_propView = 0;
  p_propData = propertyData;
  initialize();
}


/**
 * Desctructor
 */
PropertyContextPanel::~PropertyContextPanel()
{
  if (p_propView != 0) {
    delete p_propView;
    p_propView = 0;
  }
}


void PropertyContextPanel::refresh()
{
  initialize();
}


/**
 * Load property data.
 *
 * @param propertyData pointer to a tree item containing the 
 *        property data
 *
*/
void PropertyContextPanel::initialize()
{
  NULLPOINTEREXCEPTION(
    p_propData, 
    "Null Resource pointer in PropertyContextPanel::setContext");

  // Create WxGridView and place on panel
  if (p_propView != 0) {
    p_propView->Destroy();
    p_propView = 0;
  }
  wxSizer *sizer = this->GetSizer();
  p_propView = new WxGridView(this, wxID_ANY);
  sizer->Add(p_propView, 1, wxGROW|wxALL, 5);

  // Get Properties
  vector<MetaDataRequest> requests;
  vector<MetaDataResult> results;
  EDSI *edsi = EDSIFactory::getEDSI(p_propData->getUrl());
  if (edsi->getMetaData(requests, results, false)) {

    // Create data model
    GridModel *gridModel = new GridModel();
    
    // Create columns
    gridModel->InsertCols(0, NUMBER_OF_COLS);


    // Create/Load Rows
    for (int i = 0; i < results.size(); i++) {

      gridModel->InsertRows(i);

      // Determine property namespace and name values
      string nameSpace = "";
      string name = "";
      int strIndex = results[i].name.rfind(':');
      if (strIndex != string::npos) {
        name = results[i].name.substr(strIndex);
        name.erase(name.rfind(':'), 1);
        nameSpace = results[i].name.substr(0, strIndex + 1);
        nameSpace.erase(nameSpace.rfind(':'), 1);
      }

      // Load row

      // Namespace
      GridElement *namespaceCell = new GridElement();
      namespaceCell->setValue(nameSpace);
      gridModel->SetValueAsGridElement(i, NAMESPACE_COL, namespaceCell);

      // Name
      GridElement *nameCell = new GridElement();
      nameCell->setValue(name);
      gridModel->SetValueAsGridElement(i, NAME_COL, nameCell);

      // Value
      GridElement *valueCell = new GridElement();
      valueCell->setValue(results[i].value);
      gridModel->SetValueAsGridElement(i, VALUE_COL, valueCell);

    }

    // Go ahead and assign data model to grid, 
    // set properties, and refresh - WxGridView 
    // takes ownership of gridModel pointer
    p_propView->setModel(gridModel);

    // Column Labels
    p_propView->setColLabel(NAMESPACE_COL, "Namespace");
    p_propView->setColLabel(NAME_COL, "Name");
    p_propView->setColLabel(VALUE_COL, "Value");

    // Col Alignments
    p_propView->setColAlignment(NAMESPACE_COL, wxALIGN_LEFT, wxALIGN_CENTER);
    p_propView->setColAlignment(NAME_COL, wxALIGN_LEFT, wxALIGN_CENTER);
    p_propView->setColAlignment(VALUE_COL, wxALIGN_LEFT, wxALIGN_CENTER);


    // Enable grid lines
    p_propView->enableGridLines();

    // Sort by Namespace
    p_propView->sortByCol(NAMESPACE_COL, true);

  }

  // Delete EDSI
  if (edsi != 0) {
    delete edsi;
    edsi = 0;
  }

}

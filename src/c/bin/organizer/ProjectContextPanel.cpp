/**
 * @file
 *
 *
 */
#include <iostream>
  using std::cerr;
  using std::endl;
#include <set>
  using std::set;

#include "util/EcceMap.H"
#include "util/InvalidException.H"
#include "util/ErrMsg.H"

#include "dsm/ResourceDescriptor.H"
#include "dsm/Resource.H"
#include "dsm/PropertyView.H"
#include "dsm/ResourceProperty.H"
#include "dsm/GridProperty.H"

#include "wxgui/ewxPanel.H"
#include "wxgui/ewxStyledWindow.H"
#include "wxgui/WxResourceTreeItemData.H"
#include "wxgui/ewxGrid.H"
#include "wxgui/GridModel.H"
#include "wxgui/GridPropElement.H"
#include "wxgui/WxGridView.H"

#include "CalcMgr.H"
#include "ProjectContextPanel.H"


IMPLEMENT_CLASS(ProjectContextPanel, ProjectContextPanelGUI)


BEGIN_EVENT_TABLE(ProjectContextPanel, ProjectContextPanelGUI)
  EVT_GRID_CELL_LEFT_DCLICK(ProjectContextPanel::gridCellLeftDClick)
  EVT_GRID_CELL_RIGHT_CLICK(ProjectContextPanel::gridCellRightClick)
  EVT_GRID_LABEL_LEFT_CLICK(ProjectContextPanel::gridLabelLeftClick)
  EVT_GRID_RANGE_SELECT(ProjectContextPanel::gridRangeSelect)
  EVT_KEY_DOWN(ProjectContextPanel::gridOnKeyDown)
  EVT_MENU(ewxGrid::ID_EXPORT, ProjectContextPanel::onExport)
END_EVENT_TABLE()


/**
 * Constructor.
 */
ProjectContextPanel::ProjectContextPanel(CalcMgr *calcmgr,
        WxResourceTreeItemData *project, wxWindow* parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style)
  : ProjectContextPanelGUI(parent, id, pos, size, style)
  , ContextPanelInterface(calcmgr)
{
  // Initialize data members
  p_calcMgr = calcmgr;
  p_project = 0;
  p_projectView = 0;
  p_initialized = false;

  // First make sure that this is a collection resource
  EE_RT_ASSERT(((project->getResource()->getResourceType() == ResourceDescriptor::RT_COLLECTION) ||
                (project->getResource()->getResourceType() == ResourceDescriptor::RT_VIRTUAL_DOCUMENT)), 
               EE_FATAL, 
               "WxResourceTreeItemData must point to a collection resource");

  // Save Resource
  p_project = project;

  // Load view with default property view
  // This creates the WxGridView (ie p_projectView)
  // and loads resources and their property information
  initialize();

  // Sort grid
  int sortCol = p_projectView->getSortCol();
  bool ascSort = p_projectView->getAscSort();
  if (sortCol >= 0) {
    p_projectView->sortByCol(sortCol, ascSort);
  }
  else {
    p_projectView->defaultSort();
  }

  p_selections.clear();
  p_initialized = true;
}


/**
 * Destructor. 
 */
ProjectContextPanel::~ProjectContextPanel()
{
  if (p_projectView != 0) {
    delete p_projectView;
  }
}


void ProjectContextPanel::refresh()
{
  p_initialized = false;

  GridPropElement *element = 0;

  // Save current row selections - url's
  set<string> selUrls;
  vector<EcceURL> selRows = getSelections();
  for (int i = 0; i < selRows.size(); i++) {
    selUrls.insert(selRows[i].toString());
  }
  
  // Save sort information
  string sortProp = "";
  GridProperty::PropertyType sortPropType = GridProperty::PT_UNDEFINED;
  int sortCol = p_projectView->getSortCol();
  element = dynamic_cast<GridPropElement*>(p_projectView->getElement(0, sortCol));
  if (element != 0) {
    ResourceProperty *rp = dynamic_cast<ResourceProperty*>(element->getProperty());
    if (rp != 0) {
      sortProp = rp->getQName();
      sortPropType = rp->getType();
    }
  }

  // Initialize/refresh grid with property view
  initialize();
  
  // Restore selections, if any
  if (selUrls.size() > 0) {
    int rows = p_projectView->getRowCount();
    bool setGridCursor = false;
    for (int i = 0; i < rows; i++) {
      element = dynamic_cast<GridPropElement*>(p_projectView->getElement(i, 0));
      if (element != 0) {
        string res = element->getResource()->getURL().toString();
        if (selUrls.find(res) != selUrls.end()) {
          if (!setGridCursor) {
            p_projectView->setCursor(i, 0);
            setGridCursor = true;
          }
          else {
            p_projectView->selectRow(i, true);
          }
        }
      }
    }
  }

  // Restore sort
  int newSortCol;
  bool sortColFound = false;
  int cols = p_projectView->getColCount();
  for (int i = 0; i < cols; i++) {
    element = dynamic_cast<GridPropElement*>(p_projectView->getElement(0, i));
    if (element != 0) {
      ResourceProperty *rp = dynamic_cast<ResourceProperty*>(element->getProperty());
      if (rp != 0) {
        if ((sortProp == rp->getQName()) &&
            (sortPropType == rp->getType())) {
          newSortCol = i;
          sortColFound = true;
          break;
        }
      }
    }
  }
  if (sortColFound) {
    //p_projectView->setAscSort(!ascSort);
    p_projectView->sortByCol(newSortCol, p_projectView->getAscSort());
  }
  else {
    p_projectView->defaultSort();
  }

  p_initialized = true;
}




/**
 * Initialize the grid using the default property view.
 */
void ProjectContextPanel::initialize()
{
  p_propertyView = ResourceDescriptor::getResourceDescriptor().
    getPropertyView(p_calcMgr->getProjectViewId());

  // Create WxGridView and place on panel
  if (p_projectView != 0) {
    p_projectView->Destroy();
    p_projectView = 0;
  }
  wxSizer *sizer = this->GetSizer();
  p_projectView = new WxGridView(this, wxID_ANY, ORGANIZER);
  p_projectView->getGrid()->enableContextMenu(false);
  sizer->Add(p_projectView, 1, wxGROW|wxALL, 0);

  // Get properties to view on grid
  multimap<int, ResourceProperty*> properties = 
    p_propertyView->getDisplayedProperties();
  
  // Get data model
  GridModel *gridModel = new GridModel();

  // Create grid columns
  gridModel->InsertCols(0, properties.size());

  // Get rows, if none then remove columns
  vector<Resource*> *children = p_project->getResource()->getChildren();

  if (children == 0) {
    gridModel->DeleteCols(0, properties.size());
  }
  else if (children->size() == 0) {
    gridModel->DeleteCols(0, properties.size());
  }
  else {
    // Load rows - if any
    for (int i = 0; i < children->size(); i++) {
      
      // Default is to insert a single row
      gridModel->InsertRows(i);
      
      Resource *resource = (*children)[i];
      int column = 0;
      multimap<int, ResourceProperty*>::iterator mapIt =
        properties.begin();
      
      while (mapIt != properties.end()) {
        
        ResourceProperty *property = mapIt->second;

        /* If it is not a read only property then 
        // assign this as the rename property - only 
        // one column should be registered as editable
        // for the project panel and it should be the 
        // resource name column.  This is a registry
        // pointer, so no need for memory management.
        if (!property->getReadOnly()) {
          p_renameCol = column;
        }
        */

        try {
          GridPropElement *cellValue = 
            new GridPropElement(resource, property);
          gridModel->SetValueAsGridElement(i, column, cellValue);
        }
        catch (InvalidException ex) {
          cerr << ex.what() << endl;
        }
        
        if (i == 0) {
          gridModel->SetColLabelValue(column, property->getLabel());
        }
        gridModel->SetRowLabelValue(i,"");
        
        mapIt++;
        column++;
      }
    }
    sizer->Layout();
  }

  // Go ahead and assign data model to grid
  // WxGridView takes ownership of pointer
  p_projectView->setModel(gridModel);

  // Restore sort settings
  restoreSortSettings();
}


void ProjectContextPanel::saveSelections()
{
  vector<EcceURL> selections;

  set<int> selRows = p_projectView->getSelectedRows();
  set<int>::iterator setIt;

  for (setIt = selRows.begin();
       setIt != selRows.end();
       setIt++) {

    int row = *setIt;
    int col = 0;

    GridPropElement *element = 
      dynamic_cast<GridPropElement*>(p_projectView->getElement(row, col));
    
    if (element != 0) {
      Resource *resource = element->getResource();
      if (resource != 0) {
        selections.push_back(resource->getURL());
      }
    }
  }

  p_selections = selections;
}


vector<EcceURL> ProjectContextPanel::getSelections()
{
  return p_selections;
}


void ProjectContextPanel::clearSelections()
{
  p_projectView->clearSelectedRows();
}


void ProjectContextPanel::selectResource(const EcceURL &url)
{
  if (getSelections().size() == 1 && url == getSelections()[0])
    return;

  int rows = p_projectView->getRowCount();
  int cols = p_projectView->getColCount();
  int row;
  bool foundRow = false;

  if ( (rows > 0) && (cols > 0) ) {

    GridModel *model = p_projectView->getModel();

    // Find the row number
    for (int i = 0; i < rows; i++) {
      GridPropElement *element =
        dynamic_cast<GridPropElement*>(model->GetValueAsGridElement(i, 0));
      if (element->getResource()->getURL() == url) {
        row = i;
        foundRow = true;
        break;
      }
    }

    // Select the row, setting the grid cursor will
    // select the row
    if (foundRow) {
      p_projectView->setCursor(row, 0);
      p_projectView->getGrid()->MakeCellVisible(row, 0);
    }
  }
}


void ProjectContextPanel::loseFocus()
{
  p_projectView->setSelectionColor(ewxStyledWindow::getUnfocusedSelectionColor());
}


void ProjectContextPanel::gridCellLeftDClick(wxGridEvent& event)
{
  if (p_initialized) {
    int row = event.GetRow();
    int col = event.GetCol();
    
    GridPropElement *element = 
      dynamic_cast<GridPropElement*>(p_projectView->getElement(row, col));
    
    if (element != 0) {
      p_calcMgr->findNode(element->getResource()->getURL(), true);
    }
  }
  // Don't call Skip() - the findNode() call destroys the panel
}


void ProjectContextPanel::gridCellRightClick(wxGridEvent& event)
{
  if (p_initialized) {

    int selCol = event.GetCol();
    int selRow = event.GetRow();
    set<int> selRows = p_projectView->getSelectedRows();

    // Select row if not currently in selection set
    if (selRows.find(selRow) == selRows.end()) {
      p_projectView->setCursor(selRow, selCol);
    }

    saveSelections();

    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, wxID_PANEL_SELECTION);
    evt.SetString(EcceURL::toString(getSelections()));
    evt.SetInt(getSelections().size());
    ProcessEvent(evt);

    wxMenu menu;
    p_calcMgr->getContextMenu(menu);
    getContextMenu(menu);
    menu.AppendSeparator();
    p_projectView->getGrid()->getContextMenu(menu);
    PopupMenu(&menu);
  }
}


void ProjectContextPanel::gridLabelLeftClick(wxGridEvent& event)
{
  saveSortSettings();
}


void ProjectContextPanel::gridRangeSelect(wxGridRangeSelectEvent& event)
{
  // Save selections
  saveSelections();

  if (p_initialized) {
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, wxID_PANEL_SELECTION);
    evt.SetString(EcceURL::toString(getSelections()));
    evt.SetInt(getSelections().size());
    ProcessEvent(evt);
  }

  event.Skip();
}


void ProjectContextPanel::gridOnKeyDown(wxKeyEvent& event)
{
  bool skipEvent = true;
  int keyCode = event.GetKeyCode();
  int row = p_projectView->getCursorRow();
  int col = p_projectView->getCursorCol();
  set<int> selectedRows = p_projectView->getSelectedRows();

  if (selectedRows.find(row) != selectedRows.end()) {

    if (event.ControlDown()) {

      switch (keyCode) {
        case 'x':
        case 'X':
          p_calcMgr->copySelectionToClipboard(true);
          skipEvent = false;
          break;
        case 'c':
        case 'C':
          p_calcMgr->copySelectionToClipboard(false);
          skipEvent = false;
          break;
        case 'v':
        case 'V':
          p_calcMgr->paste();
          skipEvent = false;
          break;
      }
    }
    else {

      switch (keyCode) {
        case WXK_DELETE:
        case WXK_BACK:
          p_calcMgr->deleteSelection();
          skipEvent = false;
          break;
        case WXK_RETURN:
          if (selectedRows.size() == 1) {
            GridPropElement *element =
              dynamic_cast<GridPropElement*>(p_projectView->getElement(row, col));

            if (element != 0) {
              p_calcMgr->findNode(element->getResource()->getURL(), true);
            }
          }
          skipEvent = false;
          break;
      }
    }
  }

  // Skip to default handling if key down was
  // not handled here
  if (skipEvent) {
    event.Skip();
  }

}


void ProjectContextPanel::onExport(wxCommandEvent& event)
{
  p_projectView->getGrid()->doExport();
}


void ProjectContextPanel::getContextMenu(wxMenu& menu)
{
  // Get Registry
  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();

  // Get property view names from registry
  multimap<int, PropertyView*> propViews = 
    rd.getPropertyViews();

  // Append menu items 
  menu.AppendSeparator();
  multimap<int, PropertyView*>::iterator mapIt;
  for (mapIt = propViews.begin();
       mapIt != propViews.end();
       mapIt++) {
    PropertyView *pv = ((*mapIt).second); 
    menu.AppendRadioItem(pv->getId(), pv->getMenuItem());
    if (p_propertyView->getName() == pv->getName()) {
      menu.Check(pv->getId(), true);
    }
  }
}


void ProjectContextPanel::saveSortSettings()
{
  Preferences prefs("Organizer");
  prefs.setInt("Organizer.SortCol",
                  p_projectView->getSortCol());
  prefs.setBool("Organizer.IsAscSort",
                  p_projectView->getAscSort());
  prefs.saveFile();
}


void ProjectContextPanel::restoreSortSettings()
{
  Preferences prefs("Organizer");

  int sortCol = 0;
  if (prefs.getInt("Organizer.SortCol", sortCol)) {
    p_projectView->setSortCol(sortCol);
  }

  bool choice = true;
  if (prefs.getBool("Organizer.IsAscSort", choice)) {
    p_projectView->setAscSort(choice);
  }
}


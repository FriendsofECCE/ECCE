/**
 * @file
 *
 *
 */

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

#include <vector>
  using std::vector;

#include <algorithm>
  using std::stable_sort;

#include "util/EventDispatcher.H"
#include "util/Event.H"
  using namespace ecce;
#include "util/ErrMsg.H"
#include "util/ETimer.H"

#include "tdat/TAtm.H"
#include "dsm/AtomView.H"

#include "wxgui/ewxGrid.H"
#include "wxgui/WindowEvent.H"
#include "wxgui/GridModel.H"
#include "wxgui/WxGridView.H"
#include "wxgui/GridElement.H"
#include "wxgui/GridAtomElement.H"
#include "viz/SGFragment.H"

#include "viz/SetPartialChargeCmd.H"
#include "viz/SetSuffixCmd.H"
#include "viz/SetAtomNameCmd.H"
#include "viz/SetAtomTypeCmd.H"
#include "viz/SetPolarizabilityCmd.H"
#include "viz/SetChargeGroupCmd.H"
#include "viz/SetPolarizationGroupCmd.H"
#include "viz/SetEnvIndexCmd.H"
#include "viz/SetConnectionCmd.H"
#include "viz/SetCoordinatesCmd.H"
#include "viz/SetMassCmd.H"
#include "viz/SetBehaviorCmd.H"
#include "viz/AtomLabelsCmd.H"
#include "wxviz/WxVizToolFW.H"
#include "wxviz/MoleculeDataTable.H"


IMPLEMENT_DYNAMIC_CLASS(MoleculeDataTable, ewxPanel);


/**
 * Constructor.
 *
 * WARNING: Messaging will not work correctly for this constructor
 *  if multipel subclasses are created because they can't be uniquely named.
 *  Default constructure is required for the IMPLEMENT_DYNAMIC_CLASS
 */
MoleculeDataTable::MoleculeDataTable()
  : ewxPanel(),
    WxVizTool(),
    Listener()
{
  p_table = 0;
  p_visibleRows = 0;
  p_atomView = 0;
  p_viewId = -1;
}


/**
 * Constructor.
 * @param name - unique Listener id
 */
MoleculeDataTable::MoleculeDataTable(const string& name, 
                                     wxWindow* parent,
                                     wxWindowID id,
                                     const wxPoint& pos,
                                     const wxSize& size,
                                     long style)
  : ewxPanel(),
    WxVizTool(),
    Listener(name)
{
  Create(name, parent, id, pos, size, style);
}


bool MoleculeDataTable::Create(const string& name, 
                               wxWindow* parent,
                               wxWindowID id,
                               const wxPoint& pos,
                               const wxSize& size,
                               long style)
{
  p_viewId = -1;
  SetName(name);
  subscribe(this);

  // Initialize data members

  p_visibleRows = 0;
  p_atomView = ResourceDescriptor::getResourceDescriptor().getAtomView("xyz");

  connectEventHandlers();

  if (!ewxPanel::Create(parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("MoleculeDataTable creation failed") );
    return false;
  }

  if (GetSizer() == NULL) {
    SetSizer(new wxBoxSizer(wxVERTICAL));
  }

  p_table = new WxGridView(this, wxID_ANY, BUILDER);
  p_table->getGrid()->EnableGridLines();
  // Not sure this does anything but...
  p_table->getGrid()->ClearGrid();

  GetSizer()->Add(p_table, 1, wxGROW|wxALL, 0);

  // Get properties to view on grid
  multimap<int, AtomProperty*> properties = 
          p_atomView->getDisplayedProperties();
  
  // Get data model
  GridModel *gridModel = new GridModel();

  // Create grid columns
  gridModel->InsertCols(0, properties.size());

  // Create empty view (column labels only)
  int column = 0;
  multimap<int, AtomProperty*>::iterator mapIt = properties.begin();
  while (mapIt != properties.end()) {
    AtomProperty *property = mapIt->second;
    gridModel->SetColLabelValue(column, property->getLabel());
    mapIt++;
    column++;
  }

  // Go ahead and assign data model to grid
  // WxGridView takes ownership of pointer
  p_table->setModel(gridModel);
  return true;
}


/**
 * Destructor. 
 */
MoleculeDataTable::~MoleculeDataTable()
{
  unsubscribe();
}


/**
 * Clears and refills table.
 */
void MoleculeDataTable::refresh()
{
   fillTable();
}


void MoleculeDataTable::setVisibleRows(int rows, bool applySetting)
{
  /*
  // save visible rows seting
  p_visibleRows = rows;

  if (applySetting) {
    int height = 0;
    int width = 0;
    int labelHeight = p_table->getColLabelHeight();
    int rowHeight = p_table->getGrid()->GetGridWindow()->GetCharHeight();

    // Height
    if (rows > 0) {
      height = labelHeight + (rowHeight * rows); 
    }
    else {
      // No rows viewable - show labels at a minimum
      height = labelHeight;
    }

    // Width
    for (int i = 0; i < p_table->getColCount(); i++) {
      width += p_table->getColWidth(i);
    }
    width += p_table->getScrollbarWidth();

    // Set the window size
    wxSize winSize = GetSize();
    winSize.SetHeight(height);
    // Use existing width instead of trying to override it
    // winSize.SetWidth(width);
    SetSize(winSize);
    SetMaxSize(winSize);
    SetMinSize(winSize);
  }
  */
}


void MoleculeDataTable::eventMCB(const Event& event)
{
}


bool MoleculeDataTable::sortRows(const GridAtomElement *e1, const GridAtomElement *e2)
{
  bool ret = false;

  if (const_cast<GridAtomElement*>(e1)->getIndex() < const_cast<GridAtomElement*>(e2)->getIndex()) {
    ret = true;
  }

  return ret;
}


/**
 * Set the selection in the table to reflect the selection in
 * the Fragment.
 *
 * This method does nothing and should be overridden by subclases.
 */
void MoleculeDataTable::setSelections()
{
}


void MoleculeDataTable::connectEventHandlers ()
{
  // Cell value change
  this->Connect (wxEVT_GRID_CELL_CHANGE,
		 wxGridEventHandler (MoleculeDataTable::gridCellChange));

  // Row Selections
  this->Connect(wxEVT_GRID_RANGE_SELECT,
                wxGridRangeSelectEventHandler(MoleculeDataTable::gridRangeSelect));
}


void MoleculeDataTable::gridCellChange(wxGridEvent& event)
{
  bool isValidChange = false;

  GridAtomElement *element = 
    dynamic_cast<GridAtomElement*>(p_table->getElement(event.GetRow(), event.GetCol()));
  if (element != 0) {
    isValidChange = element->isValidValue();
    if (isValidChange) {
      updateFragment(element);
    }
  }
  if (!isValidChange) {
    wxBell();
    event.Veto();
  }
}


void MoleculeDataTable::gridRangeSelect(wxGridRangeSelectEvent& event)
{
   // Not sure if this is useful but just return if this is a label select.
   // Apparently the only way to tell is that the full set of rows is
   // specified
   if (event.GetTopRow() == 0 && 
         event.GetBottomRow() == p_table->getRowCount()-1) {
      return;
   }

   // I'm not sure why this isn't just dug out of the event except that
   // maybe selecting a label would provide values of everything selected??
   vector<GridAtomElement*> *selections = getCurrentSelections();

   if (selections != 0) {
      if ((selections->size() > 0) && (isVisible())) {
         notifySelections(selections, true);
      }

      delete selections;
      selections = 0;

   }
}



bool MoleculeDataTable::isVisible()
{
  //return IsShownOnScreen();
  // GDB 4/1/10
  // Use IsShown instead of IsShownOnScreen because the latter doesn't seem
  // to be accurate.  It isn't filling the Atom/Residue tables when they are
  // docked in the Builder, although it works when they are floated.
  return IsShown();
}


vector<GridAtomElement*> *MoleculeDataTable::getCurrentSelections()
{
  vector<GridAtomElement*> *selections = new vector<GridAtomElement*>;


  set<int> selRows = p_table->getSelectedRows();
  set<int>::iterator setIt;

  // Get atom indices
  for (setIt = selRows.begin();
       setIt != selRows.end();
       setIt++) {

    int row = *setIt;
    int col = 0;

    GridAtomElement *element =
      dynamic_cast<GridAtomElement*>(p_table->getElement(row, col));

    if (element != 0) {
      selections->push_back(element);
    }
  }

  if (selections->size() == 0) {
    delete selections;
    selections = 0;
  }

  return selections;
}

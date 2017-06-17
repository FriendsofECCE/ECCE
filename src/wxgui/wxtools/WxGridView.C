/**
 * @file
 *
 *
 */
#include <algorithm>
  using std::stable_sort;
#include <iostream>
  using std::cout;
  using std::endl;
#include <utility>
  using std::pair;
  using std::make_pair;

#include <wx/dataobj.h>
#include <wx/dnd.h>
#include <wx/renderer.h>

#include "dsm/ResourceProperty.H"

#include "util/EcceURL.H"
#include "util/Color.H"
#include "util/ETimer.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxGrid.H"
#include "wxgui/ewxStyledWindow.H"
#include "wxgui/GridElement.H"
#include "wxgui/GridPropElement.H"
#include "wxgui/GridModel.H"
#include "wxgui/GridDropTarget.H"
#include "wxgui/WxGridView.H"


IMPLEMENT_CLASS(WxGridView, wxWindow)

BEGIN_EVENT_TABLE(WxGridView, wxWindow)
    EVT_SIZE(WxGridView::windowResizeCB)
    EVT_GRID_SELECT_CELL(WxGridView::gridSelectCell)
    EVT_GRID_RANGE_SELECT(WxGridView::gridRangeSelect)
    EVT_KEY_DOWN(WxGridView::gridOnKeyDown)
    //NOTE for some reason KEY_UP wouldn't work for me
END_EVENT_TABLE()


/**
 * Constructor
 */
WxGridView::WxGridView(wxWindow* parent,
                       wxWindowID id,
                       string appName,
                       const wxPoint& pos,
                       const wxSize& size,
                       long style,
                       const wxString& name) : 
                       wxWindow(parent, id, pos, size, style, name)
{
    // Turn on via Connect so it can be turned off by other classes
    enableLabelClickProcessing(true);

    // Set Application name
    p_appName = appName;

    // Create new grid model
    p_dataModel = new GridModel();

    // Create new grid and assign model
    p_grid = new ewxGrid(this, wxID_ANY);
    setModel(p_dataModel);

    // Single selection or multiple selection mode
    p_multiSelect = true;

    // Initialize grid properties
    setSelectionColor(ewxStyledWindow::getFocusedSelectionColor());
    p_grid->SetRowLabelSize(0);
    p_grid->EnableGridLines(false);
    p_grid->DisableDragRowSize();
    p_sortedCol = -1;
    p_caseSensitive = false;
    p_ascSort = true;
    p_grid->GetGridWindow()->SetDropTarget(new GridDropTarget(this));

    // Add grid to window 
    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(p_grid, 1, wxGROW, 0);
    this->SetSizer(sizer);

    // Add event handler for drag and drop
    GridEvtHandler *gridHandler = new GridEvtHandler(this);
    p_grid->GetGridWindow()->PushEventHandler(gridHandler);

    // Add event handler col header window paint event
    GridColWindowEvtHandler *colWindowHandler = new GridColWindowEvtHandler(this);
    p_grid->GetGridColLabelWindow()->PushEventHandler(colWindowHandler);
}

/**
 *  Destructor
 */
WxGridView::~WxGridView()
{
  // Need to either detach and delete grid first or set the table to empty.
  // since the grid destructor still has reference to p_dataModel. Need to do
  // this before p_dataModel get deleted.
  //  RemoveChild(p_grid);
  //  p_grid->Destroy();
  p_grid->SetTable(0);
  if (p_dataModel != 0) {
    delete p_dataModel;
  }

}



/**
 * Use Connect/Disconnect instead of the macro so that other classes
 * can turn off (and thus override) the label processing.
 * This turns out to be necessary because the sorting causes lots of events
 * to trigger and there is no way for other event handlers to know what really
 * caused the event (ie residue table).  Thus a select from a sort looks
 * just like a select from a user which is interpretted as a toggle event.
 */
void WxGridView::enableLabelClickProcessing(bool enable)
{
   if (enable) {
      Connect (wxEVT_GRID_LABEL_LEFT_CLICK,
           wxGridEventHandler (WxGridView::gridLabelLeftClick));
   } else {
      Disconnect (wxEVT_GRID_LABEL_LEFT_CLICK,
           wxGridEventHandler (WxGridView::gridLabelLeftClick));
   }
}


GridModel* WxGridView::getModel()
{
  return p_dataModel;
}



void WxGridView::setModel(GridModel *model)
{
  p_grid->BeginBatch();

  GridModel *oldModel = p_dataModel;

  if (model != 0) {
    p_dataModel = model;
    p_grid->ClearSelection();
    p_selections.clear();

    p_grid->SetTable(model, false, wxGrid::wxGridSelectRows);

    // Set row, col, and cell attributes
    int cols = p_dataModel->GetNumberCols();
    int rows = p_dataModel->GetNumberRows();

    if ((cols > 0) && (rows > 0)) {
      setAttributes();
    }
    if ((oldModel != 0) && 
        (oldModel != p_dataModel)) {
      delete oldModel;
    }
  }

  p_grid->EndBatch();
}


void WxGridView::setAttributes()
{
  p_grid->BeginBatch();

  int cols = p_dataModel->GetNumberCols();
  int rows = p_dataModel->GetNumberRows();
  bool hasDefaultColSizes = false;

  if ((cols > 0) && (rows > 0)) {
    for (int i = 0; i < rows; i++) {
      if ((i % 2) == 0) {
        wxGridCellAttr *rowAttr = new wxGridCellAttr();
        rowAttr->SetBackgroundColour(p_grid->getHightLightRowColor());
        p_grid->SetRowAttr(i, rowAttr);
      }
      else {
        wxGridCellAttr *rowAttr = new wxGridCellAttr();
        p_grid->SetRowAttr(i, rowAttr);
      }
      if (i == 0) {
        int minWidth = p_grid->GetGridWindow()->GetCharWidth()*2;
        p_grid->SetColMinimalAcceptableWidth(minWidth);
        for (int j = 0; j < cols; j++) {
          GridElement *colElement = p_dataModel->GetValueAsGridElement(i, j);
          if (colElement != 0) {
            p_grid->SetColAttr(j, colElement->getAttributes());
            hasDefaultColSizes = colElement->hasDefaultWidth();
            if (hasDefaultColSizes) {
// KLS wx2.8 Jun 07
// The SetColSize method shows up as a 4 byte memory overwrite that
// causes all sorts of problems.  Maybe try again with  a newer
// version of wx
// Actually the autosize also messes up memory.  Ugg what to do
//
#define useautosize
#ifdef useautosize
            p_grid->AutoSizeColumn(j,true);
#endif
//#define manualsize
#ifdef usemanualsize
              int colWidth = p_grid->GetGridWindow()->GetCharWidth() * (colElement->getDefaultWidth());
              if (colWidth >= minWidth) {
                p_grid->SetColSize(j, colWidth);
              }
              else {
                p_grid->SetColSize(j,minWidth);
              }
#endif
            }
          }
        }
      }
    }
  }
  if (!hasDefaultColSizes) {
    p_grid->AutoSizeColumns(true);
  }

  p_grid->EndBatch();
}


set<int> WxGridView::getSelectedRows()
{
  return p_selections;
}


void WxGridView::clearSelectedRows()
{
  p_grid->ClearSelection();
}


void WxGridView::selectRow(int row, bool addToSelected) {
  p_grid->SelectRow(row, addToSelected);
}


void WxGridView::setSelectionColor(const wxColour& color)
{
  p_grid->SetSelectionBackground(color);
  p_grid->ForceRefresh();
}


int WxGridView::getRowCount()
{
  return p_grid->GetNumberRows();
}


int WxGridView::getColCount()
{
  return p_grid->GetNumberCols();
}


bool WxGridView::isRowSelected(int row) 
{
  bool ret = false;
  set<int> selRows = getSelectedRows();
  if (selRows.find(row) != selRows.end()) {
    ret = true;
  }

  return ret;
}


int WxGridView::getRowHeight(int row)
{
  int ret = 0;

  if (getRowCount() > 0) {
    if ((row < 0) || (row >= getRowCount())) {
      row = 0;
    }
    ret = p_grid->GetRowSize(row);
  }

  return ret;
}


int WxGridView::getColLabelHeight()
{
  return p_grid->GetColLabelSize();
}


int WxGridView::getColWidth(int col)
{
  int ret = 0;

  if (getColCount() > 0) {
    if ((col < 0) || (col >= getColCount())) {
      col = 0;
    }
    ret = p_grid->GetColSize(col);
  }

  return ret;
}


int WxGridView::getScrollbarWidth()
{
  return p_grid->GetScrollLineX();
}


int WxGridView::getScrollbarHeight()
{
  return p_grid->GetScrollLineY();
}


GridElement *WxGridView::getElement(int row, int col)
{
  return p_dataModel->GetValueAsGridElement(row, col);
}


int WxGridView::getCursorRow()
{
  return p_grid->GetGridCursorRow();
}


int WxGridView::getCursorCol()
{
  return p_grid->GetGridCursorCol();
}


void WxGridView::setCursor(int row, int col)
{
  p_grid->SetGridCursor(row, col);
}


void WxGridView::setColLabel(int col, const wxString& label)
{
  int numCols = getColCount();

  if (col >= 0 && col <= numCols) {
    p_grid->SetColLabelValue(col, label);
  }
}

void WxGridView::setColAlignment(int col, int horiz, int vert)
{
  int numCols = getColCount();
  int numRows = getRowCount();

  if ((col >= 0 && col <= numCols) && (numRows >= 0)) {
    wxGridCellAttr *attr = p_dataModel->GetAttr(0, col, wxGridCellAttr::Col);
    attr->SetAlignment(horiz, vert);
  }
}


void WxGridView::defaultSort()
{
  sortByCol(1, true);
  sortByCol(0, true);
}


void WxGridView::sortByCol(int col, bool ascSort, bool caseSensitive)
{
  p_grid->BeginBatch();

  // ETimer timer_main;
  // ETimer timer;
  // timer_main.start();

  int numRows = getRowCount();
  int numCols = getColCount();
  if ( (numRows > 1) && (numCols > 0) &&
       (col >= 0) && (col <= numCols)) {

    p_sortedCol = col;
    p_caseSensitive = caseSensitive;

    // Save current cursor location and selections, if any
    bool rowsSelected = (!p_selections.empty());
    GridElement *cursorElement = 0;
    set<GridElement*> selections;
    int cursorCol = 0;
    int cursorRow = 0;
    if (rowsSelected) {
      cursorRow = p_grid->GetGridCursorRow();
      cursorCol = p_grid->GetGridCursorCol();
      cursorElement = 
        p_dataModel->GetValueAsGridElement(cursorRow, cursorCol);
      set<int>::iterator setIt = p_selections.begin();
      while (setIt != p_selections.end()) {
        selections.insert(p_dataModel->GetValueAsGridElement(*setIt, cursorCol));
        setIt++;
      }
    }

    p_grid->ClearSelection();
    p_selections.clear();

    // timer.start();
    // Let model do the sort
    p_dataModel->sortGrid(col, ascSort, caseSensitive);
    // timer.stop();
    // cout << "Sort time :: " << timer.elapsedTime() << endl;

    // Only restore selections if there were any
    cursorRow = 0;
    set<int> selRows;
    if (rowsSelected) {
      for (int i = 0; i < numRows; i++) {
        GridElement *element =
          p_dataModel->GetValueAsGridElement(i, cursorCol);
        if (element != 0) {
          // Check for cursor row
          if (element == cursorElement) {
            cursorRow = i;
          }
          // Check if row selected
          if (selections.find(element) != selections.end()) {
            selRows.insert(i);
          }
        }
      }
      // Restore selections
      p_grid->SetGridCursor(cursorRow, cursorCol);
      p_grid->MakeCellVisible(cursorRow, cursorCol);
      set<int>::iterator setIt;
      for (setIt = selRows.begin();
           setIt != selRows.end();
           setIt++) {
        selectRow(*setIt, true);
      }
    }
    else {
      p_grid->ClearSelection();
      p_selections.clear();
    }
  }
  p_grid->EndBatch();

  // timer_main.stop();
  // cout << "Total sort time :: " << timer_main.elapsedTime() << endl;
}


int WxGridView::getSortCol()
{
  return p_sortedCol;
}


void WxGridView::setSortCol(int col)
{
  if ((col >= 0) && (col < getColCount())) {
    p_sortedCol = col;
  }
}


bool WxGridView::getAscSort()
{
  return p_ascSort;
}


void WxGridView::setAscSort(bool ascSort) 
{
  p_ascSort = ascSort;
}


string WxGridView::getAppName()
{
  return p_appName;
}


void WxGridView::setAppName(const string& appName) 
{
  p_appName = appName;
}


void WxGridView::enableGridLines(bool enable)
{
  p_grid->EnableGridLines(enable);
}


void WxGridView::enableCellEditor(int col, bool enable) 
{

  // Make sure its a valid column
  if ((col >= 0) && (col <= getColCount())) {

    p_grid->SetGridCursor(p_grid->GetGridCursorRow(), col);

    if (enable) {
      p_grid->ClearSelection();
      p_grid->EnableCellEditControl();
    }
    else {
      p_grid->HideCellEditControl();
    }

  }
}


int WxGridView::yToRow(int y)
{
  int ret = wxNOT_FOUND;

  int yunit = 0;
  int xunit = 0;
  p_grid->GetScrollPixelsPerUnit(&xunit, &yunit);
  int yoff = p_grid->GetScrollPos(wxVERTICAL) * yunit;
  int ypos = 0;

  y = y + yoff;
  for (int i = 0; i < p_grid->GetNumberRows(); i++) {
    int nexty = ypos + p_grid->GetRowSize(i);
    if ((y >= ypos) && (y <= nexty)) {
      ret = i;
      break;
    }
    ypos = nexty;
  }

  return ret;
}


ewxGrid * WxGridView::getGrid()
{
  return p_grid;
}


void WxGridView::enableMultiSelect(bool multiSelect)
{
  p_multiSelect = multiSelect;
}


void WxGridView::windowResizeCB(wxSizeEvent& event)
{
  this->Layout();
  this->Fit();
}


void WxGridView::gridSelectCell(wxGridEvent& event)
{
  // Set the current selection, then skip letting the
  // EVT_GRID_RANGE_SELECT to handle selection set
  // maintenance
  selectRow(event.GetRow());

  /*
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, wxID_PANEL_SELECTION);
    evt.SetString(p_task->getURL().toString());
    ProcessEvent(evt);
  */

  event.Skip(); 
}


void WxGridView::gridLabelLeftClick(wxGridEvent& event)
{
  wxBusyCursor wait;
  int col = event.GetCol();
  p_ascSort = !p_ascSort;
  sortByCol(col, p_ascSort);

  // Let the parent window handle label click,
  // to save preferences for column sorting
  wxWindow *parent = GetParent();
  wxGridEvent gridEvent(event.GetId(), event.GetEventType(), parent);
  gridEvent.SetEventObject(parent);
  parent->GetEventHandler()->AddPendingEvent(gridEvent);

  // Don't call Skip()
  // Cause selections to be cleared
  // Only seemed to happen in atom table (kls 6/6/07)
  //event.Skip();
}


void WxGridView::gridRangeSelect(wxGridRangeSelectEvent& event)
{
  int topRow = event.GetTopRow();
  int bottomRow = event.GetBottomRow();
  int numRows = p_dataModel->GetNumberRows();

  if ( (topRow >= 0) && (bottomRow <= (numRows - 1)) ) {
    if (event.Selecting()) {
      if (!p_multiSelect && (topRow != bottomRow || event.ControlDown())) {
        selectRow(getCursorRow());
      }
      else {
        p_grid->SetSelectionBackground(ewxStyledWindow::getFocusedSelectionColor());
        for (int i = topRow; i <= bottomRow; i++) {
          p_selections.insert(i);
        }
      }
    }
    else {
      for (int i = topRow; i <= bottomRow; i++) {
        p_selections.erase(i);
      }
    }
  }
  // cout << "p_selections.size()" << p_selections.size() << endl;
  event.Skip();
}

void WxGridView::gridOnKeyDown(wxKeyEvent& event)
{
  // For some reason the ESCAPE key is special and doesn't propagate even
  // though all other keys also seem to have ShouldPropagate returning
  // false.  We hack this here so that the ESCAPE key will move up the chain.
  // 100 is some arbitrary depth level that is probably way overdone.
  if (event.GetKeyCode() == WXK_ESCAPE)
     event.ResumePropagation(100);
  // Let the parent window handle key events if it wants to.
  // This allows the parent to override the default key
  // processing of the grid
  wxWindow *parent = GetParent();
  wxKeyEvent keyEvt(event);
  keyEvt.SetEventObject(parent);
  if (!parent->GetEventHandler()->ProcessEvent(keyEvt)) {
    event.Skip();
  }
}


GridEvtHandler::GridEvtHandler(WxGridView* gridView)
{
  Connect(wxEVT_LEFT_DOWN,
          wxMouseEventHandler(GridEvtHandler::OnMouseLeftDown));
  Connect(wxEVT_MOTION,
          wxMouseEventHandler(GridEvtHandler::OnMouseMotion));
  Connect(wxEVT_LEFT_UP,
          wxMouseEventHandler(GridEvtHandler::OnMouseLeftUp));
  p_gridView = gridView;
  p_potentialDrag = false;
}


GridEvtHandler::~GridEvtHandler()
{
  Disconnect(wxEVT_LEFT_DOWN,
             wxMouseEventHandler(GridEvtHandler::OnMouseLeftDown));
  Disconnect(wxEVT_MOTION,
             wxMouseEventHandler(GridEvtHandler::OnMouseMotion));
  Disconnect(wxEVT_LEFT_UP,
             wxMouseEventHandler(GridEvtHandler::OnMouseLeftUp));
  p_gridView = 0;
}


void GridEvtHandler::OnMouseLeftDown(wxMouseEvent &event)
{
  int row = p_gridView->yToRow(event.GetY());
  bool rowSelected = false;
  set<int> selRows = p_gridView->getSelectedRows();
  set<int>::iterator setIt = selRows.begin();

/*
cout << "Row :: " << row << endl;
cout << "Selected Rows " << endl;
while (setIt != selRows.end()) {
  cout << "\t" << *setIt << endl;
  setIt++;
}
setIt = selRows.begin();
*/

  while (setIt != selRows.end()) {
    if (row == (*setIt)) {
      rowSelected = true;
      break;
    }
    setIt++;
  }

  if (rowSelected && !event.ControlDown()) {
    p_potentialDrag = true;
  }
  else {
    event.Skip();
  }
}


void GridEvtHandler::OnMouseMotion(wxMouseEvent &event)
{
  if (event.Dragging() && (p_potentialDrag) ) {

    wxString urls = "";
    set<int> selRows = p_gridView->getSelectedRows();
    set<int>::iterator setIt = selRows.begin();
    while (setIt != selRows.end()) {
      GridModel *model = p_gridView->getModel();
      if (model != 0) {
        GridPropElement *element = 
          dynamic_cast<GridPropElement*>(model->GetValueAsGridElement((*setIt), 0));
        if (element != 0) {
          Resource *resource = element->getResource();
          if (resource != 0) {
            urls += wxT(resource->getURL().toString()+"\n");
          }
        }
      }
      setIt++;
    }
    if (urls.EndsWith(wxT("\n")))
      urls.RemoveLast();
    wxTextDataObject mydata(urls);
    wxDropSource dragSource(mydata, p_gridView->getGrid());
    wxDragResult result = dragSource.DoDragDrop(wxDrag_DefaultMove);

    switch (result) {
      case wxDragCopy:
      case wxDragMove:
      {
        // Refresh grid/treectrl
        wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, wxID_REFRESH);
        p_gridView->GetEventHandler()->AddPendingEvent(event);
      }
        break;
      case wxDragError:
      case wxDragNone:
      case wxDragCancel:
      default:
        break;
    }
    p_potentialDrag = false;
    event.Skip();
  }
  else {
    event.Skip();
  }
}


void GridEvtHandler::OnMouseLeftUp(wxMouseEvent &event)
{
  int row = p_gridView->yToRow(event.GetY());
  int col = p_gridView->getGrid()->XToCol(event.GetX());
  if (p_potentialDrag) {
    p_gridView->selectRow(row);
    p_gridView->setCursor(row,col);
  }
  p_potentialDrag = false;
  event.Skip();
}


GridColWindowEvtHandler::GridColWindowEvtHandler(WxGridView* gridView)
{
  Connect(wxEVT_PAINT,
          wxPaintEventHandler(GridColWindowEvtHandler::OnColumnHeaderPaint));
  p_gridView = gridView;
}


GridColWindowEvtHandler::~GridColWindowEvtHandler()
{
  Disconnect(wxEVT_PAINT,
             wxPaintEventHandler(GridColWindowEvtHandler::OnColumnHeaderPaint));
  p_gridView = 0;
}


void GridColWindowEvtHandler::OnColumnHeaderPaint(wxPaintEvent &event)
{
  wxGrid *grid = p_gridView->getGrid();
  wxWindow *colWindow = grid->GetGridColLabelWindow();
  wxPaintDC dc(colWindow);

  // Determine start of columns, may have scrolled
  int totColSize = 0;
  int xStart, y, xUnit;
  grid->GetViewStart(&xStart, &y);
  grid->GetScrollPixelsPerUnit(&xUnit, &y);
  totColSize =  -(xStart * xUnit);
  int colWindowWidth, colWindowHeight;
  colWindow->GetSize(&colWindowWidth, &colWindowHeight);
  dc.SetClippingRegion(0,0,colWindowWidth, colWindowHeight);

  // For each column, draw it's rectangle, it's column name,
  // and it's sort indicator, if any
  for (int col = 0; col < grid->GetNumberCols(); col++) {

    // Determine col width and height
    int colSize = grid->GetColSize(col);
    int colHeight = grid->GetColLabelSize();

    // Return if height or width is 0
    if (colSize <= 0 || colHeight <= 0 )
        return;

    // Draw a blank header button
    wxRect rect;
#ifdef __WXGTK20__
    rect.SetX( totColSize + 1 );
    rect.SetY( 1 );
    rect.SetWidth( colSize - 2 );
    rect.SetHeight( colHeight - 2 );
    wxWindowDC *win_dc = (wxWindowDC*) &dc;
    wxRendererNative::Get().DrawHeaderButton( win_dc->m_owner, dc, rect, 0 );
#else
    int colRight = totColSize + colSize - 1;

    dc.SetPen( wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DDKSHADOW),1, wxSOLID) );
    dc.DrawLine( colRight, 0,
                 colRight, colHeight-1 );

    dc.DrawLine( totColSize, 0, colRight, 0 );

    dc.DrawLine( totColSize, colHeight-1,
                 colRight+1, colHeight-1 );

    dc.SetPen( *wxWHITE_PEN );
    dc.DrawLine( totColSize, 1, totColSize, colHeight-1 );
    dc.DrawLine( totColSize, 1, colRight, 1 );
#endif

    // Draw sort indicator, if necessary
    wxBitmap bmp;
    bool isSortCol = false;
    if (col == p_gridView->getSortCol()) {
      isSortCol = true;
      if (p_gridView->getAscSort()) {
        bmp.LoadFile(ewxBitmap::pixmapFile("down16rmask.xpm"), wxBITMAP_TYPE_XPM);
      }
      else {
        bmp.LoadFile(ewxBitmap::pixmapFile("up16rmask.xpm"), wxBITMAP_TYPE_XPM);
      }
      //int left = totColSize + 3;
      //int top =  3;
      wxMask *mask = new wxMask(bmp, wxColour("RED"));
      bmp.SetMask(mask);
      //dc.DrawBitmap(bmp, left, top, true);
    }

    // Draw column label
    dc.SetBackgroundMode( wxTRANSPARENT );
    dc.SetTextForeground(grid->GetLabelTextColour() );
    dc.SetFont(grid->GetLabelFont() );
    int hAlign, vAlign;
    grid->GetColLabelAlignment( &hAlign, &vAlign );
    rect.SetX(totColSize + 4);
    rect.SetY(2);
    rect.SetWidth(colSize - 4);
    rect.SetHeight(colHeight - 4);
    if (isSortCol) {
      dc.DrawLabel(grid->GetColLabelValue(col), bmp,  rect, wxALIGN_CENTER | wxALIGN_TOP);
    }
    else {
      dc.DrawLabel(grid->GetColLabelValue(col), rect, wxALIGN_CENTER | wxALIGN_TOP);
    }

    // increment for next column
    totColSize += colSize;
  }
}

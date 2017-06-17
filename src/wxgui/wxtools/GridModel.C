/**
 * @file
 *
 *
 */

#include <stdlib.h>
#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;

#include <map>
  using std::map;

#include <set>
  using std::set;

#include <utility>
  using std::pair;
  using std::make_pair;

#include <algorithm>
  using std::stable_sort;

#include "util/StringConverter.H"

#include "wxgui/GridElement.H"

#include "wxgui/GridModel.H"



// Initialize static memers
int GridModel::p_sortCol = 0;
bool GridModel::p_ascSort = true;
bool GridModel::p_caseSensitive = false;


/**
 *  Constructor.
 */
GridModel::GridModel() : wxGridTableBase()
{
  SetAttrProvider(new wxGridCellAttrProvider);
  p_rows = 0;
  p_cols = 0;
}


/**
 *  Destructor for GridModel
 */
GridModel::~GridModel()
{
}


/**
 *  Returns the number of elements within the model.  This 
 *  is row x column.
 */
int GridModel::size()
{
  return p_rows * p_cols;
}


/**
 * Returns the number of rows in the grid.
 */
int GridModel::GetNumberRows()
{
  return p_rows;
}


/**
 * Returns the number of columns in the grid.
 */
int GridModel::GetNumberCols() 
{
  return p_cols;
}


/**
 * Indicates if there is not a GridElement 
 * object stored in the cell.  If the row and column
 * values reference a non-existent cell, then a true value
 * will be returned.
 */
bool GridModel::IsEmptyCell(int row, int col) 
{
  bool ret = true;

  if (isValidCell(row, col)) {
    if (p_elements[row][col] != 0) {
      ret = false;
    }
  }
  return ret;
}


/**
 * Returns the string value of a GridElement. If the row and
 * column values reference a non-existent cell, then an
 * empty string will be returned.
 */
wxString GridModel::GetValue(int row, int col) 
{
  wxString ret = "";

  if (isValidCell(row, col)) {
    if (p_elements[row][col] != 0) {
      ret = p_elements[row][col]->getValue();
    }
  }
  return ret;
}


void GridModel::SetValue(int row, int col, const wxString& value)
{
  if (isValidCell(row, col)) {
    if (p_elements[row][col] != 0) {
      p_elements[row][col]->setValue(value);
    }
  }
}


GridElement* GridModel::GetValueAsGridElement(int row, int col)
{
  GridElement *ret = 0;
  if (isValidCell(row, col)) {
    ret = p_elements[row][col];
  }
  return ret;
}


void GridModel::SetValueAsGridElement(int row, int col, GridElement* element)
{
  if (isValidCell(row, col)) {
    if (p_elements[row][col] != 0) {
      delete p_elements[row][col];
      p_elements[row][col] = 0;
    }
    p_elements[row][col] = element;
  }
}


void GridModel::Clear()
{
  if (p_rows > 0) {
    for (int i = 0; i < p_rows; i++) {
      for (int j = 0; j < p_cols; j++) {
        if (p_elements[i][j] != 0) {
          delete p_elements[i][j];
          p_elements[i][j] = 0;
        }
        p_elements[i].clear();
      }
    }
    p_elements.clear();
  }
  p_rows = 0;
  p_cols = 0;
}


bool GridModel::InsertRows(size_t pos, size_t numRows)
{
  bool ret = true;

  if (pos >= 0) {
    if (pos >= p_rows) {
      ret = AppendRows(numRows);
    }
    else { 
      vector<GridElement*> newRow;
      for (int i = 0; i < p_cols; i++) {
        newRow.push_back(0);
      }
      p_elements.insert(p_elements.begin()+pos, numRows, newRow);

      // Update p_rows
      p_rows += numRows;

      // Update View
      if (GetView()) {
        wxGridTableMessage msg(this,
                               wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
                               pos,
                               numRows);
        GetView()->ProcessTableMessage(msg);
      }
    }
  }

  return ret;
}


bool GridModel::AppendRows(size_t numRows)
{
  bool ret = true;
  
  vector<GridElement*> newRow;
  for (int i = 0; i < p_cols; i++) {
    newRow.push_back(0);
  }
  p_elements.resize(p_rows + numRows, newRow);

  // Update p_rows
  p_rows += numRows;

  // Update View 
  if (GetView()) {
    wxGridTableMessage msg(this,
                           wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
                           numRows);
    GetView()->ProcessTableMessage(msg);
  }

  return ret;
}


bool GridModel::DeleteRows(size_t pos, size_t numRows)
{
  bool ret = false;

  if (isValidRow(pos)) { 

    if (numRows > 0) {

      // Don't try to delete more rows
      // then there are
      if (numRows > p_rows - pos) {
        numRows = p_rows - pos;
      }

      // Remove GridElements
      for (int i = pos; i < pos + numRows; i++) {
        for (int j = 0; j < p_cols; j++) {
          if (p_elements[i][j] != 0) {
            delete p_elements[i][j];
            p_elements[i][j] = 0;
          }
        }
      }

      // Erase Rows
      int endPos = pos + numRows;
      p_elements.erase(p_elements.begin()+pos, p_elements.begin()+endPos);

      // Update p_rows
      p_rows -= numRows;

      // Update View 
      if (GetView()) {
        wxGridTableMessage msg(this,
                               wxGRIDTABLE_NOTIFY_ROWS_DELETED,
                               pos,
                               numRows);
        GetView()->ProcessTableMessage(msg);
      }
    }
  }

  return ret;
}


bool GridModel::InsertCols(size_t pos, size_t numCols)
{
  bool ret = true;

  if (pos >= 0) {

    if (p_rows == 0) { 
      p_cols += numCols;
    }
    else if (pos >= p_cols) {
      ret = AppendCols(numCols);
    }
    else {

      // Insert columns
      GridElement *colVal = 0;
      for (int i = 0; i < p_rows; i++) {
        p_elements[i].insert(p_elements[i].begin()+pos, numCols, colVal);
      }

      // Update p_cols
      p_cols += numCols;

      // Update View
      if (GetView()) {
        wxGridTableMessage msg(this,
                               wxGRIDTABLE_NOTIFY_COLS_INSERTED,
                               pos,
                               numCols);
        GetView()->ProcessTableMessage(msg);
      }
    }
  }

  return ret;
}


bool GridModel::AppendCols(size_t numCols)
{
  bool ret = true;

  if (p_rows == 0) { 
    p_cols += numCols;
  }
  else {

    // Append columns
    GridElement *colVal = 0;
    for (int i = 0; i < p_rows; i++) {
      p_elements[i].resize(p_cols + numCols, colVal);
    }

    // Update p_cols
    p_cols += numCols;

    // Update View
    if (GetView()) {
      wxGridTableMessage msg(this,
                             wxGRIDTABLE_NOTIFY_COLS_APPENDED,
                             numCols);
      GetView()->ProcessTableMessage(msg);
    }
  }

  return ret;
}


bool GridModel::DeleteCols(size_t pos, size_t numCols)
{
  bool ret = false;

  if (isValidCol(pos)) { 

    if (numCols > 0) {

      // Don't try to delete more cols
      // then there are
      if (numCols > p_cols - pos) {
        numCols = p_cols - pos;
      }

      if (p_rows == 0) {
        
        // Just update p_cols
        p_cols -= numCols;

      }
      else {

        // Remove GridElements and erase columns
        for (int i = 0; i < p_rows; i++) {
          for (int j = pos; j < pos + numCols; j++) {
            if (p_elements[i][j] != 0) {
              delete p_elements[i][j];
              p_elements[i][j] = 0;
            }
          }
          int endPos = pos + numCols;
          p_elements[i].erase(p_elements[i].begin()+pos, p_elements[i].begin()+endPos);
        }

        // Update p_cols
        p_cols -= numCols;

        // Update view 
        if (GetView()) {
          wxGridTableMessage msg(this,
                                 wxGRIDTABLE_NOTIFY_COLS_DELETED,
                                 pos,
                                 numCols);
          GetView()->ProcessTableMessage(msg);
        }
      }
    }
  }

  return ret;
}


wxString GridModel::GetRowLabelValue(int row)
{ 
  string ret = "";

  map<int, string>::iterator mapIt;
  mapIt = p_rowLabels.find(row);
  if (mapIt != p_rowLabels.end()) {
    ret = mapIt->second;
  }

  return ret;
}


wxString GridModel::GetColLabelValue(int col)
{
  string ret = "";

  map<int, string>::iterator mapIt;
  mapIt = p_colLabels.find(col);
  if (mapIt != p_colLabels.end()) {
    ret = mapIt->second;
  }

  return ret;

}


void GridModel::SetRowLabelValue(int row, const wxString& label)
{
  p_rowLabels.insert(make_pair(row, label));
}


void GridModel::SetColLabelValue(int col, const wxString& label)
{
  p_colLabels.insert(make_pair(col,label));
}


void GridModel::refresh()
{
  if (GetView()) {
    wxGridTableMessage msg(this,
                           wxGRIDTABLE_NOTIFY_ROWS_DELETED,
                           0,
                           GetNumberRows());
    GetView()->ProcessTableMessage(msg);
  }
  if (GetView()) {
    wxGridTableMessage msg(this,
                           wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
                           0,
                           GetNumberRows());
    GetView()->ProcessTableMessage(msg);
  }
}


bool GridModel::isValidCell(int row, int col)
{
  bool ret = false;
  if ((row >= 0) && (row < p_elements.size())) {
    if ((col >= 0) && (col < p_elements[row].size())) {
      ret = true;
    }
  }
  return ret;
}


bool GridModel::isValidRow(int row)
{
  return ((row >= 0) && (row < p_rows));
}


bool GridModel::isValidCol(int col)
{
  return ((col >= 0) && (col < p_cols));
}


void GridModel::sortGrid(int col, bool ascSort, bool caseSensitive) 
{
  // Save sort settings
  p_sortCol = col;
  p_ascSort = ascSort;
  p_caseSensitive = caseSensitive;

  // Needs to be something to sort
  if ((p_cols >= 0) && (p_rows >= 0)) {

    // If sort col is valid, then go ahead with sort
    if (isValidCol(col)) {
        stable_sort(p_elements.begin(), p_elements.end(), sortRows);
    }
  }
}


bool GridModel::sortRows(const vector<GridElement*> r1, const vector<GridElement*> r2)
{
  bool ret = false;
  int sortCol = p_sortCol;

  if ((sortCol < r1.size()) && (sortCol < r2.size())) {
    GridElement *e1 = r1[sortCol];
    GridElement *e2 = r2[sortCol];
    if (p_ascSort) {
      ret = (e1->compare(e2, p_caseSensitive) < 0);
    }
    else {
      ret = (e1->compare(e2, p_caseSensitive) > 0);
    }
  }
  
  return ret;
}


/**
 * Finds first row that matches. Returns -1 if val was not found in the col
 */
int GridModel::findRowByColVal(int col, wxString val)
{
  int ret = -1;
  if (isValidCol(col)) {
    for (int i = 0; i < p_rows; i++) {
      GridElement *element = p_elements[i][col]; 
      if (element != 0) {
        if (element->getValue().Cmp(val) == 0) {
          ret = i;
          break;
        }
      }
    }
  }
  return ret;
}



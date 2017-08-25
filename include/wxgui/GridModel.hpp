/**
 * @file
 *
 *
 */

#ifndef GRIDMODEL_HH
#define GRIDMODEL_HH

#ifndef WX_PRECOMP
#include "wx/wx.h"
#include "wx/generic/grid.h"
#endif

#include <utility>
    using std::pair;

#include <string>
    using std::string;

#include <vector>
    using std::vector;


#include "wxgui/GridElement.H"



class GridModel : public wxGridTableBase
{
    public:

      GridModel();

      virtual ~GridModel();

      virtual int size();
      virtual int GetNumberRows();
      virtual int GetNumberCols();

      virtual bool IsEmptyCell(int row, int col);

      virtual wxString GetValue(int row, int col);
      virtual void SetValue(int row, int col, const wxString& value);

      virtual GridElement* GetValueAsGridElement(int row, int col);
      virtual void  SetValueAsGridElement(int row, int col, GridElement* element);

      virtual void Clear();
      virtual bool InsertRows(size_t pos = 0, size_t numRows = 1);
      virtual bool AppendRows(size_t numRows = 1);
      virtual bool DeleteRows(size_t pos = 0, size_t numRows = 1);
      virtual bool InsertCols(size_t pos = 0, size_t numCols = 1);
      virtual bool AppendCols(size_t numCols = 1);
      virtual bool DeleteCols(size_t pos = 0, size_t numCols = 1);

      virtual wxString GetRowLabelValue(int row);
      virtual wxString GetColLabelValue(int col);
      virtual void SetRowLabelValue(int row, const wxString& label);
      virtual void SetColLabelValue(int col, const wxString& label);

      virtual void refresh();

      virtual void sortGrid(int col, bool ascSort, bool caseSensitive);

      virtual int findRowByColVal(int col, wxString val);

    protected:

      bool isValidCell(int row,int col);
      bool isValidRow(int row);
      bool isValidCol(int col);

      static bool sortRows(const vector<GridElement*> r1, const vector<GridElement*> r2);

      vector<vector<GridElement*> >
        p_elements;      /**< Grid elements, each cell may contain a GridElement
                              pointer. */

      int p_rows;        /** Number of rows in grid */

      int p_cols;        /** Number of cols in grid */

      map<int, string>
        p_rowLabels;     /**< Row labels, vector index cooresponds to the row number. */

      map<int,string>
        p_colLabels;     /**< Column labels, vector index cooresponds to the column number. */

      static int p_sortCol;     /** Requested sort col */

      static bool p_ascSort;    /** If true, indicates an ascending sort is requested
                             Descending sort otherwise */
     
      static bool p_caseSensitive;  /** If true, indicates requested sort to be 
                                 case sensitive */
};

#endif

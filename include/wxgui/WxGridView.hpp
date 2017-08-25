/**
 * @file
 *
 *
 */

#ifndef WXGRIDVIEW_HH
#define WXGRIDVIEW_HH


#ifndef WX_PRECOMP
#include "wx/wx.h"
#include "wx/generic/grid.h"
#endif


#include <iostream>
  using std::cout;
  using std::endl;

#include <utility>
  using std::pair;

#include <set>
  using std::set;


class GridModel;
class GridElement;
class ewxGrid;


class WxGridView: public wxWindow
{
  DECLARE_CLASS(WxGridView)
  DECLARE_EVENT_TABLE()

  public:

    WxGridView(wxWindow* parent,
               wxWindowID id,
               string appName = "",
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0,
               const wxString& name = wxPanelNameStr);

    virtual ~WxGridView();


    GridModel *getModel();
    void setModel(GridModel *model);

    set<int> getSelectedRows();
    void clearSelectedRows();
    void selectRow(int row, bool addToSelected = false);
    void setSelectionColor(const wxColour& color);
    int getRowCount();
    int getColCount();
    bool isRowSelected(int row);

    int getRowHeight(int row=0); 
    int getColLabelHeight();
    int getColWidth(int col=0);
    int getScrollbarWidth();
    int getScrollbarHeight();

    GridElement *getElement(int row, int col);

    int getCursorRow();
    int getCursorCol();
    void setCursor(int row, int col);

    void setColLabel(int col, const wxString& label);
    void setColAlignment(int col, int horiz, int vert);
    void defaultSort();
    void sortByCol(int col, bool ascSort,  bool caseSensitive = false);
    int getSortCol();
    void setSortCol(int col);
    bool getAscSort();
    void setAscSort(bool ascSort);
    string getAppName();
    void setAppName(const string& appName);

    void enableGridLines(bool enable = true);
    void enableCellEditor(int col, bool enable = true);

    int yToRow(int y);

    ewxGrid * getGrid();
  
    void enableMultiSelect(bool multiSelect);
    void enableLabelClickProcessing(bool);


  protected:

    void setAttributes();
    void windowResizeCB(wxSizeEvent& event);
    void gridSelectCell(wxGridEvent& event);
    void gridLabelLeftClick(wxGridEvent& event);
    void gridRangeSelect(wxGridRangeSelectEvent& event);
    void gridOnKeyDown(wxKeyEvent& event);

    GridModel *p_dataModel;
    ewxGrid *p_grid;
    string p_appName;
    set<int> p_selections;
    int p_sortedCol;        /**< The current sort column */
    bool p_caseSensitive;
    bool p_ascSort;         /**< Indicates the sort column is ascending if true,
                                 otherwise it is a descending sort */
    bool p_multiSelect;
};



class GridEvtHandler : public wxEvtHandler
{
  public:

    GridEvtHandler(WxGridView* gridView);
    ~GridEvtHandler();

  protected:

    virtual void OnMouseLeftDown(wxMouseEvent &event);
    virtual void OnMouseMotion(wxMouseEvent &event);
    virtual void OnMouseLeftUp(wxMouseEvent &event);

    WxGridView* p_gridView;
    bool p_potentialDrag;
};


class GridColWindowEvtHandler : public wxEvtHandler
{
  public:

    GridColWindowEvtHandler(WxGridView* gridView);
    ~GridColWindowEvtHandler();

  protected:

    virtual void OnColumnHeaderPaint(wxPaintEvent &event);

    WxGridView* p_gridView;
};

#endif




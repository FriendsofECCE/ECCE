/**
 * @file
 *
 *
 */

#ifndef GRIDCELLPROPRENDERER_HH
#define GRIDCELLPROPRENDERER_HH

#ifndef WX_PRECOMP
#include "wx/wx.h"
#include "wx/generic/grid.h"
#endif

class GridPropElement;



/**
 * Renders a GridPropElement image in the grid's cell.
 *
 * @todo this class should probably inherit from wxGridCellRenderer and 
 *       implement its own getBestSize() and Clone() methods 
 *       instead of using the string renderer's.  Seems to work ok
 *       for now.
 */
class GridCellPropImageRenderer : public wxGridCellStringRenderer
{
  public:

    GridCellPropImageRenderer();

    virtual ~GridCellPropImageRenderer();


    virtual void Draw(wxGrid& grid,
                      wxGridCellAttr& attr,
                      wxDC& dc,
                      const wxRect& rectCell,
                      int row, int col,
                      bool isSelected);

  protected:

    void drawIcon(GridPropElement *element,
                  wxGrid& grid,
                  wxGridCellAttr& attr,
                  wxDC& dc,
                  const wxRect& rectCell,
                  int row, int col,
                  bool isSelected);

    void drawResourceIcon(GridPropElement *element,
                          int align,
                          wxGrid& grid,
                          wxGridCellAttr& attr,
                          wxDC& dc,
                          const wxRect& rectCell,
                          int row, int col,
                          bool isSelected);

    void drawRunstateIcon(GridPropElement *element,
                           wxGrid& grid,
                          wxGridCellAttr& attr,
                          wxDC& dc,
                          const wxRect& rectCell,
                          int row, int col,
                          bool isSelected);

    void drawIcon(const wxIcon& icon, wxDC& dc, const wxRect& rectCell,
                  int align = wxALIGN_CENTER);
};

#endif


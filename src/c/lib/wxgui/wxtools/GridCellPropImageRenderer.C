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
  using std::cout;
  using std::cerr;
  using std::endl;


#include "util/StringConverter.H"
#include "util/ResourceUtils.H"

#include "dsm/ResourceProperty.H"
#include "dsm/GridProperty.H"

#include "wxgui/ewxColor.H"
#include "wxgui/GridModel.H"
#include "wxgui/GridPropElement.H"
#include "wxgui/WxResourceImageList.H"
#include "wxgui/WxState.H"
#include "wxgui/WxStateImageList.H"

#include "wxgui/GridCellPropImageRenderer.H"


class wxGridWindow;


GridCellPropImageRenderer::GridCellPropImageRenderer()
{}


GridCellPropImageRenderer::~GridCellPropImageRenderer()
{}


void GridCellPropImageRenderer::Draw(wxGrid& grid,
                                 wxGridCellAttr& attr,
                                 wxDC& dc,
                                 const wxRect& rectCell,
                                 int row, int col,
                                 bool isSelected)
{
  wxGridCellRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);
  
  GridModel *model = 
    dynamic_cast<GridModel*>(grid.GetTable());   

  if (model != 0) {

    GridPropElement *element = 
      dynamic_cast<GridPropElement*>(model->GetValueAsGridElement(row, col));   

    if (element != 0) {

      if (!element->getValue().empty()) {

        switch (element->getPropertyType()) {

          case GridProperty::PT_ICON:
            drawIcon(element, grid, attr, dc, rectCell, row, col, isSelected);
            break;
          case GridProperty::PT_RESOURCE_ICON:
            drawResourceIcon(element, wxALIGN_CENTER, grid, attr, dc,
                             rectCell, row, col, isSelected);
            break;
          case GridProperty::PT_RUNSTATE_ICON:
            drawRunstateIcon(element, grid, attr, dc, rectCell,
                             row, col, isSelected);
            break;
          default:
            break;
        }
      }

    }
  }
}


void GridCellPropImageRenderer::drawIcon(GridPropElement *element,
                                         wxGrid& grid,
                                         wxGridCellAttr& attr,
                                         wxDC& dc,
                                         const wxRect& rectCell,
                                         int row, int col,
                                         bool isSelected) 
{
  // Not Implemented
}


void GridCellPropImageRenderer::drawResourceIcon(GridPropElement *element,
                                                 int align,
                                                 wxGrid& grid,
                                                 wxGridCellAttr& attr,
                                                 wxDC& dc,
                                                 const wxRect& rectCell,
                                                 int row, int col,
                                                 bool isSelected)
{
  int imageIdx;
  if (StringConverter::toInt(element->getValue().c_str(), imageIdx)) {
    wxIcon icon = WxResourceImageList::getImageList()->GetIcon(imageIdx);
    drawIcon(icon, dc, rectCell, align);
  }
}


void GridCellPropImageRenderer::drawRunstateIcon(GridPropElement *element,
                                                 wxGrid& grid,
                                                 wxGridCellAttr& attr,
                                                 wxDC& dc,
                                                 const wxRect& rectCell,
                                                 int row, int col,
                                                 bool isSelected) 
{
  drawIcon(WxStateImageList::getImageList()->GetIcon(
           ResourceUtils::stringToState(element->getValue().c_str())),
           dc, rectCell);
}


void GridCellPropImageRenderer::drawIcon(const wxIcon& icon, wxDC& dc,
                                         const wxRect& rectCell, int align)
{
  // Get the bitmap's size 
  int w0 = icon.GetWidth();
  int h0 = icon.GetHeight();

  int x = rectCell.x;
  int y = rectCell.y;
  int width = rectCell.width;
  int height = rectCell.height;

  // Get bitmap drawing location
  int x0 = x;
  int y0 = y;
  if (width > w0) {
    if (align & wxALIGN_LEFT)
      x0 = x;
    else if (align & wxALIGN_RIGHT)
      x0 = x+width-w0;
    else
      x0 = x+(width-w0)/2;
  }
  if (height > h0) {
    if (align & wxALIGN_TOP)
      y0 = y;
    else if (align & wxALIGN_BOTTOM)
      y0 = y+height-h0;
    else
      y0 = y+(height-h0)/2;
  }

  // Draw bitmap
  dc.DrawIcon(icon, x0, y0);
}


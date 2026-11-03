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

#include "dsm/Resource.H"
#include "dsm/VDoc.H"
#include "dsm/ResourceProperty.H"
#include "dsm/GridProperty.H"

#include "wxgui/ewxColor.H"
#include "wxgui/GridModel.H"
#include "wxgui/GridPropElement.H"
#include "wxgui/WxResourceImageList.H"
#include "wxgui/WxState.H"
#include "wxgui/WxStateImageList.H"
#include "wxgui/WxState.H"

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
            drawResourceIcon(element, wxALIGN_LEFT, grid, attr, dc,
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


wxSize GridCellPropImageRenderer::GetBestSize(wxGrid& grid, wxGridCellAttr& attr,
                                              wxDC& dc, int row, int col)
{
  GridModel *model = dynamic_cast<GridModel*>(grid.GetTable());
  GridPropElement *element = model ? dynamic_cast<GridPropElement*>(
      model->GetValueAsGridElement(row, col)) : 0;
  if (!element || element->getPropertyType() != GridProperty::PT_RESOURCE_ICON)
    return wxGridCellStringRenderer::GetBestSize(grid, attr, dc, row, col);
  Resource *res = element->getResource();
  wxString app = res && res->hasAccess() ? wxString::FromUTF8(
      res->getProp(VDoc::getEcceNamespace() + ":application").c_str()) : wxString();
  dc.SetFont(attr.GetFont());
  const wxSize t = dc.GetTextExtent(app);
  return wxSize(16 + grid.FromDIP(14) + t.GetWidth(), wxMax(t.GetHeight(), 16));
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
  if (StringConverter::toInt(element->getValue().ToStdString(), imageIdx)) {
    wxIcon icon = WxResourceImageList::getImageList()->GetIcon(imageIdx);
    drawIcon(icon, dc, rectCell, align);
    // Type and application are one column: the icon, then the application's
    // name.
    Resource *res = element->getResource();
    if (res && res->hasAccess()) {
      wxString app = wxString::FromUTF8(
          res->getProp(VDoc::getEcceNamespace() + ":application").c_str());
      const int x = rectCell.x + icon.GetWidth() + grid.FromDIP(6);
      dc.SetFont(attr.GetFont());
      dc.SetTextForeground(isSelected ? grid.GetSelectionForeground()
                                      : attr.GetTextColour());
      dc.DrawText(app, x, rectCell.y +
                  (rectCell.height - dc.GetTextExtent(app).GetHeight()) / 2);
    }
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
  // "running?": the Organizer's mark for a running job out of contact.
  const string value = element->getValue().ToStdString();
  drawIcon(WxStateImageList::getImageList()->GetIcon(
           value == WxState::getName(
               (ResourceDescriptor::RUNSTATE)WxState::OUT_OF_CONTACT)
           ? (int)WxState::OUT_OF_CONTACT
           : (int)ResourceUtils::stringToState(value)),
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


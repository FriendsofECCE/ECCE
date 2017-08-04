#include "wx/dc.h"
#include "wx/brush.h"
#include "wx/image.h"
#include "wx/odcombo.h"

#include "wxgui/ewxStyledWindow.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/ImageDropDown.H"

ImageDropDown::ImageDropDown(wxWindow* parent,
                             wxWindowID id,
                             const wxString& value,
                             const wxPoint& pos,
                             const wxSize& size,
                             const wxArrayString& choices,
                             const wxArrayString& images,
                             const wxSize& imageSize,
                             long style,
                             const wxValidator& validator,
                             const wxString& name)
  : wxOwnerDrawnComboBox(parent, id, value, pos, size, choices, style, validator, name),
    p_images(images),
    p_imageSize(imageSize)
{
}


void ImageDropDown::OnDrawItem( wxDC& dc,
                                const wxRect& rect,
                                int item,
                                int flags ) const
{
  if ( item == wxNOT_FOUND )
    return;
    
  wxRect r(rect);
  r.Deflate(3);
  r.height -= 2;

  if (!(flags & wxODCB_PAINTING_CONTROL)) {
    dc.DrawBitmap(ewxBitmap(p_images[item]), r.x+1, r.y+1, true);
    dc.DrawText(GetString( item ),
                r.x+10+p_imageSize.GetX(),
                (r.y + 2) + (r.height - dc.GetCharHeight())/2);

  }
  else
  {
    dc.DrawBitmap(ewxBitmap(p_images[item]), r.x+1, r.y+1, true);
  }
}


void ImageDropDown::OnDrawBackground(wxDC& dc, const wxRect& rect,
                                     int item, int flags) const
{
  //wxOwnerDrawnComboBox::OnDrawBackground(dc,rect,item,flags);

  // If item is selected or even, or we are painting the
  // combo control itself, use the default rendering.

  if (flags & (wxODCB_PAINTING_SELECTED))
  {
    wxOwnerDrawnComboBox::OnDrawBackground(dc,rect,item,flags);
    return;
  }

  // Otherwise, draw every other background with different colour.
  dc.SetBrush(wxBrush(ewxStyledWindow::getInputColor()));
  dc.SetPen(wxPen(ewxStyledWindow::getInputColor()));
  dc.DrawRectangle(rect);
}



wxCoord ImageDropDown::OnMeasureItem( size_t item ) const
{
  // Simply demonstrate the ability to have variable-height items
  /*
  if ( item & 1 )
    return 36;
  else
  */
  wxCoord height = wxOwnerDrawnComboBox::OnMeasureItem(item);
  if (height < p_imageSize.GetY())
    height = p_imageSize.GetY();
  return height + 6;
}


wxCoord ImageDropDown::OnMeasureItemWidth(size_t item) const
{
  wxClientDC dc(const_cast<ImageDropDown *>(this));
  wxCoord x, y;
  dc.GetTextExtent(GetString(item), &x, &y, 0, 0);
  return p_imageSize.GetX()+x+10;
  // default - will be measured from text width
}

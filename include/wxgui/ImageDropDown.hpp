#include "wx/odcombo.h"


class ImageDropDown : public wxOwnerDrawnComboBox
{
public:
  ImageDropDown(wxWindow* parent,
                wxWindowID id,
                const wxString& value,
                const wxPoint& pos,
                const wxSize& size,
                const wxArrayString& choices,
                const wxArrayString& images,
                const wxSize& imageSize,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = "comboBox");

  virtual void OnDrawItem( wxDC& dc,
                           const wxRect& rect,
                           int item,
                           int flags ) const;

  virtual void OnDrawBackground( wxDC& dc, const wxRect& rect,
                                 int item, int flags ) const;

  virtual wxCoord OnMeasureItem( size_t item ) const;

  virtual wxCoord OnMeasureItemWidth( size_t WXUNUSED(item) ) const;
protected:
  
  wxArrayString p_images;
  wxSize p_imageSize;
};

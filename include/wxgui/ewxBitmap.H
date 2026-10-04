#ifndef _EWXBITMAP_H
#define _EWXBITMAP_H

#include "wx/bitmap.h"
#include "wx/bmpbndl.h"
#include "wxgui/ewxStyledWindow.H"

/**
 * An extenstion of the wxBitmap class that knows where to find
 * ecce bitmaps.
 */
class ewxBitmap : public wxBitmap
{
   public:
      ewxBitmap();  
      ewxBitmap(const wxString& name, long type=wxBITMAP_TYPE_XPM);
      ewxBitmap(const ewxBitmap& bitmap);

      static wxString pixmapFile(const wxString& name);
      static wxBitmapBundle bundle(const wxString& name,
                                   long type=wxBITMAP_TYPE_XPM);
      static wxString genericIconName(const wxString& file);
      static wxBitmap themedIcon(const wxString& icon, const wxSize& size);
};

#endif

#ifndef _EWXBITMAP_H
#define _EWXBITMAP_H

#include "wx/bitmap.h"
#include "wx/bmpbndl.h"
#include "wx/icon.h"
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
      static wxBitmapBundle svgIcon(const wxString& name, const wxSize& size,
                                    const wxString& fallback = wxString());
      static wxBitmapBundle colorIcon(const wxString& svg, const wxSize& size);
      static wxString genericIconName(const wxString& file);
      static wxBitmap saveIcon();
      static wxBitmap themedIcon(const wxString& icon, const wxSize& size);
      static wxBitmap loadPixmap(const wxString& name, long type);
      static wxBitmap placeholder(const wxString& name);
      static wxIcon icon(const wxString& name);

      // Pixels per logical pixel for bitmaps the toolkit does not scale
      // itself: >1 only on wxMSW (wxGTK and wxOSX scale a bitmap on draw).
      // ECCE_BITMAP_SCALE overrides it, to exercise the path anywhere.
      static double dpiScale();
      static int scaled(int px);
      static wxSize scaled(const wxSize& size);
      static wxBitmap scaled(const wxBitmap& bitmap);
};

#endif

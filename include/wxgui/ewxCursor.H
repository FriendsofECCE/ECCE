#ifndef _EWXCURSOR_H
#define _EWXCURSOR_H

#include "wx/cursor.h"

/**
 * An extenstion of the wxBitmap class that knows where to find
 * ecce bitmaps.
 */
class ewxCursor : public wxCursor
{
public:
  ewxCursor();
  ewxCursor(const wxString& name, long type=wxBITMAP_TYPE_XPM,
            int hotSpotX = 0, int hotSpotY = 0);
  ewxCursor(const ewxCursor& cursor);

  static wxString pixmapFile(const wxString& name);
};

#endif

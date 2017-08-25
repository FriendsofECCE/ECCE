#include <iostream>
using std::cerr;
using std::endl;

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/Ecce.H"
#include "wxgui/ewxCursor.H"


/**
 * Helper method to construct proper string for use in calling
 * superclasses constructor.
 */
wxString ewxCursor::pixmapFile(const wxString& name) 
{
  wxString ret = Ecce::ecceDataPath();
  ret.append("/client/cursors/");
  ret.append(name);
  return ret;
}


/**
 * Constructor.
 * Takes name of bitmap and prepends the path where the file is located
 * so wxBitmap can find it.
 *
 * @todo API changed since 2.6.2, need to update this once upgraded to 2.8
 */
ewxCursor::ewxCursor(const wxString& name, long type,
                     int hotSpotX, int hotSpotY):
  wxCursor(wxImage(pixmapFile(name)))
{
}


ewxCursor::ewxCursor()
{
}


/**
 * Copy constructor needed to create bitmaps in menus using dialogblocks.
 */
ewxCursor::ewxCursor(const ewxCursor& cursor ) : wxCursor(cursor)
{
}


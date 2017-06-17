#include <iostream>
using namespace std;

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/Ecce.H"
#include "wxgui/ewxBitmap.H"


/**
 * Helper method to construct proper string for use in calling
 * superclasses constructor.
 */
wxString ewxBitmap::pixmapFile(const wxString& name) 
{
  wxString ret = Ecce::ecceDataPath();
  ret.append("/client/pixmaps/");
  ret.append(name);
  return ret;
}


/**
 * Constructor.
 * Takes name of bitmap and prepends the path where the file is located
 * so wxBitmap can find it.
 */
ewxBitmap::ewxBitmap(const wxString& name, long type)
{
  LoadFile( pixmapFile(name), (wxBitmapType)type);
}

ewxBitmap::ewxBitmap( )
{
}

/**
 * Copy constructor needed to create bitmaps in menus using dialogblocks.
 */
ewxBitmap::ewxBitmap(const ewxBitmap& bitmap ) : wxBitmap(bitmap)
{
}

#include <iostream>
using namespace std;

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include "wx/artprov.h"
#include "wx/filename.h"

#include "util/Ecce.H"
#include "wxgui/ewxBitmap.H"


// ECCE's own pixmaps for generic actions, and the freedesktop icon that
// replaces each (#210).  Chemistry icons are not listed: they stay ECCE's.
static const struct { const char* file; const char* icon; } GENERIC_ICONS[] = {
  {"gohome",          "go-home"},
  {"goup",            "go-up"},
  {"back",            "go-previous"},
  {"forward",         "go-next"},
  {"editcut",         "edit-cut"},
  {"editcopy",        "edit-copy"},
  {"editpaste",       "edit-paste"},
  {"editdelete",      "edit-delete"},
  {"delete",          "edit-delete"},
  {"rename",          "document-edit"},
  {"upload",          "document-send"},
  {"download",        "folder-download"},
  {"filefind",        "edit-find"},
  {"reload",          "view-refresh"},
  {"info",            "dialog-information"},
  {"filenew",         "document-new"},
  {"fileopen",        "document-open"},
  {"filesave",        "document-save"},
  {"save",            "document-save"},
  {"filesaveas",      "document-save-as"},
  {"undo",            "edit-undo"},
  {"redo",            "edit-redo"},
  {"msg_information", "dialog-information"},
  {"msg_question",    "dialog-question"},
  {"msg_exclamation", "dialog-warning"},
  {"msg_error",       "dialog-error"},
  {"player_play",     "media-playback-start"},
  {"player_stop",     "media-playback-stop"},
  {"viewmagfit",      "zoom-fit-best"},
  {"viewmag1",        "zoom-original"},
  {"viewmag+",        "zoom-in"},
  {"viewmag-",        "zoom-out"},
  {"1uparrow",        "go-up"},
  {"1downarrow",      "go-down"},
};


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


wxString ewxBitmap::genericIconName(const wxString& file)
{
  wxString base = wxFileName(file).GetName();
  for (size_t i = 0; i < WXSIZEOF(GENERIC_ICONS); i++) {
    if (base == GENERIC_ICONS[i].file) return GENERIC_ICONS[i].icon;
  }
  return wxEmptyString;
}


/**
 * The theme's symbolic icon, drawn in the theme's text colour.
 * GTK hands symbolic icons back in a fixed dark grey, which vanishes on a
 * dark theme, so the colour is applied here; the alpha is the shape.
 */
wxBitmap ewxBitmap::themedIcon(const wxString& icon, const wxSize& size)
{
  wxBitmap art = wxArtProvider::GetBitmap(icon + "-symbolic", wxART_OTHER,
                                          size);
  if (!art.IsOk()) return art;
  wxImage image = art.ConvertToImage();
  if (!image.HasAlpha()) return art;
  wxColour ink = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT);
  unsigned char* rgb = image.GetData();
  for (int i = 0, n = image.GetWidth()*image.GetHeight(); i < n; i++) {
    rgb[3*i] = ink.Red();
    rgb[3*i+1] = ink.Green();
    rgb[3*i+2] = ink.Blue();
  }
  return wxBitmap(image);
}


/**
 * For a toolbar or button that should be sharp at any scale: the themed
 * icon at the pixmap's size and twice it, or the pixmap alone.
 */
wxBitmapBundle ewxBitmap::bundle(const wxString& name, long type)
{
  wxBitmap legacy;
  legacy.LoadFile(pixmapFile(name), (wxBitmapType)type);
  wxString icon = genericIconName(name);
  if (!icon.empty()) {
    wxSize size = legacy.IsOk() ? legacy.GetSize() : wxSize(16, 16);
    wxBitmap one = themedIcon(icon, size);
    wxBitmap two = themedIcon(icon, size*2);
    if (one.IsOk() && two.IsOk()) return wxBitmapBundle::FromBitmaps(one, two);
    if (one.IsOk()) return wxBitmapBundle(one);
  }
  return wxBitmapBundle(legacy);
}


/**
 * Constructor.
 * Takes name of bitmap and prepends the path where the file is located
 * so wxBitmap can find it.  A generic action's pixmap is replaced by the
 * theme's icon at the same size, when the theme has one.
 */
ewxBitmap::ewxBitmap(const wxString& name, long type)
{
  LoadFile( pixmapFile(name), (wxBitmapType)type);
  wxString icon = genericIconName(name);
  if (!icon.empty()) {
    wxBitmap art = themedIcon(icon, IsOk() ? GetSize() : wxSize(16, 16));
    if (art.IsOk()) wxBitmap::operator=(art);
  }
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

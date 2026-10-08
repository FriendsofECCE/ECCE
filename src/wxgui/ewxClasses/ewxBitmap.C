#include <iostream>
#include <cstdio>
using namespace std;

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif
#include "wx/artprov.h"
#include "wx/filename.h"
#include "wx/dcmemory.h"
#include "wx/log.h"
#include "wx/ffile.h"
#include "wx/bmpbndl.h"
#include <cstdlib>
#include <cstring>

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


//  The bundled Lucide icon (data/client/pixmaps/lucide) for each of those
//  freedesktop names, for platforms whose theme has none.
static const struct { const char* icon; const char* lucide; } LUCIDE_ICONS[] = {
  {"go-home", "house"},
  {"go-up", "arrow-up"},
  {"go-down", "arrow-down"},
  {"go-previous", "arrow-left"},
  {"go-next", "arrow-right"},
  {"edit-cut", "scissors"},
  {"edit-copy", "copy"},
  {"edit-paste", "clipboard-paste"},
  {"edit-delete", "trash"},
  {"document-edit", "pencil"},
  {"document-send", "upload"},
  {"folder-download", "download"},
  {"edit-find", "search"},
  {"view-refresh", "refresh-cw"},
  {"dialog-information", "info"},
  {"document-new", "file-plus"},
  {"document-open", "folder-open"},
  {"document-save", "save"},
  {"document-save-as", "file-down"},
  {"edit-undo", "undo-2"},
  {"edit-redo", "redo-2"},
  {"dialog-question", "circle-question-mark"},
  {"dialog-warning", "triangle-alert"},
  {"dialog-error", "circle-x"},
  {"media-playback-start", "play"},
  {"media-playback-stop", "square"},
  {"zoom-fit-best", "scan"},
  {"zoom-original", "maximize"},
  {"zoom-in", "zoom-in"},
  {"zoom-out", "zoom-out"},
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


#ifdef __WXMSW__
//  wxMSW toolbars and buttons ignore a bitmap's alpha channel (transparent
//  pixels show their colour, mostly black), but honour a mask.  Edge pixels
//  are blended onto the button face colour first, so the outline stays
//  smooth where a plain alpha threshold would leave it jagged.
static wxBitmap alphaToMask(const wxBitmap& bmp)
{
  if (!bmp.IsOk() || !bmp.HasAlpha()) return bmp;
  wxImage image = bmp.ConvertToImage();
  if (!image.HasAlpha()) return bmp;
  wxColour face = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
  const unsigned char mr = 1, mg = 2, mb = 3;   // the mask colour
  unsigned char* rgb = image.GetData();
  unsigned char* a = image.GetAlpha();
  for (int i = 0, n = image.GetWidth() * image.GetHeight(); i < n; i++) {
    unsigned char* p = rgb + 3 * i;
    if (a[i] < 24) {
      p[0] = mr; p[1] = mg; p[2] = mb;
    } else if (a[i] < 255) {
      int w = a[i];
      p[0] = (p[0] * w + face.Red() * (255 - w)) / 255;
      p[1] = (p[1] * w + face.Green() * (255 - w)) / 255;
      p[2] = (p[2] * w + face.Blue() * (255 - w)) / 255;
    }
  }
  image.ClearAlpha();
  image.SetMaskColour(mr, mg, mb);
  return wxBitmap(image);
}
#else
static const wxBitmap& alphaToMask(const wxBitmap& bmp) { return bmp; }
#endif


/**
 * The bundled Lucide icon drawn in the text colour (its stroke is
 * currentColor), for platforms with no icon theme.
 */
static wxBitmap lucideIcon(const wxString& icon, const wxSize& size,
                           const wxColour& ink)
{
  for (size_t i = 0; i < WXSIZEOF(LUCIDE_ICONS); i++) {
    if (icon != LUCIDE_ICONS[i].icon) continue;
    wxFileName file(ewxBitmap::pixmapFile(
        wxString("lucide/") + LUCIDE_ICONS[i].lucide + ".svg"));
    wxFFile in(file.GetFullPath());
    wxString svg;
    if (!in.IsOpened() || !in.ReadAll(&svg)) return wxBitmap();
    svg.Replace("currentColor", ink.GetAsString(wxC2S_HTML_SYNTAX));
    wxScopedCharBuffer utf8 = svg.utf8_str();
    wxCharBuffer data(utf8.length());
    memcpy(data.data(), utf8.data(), utf8.length());
    wxBitmapBundle b = wxBitmapBundle::FromSVG(data.data(), size);
    return b.IsOk() ? alphaToMask(b.GetBitmap(size)) : wxBitmap();
  }
  return wxBitmap();
}


/**
 * The theme's symbolic icon, drawn in the theme's text colour.
 * GTK hands symbolic icons back in a fixed dark grey, which vanishes on a
 * dark theme, so the colour is applied here; the alpha is the shape.
 * Without a theme icon (always on Windows and macOS, or with
 * ECCE_FORCE_ICON_BUNDLE set, for tests) the bundled Lucide icon is used.
 */
wxBitmap ewxBitmap::themedIcon(const wxString& icon, const wxSize& size)
{
  wxColour ink = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT);
#ifdef __WXGTK__
  if (!getenv("ECCE_FORCE_ICON_BUNDLE")) {
    wxBitmap art = wxArtProvider::GetBitmap(icon + "-symbolic", wxART_OTHER,
                                            size);
    if (art.IsOk()) {
      wxImage image = art.ConvertToImage();
      if (image.HasAlpha()) {
        unsigned char* rgb = image.GetData();
        for (int i = 0, n = image.GetWidth()*image.GetHeight(); i < n; i++) {
          rgb[3*i] = ink.Red();
          rgb[3*i+1] = ink.Green();
          rgb[3*i+2] = ink.Blue();
        }
        return wxBitmap(image);
      }
      return art;
    }
  }
#endif
  return lucideIcon(icon, size, ink);
}


/**
 * The theme's own save icon (wxART_FILE_SAVE), for the editors' save
 * button; the old pixmap only if the theme has none.
 */
wxBitmap ewxBitmap::saveIcon()
{
  wxBitmap art = wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_BUTTON,
                                          wxSize(16, 16));
  return art.IsOk() ? art : wxBitmap(ewxBitmap("save.xpm"));
}


/**
 * The pixmap, loaded by the type its extension names.  Callers pass the
 * XPM default for PNG files too: wxGTK sniffs the format through
 * GdkPixbuf, wxOSX and wxMSW do not and fail.  No log dialog on failure.
 */
wxBitmap ewxBitmap::loadPixmap(const wxString& name, long type)
{
  wxString ext = wxFileName(name).GetExt().Lower();
  wxBitmapType t = (wxBitmapType)type;
  if (ext == "png") t = wxBITMAP_TYPE_PNG;
  else if (ext == "xpm") t = wxBITMAP_TYPE_XPM;
  else if (ext == "gif") t = wxBITMAP_TYPE_GIF;
  else if (ext == "jpg" || ext == "jpeg") t = wxBITMAP_TYPE_JPEG;
  else if (ext == "bmp") t = wxBITMAP_TYPE_BMP;
  wxBitmap bmp;
  wxLogNull quiet;
  bmp.LoadFile(pixmapFile(name), t);
  return alphaToMask(bmp);
}


/**
 * A visible stand-in for a pixmap that did not load, so a toolbar or
 * image list never receives an invalid bitmap (wxOSX crashes on one).
 */
wxBitmap ewxBitmap::placeholder(const wxString& name)
{
  fprintf(stderr, "ECCE: pixmap %s did not load; using a placeholder\n",
          (const char*) pixmapFile(name).utf8_str());
  wxBitmap bmp(16, 16);
  wxMemoryDC dc(bmp);
  dc.SetBackground(*wxLIGHT_GREY_BRUSH);
  dc.Clear();
  dc.SetPen(*wxBLACK_PEN);
  dc.SetBrush(*wxTRANSPARENT_BRUSH);
  dc.DrawRectangle(0, 0, 16, 16);
  dc.DrawLine(0, 0, 16, 16);
  dc.SelectObject(wxNullBitmap);
  return bmp;
}


/**
 * A window icon from a pixmap.  wxIcon(file, XPM) has no handler on wxOSX
 * and logs an error dialog; going through the bitmap works everywhere.
 */
wxIcon ewxBitmap::icon(const wxString& name)
{
  wxIcon ret;
  ret.CopyFromBitmap(ewxBitmap(name));
  return ret;
}


/**
 * For a toolbar or button that should be sharp at any scale: the themed
 * icon at the pixmap's size and twice it, or the pixmap alone.
 */
wxBitmapBundle ewxBitmap::bundle(const wxString& name, long type)
{
  wxBitmap legacy = loadPixmap(name, type);
  wxString icon = genericIconName(name);
  if (!icon.empty()) {
    wxSize size = legacy.IsOk() ? legacy.GetSize() : wxSize(16, 16);
    wxBitmap one = themedIcon(icon, size);
    wxBitmap two = themedIcon(icon, size*2);
    if (one.IsOk() && two.IsOk()) return wxBitmapBundle::FromBitmaps(one, two);
    if (one.IsOk()) return wxBitmapBundle(one);
  }
  return wxBitmapBundle(legacy.IsOk() ? legacy : placeholder(name));
}


/**
 * Constructor.
 * Takes name of bitmap and prepends the path where the file is located
 * so wxBitmap can find it.  A generic action's pixmap is replaced by the
 * theme's icon at the same size, when the theme has one.
 */
ewxBitmap::ewxBitmap(const wxString& name, long type)
{
  wxBitmap::operator=(loadPixmap(name, type));
  wxString icon = genericIconName(name);
  if (!icon.empty()) {
    wxBitmap art = themedIcon(icon, IsOk() ? GetSize() : wxSize(16, 16));
    if (art.IsOk()) wxBitmap::operator=(art);
  }
  if (!IsOk()) wxBitmap::operator=(placeholder(name));
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

/**
 * @file
 * @author Lisong Sun
 *
 * ECCE tool button class.
 *
 */

#include <cstdlib>
#include "dsm/ResourceTool.H"
#include "wxgui/ewxPanel.H"
#include "wxgui/ewxBitmap.H"
#include "wxgui/EcceTool.H"

#include "wx/dcclient.h"
#include "wx/renderer.h"
#include "wx/settings.h"

//  The flat SVG icon of each tool (data/client/pixmaps/svg), and the
//  label drawn under it; the old pixmaps carried the label in the image.
static const struct { const char* pixmap; const char* svg; } TOOL_SVG[] = {
  {"gwbuilder2.xpm", "builder"},
  {"gwbst2.xpm", "basisset"},
  {"gweditor2.xpm", "editor"},
  {"gwlauncher2.xpm", "launcher"},
  {"gwviewer2.xpm", "viewer"},
  {"gwpertab2.xpm", "periodictable"},
  {"gwmachinebrowser2.xpm", "machinebrowser"},
  {"gworganizer2.xpm", "organizer"},
  {"gwcalcmgr2.xpm", "organizer"},
};

static const int ICON_SIZE = 40;   // keeps a button about as tall as the old 68 px one
static const int BUTTON_WIDTH = 68;

static wxBitmapBundle toolBundle(const wxString& pixmap)
{
  for (size_t i = 0; i < WXSIZEOF(TOOL_SVG); i++) {
    if (pixmap == TOOL_SVG[i].pixmap) {
      wxBitmapBundle b = wxBitmapBundle::FromSVGFile(
          ewxBitmap::pixmapFile(wxString("svg/tool-") + TOOL_SVG[i].svg
                                + ".svg"), wxSize(ICON_SIZE, ICON_SIZE));
      if (b.IsOk()) return b;
    }
  }
  return wxBitmapBundle();
}

//  A label short enough for the button's width.
static wxString shortLabel(const wxString& label)
{
  if (label.StartsWith("Electronic")) return "Editor";
  if (label.StartsWith("Basis")) return "Basis Set";
  if (label.StartsWith("Machine")) return "Machines";
  // the icon already says it is an editor
  if (label.EndsWith(" Editor")) return label.Left(label.length() - 7);
  return label;
}


BEGIN_EVENT_TABLE( EcceTool, wxPanel )

  EVT_LEFT_DOWN         (EcceTool::OnMouseLeftDown)
  EVT_LEFT_UP           (EcceTool::OnMouseLeftUp)
  EVT_RIGHT_DOWN        (EcceTool::OnMouseRightDown)
  EVT_ENTER_WINDOW      (EcceTool::OnMouseEnterWindow)
  EVT_LEAVE_WINDOW      (EcceTool::OnMouseLeaveWindow)
  EVT_PAINT             (EcceTool::OnPaint)
  EVT_SET_FOCUS         (EcceTool::OnFocus)
  EVT_KILL_FOCUS        (EcceTool::OnFocus)
  EVT_KEY_DOWN          (EcceTool::OnKeyDown)
  //  EVT_MENU              (ID_ECCETOOL_NEW, EcceTool::OnMenuClick)

END_EVENT_TABLE()


EcceTool::EcceTool() {}


EcceTool::~EcceTool()
{
  delete p_invokeParam;
}


EcceTool::EcceTool(wxWindow * parent, ResourceTool * resTool)
{
  p_bitmap = ewxBitmap(resTool->getIcon(), wxBITMAP_TYPE_XPM);
  p_bundle = toolBundle(resTool->getIcon());
  if (p_bundle.IsOk()) p_label = shortLabel(resTool->getLabel());
  Create(parent, resTool->getId(), resTool->getName());
}


EcceTool::EcceTool(wxWindow * parent, ResourceTool * resTool,
                   const wxString& label)
{
  p_bitmap = ewxBitmap(resTool->getIcon(), wxBITMAP_TYPE_XPM);
  p_bundle = toolBundle(resTool->getIcon());
  if (p_bundle.IsOk()) p_label = shortLabel(label);
  Create(parent, resTool->getId(), resTool->getName());
}


EcceTool::EcceTool(wxWindow *parent, const wxString& name)
{
  ResourceTool *resTool =
          ResourceDescriptor::getResourceDescriptor().getTool(name.ToStdString());
  p_bitmap = ewxBitmap(resTool->getIcon(), wxBITMAP_TYPE_XPM);
  p_bundle = toolBundle(resTool->getIcon());
  if (p_bundle.IsOk()) p_label = shortLabel(resTool->getLabel());
  Create(parent, resTool->getId(), resTool->getName());
}


EcceTool::EcceTool(wxWindow * parent, wxWindowID id,
                   const wxString& name, const wxBitmap& bitmap)
{
  p_bitmap = bitmap;
  Create(parent, id, name);
}


void EcceTool::Create(wxWindow * parent, wxWindowID id, const wxString& name)
{
  p_isHover = false;
  // wxMSW draws in device pixels, so the layout below is scaled by hand.
  int height = ewxBitmap::scaled(68);
  int width = ewxBitmap::scaled(BUTTON_WIDTH);
  if (p_bundle.IsOk()) {
    height = ewxBitmap::scaled(ICON_SIZE + 10) + parent->GetCharHeight();
    // wide enough for the label, which the sizer then makes room for
    width = wxMax(width, parent->GetTextExtent(p_label).x
                         + ewxBitmap::scaled(12));
  }
  ewxPanel::Create(parent, id, wxDefaultPosition, wxSize(width, height),
                   wxNO_BORDER|wxTAB_TRAVERSAL, name);
  SetMinSize(wxSize(width, height));

  p_isSunken = false;
  p_invokeParam = new InvokeParam;
  p_invokeParam->forceNew = 0;
  p_invokeParam->name = name.c_str();
  
  //   p_newMenu = NULL;
  //   if (hasMenu) {
  //     p_newMenu = new wxMenu;
  //     p_newMenu->Append(ID_ECCETOOL_NEW, "&New...", "", wxITEM_NORMAL);
  //     p_newMenu->SetEventHandler(this);
  //   }
}


void EcceTool::setStatus(bool isSunken)
{
  p_isSunken = isSunken;
  Refresh();
}


bool EcceTool::isSunken()
{
  return p_isSunken;
}


void EcceTool::OnMouseLeftDown( wxMouseEvent& event )
{
  setStatus(true);
}


void EcceTool::OnMouseLeftUp( wxMouseEvent& event )
{
  if (p_isSunken) {
    toolActivate(event.ShiftDown());
  }
  setStatus(false);
}


void EcceTool::OnMouseEnterWindow( wxMouseEvent& event )
{
  p_isHover = true;
  Refresh();
}


void EcceTool::OnMouseLeaveWindow( wxMouseEvent& event )
{
  p_isHover = false;
  setStatus(false);
}


void EcceTool::OnMouseRightDown( wxMouseEvent& event )
{
  toolActivate(true);
}


// void EcceTool::OnMenuClick( wxCommandEvent& event)
// {
//   toolActivate(true);
// }


//  The thumbnail is a window cut into the raised button: a two-pixel frame in
//  the theme's own shades, dark side up and left, light side down and right,
//  the reverse of the button's bevel.  Filled one-pixel rectangles rather
//  than DrawLine, whose end point is drawn or not depending on the backend.
static void fillRect(wxDC& dc, const wxColour& c, int x, int y, int w, int h)
{
  dc.SetPen(*wxTRANSPARENT_PEN);
  dc.SetBrush(wxBrush(c));
  dc.DrawRectangle(x, y, w, h);
}

static void drawThumbnailFrame(wxDC& dc, const wxRect& img)
{
  wxColour dark = wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW);
  wxColour light = wxSystemSettings::GetColour(wxSYS_COLOUR_3DHIGHLIGHT);
  wxColour mid = wxSystemSettings::GetColour(wxSYS_COLOUR_3DDKSHADOW);
  if (mid == dark) mid = dark.ChangeLightness(80);
  //  Drawn around the image, on the button face, so a dark picture does not
  //  swallow it.
  wxRect r = img; r.Inflate(2);
  for (int i = 0; i < 2; i++) {
    const int x = r.x + i, y = r.y + i, w = r.width - 2 * i, h = r.height - 2 * i;
    const wxColour& tl = (i == 0) ? mid : dark;
    fillRect(dc, tl, x, y, w, 1);
    fillRect(dc, tl, x, y, 1, h);
    fillRect(dc, light, x + 1, y + h - 1, w - 1, 1);
    fillRect(dc, light, x + w - 1, y + 1, 1, h - 1);
  }
}



void EcceTool::OnPaint( wxPaintEvent& event )
{
  wxPaintDC dc(this);
  PrepareDC(dc);

  //  Always the theme's raised push button: a flat icon gives no sign
  //  it can be clicked until the mouse is over it.
  wxRect rect(wxPoint(0, 0), GetClientSize());
  int flags = IsEnabled() ? 0 : wxCONTROL_DISABLED;
  if (p_isSunken) flags |= wxCONTROL_PRESSED;
  else if (p_isHover) flags |= wxCONTROL_CURRENT;
  wxRendererNative::Get().DrawPushButton(this, dc, rect, flags);

  if (p_bundle.IsOk()) {
    const int icon = ewxBitmap::scaled(ICON_SIZE);
    const int top = ewxBitmap::scaled(4);
    const bool pixels = ewxBitmap::dpiScale() > 1;
    wxBitmap bmp = p_thumbnail.IsOk() ? p_thumbnail
                 : pixels ? p_bundle.GetBitmap(wxSize(icon, icon))
                          : p_bundle.GetBitmapFor(this);
    if (!IsEnabled()) bmp = bmp.ConvertToDisabled();
    wxSize sz = p_thumbnail.IsOk() || pixels ? wxSize(icon, icon)
                                   : p_bundle.GetPreferredLogicalSizeFor(this);
    dc.DrawBitmap(bmp, (rect.width - sz.x) / 2, top, true);
    if (p_thumbnail.IsOk()) drawThumbnailFrame(dc, wxRect(
        wxPoint((rect.width - sz.x) / 2, top), sz));
    dc.SetFont(GetFont());
    dc.SetTextForeground(wxSystemSettings::GetColour(
        IsEnabled() ? wxSYS_COLOUR_BTNTEXT : wxSYS_COLOUR_GRAYTEXT));
    wxSize ts = dc.GetTextExtent(p_label);
    dc.DrawText(p_label, wxMax(0, (rect.width - ts.x) / 2),
                top + icon + ewxBitmap::scaled(2));
  } else {
    dc.DrawBitmap(p_bitmap, ewxBitmap::scaled(2), ewxBitmap::scaled(2), true);
  }

  if (HasFocus())
    wxRendererNative::Get().DrawFocusRect(this, dc, rect.Deflate(2), 0);

  event.Skip();
}


void EcceTool::OnFocus( wxFocusEvent& event )
{
  Refresh();
  event.Skip();
}


void EcceTool::OnKeyDown( wxKeyEvent& event )
{
  int key = event.GetKeyCode();
  if (key == WXK_SPACE || key == WXK_RETURN || key == WXK_NUMPAD_ENTER)
    toolActivate(event.ShiftDown());
  else
    event.Skip();
}


/**
 * Invoke a tool by calling Gateway method to do the job.
 * We have changed the meaning of shifDown to be reuse a tool whereas
 * no shiftDown just means get a new tool (at least in the context
 * of the gateway).
 */
void EcceTool::toolActivate(bool shiftDown)
{
  if (!IsEnabled()) return;
  wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
  p_invokeParam->forceNew = (shiftDown?1:0);
  evt.SetClientObject(p_invokeParam);

  ProcessEvent(evt);
}


/**
 * A picture in place of the icon (the calculation's thumbnail on the
 * Builder button).  With the SVG icon the paint draws the bundle, not
 * p_bitmap, so the picture is kept scaled to the icon's size instead.
 */
void EcceTool::clearBitMap()
{
  p_thumbnail = wxBitmap();
  Refresh();
}


//  The stored 64 px thumbnail has the molecule small in the middle of its
//  background; on a scaled display that reads as a small picture in a white
//  square, so the margin is cut (square, 4 px of margin kept).
static void cropToContent(wxImage& image)
{
  const int w = image.GetWidth(), h = image.GetHeight();
  if (w < 8 || h < 8 || image.HasAlpha()) return;
  const unsigned char* d = image.GetData();
  const int br = d[0], bg = d[1], bb = d[2];   // corner = background
  int x0 = w, y0 = h, x1 = -1, y1 = -1;
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      const unsigned char* p = d + 3 * (y * w + x);
      if (abs(p[0] - br) + abs(p[1] - bg) + abs(p[2] - bb) > 40) {
        x0 = wxMin(x0, x); x1 = wxMax(x1, x);
        y0 = wxMin(y0, y); y1 = wxMax(y1, y);
      }
    }
  if (x1 < 0) return;
  const int side = wxMin(wxMax(x1 - x0, y1 - y0) + 1 + 8, wxMin(w, h));
  const int cx = (x0 + x1) / 2, cy = (y0 + y1) / 2;
  const int x = wxMax(0, wxMin(w - side, cx - side / 2));
  const int y = wxMax(0, wxMin(h - side, cy - side / 2));
  image = image.GetSubImage(wxRect(x, y, side, side));
}


void EcceTool::setBitMap(wxBitmap bitmap)
{
  p_bitmap = bitmap;
  if (p_bundle.IsOk() && bitmap.IsOk()) {
    const bool pixels = ewxBitmap::dpiScale() > 1;
    const double scale = pixels ? 1.0 : GetContentScaleFactor();
    const int px = pixels ? ewxBitmap::scaled(ICON_SIZE)
                          : (int)(ICON_SIZE * scale + 0.5);
    wxImage image = bitmap.ConvertToImage();
    if (pixels) cropToContent(image);
    image.Rescale(px, px, wxIMAGE_QUALITY_HIGH);
    p_thumbnail = wxBitmap(image, -1, scale);
  }
  Refresh();
}

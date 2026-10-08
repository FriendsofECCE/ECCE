/**
 * @file
 * @author Lisong Sun
 *
 * ECCE tool button class.
 *
 */

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
  int height = 68;
  if (p_bundle.IsOk())
    height = ICON_SIZE + 10 + parent->GetCharHeight();
  ewxPanel::Create(parent, id, wxDefaultPosition,
                   wxSize(BUTTON_WIDTH, height),
                   wxNO_BORDER|wxTAB_TRAVERSAL, name);

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


void EcceTool::OnPaint( wxPaintEvent& event )
{
  wxPaintDC dc(this);
  PrepareDC(dc);

  //  Always the theme's raised push button: a flat icon gives no sign
  //  it can be clicked until the mouse is over it.
  wxRect rect(wxPoint(0, 0), GetClientSize());
  int flags = 0;
  if (p_isSunken) flags |= wxCONTROL_PRESSED;
  else if (p_isHover) flags |= wxCONTROL_CURRENT;
  wxRendererNative::Get().DrawPushButton(this, dc, rect, flags);

  if (p_bundle.IsOk()) {
    wxBitmap bmp = p_bundle.GetBitmapFor(this);
    wxSize sz = p_bundle.GetPreferredLogicalSizeFor(this);
    dc.DrawBitmap(bmp, (rect.width - sz.x) / 2, 4, true);
    dc.SetFont(GetFont());
    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
    wxSize ts = dc.GetTextExtent(p_label);
    dc.DrawText(p_label, wxMax(0, (rect.width - ts.x) / 2),
                4 + ICON_SIZE + 2);
  } else {
    dc.DrawBitmap(p_bitmap, 2, 2, true);
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
  wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
  p_invokeParam->forceNew = (shiftDown?1:0);
  evt.SetClientObject(p_invokeParam);

  ProcessEvent(evt);
}


void EcceTool::setBitMap(wxBitmap bitmap)
{
  p_bitmap = bitmap;
  Refresh();
}

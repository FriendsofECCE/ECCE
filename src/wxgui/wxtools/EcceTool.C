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

#include <cstdlib>
#include "wx/bmpbndl.h"
#include "wx/dcclient.h"
#include "wx/filename.h"
#include "wx/settings.h"

// MOCK-UP ONLY (wip/org-icons, #210): with ECCE_MOCK_SVG_ICONS=1 a tool
// button draws data/client/pixmaps/svg/tool-<name>.svg and its label under
// it, instead of the pixmap with baked-in text.
static wxBitmapBundle mockBundle(const wxString& pixmap)
{
  static const struct { const char* pix; const char* svg; } MAP[] = {
    {"gwbuilder2.xpm","builder"},{"gwbst2.xpm","basisset"},
    {"gweditor2.xpm","editor"},{"gwlauncher2.xpm","launcher"},
    {"gwviewer2.xpm","viewer"},{"gwpertab2.xpm","periodictable"},
    {"gwmachinebrowser2.xpm","machinebrowser"},
    {"gworganizer2.xpm","organizer"},{"gwcalcmgr2.xpm","organizer"}};
  if (!getenv("ECCE_MOCK_SVG_ICONS")) return wxBitmapBundle();
  for (size_t i = 0; i < WXSIZEOF(MAP); i++)
    if (pixmap == MAP[i].pix)
      return wxBitmapBundle::FromSVGFile(
          ewxBitmap::pixmapFile(wxString("svg/tool-") + MAP[i].svg + ".svg"),
          wxSize(48, 48));
  return wxBitmapBundle();
}


BEGIN_EVENT_TABLE( EcceTool, wxPanel )

  EVT_LEFT_DOWN         (EcceTool::OnMouseLeftDown)
  EVT_LEFT_UP           (EcceTool::OnMouseLeftUp)
  EVT_RIGHT_DOWN        (EcceTool::OnMouseRightDown)
  EVT_ENTER_WINDOW      (EcceTool::OnMouseEnterWindow)
  EVT_LEAVE_WINDOW      (EcceTool::OnMouseLeaveWindow)
  EVT_PAINT             (EcceTool::OnPaint)
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
  p_mock = mockBundle(resTool->getIcon());
  p_mockLabel = resTool->getLabel();
  Create(parent, resTool->getId(), resTool->getName());
}


EcceTool::EcceTool(wxWindow *parent, const wxString& name)
{
  ResourceTool *resTool =
          ResourceDescriptor::getResourceDescriptor().getTool(name.ToStdString());
  p_bitmap = ewxBitmap(resTool->getIcon(), wxBITMAP_TYPE_XPM);
  p_mock = mockBundle(resTool->getIcon());
  p_mockLabel = resTool->getLabel();
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
  ewxPanel::Create(parent, id, wxDefaultPosition,
                   wxSize(68, p_mock.IsOk() ? 84 : 68),
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
}


void EcceTool::OnMouseLeaveWindow( wxMouseEvent& event )
{
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
  
  if (p_mock.IsOk()) {
    wxBitmap b = p_mock.GetBitmapFor(this);
    wxSize sz = p_mock.GetPreferredLogicalSizeFor(this);
    dc.DrawBitmap(b, (68 - sz.x) / 2, 4, true);
    dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
    wxString l = p_mockLabel;   // the short form fits the 68 px button
    if (l.StartsWith("Electronic")) l = "Editor";
    else if (l.StartsWith("Basis")) l = "Basis Set";
    else if (l.StartsWith("Machine")) l = "Machines";
    wxSize ts = dc.GetTextExtent(l);
    dc.DrawText(l, wxMax(0, (68 - ts.x) / 2), 58);
  } else
  dc.DrawBitmap(p_bitmap, 2, 2, false);

  drawButtonBorder(dc, p_isSunken);

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

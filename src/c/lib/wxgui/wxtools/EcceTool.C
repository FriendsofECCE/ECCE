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
  Create(parent, resTool->getId(), resTool->getName());
}


EcceTool::EcceTool(wxWindow *parent, const wxString& name)
{
  ResourceTool *resTool =
          ResourceDescriptor::getResourceDescriptor().getTool(name.c_str());
  p_bitmap = ewxBitmap(resTool->getIcon(), wxBITMAP_TYPE_XPM);
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
  ewxPanel::Create(parent, id, wxDefaultPosition, wxSize(68, 68),
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

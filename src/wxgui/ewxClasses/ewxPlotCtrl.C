#include <wx/dcmemory.h>
#include <wx/menu.h>
#include <wx/scrolbar.h>

#include "wxgui/ewxFrame.H"
#include "wxgui/ewxPlotCtrl.H"

  
static double distance(double x, double y, wxPoint2DDouble p2)
{
  return sqrt( pow(x-p2.m_x,2) + pow(y-p2.m_y,2) );
}


IMPLEMENT_CLASS(ewxPlotCtrl, wxPlotCtrl)


const wxWindowID ewxPlotCtrl::ID_PLOT_MENU_CLONE = wxNewId();
const wxWindowID ewxPlotCtrl::ID_PLOT_MENU_ZOOM_TO_FIT = wxNewId();
const wxWindowID ewxPlotCtrl::ID_PLOT_MENU_SHOW_CROSSHAIR = wxNewId();
const wxWindowID ewxPlotCtrl::ID_PLOT_MENU_DRAW_SYMBOLS = wxNewId();
const wxWindowID ewxPlotCtrl::ID_PLOT_MENU_DRAW_GRID = wxNewId();
const wxWindowID ewxPlotCtrl::ID_PLOT_MENU_SHOW_POINT_LABEL = wxNewId();
const wxWindowID ewxPlotCtrl::ID_PLOT_MENU_SHOW_AXIS_X = wxNewId();
const wxWindowID ewxPlotCtrl::ID_PLOT_MENU_SHOW_AXIS_Y = wxNewId();
const wxWindowID ewxPlotCtrl::ID_PLOT_MENU_SHOW_AXIS_LABEL_X = wxNewId();
const wxWindowID ewxPlotCtrl::ID_PLOT_MENU_SHOW_AXIS_LABEL_Y = wxNewId();
const wxWindowID ewxPlotCtrl::ID_PLOT_MENU_SHOW_TITLE = wxNewId();
const wxWindowID ewxPlotCtrl::ID_PLOT_MENU_SHOW_KEY = wxNewId();


BEGIN_EVENT_TABLE(ewxPlotCtrl, wxPlotCtrl)
  EVT_PLOTCTRL_CLICKED(wxID_ANY, ewxPlotCtrl::OnClick)
  EVT_RIGHT_DOWN(ewxPlotCtrl::OnRightClick)
  EVT_MENU(ID_PLOT_MENU_CLONE,             ewxPlotCtrl::OnMenuClone)
  EVT_MENU(ID_PLOT_MENU_ZOOM_TO_FIT,       ewxPlotCtrl::OnMenuZoomToFit)
  EVT_MENU(ID_PLOT_MENU_SHOW_CROSSHAIR,    ewxPlotCtrl::OnMenuShowCrosshair)
  EVT_MENU(ID_PLOT_MENU_DRAW_SYMBOLS,      ewxPlotCtrl::OnMenuDrawSymbols)
  EVT_MENU(ID_PLOT_MENU_DRAW_GRID,         ewxPlotCtrl::OnMenuDrawGrid)
  EVT_MENU(ID_PLOT_MENU_SHOW_POINT_LABEL,  ewxPlotCtrl::OnMenuShowPointLabel)
  EVT_MENU(ID_PLOT_MENU_SHOW_AXIS_X,       ewxPlotCtrl::OnMenuShowAxisX)
  EVT_MENU(ID_PLOT_MENU_SHOW_AXIS_Y,       ewxPlotCtrl::OnMenuShowAxisY)
  EVT_MENU(ID_PLOT_MENU_SHOW_AXIS_LABEL_X, ewxPlotCtrl::OnMenuShowAxisLabelX)
  EVT_MENU(ID_PLOT_MENU_SHOW_AXIS_LABEL_Y, ewxPlotCtrl::OnMenuShowAxisLabelY)
  EVT_MENU(ID_PLOT_MENU_SHOW_TITLE,        ewxPlotCtrl::OnMenuShowTitle)
  EVT_MENU(ID_PLOT_MENU_SHOW_KEY,          ewxPlotCtrl::OnMenuShowKey)
  EVT_UPDATE_UI(ID_PLOT_MENU_SHOW_CROSSHAIR,
                ewxPlotCtrl::UpdateUIShowCrosshair)
  EVT_UPDATE_UI(ID_PLOT_MENU_DRAW_SYMBOLS,
                ewxPlotCtrl::UpdateUIDrawSymbols)
  EVT_UPDATE_UI(ID_PLOT_MENU_DRAW_GRID,
                ewxPlotCtrl::UpdateUIDrawGrid)
  EVT_UPDATE_UI(ID_PLOT_MENU_SHOW_POINT_LABEL,
                ewxPlotCtrl::UpdateUIShowPointLabel)
  EVT_UPDATE_UI(ID_PLOT_MENU_SHOW_AXIS_X,
                ewxPlotCtrl::UpdateUIShowAxisX)
  EVT_UPDATE_UI(ID_PLOT_MENU_SHOW_AXIS_Y,
                ewxPlotCtrl::UpdateUIShowAxisY)
  EVT_UPDATE_UI(ID_PLOT_MENU_SHOW_AXIS_LABEL_X,
                ewxPlotCtrl::UpdateUIShowAxisLabelX)
  EVT_UPDATE_UI(ID_PLOT_MENU_SHOW_AXIS_LABEL_Y,
                ewxPlotCtrl::UpdateUIShowAxisLabelY)
  EVT_UPDATE_UI(ID_PLOT_MENU_SHOW_TITLE,
                ewxPlotCtrl::UpdateUIShowTitle)
  EVT_UPDATE_UI(ID_PLOT_MENU_SHOW_KEY,
                ewxPlotCtrl::UpdateUIShowKey)
END_EVENT_TABLE()


ewxPlotCtrl::ewxPlotCtrl()
  : wxPlotCtrl()
  , p_menu(NULL)
  , p_showPointLabel(true)
  , p_pointMarker()
  , p_xValueMap()
  , p_yValueMap()
{
  Init();
}


ewxPlotCtrl::ewxPlotCtrl( wxWindow *parent, wxWindowID win_id,
        const wxPoint &pos, const wxSize &size,
        wxPlotCtrlAxis_Type flags, const wxString& name)
  : wxPlotCtrl()
  , p_menu(NULL)
  , p_showPointLabel(true)
  , p_pointMarker()
  , p_xValueMap()
  , p_yValueMap()
{
  Init();
  (void)Create(parent, win_id, pos, size, flags, name);
}


ewxPlotCtrl::~ewxPlotCtrl()
{
}


bool ewxPlotCtrl::Create( wxWindow *parent, wxWindowID id, const wxPoint &pos,
                          const wxSize &size, wxPlotCtrlAxis_Type flags,
                          const wxString& name )
{
  if (!wxPlotCtrl::Create(parent, id, pos, size, flags, name)) {
    wxFAIL_MSG( wxT("ewxPlotCtrl creation failed") );
    return false;
  }

  SetMinSize(wxSize(300,200));

  // get rid of "active" bitmap
  m_activeBitmap = wxNullBitmap;
  m_inactiveBitmap = wxNullBitmap;

  m_xAxisScrollbar->Hide();
  m_yAxisScrollbar->Hide();

  // create menu
  p_menu = new wxMenu;
  p_menu->Append(ID_PLOT_MENU_CLONE,
          _T("Clone current plot"),
          _T("Creates a new frame containing a duplicate of the current plot"));
  p_menu->Append(ID_PLOT_MENU_ZOOM_TO_FIT,
          _T("Zoom to fit"),
          _T("Zoom to show entire data plot"));
  p_menu->AppendCheckItem(ID_PLOT_MENU_SHOW_CROSSHAIR,
          _T("Crosshair cursor"),
          _T("Show the cursor as a crosshair"));
  p_menu->AppendSeparator();
  p_menu->AppendCheckItem(ID_PLOT_MENU_DRAW_SYMBOLS,
          _T("Draw curve symbols"),
          _T("Draw symbols for each curve data point"));
  p_menu->AppendCheckItem(ID_PLOT_MENU_DRAW_GRID,
          _T("Draw plot grid"),
          _T("Draw the gridlines in the plot"));
  p_menu->AppendSeparator();
  p_menu->AppendCheckItem(ID_PLOT_MENU_SHOW_POINT_LABEL,
          _T("Show label for selected point"),
          _T("Show label for selected point"));
  p_menu->AppendCheckItem(ID_PLOT_MENU_SHOW_AXIS_X,
          _T("Show x-axis"),
          _T("Show the x-axis"));
  p_menu->AppendCheckItem(ID_PLOT_MENU_SHOW_AXIS_Y,
          _T("Show y-axis"),
          _T("Show the y-axis"));
  p_menu->AppendCheckItem(ID_PLOT_MENU_SHOW_AXIS_LABEL_X,
          _T("Show x-axis label"),
          _T("Show the x-axis label"));
  p_menu->AppendCheckItem(ID_PLOT_MENU_SHOW_AXIS_LABEL_Y,
          _T("Show y-axis label"),
          _T("Show the y-axis label"));
  p_menu->AppendCheckItem(ID_PLOT_MENU_SHOW_TITLE,
          _T("Show title"),
          _T("Show the title of the plot"));
  p_menu->AppendCheckItem(ID_PLOT_MENU_SHOW_KEY,
          _T("Show key"),
          _T("Show a key for the plot"));

  return true;
}


ewxPlotCtrl * ewxPlotCtrl::Clone(wxWindow * parent)
{
  ewxPlotCtrl * clone = new ewxPlotCtrl(parent);

  // copy data (curves)
  for (int i = 0; i < GetCurveCount(); ++i) {
    wxPlotData * data;
    wxPlotCurve * curve;
    if ((data = GetDataCurve(i))) {
      clone->AddCurve(*data);
    } else if ((curve = GetCurve(i))) {
      clone->AddCurve(*data);
    }
  }

  if (IsCursorValid()) {
    clone->SetCursorDataIndex(GetCursorCurveIndex(), GetCursorDataIndex());
  }

  // copy plot settings
  clone->SetCrossHairCursor(GetCrossHairCursor());
  clone->SetDrawSymbols(GetDrawSymbols());
  clone->SetDrawLines(GetDrawLines());
  clone->SetDrawSpline(GetDrawSpline());
  clone->SetDrawGrid(GetDrawGrid());
  clone->SetShowPointLabel(GetShowPointLabel());
  clone->SetShowXAxis(GetShowXAxis());
  clone->SetShowYAxis(GetShowYAxis());
  clone->SetShowXAxisLabel(GetShowXAxisLabel());
  clone->SetShowYAxisLabel(GetShowYAxisLabel());
  clone->SetXAxisLabel(GetXAxisLabel());
  clone->SetYAxisLabel(GetYAxisLabel());
  clone->SetShowPlotTitle(GetShowPlotTitle());
  clone->SetPlotTitle(GetPlotTitle());
  clone->SetShowKey(GetShowKey());

  clone->SetYValueMap(GetYValueMap());

  return clone;
}


void ewxPlotCtrl::CalcBoundingPlotRect()
{
  int i, count = m_curves.GetCount();

  if (count > 0)
  {
    bool valid_rect = false;

    wxRect2DDouble rect = m_curves[0].GetBoundingRect();

    if ( IsFinite(rect.m_x, wxT("left curve boundary is NaN")) &&
      IsFinite(rect.m_y, wxT("bottom curve boundary is NaN")) &&
      IsFinite(rect.GetRight(), wxT("right curve boundary is NaN")) &&
      IsFinite(rect.GetBottom(), wxT("top curve boundary is NaN")) &&
      (rect.m_width >= 0) && (rect.m_height >= 0) )
    {
      valid_rect = true;
    }
    else
      rect = wxNullPlotBounds;

    for (i = 1; i < count; i++)
    {
      wxRect2DDouble curveRect = m_curves[i].GetBoundingRect();

      wxRect2DDouble newRect;
      if (!valid_rect)
        newRect = curveRect;
      else {
        // NOTE: CreateUnion won't work correctly since it assumes all values
        // are positive.
        //newRect = rect.CreateUnion(curveRect);
        double xmin = wxMin(rect.m_x, curveRect.m_x);
        double ymin = wxMin(rect.m_y, curveRect.m_y);
        double xmax = wxMax(rect.m_x + rect.m_width,
                            curveRect.m_x + curveRect.m_width);
        double ymax = wxMax(rect.m_y + rect.m_height,
                            curveRect.m_y + curveRect.m_height);
        newRect = wxRect2DDouble(xmin, ymin, xmax-xmin, ymax-ymin);
      }

      if ( IsFinite(newRect.m_x, wxT("left curve boundary is NaN")) &&
        IsFinite(newRect.m_y, wxT("bottom curve boundary is NaN")) &&
        IsFinite(newRect.GetRight(), wxT("right curve boundary is NaN")) &&
        IsFinite(newRect.GetBottom(), wxT("top curve boundary is NaN")) &&
        (newRect.m_width >= 0) && (newRect.m_height >= 0))
      {
        if (!valid_rect) valid_rect = true;
        rect = newRect;
      }
    }

    // maybe just a single point, center it using default size
    bool zeroWidth = false, zeroHeight = false;

    if (rect.m_width == 0.0)
    {
      zeroWidth = true;
      rect.m_x  -= 1;
      rect.m_width = 2;
    }
    if (rect.m_height == 0.0)
    {
      zeroHeight = true;
      rect.m_y   -= 1;
      rect.m_height = 2;
    }

    m_curveBoundingRect = rect;

    // add some padding so the edge points can be seen
    double w = (!zeroWidth)  ? rect.m_width/50.0  : 0.0;
    double h = (!zeroHeight) ? rect.m_height/50.0 : 0.0;
    m_curveBoundingRect.Inset(-w, -h, -w, -h);
  }
  else
    m_curveBoundingRect = m_defaultPlotRect;

  AdjustScrollBars();
}


void ewxPlotCtrl::SetCurveBoundingRect(const wxRect2DDouble& rect)
{
  m_curveBoundingRect = rect;
  AdjustScrollBars();
}


int ewxPlotCtrl::FindMarker(const wxPlotMarker& marker)
{
  for (int i=0, count=m_plotMarkers.GetCount(); i<count; ++i) {
    if (m_plotMarkers[i] == marker) {
      return i;
    }
  }
  return wxNOT_FOUND;
}


void ewxPlotCtrl::SetShowPointLabel(bool show)
{
  p_showPointLabel = show;
  if (!p_showPointLabel) {
    // Remove the last marker for the point value.
    int index;
    if ((index = FindMarker(p_pointMarker)) != wxNOT_FOUND) {
      RemoveMarker(index);
    }
  } else {
    wxPlotCtrl *plotCtrl = this;
    wxPoint2DDouble curvePt = plotCtrl->GetCursorPoint();
    // Add a marker indicating the value of the selected point.
    wxString text;
    text << "(";
    if (p_xValueMap.find(curvePt) != p_xValueMap.end()) {
      text << p_xValueMap[curvePt];
    } else {
      text << curvePt.m_x;
    }
    text << ", ";
    if (p_yValueMap.find(curvePt) != p_yValueMap.end()) {
      text << p_yValueMap[curvePt];
    } else {
      text << curvePt.m_y;
    }
    text << ")";
    wxBitmap bitmap = wxBitmap(500,500); // big enough to hold our text
    wxMemoryDC dc;
    dc.SetFont(*wxSWISS_FONT);
    dc.SelectObject(bitmap);
    wxSize size(dc.GetTextExtent(text));
    dc.SelectObject(wxNullBitmap);
    bitmap = wxBitmap(size.GetWidth(), size.GetHeight());
    dc.SelectObject(bitmap);
    dc.Clear();
    dc.DrawText(text, 0, 0);
    dc.SelectObject(wxNullBitmap);
    p_pointMarker.CreateBitmapMarker(curvePt, bitmap);
    AddMarker(p_pointMarker);
  }
  Redraw(wxPLOTCTRL_REDRAW_PLOT);
}


void ewxPlotCtrl::SetXValueMap(const AxisMap& axisMap)
{
  p_xValueMap = axisMap;
}


void ewxPlotCtrl::SetYValueMap(const AxisMap& axisMap)
{
  p_yValueMap = axisMap;
}


void ewxPlotCtrl::OnClick( wxPlotCtrlEvent &event )
{
  event.Skip();

  wxPlotCtrl *plotCtrl = this;
  wxPlotData *curve;
  double mouseX = event.GetX(),
         mouseY = event.GetY();
  int closestCurveDataIndex;
  int closestCurveIndex;

  if (event.GetCurveIndex() != -1) {
    // User clicked directly on a point.
    curve = plotCtrl->GetDataCurve(event.GetCurveIndex());
  } else {
    // User clicked somewhere on the plot area, not a curve/point directly.
    // We must find the closest data point on any of our curves.
    wxArrayInt plotDataIndexes = GetPlotDataIndexes();
    int count = plotDataIndexes.GetCount();
    if (count > 0) {
      closestCurveIndex = plotDataIndexes[0];
      curve = plotCtrl->GetDataCurve(closestCurveIndex);
      closestCurveDataIndex = curve->GetIndexFromXY(mouseX, mouseY);
      wxPoint2DDouble closestPoint = curve->GetPoint(closestCurveDataIndex);
      double closestDistance = distance(mouseX, mouseY, closestPoint);
      for (int i=1; i<count; ++i) {
        int currentCurveIndex = plotDataIndexes[i];
        curve = plotCtrl->GetDataCurve(currentCurveIndex);
        int currentCurveDataIndex = curve->GetIndexFromXY(mouseX, mouseY);
        wxPoint2DDouble currentPoint = curve->GetPoint(currentCurveDataIndex);
        double currentDistance = distance(mouseX, mouseY, currentPoint);
        if (currentDistance < closestDistance) {
          closestDistance = currentDistance;
          closestCurveIndex = currentCurveIndex;
          closestCurveDataIndex = currentCurveDataIndex;
        }
      }
      curve = plotCtrl->GetDataCurve(closestCurveIndex);
    }
  }

  if (curve) {
    closestCurveDataIndex = curve->GetIndexFromXY(mouseX, mouseY);
    plotCtrl->SetCursorDataIndex(closestCurveIndex,closestCurveDataIndex,true);

    wxPoint2DDouble curvePt = plotCtrl->GetCursorPoint();
    wxPlotCtrlEvent pt_click_event(wxEVT_PLOTCTRL_POINT_CLICKED, GetId(), this);
    pt_click_event.SetPosition(curvePt.m_x, curvePt.m_y);
    pt_click_event.SetCurve(curve, closestCurveIndex);
    pt_click_event.SetCurveDataIndex(closestCurveDataIndex);
    (void)DoSendEvent( pt_click_event );

    SetShowPointLabel(false); // removes last point label
    SetShowPointLabel(true); // adds new point label
  }
}


void ewxPlotCtrl::OnRightClick( wxMouseEvent &event )
{
  p_menu->UpdateUI();
  PopupMenu(p_menu);
}


void ewxPlotCtrl::OnMenuClone( wxCommandEvent& event )
{
  ewxFrame * frame = new ewxFrame(this, wxID_ANY, GetPlotTitle());
  Clone(frame);
  frame->Show();
}


void ewxPlotCtrl::OnMenuZoomToFit( wxCommandEvent &event )
{
  MakeCurveVisible(-1);
}


void ewxPlotCtrl::OnMenuShowCrosshair( wxCommandEvent &event )
{
  SetCrossHairCursor(!GetCrossHairCursor());
}


void ewxPlotCtrl::OnMenuDrawSymbols( wxCommandEvent &event )
{
  SetDrawSymbols(!GetDrawSymbols());
}


void ewxPlotCtrl::OnMenuDrawGrid( wxCommandEvent &event )
{
  SetDrawGrid(!GetDrawGrid());
}


void ewxPlotCtrl::OnMenuShowPointLabel( wxCommandEvent &event )
{
  SetShowPointLabel(!p_showPointLabel);
}


void ewxPlotCtrl::OnMenuShowAxisX( wxCommandEvent &event )
{
  SetShowXAxis(!GetShowXAxis());
}


void ewxPlotCtrl::OnMenuShowAxisY( wxCommandEvent &event )
{
  SetShowYAxis(!GetShowYAxis());
}


void ewxPlotCtrl::OnMenuShowAxisLabelX( wxCommandEvent &event )
{
  SetShowXAxisLabel(!GetShowXAxisLabel());
}


void ewxPlotCtrl::OnMenuShowAxisLabelY( wxCommandEvent &event )
{
  SetShowYAxisLabel(!GetShowYAxisLabel());
}


void ewxPlotCtrl::OnMenuShowTitle( wxCommandEvent &event )
{
  SetShowPlotTitle(!GetShowPlotTitle());
}


void ewxPlotCtrl::OnMenuShowKey( wxCommandEvent &event )
{
  SetShowKey(!GetShowKey());
}


void ewxPlotCtrl::UpdateUIShowCrosshair( wxUpdateUIEvent &event )
{
  event.Check(GetCrossHairCursor());
}


void ewxPlotCtrl::UpdateUIDrawSymbols( wxUpdateUIEvent &event )
{
  event.Check(GetDrawSymbols());
}


void ewxPlotCtrl::UpdateUIDrawGrid( wxUpdateUIEvent &event )
{
  event.Check(GetDrawGrid());
}


void ewxPlotCtrl::UpdateUIShowPointLabel( wxUpdateUIEvent &event )
{
  event.Check(p_showPointLabel);
}


void ewxPlotCtrl::UpdateUIShowAxisX( wxUpdateUIEvent &event )
{
  event.Check(GetShowXAxis());
}


void ewxPlotCtrl::UpdateUIShowAxisY( wxUpdateUIEvent &event )
{
  event.Check(GetShowYAxis());
}


void ewxPlotCtrl::UpdateUIShowAxisLabelX( wxUpdateUIEvent &event )
{
  event.Check(GetShowXAxisLabel());
}


void ewxPlotCtrl::UpdateUIShowAxisLabelY( wxUpdateUIEvent &event )
{
  event.Check(GetShowYAxisLabel());
}


void ewxPlotCtrl::UpdateUIShowTitle( wxUpdateUIEvent &event )
{
  event.Check(GetShowPlotTitle());
}


void ewxPlotCtrl::UpdateUIShowKey( wxUpdateUIEvent &event )
{
  event.Check(GetShowKey());
}


void ewxPlotCtrl::ProcessAreaEVT_MOUSE_EVENTS( wxMouseEvent &event )
{
  if (event.RightDown()) {
    OnRightClick(event);
  } else {
    wxPlotCtrl::ProcessAreaEVT_MOUSE_EVENTS(event);
  }
}


void ewxPlotCtrl::ProcessAxisEVT_MOUSE_EVENTS( wxMouseEvent &event )
{
  if (event.RightDown()) {
    OnRightClick(event);
  } else {
    wxPlotCtrl::ProcessAxisEVT_MOUSE_EVENTS(event);
  }
}


void ewxPlotCtrl::Init()
{
}


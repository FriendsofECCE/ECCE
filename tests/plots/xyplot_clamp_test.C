// The zoomed-out view of an XYPlotCanvas is its data range and no zoom
// leaves it (#495).  Run under a display (xvfb-run).
#include <cmath>
#include <cstdio>

#include "wx/wx.h"

#include "XYPlotCanvas.H"

static int failures = 0;

static void expect(const char *what, bool ok)
{
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) failures++;
}

class CountedPlot : public XYPlotCanvas
{
 public:
  CountedPlot(wxWindow *parent) : XYPlotCanvas(parent) {}
  int refreshes = 0;
  void Refresh(bool erase = true, const wxRect *rect = NULL) override
  {
    refreshes++;
    XYPlotCanvas::Refresh(erase, rect);
  }
};

static void wheel(XYPlotCanvas *plot, int rotation, wxMouseWheelAxis axis,
                  const wxPoint& at, bool shift = false)
{
  wxMouseEvent event(wxEVT_MOUSEWHEEL);
  event.m_wheelRotation = rotation;
  event.m_wheelDelta = 120;
  event.m_wheelAxis = axis;
  event.SetPosition(at);
  event.SetShiftDown(shift);
  event.SetEventObject(plot);
  plot->GetEventHandler()->ProcessEvent(event);
}

class App : public wxApp
{
 public:
  bool OnInit() override
  {
    wxFrame *f = new wxFrame(NULL, wxID_ANY, "t", wxDefaultPosition,
                             wxSize(500, 320));
    CountedPlot *p = new CountedPlot(f);
    f->Show();
    std::vector<double> x, y;
    for (int i = 1; i <= 38; i++) { x.push_back(i); y.push_back(-1.0 / i); }
    p->addCurve(x, y);
    wxPoint at;
    p->pointPosition(0, 0, &at);          // lays the plot out
    double fx0, fx1, fy0, fy1;
    p->viewRange(&fx0, &fx1, &fy0, &fy1);

    for (int i = 0; i < 12; i++) p->zoomBy(1.25);
    p->pointPosition(0, 0, &at);
    double a, b, c, d;
    p->viewRange(&a, &b, &c, &d);
    expect("zooming out stops at the data range",
           std::fabs(a - fx0) < 1e-9 && std::fabs(b - fx1) < 1e-9 &&
           std::fabs(c - fy0) < 1e-9 && std::fabs(d - fy1) < 1e-9);
    expect("the data range is a small margin around the data",
           fx0 > -2.0 && fx0 < 1.0 && fx1 > 38.0 && fx1 < 40.0);

    for (int i = 0; i < 5; i++) p->zoomBy(0.8);
    p->pointPosition(0, 0, &at);
    p->viewRange(&a, &b, &c, &d);
    expect("zooming in narrows the view", b - a < 0.4 * (fx1 - fx0));
    for (int i = 0; i < 40; i++) p->zoomBy(1.25);
    p->pointPosition(0, 0, &at);
    p->viewRange(&a, &b, &c, &d);
    expect("zooming back out returns to the data range",
           std::fabs(a - fx0) < 1e-9 && std::fabs(b - fx1) < 1e-9);
    p->pointPosition(0, 18, &at);
    p->refreshes = 0;
    for (int i = 0; i < 8; i++) wheel(p, 15, wxMOUSE_WHEEL_VERTICAL, at);
    expect("wheel burst waits for a frame", p->refreshes == 0);
    wxTimer *check = new wxTimer(this);
    Bind(wxEVT_TIMER, [=](wxTimerEvent&) {
      expect("fractional wheel burst refreshes once", p->refreshes == 1);
      wxPoint pos;
      p->pointPosition(0, 18, &pos);
      double x0, x1, y0, y1;
      p->viewRange(&x0, &x1, &y0, &y1);
      expect("fractional rotations sum to one notch",
             std::fabs((x1 - x0) / (fx1 - fx0) - 0.8) < 1e-9 &&
             std::fabs((y1 - y0) / (fy1 - fy0) - 0.8) < 1e-9);
      p->refreshes = 0;
      for (int i = 0; i < 8; i++) wheel(p, 120, wxMOUSE_WHEEL_HORIZONTAL, pos);
      wxTimer *horizontal = new wxTimer(this);
      Bind(wxEVT_TIMER, [=](wxTimerEvent&) {
        expect("horizontal wheel does not refresh after a frame", p->refreshes == 0);
        wxPoint centre;
        p->pointPosition(0, 18, &centre);
        double hx0, hx1, hy0, hy1;
        p->viewRange(&hx0, &hx1, &hy0, &hy1);
        expect("horizontal wheel preserves both axes",
               x0 == hx0 && x1 == hx1 && y0 == hy0 && y1 == hy1);
        for (int i = 0; i < 8; i++)
          wheel(p, 15, wxMOUSE_WHEEL_VERTICAL, centre, i < 4);
        wxTimer *mixed = new wxTimer(this);
        Bind(wxEVT_TIMER, [=](wxTimerEvent&) {
          expect("mixed Shift wheel burst refreshes once", p->refreshes == 1);
          wxPoint ignored;
          p->pointPosition(0, 18, &ignored);
          double mx0, mx1, my0, my1;
          p->viewRange(&mx0, &mx1, &my0, &my1);
          expect("Shift wheel affects only the vertical axis",
                 std::fabs((mx1 - mx0) / (x1 - x0) - std::pow(0.8, 0.5)) < 1e-9 &&
                 std::fabs((my1 - my0) / (y1 - y0) - 0.8) < 1e-9);
          std::exit(failures ? 1 : 0);
        }, mixed->GetId());
        mixed->StartOnce(60);
      }, horizontal->GetId());
      horizontal->StartOnce(60);
    }, check->GetId());
    check->StartOnce(60);
    return true;
  }
};

wxIMPLEMENT_APP(App);

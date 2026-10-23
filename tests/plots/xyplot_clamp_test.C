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

class App : public wxApp
{
 public:
  bool OnInit() override
  {
    wxFrame *f = new wxFrame(NULL, wxID_ANY, "t", wxDefaultPosition,
                             wxSize(500, 320));
    XYPlotCanvas *p = new XYPlotCanvas(f);
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
    std::exit(failures ? 1 : 0);
    return true;
  }
};

wxIMPLEMENT_APP(App);

/**
 * @file
 *
 * The surface every x-y plot in the Builder is drawn on (#930).
 *
 * Same axes, ticks, fonts, palette and hover readout as SpectrumCanvas
 * (the vibrational spectrum) so all plots look alike; it replaces the old
 * ewxPlotCtrl in the property panels.  Defined inline, like
 * SpectrumCanvas.H, so a test can paint it onto a bitmap.
 *
 * Interaction: a click on a point selects it and calls the point handler
 * (the Geometry Trace shows that frame); dragging a rectangle zooms, the
 * wheel zooms about the pointer, right/middle drag pans, double click on
 * empty plot or Home fits everything.
 */

#ifndef XYPLOTCANVAS_H_
#define XYPLOTCANVAS_H_

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <vector>

#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/panel.h>
#include <wx/settings.h>

#include "tdat/VibSpectrum.H"
#include "wxgui/PlotStyle.H"

class XYPlotCanvas : public wxPanel
{
  public:

    /** LEVELS draws each point as a short horizontal bar (MO energies). */
    enum Style { LINE, LINE_MARKERS, MARKERS, LEVELS };

    /** Called with the curve and the zero-based point that was clicked. */
    typedef std::function<void(int curve, int index)> PointHandler;

    XYPlotCanvas(wxWindow *parent)
      //  Full repaint on resize: see SpectrumCanvas.
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE),
        p_palette(PlotPalette::system()), p_showKey(false), p_showXAxis(true),
        p_selCurve(-1), p_selIndex(-1), p_hoverCurve(-1), p_hoverIndex(-1),
        p_zoomed(false), p_xPad(0), p_pressed(false), p_dragging(false),
        p_panning(false), p_band(false)
    {
      SetBackgroundStyle(wxBG_STYLE_PAINT);
      SetMinSize(wxSize(240, 160));
      Bind(wxEVT_PAINT, &XYPlotCanvas::onPaint, this);
      Bind(wxEVT_LEFT_DOWN, &XYPlotCanvas::onLeftDown, this);
      Bind(wxEVT_LEFT_UP, &XYPlotCanvas::onLeftUp, this);
      Bind(wxEVT_LEFT_DCLICK, &XYPlotCanvas::onDoubleClick, this);
      Bind(wxEVT_RIGHT_DOWN, &XYPlotCanvas::onPanDown, this);
      Bind(wxEVT_MIDDLE_DOWN, &XYPlotCanvas::onPanDown, this);
      Bind(wxEVT_RIGHT_UP, &XYPlotCanvas::onPanUp, this);
      Bind(wxEVT_MIDDLE_UP, &XYPlotCanvas::onPanUp, this);
      Bind(wxEVT_MOTION, &XYPlotCanvas::onMotion, this);
      Bind(wxEVT_MOUSEWHEEL, &XYPlotCanvas::onWheel, this);
      Bind(wxEVT_KEY_DOWN, &XYPlotCanvas::onKey, this);
      Bind(wxEVT_LEAVE_WINDOW, &XYPlotCanvas::onLeave, this);
      Bind(wxEVT_MOUSE_CAPTURE_LOST, &XYPlotCanvas::onCaptureLost, this);
    }

    // ---- data -----------------------------------------------------------

    /** Add a curve; `series` picks its palette colour (default: its index). */
    int addCurve(const double *x, const double *y, size_t n,
                 const wxString& name = wxString(), int series = -1,
                 Style style = LINE_MARKERS)
    {
      Curve c;
      c.x.assign(x, x + n);
      c.y.assign(y, y + n);
      c.name = name;
      c.series = series >= 0 ? series : (int)p_curves.size();
      c.style = style;
      c.marker = 0;
      c.labels.assign(n, wxString());
      p_curves.push_back(c);
      p_zoomed = false;
      Refresh();
      return (int)p_curves.size() - 1;
    }

    int addCurve(const std::vector<double>& x, const std::vector<double>& y,
                 const wxString& name = wxString(), int series = -1,
                 Style style = LINE_MARKERS)
    {
      const size_t n = std::min(x.size(), y.size());
      return addCurve(n ? &x[0] : 0, n ? &y[0] : 0, n, name, series, style);
    }

    /** Drop every curve (and the selection); axis titles stay. */
    void clearCurves()
    {
      p_curves.clear();
      p_selCurve = p_selIndex = p_hoverCurve = p_hoverIndex = -1;
      p_zoomed = false;
      Refresh();
    }

    /** 0 circle, 1 square, 2 cross. */
    void setCurveMarker(int curve, int marker)
    {
      if (curveOk(curve)) p_curves[curve].marker = marker;
      Refresh();
    }

    /** Draw a curve in this colour rather than its palette series. */
    void setCurveColour(int curve, const wxColour& colour)
    {
      if (curveOk(curve)) p_curves[curve].colour = colour;
      Refresh();
    }

    /** Text shown in the hover readout for one point. */
    void setPointLabel(int curve, int index, const wxString& label)
    {
      if (pointOk(curve, index)) p_curves[curve].labels[index] = label;
    }

    void setXAxisLabel(const wxString& s) { p_xLabel = s; Refresh(); }
    void setYAxisLabel(const wxString& s) { p_yLabel = s; Refresh(); }
    void setTitle(const wxString& s) { p_title = s; Refresh(); }
    wxString title() const { return p_title; }
    void setShowKey(bool show) { p_showKey = show; Refresh(); }
    void setShowXAxis(bool show) { p_showXAxis = show; Refresh(); }
    /** Room, in x units, kept clear on either side of the data. */
    void setXPadding(double pad) { p_xPad = pad; Refresh(); }

    const PlotPalette& palette() const { return p_palette; }
    wxColour seriesColour(int i) const { return p_palette.series(i); }

    int curveCount() const { return (int)p_curves.size(); }
    int pointCount(int curve) const
    {
      return curveOk(curve) ? (int)p_curves[curve].x.size() : 0;
    }
    double xAt(int curve, int i) const { return p_curves[curve].x[i]; }
    double yAt(int curve, int i) const { return p_curves[curve].y[i]; }

    // ---- selection and clicks -------------------------------------------

    void setPointHandler(const PointHandler& h) { p_handler = h; }

    void setSelectedPoint(int curve, int index)
    {
      p_selCurve = curve;
      p_selIndex = index;
      Refresh();
    }
    int selectedCurve() const { return p_selCurve; }
    int selectedIndex() const { return p_selIndex; }

    /**
     * Where a point is drawn at the current size; false if off screen.
     * Lays the plot out itself, so it works before the first paint.
     */
    bool pointPosition(int curve, int index, wxPoint *at)
    {
      const wxSize size = GetClientSize();
      if (size.x < 10 || size.y < 10) return false;
      wxBitmap bmp(size.x, size.y);
      wxMemoryDC dc(bmp);
      paintOnto(dc, size);
      if (!pointOk(curve, index) || !p_laidOut) return false;
      *at = wxPoint(xOf(p_curves[curve].x[index]), yOf(p_curves[curve].y[index]));
      return p_plot.Contains(*at);
    }

    /** The point nearest `p` within a few pixels, or false. */
    bool pointAt(const wxPoint& p, int *curve, int *index) const
    {
      double best = 10.0;
      bool found = false;
      for (size_t c = 0; c < p_curves.size(); c++) {
        const Curve& cv = p_curves[c];
        for (size_t i = 0; i < cv.x.size(); i++) {
          const int px = xOf(cv.x[i]), py = yOf(cv.y[i]);
          double dx = px - p.x;
          if (cv.style == LEVELS) dx = std::max(0.0, std::fabs(dx) - 6.0);
          const double d = std::sqrt(dx * dx + (double)(py - p.y) * (py - p.y));
          if (d < best) { best = d; *curve = (int)c; *index = (int)i; found = true; }
        }
      }
      return found;
    }

    /** What a click at `p` does. */
    bool clickAt(const wxPoint& p)
    {
      int c, i;
      if (!p_plot.Contains(p) || !pointAt(p, &c, &i)) return false;
      setSelectedPoint(c, i);
      if (p_handler) p_handler(c, i);
      return true;
    }

    void fitAll() { p_zoomed = false; Refresh(); }

    // ---- painting ------------------------------------------------------

    void paintOnto(wxDC& dc, const wxSize& size)
    {
      p_laidOut = false;
      dc.SetBackground(wxBrush(p_palette.background));
      dc.Clear();
      dc.SetFont(*wxSMALL_FONT);
      dc.SetTextForeground(p_palette.text);

      double xlo, xhi, ylo, yhi;
      if (!dataRange(&xlo, &xhi, &ylo, &yhi)) {
        dc.SetFont(*wxNORMAL_FONT);
        const wxString msg = "Nothing to plot.";
        const wxSize e = dc.GetTextExtent(msg);
        dc.DrawText(msg, (size.x - e.x) / 2, (size.y - e.y) / 2);
        return;
      }
      if (p_zoomed) {
        xlo = p_vx0; xhi = p_vx1; ylo = p_vy0; yhi = p_vy1;
      }
      p_x0 = xlo; p_x1 = xhi; p_y0 = ylo; p_y1 = yhi;

      const int ch = dc.GetCharHeight();
      dc.SetFont(*wxNORMAL_FONT);
      const int th = dc.GetCharHeight();
      dc.SetFont(*wxSMALL_FONT);
      const bool haveTitle = !p_title.empty() && size.y >= 150;
      const bool haveKey = p_showKey && hasNames();
      const bool xTitle = p_showXAxis && !p_xLabel.empty() && size.y >= 140;
      const bool yTitle = !p_yLabel.empty();

      //  Y ticks first: the left margin depends on how wide they print.
      const int plotHGuess = std::max(40, size.y - 60);
      SpectrumTicks yt = niceTicks(ylo, yhi, std::max(3, plotHGuess / 45));
      int labelW = 0;
      for (size_t i = 0; i < yt.values.size(); i++)
        labelW = std::max(labelW,
                          dc.GetTextExtent(tickText(yt, yt.values[i])).x);
      const int left = (yTitle ? th + 8 : 4) + 10 + labelW;
      const int right = 14;
      int top = 8;
      if (haveTitle) top += th + 4;
      if (haveKey) top += ch + 6;
      const int bottom = p_showXAxis
          ? ch + 12 + (xTitle ? th + 6 : 0) : 8;
      const int plotW = std::max(40, size.x - left - right);
      const int plotH = std::max(30, size.y - top - bottom);
      p_plot = wxRect(left, top, plotW, plotH);
      p_laidOut = true;

      yt = niceTicks(ylo, yhi, std::max(3, plotH / 45));
      SpectrumTicks xt = niceTicks(xlo, xhi, std::max(3, plotW / 80));

      //  Grid.
      dc.SetBrush(*wxTRANSPARENT_BRUSH);
      dc.SetPen(wxPen(p_palette.grid, 1));
      for (size_t i = 0; i < xt.values.size() && p_showXAxis; i++) {
        const int x = xOf(xt.values[i]);
        dc.DrawLine(x, p_plot.y, x, p_plot.GetBottom());
      }
      for (size_t i = 0; i < yt.values.size(); i++) {
        const int y = yOf(yt.values[i]);
        dc.DrawLine(p_plot.x, y, p_plot.GetRight(), y);
      }

      //  Band being dragged out.
      if (p_band) {
        const int a = xOf(p_bandA.x), b = xOf(p_bandB.x);
        const int c = yOf(p_bandA.y), d = yOf(p_bandB.y);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(PlotPalette::mix(p_palette.text,
                                             p_palette.background, 0.85)));
        dc.DrawRectangle(std::min(a, b), std::min(c, d), std::abs(a - b) + 1,
                         std::abs(c - d) + 1);
      }

      dc.SetClippingRegion(p_plot.x - 4, p_plot.y - 4, p_plot.width + 8,
                           p_plot.height + 8);
      for (size_t c = 0; c < p_curves.size(); c++) paintCurve(dc, (int)c);
      if (pointOk(p_selCurve, p_selIndex)) {
        const Curve& cv = p_curves[p_selCurve];
        const int x = xOf(cv.x[p_selIndex]), y = yOf(cv.y[p_selIndex]);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(p_palette.imaginary, 2));
        dc.DrawCircle(x, y, 7);
        dc.SetBrush(wxBrush(p_palette.imaginary));
        dc.DrawCircle(x, y, 3);
      }
      dc.DestroyClippingRegion();

      //  Frame, ticks and labels.
      dc.SetBrush(*wxTRANSPARENT_BRUSH);
      dc.SetPen(wxPen(p_palette.axis, 1));
      dc.DrawRectangle(p_plot);
      dc.SetTextForeground(p_palette.text);
      for (size_t i = 0; i < yt.values.size(); i++) {
        const int y = yOf(yt.values[i]);
        dc.DrawLine(p_plot.x - 4, y, p_plot.x, y);
        const wxString t = tickText(yt, yt.values[i]);
        const wxSize e = dc.GetTextExtent(t);
        dc.DrawText(t, p_plot.x - 7 - e.x, y - e.y / 2);
      }
      if (p_showXAxis) {
        for (size_t i = 0; i < xt.values.size(); i++) {
          const int x = xOf(xt.values[i]);
          dc.DrawLine(x, p_plot.GetBottom(), x, p_plot.GetBottom() + 4);
          const wxString t = tickText(xt, xt.values[i]);
          const wxSize e = dc.GetTextExtent(t);
          dc.DrawText(t, x - e.x / 2, p_plot.GetBottom() + 6);
        }
      }

      dc.SetFont(*wxNORMAL_FONT);
      if (xTitle) {
        const wxSize e = dc.GetTextExtent(p_xLabel);
        dc.DrawText(p_xLabel, p_plot.x + (p_plot.width - e.x) / 2,
                    p_plot.GetBottom() + ch + 10);
      }
      if (yTitle) drawVerticalText(dc, p_yLabel, 6, p_plot.y + p_plot.height / 2,
                                   p_plot.height);
      if (haveTitle) {
        wxString t = p_title;
        wxSize e = dc.GetTextExtent(t);
        if (e.x <= size.x - 8)
          dc.DrawText(t, (size.x - e.x) / 2, 6);
      }
      dc.SetFont(*wxSMALL_FONT);
      if (haveKey) paintKey(dc, p_plot.x, top - ch - 4);
      if (p_hoverCurve >= 0) paintReadout(dc, size);
    }

  private:

    struct Curve
    {
      std::vector<double> x, y;
      std::vector<wxString> labels;
      wxString name;
      int series, marker;
      Style style;
      wxColour colour;       // invalid: the palette's series colour
    };

    wxColour colourOf(const Curve& cv) const
    {
      return cv.colour.IsOk() ? cv.colour : p_palette.series(cv.series);
    }

    bool curveOk(int c) const { return c >= 0 && c < (int)p_curves.size(); }
    bool pointOk(int c, int i) const
    {
      return curveOk(c) && i >= 0 && i < (int)p_curves[c].x.size();
    }
    bool hasNames() const
    {
      for (size_t i = 0; i < p_curves.size(); i++)
        if (!p_curves[i].name.empty()) return true;
      return false;
    }

    static wxString tickText(const SpectrumTicks& t, double v)
    {
      if (t.decimals > 6 || std::fabs(v) >= 1e7)
        return wxString::Format("%.3g", v);
      return wxString::Format("%.*f", t.decimals, v);
    }

    //  The range the data fills, with a margin so end points are not on
    //  the frame.
    bool dataRange(double *xlo, double *xhi, double *ylo, double *yhi) const
    {
      bool any = false;
      double x0 = 0, x1 = 0, y0 = 0, y1 = 0;
      for (size_t c = 0; c < p_curves.size(); c++)
        for (size_t i = 0; i < p_curves[c].x.size(); i++) {
          const double x = p_curves[c].x[i], y = p_curves[c].y[i];
          if (!std::isfinite(x) || !std::isfinite(y)) continue;
          if (!any) { x0 = x1 = x; y0 = y1 = y; any = true; continue; }
          x0 = std::min(x0, x); x1 = std::max(x1, x);
          y0 = std::min(y0, y); y1 = std::max(y1, y);
        }
      if (!any) return false;
      double dx = x1 - x0, dy = y1 - y0;
      if (dx <= 0) dx = std::max(1.0, std::fabs(x0) * 0.1);
      if (dy <= 0) dy = std::max(1e-6, std::fabs(y0) * 0.1);
      const double px = std::max(0.04 * dx, p_xPad);
      *xlo = x0 - px;  *xhi = x1 + px;
      *ylo = y0 - 0.06 * dy; *yhi = y1 + 0.06 * dy;
      if (x1 == x0) { *xlo = x0 - dx / 2; *xhi = x0 + dx / 2; }
      return true;
    }

    int xOf(double x) const
    {
      const double f = (x - p_x0) / (p_x1 - p_x0);
      return p_plot.x + (int)std::floor(f * (p_plot.width - 1) + 0.5);
    }
    int yOf(double y) const
    {
      const double f = (y - p_y0) / (p_y1 - p_y0);
      return p_plot.GetBottom() - (int)std::floor(f * (p_plot.height - 1) + 0.5);
    }
    double xFrom(int px) const
    {
      return p_x0 + (px - p_plot.x) / (double)std::max(1, p_plot.width - 1) *
             (p_x1 - p_x0);
    }
    double yFrom(int py) const
    {
      return p_y0 + (p_plot.GetBottom() - py) /
             (double)std::max(1, p_plot.height - 1) * (p_y1 - p_y0);
    }

    static void drawVerticalText(wxDC& dc, const wxString& text, int cx,
                                 int cy, int maxLength)
    {
      wxString t = text;
      wxSize e = dc.GetTextExtent(t);
      //  Too long for the plot: the unit is the part worth keeping.
      const int open = t.Find('(');
      if (e.x > maxLength && open != wxNOT_FOUND) {
        t = t.Mid(open + 1, t.Length() - open - 2);
        e = dc.GetTextExtent(t);
      }
      if (e.x > maxLength) return;
      dc.DrawRotatedText(t, cx, cy + e.x / 2, 90);
    }

    void paintCurve(wxDC& dc, int c)
    {
      const Curve& cv = p_curves[c];
      const wxColour col = colourOf(cv);
      std::vector<wxPoint> pts;
      for (size_t i = 0; i < cv.x.size(); i++)
        if (std::isfinite(cv.x[i]) && std::isfinite(cv.y[i]))
          pts.push_back(wxPoint(xOf(cv.x[i]), yOf(cv.y[i])));
      if (pts.empty()) return;
      if ((cv.style == LINE || cv.style == LINE_MARKERS) && pts.size() > 1) {
        dc.SetPen(wxPen(col, 2));
        dc.DrawLines((int)pts.size(), &pts[0]);
      }
      if (cv.style == LEVELS) {
        dc.SetPen(wxPen(col, 2));
        for (size_t i = 0; i < pts.size(); i++)
          dc.DrawLine(pts[i].x - 7, pts[i].y, pts[i].x + 8, pts[i].y);
      } else if (cv.style != LINE || pts.size() == 1) {
        dc.SetPen(wxPen(col, 1));
        dc.SetBrush(wxBrush(col));
        for (size_t i = 0; i < pts.size(); i++) drawMarker(dc, cv.marker, pts[i]);
      }
    }

    static void drawMarker(wxDC& dc, int kind, const wxPoint& p)
    {
      if (kind == 1) dc.DrawRectangle(p.x - 3, p.y - 3, 7, 7);
      else if (kind == 2) {
        dc.DrawLine(p.x - 3, p.y - 3, p.x + 4, p.y + 4);
        dc.DrawLine(p.x - 3, p.y + 3, p.x + 4, p.y - 4);
      } else dc.DrawCircle(p, 3);
    }

    void paintKey(wxDC& dc, int x, int y)
    {
      for (size_t i = 0; i < p_curves.size(); i++) {
        const Curve& cv = p_curves[i];
        if (cv.name.empty()) continue;
        const wxColour col = colourOf(cv);
        dc.SetPen(wxPen(col, 2));
        dc.DrawLine(x, y + dc.GetCharHeight() / 2, x + 14,
                    y + dc.GetCharHeight() / 2);
        dc.SetTextForeground(p_palette.text);
        dc.DrawText(cv.name, x + 19, y);
        x += 19 + dc.GetTextExtent(cv.name).x + 16;
      }
    }

    void paintReadout(wxDC& dc, const wxSize& size)
    {
      if (!pointOk(p_hoverCurve, p_hoverIndex)) return;
      const Curve& cv = p_curves[p_hoverCurve];
      std::vector<wxString> lines;
      if (!cv.name.empty()) lines.push_back(cv.name);
      if (!cv.labels[p_hoverIndex].empty())
        lines.push_back(cv.labels[p_hoverIndex]);
      lines.push_back(wxString::Format("Point %d", p_hoverIndex + 1));
      lines.push_back(wxString::Format("x  %.6g", cv.x[p_hoverIndex]));
      lines.push_back(wxString::Format("y  %.6g", cv.y[p_hoverIndex]));
      dc.SetFont(*wxSMALL_FONT);
      const int ch = dc.GetCharHeight();
      int w = 0;
      for (size_t i = 0; i < lines.size(); i++)
        w = std::max(w, dc.GetTextExtent(lines[i]).x);
      const int bw = w + 12, bh = (int)lines.size() * (ch + 1) + 8;
      int x = p_hoverAt.x + 14, y = p_hoverAt.y + 14;
      if (x + bw > size.x - 2) x = p_hoverAt.x - bw - 10;
      if (y + bh > size.y - 2) y = p_hoverAt.y - bh - 10;
      x = std::max(2, x);
      y = std::max(2, y);
      dc.SetPen(wxPen(p_palette.axis, 1));
      dc.SetBrush(wxBrush(p_palette.box));
      dc.DrawRectangle(x, y, bw, bh);
      dc.SetTextForeground(p_palette.text);
      for (size_t i = 0; i < lines.size(); i++)
        dc.DrawText(lines[i], x + 6, y + 4 + (int)i * (ch + 1));
    }

    // ---- mouse ----------------------------------------------------------

    void onPaint(wxPaintEvent&)
    {
      wxAutoBufferedPaintDC dc(this);
      paintOnto(dc, GetClientSize());
    }

    void startView()
    {
      if (p_zoomed) return;
      p_vx0 = p_x0; p_vx1 = p_x1; p_vy0 = p_y0; p_vy1 = p_y1;
      p_zoomed = true;
    }

    void zoomAbout(double cx, double cy, double factor)
    {
      startView();
      p_vx0 = cx + (p_vx0 - cx) * factor;  p_vx1 = cx + (p_vx1 - cx) * factor;
      p_vy0 = cy + (p_vy0 - cy) * factor;  p_vy1 = cy + (p_vy1 - cy) * factor;
      Refresh();
    }

    void onLeftDown(wxMouseEvent& e)
    {
      SetFocus();
      if (!p_laidOut || !p_plot.Contains(e.GetPosition())) return;
      p_pressed = true;
      p_dragging = false;
      p_pressAt = e.GetPosition();
      if (!HasCapture()) CaptureMouse();
    }

    void onMotion(wxMouseEvent& e)
    {
      const wxPoint p = e.GetPosition();
      if (!p_laidOut) return;
      if (p_panning) {
        startView();
        const double dx = xFrom(p_lastPan.x) - xFrom(p.x);
        const double dy = yFrom(p_lastPan.y) - yFrom(p.y);
        p_vx0 += dx; p_vx1 += dx; p_vy0 += dy; p_vy1 += dy;
        p_lastPan = p;
        Refresh();
        return;
      }
      if (p_pressed && e.LeftIsDown()) {
        if (!p_dragging && (std::abs(p.x - p_pressAt.x) > 4 ||
                            std::abs(p.y - p_pressAt.y) > 4))
          p_dragging = true;
        if (p_dragging) {
          p_band = true;
          p_bandA = wxRealPoint(xFrom(p_pressAt.x), yFrom(p_pressAt.y));
          p_bandB = wxRealPoint(xFrom(p.x), yFrom(p.y));
          Refresh();
        }
        return;
      }
      int c = -1, i = -1;
      const bool hit = p_plot.Contains(p) && pointAt(p, &c, &i);
      if (!hit) { c = -1; i = -1; }
      if (hit || p_hoverCurve >= 0) {
        p_hoverCurve = c;
        p_hoverIndex = i;
        p_hoverAt = p;
        Refresh();
      }
      SetCursor(hit ? wxCursor(wxCURSOR_HAND) : wxNullCursor);
    }

    void onLeftUp(wxMouseEvent& e)
    {
      if (!p_pressed) return;
      p_pressed = false;
      if (HasCapture()) ReleaseMouse();
      const wxPoint p = e.GetPosition();
      if (p_dragging) {
        p_dragging = false;
        p_band = false;
        if (std::abs(p.x - p_pressAt.x) >= 6 && std::abs(p.y - p_pressAt.y) >= 6) {
          const double xa = xFrom(p_pressAt.x), xb = xFrom(p.x);
          const double ya = yFrom(p_pressAt.y), yb = yFrom(p.y);
          p_vx0 = std::min(xa, xb); p_vx1 = std::max(xa, xb);
          p_vy0 = std::min(ya, yb); p_vy1 = std::max(ya, yb);
          p_zoomed = true;
        }
        Refresh();
        return;
      }
      clickAt(p);
    }

    void onDoubleClick(wxMouseEvent& e)
    {
      int c, i;
      if (p_laidOut && p_plot.Contains(e.GetPosition()) &&
          !pointAt(e.GetPosition(), &c, &i))
        fitAll();
    }

    void onPanDown(wxMouseEvent& e)
    {
      if (!p_laidOut || !p_plot.Contains(e.GetPosition())) return;
      SetFocus();
      p_panning = true;
      p_lastPan = e.GetPosition();
      if (!HasCapture()) CaptureMouse();
    }

    void onPanUp(wxMouseEvent&)
    {
      p_panning = false;
      if (HasCapture()) ReleaseMouse();
    }

    void onCaptureLost(wxMouseCaptureLostEvent&)
    {
      p_panning = p_pressed = p_dragging = p_band = false;
      Refresh();
    }

    void onWheel(wxMouseEvent& e)
    {
      if (!p_laidOut) return;
      const int delta = e.GetWheelDelta() > 0 ? e.GetWheelDelta() : 120;
      const double notches = e.GetWheelRotation() / (double)delta;
      zoomAbout(xFrom(e.GetPosition().x), yFrom(e.GetPosition().y),
                std::pow(0.8, notches));
    }

    void onKey(wxKeyEvent& e)
    {
      const double cx = 0.5 * (p_x0 + p_x1), cy = 0.5 * (p_y0 + p_y1);
      switch (e.GetKeyCode()) {
        case WXK_HOME: fitAll(); return;
        case '+': case '=': case WXK_NUMPAD_ADD: zoomAbout(cx, cy, 0.8); return;
        case '-': case WXK_NUMPAD_SUBTRACT: zoomAbout(cx, cy, 1.25); return;
        default: e.Skip();
      }
    }

    void onLeave(wxMouseEvent&)
    {
      if (p_hoverCurve >= 0 && !p_pressed) {
        p_hoverCurve = p_hoverIndex = -1;
        Refresh();
      }
    }

    PlotPalette p_palette;
    std::vector<Curve> p_curves;
    wxString p_title, p_xLabel, p_yLabel;
    bool p_showKey, p_showXAxis;
    PointHandler p_handler;
    int p_selCurve, p_selIndex, p_hoverCurve, p_hoverIndex;
    wxPoint p_hoverAt;

    //  View: the full data range unless zoomed.  p_x0.. are what the last
    //  paint used, which the mouse maths reads.
    bool   p_zoomed;
    double p_xPad;
    double p_vx0 = 0, p_vx1 = 1, p_vy0 = 0, p_vy1 = 1;
    double p_x0 = 0, p_x1 = 1, p_y0 = 0, p_y1 = 1;
    wxRect p_plot;
    bool   p_laidOut = false;

    bool p_pressed, p_dragging, p_panning, p_band;
    wxPoint p_pressAt, p_lastPan;
    wxRealPoint p_bandA, p_bandB;
};

#endif

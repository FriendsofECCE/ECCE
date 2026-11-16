#include "QsarWindow.H"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/dcbuffer.h>
#include <wx/filedlg.h>
#include <wx/graphics.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/progdlg.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/settings.h>

#include "ChecksDialog.H"
#include "SiExport.H"
#include "util/Ecce.H"
#include "util/Filesystem.H"

using std::string;
using std::vector;
namespace fs = ecce::fs;

namespace {
string quote(const string& s)
{
  string q;
  for (char c : s) if (c != '\'' && c != '"') q += c;
  return "'" + q + "'";
}

string tool()
{
  const char *home = getenv("ECCE_HOME");
  return home && *home ? string(home) + "/bin/ecce-qsar" : string("ecce-qsar");
}

wxString U(const string& s) { return wxString::FromUTF8(s.c_str()); }

string slurp(const fs::path& p)
{
  std::ifstream f(p);
  return string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
}

string scalingToken(int sel)
{
  static const char *const t[] = {"uv", "pareto", "ctr"};
  return t[sel < 0 || sel > 2 ? 0 : sel];
}

const char *const SCALING_TIPS[] = {
  "Autoscale: each descriptor is centred and divided by its standard deviation, so every "
  "descriptor counts equally whatever its unit.",
  "Pareto: centred and divided by the square root of the standard deviation; large "
  "descriptors count somewhat more than small ones.",
  "Centre only: the mean is subtracted but nothing is divided; descriptors with large "
  "numbers dominate."};

// "0.123" for a number, "-" for nothing.
string num3(const string& s)
{
  char *end;
  const double v = strtod(s.c_str(), &end);
  if (s.empty() || end == s.c_str()) return "-";
  char b[32];
  snprintf(b, sizeof b, "%.3f", v);
  return b;
}

string percent(double fraction)
{
  char b[32];
  snprintf(b, sizeof b, "%.0f", fraction * 100.0);
  return b;
}

// "bp (degC)" -> unit "°C"; header text with the degree sign set.
string niceHeader(string h)
{
  const size_t at = h.find("degC");
  if (at != string::npos) h.replace(at, 4, "\xC2\xB0" "C");
  return h;
}

// One non-overlapping-label scatter plot with axes, ticks and a hover readout.
const wxColour BLUE(30, 90, 170), RED(200, 50, 40), ORANGE(230, 140, 20), INK(40, 40, 40),
               GRIDC(225, 225, 225);

wxFont plotFont(double size = 1.0, bool bold = false)
{
  wxFont f = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
  f.SetPointSize((int)std::lround(f.GetPointSize() * size));
  if (bold) f.SetWeight(wxFONTWEIGHT_BOLD);
  return f;
}

double niceStep(double range, int want)
{
  const double raw = range / std::max(1, want);
  const double mag = std::pow(10.0, std::floor(std::log10(raw)));
  const double f = raw / mag;
  return mag * (f < 1.5 ? 1 : f < 3.5 ? 2 : f < 7.5 ? 5 : 10);
}

string tickText(double v, double step)
{
  char b[32];
  const int dec = step >= 1 ? 0 : (int)std::ceil(-std::log10(step));
  snprintf(b, sizeof b, "%.*f", std::min(dec, 6), std::fabs(v) < step * 1e-6 ? 0.0 : v);
  return b;
}
}  // namespace


class QsarPlot : public wxPanel
{
public:
  QsarPlot(wxWindow *parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(440, 420))
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &QsarPlot::OnPaint, this);
    Bind(wxEVT_MOTION, &QsarPlot::OnMove, this);
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { p_hover = -1; UnsetToolTip(); Refresh(); });
    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) { click(e.GetPosition()); });
  }

  /// Called with the molecule name when a point is clicked.
  std::function<void(const string&)> onPick;
  /// Tick labels of the last paint, for the test hook.
  string xLabels, yLabels;

  /// flags: 0 normal, 1 outside the model limits, 2 kept out of the fit (test set).
  void set(const string& title, const string& xl, const string& yl, const vector<string>& names,
           const vector<double>& x, const vector<double>& y, const vector<int>& flag,
           bool parity, const vector<string>& tips)
  {
    p_title = title; p_xl = xl; p_yl = yl; p_names = names; p_x = x; p_y = y; p_flag = flag;
    p_parity = parity; p_hover = p_sel = -1; p_tips = tips;
    UnsetToolTip();
    Refresh();
  }

  size_t count() const { return p_x.size(); }
  const string& tip(size_t i) const { return p_tips[i]; }
  wxPoint position(size_t i) const
  {
    const Frame f = frame(nullptr);
    return wxPoint((int)std::lround(X(f, p_x[i])), (int)std::lround(Y(f, p_y[i])));
  }
  /// The point within 10 px of `pos`, or -1.
  int hit(wxPoint pos) const
  {
    int best = -1;
    if (p_x.empty() || p_x.size() != p_y.size() || p_tips.size() != p_x.size()) return best;
    const Frame f = frame(nullptr);
    double bd = 100;
    for (size_t i = 0; i < p_x.size(); i++) {
      const double dx = X(f, p_x[i]) - pos.x, dy = Y(f, p_y[i]) - pos.y;
      if (dx * dx + dy * dy < bd) { bd = dx * dx + dy * dy; best = (int)i; }
    }
    return best;
  }
  /// Show the details of the point under `pos` as its tooltip; returns them.
  string hover(wxPoint pos)
  {
    const int i = hit(pos);
    if (i != p_hover) { p_hover = i; Refresh(); }
    if (i < 0) { UnsetToolTip(); return ""; }
    SetToolTip(U(p_tips[i]));
    return p_tips[i];
  }
  int click(wxPoint pos)
  {
    p_sel = hit(pos);
    if (p_sel >= 0 && onPick) onPick(p_names[p_sel]);
    Refresh();
    return p_sel;
  }

private:
  struct Frame { double x0, x1, y0, y1; int l, t, w, h; };

  Frame frame(wxGraphicsContext *gc) const
  {
    Frame f{};
    double x0 = *std::min_element(p_x.begin(), p_x.end()), x1 = *std::max_element(p_x.begin(), p_x.end());
    double y0 = *std::min_element(p_y.begin(), p_y.end()), y1 = *std::max_element(p_y.begin(), p_y.end());
    if (p_parity) { x0 = y0 = std::min(x0, y0); x1 = y1 = std::max(x1, y1); }
    if (x1 - x0 < 1e-12) { x0 -= 1; x1 += 1; }
    if (y1 - y0 < 1e-12) { y0 -= 1; y1 += 1; }
    const double px = 0.06 * (x1 - x0), py = 0.06 * (y1 - y0);
    f.x0 = x0 - px; f.x1 = x1 + px; f.y0 = y0 - py; f.y1 = y1 + py;
    // Ends on tick values, so the axis labels bracket the data and y = x spans the range.
    auto enclose = [](double& lo, double& hi) {
      const double step = niceStep(hi - lo, 6);
      lo = std::floor(lo / step) * step; hi = std::ceil(hi / step) * step;
    };
    enclose(f.x0, f.x1); enclose(f.y0, f.y1);
    if (p_parity) { f.x0 = f.y0 = std::min(f.x0, f.y0); f.x1 = f.y1 = std::max(f.x1, f.y1); }
    const wxSize s = GetClientSize();
    f.l = 58; f.t = 34; f.w = std::max(20, s.x - f.l - 16); f.h = std::max(20, s.y - f.t - 70);
    (void)gc;
    return f;
  }
  double X(const Frame& f, double v) const { return f.l + (v - f.x0) / (f.x1 - f.x0) * f.w; }
  double Y(const Frame& f, double v) const { return f.t + f.h - (v - f.y0) / (f.y1 - f.y0) * f.h; }

  void text(wxGraphicsContext *gc, const string& s, double x, double y, const wxColour& c,
            double size = 1.0, bool bold = false, double angle = 0) const
  {
    gc->SetFont(plotFont(size, bold), c);
    if (angle == 0) gc->DrawText(U(s), x, y); else gc->DrawText(U(s), x, y, angle);
  }
  wxSize extent(wxGraphicsContext *gc, const string& s, double size = 1.0) const
  {
    gc->SetFont(plotFont(size), INK);
    double w, h;
    gc->GetTextExtent(U(s), &w, &h);
    return wxSize((int)std::ceil(w), (int)std::ceil(h));
  }

  void OnPaint(wxPaintEvent&)
  {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;
    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
    text(gc.get(), p_title, 8, 6, INK, 1.1, true);
    if (p_x.empty() || p_x.size() != p_y.size()) return;
    const Frame f = frame(gc.get());
    // Grid and ticks.
    xLabels.clear(); yLabels.clear();
    gc->SetPen(wxPen(GRIDC));
    const double sx = niceStep(f.x1 - f.x0, 6), sy = niceStep(f.y1 - f.y0, 6);
    for (double v = std::ceil(f.x0 / sx) * sx; v <= f.x1; v += sx) {
      gc->StrokeLine(X(f, v), f.t, X(f, v), f.t + f.h);
      const string t = tickText(v, sx);
      xLabels += t + " ";
      text(gc.get(), t, X(f, v) - extent(gc.get(), t).x / 2.0, f.t + f.h + 4, INK, 0.9);
    }
    for (double v = std::ceil(f.y0 / sy) * sy; v <= f.y1; v += sy) {
      gc->StrokeLine(f.l, Y(f, v), f.l + f.w, Y(f, v));
      const string t = tickText(v, sy);
      yLabels += t + " ";
      text(gc.get(), t, f.l - 6 - extent(gc.get(), t, 0.9).x, Y(f, v) - 8, INK, 0.9);
    }
    gc->SetPen(wxPen(wxColour(120, 120, 120)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRectangle(f.l, f.t, f.w, f.h);
    if (p_parity) {
      gc->SetPen(wxPen(wxColour(150, 150, 150), 1, wxPENSTYLE_SHORT_DASH));
      gc->StrokeLine(X(f, f.x0), Y(f, f.x0), X(f, f.x1), Y(f, f.x1));
    } else {
      gc->SetPen(wxPen(wxColour(170, 170, 170)));
      if (f.x0 < 0 && f.x1 > 0) gc->StrokeLine(X(f, 0), f.t, X(f, 0), f.t + f.h);
      if (f.y0 < 0 && f.y1 > 0) gc->StrokeLine(f.l, Y(f, 0), f.l + f.w, Y(f, 0));
    }
    text(gc.get(), p_xl, f.l + f.w / 2.0 - extent(gc.get(), p_xl).x / 2.0, f.t + f.h + 24, INK);
    text(gc.get(), p_yl, 8, f.t + f.h / 2.0 + extent(gc.get(), p_yl).x / 2.0, INK, 1.0, false,
         M_PI / 2);
    // Points; the labels go in only where they do not overlap one already placed.
    vector<wxRect> placed;
    for (size_t i = 0; i < p_x.size(); i++) {
      const double px = X(f, p_x[i]), py = Y(f, p_y[i]);
      const int fl = p_flag[i];
      gc->SetPen(wxPen(fl == 1 ? ORANGE : fl == 2 ? RED : BLUE, fl == 1 ? 2 : 1));
      gc->SetBrush(fl == 1 ? *wxTRANSPARENT_BRUSH : wxBrush(fl == 2 ? RED : BLUE));
      if (fl == 1) gc->DrawRectangle(px - 4, py - 4, 8, 8);
      else gc->DrawEllipse(px - 3.5, py - 3.5, 7, 7);
      placed.push_back(wxRect((int)px - 4, (int)py - 4, 8, 8));
      if ((int)i == p_sel) {
        gc->SetPen(wxPen(*wxBLACK, 2));
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->DrawEllipse(px - 8, py - 8, 16, 16);
      }
    }
    for (size_t i = 0; i < p_x.size(); i++) {
      const wxSize e = extent(gc.get(), p_names[i], 0.85);
      const double px = X(f, p_x[i]), py = Y(f, p_y[i]);
      const wxPoint cand[] = {{(int)px + 6, (int)py - e.y / 2}, {(int)px - 6 - e.x, (int)py - e.y / 2},
                              {(int)px - e.x / 2, (int)py - 6 - e.y}, {(int)px - e.x / 2, (int)py + 6}};
      for (const wxPoint& c : cand) {
        wxRect r(c.x, c.y, e.x, e.y);
        if (r.x < f.l || r.GetRight() > f.l + f.w || r.y < f.t || r.GetBottom() > f.t + f.h) continue;
        bool hit = false;
        for (const wxRect& q : placed) if (q.Intersects(r)) { hit = true; break; }
        if (hit) continue;
        placed.push_back(r);
        text(gc.get(), p_names[i], c.x, c.y, INK, 0.85);
        break;
      }
    }
    // Legend.
    int lx = f.l, ly = f.t + f.h + 44;
    auto key = [&](int fl, const string& what) {
      gc->SetPen(wxPen(fl == 1 ? ORANGE : fl == 2 ? RED : BLUE, fl == 1 ? 2 : 1));
      gc->SetBrush(fl == 1 ? *wxTRANSPARENT_BRUSH : wxBrush(fl == 2 ? RED : BLUE));
      if (fl == 1) gc->DrawRectangle(lx, ly + 4, 8, 8); else gc->DrawEllipse(lx, ly + 4, 8, 8);
      text(gc.get(), what, lx + 13, ly, INK, 0.9);
      lx += 13 + extent(gc.get(), what, 0.9).x + 14;
    };
    bool any1 = false, any2 = false;
    for (int fl : p_flag) { any1 |= fl == 1; any2 |= fl == 2; }
    key(0, p_parity ? "fitted molecules" : "molecules");
    if (any2) key(2, "test set (not in the fit)");
    if (any1) key(1, "outside the model limits (95 %, T\xC2\xB2 or DModX)");
    // Hover readout.
    if (p_hover >= 0 && (size_t)p_hover < p_x.size()) {
      char b[160];
      snprintf(b, sizeof b, "%s: %.4g, %.4g", p_names[p_hover].c_str(), p_x[p_hover], p_y[p_hover]);
      const wxSize e = extent(gc.get(), b);
      double bx = std::min<double>(X(f, p_x[p_hover]) + 10, f.l + f.w - e.x - 8);
      double by = std::max<double>(f.t + 2, Y(f, p_y[p_hover]) - e.y - 10);
      gc->SetPen(wxPen(wxColour(90, 90, 90)));
      gc->SetBrush(wxBrush(wxColour(255, 255, 225)));
      gc->DrawRectangle(bx - 3, by - 2, e.x + 6, e.y + 4);
      text(gc.get(), b, bx, by, INK);
    }
  }

  void OnMove(wxMouseEvent& e)
  {
    hover(e.GetPosition());
  }

  string p_title, p_xl, p_yl;
  vector<string> p_names;
  vector<double> p_x, p_y;
  vector<int> p_flag;
  bool p_parity = false;
  vector<string> p_tips;
  int p_hover = -1, p_sel = -1;
};

// Variance bars per component with the cumulative line (PCA: R2X; PLS: R2X, R2Y, Q2).
class QsarScree : public wxPanel
{
public:
  string yLabels;
  QsarScree(wxWindow *parent) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(440, 420))
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &QsarScree::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
  }
  void set(const vector<vector<double>>& bars, const vector<vector<double>>& lines)
  {
    p_bars = bars; p_lines = lines;
    Refresh();
  }

private:
  void OnPaint(wxPaintEvent&)
  {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    const wxSize sz = GetClientSize();
    if (p_bars.empty() || p_bars[0].empty() || sz.x < 180 || sz.y < 180) return;
    const int n = (int)std::min<size_t>(10, p_bars[0].size()), left = 55, top = 70;
    const double w = sz.x - left - 20, h = sz.y - top - 50;
    double lo = 0, hi = 100;
    for (const auto *group : {&p_bars, &p_lines})
      for (const auto& s : *group)
        for (int i = 0; i < n && i < (int)s.size(); i++)
          if (std::isfinite(s[i])) lo = std::min(lo, s[i] * 100);
    const double step = niceStep(hi - lo, 4);
    lo = std::floor(lo / step) * step;
    auto Y = [&](double pc) { return top + (int)((hi - pc) / (hi - lo) * h); };
    auto X = [&](int i) { return left + (int)((i + .5) * w / n); };
    dc.SetFont(plotFont());
    dc.SetTextForeground(INK);
    dc.DrawText("Variance explained (%)", left, 44);
    yLabels.clear();
    for (double v = lo; v <= hi + step * 1e-6; v += step) {
      dc.SetPen(wxPen(GRIDC));
      dc.DrawLine(left, Y(v), left + (int)w, Y(v));
      const wxString t = U(tickText(v, step));
      dc.DrawText(t, left - 6 - dc.GetTextExtent(t).x, Y(v) - 7);
      yLabels += t.ToStdString() + " ";
    }
    const wxColour col[] = {BLUE, ORANGE, wxColour(25, 140, 90)};
    const char *lab[] = {"R\xC2\xB2X", "R\xC2\xB2Y", "Q\xC2\xB2"};
    const int count = (int)std::min<size_t>(3, p_bars.size());
    for (int s = 0; s < count; s++) {
      const double bw = std::min(22.0, .7 * w / n / count);
      dc.SetPen(wxPen(col[s])); dc.SetBrush(wxBrush(col[s]));
      for (int i = 0; i < n && i < (int)p_bars[s].size(); i++) {
        if (!std::isfinite(p_bars[s][i])) continue;
        const int y = Y(p_bars[s][i] * 100), zero = Y(0);
        dc.DrawRectangle(X(i) + (int)((s - count / 2.0) * bw), std::min(y, zero),
                         std::max(1, (int)bw - 1), std::max(1, std::abs(zero - y)));
      }
      dc.SetPen(wxPen(col[s], 2));
      for (int i = 1; i < n && i < (int)p_lines[s].size(); i++)
        if (std::isfinite(p_lines[s][i]) && std::isfinite(p_lines[s][i - 1]))
          dc.DrawLine(X(i - 1), Y(p_lines[s][i - 1] * 100), X(i), Y(p_lines[s][i] * 100));
      for (int i = 0; i < n && i < (int)p_lines[s].size(); i++)
        if (std::isfinite(p_lines[s][i])) dc.DrawCircle(X(i), Y(p_lines[s][i] * 100), 3);
      const int lx = left + s * (int)(w / count);
      dc.DrawRectangle(lx, 10, 9, 9);
      dc.DrawText(U(string(lab[s]) + " per component"), lx + 14, 6);
      dc.DrawLine(lx, 33, lx + 10, 33);
      dc.DrawText(U(string("Cumulative ") + lab[s]), lx + 14, 26);
    }
    dc.SetTextForeground(INK);
    for (int i = 0; i < n; i++) dc.DrawText(wxString::Format("%d", i + 1), X(i) - 4, top + (int)h + 5);
    dc.DrawText("Component", left + (int)w / 2 - 30, sz.y - 24);
  }
  vector<vector<double>> p_bars, p_lines;
};


namespace {
// Tab-separated text of the grid: all of it with headings, or the selected cells.
wxString gridText(wxGrid *g, bool whole)
{
  wxString text;
  int r0 = 0, r1 = g->GetNumberRows() - 1, c0 = 0, c1 = g->GetNumberCols() - 1;
  if (!whole) {
    r0 = r1 + 1; c0 = c1 + 1; r1 = c1 = -1;
    for (int r = 0; r < g->GetNumberRows(); ++r)
      for (int c = 0; c < g->GetNumberCols(); ++c)
        if (g->IsInSelection(r, c)) {
          r0 = std::min(r0, r); r1 = std::max(r1, r);
          c0 = std::min(c0, c); c1 = std::max(c1, c);
        }
    if (r1 < 0) { r0 = r1 = g->GetGridCursorRow(); c0 = c1 = g->GetGridCursorCol(); }
  }
  if (r0 < 0 || c0 < 0 || c1 < c0) return text;
  if (whole) {
    for (int c = c0; c <= c1; ++c) { if (c != c0) text += "\t"; text += g->GetColLabelValue(c); }
    text += "\n";
  }
  for (int r = r0; r <= r1; ++r) {
    for (int c = c0; c <= c1; ++c) {
      if (c != c0) text += "\t";
      if (whole || !g->IsSelection() || g->IsInSelection(r, c)) text += g->GetCellValue(r, c);
    }
    text += "\n";
  }
  return text;
}

bool copyGrid(wxGrid *g, bool whole)
{
  if (!wxTheClipboard->Open()) return false;
  const bool ok = wxTheClipboard->SetData(new wxTextDataObject(gridText(g, whole)));
  wxTheClipboard->Close();
  return ok;
}

wxString clipboardText()
{
  wxTextDataObject text;
  if (wxTheClipboard->Open()) { wxTheClipboard->GetData(text); wxTheClipboard->Close(); }
  return text.GetText();
}

wxButton *copyButton(wxWindow *parent, wxGrid *g)
{
  wxButton *b = new wxButton(parent, wxID_ANY, "Copy table");
  b->SetToolTip("Copy the whole table with its headings (Ctrl+C copies the selected cells).");
  b->Bind(wxEVT_BUTTON, [g](wxCommandEvent&) { copyGrid(g, true); });
  return b;
}

// A results grid: worded headers, 3 decimals, rows up to `used` highlighted and
// the column `emph` in bold on a tint.
void fillGrid(wxGrid *g, const vector<string>& heads, const vector<vector<string>>& rows,
              size_t used, int emph)
{
  if (g->GetNumberRows()) g->DeleteRows(0, g->GetNumberRows());
  if (g->GetNumberCols()) g->DeleteCols(0, g->GetNumberCols());
  g->AppendCols((int)heads.size());
  g->AppendRows((int)rows.size());
  for (size_t j = 0; j < heads.size(); j++) g->SetColLabelValue((int)j, U(heads[j]));
  for (size_t i = 0; i < rows.size(); i++)
    for (size_t j = 0; j < heads.size() && j < rows[i].size(); j++) {
      g->SetCellValue((int)i, (int)j, U(rows[i][j]));
      g->SetReadOnly((int)i, (int)j);
      if (j > 0) g->SetCellAlignment((int)i, (int)j, wxALIGN_RIGHT, wxALIGN_CENTRE);
      if (i < used) g->SetCellBackgroundColour((int)i, (int)j, wxColour(225, 240, 255));
      if ((int)j == emph) {
        g->SetCellBackgroundColour((int)i, (int)j, i < used ? wxColour(190, 220, 250)
                                                            : wxColour(238, 238, 238));
        wxFont f = g->GetDefaultCellFont();
        f.SetWeight(wxFONTWEIGHT_BOLD);
        g->SetCellFont((int)i, (int)j, f);
      }
    }
  g->AutoSizeColumns();
  g->AutoSizeRows();
}

wxGrid *makeGrid(wxWindow *parent, const wxSize& size)
{
  wxGrid *g = new wxGrid(parent, wxID_ANY, wxDefaultPosition, size);
  g->CreateGrid(0, 0);
  g->EnableEditing(false);
  g->HideRowLabels();
  g->SetColLabelSize(wxGRID_AUTOSIZE);
  g->Bind(wxEVT_CHAR_HOOK, [g](wxKeyEvent& e) {
    if (e.GetModifiers() == wxMOD_CONTROL && (e.GetKeyCode() == 'C' || e.GetKeyCode() == 'c'))
      copyGrid(g, false);
    else e.Skip();
  });
  return g;
}

wxStaticText *wrapped(wxWindow *parent, const string& s, int width)
{
  wxStaticText *t = new wxStaticText(parent, wxID_ANY, U(s));
  t->Wrap(width);
  return t;
}
}  // namespace


string QsarWindow::run(const string& args, int& status) const
{
  string out;
#ifdef _WIN32
  fs::path py = fs::path(Ecce::ecceHome()) / "python" / "python.exe";
  string pyExe = fs::exists(py) ? py.string() : string("python.exe");
  status = Ecce::readCommand("\"" + pyExe + "\" \"" + tool() + "\" " + args, out);
#else
  status = Ecce::readCommand(quote(tool()) + " " + args + " 2>&1", out);
#endif
  return out;
}

string QsarWindow::failure(const string& what) const
{
  const size_t e = p_summary.find("error: ");
  return e == string::npos ? what : p_summary.substr(e + 7, p_summary.find('\n', e) - e - 7);
}

bool QsarWindow::load(const vector<EcceURL>& calcs, const string& groups, string& why)
{
  std::unique_ptr<wxProgressDialog> pd;
  GatherProgress progress;
  if (calcs.size() > 3) {
    pd.reset(new wxProgressDialog("QSAR", "Starting...", (int)calcs.size(), this,
        wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_APP_MODAL));
    progress = [&pd](size_t i, size_t n, const string& name) {
      return pd->Update((int)i, wxString::Format("Reading %zu of %zu: %s", i + 1, n, U(name)));
    };
  }
  string text;
  string key;
  if (!qsarTable(calcs, groups, text, why, progress, &key)) return false;
  vector<vector<string>> all = parseCsv(text);
  if (all.empty()) { why = "The table is empty."; return false; }
  p_columnText.clear();
  for (const auto& row : parseCsv(key))
    if (row.size() == 2) p_columnText.push_back(row[1]);
  if (p_columnText.size() != all[0].size()) {
    why = "Descriptor key does not match the table columns.";
    return false;
  }
  p_calcs = calcs;
  p_groups = groups;
  p_csvText = text;
  p_head = all[0];
  p_rows.assign(all.begin() + 1, all.end());
  p_table = (fs::path(p_dir) / "table.csv").string();
  std::ofstream(p_table) << text;
  p_measured.clear();
  int st = 0;
  const string cols = run("columns " + quote(p_table), st);
  if (st == 0) {
    std::istringstream is(cols);
    for (string l; std::getline(is, l); ) if (!l.empty()) p_measured.push_back(l);
  }
  string path = calcs.empty() ? "" : calcs.front().getPath();
  while (path.size() > 1 && path.back() == '/') path.pop_back();
  const size_t cut = path.rfind('/');
  p_folder = cut == string::npos || cut == 0 ? path : path.substr(0, cut);
  const size_t nm = p_folder.rfind('/');
  p_name = nm == string::npos ? p_folder : p_folder.substr(nm + 1);
  return true;
}

QsarWindow::QsarWindow(wxWindow *parent, const vector<EcceURL>& calcs, const string& groups,
                       std::function<vector<EcceURL>()> chooseFolder,
                       std::function<void(QsarWindow *)> onDestroy)
  : wxFrame(parent, wxID_ANY, "QSAR", wxDefaultPosition, wxSize(1100, 700)),
    p_choose(chooseFolder), p_onDestroy(onDestroy)
{
  static int serial = 0;
  std::error_code ec;
  p_dir = (fs::temp_directory_path() / ("ecce-qsar-" +
      std::to_string((long)fs::file_time_type::clock::now().time_since_epoch().count()) +
      "-" + std::to_string(serial++))).string();
  fs::create_directories(p_dir, ec);
  if (!load(calcs, groups, p_why)) return;
  p_ok = true;
  build();
  refreshHeader();
  fillTable();
}

QsarWindow::~QsarWindow()
{
  if (p_onDestroy) p_onDestroy(this);
  std::error_code ec;
  if (!p_dir.empty()) fs::remove_all(p_dir, ec);
}

void QsarWindow::refreshHeader()
{
  SetTitle(wxString::Format(wxString::FromUTF8("QSAR \xE2\x80\x93 %s (%zu calculations)"), U(p_name),
                            p_calcs.size()));
  p_header->SetLabel(wxString::Format("Data: %s, %zu calculations", U(p_folder), p_calcs.size()));
  p_plsY->Clear();
  for (const string& y : p_measured) p_plsY->Append(U(niceHeader(y)));
  if (p_plsY->GetCount()) p_plsY->SetSelection(0);
  p_plsRun->Enable(p_plsY->GetCount() > 0);
  updateCircular();
}

void QsarWindow::fillTable()
{
  if (p_tableGrid->GetNumberRows()) p_tableGrid->DeleteRows(0, p_tableGrid->GetNumberRows());
  if (p_tableGrid->GetNumberCols()) p_tableGrid->DeleteCols(0, p_tableGrid->GetNumberCols());
  p_tableGrid->AppendCols((int)p_head.size());
  p_tableGrid->AppendRows((int)p_rows.size());
  for (size_t j = 0; j < p_head.size(); j++) p_tableGrid->SetColLabelValue((int)j, U(niceHeader(p_head[j])));
  for (size_t i = 0; i < p_rows.size(); i++)
    for (size_t j = 0; j < p_head.size() && j < p_rows[i].size(); j++)
      p_tableGrid->SetCellValue((int)i, (int)j, U(p_rows[i][j]));
  p_tableGrid->AutoSizeColumns();
}

string QsarWindow::keyText() const
{
  string text;
  for (size_t i = 0; i < p_head.size() && i < p_columnText.size(); ++i)
    text += p_head[i] + ": " + p_columnText[i] + "\n\n";
  return text;
}

wxDialog *QsarWindow::createKey()
{
  auto *dlg = new wxDialog(this, wxID_ANY, "QSAR descriptor key", wxDefaultPosition,
                           wxSize(820, 520), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
  auto *box = new wxTextCtrl(dlg, wxID_ANY, U(keyText()), wxDefaultPosition, wxDefaultSize,
                             wxTE_MULTILINE | wxTE_READONLY);
  auto *top = new wxBoxSizer(wxVERTICAL);
  top->Add(box, 1, wxEXPAND | wxALL, 8);
  auto *buttons = new wxBoxSizer(wxHORIZONTAL);
  auto *copy = new wxButton(dlg, wxID_COPY, "Copy All");
  buttons->Add(copy, 0, wxALL, 5);
  buttons->AddStretchSpacer();
  auto *close = new wxButton(dlg, wxID_CLOSE, "Close");
  buttons->Add(close, 0, wxALL, 5);
  top->Add(buttons, 0, wxEXPAND);
  dlg->SetSizer(top);
  copy->Bind(wxEVT_BUTTON, [box](wxCommandEvent&) {
    if (wxTheClipboard->Open()) {
      wxTheClipboard->SetData(new wxTextDataObject(box->GetValue()));
      wxTheClipboard->Close();
    }
  });
  close->Bind(wxEVT_BUTTON, [dlg](wxCommandEvent&) { dlg->EndModal(wxID_CLOSE); });
  return dlg;
}

// Test hook: drives the header motion handler of the column and returns its tooltip.
string QsarWindow::headerTooltip(const string& name)
{
  const auto at = std::find(p_head.begin(), p_head.end(), name);
  if (!p_tableGrid || at == p_head.end()) return "";
  const int col = (int)(at - p_head.begin());
  if (p_tableGrid->GetNumberRows()) p_tableGrid->MakeCellVisible(0, col);
  int x = 2;
  for (int i = 0; i < col; ++i) x += p_tableGrid->GetColSize(i);
  const wxPoint point = p_tableGrid->CalcScrolledPosition(wxPoint(x, 2));
  wxMouseEvent event(wxEVT_MOTION);
  event.SetPosition(wxPoint(point.x, 2));
  wxWindow *labels = p_tableGrid->GetGridColLabelWindow();
  event.SetEventObject(labels);
  labels->GetEventHandler()->ProcessEvent(event);
  return labels->GetToolTipText().ToStdString(wxConvUTF8);
}

void QsarWindow::setStatus(const string& s) { if (p_status) p_status->SetLabel(U(s)); }

string QsarWindow::status() const { return p_plsStatus.empty() ? p_pcaStatus : p_plsStatus; }

void QsarWindow::showTab(int n) { if (p_book) p_book->SetSelection(n); }

bool QsarWindow::setGroups(const string& groups, string& why)
{
  if (!load(p_calcs, groups, why)) return false;
  p_summary.clear(); p_pcaStatus.clear(); p_plsStatus.clear();
  if (p_book) { refreshHeader(); fillTable(); }
  return true;
}

// A descriptor that models the same property as the one chosen makes the PLS circular.
void QsarWindow::updateCircular()
{
  p_circular.clear();
  string y = p_plsY ? p_plsY->GetStringSelection().ToStdString(wxConvUTF8) : "";
  std::transform(y.begin(), y.end(), y.begin(), ::tolower);
  struct Pair { const char *key, *descriptor, *what; };
  static const Pair pairs[] = {{"logp", "SlogP", "logP"}, {"refract", "SMR", "molar refractivity"},
                               {"tpsa", "TPSA", "polar surface area"}};
  for (const Pair& p : pairs)
    if (y.find(p.key) != string::npos &&
        std::find(p_head.begin(), p_head.end(), p.descriptor) != p_head.end()) {
      p_circular = p.descriptor;
      p_plsWarn->SetLabel(U(string(p.descriptor) + " is itself a model estimate of " + p.what +
          ", so a model of " + p_plsY->GetStringSelection().ToStdString(wxConvUTF8) +
          " that uses it is partly circular."));
      p_plsWarn->Wrap(560);
      p_plsLeave->SetLabel(U(string("Leave out ") + p.descriptor));
      break;
    }
  p_plsWarn->Show(!p_circular.empty());
  p_plsLeave->Show(!p_circular.empty());
  p_plsLeave->SetValue(true);
  if (p_book) p_book->Layout();
}

void QsarWindow::build()
{
  wxPanel *root = new wxPanel(this);
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *hr = new wxBoxSizer(wxHORIZONTAL);
  p_header = new wxStaticText(root, wxID_ANY, "");
  hr->Add(p_header, 1, wxALIGN_CENTER_VERTICAL | wxALL, 6);
  wxButton *change = new wxButton(root, wxID_ANY, "Change folder...");
  change->SetToolTip("Use the folder (or calculations) now selected in the Organizer.");
  hr->Add(change, 0, wxALL, 4);
  top->Add(hr, 0, wxEXPAND);
  p_book = new wxNotebook(root, wxID_ANY);
  top->Add(p_book, 1, wxEXPAND | wxLEFT | wxRIGHT, 4);
  p_status = new wxStaticText(root, wxID_ANY, "Choose a tab and run the analysis.");
  top->Add(p_status, 0, wxEXPAND | wxALL, 6);
  root->SetSizer(top);

  // Table
  wxPanel *tp = new wxPanel(p_book);
  wxBoxSizer *ts = new wxBoxSizer(wxVERTICAL);
  wxStaticBoxSizer *gb = new wxStaticBoxSizer(wxHORIZONTAL, tp, "Descriptors to include");
  static const struct { const char *name, *label, *tip; } groupDefs[] = {
    {"counts", "Counts", "atoms, bonds, rings, donors/acceptors, rotatable bonds"},
    {"topology", "Topological indices", "Wiener, Zagreb, chi, Kier, Balaban, information content"},
    {"models", "Property models", "SlogP, SMR, TPSA, surface area"},
    {"charges", "Gasteiger charges", "Q_PC+, Q_PC-, extremes"},
    {"vsa", "Surface areas (VSA)", "surface area by charge, SlogP and SMR"},
    {"qm", "From the calculations", "HOMO, LUMO, gap, dipole, charges"}};
  for (const auto& g : groupDefs) {
    wxCheckBox *c = new wxCheckBox(gb->GetStaticBox(), wxID_ANY, wxString(g.label));
    c->SetToolTip(g.tip);
    c->SetClientData((void *)g.name);
    c->SetValue(("," + p_groups + ",").find(string(",") + g.name + ",") != string::npos);
    gb->Add(c, 0, wxALL, 4);
    p_groupBoxes.push_back(c);
    c->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
      string groups;
      for (wxCheckBox *b : p_groupBoxes)
        if (b->GetValue()) groups += (groups.empty() ? "" : ",") + string((const char *)b->GetClientData());
      if (groups.empty()) return;
      string why;
      if (!setGroups(groups, why) && why != "Cancelled.")
        wxMessageBox(U(why), "QSAR", wxOK | wxICON_ERROR, this);
      else setStatus("Table updated: " + std::to_string(p_head.size()) + " columns.");
    });
  }
  ts->Add(gb, 0, wxEXPAND | wxALL, 4);
  p_tableGrid = makeGrid(tp, wxSize(600, 300));
  ts->Add(p_tableGrid, 1, wxEXPAND | wxALL, 4);
  wxButton *exp = new wxButton(tp, wxID_ANY, "Export CSV...");
  wxButton *keyButton = new wxButton(tp, wxID_ANY, "Key...");
  wxBoxSizer *tb = new wxBoxSizer(wxHORIZONTAL);
  tb->Add(exp, 0, wxALL, 4);
  tb->Add(keyButton, 0, wxALL, 4);
  tb->Add(copyButton(tp, p_tableGrid), 0, wxALL, 4);
  ts->Add(tb, 0);
  keyButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    std::unique_ptr<wxDialog> dlg(createKey());
    dlg->ShowModal();
  });
  // Column labels sit in their own window; cell tooltips never reach it.
  wxWindow *labels = p_tableGrid->GetGridColLabelWindow();
  labels->Bind(wxEVT_MOTION, [this, labels](wxMouseEvent& e) {
    const int col = p_tableGrid->XToCol(p_tableGrid->CalcUnscrolledPosition(e.GetPosition()).x);
    const wxString text = col >= 0 && col < (int)p_columnText.size() ? U(p_columnText[col]) : wxString();
    if (text != labels->GetToolTipText()) {
      if (text.empty()) labels->UnsetToolTip(); else labels->SetToolTip(text);
    }
    e.Skip();
  });
  labels->Bind(wxEVT_LEAVE_WINDOW, [labels](wxMouseEvent& e) { labels->UnsetToolTip(); e.Skip(); });
  tp->SetSizer(ts);
  p_book->AddPage(tp, "Table");

  // PCA
  wxPanel *pp = new wxPanel(p_book);
  wxBoxSizer *ps = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *pr = new wxBoxSizer(wxHORIZONTAL);
  pr->Add(new wxStaticText(pp, wxID_ANY, "Scaling:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  p_pcaScale = new wxChoice(pp, wxID_ANY);
  for (const char *s : {"Autoscale", "Pareto", "Centre only"}) p_pcaScale->Append(s);
  p_pcaScale->SetSelection(0);
  pr->Add(p_pcaScale, 0, wxALL, 4);
  pr->Add(new wxStaticText(pp, wxID_ANY, "Components:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  p_pcaComps = new wxChoice(pp, wxID_ANY);
  p_pcaComps->Append("Automatic (cross-validation)");
  for (int i = 1; i <= 10; i++) p_pcaComps->Append(wxString::Format("%d", i));
  p_pcaComps->SetSelection(0);
  pr->Add(p_pcaComps, 0, wxALL, 4);
  wxButton *runPcaButton = new wxButton(pp, wxID_ANY, "Run PCA");
  pr->Add(runPcaButton, 0, wxALL, 4);
  wxButton *savePca = new wxButton(pp, wxID_ANY, "Save Results...");
  pr->Add(savePca, 0, wxALL, 4);
  ps->Add(pr, 0, wxEXPAND);
  wxBoxSizer *pm = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *pl = new wxBoxSizer(wxVERTICAL);
  p_pcaSentence = wrapped(pp, "", 560);
  pl->Add(p_pcaSentence, 0, wxEXPAND | wxALL, 4);
  p_pcaGrid = makeGrid(pp, wxSize(560, 230));
  pl->Add(p_pcaGrid, 0, wxEXPAND | wxALL, 4);
  wxBoxSizer *pc = new wxBoxSizer(wxHORIZONTAL);
  p_pcaConst = new wxStaticText(pp, wxID_ANY, "");
  p_pcaShow = new wxButton(pp, wxID_ANY, "Show...");
  p_pcaShow->Hide();
  pc->Add(p_pcaConst, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  pc->Add(p_pcaShow, 0, wxALL, 2);
  pc->Add(copyButton(pp, p_pcaGrid), 0, wxALL, 2);
  pl->Add(pc, 0, wxEXPAND);
  pl->Add(wrapped(pp,
      "How to read this: each molecule is a point; Component 1 and Component 2 are the two "
      "directions along which the molecules differ most, and the percentages on the axes say "
      "how much of the differences each direction holds. Molecules close together have similar descriptors. "
      "Read the column 'Cumulative variance explained' to see how much the first components "
      "capture together; the cross-validated prediction (Q\xC2\xB2) says how much of it holds up when parts of the "
      "data are left out. Squares outside the limits are unlike the rest.", 560), 0, wxEXPAND | wxALL, 4);
  pm->Add(pl, 1, wxEXPAND);
  p_pcaPlots = new wxNotebook(pp, wxID_ANY);
  p_pcaPlot = new QsarPlot(p_pcaPlots);
  p_pcaScree = new QsarScree(p_pcaPlots);
  p_pcaPlots->AddPage(p_pcaPlot, "Scores");
  p_pcaPlots->AddPage(p_pcaScree, "Variance");
  pm->Add(p_pcaPlots, 1, wxEXPAND | wxALL, 4);
  ps->Add(pm, 1, wxEXPAND);
  pp->SetSizer(ps);
  p_book->AddPage(pp, "PCA");

  // PLS
  wxPanel *lp = new wxPanel(p_book);
  wxBoxSizer *ls = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *lr = new wxBoxSizer(wxHORIZONTAL);
  lr->Add(new wxStaticText(lp, wxID_ANY, "Model:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  p_plsY = new wxChoice(lp, wxID_ANY);
  lr->Add(p_plsY, 0, wxALL, 4);
  lr->Add(new wxStaticText(lp, wxID_ANY, "Scaling:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  p_plsScale = new wxChoice(lp, wxID_ANY);
  for (const char *s : {"Autoscale", "Pareto", "Centre only"}) p_plsScale->Append(s);
  p_plsScale->SetSelection(0);
  lr->Add(p_plsScale, 0, wxALL, 4);
  lr->Add(new wxStaticText(lp, wxID_ANY, "Components:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  p_plsComps = new wxChoice(lp, wxID_ANY);
  p_plsComps->Append("Automatic (cross-validation)");
  for (int i = 1; i <= 10; i++) p_plsComps->Append(wxString::Format("%d", i));
  p_plsComps->SetSelection(0);
  lr->Add(p_plsComps, 0, wxALL, 4);
  ls->Add(lr, 0, wxEXPAND);
  wxBoxSizer *lr2 = new wxBoxSizer(wxHORIZONTAL);
  lr2->Add(new wxStaticText(lp, wxID_ANY, "Test set (% of molecules kept out):"), 0,
           wxALIGN_CENTER_VERTICAL | wxALL, 4);
  p_plsTest = new wxSpinCtrl(lp, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize,
                             wxSP_ARROW_KEYS, 0, 50, 0);
  lr2->Add(p_plsTest, 0, wxALL, 4);
  lr2->Add(new wxStaticText(lp, wxID_ANY, "Permutation runs:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  p_plsPerm = new wxSpinCtrl(lp, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize,
                             wxSP_ARROW_KEYS, 0, 500, 0);
  lr2->Add(p_plsPerm, 0, wxALL, 4);
  p_plsRun = new wxButton(lp, wxID_ANY, "Run PLS");
  lr2->Add(p_plsRun, 0, wxALL, 4);
  wxButton *savePls = new wxButton(lp, wxID_ANY, "Save Results...");
  lr2->Add(savePls, 0, wxALL, 4);
  ls->Add(lr2, 0, wxEXPAND);
  p_plsWarn = new wxStaticText(lp, wxID_ANY, "");
  p_plsWarn->SetForegroundColour(wxColour(150, 0, 0));
  p_plsLeave = new wxCheckBox(lp, wxID_ANY, "Leave out");
  ls->Add(p_plsWarn, 0, wxLEFT | wxRIGHT | wxTOP, 6);
  ls->Add(p_plsLeave, 0, wxLEFT | wxBOTTOM, 6);
  wxBoxSizer *lm = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *ll = new wxBoxSizer(wxVERTICAL);
  p_plsSentence = wrapped(lp, "", 560);
  ll->Add(p_plsSentence, 0, wxEXPAND | wxALL, 4);
  p_plsGrid = makeGrid(lp, wxSize(560, 140));
  ll->Add(p_plsGrid, 0, wxEXPAND | wxALL, 4);
  wxBoxSizer *vl = new wxBoxSizer(wxHORIZONTAL);
  vl->Add(new wxStaticText(lp, wxID_ANY, "Descriptors by importance (VIP, variable importance in projection)"),
          0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
  ll->Add(vl, 0);
  p_vipGrid = makeGrid(lp, wxSize(560, 130));
  vl->Add(copyButton(lp, p_vipGrid), 0, wxLEFT, 8);
  ll->Add(p_vipGrid, 0, wxEXPAND | wxALL, 4);
  wxBoxSizer *lc = new wxBoxSizer(wxHORIZONTAL);
  p_plsConst = new wxStaticText(lp, wxID_ANY, "");
  p_plsShow = new wxButton(lp, wxID_ANY, "Show...");
  p_plsShow->Hide();
  lc->Add(p_plsConst, 0, wxALIGN_CENTER_VERTICAL | wxALL, 4);
  lc->Add(p_plsShow, 0, wxALL, 2);
  lc->Add(copyButton(lp, p_plsGrid), 0, wxALL, 2);
  ll->Add(lc, 0, wxEXPAND);
  ll->Add(wrapped(lp,
      "How to read this: the plot compares what was measured with what the model gives. "
      "The variance of the property explained (R\xC2\xB2Y) is how well the model reproduces the molecules "
      "it was built on; the cross-validated prediction (Q\xC2\xB2) is how well it predicts molecules left "
      "out, and is the number to judge the model by (close to 1 is good, near 0 or below is no "
      "better than the average). Importance (VIP) says how much a descriptor matters: above 1 is "
      "more than average.", 560), 0, wxEXPAND | wxALL, 4);
  lm->Add(ll, 1, wxEXPAND);
  p_plsPlots = new wxNotebook(lp, wxID_ANY);
  p_plsPlot = new QsarPlot(p_plsPlots);
  p_plsScree = new QsarScree(p_plsPlots);
  p_plsPlots->AddPage(p_plsPlot, "Predicted vs observed");
  p_plsPlots->AddPage(p_plsScree, "Variance");
  lm->Add(p_plsPlots, 1, wxEXPAND | wxALL, 4);
  ls->Add(lm, 1, wxEXPAND);
  lp->SetSizer(ls);
  p_book->AddPage(lp, "PLS");

  wxBoxSizer *outer = new wxBoxSizer(wxVERTICAL);
  outer->Add(root, 1, wxEXPAND);
  SetSizer(outer);

  p_pcaScale->SetToolTip(SCALING_TIPS[0]);
  p_plsScale->SetToolTip(SCALING_TIPS[0]);
  p_pcaScale->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
    p_pcaScale->SetToolTip(SCALING_TIPS[std::max(0, p_pcaScale->GetSelection())]); });
  p_plsScale->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
    p_plsScale->SetToolTip(SCALING_TIPS[std::max(0, p_plsScale->GetSelection())]); });
  p_plsY->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { updateCircular(); });
  for (QsarPlot *plot : {p_pcaPlot, p_plsPlot})
    plot->onPick = [this](const string& name) { pickRow(name); };

  change->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    vector<EcceURL> c = p_choose ? p_choose() : vector<EcceURL>();
    if (c.empty()) {
      wxMessageBox("Select the project folder (or calculations) in the Organizer first.", "QSAR",
                   wxOK | wxICON_INFORMATION, this);
      return;
    }
    string why;
    if (!load(c, p_groups, why)) {
      if (why != "Cancelled.") wxMessageBox(U(why), "QSAR", wxOK | wxICON_ERROR, this);
      return;
    }
    p_summary.clear();
    refreshHeader();
    fillTable();
    setStatus("Folder changed.");
  });
  exp->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog dlg(this, "Write the QSAR table to", "", "qsar.csv", "CSV (*.csv)|*.csv",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK) std::ofstream(dlg.GetPath().ToStdString(wxConvUTF8)) << p_csvText;
  });
  runPcaButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    string why;
    if (!this->runPca(scalingToken(p_pcaScale->GetSelection()), p_pcaComps->GetSelection(), why))
      wxMessageBox(U(why), "QSAR", wxOK | wxICON_ERROR, this);
  });
  p_plsRun->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    string why;
    const int k = p_plsY->GetSelection();
    vector<string> ex;
    if (!p_circular.empty() && p_plsLeave->GetValue()) ex.push_back(p_circular);
    if (k < 0 || !runPls(p_measured[k], scalingToken(p_plsScale->GetSelection()),
                         p_plsComps->GetSelection(), p_plsTest->GetValue(), p_plsPerm->GetValue(),
                         why, ex))
      wxMessageBox(U(why), "QSAR", wxOK | wxICON_ERROR, this);
  });
  for (wxButton *b : {savePca, savePls})
    b->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
      wxFileDialog dlg(this, "Write the results to (a file name inside the folder to use)", "",
                       "results", "*", wxFD_SAVE);
      if (dlg.ShowModal() != wxID_OK) return;
      string why;
      if (!saveResults(dlg.GetPath().ToStdString(wxConvUTF8), why))
        wxMessageBox(U(why), "QSAR", wxOK | wxICON_ERROR, this);
    });
  p_pcaShow->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { showNotes(); });
  p_plsShow->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { showNotes(); });
}

void QsarWindow::showNotes()
{
  string t;
  for (const string& n : p_notes) t += n + "\n";
  wxMessageBox(U(t.empty() ? "Nothing was left out." : t), "Descriptors left out", wxOK, this);
}

// The "note:" lines of the report; constant descriptors counted for the one-line message.
static void takeNotes(const string& summary, vector<string>& notes, int& constant)
{
  notes.clear();
  constant = 0;
  std::istringstream is(summary);
  for (string l; std::getline(is, l); )
    if (l.compare(0, 6, "note: ") == 0) {
      notes.push_back(l.substr(6));
      if (l.find("constant") != string::npos) constant++;
    }
}

bool QsarWindow::runPca(const string& scaling, int components, string& why)
{
  if (!p_ok) { why = p_why; return false; }
  string args = "pca " + quote(p_table) + " --scale " + scaling + " --outdir " + quote(p_dir);
  if (components > 0) args += " --components " + std::to_string(components);
  for (const string& y : p_measured) args += " --y " + quote(y);
  int st = 0;
  p_summary = run(args, st);
  if (st != 0) { why = failure("ecce-qsar pca failed"); return false; }
  vector<vector<string>> sc = parseCsv(slurp(fs::path(p_dir) / "pca_scores.csv"));
  vector<vector<string>> comp = parseCsv(slurp(fs::path(p_dir) / "pca_components.csv"));
  const size_t A = sc.empty() ? 0 : sc[0].size() - 3;
  double t2lim = 1e300, dmlim = 1e300;
  { const size_t a = p_summary.find("T2 limit (95%) ");
    if (a != string::npos) sscanf(p_summary.c_str() + a, "T2 limit (95%%) %lf, DModX limit (95%%) %lf", &t2lim, &dmlim); }
  vector<string> names, tips;
  vector<double> x, y;
  vector<int> flag;
  for (size_t i = 1; i < sc.size(); i++) {
    if (sc[i].size() < 5 || A < 2) continue;
    names.push_back(sc[i][0]);
    tips.push_back(sc[i][0] + "\nComponent 1: " + sc[i][1] + "\nComponent 2: " + sc[i][2] +
                   "\nT\xC2\xB2: " + sc[i][A + 1] + "\nDModX: " + sc[i][A + 2]);
    x.push_back(atof(sc[i][1].c_str()));
    y.push_back(atof(sc[i][2].c_str()));
    flag.push_back(atof(sc[i][A + 1].c_str()) > t2lim || atof(sc[i][A + 2].c_str()) > dmlim ? 1 : 0);
  }
  vector<vector<string>> rows;
  double cum = 0, qcum = 0;
  for (size_t i = 1; i < comp.size(); i++) {
    if (comp[i].size() < 6) continue;
    rows.push_back({comp[i][0], num3(comp[i][1]), percent(atof(comp[i][2].c_str())),
                    percent(atof(comp[i][3].c_str())), num3(comp[i][4]), num3(comp[i][5])});
    if (i == A) { cum = atof(comp[i][3].c_str()); qcum = atof(comp[i][5].c_str()); }
  }
  fillGrid(p_pcaGrid, {"Component", "Eigenvalue", "Variance explained by this component (%)",
                       "Cumulative variance explained (%)",
                       "Cross-validated prediction, this component (Q\xC2\xB2)",
                       "Cumulative cross-validated prediction (Q\xC2\xB2)"},
           rows, A, 3);
  char b[160];
  snprintf(b, sizeof b, "The first %zu component%s explain%s %s %% of the variation between the molecules.",
           A, A == 1 ? "" : "s", A == 1 ? "s" : "", percent(cum).c_str());
  p_pcaSentence->SetLabel(U(b));
  p_pcaSentence->Wrap(560);
  auto share = [&](size_t k) { return k < comp.size() && comp[k].size() > 2 ? atof(comp[k][2].c_str()) : 0.0; };
  p_pcaPlot->set("Molecules by their first two components",
                 "Component 1 (" + percent(share(1)) + " % of the variance)",
                 "Component 2 (" + percent(share(2)) + " % of the variance)", names, x, y, flag,
                 false, tips);
  fillScree(p_pcaScree, comp, false);
  int constant = 0;
  takeNotes(p_summary, p_notes, constant);
  p_pcaConst->SetLabel(constant ? wxString::Format("%d descriptors are constant across these molecules "
                                                   "and were left out", constant) : wxString());
  p_pcaShow->Show(!p_notes.empty());
  p_pcaStatus = string("PCA done: ") + std::to_string(A) + " components explain " + percent(cum) +
                " % of the variation (cross-validated Q\xC2\xB2 " + num3(std::to_string(qcum)) + ")";
  setStatus(p_pcaStatus);
  p_plsStatus.clear();
  p_book->Layout();
  return true;
}

bool QsarWindow::runPls(const string& y, const string& scaling, int components, int testPercent,
                        int permutations, string& why, const vector<string>& exclude)
{
  if (!p_ok) { why = p_why; return false; }
  string args = "pls " + quote(p_table) + " --y " + quote(y) + " --scale " + scaling +
                " --outdir " + quote(p_dir) + " --save-model " + quote(p_dir + "/pls_model.json");
  if (components > 0) args += " --components " + std::to_string(components);
  if (testPercent > 0) args += " --test-fraction " + std::to_string(testPercent / 100.0);
  if (permutations > 0) args += " --permutations " + std::to_string(permutations);
  for (const string& e : exclude) args += " --exclude " + quote(e);
  int st = 0;
  p_summary = run(args, st);
  if (st != 0) { why = failure("ecce-qsar pls failed"); return false; }
  const string label = niceHeader(y);
  vector<vector<string>> pr = parseCsv(slurp(fs::path(p_dir) / "pls_predictions.csv"));
  vector<string> names, tips;
  vector<double> obs, pred;
  vector<int> flag;
  for (size_t i = 1; i < pr.size(); i++) {
    if (pr[i].size() < 4) continue;
    names.push_back(pr[i][0]);
    char res[40];
    snprintf(res, sizeof res, "%.6g", atof(pr[i][2].c_str()) - atof(pr[i][3].c_str()));
    tips.push_back(pr[i][0] + "\nObserved " + y + ": " + pr[i][2] + "\nPredicted " + y + ": " +
                   pr[i][3] + "\nResidual (observed - predicted): " + res +
                   (pr[i][1] == "test" ? "\nTest set" : "\nFitted"));
    flag.push_back(pr[i][1] == "test" ? 2 : 0);
    obs.push_back(atof(pr[i][2].c_str()));
    pred.push_back(atof(pr[i][3].c_str()));
  }
  p_plsPlot->set("PLS of " + label + ": predicted vs observed", "observed " + label,
                 "predicted " + label, names, obs, pred, flag, true, tips);
  vector<vector<string>> comp = parseCsv(slurp(fs::path(p_dir) / "pls_components.csv"));
  fillScree(p_plsScree, comp, true);
  const vector<vector<string>> sc = parseCsv(slurp(fs::path(p_dir) / "pls_scores.csv"));
  const size_t A = sc.empty() ? 0 : sc[0].size() - 3;
  vector<vector<string>> rows;
  for (size_t i = 1; i < comp.size(); i++) {
    if (comp[i].size() < 8) continue;
    rows.push_back({comp[i][0], percent(atof(comp[i][1].c_str())), percent(atof(comp[i][2].c_str())),
                    percent(atof(comp[i][3].c_str())), percent(atof(comp[i][4].c_str())),
                    num3(comp[i][5]), num3(comp[i][6]), num3(comp[i][7])});
  }
  fillGrid(p_plsGrid, {"Component", "Descriptor variance explained by this component (%)",
                       "Cumulative descriptor variance explained (%)",
                       "Property variance explained by this component (%)",
                       "Cumulative property variance explained (%)",
                       "Cross-validated prediction, this component (Q\xC2\xB2)",
                       "Cumulative cross-validated prediction (Q\xC2\xB2)",
                       "Prediction error on left-out molecules (RMSECV)"}, rows, A, 6);
  // VIP, largest first.
  vector<vector<string>> vip = parseCsv(slurp(fs::path(p_dir) / "pls_vip.csv"));
  vector<vector<string>> vrows;
  for (size_t i = 1; i < vip.size(); i++) if (vip[i].size() >= 2) vrows.push_back({vip[i][0], vip[i][1]});
  std::stable_sort(vrows.begin(), vrows.end(), [](const vector<string>& a, const vector<string>& b) {
    return atof(a[1].c_str()) > atof(b[1].c_str()); });
  for (auto& r : vrows) r[1] = num3(r[1]);
  fillGrid(p_vipGrid, {"Descriptor", "Importance in the model (VIP)"}, vrows, 0, 1);
  double r2y = 0, q2 = 0, rmsee = 0, rmsecv = 0;
  const size_t a = p_summary.find("R2Y ");
  if (a != string::npos)
    sscanf(p_summary.c_str() + a, "R2Y %lf  Q2 %lf  RMSEE %lf  RMSECV %lf", &r2y, &q2, &rmsee, &rmsecv);
  char b[320];
  snprintf(b, sizeof b, "The model of %s uses %zu component%s. It reproduces %s %% of the variation in "
           "%s (R\xC2\xB2Y) and predicts left-out molecules with Q\xC2\xB2 = %.2f; judge it by Q\xC2\xB2.",
           label.c_str(), A, A == 1 ? "" : "s", percent(r2y).c_str(), label.c_str(), q2);
  p_plsSentence->SetLabel(U(b));
  p_plsSentence->Wrap(560);
  int constant = 0;
  takeNotes(p_summary, p_notes, constant);
  p_plsConst->SetLabel(constant ? wxString::Format("%d descriptors are constant across these molecules "
                                                   "and were left out", constant) : wxString());
  p_plsShow->Show(!p_notes.empty());
  snprintf(b, sizeof b, "PLS of %s done: property variance explained (R\xC2\xB2Y) %.2f, cross-validated prediction (Q\xC2\xB2) %.2f, prediction error (RMSECV) %.2f", label.c_str(), r2y, q2, rmsecv);
  p_plsStatus = b;
  setStatus(p_plsStatus);
  p_book->Layout();
  return true;
}

bool QsarWindow::saveResults(const string& dir, string& why) const
{
  std::error_code ec;
  fs::create_directories(dir, ec);
  for (const char *f : {"table.csv", "pca_scores.csv", "pca_loadings.csv", "pca_components.csv",
                        "pls_components.csv", "pls_scores.csv", "pls_vip.csv",
                        "pls_predictions.csv", "pls_permutation.csv", "pls_model.json",
                        "analysis_components.csv", "analysis_vip.csv", "analysis_summary.txt"}) {
    const fs::path src = fs::path(p_dir) / f;
    if (!fs::exists(src)) continue;
    fs::copy_file(src, fs::path(dir) / f, fs::copy_options::overwrite_existing, ec);
    if (ec) { why = string("cannot write ") + f + ": " + ec.message(); return false; }
  }
  return true;
}

// Component statistics of ecce-qsar's pca/pls_components.csv as the Variance plot's series.
void QsarWindow::fillScree(QsarScree *plot, const vector<vector<string>>& comp, bool pls)
{
  const int series = pls ? 3 : 1;
  vector<vector<double>> bars(series), lines(series);
  for (size_t i = 1; i < comp.size() && i <= 10; i++)
    for (int j = 0; j < series; j++) {
      const size_t col = pls ? 1 + 2 * j : 2;
      if (col + 1 >= comp[i].size()) continue;
      bars[j].push_back(comp[i][col].empty() ? NAN : atof(comp[i][col].c_str()));
      lines[j].push_back(comp[i][col + 1].empty() ? NAN : atof(comp[i][col + 1].c_str()));
    }
  plot->set(bars, lines);
}

// A clicked plot point selects its molecule's row in the Table tab.
void QsarWindow::pickRow(const string& name)
{
  for (size_t r = 0; r < p_rows.size(); r++)
    if (!p_rows[r].empty() && p_rows[r][0] == name) {
      p_tableGrid->ClearSelection();
      p_tableGrid->SetGridCursor((int)r, 0);
      p_tableGrid->SelectRow((int)r);
      p_tableGrid->MakeCellVisible((int)r, 0);
      setStatus(name + " is selected in the Table tab.");
      return;
    }
}

// 0: the result plot of the tab last run, 1: its Variance plot, 2: the Table tab.
void QsarWindow::selectPresentationPage(int page)
{
  const bool pls = !p_plsStatus.empty();
  showTab(page == 2 ? 0 : pls ? 2 : 1);
  (pls ? p_plsPlots : p_pcaPlots)->SetSelection(page == 1 ? 1 : 0);
  Layout();
  p_book->Layout();
}

// Test hook: drives the real copy, Ctrl+C, hover and click paths of the last run's tab and
// records the outcome and plot axis labels in `dir`.
bool QsarWindow::verifyPresentation(const string& dir, string& why)
{
  const bool pls = !p_plsStatus.empty();
  wxGrid *grid = pls ? p_plsGrid : p_pcaGrid;
  QsarPlot *plot = pls ? p_plsPlot : p_pcaPlot;
  QsarScree *scree = pls ? p_plsScree : p_pcaScree;
  if (!grid->GetNumberRows() || !plot->count()) { why = "no presentation data"; return false; }
  selectPresentationPage(0);
  const bool copied = copyGrid(grid, true);
  const wxString whole = clipboardText();
  const bool wholeOk = copied && whole == gridText(grid, true);
  std::ofstream(fs::path(dir) / "presentation.tsv") << whole.ToStdString(wxConvUTF8);
  grid->ClearSelection();
  grid->SelectBlock(0, 0, 0, std::min(1, grid->GetNumberCols() - 1));
  wxKeyEvent key(wxEVT_CHAR_HOOK);
  key.m_keyCode = 'C';
  key.SetControlDown(true);
  grid->GetEventHandler()->ProcessEvent(key);
  const wxString block = clipboardText();
  const bool selection = !block.empty() && block == gridText(grid, false);
  std::ofstream(fs::path(dir) / "selection.tsv") << block.ToStdString(wxConvUTF8);
  grid->ClearSelection();
  plot->Refresh(); plot->Update();
  std::ofstream hoverFile(fs::path(dir) / "hover.txt");
  for (size_t i = 0; i < plot->count(); i++) hoverFile << plot->tip(i) << "\n\n";
  hoverFile.close();
  const wxPoint pos = plot->position(0);
  const int at = plot->hit(pos);
  const bool hoverOk = at >= 0 && plot->hover(pos) == plot->tip(at);
  p_tableGrid->ClearSelection();
  const int clicked = plot->click(pos);
  bool clickOk = false;
  for (size_t r = 0; clicked >= 0 && r < p_rows.size(); r++)
    if (p_tableGrid->IsInSelection((int)r, 0) && !p_rows[r].empty() &&
        plot->tip(clicked).compare(0, p_rows[r][0].size(), p_rows[r][0]) == 0)
      clickOk = true;
  selectPresentationPage(1);
  scree->Refresh(); scree->Update();
  selectPresentationPage(0);
  auto verdict = [](bool pass) { return pass ? "PASS" : "FAIL"; };
  std::ofstream(fs::path(dir) / "presentation-check.txt") <<
    "copy=" << verdict(wholeOk) << "\nselection=" << verdict(selection) <<
    "\nhover=" << verdict(hoverOk) << "\nclick=" << verdict(clickOk) <<
    "\nplot-x=" << plot->xLabels << "\nplot-y=" << plot->yLabels <<
    "\nscree-y=" << scree->yLabels << "\n";
  if (!(wholeOk && selection && hoverOk && clickOk)) {
    why = "presentation clipboard, hover or click check failed";
    return false;
  }
  return true;
}

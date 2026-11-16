#include "wxgui/DiffractionDialog.H"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <locale>
#include <sstream>

#include <algorithm>
#include <memory>

#include <wx/dcbuffer.h>
#include <wx/display.h>
#include <wx/scrolwin.h>
#include <wx/dcgraph.h>
#include <wx/dcmemory.h>
#include <wx/graphics.h>
#include <wx/region.h>
#include <wx/settings.h>
#include <wx/listctrl.h>
#include <wx/filedlg.h>
#include <wx/process.h>
#include <wx/evtloop.h>
#include <wx/tglbtn.h>
#include <wx/wrapsizer.h>
#include <wx/popupwin.h>
#include "wxgui/WindowShot.H"
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/stopwatch.h>
#include <wx/sstream.h>

#include "util/Ecce.H"
#include "util/Filesystem.H"

using std::string;
using std::vector;
namespace fs = ecce::fs;

namespace {
const int PIXELS = 400;
const double POW_L = 64, POW_R = 16;

// Powder is for periodic structures: the Builder (and the results entry)
// write them as an .xyz whose comment line carries Lattice="...".
bool hasLattice(const string& path)
{
  std::ifstream f(path);
  string a, b;
  std::getline(f, a);
  std::getline(f, b);
  return path.size() > 4 && path.compare(path.size() - 4, 4, ".xyz") == 0 &&
         b.find("Lattice=") != string::npos;
}

string drain(wxInputStream *in)
{
  string out;
  char buf[4096];
  while (in && in->CanRead()) {
    in->Read(buf, sizeof buf);
    size_t n = in->LastRead();
    if (!n) break;
    out.append(buf, n);
  }
  return out;
}

string num(double v)
{
  std::ostringstream o;
  o.imbue(std::locale::classic());
  o << v;
  return o.str();
}

double parse(const wxString& t)
{
  std::istringstream i(string(t.ToUTF8().data()));
  i.imbue(std::locale::classic());
  double v = 0;
  i >> v;
  return v;
}

string tool()
{
  const char *home = getenv("ECCE_HOME");
  return home && *home ? string(home) + "/bin/ecce-xrd" : string("ecce-xrd");
}
}  // namespace


// The image views (fibre, frame, layer): the dialog draws everything with
// wxGraphicsContext; the panel only supplies the bitmap and the pointer.
class DiffractionPanel : public wxPanel
{
public:
  DiffractionPanel(wxWindow *parent, DiffractionView *d)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(480, 520),
              wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS), p_d(d)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetToolTip("Wheel: zoom at pointer; drag: zoom rectangle; middle/right or Shift+drag: pan; Home or double click empty space: fit");
    Bind(wxEVT_PAINT, &DiffractionPanel::onPaint, this);
    Bind(wxEVT_MOTION, &DiffractionPanel::onMove, this);
    for (auto type : {wxEVT_LEFT_DOWN, wxEVT_MIDDLE_DOWN, wxEVT_RIGHT_DOWN})
      Bind(type, &DiffractionPanel::onDown, this);
    for (auto type : {wxEVT_LEFT_UP, wxEVT_MIDDLE_UP, wxEVT_RIGHT_UP})
      Bind(type, &DiffractionPanel::onUp, this);
    Bind(wxEVT_MOUSEWHEEL, [this](wxMouseEvent& e) {
      if (e.GetWheelAxis() != wxMOUSE_WHEEL_VERTICAL || !area().Contains(e.GetPosition()) || !e.GetWheelDelta()) return;
      SetFocus();
      zoom(e.GetPosition(), std::pow(0.8, double(e.GetWheelRotation()) / e.GetWheelDelta()));
    });
    Bind(wxEVT_LEFT_DCLICK, [this](wxMouseEvent& e) {
      double x, y;
      if (pixelAt(e.GetPosition(), x, y) &&
          (!(p_d->p_single || p_d->p_frame) || !p_d->spotNear(x, y))) reset();
    });
    Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& e) {
      if (e.GetKeyCode() == WXK_HOME) reset();
      else if (e.GetKeyCode() == WXK_ESCAPE) cancelDrag();
      else e.Skip();
    });
    Bind(wxEVT_MOUSE_CAPTURE_LOST, [this](wxMouseCaptureLostEvent&) {
      p_pressed = p_dragging = false; Refresh();
    });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) {
      if (!p_d->p_process && !p_d->p_debounce.IsRunning() && !p_d->p_dirty) p_d->p_pointer->SetLabel("");
      if (!p_d->p_pinned) p_d->clearTip();
    });
  }
  void setImage(const wxImage& im) { p_bmp = im.IsOk() ? wxBitmap(im) : wxBitmap(); Refresh(); }
  const wxBitmap& bitmap() const { return p_bmp; }
  wxRect area() const { return p_d->imageArea(GetClientSize()); }
  wxRect2DDouble imageRect(const wxSize& size) const
  {
    const wxRect v = p_d->imageArea(size);
    const double w = v.width / p_span;
    return wxRect2DDouble(v.x - p_x * w, v.y - p_y * w, w, w);
  }
  bool pixelAt(const wxPoint& p, double& x, double& y) const
  {
    if (!area().Contains(p)) return false;
    const wxRect2DDouble r = imageRect(GetClientSize());
    const int n = p_d->imagePixels();
    x = (p.x - r.m_x) * n / r.m_width;
    y = (p.y - r.m_y) * n / r.m_height;
    return x >= 0 && y >= 0 && x < n && y < n;
  }
  wxPoint position(double x, double y) const
  {
    const wxRect2DDouble r = imageRect(GetClientSize());
    return wxPoint(std::lround(r.m_x + x * r.m_width / p_d->imagePixels()),
                   std::lround(r.m_y + y * r.m_height / p_d->imagePixels()));
  }
  void reset() { cancelDrag(); p_x = p_y = 0; p_span = 1; Refresh(); }
  void zoom(const wxPoint& p, double factor)
  {
    const wxRect v = area();
    const double span = std::clamp(p_span * factor, 1.0 / 32, 1.0);
    p_x += (p.x - v.x) / double(v.width) * (p_span - span);
    p_y += (p.y - v.y) / double(v.height) * (p_span - span);
    p_span = span;
    clamp(); Refresh();
  }

private:
  void clamp()
  {
    p_x = std::clamp(p_x, 0.0, 1 - p_span);
    p_y = std::clamp(p_y, 0.0, 1 - p_span);
  }
  void cancelDrag()
  {
    p_pressed = p_dragging = false;
    if (HasCapture()) ReleaseMouse();
    Refresh();
  }
  void onDown(wxMouseEvent& e)
  {
    if (!area().Contains(e.GetPosition())) return;
    SetFocus();
    p_start = p_last = e.GetPosition();
    p_pan = !e.LeftDown() || e.ShiftDown();
    p_second = e.ShiftDown();
    p_pressed = true; p_dragging = false;
    if (!HasCapture()) CaptureMouse();
  }
  void onUp(wxMouseEvent& e)
  {
    if (!p_pressed) return;
    const bool dragged = p_dragging;
    cancelDrag();
    if (dragged && !p_pan) {
      const wxRect v = area();
      const wxPoint b(std::clamp(e.GetX(), v.x, v.GetRight()),
                      std::clamp(e.GetY(), v.y, v.GetBottom()));
      // Keep the film square so reciprocal distances and ring shapes agree.
      const int side = std::max(std::abs(b.x - p_start.x), std::abs(b.y - p_start.y));
      if (std::abs(b.x - p_start.x) < 6 || std::abs(b.y - p_start.y) < 6) return;
      p_x += (std::min(b.x, p_start.x) - v.x) * p_span / v.width;
      p_y += (std::min(b.y, p_start.y) - v.y) * p_span / v.height;
      p_span = std::max(1.0 / 32, p_span * side / v.width);
      clamp(); Refresh();
    } else if (!dragged && e.LeftUp()) {
      double x, y;
      if (pixelAt(e.GetPosition(), x, y)) {
        p_d->onImageClick(x, y, p_second);
        wxString tip = p_d->pointerText(x, y);
        if (!p_d->p_measure->GetLabel().empty()) tip += "\n" + p_d->p_measure->GetLabel();
        p_d->showTip(this, e.GetPosition(), tip, true);
      }
    }
  }
  void onPaint(wxPaintEvent&)
  {
    wxAutoBufferedPaintDC dc(this);
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;
    const wxSize s = GetClientSize();
    p_d->paint(gc.get(), s.x, s.y);
    if (p_pressed && p_dragging && !p_pan) {
      gc->SetPen(wxPen(wxColour(30, 100, 200), 1, wxPENSTYLE_SHORT_DASH));
      gc->SetBrush(*wxTRANSPARENT_BRUSH);
      gc->DrawRectangle(std::min(p_start.x, p_last.x), std::min(p_start.y, p_last.y),
                        std::abs(p_last.x - p_start.x), std::abs(p_last.y - p_start.y));
    }
  }
  void onMove(wxMouseEvent& e)
  {
    if (p_pressed) {
      const wxPoint p = e.GetPosition();
      if (std::abs(p.x - p_start.x) > 4 || std::abs(p.y - p_start.y) > 4) p_dragging = true;
      if (p_pan && p_dragging) {
        p_x -= (p.x - p_last.x) * p_span / area().width;
        p_y -= (p.y - p_last.y) * p_span / area().height;
        clamp();
      }
      p_last = p;
      Refresh();
      return;
    }
    double x, y;
    const wxString t = pixelAt(e.GetPosition(), x, y) ? p_d->pointerText(x, y) : wxString();
    if (!p_d->p_process && !p_d->p_debounce.IsRunning()) p_d->p_pointer->SetLabel(t);
    p_d->showTip(this, e.GetPosition(), t);
  }
  DiffractionView *p_d;
  wxBitmap p_bmp;
  double p_x = 0, p_y = 0, p_span = 1;
  bool p_pressed = false, p_dragging = false, p_pan = false, p_second = false;
  wxPoint p_start, p_last;
};


// The powder pattern; the pointer reports the reflection nearest to it.
class PowderPanel : public wxPanel
{
public:
  PowderPanel(wxWindow *parent, DiffractionView *d)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(560, 400)), p_d(d)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &PowderPanel::onPaint, this);
    Bind(wxEVT_MOTION, &PowderPanel::onMove, this);
    Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& e) {
      double tt;
      if (!twoThetaAt(e.GetX(), tt)) return;
      const double tol = 4 * (p_d->p_tt.back() - p_d->p_tt.front()) /
                         std::max(10.0, double(GetClientSize().x - POW_L - POW_R));
      p_d->selectReflection(tt, tol);
      p_d->showTip(this, e.GetPosition(), p_d->cursorText(tt, tol), true);
    });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) {
      if (!p_d->p_process && !p_d->p_debounce.IsRunning() && !p_d->p_dirty) p_d->p_pointer->SetLabel("");
      if (!p_d->p_pinned) p_d->clearTip();
    });
  }
  // 2theta under a window x, false outside the plot.
  bool twoThetaAt(int x, double& tt) const
  {
    if (p_d->p_tt.size() < 2) return false;
    const double l = POW_L, w = std::max(10.0, GetClientSize().x - POW_L - POW_R);
    if (x < l || x > l + w) return false;
    tt = p_d->p_tt.front() + (x - l) * (p_d->p_tt.back() - p_d->p_tt.front()) / w;
    return true;
  }

private:
  void onPaint(wxPaintEvent&)
  {
    wxPaintDC dc(this);
    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    if (!gc) return;
    const wxSize s = GetClientSize();
    p_d->paint(gc.get(), s.x, s.y);
  }
  void onMove(wxMouseEvent& e)
  {
    double tt;
    wxString t;
    if (twoThetaAt(e.GetX(), tt)) {
      const double perPx = (p_d->p_tt.back() - p_d->p_tt.front()) / std::max(10.0, double(GetClientSize().x - POW_L - POW_R));
      t = p_d->cursorText(tt, 4 * perPx);
    }
    if (!p_d->p_process && !p_d->p_debounce.IsRunning()) p_d->p_pointer->SetLabel(t);
    p_d->showTip(this, e.GetPosition(), t);
  }
  DiffractionView *p_d;
};


namespace {
const wxColour CALC(20, 60, 170), MEAS(205, 40, 40), INK(40, 40, 40), GRID(225, 225, 225);

void text(wxGraphicsContext *gc, const wxString& s, double x, double y, const wxColour& c,
          double size = 1.0, bool bold = false, double angle = 0)
{
  wxFont f = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
  f.SetPointSize((int)std::lround(f.GetPointSize() * size));
  if (bold) f.SetWeight(wxFONTWEIGHT_BOLD);
  gc->SetFont(f, c);
  if (angle == 0) gc->DrawText(s, x, y); else gc->DrawText(s, x, y, angle);
}

wxSize extent(wxGraphicsContext *gc, const wxString& s, double size = 1.0)
{
  wxFont f = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
  f.SetPointSize((int)std::lround(f.GetPointSize() * size));
  gc->SetFont(f, INK);
  double w, h;
  gc->GetTextExtent(s, &w, &h);
  return wxSize((int)std::ceil(w), (int)std::ceil(h));
}

wxString U(const char *s) { return wxString::FromUTF8(s); }

wxString hklText(int h, int k, int l)
{
  if (h >= 0 && k >= 0 && l >= 0 && h < 10 && k < 10 && l < 10)
    return wxString::Format("%d%d%d", h, k, l);
  return wxString::Format("%d %d %d", h, k, l);
}
}  // namespace

wxRect DiffractionView::imageArea(const wxSize& s) const
{
  const int top = 30, side = 12, bottom = 26;
  const int side2 = std::max(10, std::min(s.x - 2 * side, s.y - top - bottom));
  return wxRect((s.x - side2) / 2, top, side2, side2);
}

int DiffractionView::imagePixels() const { return p_single || p_frame ? p_layerSize : p_pixels; }

void DiffractionView::paint(wxGraphicsContext *gc, double W, double H) const
{
  gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
  gc->SetBrush(*wxWHITE_BRUSH);
  gc->SetPen(*wxTRANSPARENT_PEN);
  gc->DrawRectangle(0, 0, W, H);
  if (p_powder) paintPowder(gc, W, H);
  else paintImage(gc, W, H);
}

void DiffractionView::paintPowder(wxGraphicsContext *gc, double W, double H) const
{
  const vector<double>& tt = p_tt;
  if (tt.size() < 2) return;
  const bool two = !p_meas.empty() && p_meas.size() == tt.size();
  const double L = POW_L, R = POW_R, T = 16, B = 52, gap = 14;
  const double pw = std::max(10.0, W - L - R);
  const int n = two ? 2 : 1;
  const double ph = std::max(10.0, (H - T - B - (n - 1) * gap) / n);
  const double x0 = tt.front(), x1 = tt.back();
  auto px = [&](double v) { return L + (v - x0) / (x1 - x0) * pw; };
  double step = 1;
  for (double c : {1., 2., 5., 10., 20., 50.}) { step = c; if ((x1 - x0) / c <= 12) break; }
  for (int k = 0; k < n; k++) {
    const double top = T + k * (ph + gap), bot = top + ph;
    auto py = [&](double v) { return bot - std::min(std::max(v, 0.0), 108.0) / 108.0 * ph; };
    gc->SetPen(wxPen(GRID, 1));
    for (int v = 0; v <= 100; v += 25) {
      gc->StrokeLine(L, py(v), L + pw, py(v));
      const wxString s = wxString::Format("%d", v);
      text(gc, s, L - 8 - extent(gc, s).x, py(v) - extent(gc, s).y / 2.0, INK);
    }
    gc->SetPen(wxPen(INK, 1));
    gc->StrokeLine(L, top, L, bot);
    gc->StrokeLine(L, bot, L + pw, bot);
    for (double v = std::ceil(x0 / step) * step; v <= x1 + 1e-9; v += step) {
      gc->StrokeLine(px(v), bot, px(v), bot + 4);
      if (k == n - 1) {
        const wxString s = wxString::Format("%g", v);
        text(gc, s, px(v) - extent(gc, s).x / 2.0, bot + 6, INK);
      }
    }
    const vector<double>& y = (k == 0) ? p_y : p_meas;
    const wxColour col = (k == 0) ? CALC : MEAS;
    wxGraphicsPath path = gc->CreatePath();
    bool open = false;
    for (size_t i = 0; i < y.size() && i < tt.size(); i++) {
      if (y[i] != y[i]) { open = false; continue; }
      if (open) path.AddLineToPoint(px(tt[i]), py(y[i]));
      else { path.MoveToPoint(px(tt[i]), py(y[i])); open = true; }
    }
    gc->SetPen(wxPen(col, 1.4));
    gc->StrokePath(path);
    text(gc, k == 0 ? (p_debye ? "Debye scattering (calculated)" : "calculated") : "measured", L + 8, top + 4, col, 1.0, true);
    if (k == 0) {
      // hkl at the peaks above a threshold, strongest first, none on top of another
      vector<const Line *> order;
      for (const Line& l : p_lines)
        if (!l.a2 && l.rel >= p_plabel->GetValue() && l.rel > 0 && l.tt >= x0 && l.tt <= x1) order.push_back(&l);
      std::sort(order.begin(), order.end(), [](const Line *a, const Line *b) { return a->rel > b->rel; });
      vector<wxRect> placed;
      for (const Line *l : order) {
        double top2 = 0;
        for (size_t i = 0; i < tt.size(); i++)
          if (std::fabs(tt[i] - l->tt) <= 0.3 && y[i] == y[i]) top2 = std::max(top2, y[i]);
        const wxString s = hklText(l->h, l->k, l->l);
        const wxSize e = extent(gc, s, 0.9);
        const wxRect r((int)(px(l->tt) - e.x / 2.0), (int)(py(top2) - e.y - 3), e.x, e.y);
        if (r.y < top) continue;
        bool clash = false;
        for (wxRect q : placed) { q.Inflate(2, 0); clash = clash || q.Intersects(r); }
        if (clash) continue;
        placed.push_back(r);
        text(gc, s, r.x, r.y, INK, 0.9);
      }
    }
  }
  const double bot = T + n * ph + (n - 1) * gap;
  const wxString xt = wxString::FromUTF8("2\xCE\xB8 (\xC2\xB0)    \xCE\xBB = ") +
                      wxString::Format("%.4f ", parse(p_pwave->GetValue())) + wxString::FromUTF8("\xC3\x85");
  text(gc, xt, L + pw / 2 - extent(gc, xt).x / 2.0, bot + 28, INK);
  const wxString yt = "Relative intensity";
  text(gc, yt, 8, T + (bot - T) / 2 + extent(gc, yt).x / 2.0, INK, 1.0, false, M_PI / 2);
}

void DiffractionView::paintImage(wxGraphicsContext *gc, double W, double H) const
{
  const wxRect v = imageArea(wxSize((int)W, (int)H));
  const wxRect2DDouble r = p_panel->imageRect(wxSize((int)W, (int)H));
  struct { double x, y, width, height; } a = {r.m_x, r.m_y, r.m_width, r.m_height};
  const double f = (double)a.width / imagePixels();
  const bool dark = darkScheme();
  wxString title, caption;
  if (p_single) {
    static const char *fmt[] = {"hk%d", "h%dl", "%dkl"};
    title = wxString::Format(fmt[std::max(0, p_zone->GetSelection())], (int)std::lround(number(p_layerNo))) +
            " layer (reciprocal lattice, simulated)";
    caption = "Simulated precession photograph; darkness follows the relative intensity, 100 = the strongest reflection of the whole crystal";
  } else if (p_frame) {
    title = wxString::Format(U("Rotating crystal, φ %d to %g° (simulated)"), p_fphi->GetValue(),
                             p_fphi->GetValue() + number(p_fosc));
    caption = wxString::Format("Detector %.0f mm from the crystal, %s radiation",
                               number(p_fdist), p_source->GetStringSelection());
  } else {
    title = "Fibre diffraction pattern (simulated)";
    caption = wxString::Format("Flat film %g mm square, %g mm from the sample, logarithmic scale",
                               number(p_film), number(p_distance)) + wxString::FromUTF8(", \xCE\xBB = ") +
              wxString::Format("%g ", number(p_wavelength)) + wxString::FromUTF8("\xC3\x85");
  }
  text(gc, title, v.x, 6, INK, 1.1, true);
  gc->PushState();
  gc->Clip(v.x, v.y, v.width, v.height);
  if (p_single) {
    if (!p_layerReady) { gc->PopState(); return; }
    if (p_panel->bitmap().IsOk()) {
      gc->SetInterpolationQuality(wxINTERPOLATION_BEST);
      gc->DrawBitmap(p_panel->bitmap(), a.x, a.y, a.width, a.height);
    }
    const wxColour ink = dark ? wxColour(255, 255, 255) : INK, axis = dark ? wxColour(130, 185, 255) : CALC;
    gc->SetPen(wxPen(wxColour(170, 170, 170), 1));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRectangle(a.x, a.y, a.width, a.height);
    if (p_absent->IsChecked()) {
      gc->SetPen(wxPen(wxColour(150, 150, 150), 1));
      for (const Spot& s : p_spots)
        if (s.absent) gc->DrawEllipse(a.x + s.px * f - 3.5 * f, a.y + s.py * f - 3.5 * f, 7 * f, 7 * f);
    }
    if (p_lhkl->IsChecked()) {
      vector<const Spot *> order;
      for (const Spot& s : p_spots)
        if (!s.absent && s.rel >= 0.5) order.push_back(&s);
      std::sort(order.begin(), order.end(), [](const Spot *x, const Spot *y) { return x->rel > y->rel; });
      vector<wxRect> placed;
      for (const Spot *s : order) {
        const wxString t = hklText(s->h, s->k, s->l);
        const wxSize e = extent(gc, t, 0.8);
        const wxRect r((int)(a.x + s->px * f + 4), (int)(a.y + s->py * f + 3), e.x, e.y);
        if (r.x < v.x || r.y < v.y || r.x + r.width > v.x + v.width || r.y + r.height > v.y + v.height) continue;
        bool clash = false;
        for (wxRect q : placed) { q.Inflate(2, 0); clash = clash || q.Intersects(r); }
        if (clash) continue;
        placed.push_back(r);
        text(gc, t, r.x, r.y, ink, 0.8);
      }
    }
    if (p_laxes->IsChecked()) {
      static const char *names[] = {"a*", "b*", "c*"};
      double longest = 0;
      for (const auto& v : p_axes) longest = std::max(longest, std::hypot(v[0], v[1]));
      const double ox = v.x + 26, oy = v.y + v.height - 26;
      for (int k = 0; k < 3 && longest > 1e-6; k++) {
        const double vx = p_axes[k][0], vy = p_axes[k][1], n = std::hypot(vx, vy);
        if (n < 1e-6) continue;
        const double L = 44.0 * n / longest, ux = vx / n, uy = -vy / n;
        const double ex = ox + ux * L, ey = oy + uy * L;
        gc->SetPen(wxPen(axis, 1.5));
        gc->StrokeLine(ox, oy, ex, ey);
        gc->StrokeLine(ex, ey, ex - 7 * ux + 3 * uy, ey - 7 * uy - 3 * ux);
        gc->StrokeLine(ex, ey, ex - 7 * ux - 3 * uy, ey - 7 * uy + 3 * ux);
        text(gc, names[k], ex + 6 * ux - 6, ey + 6 * uy - 8, axis, 1.0, true);
      }
    }
  } else if (p_panel->bitmap().IsOk()) {
    gc->SetInterpolationQuality(wxINTERPOLATION_BEST);
    gc->DrawBitmap(p_panel->bitmap(), a.x, a.y, a.width, a.height);
    if (p_frame) {
      const wxColour ink = dark ? wxColour(255, 255, 255) : CALC, ring = dark ? wxColour(255, 200, 80) : wxColour(200, 60, 60);
      const double cx = a.x + a.width / 2.0, cy = a.y + a.height / 2.0, mm = p_layerScale * f;
      struct RingLabel { wxString label; double x, y; wxSize size; };
      vector<RingLabel> ringLabels;
      if (p_frings->IsChecked() && p_geomLambda > 0)
        for (double d : {3.0, 2.0, 1.5, 1.0}) {
          const double sn = p_geomLambda / (2 * d);
          if (sn >= 1) continue;
          const double t2 = 2 * std::asin(sn);
          if (t2 >= M_PI / 2) continue;
          const double rad = p_geomDistance * std::tan(t2) * mm;
          gc->SetPen(wxPen(ring, 1));
          gc->SetBrush(*wxTRANSPARENT_BRUSH);
          gc->DrawEllipse(cx - rad, cy - rad, 2 * rad, 2 * rad);
          const wxString s = wxString::Format("d = %.2f ", d) + wxString::FromUTF8("\xC3\x85 (2\xCE\xB8 = ") +
                             wxString::Format("%.1f", t2 * 180 / M_PI) + wxString::FromUTF8("\xC2\xB0)");
          const wxSize e = extent(gc, s, 0.9);
          for (double deg : {50., 130., 20., 160., 80.}) {
            const double x = cx + rad * std::cos(deg * M_PI / 180), y = cy - rad * std::sin(deg * M_PI / 180);
            if (x > v.x + 4 && x + e.x + 6 < v.x + v.width - 4 && y - e.y - 5 > v.y + 4 && y < v.y + v.height - 4) {
              ringLabels.push_back({s, x + 3, y - e.y - 5, e});
              break;
            }
          }
        }
      for (const auto& label : ringLabels) {
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(dark ? *wxBLACK : *wxWHITE));
        gc->DrawRectangle(label.x - 2, label.y - 2, label.size.x + 4, label.size.y + 4);
        text(gc, label.label, label.x, label.y, ring, 0.9);
      }
      if (p_flabel->IsChecked()) {
        vector<const Spot *> order;
        for (const Spot& s : p_spots) order.push_back(&s);
        std::sort(order.begin(), order.end(), [](const Spot *x, const Spot *y) { return x->I > y->I; });
        for (size_t i = 0; i < order.size() && i < 12; i++) {
          const Spot& s = *order[i];
          text(gc, wxString::Format("%d %d %d", s.h, s.k, s.l), a.x + s.px * f + 7, a.y + s.py * f - 16, ink, 0.9);
        }
      }
    }
  }
  // The stop is geometric; repaint its edge at the viewport resolution.
  if (p_panel->bitmap().IsOk()) {
    double radius = 0;
    if (p_single) radius = std::max(5.0, 0.018 * p_layerSize) * f;
    else if (p_frame) radius = 3.0 * p_layerScale * f;
    else {
      const double sn = number(p_wavelength) * 0.012 / 2;
      // Enclose the raster mask's pixel corners with a smooth film edge.
      radius = sn < 1 ? number(p_distance) * std::tan(2 * std::asin(sn)) /
                         number(p_film) * a.width + std::sqrt(0.5) * f : 0;
    }
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(wxBrush(dark ? *wxBLACK : *wxWHITE));
    gc->DrawEllipse(a.x + a.width / 2 - radius, a.y + a.height / 2 - radius, 2 * radius, 2 * radius);
  }
  gc->PopState();
  text(gc, caption, v.x, v.y + v.height + 6, wxColour(90, 90, 90), 0.9);
}

bool DiffractionView::darkScheme() const { return p_scheme && p_scheme->GetSelection() == 1; }

// The grey level of a fibre or frame PNG (as ecce-xrd wrote it) back to a 0..1
// display level, so exposure and colour scheme apply to the same data.
void DiffractionView::setLevels(const wxImage& im, bool inverted)
{
  p_levelN = im.GetWidth();
  p_level.assign((size_t)im.GetWidth() * im.GetHeight(), 0.f);
  const unsigned char *d = im.GetData();
  for (size_t k = 0; k < p_level.size(); k++) {
    const float g = d[3 * k] / 255.f;
    p_level[k] = inverted ? 1.f - g : g;
  }
}

// Spots as Gaussian blobs of optical density on a film that is faintly fogged
// toward the centre, with grain. The beamstop is drawn as an overlay. Strength follows
// I_rel on the crystal-wide scale, size grows gently with it.
void DiffractionView::renderFilm()
{
  const int n = p_layerSize;
  p_levelN = n;
  p_level.assign((size_t)n * n, 0.f);
  const double G = 12.0, sig0 = std::max(1.0, p_layerRmax / 6.0);
  for (const Spot& s : p_spots) {
    const double q = s.rel / 100.0;
    if (s.absent || q < 1e-7) continue;
    const double sig = sig0 * (0.8 + 0.5 * std::sqrt(std::min(q, 1.0))), amp = G * q;
    const int rad = (int)std::ceil(3.5 * sig);
    const int x0 = std::max(0, (int)s.px - rad), x1 = std::min(n - 1, (int)s.px + rad);
    const int y0 = std::max(0, (int)s.py - rad), y1 = std::min(n - 1, (int)s.py + rad);
    for (int y = y0; y <= y1; y++)
      for (int x = x0; x <= x1; x++) {
        const double dx = x + 0.5 - s.px, dy = y + 0.5 - s.py;
        p_level[(size_t)y * n + x] += (float)(amp * std::exp(-(dx * dx + dy * dy) / (2 * sig * sig)));
      }
  }
  const double c = n / 2.0;
  for (int y = 0; y < n; y++)
    for (int x = 0; x < n; x++) {
      const double r = std::hypot(x + 0.5 - c, y + 0.5 - c);
      unsigned h = (unsigned)x * 73856093u ^ (unsigned)y * 19349663u;
      h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
      const double grain = (h & 0xffff) / 32768.0 - 1.0;
      float& d = p_level[(size_t)y * n + x];
      d += (float)(0.012 + 0.05 * std::exp(-r / (0.18 * n)) + 0.006 * grain);
    }
}

// levels -> colours with the current exposure and scheme, then the backdrop mix.
void DiffractionView::updateImage()
{
  if (p_single && !p_layerReady) return;
  if (p_level.empty() || p_levelN <= 0) return;
  const double gain = std::pow(2.0, p_exposure->GetValue() / 10.0);
  unsigned char lut[256][3];
  for (int i = 0; i < 256; i++) {
    const double v = i / 255.0;
    if (!darkScheme()) {
      lut[i][0] = lut[i][1] = lut[i][2] = (unsigned char)std::lround(255 * (1 - v));
      continue;
    }
    static const double st[4][4] = {{0, 0, 0, 0}, {0.35, 25, 55, 200}, {0.7, 255, 150, 30}, {1, 255, 255, 255}};
    int k = 0;
    while (k < 2 && v > st[k + 1][0]) k++;
    const double t = std::min(1.0, std::max(0.0, (v - st[k][0]) / (st[k + 1][0] - st[k][0])));
    for (int c = 0; c < 3; c++) lut[i][c] = (unsigned char)std::lround(st[k][c + 1] * (1 - t) + st[k + 1][c + 1] * t);
  }
  wxImage im(p_levelN, p_levelN);
  unsigned char *o = im.GetData();
  for (size_t k = 0; k < p_level.size(); k++) {
    double v = p_single ? 1.0 - std::exp(-gain * p_level[k]) : std::min(1.0, gain * p_level[k]);
    if (!p_single && !p_frame) {
      v = std::max(0.0, double(p_level[k]));
      if (p_scale->GetSelection() == 0) v = std::clamp(1 + std::log10(std::max(v, 1e-300)) / 3.5, 0.0, 1.0);
      else if (p_scale->GetSelection() == 1) v = std::sqrt(std::clamp(v, 0.0, 1.0));
      else v = std::clamp(v, 0.0, 1.0);
      v = std::clamp(gain * std::pow(v, std::max(0.01, number(p_gamma))), 0.0, 1.0);
    }
    const unsigned char *c = lut[(int)std::lround(255 * v)];
    o[3 * k] = c[0]; o[3 * k + 1] = c[1]; o[3 * k + 2] = c[2];
  }
  p_calc = im;
  wxCommandEvent none;
  onOpacity(none);
}

void DiffractionView::setDisplay(int exposure, int scheme)
{
  p_exposure->SetValue(std::max(-20, std::min(20, exposure)));
  p_scheme->SetSelection(scheme == 1 ? 1 : 0);
  updateImage();
}

const DiffractionView::Spot *DiffractionView::spotNear(double x, double y) const
{
  const Spot *best = 0;
  // Keep the hit radius in display pixels at every zoom level.
  const double f = p_panel->imageRect(p_panel->GetClientSize()).m_width / imagePixels();
  double bd = std::pow(12.0 / f, 2);
  for (const Spot& s : p_spots) {
    const double d2 = (s.px - x) * (s.px - x) + (s.py - y) * (s.py - y);
    if (d2 < bd) { bd = d2; best = &s; }
  }
  return best;
}

static int gcd3(int a, int b, int c)
{
  a = std::abs(a); b = std::abs(b); c = std::abs(c);
  while (b) { int t = a % b; a = b; b = t; }
  while (c) { int t = a % c; a = c; c = t; }
  return a;
}

wxString DiffractionView::measureText(const Spot& a, const Spot *b) const
{
  const double sc = p_layerScale, c = p_layerSize / 2.0;
  const double ax = (a.px - c) / sc, ay = (c - a.py) / sc;
  if (!b) {
    if (p_frame)
      return wxString::Format(U("(%d %d %d)   d = %.3f Å   2θ = %.2f°   I = %.4g   at (%.2f, %.2f) mm"),
                              a.h, a.k, a.l, a.d, 2 * std::asin(p_geomLambda / (2 * a.d)) * 180 / M_PI, a.I, ax, ay);
    return wxString::Format(U("(%d %d %d) picked, d = %.3f Å; click a second spot"), a.h, a.k, a.l, a.d);
  }
  const double bx = (b->px - c) / sc, by = (c - b->py) / sc;
  const double dist = std::hypot(bx - ax, by - ay);
  if (p_frame) {
    const double na = std::hypot(ax, ay), nb = std::hypot(bx, by);
    double ang = 0;
    if (na > 0 && nb > 0)
      ang = std::acos(std::max(-1.0, std::min(1.0, (ax * bx + ay * by) / (na * nb)))) * 180 / M_PI;
    return wxString::Format(U("(%d %d %d) to (%d %d %d): %.2f mm on the detector, %.1f° at the beam centre; d = %.3f and %.3f Å"),
                            a.h, a.k, a.l, b->h, b->k, b->l, dist, ang, a.d, b->d);
  }
  wxString t = wxString::Format(U("(%d %d %d) to (%d %d %d): %.4f Å⁻¹, row d = %.3f Å"),
                                a.h, a.k, a.l, b->h, b->k, b->l, dist, dist > 0 ? 1.0 / dist : 0.0);
  const int n = gcd3(b->h - a.h, b->k - a.k, b->l - a.l);
  if (n > 0 && dist > 0)
    t += wxString::Format(U("; %d spots between: %.4f Å⁻¹ per step, cell edge %.3f Å (orthogonal axes)"),
                          n - 1, dist / n, n / dist);
  return t;
}

wxString DiffractionView::measure(int h1, int k1, int l1, int h2, int k2, int l2)
{
  const Spot *a = 0, *b = 0;
  for (const Spot& s : p_spots) {
    if (!a && s.h == h1 && s.k == k1 && s.l == l1) a = &s;
    if (!b && s.h == h2 && s.k == k2 && s.l == l2) b = &s;
  }
  if (!a || !b || !(p_single || p_frame)) return "";
  p_pick = 0;
  onImageClick(a->px, a->py, false);
  onImageClick(b->px, b->py, true);
  return p_measure->GetLabel();
}

wxString DiffractionView::measureStrongest()
{
  const Spot *a = 0, *b = 0;
  for (const Spot& s : p_spots) {
    if (s.absent) continue;
    if (!a || s.I > a->I) { b = a; a = &s; }
    else if (!b || s.I > b->I) b = &s;
  }
  return a && b ? measure(a->h, a->k, a->l, b->h, b->k, b->l) : wxString();
}

// Detector frame: a click picks a spot, a shift-click a second one to measure
// from the first.  Layer: two plain clicks.
void DiffractionView::onImageClick(double x, double y, bool second)
{
  const Spot *s = spotNear(x, y);
  if (!s || !(p_single || p_frame)) { p_pick = 0; p_measure->SetLabel(""); return; }
  wxString t;
  if (p_frame) {
    if (second && p_pick) t = measureText(*p_pick, s);
    else { p_pick = s; t = measureText(*s, 0); }
  } else if (p_pick && p_pick != s) {
    t = measureText(*p_pick, s);
    p_pick = 0;
  } else {
    p_pick = s;
    t = measureText(*s, 0);
  }
  p_measure->SetLabel(t);
  if (!p_process) p_pointer->SetLabel(t);
}

wxString DiffractionView::cursorText(double tt, double tol) const
{
  if (p_tt.size() < 2) return "";
  const double lam = parse(p_pwave->GetValue()), s = std::sin(tt * M_PI / 360);
  wxString t = wxString::Format(U("2θ = %.3f°"), tt);
  if (s > 0 && lam > 0) {
    const double d = lam / (2 * s);
    t += wxString::Format(U("   d = %.4f Å   Q = %.4f Å⁻¹"), d, 2 * M_PI / d);
  }
  const size_t i = (size_t)std::lround(std::min(std::max((tt - p_tt.front()) / (p_tt.back() - p_tt.front()), 0.0), 1.0) *
                                       (p_tt.size() - 1));
  if (i < p_y.size() && p_y[i] == p_y[i]) t += wxString::Format("   I = %.1f", p_y[i]);
  const wxString r = reflectionText(tt, tol);
  return r.empty() ? t : t + "   |   " + r;
}

wxBitmap DiffractionView::render(int w, int h, int k) const
{
  wxBitmap bmp(w * k, h * k, 24);
  wxMemoryDC dc(bmp);
  dc.SetBackground(*wxWHITE_BRUSH);
  dc.Clear();
  std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
  if (gc) {
    gc->Scale(k, k);
    paint(gc.get(), w, h);
  }
  gc.reset();
  dc.SelectObject(wxNullBitmap);
  return bmp;
}


DiffractionView::DiffractionView(wxWindow *parent, const string& structure,
                                 const vector<int>& selection)
  : wxPanel(parent, wxID_ANY), p_debounce(this), p_poll(this),
    p_structure(structure), p_pixels(PIXELS), p_periodic(hasLattice(structure)),
    p_powder(false), p_single(false), p_frame(false), p_debye(false), p_layerScale(1), p_layerSize(560),
    p_layerReady(false), p_layerRmax(8), p_geomDistance(0), p_geomLambda(0),
    p_levelN(0), p_pick(0)
{
  static int serial = 0;
  for (auto& v : p_axes) v[0] = v[1] = 0;
  std::error_code ec;
  p_dir = (fs::temp_directory_path() / ("ecce-xrd-" + std::to_string(wxGetProcessId()) + "-" +
                                        std::to_string(serial++))).string();
  fs::create_directories(p_dir, ec);
  std::ostringstream sel;
  for (size_t i = 0; i < selection.size(); ) {
    size_t j = i;
    while (j + 1 < selection.size() && selection[j + 1] == selection[j] + 1) j++;
    sel << (i ? "," : "") << selection[i];
    if (j > i) sel << "-" << selection[j];
    i = j + 1;
  }
  p_selectionText = sel.str();

  auto *top = new wxBoxSizer(wxVERTICAL);
  auto *bar = new wxBoxSizer(wxHORIZONTAL);
  bar->Add(new wxStaticText(this, wxID_ANY, "View"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
  p_view = new wxChoice(this, wxID_ANY);
  for (const char *v : {"Fibre", "Powder", "Single crystal", "Detector frame", "Debye"})
    p_view->Append(v);
  p_view->SetToolTip("Choose the calculated diffraction view; crystal views require a unit cell");
  p_view->SetMinSize(wxSize(90, -1));
  bar->Add(p_view, 1, wxRIGHT, 6);
  bar->Add(new wxStaticText(this, wxID_ANY, "Source"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
  p_source = new wxChoice(this, wxID_ANY);
  for (const char *v : {"Cu (1.5406 Å)", "Mo (0.7093 Å)", "Co (1.7890 Å)", "Other"})
    p_source->Append(U(v));
  p_source->SetSelection(0);
  p_source->SetToolTip("Radiation wavelength; choose Other and enter a wavelength in Options");
  p_source->SetMinSize(wxSize(130, -1));
  bar->Add(p_source, 1, wxRIGHT, 6);
  p_pattern = new wxToggleButton(this, wxID_ANY, "Pattern", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
  p_pattern->SetToolTip("Open the whole pattern in a separate window for close study and export; toggle again to dock it");
  bar->Add(p_pattern, 0);
  top->Add(bar, 0, wxEXPAND | wxALL, 4);
  p_panel = new DiffractionPanel(this, this);
  p_plot = new PowderPanel(this, this);
  for (wxWindow *w : {static_cast<wxWindow*>(p_panel), static_cast<wxWindow*>(p_plot)}) {
    w->SetMinSize(wxSize(180, 140));
    top->Add(w, 1, wxEXPAND | wxLEFT | wxRIGHT, 4);
  }
  p_pointer = new wxStaticText(this, wxID_ANY, "Calculating...", wxDefaultPosition,
                              wxDefaultSize, wxST_ELLIPSIZE_END | wxST_NO_AUTORESIZE);
  top->Add(p_pointer, 0, wxEXPAND | wxALL, 4);
  p_status = p_pointer;
  p_measure = new wxStaticText(this, wxID_ANY, "");
  p_measure->Hide();
  auto *buttons = new wxWrapSizer(wxHORIZONTAL);
  auto button = [&](const char *label, const char *tip) {
    auto *b = new wxButton(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    b->SetToolTip(tip); buttons->Add(b, 0, wxRIGHT | wxBOTTOM, 4); return b;
  };
  auto *options = button("Options...", "Geometry, sample and display settings of the current view");
  p_experimental = button("Experimental...", "Load a measured powder curve, or a backdrop picture for the fibre film");
  auto *png = button("Save PNG (view)...", "Export the pattern at the current zoom, including display overlays");
  auto *csv = button("Save CSV...", "Export the full calculated data, independent of zoom and display settings");
  p_update = button("Update", "Calculate the changed settings for this slow structure");
  p_update->Hide();
  top->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
  SetSizer(top);
  SetMinSize(wxSize(300, 220));

  p_options = new wxDialog(this, wxID_ANY, "Diffraction options", wxDefaultPosition,
                           wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
  auto *outer = new wxBoxSizer(wxVERTICAL);
  auto *lp = new wxScrolledWindow(p_options, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
  p_leftWin = lp;
  lp->SetScrollRate(0, 10);
  auto *left = new wxBoxSizer(wxVERTICAL);
  p_left = left;
  auto *geometry = new wxStaticBoxSizer(wxVERTICAL, lp, "Geometry");
  auto *sample = new wxStaticBoxSizer(wxVERTICAL, lp, "Sample");
  auto *display = new wxStaticBoxSizer(wxVERTICAL, lp, "Display");
  for (auto *g : {geometry, sample, display}) left->Add(g, 0, wxEXPAND | wxALL, 4);
  // A row's mask keeps one Options window in step with the selected view.
  auto row = [&](wxSizer *group, const wxString& label, wxWindow *control, int mask, const wxString& tip) {
    auto *r = new wxBoxSizer(wxHORIZONTAL);
    if (!label.empty()) {
      auto *l = new wxStaticText(lp, wxID_ANY, label);
      l->SetToolTip(tip);
      l->Wrap(280);
      r->Add(l, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    }
    control->SetToolTip(tip);
    r->Add(control, label.empty() ? 1 : 0, wxALIGN_CENTER_VERTICAL);
    group->Add(r, 0, wxEXPAND | wxALL, 3);
    p_optionRows.push_back({r, mask});
  };
  auto field = [&](wxSizer *g, const wxString& label, const char *value, int mask, const wxString& tip) {
    auto *t = new wxTextCtrl(lp, wxID_ANY, value, wxDefaultPosition, wxSize(80, -1), wxTE_PROCESS_ENTER);
    row(g, label, t, mask, tip);
    p_defaults.push_back({t, value});
    return t;
  };
  auto choice = [&](wxSizer *g, const char *label, std::initializer_list<const char*> values,
                    int mask, const char *tip) {
    auto *c = new wxChoice(lp, wxID_ANY);
    for (auto *v : values) c->Append(v);
    c->SetSelection(0); row(g, label, c, mask, tip); return c;
  };
  auto check = [&](wxSizer *g, const char *label, int mask, const char *tip, bool value = false) {
    auto *c = new wxCheckBox(lp, wxID_ANY, U(label));
    c->SetValue(value); row(g, "", c, mask, tip); return c;
  };
  p_pwave = field(geometry, U("Wavelength (Å)"), "1.5406", 31, "Follows Source; typing a wavelength selects Other");
  p_wavelength = p_pwave;
  if (!p_periodic) p_pwave->ChangeValue("1.5418");
  p_distance = field(geometry, "Film distance (mm)", "30", 1, "Sample to flat film");
  p_film = field(geometry, "Film size (mm)", "40", 1, "Edge of the square film");
  p_ptmin = field(geometry, U("2θ from (°)"), "5", 18, "Start of the scattering-angle range");
  p_ptmax = field(geometry, U("2θ to (°)"), "90", 18, "End of the scattering-angle range");
  p_step = field(geometry, U("2θ step (°)"), "0.01", 18, "Spacing of the calculated intensity samples");
  p_zone = choice(geometry, "Zone", {"hk(l): l is the layer", "h(k)l: k is the layer", "(h)kl: h is the layer"}, 4,
                  "Reciprocal-lattice plane; the layer number fixes the index in brackets");
  p_layerNo = field(geometry, "Layer number", "0", 4, "0 gives the plane through the reciprocal origin; positive and negative layers are allowed");
  p_lres = field(geometry, U("Smallest d (Å)"), "1", 4, "Resolution limit; reflections with a smaller spacing are left out");
  p_fdist = field(geometry, "Detector distance (mm)", "90", 8, "Crystal to flat detector");
  p_fphi = new wxSlider(lp, wxID_ANY, 0, 0, 359, wxDefaultPosition, wxSize(150, -1), wxSL_HORIZONTAL | wxSL_LABELS);
  row(geometry, U("Start angle φ (°)"), p_fphi, 8, U("Crystal orientation at the start; turns about the vertical axis through φ to φ + oscillation"));
  p_fosc = field(geometry, U("Oscillation (°)"), "10", 8, "Angle turned during the exposure");

  p_transform = choice(sample, "Transform", {"Auto", "Helical", "Finite model"}, 1,
                       "Auto fits a regular helical backbone; otherwise uses the finite model");
  p_repeat = field(sample, U("Layer-line repeat for the pointer (Å), 0 = from the model"), "0", 1,
                   "Sets layer numbers in the pointer and tooltip only; never changes the calculated intensity");
  p_length = field(sample, U("Fibre length (Å), 0 = no broadening"), "100", 1, "Gaussian layer-line broadening with FWHM 1/length");
  p_mis = field(sample, U("Misalignment of the fibre axes (°)"), "8", 1, "Angular spread of the fibre axes; draws reflections into arcs");
  p_b = field(sample, U("Thermal smearing B (Å²)"), "20", 1, "Isotropic displacement (Debye-Waller) parameter; a larger B fades the outer layer lines");
  p_axis = choice(sample, "Fibre axis", {"Auto (helix / principal axis)", "First to last selected atom"}, 1,
                  "Fitted helix or principal axis, or the line between the first and last selected atoms");
  p_selectedOnly = check(sample, "Selected atoms only", 1, "Calculate only the atoms selected when Diffraction opened", !selection.empty());
  p_selectedOnly->Enable(!selection.empty());
  p_doublet = check(sample, "Kα1/Kα2 doublet", 18, "Combine K-alpha1 and K-alpha2 in a 2:1 ratio; available for Cu, Mo and Co", true);
  p_pb = field(sample, U("Thermal smearing B (Å²)"), "0", 18, "Isotropic displacement (Debye-Waller) parameter");
  p_pfwhm = field(sample, U("Peak width, FWHM (° 2θ)"), "0.1", 2, "Pseudo-Voigt with equal Gaussian and Lorentzian parts");
  p_lb = field(sample, U("Thermal smearing B (Å²)"), "0", 4, "Isotropic displacement (Debye-Waller) parameter");
  p_fnoise = check(sample, "Noise", 8, "Include simulated counting noise in the detector frame", true);

  p_scale = choice(display, "Intensity scaling", {"Logarithmic", "Square root", "Linear"}, 1,
                   "Stretch the intensity for display only; the CSV retains raw intensities");
  p_gamma = field(display, "Display gamma (contrast)", "1", 1, "Display exponent; changes contrast without recalculating intensities");
  auto *back = new wxButton(lp, wxID_ANY, "Load Backdrop...");
  row(display, "Backdrop picture", back, 1, "Load a measured film picture behind the calculated pattern");
  p_opacity = new wxSlider(lp, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxSize(150, -1));
  row(display, "Pattern opacity over backdrop", p_opacity, 1, "Mix the calculated pattern with the backdrop picture");
  p_opacity->Enable(false);
  p_exposure = new wxSlider(lp, wxID_ANY, 0, -20, 20, wxDefaultPosition, wxSize(150, -1));
  row(display, "Exposure", p_exposure, 12, "Intensity gain before saturation, in tenths of a stop");
  p_scheme = choice(display, "Colour scheme", {"Film (dark on light)", "Dark (black, blue, orange, white)"}, 12,
                    "Film greyscale or false colour; changes the display only");
  p_absent = check(display, "Show extinct reflections", 4, "Open circles for reflections extinct in this model (|F| approximately zero)");
  p_lhkl = check(display, "Label spots (hkl)", 4, "Label spots above 0.5 percent of the strongest reflection");
  p_laxes = check(display, "Reciprocal axes", 4, "Draw the in-plane reciprocal-axis key", true);
  p_flabel = check(display, "Label spots (hkl)", 8, "Label the twelve strongest detector reflections");
  p_frings = check(display, "Resolution rings", 8, "Draw rings at spacings of 3, 2, 1.5 and 1 Angstrom");
  p_plabel = new wxSpinCtrl(lp, wxID_ANY, "8", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 0, 100, 8);
  row(display, "Label peaks above (%)", p_plabel, 2, "hkl labels at or above this percentage of the strongest peak");
  p_list = new wxListCtrl(lp, wxID_ANY, wxDefaultPosition, wxSize(450, 130), wxLC_REPORT | wxLC_SINGLE_SEL);
  const char *cols[] = {"h k l", "d (Å)", "2θ (°)", "|F|", "Multiplicity", "I (rel.)"};
  for (int i = 0; i < 6; i++) p_list->InsertColumn(i, U(cols[i]), wxLIST_FORMAT_LEFT, i ? 72 : 70);
  row(display, "", p_list, 2, "Calculated reflection list; clicking a peak selects its row here");
  lp->SetSizer(left);
  outer->Add(lp, 1, wxEXPAND | wxALL, 4);
  auto *foot = new wxBoxSizer(wxHORIZONTAL);
  auto *defaults = new wxButton(p_options, wxID_ANY, "Restore Defaults");
  defaults->SetToolTip("Restore the current view's geometry, sample and display defaults");
  auto *close = new wxButton(p_options, wxID_CLOSE, "Close");
  close->SetToolTip("Close Options and keep the settings");
  foot->Add(defaults, 0); foot->AddStretchSpacer(); foot->Add(close, 0);
  outer->Add(foot, 0, wxEXPAND | wxALL, 8);
  p_options->SetSizer(outer);
  p_options->Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { p_options->Hide(); });
  close->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { p_options->Hide(); });
  defaults->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { restoreDefaults(); });

  Bind(wxEVT_TIMER, [this](wxTimerEvent& e) {
    if (e.GetId() == p_debounce.GetId()) startCalculation(); else pollRun();
  });
  Bind(wxEVT_END_PROCESS, &DiffractionView::processEnded, this);
  p_view->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
    const int mode = p_view->GetSelection();
    if (!p_periodic && mode >= 1 && mode <= 3) {
      p_view->SetSelection(p_debye ? 4 : 0);
      status("This view needs a periodic structure with a unit cell."); return;
    }
    onPattern(mode); requestCalculation();
  });
  p_source->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { onSource(); requestCalculation(); });
  p_pattern->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent&) { if (p_breakout) p_breakout(p_pattern->GetValue()); });
  options->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { showOptions(); });
  p_experimental->Bind(wxEVT_BUTTON, [this](wxCommandEvent& e) { if (p_powder) onMeasured(e); else onBackdrop(e); });
  p_update->Bind(wxEVT_BUTTON, &DiffractionView::onCalculate, this);
  png->Bind(wxEVT_BUTTON, &DiffractionView::onSavePng, this);
  csv->Bind(wxEVT_BUTTON, &DiffractionView::onSaveCsv, this);
  back->Bind(wxEVT_BUTTON, &DiffractionView::onBackdrop, this);
  for (auto entry : p_defaults) {
    auto *c = entry.first;
    auto changed = [this, c](wxCommandEvent&) {
      if (p_loading) return;
      if (c == p_pwave) { p_source->SetSelection(3); onSource(); }
      if (c == p_gamma) updateImage();
      else if (c != p_repeat) requestCalculation();
    };
    c->Bind(wxEVT_TEXT, changed);
    c->Bind(wxEVT_TEXT_ENTER, changed);
  }
  for (wxChoice *c : {p_transform, p_axis, p_zone})
    c->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { requestCalculation(); });
  for (wxCheckBox *c : {p_selectedOnly, p_doublet, p_fnoise})
    c->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { requestCalculation(); });
  p_fphi->Bind(wxEVT_SCROLL_CHANGED, [this](wxScrollEvent&) { requestCalculation(); });
  p_plabel->Bind(wxEVT_SPINCTRL, [this](wxCommandEvent&) { p_plot->Refresh(); });
  p_exposure->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) { updateImage(); });
  p_scheme->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { updateImage(); });
  p_scale->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { updateImage(); });
  for (wxCheckBox *c : {p_absent, p_lhkl, p_laxes, p_flabel, p_frings})
    c->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { p_panel->Refresh(); });
  p_opacity->Bind(wxEVT_SLIDER, &DiffractionView::onOpacity, this);
  onPattern(p_periodic ? 1 : 0);
  p_loading = false;
  requestCalculation();
}

DiffractionView::~DiffractionView()
{
  clearTip();
  cancelCalculation();
  p_debounce.Stop();
  p_poll.Stop();
  delete p_options;
  std::error_code ec;
  if (!p_dir.empty()) fs::remove_all(p_dir, ec);
}

void DiffractionView::status(const wxString& text) { p_status->SetLabel(text); }

double DiffractionView::number(wxTextCtrl *c) const
{
  return parse(c->GetValue());
}

void DiffractionView::onPattern(int mode)
{
  if (!p_periodic && mode != 4) mode = 0;
  p_debye = mode == 4;
  p_powder = mode == 1 || p_debye;
  p_single = mode == 2;
  p_frame = mode == 3;
  p_view->SetSelection(mode);
  p_source->SetString(0, U(mode == 0 ? "Cu (1.5418 Å)" : "Cu (1.5406 Å)"));
  if (p_source->GetSelection() < 3) onSource();
  for (auto& row : p_optionRows) row.first->ShowItems((row.second & (1 << mode)) != 0);
  p_panel->Show(!p_powder);
  p_plot->Show(p_powder);
  p_experimental->Enable(p_powder || mode == 0);
  p_options->SetTitle("Diffraction options: " + p_view->GetStringSelection());
  p_left->Layout();
  p_leftWin->FitInside();
  if (p_options->IsShown()) showOptions();
  p_lines.clear();
  p_pointer->SetLabel("");
  p_measure->SetLabel("");
  p_pick = 0;
  p_level.clear();
  p_levelN = 0;
  p_calc = wxImage();
  p_spots.clear();
  p_layerReady = false;
  p_panel->reset();
  p_panel->setImage(wxImage());
  p_leftWin->FitInside();
  Layout();
}

void DiffractionView::setDebye()
{
  onPattern(4);
}

bool DiffractionView::testImageView(const string& prefix, string& why)
{
  if (p_powder) { why = "image view required"; return false; }
  auto mouse = [this](wxEventType type, wxPoint pos, bool shift = false) {
    wxMouseEvent e(type);
    e.SetPosition(pos); e.SetShiftDown(shift);
    if (type == wxEVT_MOUSEWHEEL) { e.m_wheelRotation = 360; e.m_wheelDelta = 120; }
    p_panel->GetEventHandler()->ProcessEvent(e);
  };
  auto home = [this]() {
    wxKeyEvent e(wxEVT_CHAR_HOOK); e.m_keyCode = WXK_HOME;
    p_panel->GetEventHandler()->ProcessEvent(e);
  };
  p_panel->reset();
  if (p_single) p_lhkl->SetValue(true);
  if (!savePng(prefix + ".full.png", why)) return false;
  const wxRect2DDouble full = p_panel->imageRect(p_panel->GetClientSize());
  const Spot *pick = nullptr;
  double nearest = 1e30;
  for (const Spot& s : p_spots) {
    const double r = std::hypot(s.px - imagePixels() / 2.0, s.py - imagePixels() / 2.0);
    if (!s.absent && r < nearest) { nearest = r; pick = &s; }
  }
  const double px = pick ? pick->px : imagePixels() / 2.0;
  const double py = pick ? pick->py : imagePixels() / 2.0;
  const wxPoint anchor = p_panel->position(px, py);
  double beforeX, beforeY, afterX, afterY;
  if (!p_panel->pixelAt(anchor, beforeX, beforeY)) { why = "anchor outside image"; return false; }
  mouse(wxEVT_MOUSEWHEEL, anchor);
  if (!p_panel->pixelAt(anchor, afterX, afterY) || std::abs(beforeX - afterX) > 1 || std::abs(beforeY - afterY) > 1 ||
      p_panel->imageRect(p_panel->GetClientSize()).m_width < full.m_width * 1.9) {
    why = "wheel did not zoom about cursor"; return false;
  }
  if (!savePng(prefix + ".zoom.png", why)) return false;
  const wxPoint drag = p_panel->area().GetTopLeft() + wxPoint(80, 80);
  const wxPoint oldPick = p_panel->position(px, py);
  mouse(wxEVT_LEFT_DOWN, drag, true);
  mouse(wxEVT_MOTION, drag + wxPoint(20, 15), true);
  mouse(wxEVT_LEFT_UP, drag + wxPoint(20, 15), true);
  const wxPoint newPick = p_panel->position(px, py);
  if (std::abs(newPick.x - oldPick.x - 20) > 1 || std::abs(newPick.y - oldPick.y - 15) > 1) {
    why = "Shift drag did not pan"; return false;
  }
  if (pick) {
    p_pick = nullptr;
    mouse(wxEVT_LEFT_DOWN, newPick);
    mouse(wxEVT_LEFT_UP, newPick);
    const wxString expected = wxString::Format("(%d %d %d)", pick->h, pick->k, pick->l);
    if (!p_measure->GetLabel().Contains(expected)) { why = "zoomed click picked wrong hkl"; return false; }
    fprintf(stderr, "DIFFRACTION: zoomed click %s\n", p_measure->GetLabel().ToUTF8().data());
  }
  if (!savePng(prefix + ".pan.png", why)) return false;
  home();
  if (p_panel->imageRect(p_panel->GetClientSize()) != full) { why = "Home did not reset"; return false; }
  if (!savePng(prefix + ".reset.png", why)) return false;
  const wxRect v = p_panel->area();
  const wxPoint a(v.x + v.width / 4, v.y + v.height / 4), b(v.x + 3 * v.width / 4, v.y + 3 * v.height / 4);
  mouse(wxEVT_LEFT_DOWN, a); mouse(wxEVT_MOTION, b); mouse(wxEVT_LEFT_UP, b);
  if (p_panel->imageRect(p_panel->GetClientSize()).m_width < full.m_width * 1.9) {
    why = "rectangle did not zoom"; return false;
  }
  if (!savePng(prefix + ".rectangle.png", why)) return false;
  // Reset only on empty image space, never on a reflection.
  bool empty = false;
  for (int y = v.y + 10; y < v.GetBottom() && !empty; y += 17)
    for (int x = v.x + 10; x < v.GetRight() && !empty; x += 17) {
      double ix, iy;
      if (p_panel->pixelAt(wxPoint(x, y), ix, iy) &&
          (!(p_single || p_frame) || !spotNear(ix, iy))) {
        mouse(wxEVT_LEFT_DCLICK, wxPoint(x, y)); empty = true;
      }
    }
  if (!empty || p_panel->imageRect(p_panel->GetClientSize()) != full) {
    why = "empty double click did not reset"; return false;
  }
  return true;
}

wxString DiffractionView::sizeText() const
{
  const wxRect area = wxDisplay(this).GetClientArea();
  return wxString::Format("window %dx%d, display client area %dx%d", GetSize().x, GetSize().y, area.width, area.height);
}

void DiffractionView::onSource()
{
  static const char *w[] = {"1.5406", "0.7093", "1.7890"};
  if (p_source->GetSelection() >= 0 && p_source->GetSelection() < 3) p_pwave->ChangeValue(p_source->GetSelection() == 0 && !p_powder && !p_single && !p_frame ? "1.5418" : w[p_source->GetSelection()]);
  p_doublet->Enable(p_source->GetSelection() < 3);
}

wxString DiffractionView::setSource(int index, int doublet)
{
  p_source->SetSelection(index);
  onSource();
  if (doublet >= 0) p_doublet->SetValue(doublet != 0);
  return wxString::Format("doublet=%d enabled=%d", p_doublet->GetValue() ? 1 : 0, p_doublet->IsEnabled() ? 1 : 0);
}

bool DiffractionView::setMeasured(const string& path, string& why)
{
  std::ifstream f(path);
  if (!f) { why = "cannot read " + path; return false; }
  p_measured = path;
  why.clear();
  return true;
}

void DiffractionView::onMeasured(wxCommandEvent&)
{
  wxFileDialog dlg(this, U("Experimental powder pattern"), "", "",
                   "Powder patterns (*.xrdml;*.brml;*.xy;*.xye;*.dat;*.txt;*.csv)|"
                   "*.xrdml;*.brml;*.xy;*.xye;*.dat;*.txt;*.csv|All files|*",
                   wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  string why;
  if (dlg.ShowModal() != wxID_OK) return;
  // A file that cannot be read says why in a message, not only in the status line.
  if (!setMeasured(dlg.GetPath().ToStdString(wxConvUTF8), why)) {
    wxMessageBox(wxString::FromUTF8(why.c_str()), "Experimental pattern", wxOK | wxICON_ERROR, this);
    return;
  }
  requestCalculation();
}

wxString DiffractionView::reflectionText(double tt, double tol) const
{
  const Line *best = 0;
  for (const Line& l : p_lines)
    if (l.rel >= 0.5 && std::fabs(l.tt - tt) <= tol && (!best || l.rel > best->rel)) best = &l;
  if (!best) return "";
  return wxString::Format(U("(%s)   d = %.3f Å   2θ = %.2f°   I = %.1f %%   multiplicity = %d"), best->hkl, best->d, best->tt, best->rel, best->mult);
}

bool DiffractionView::calculatePowder(string& why)
{
  static const char *srcs[] = {"cu", "mo", "co"};
  const int src = p_source->GetSelection();
  const bool doublet = src >= 0 && src < 3 && p_doublet->GetValue();
  vector<string> a = {p_debye ? "debye" : "powder", p_structure,
                      "--wavelength", num(number(p_pwave)),
                      "--step", num(number(p_step)),
                      "--tt-min", num(number(p_ptmin)), "--tt-max", num(number(p_ptmax)),
                      "--B", num(number(p_pb)),
                      "--out", p_png, "--out", p_csv};
  if (!p_debye) {
    a.insert(a.end(), {"--fwhm", num(number(p_pfwhm)), "--table", p_tab});
  }
  if (!p_measured.empty()) { a.push_back("--measured"); a.push_back(p_measured); }
  if (doublet) { a.push_back("--source"); a.push_back(srcs[src]); }
  return launch(a, [this](string& why) {
  p_tt.clear(); p_y.clear(); p_meas.clear(); p_lines.clear();
  string line;
  std::ifstream f(p_csv);
  std::getline(f, line);
  while (std::getline(f, line)) {
    double t, y, m = std::nan("");
    const int n = sscanf(line.c_str(), "%lf,%lf,%lf", &t, &y, &m);
    if (n < 2) continue;
    p_tt.push_back(t); p_y.push_back(y); p_meas.push_back(n == 3 ? m : std::nan(""));
  }
  if (p_measured.empty()) p_meas.clear();
  p_list->DeleteAllItems();
  if (!p_debye) {
    std::ifstream g(p_tab);
    std::getline(g, line);
    while (std::getline(g, line)) {
      int h, k, l, mult;
      double d, tt, fa, rel;
      char which[8] = "";
      const int nf = sscanf(line.c_str(), "%d,%d,%d,%lf,%lf,%lf,%d,%lf,%7s", &h, &k, &l, &d, &tt, &fa, &mult, &rel, which);
      if (nf < 8) continue;
      Line ln = {tt, d, rel, wxString::Format("%d %d %d", h, k, l) + (nf == 9 ? U((string("  K\xCE\xB1") + (string(which) == "Ka2" ? "2" : "1")).c_str()) : wxString()),
                 h, k, l, mult, nf == 9 && string(which) == "Ka2"};
      p_lines.push_back(ln);
      const long row = p_list->InsertItem(p_list->GetItemCount(), ln.hkl);
      p_list->SetItem(row, 1, wxString::Format("%.4f", d));
      p_list->SetItem(row, 2, wxString::Format("%.3f", tt));
      p_list->SetItem(row, 3, wxString::Format("%.2f", fa));
      p_list->SetItem(row, 4, wxString::Format("%d", mult));
      p_list->SetItem(row, 5, wxString::Format("%.2f", rel));
    }
  }
  p_plot->Refresh();
  return true;
  }, why);
}

bool DiffractionView::setLayer(const string& zone, int layer, double dmin, bool showAbsent,
                                 string& why)
{
  const int z = zone == "l" ? 0 : zone == "k" ? 1 : zone == "h" ? 2 : -1;
  if (!p_periodic) { why = "single crystal needs a periodic structure"; return false; }
  if (z < 0) { why = "zone is h, k or l"; return false; }
  p_zone->SetSelection(z);
  p_layerNo->SetValue(wxString::Format("%d", layer));
  p_lres->SetValue(wxString(num(dmin)));
  p_absent->SetValue(showAbsent);
  onPattern(2);
  why.clear();
  return true;
}

wxString DiffractionView::spotText(int h, int k, int l) const
{
  for (const Spot& s : p_spots)
    if (s.h == h && s.k == k && s.l == l)
      return wxString::Format(U("(%d %d %d)   d = %.3f Å   |F| = %.2f   I = %.4g%s"), h, k, l, s.d, s.F,
                              s.I, s.absent ? U("   extinct in this model (|F| ≈ 0)") : wxString());
  return "";
}

bool DiffractionView::setFrame(double phi, double osc, bool labels, bool rings, string& why)
{
  if (!p_periodic) { why = "detector frame needs a periodic structure"; return false; }
  p_source->SetSelection(1);
  onSource();
  p_fphi->SetValue((int)std::lround(phi));
  p_fosc->SetValue(wxString(num(osc)));
  p_flabel->SetValue(labels);
  p_frings->SetValue(rings);
  onPattern(3);
  why.clear();
  return true;
}

wxString DiffractionView::framePointerText(double x, double y) const
{
  const Spot *best = spotNear(x, y);
  if (best) return spotText(best->h, best->k, best->l);
  if (p_frings->IsChecked() && p_geomLambda > 0) {
    const double radius = std::hypot(x - p_layerSize / 2.0, y - p_layerSize / 2.0);
    const double scale = p_panel->imageRect(p_panel->GetClientSize()).m_width / imagePixels();
    for (double d : {3.0, 2.0, 1.5, 1.0}) {
      const double sn = p_geomLambda / (2 * d);
      if (sn >= 1) continue;
      const double angle = 2 * std::asin(sn);
      if (angle >= M_PI / 2) continue;
      if (std::fabs(radius - p_geomDistance * std::tan(angle) * p_layerScale) * scale <= 5)
        return wxString::Format(U("Resolution ring   d = %.2f Å   2θ = %.2f°"), d, angle * 180 / M_PI);
    }
  }
  return wxString();
}

bool DiffractionView::calculateFrame(string& why)
{
  vector<string> a = {"frame", p_structure,
                      "--wavelength", num(number(p_pwave)),
                      "--phi", std::to_string(p_fphi->GetValue()),
                      "--osc", num(number(p_fosc)), "--distance", num(number(p_fdist)),
                      "--pixels", std::to_string(p_layerSize), "--no-beamstop",
                      "--out", p_png, "--out", p_csv, "--geometry", p_runDir + "/geometry.txt"};
  if (!p_fnoise->IsChecked()) a.push_back("--no-noise");
  return launch(a, [this](string& why) {
  if (!p_calc.LoadFile(wxString::FromUTF8(p_png.c_str()), wxBITMAP_TYPE_PNG)) {
    why = "cannot read the frame image";
    return false;
  }
  setLevels(p_calc, true);
  std::ifstream g(p_runDir + "/geometry.txt");
  string key;
  double val;
  while (g >> key >> val) {
    if (key == "scale") p_layerScale = val;
    if (key == "size") p_layerSize = (int)val;
    if (key == "rmax") p_layerRmax = val;
    if (key == "distance") p_geomDistance = val;
    if (key == "wavelength") p_geomLambda = val;
    if (key.size() == 4 && key.compare(0, 2, "ax") == 0 && key[2] >= '0' && key[2] <= '2')
      p_axes[key[2] - '0'][key[3] == 'y'] = val;
  }
  p_spots.clear();
  std::ifstream f(p_csv);
  string line;
  std::getline(f, line);
  while (std::getline(f, line)) {
    Spot s;
    double x, y, phi, tt, fa;
    int ab;
    if (sscanf(line.c_str(), "%d,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d", &s.h, &s.k, &s.l, &x, &y, &phi,
               &tt, &s.d, &s.I, &fa, &ab) != 11) continue;
    s.px = p_layerSize / 2.0 + x * p_layerScale;
    s.py = p_layerSize / 2.0 - y * p_layerScale;
    s.F = fa;
    s.absent = ab != 0;
    s.rel = 0;
    p_spots.push_back(s);
  }
  p_back = wxImage();
  p_pick = 0;
  p_measure->SetLabel("");
  updateImage();
  return true;
  }, why);
}

wxString DiffractionView::layerPointerText(double x, double y) const
{
  const Spot *best = spotNear(x, y);
  return best ? spotText(best->h, best->k, best->l) : wxString();
}

bool DiffractionView::calculateLayer(string& why)
{
  static const char *zones[] = {"l", "k", "h"};
  const int layerNo = (int)std::lround(number(p_layerNo));
  vector<string> a = {"layer", p_structure,
                      "--zone", string(zones[std::max(0, p_zone->GetSelection())]) + "=" + std::to_string(layerNo),
                      "--wavelength", num(number(p_pwave)),
                      "--resolution", num(number(p_lres)), "--B", num(number(p_lb)),
                      "--size", std::to_string(p_layerSize),
                      "--out", p_csv, "--geometry", p_runDir + "/geometry.txt"};
  if (p_absent->IsChecked()) a.push_back("--show-absent");
  return launch(a, [this](string& why) {
  std::ifstream g(p_runDir + "/geometry.txt");
  string key;
  double val;
  while (g >> key >> val) {
    if (key == "scale") p_layerScale = val;
    if (key == "size") p_layerSize = (int)val;
    if (key == "rmax") p_layerRmax = val;
    if (key == "distance") p_geomDistance = val;
    if (key == "wavelength") p_geomLambda = val;
    if (key.size() == 4 && key.compare(0, 2, "ax") == 0 && key[2] >= '0' && key[2] <= '2')
      p_axes[key[2] - '0'][key[3] == 'y'] = val;
  }
  p_spots.clear();
  std::ifstream f(p_csv);
  string line;
  std::getline(f, line);
  while (std::getline(f, line)) {
    Spot s;
    double x, y, irel;
    int ab;
    if (sscanf(line.c_str(), "%d,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%d", &s.h, &s.k, &s.l, &x, &y, &s.d,
               &s.F, &s.I, &irel, &ab) != 10) continue;
    s.px = p_layerSize / 2.0 + x * p_layerScale;
    s.py = p_layerSize / 2.0 - y * p_layerScale;
    s.absent = ab != 0;
    s.rel = irel;
    p_spots.push_back(s);
  }
  p_back = wxImage();
  p_layerReady = true;
  p_pick = 0;
  p_measure->SetLabel("");
  renderFilm();
  updateImage();
  return true;
  }, why);
}

bool DiffractionView::prepareFibre(string& why)
{
  vector<string> a = {"fibre", p_structure,
                      "--transform", p_transform->GetSelection() == 1 ? "helical" :
                                     p_transform->GetSelection() == 2 ? "finite" : "auto",
                      "--geometry", p_runDir + "/fibre-geometry.txt",
                      "--wavelength", num(number(p_wavelength)),
                      "--distance", num(number(p_distance)),
                      "--film-size", num(number(p_film)),
                      "--pixels", std::to_string(p_pixels),
                      "--B", num(number(p_b)),
                      "--length-broadening", num(number(p_length)),
                      "--misalignment", num(number(p_mis)),
                      "--scale", "linear",
                      "--decades", "3.5",
                      "--no-invert",
                      "--gamma", "1",
                      "--out", p_png, "--out", p_csv};
  if (p_selectedOnly->IsChecked() && !p_selectionText.empty()) {
    a.push_back("--select");
    a.push_back(p_selectionText);
  }
  if (p_axis->GetSelection() == 1) {
    a.push_back("--axis");
    a.push_back("selection");
  }
  return launch(a, [this](string& why) {
  if (!p_calc.LoadFile(wxString::FromUTF8(p_png.c_str()), wxBITMAP_TYPE_PNG)) {
    why = "cannot read the pattern image";
    return false;
  }
  std::ifstream geometry(p_runDir + "/fibre-geometry.txt");
  string geometryKey;
  double repeat = 0;
  if (geometry >> geometryKey >> repeat)
    p_inferredRepeat = repeat;
  setLevels(p_calc, false);
  p_r.clear(); p_z.clear(); p_i.clear();
  std::ifstream f(p_csv);
  string line;
  std::getline(f, line);
  while (std::getline(f, line)) {
    double x, y, r, z, i;
    if (sscanf(line.c_str(), "%lf,%lf,%lf,%lf,%lf", &x, &y, &r, &z, &i) != 5) continue;
    p_r.push_back((float)r); p_z.push_back((float)z); p_i.push_back((float)i);
  }
  vector<float> outer;
  for (size_t k = 0; k < p_i.size(); ++k)
    if (std::hypot(p_r[k], p_z[k]) >= 0.012) outer.push_back(p_i[k]);
  std::sort(outer.begin(), outer.end());
  const double rank = outer.empty() ? 0 : (outer.size() - 1) * 0.999;
  const size_t lo = (size_t)rank;
  const double ref = outer.empty() ? 1 : outer[lo] + (rank - lo) * (outer[std::min(lo + 1, outer.size() - 1)] - outer[lo]);
  p_levelN = p_pixels;
  p_level.resize(p_i.size());
  for (size_t k = 0; k < p_i.size(); ++k)
    p_level[k] = std::hypot(p_r[k], p_z[k]) >= 0.012 ? p_i[k] / std::max(ref, 1e-30) : 0;
  updateImage();
  return true;
  }, why);
}

void DiffractionView::onOpacity(wxCommandEvent&)
{
  if (!p_calc.IsOk()) return;
  if (!p_back.IsOk()) { p_panel->setImage(p_calc); return; }
  const double a = p_opacity->GetValue() / 100.0;
  wxImage out(p_calc.GetWidth(), p_calc.GetHeight());
  const unsigned char *c = p_calc.GetData(), *b = p_back.GetData();
  unsigned char *o = out.GetData();
  for (int k = 0; k < p_calc.GetWidth() * p_calc.GetHeight() * 3; k++)
    o[k] = (unsigned char)(a * c[k] + (1 - a) * b[k] + 0.5);
  p_panel->setImage(out);
}

bool DiffractionView::setBackdrop(const string& path, string& why)
{
  wxImage im;
  if (!im.LoadFile(wxString::FromUTF8(path.c_str()))) { why = "cannot read " + path; return false; }
  p_back = im.Scale(p_pixels, p_pixels, wxIMAGE_QUALITY_BILINEAR);
  p_opacity->Enable(true);
  wxCommandEvent none;
  onOpacity(none);
  return true;
}

void DiffractionView::onBackdrop(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Backdrop image", "", "", "Images (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg",
                   wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  string why;
  if (dlg.ShowModal() == wxID_OK && !setBackdrop(dlg.GetPath().ToStdString(wxConvUTF8), why))
    status(wxString::FromUTF8(why.c_str()));
}

wxString DiffractionView::pointerText(double x, double y) const
{
  if (p_single) return layerPointerText(x, y);
  if (p_frame) return framePointerText(x, y);
  const size_t k = (size_t)y * p_pixels + (size_t)x;
  if (k >= p_i.size()) return "";
  const double r = p_r[k], z = p_z[k], s = std::hypot(r, z);
  const double rep = number(p_repeat) > 0 ? number(p_repeat) : p_inferredRepeat;
  wxString t;
  if (rep > 0) t += wxString::Format("Layer line l = %.2f   ", z * rep);
  t += wxString::Format(U("R = %.4f   Z = %.4f Å⁻¹"), r, z);
  if (s > 0) t += wxString::Format(U("   d = %.2f Å"), 1.0 / s);
  t += wxString::Format("   I = %.3g", p_i[k]);
  return t;
}

bool DiffractionView::savePng(const string& path, string& why) const
{
  const bool ready = p_powder ? !p_tt.empty() : p_single ? p_layerReady : p_calc.IsOk();
  if (!ready) { why = "nothing calculated yet"; return false; }
  const wxSize size = p_panel->GetClientSize();
  const wxBitmap bmp = p_powder ? render(900, p_meas.empty() ? 560 : 700, 2) : render(size.x, size.y, 2);
  if (!bmp.SaveFile(wxString::FromUTF8(path.c_str()), wxBITMAP_TYPE_PNG)) {
    why = "cannot write " + path;
    return false;
  }
  return true;
}

bool DiffractionView::saveCsv(const string& path, string& why) const
{
  std::error_code ec;
  if (!fs::exists(p_csv)) { why = "nothing calculated yet"; return false; }
  fs::copy_file(p_csv, path, fs::copy_options::overwrite_existing, ec);
  if (ec) { why = "cannot write " + path + ": " + ec.message(); return false; }
  return true;
}

void DiffractionView::onSavePng(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Save current view (Home resets the whole image)", "", "diffraction.png", "PNG (*.png)|*.png",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  string why;
  if (dlg.ShowModal() == wxID_OK && !savePng(dlg.GetPath().ToStdString(wxConvUTF8), why))
    status(wxString::FromUTF8(why.c_str()));
}

void DiffractionView::onSaveCsv(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Save the pattern as CSV", "", "diffraction.csv", "CSV (*.csv)|*.csv",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  string why;
  if (dlg.ShowModal() == wxID_OK && !saveCsv(dlg.GetPath().ToStdString(wxConvUTF8), why))
    status(wxString::FromUTF8(why.c_str()));
}

void DiffractionView::onCalculate(wxCommandEvent&)
{
  p_debounce.Stop();
  startCalculation();
}

void DiffractionView::cancelCalculation()
{
  if (!p_process) return;
  p_process->Detach();
  wxProcess::Kill(p_pid, wxSIGKILL, wxKILL_CHILDREN);
  p_process = nullptr;
  p_pid = 0;
  p_poll.Stop();
  p_load = {};
}

void DiffractionView::requestCalculation()
{
  if (p_loading) return;
  clearTip();
  cancelCalculation();
  p_dirty = true;
  p_error.clear();
  if (p_manual) {
    p_debounce.Stop();
    status("This structure takes over 2 seconds. Settings changed; press Update.");
  } else {
    status("Calculating...");
    p_debounce.StartOnce(300);
  }
}

bool DiffractionView::launch(const vector<string>& args,
                            const std::function<bool(string&)>& load, string& why)
{
  wxArrayString argv;
#ifdef _WIN32
  fs::path py = fs::path(Ecce::ecceHome()) / "python" / "python.exe";
  argv.Add(wxString::FromUTF8((fs::exists(py) ? py.string() : string("python.exe")).c_str()));
#endif
  argv.Add(wxString::FromUTF8(tool().c_str()));
  for (const auto& a : args) argv.Add(wxString::FromUTF8(a.c_str()));
  p_output.clear();
  p_process = new wxProcess(this);
  p_process->Redirect();
  p_load = load;
  p_watch.Start();
  vector<const wxChar*> raw;
  for (const auto& arg : argv) raw.push_back(arg.c_str());
  raw.push_back(nullptr);
  p_pid = wxExecute(raw.data(), wxEXEC_ASYNC | wxEXEC_MAKE_GROUP_LEADER, p_process);
  if (p_pid <= 0) {
    delete p_process; p_process = nullptr; p_load = {};
    why = "Cannot start ecce-xrd"; return false;
  }
  p_poll.Start(50);
  return true;
}

void DiffractionView::startCalculation()
{
  p_debounce.Stop();
  cancelCalculation();
  ++p_generation;
  p_dirty = false;
  p_error.clear();
  p_runDir = p_dir + "/run-" + std::to_string(p_generation);
  std::error_code ec;
  fs::create_directories(p_runDir, ec);
  p_png = p_runDir + "/pattern.png";
  p_csv = p_runDir + "/pattern.csv";
  p_tab = p_runDir + "/reflections.csv";
  status("Calculating...");
  const bool ok = p_single ? calculateLayer(p_error) : p_frame ? calculateFrame(p_error) :
                  p_powder ? calculatePowder(p_error) : prepareFibre(p_error);
  if (!ok) status(wxString::FromUTF8(p_error.c_str()));
}

void DiffractionView::pollRun()
{
  if (!p_process) return;
  p_output += drain(p_process->GetInputStream());
  p_output += drain(p_process->GetErrorStream());
  if (!p_manual && p_watch.Time() > 2000) {
    p_manual = true;
    p_update->Show();
    Layout();
  }
  status(p_manual ? "Calculating... This structure takes over 2 seconds; subsequent changes need Update." :
                    "Calculating...");
}

void DiffractionView::processEnded(wxProcessEvent& event)
{
  if (!p_process || event.GetPid() != p_pid) return;
  pollRun();
  p_poll.Stop();
  delete p_process; p_process = nullptr; p_pid = 0;
  auto load = std::move(p_load);
  p_load = {};
  if (event.GetExitCode() != 0) {
    const size_t e = p_output.find("error: ");
    p_error = e == string::npos ? "ecce-xrd failed: " + p_output.substr(0, 300) :
              p_output.substr(e + 7, p_output.find('\n', e) - e - 7);
  } else if (load && load(p_error)) {
    p_completed = p_generation;
  }
  status(!p_error.empty() ? wxString::FromUTF8(p_error.c_str()) : wxString(p_manual ?
         "This structure takes over 2 seconds; change settings, then press Update." : "Move the pointer over the pattern."));
}

bool DiffractionView::waitForResult(string& why)
{
  // Script hooks wait on the same completion event as the UI, including a
  // pending debounce. No modal progress dialog or GUI calculation uses this loop.
  if (p_debounce.IsRunning()) { p_debounce.Stop(); startCalculation(); }
  wxGUIEventLoop loop;
  wxEventLoopActivator activate(&loop);
  while (p_process || p_debounce.IsRunning()) {
    loop.DispatchTimeout(50);
    wxTheApp->ProcessPendingEvents();
    if (p_debounce.IsRunning() && !p_process) { p_debounce.Stop(); startCalculation(); }
  }
  why = p_error;
  return why.empty() && p_completed != 0 && !p_dirty;
}

bool DiffractionView::calculate(string& why)
{
  startCalculation();
  return waitForResult(why);
}

void DiffractionView::showOptions()
{
  p_left->Layout();
  p_leftWin->FitInside();
  const wxRect area = wxDisplay(this).GetClientArea();
  const wxSize content = p_left->GetMinSize();
  p_options->SetSize(std::min(content.x + 40, area.width - 40),
                     std::min(content.y + 90, area.height - 60));
  p_options->Layout();
  p_options->Show();
  p_options->Raise();
}

void DiffractionView::restoreDefaults()
{
  p_loading = true;
  for (const auto& entry : p_defaults)
    if (entry.first->IsShown()) entry.first->ChangeValue(entry.second);
  p_source->SetSelection(0); onSource();
  if (!p_powder && !p_single && !p_frame) {
    p_axis->SetSelection(0); p_transform->SetSelection(0); p_scale->SetSelection(0);
    p_opacity->SetValue(100); p_selectedOnly->SetValue(!p_selectionText.empty());
  }
  if (p_powder) { p_doublet->SetValue(true); p_plabel->SetValue(8); }
  if (p_single) { p_zone->SetSelection(0); p_absent->SetValue(false); p_lhkl->SetValue(false); p_laxes->SetValue(true); }
  if (p_frame) { p_fphi->SetValue(0); p_fnoise->SetValue(true); p_flabel->SetValue(false); p_frings->SetValue(false); }
  p_exposure->SetValue(0); p_scheme->SetSelection(0);
  p_loading = false;
  updateImage(); requestCalculation();
}

void DiffractionView::setBreakoutHandler(const std::function<void(bool)>& handler)
{
  p_breakout = handler;
}

DiffractionDialog::DiffractionDialog(wxWindow *parent, const string& structure,
                                     const vector<int>& selection)
  : wxDialog(parent, wxID_ANY, "Diffraction", wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
  p_view = new DiffractionView(this, structure, selection);
  auto *s = new wxBoxSizer(wxVERTICAL);
  s->Add(p_view, 1, wxEXPAND);
  SetSizer(s);
  SetMinSize(wxSize(480, 360));
  const wxRect area = wxDisplay(this).GetClientArea();
  SetSize(std::min(960, area.width - 40), std::min(720, area.height - 60));
  p_view->setBreakoutHandler([this](bool open) { breakout(open); });
  Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { if (p_dock) breakout(false); else Destroy(); });
}

DiffractionDialog::~DiffractionDialog()
{
  // A docked view belongs to the same controller even while reparented.
  if (p_view && p_view->GetParent() != this) delete p_view;
}

void DiffractionDialog::dockTo(wxWindow *host)
{
  p_dock = host;
  breakout(false);
}

void DiffractionDialog::breakout(bool open)
{
  if (!p_dock) {
    if (open) { Show(); Raise(); }
    return;
  }
  wxWindow *target = open ? static_cast<wxWindow*>(this) : p_dock;
  auto *s = p_view->GetContainingSizer();
  if (s) s->Detach(p_view);
  p_view->Reparent(target);
  target->GetSizer()->Add(p_view, 1, wxEXPAND);
  p_view->p_pattern->SetValue(open);
  p_view->Show();
  p_dock->GetSizer()->Show(size_t(0), open);
  p_dock->Layout(); Layout();
  if (open) { Show(); Raise(); } else Hide();
}

void DiffractionView::clearTip()
{
  delete p_tip; p_tip = nullptr;
  p_tipText.clear(); p_pinned = false;
}

void DiffractionView::showTip(wxWindow *canvas, const wxPoint& pos, const wxString& label, bool pin)
{
  if (p_pinned && !pin) return;
  if (label.empty()) { clearTip(); return; }
  if (p_tip && p_tipText == label && !pin) return;
  clearTip();
  p_pinned = pin;
  p_tipText = label;
  p_tip = new wxPopupWindow(this, wxBORDER_SIMPLE);
  p_tip->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK));
  auto *s = new wxBoxSizer(wxVERTICAL);
  auto *t = new wxStaticText(p_tip, wxID_ANY, label);
  t->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT));
  t->Wrap(420);
  s->Add(t, 0, wxALL, 6);
  p_tip->SetSizerAndFit(s);
  p_tip->Position(canvas->ClientToScreen(pos + wxPoint(12, 16)), wxSize(0, 0));
  p_tip->Show();
}

void DiffractionView::selectReflection(double tt, double tolerance)
{
  long best = -1;
  for (size_t i = 0; i < p_lines.size(); ++i)
    if (std::fabs(p_lines[i].tt - tt) <= tolerance &&
        (best < 0 || p_lines[i].rel > p_lines[best].rel)) best = (long)i;
  if (best >= 0) {
    p_list->SetItemState(best, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
                        wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
    p_list->EnsureVisible(best);
  }
}

wxString DiffractionView::script(const vector<string>& args)
{
  if (args.size() < 2 || args[1] == "state")
    return wxString::Format("view=%d generation=%u completed=%u running=%d dirty=%d manual=%d docked=%d source=%d wavelength=%g repeat=%g",
      p_view->GetSelection(), p_generation, p_completed, p_process != nullptr, p_dirty, p_manual,
      !dynamic_cast<DiffractionDialog*>(GetParent()), p_source->GetSelection(), number(p_pwave), number(p_repeat));
  string why;
  if (args[1] == "wait") return waitForResult(why) ? "ready" : wxString::FromUTF8(why.empty() ? "Update required" : why.c_str());
  if (args[1] == "selection") {
    const long row = p_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    return wxString::Format("pinned=%d selected=%s", p_pinned,
                            row >= 0 ? p_list->GetItemText(row) : wxString("none"));
  }
  if (args[1] == "update") { startCalculation(); return "started"; }
  if (args[1] == "options") { showOptions(); return "done"; }
  if (args[1] == "close-options") { p_options->Hide(); return "done"; }
  if (args[1] == "defaults") { restoreDefaults(); return "done"; }
  if (args[1] == "screen" && args.size() > 2) {
    const wxSize size = wxGetDisplaySize();
    wxBitmap bitmap(size.x, size.y, 24);
    wxMemoryDC memory(bitmap);
    wxScreenDC screen;
    memory.Blit(0, 0, size.x, size.y, &screen, 0, 0);
    memory.SelectObject(wxNullBitmap);
    return bitmap.SaveFile(wxString::FromUTF8(args[2].c_str()), wxBITMAP_TYPE_PNG) ? "saved" : "failed";
  }
  if (args[1] == "options-shot" && args.size() > 2)
    return ecceWindowShot(p_options, wxString::FromUTF8(args[2].c_str())) ? "saved" : "failed";
  if (args[1] == "export" && args.size() > 3)
    return savePng(args[2], why) && saveCsv(args[3], why) ? "saved" : wxString::FromUTF8(why.c_str());
  if (args[1] == "set" && args.size() > 3) {
    const string& key = args[2];
    wxTextCtrl *field = nullptr;
    const std::pair<const char*, wxTextCtrl*> fields[] = {
      {"wavelength", p_pwave}, {"min", p_ptmin}, {"max", p_ptmax}, {"step", p_step}, {"width", p_pfwhm},
      {"B", p_powder ? p_pb : p_single ? p_lb : p_b}, {"distance", p_frame ? p_fdist : p_distance},
      {"film", p_film}, {"length", p_length}, {"misalignment", p_mis}, {"repeat", p_repeat},
      {"gamma", p_gamma}, {"layer", p_layerNo}, {"resolution", p_lres}, {"osc", p_fosc}};
    for (const auto& f : fields) if (key == f.first) field = f.second;
    if (field) {
      for (size_t i = 3; i < args.size(); ++i) field->SetValue(wxString::FromUTF8(args[i].c_str()));
      return "changed";
    }
    wxChoice *choice = key == "view" ? p_view : key == "source" ? p_source :
                       key == "scaling" ? p_scale : key == "scheme" ? p_scheme :
                       key == "transform" ? p_transform : key == "axis" ? p_axis : key == "zone" ? p_zone : nullptr;
    if (choice) {
      const int value = std::atoi(args[3].c_str());
      if (value < 0 || value >= (int)choice->GetCount()) return "invalid choice";
      choice->SetSelection(value);
      wxCommandEvent event(wxEVT_CHOICE, choice->GetId());
      event.SetEventObject(choice); choice->GetEventHandler()->ProcessEvent(event);
      return "changed";
    }
    if (key == "phi") {
      p_fphi->SetValue(std::atoi(args[3].c_str()));
      wxScrollEvent e(wxEVT_SCROLL_CHANGED, p_fphi->GetId());
      p_fphi->GetEventHandler()->ProcessEvent(e); return "changed";
    }
    return "unknown field";
  }
  if ((args[1] == "hover" || args[1] == "click") && args.size() > 2) {
    const bool click = args[1] == "click";
    wxWindow *canvas = p_powder ? static_cast<wxWindow*>(p_plot) : static_cast<wxWindow*>(p_panel);
    wxPoint pos;
    if (p_powder && p_tt.size() > 1) {
      const double tt = std::atof(args[2].c_str());
      pos = wxPoint(std::lround(POW_L + (tt - p_tt.front()) / (p_tt.back() - p_tt.front()) *
                               (p_plot->GetClientSize().x - POW_L - POW_R)), p_plot->GetClientSize().y / 2);
    } else if (args.size() > 4) {
      const Spot *found = nullptr;
      for (const auto& s : p_spots)
        if (s.h == std::atoi(args[2].c_str()) && s.k == std::atoi(args[3].c_str()) && s.l == std::atoi(args[4].c_str())) { found = &s; break; }
      if (!found) return "no spot";
      pos = p_panel->position(found->px, found->py);
    } else return "no pattern";
    wxMouseEvent event(click ? wxEVT_LEFT_UP : wxEVT_MOTION);
    event.SetPosition(pos);
    if (click && !p_powder) {
      wxMouseEvent down(wxEVT_LEFT_DOWN); down.SetPosition(pos);
      canvas->GetEventHandler()->ProcessEvent(down);
    }
    canvas->GetEventHandler()->ProcessEvent(event);
    return p_tipText;
  }
  return "unknown xrd command";
}

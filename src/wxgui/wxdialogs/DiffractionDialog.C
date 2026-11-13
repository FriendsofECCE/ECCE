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
#include <wx/progdlg.h>
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

class Child : public wxProcess
{
public:
  Child() : wxProcess(wxPROCESS_REDIRECT), done(false), code(-1) {}
  void OnTerminate(int, int status) override { done = true; code = status; }
  bool done;
  int code;
};

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
  DiffractionPanel(wxWindow *parent, DiffractionDialog *d)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(480, 520)), p_d(d)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &DiffractionPanel::onPaint, this);
    Bind(wxEVT_MOTION, &DiffractionPanel::onMove, this);
    Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) {
      int x, y;
      if (pixelAt(e.GetPosition(), x, y)) p_d->onImageClick(x, y, e.ShiftDown());
    });
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { p_d->p_pointer->SetLabel(""); });
  }
  void setImage(const wxImage& im) { p_bmp = im.IsOk() ? wxBitmap(im) : wxBitmap(); Refresh(); }
  const wxBitmap& bitmap() const { return p_bmp; }
  /// Image pixel (0..p_pixels or layer size) under a window position, or false.
  bool pixelAt(const wxPoint& p, int& x, int& y) const
  {
    const wxRect r = p_d->imageArea(GetClientSize());
    if (!r.Contains(p)) return false;
    const int n = p_d->imagePixels();
    x = (p.x - r.x) * n / r.width;
    y = (p.y - r.y) * n / r.height;
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
    int x, y;
    p_d->p_pointer->SetLabel(pixelAt(e.GetPosition(), x, y) ? p_d->pointerText(x, y) : wxString());
  }
  DiffractionDialog *p_d;
  wxBitmap p_bmp;
};


// The powder pattern; the pointer reports the reflection nearest to it.
class PowderPanel : public wxPanel
{
public:
  PowderPanel(wxWindow *parent, DiffractionDialog *d)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(560, 400)), p_d(d)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &PowderPanel::onPaint, this);
    Bind(wxEVT_MOTION, &PowderPanel::onMove, this);
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { p_d->p_pointer->SetLabel(""); });
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
    p_d->p_pointer->SetLabel(t);
  }
  DiffractionDialog *p_d;
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

wxRect DiffractionDialog::imageArea(const wxSize& s) const
{
  const int top = 30, side = 12, bottom = 26;
  const int side2 = std::max(10, std::min(s.x - 2 * side, s.y - top - bottom));
  return wxRect((s.x - side2) / 2, top, side2, side2);
}

int DiffractionDialog::imagePixels() const { return p_single || p_frame ? p_layerSize : p_pixels; }

void DiffractionDialog::paint(wxGraphicsContext *gc, double W, double H) const
{
  gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
  gc->SetBrush(*wxWHITE_BRUSH);
  gc->SetPen(*wxTRANSPARENT_PEN);
  gc->DrawRectangle(0, 0, W, H);
  if (p_powder) paintPowder(gc, W, H);
  else paintImage(gc, W, H);
}

void DiffractionDialog::paintPowder(wxGraphicsContext *gc, double W, double H) const
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
    text(gc, two ? (k == 0 ? "calculated" : "measured") : "calculated", L + 8, top + 4, col, 1.0, true);
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

void DiffractionDialog::paintImage(wxGraphicsContext *gc, double W, double H) const
{
  const wxRect a = imageArea(wxSize((int)W, (int)H));
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
                               number(p_fdist), p_fsource->GetStringSelection());
  } else {
    title = "Fibre diffraction pattern (simulated)";
    caption = wxString::Format("Flat film %g mm square, %g mm from the sample, logarithmic scale",
                               number(p_film), number(p_distance)) + wxString::FromUTF8(", \xCE\xBB = ") +
              wxString::Format("%g ", number(p_wavelength)) + wxString::FromUTF8("\xC3\x85");
  }
  text(gc, title, a.x, 6, INK, 1.1, true);
  if (p_single) {
    if (!p_layerReady) return;
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
        if (r.x < a.x || r.y < a.y || r.x + r.width > a.x + a.width || r.y + r.height > a.y + a.height) continue;
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
      const double ox = a.x + 26, oy = a.y + a.height - 26;
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
      gc->Clip(a.x, a.y, a.width, a.height);
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
            if (x > a.x + 4 && x + e.x < a.x + a.width - 4 && y - e.y > a.y + 4 && y < a.y + a.height - 4) {
              text(gc, s, x + 3, y - e.y - 2, ring, 0.9);
              break;
            }
          }
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
      gc->ResetClip();
    }
  }
  text(gc, caption, a.x, a.y + a.height + 6, wxColour(90, 90, 90), 0.9);
}

bool DiffractionDialog::darkScheme() const { return p_scheme && p_scheme->GetSelection() == 1; }

// The grey level of a fibre or frame PNG (as ecce-xrd wrote it) back to a 0..1
// display level, so exposure and colour scheme apply to the same data.
void DiffractionDialog::setLevels(const wxImage& im, bool inverted)
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
// toward the centre, with grain and a small round beamstop.  Strength follows
// I_rel on the crystal-wide scale, size grows gently with it.
void DiffractionDialog::renderFilm()
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
  const double c = n / 2.0, stop = std::max(5.0, 0.018 * n);
  for (int y = 0; y < n; y++)
    for (int x = 0; x < n; x++) {
      const double r = std::hypot(x + 0.5 - c, y + 0.5 - c);
      unsigned h = (unsigned)x * 73856093u ^ (unsigned)y * 19349663u;
      h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
      const double grain = (h & 0xffff) / 32768.0 - 1.0;
      float& d = p_level[(size_t)y * n + x];
      d += (float)(0.012 + 0.05 * std::exp(-r / (0.18 * n)) + 0.006 * grain);
      if (r < stop) d = 0.f;
    }
}

// levels -> colours with the current exposure and scheme, then the backdrop mix.
void DiffractionDialog::updateImage()
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
    const double v = p_single ? 1.0 - std::exp(-gain * p_level[k]) : std::min(1.0, gain * p_level[k]);
    const unsigned char *c = lut[(int)std::lround(255 * v)];
    o[3 * k] = c[0]; o[3 * k + 1] = c[1]; o[3 * k + 2] = c[2];
  }
  p_calc = im;
  wxCommandEvent none;
  onOpacity(none);
}

void DiffractionDialog::setDisplay(int exposure, int scheme)
{
  p_exposure->SetValue(std::max(-20, std::min(20, exposure)));
  p_scheme->SetSelection(scheme == 1 ? 1 : 0);
  updateImage();
}

const DiffractionDialog::Spot *DiffractionDialog::spotNear(int x, int y) const
{
  const Spot *best = 0;
  double bd = 12.0 * 12.0;
  for (const Spot& s : p_spots) {
    if (p_single && s.absent && !p_absent->IsChecked()) continue;
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

wxString DiffractionDialog::measureText(const Spot& a, const Spot *b) const
{
  const double sc = p_layerScale, c = p_layerSize / 2.0;
  const double ax = (a.px - c) / sc, ay = (c - a.py) / sc;
  if (!b) {
    if (p_frame)
      return wxString::Format(U("(%d %d %d)   d = %.3f Å   2θ = %.2f°   I = %.4g   at (%.2f, %.2f) mm"),
                              a.h, a.k, a.l, a.d, a.F, a.I, ax, ay);
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

wxString DiffractionDialog::measure(int h1, int k1, int l1, int h2, int k2, int l2)
{
  const Spot *a = 0, *b = 0;
  for (const Spot& s : p_spots) {
    if (!a && s.h == h1 && s.k == k1 && s.l == l1) a = &s;
    if (!b && s.h == h2 && s.k == k2 && s.l == l2) b = &s;
  }
  if (!a || !b || !(p_single || p_frame)) return "";
  p_pick = 0;
  onImageClick((int)std::lround(a->px), (int)std::lround(a->py), false);
  onImageClick((int)std::lround(b->px), (int)std::lround(b->py), true);
  return p_measure->GetLabel();
}

wxString DiffractionDialog::measureStrongest()
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
void DiffractionDialog::onImageClick(int x, int y, bool second)
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
}

wxString DiffractionDialog::cursorText(double tt, double tol) const
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

wxBitmap DiffractionDialog::render(int w, int h, int k) const
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


DiffractionDialog::DiffractionDialog(wxWindow *parent, const string& structure,
                                     const vector<int>& selection)
  : wxDialog(parent, wxID_ANY, "Diffraction", wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    p_structure(structure), p_pixels(PIXELS), p_periodic(hasLattice(structure)),
    p_powder(false), p_single(false), p_frame(false), p_layerScale(1), p_layerSize(560),
    p_layerReady(false), p_layerRmax(8), p_geomDistance(0), p_geomLambda(0),
    p_levelN(0), p_pick(0)
{
  static int serial = 0;
  for (auto& v : p_axes) v[0] = v[1] = 0;
  std::error_code ec;
  p_dir = (fs::temp_directory_path() / ("ecce-xrd-" + std::to_string(wxGetProcessId()) + "-" +
                                        std::to_string(serial++))).string();
  fs::create_directories(p_dir, ec);
  p_png = p_dir + "/pattern.png";
  p_csv = p_dir + "/pattern.csv";
  p_tab = p_dir + "/reflections.csv";

  // The selection as 1-40,57 for --select.
  std::ostringstream sel;
  for (size_t i = 0; i < selection.size(); ) {
    size_t j = i;
    while (j + 1 < selection.size() && selection[j + 1] == selection[j] + 1) j++;
    sel << (i ? "," : "") << selection[i];
    if (j > i) sel << "-" << selection[j];
    i = j + 1;
  }
  p_selectionText = sel.str();

  wxBoxSizer *top = new wxBoxSizer(wxHORIZONTAL);
  wxScrolledWindow *lp = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
  lp->SetScrollRate(0, 10);
  wxBoxSizer *left = new wxBoxSizer(wxVERTICAL);

  wxStaticBoxSizer *type = new wxStaticBoxSizer(wxVERTICAL, lp, "Pattern");
  wxRadioButton *fibre = new wxRadioButton(lp, wxID_ANY, "Fibre", wxDefaultPosition,
                                           wxDefaultSize, wxRB_GROUP);
  wxRadioButton *powder = new wxRadioButton(lp, wxID_ANY, "Powder (periodic structures only)");
  wxRadioButton *single = new wxRadioButton(lp, wxID_ANY, "Single crystal (periodic structures only)");
  wxRadioButton *frame = new wxRadioButton(lp, wxID_ANY, "Detector frame, rotating crystal (periodic structures only)");
  powder->Enable(p_periodic);
  single->Enable(p_periodic);
  frame->Enable(p_periodic);
  p_rbFibre = fibre; p_rbPowder = powder; p_rbSingle = single;
  powder->SetValue(p_periodic);
  fibre->SetValue(!p_periodic);
  type->Add(fibre, 0, wxALL, 3);
  type->Add(powder, 0, wxALL, 3);
  type->Add(single, 0, wxALL, 3);
  type->Add(frame, 0, wxALL, 3);
  left->Add(type, 0, wxEXPAND | wxALL, 4);
  p_left = left;
  wxBoxSizer *fibreBox = new wxBoxSizer(wxVERTICAL);
  p_fibreItems = fibreBox;

  wxFlexGridSizer *grid = new wxFlexGridSizer(2, 4, 6);
  auto field = [&](const wxString& label, const char *value, const wxString& tip) {
    grid->Add(new wxStaticText(lp, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    wxTextCtrl *t = new wxTextCtrl(lp, wxID_ANY, value, wxDefaultPosition, wxSize(80, -1));
    t->SetToolTip(tip);
    grid->Add(t, 0);
    return t;
  };
  p_wavelength = field(U("Wavelength (Å)"), "1.5418", U("Cu Kα is 1.5418"));
  p_distance = field("Film distance (mm)", "30", "Sample to flat film");
  p_film = field("Film size (mm)", "40", "Edge of the square film");
  p_b = field(U("B (Å²)"), "20", U("Isotropic displacement (Debye–Waller) parameter"));
  p_length = field(U("Fibre length (Å)"), "100", "Gaussian broadening along the layer lines; 0 = none");
  p_mis = field(U("Misalignment (°)"), "8", "Spread of the fibre axes: arcs");
  p_repeat = field(U("Repeat (Å)"), "0", "Axial repeat; the pointer then reports the layer line l = Z * repeat");
  p_gamma = field("Gamma", "1", "Display exponent");
  grid->Add(new wxStaticText(lp, wxID_ANY, "Axis"), 0, wxALIGN_CENTER_VERTICAL);
  p_axis = new wxChoice(lp, wxID_ANY);
  p_axis->Append("Auto (principal axis)");
  p_axis->Append("First to last selected atom");
  p_axis->SetSelection(0);
  grid->Add(p_axis, 0);
  grid->Add(new wxStaticText(lp, wxID_ANY, "Scaling"), 0, wxALIGN_CENTER_VERTICAL);
  p_scale = new wxChoice(lp, wxID_ANY);
  for (const char *s : {"log", "sqrt", "linear"}) p_scale->Append(s);
  p_scale->SetSelection(0);
  grid->Add(p_scale, 0);
  fibreBox->Add(grid, 0, wxALL, 6);
  p_selectedOnly = new wxCheckBox(lp, wxID_ANY, "Selected atoms only");
  p_selectedOnly->SetValue(!selection.empty());
  p_selectedOnly->Enable(!selection.empty());
  fibreBox->Add(p_selectedOnly, 0, wxALL, 6);

  wxButton *back = new wxButton(lp, wxID_ANY, "Load Backdrop...");
  fibreBox->Add(back, 0, wxLEFT | wxRIGHT | wxTOP, 6);
  fibreBox->Add(new wxStaticText(lp, wxID_ANY, "Calculated pattern opacity"), 0, wxLEFT | wxTOP, 6);
  p_opacity = new wxSlider(lp, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxSize(200, -1));
  fibreBox->Add(p_opacity, 0, wxLEFT | wxRIGHT, 6);
  // It fades the pattern over a backdrop picture; without one it has nothing to do.
  p_opacity->Enable(false);
  p_opacity->SetToolTip(wxString::FromUTF8("Opacity of the calculated pattern over the backdrop "
                                           "picture; load one with Load Backdrop... first"));
  left->Add(fibreBox, 0);

  wxBoxSizer *powderBox = new wxBoxSizer(wxVERTICAL);
  p_powderItems = powderBox;
  wxFlexGridSizer *pg = new wxFlexGridSizer(2, 4, 6);
  auto pfield = [&](const wxString& label, const char *value, const wxString& tip) {
    pg->Add(new wxStaticText(lp, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    wxTextCtrl *t = new wxTextCtrl(lp, wxID_ANY, value, wxDefaultPosition, wxSize(80, -1));
    t->SetToolTip(tip);
    pg->Add(t, 0);
    return t;
  };
  pg->Add(new wxStaticText(lp, wxID_ANY, "Source"), 0, wxALIGN_CENTER_VERTICAL);
  p_source = new wxChoice(lp, wxID_ANY);
  for (const char *c : {"Cu Kα (1.5406 Å)", "Mo Kα (0.7093 Å)", "Co Kα (1.7890 Å)", "Other (type the wavelength)"})
    p_source->Append(U(c));
  p_source->SetSelection(0);
  pg->Add(p_source, 0);
  pg->AddSpacer(0);
  p_doublet = new wxCheckBox(lp, wxID_ANY, "K\u03b11/K\u03b12 doublet");
  p_doublet->SetValue(true);
  p_doublet->SetToolTip(U("Draw every reflection at the Kα1 and Kα2 wavelengths (Kα2 at half the intensity); Cu, Mo and Co sources only"));
  pg->Add(p_doublet, 0);
  p_pwave = pfield(U("Wavelength (Å)"), "1.5406", "Used for the pattern; the Source choice fills it");
  p_ptmin = pfield(U("2θ from (°)"), "5", U("Degrees"));
  p_ptmax = pfield(U("2θ to (°)"), "90", U("Degrees"));
  p_pfwhm = pfield(U("Peak width, FWHM (° 2θ)"), "0.1", U("Pseudo-Voigt with equal Gauss and Lorentz parts"));
  p_pb = pfield(U("B (Å²)"), "0", U("Isotropic displacement (Debye–Waller) parameter"));
  powderBox->Add(pg, 0, wxALL, 6);
  powderBox->Add(new wxStaticText(lp, wxID_ANY, "Label peaks above (% of strongest)"), 0, wxLEFT | wxTOP, 6);
  p_plabel = new wxSpinCtrl(lp, wxID_ANY, "8", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100, 8);
  p_plabel->SetToolTip("hkl labels on peaks at or above this percentage of the strongest; 100 = only the strongest");
  powderBox->Add(p_plabel, 0, wxLEFT | wxRIGHT, 6);
  wxButton *meas = new wxButton(lp, wxID_ANY, "Experimental...");
  powderBox->Add(meas, 0, wxLEFT | wxRIGHT | wxTOP, 6);
  left->Add(powderBox, 0);

  wxBoxSizer *layerBox = new wxBoxSizer(wxVERTICAL);
  p_layerItems = layerBox;
  wxFlexGridSizer *lg = new wxFlexGridSizer(2, 4, 6);
  auto lfield = [&](const wxString& label, const char *value, const wxString& tip) {
    lg->Add(new wxStaticText(lp, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    wxTextCtrl *t = new wxTextCtrl(lp, wxID_ANY, value, wxDefaultPosition, wxSize(80, -1));
    t->SetToolTip(tip);
    lg->Add(t, 0);
    return t;
  };
  lg->Add(new wxStaticText(lp, wxID_ANY, "Zone"), 0, wxALIGN_CENTER_VERTICAL);
  p_zone = new wxChoice(lp, wxID_ANY);
  for (const char *c : {"hk(l): l is the layer", "h(k)l: k is the layer", "(h)kl: h is the layer"})
    p_zone->Append(c);
  p_zone->SetSelection(0);
  lg->Add(p_zone, 0);
  p_layerNo = lfield("Layer number", "0", "0 gives hk0, h0l or 0kl; 1, 2 ... the layers above it");
  p_lres = lfield(U("Smallest d (Å)"), "1", "Resolution limit: spots with a lattice spacing d below this are left out");
  p_lb = lfield(U("B (Å²)"), "0", U("Isotropic displacement (Debye–Waller) parameter"));
  layerBox->Add(lg, 0, wxALL, 6);
  p_absent = new wxCheckBox(lp, wxID_ANY, "Show absent reflections");
  p_absent->SetToolTip("Open circles where the structure factor is zero by symmetry");
  p_lhkl = new wxCheckBox(lp, wxID_ANY, "Label spots (hkl)");
  p_lhkl->SetToolTip("hkl next to every spot above 0.5 % of the strongest reflection");
  p_laxes = new wxCheckBox(lp, wxID_ANY, "Reciprocal axes");
  p_laxes->SetValue(true);
  for (wxCheckBox *c : {p_absent, p_lhkl, p_laxes}) layerBox->Add(c, 0, wxLEFT | wxRIGHT | wxTOP, 6);
  left->Add(layerBox, 0);

  // Detector frame (#985): a crystal turned through an oscillation range.
  wxBoxSizer *frameBox = new wxBoxSizer(wxVERTICAL);
  p_frameItems = frameBox;
  wxFlexGridSizer *fg = new wxFlexGridSizer(2, 4, 6);
  auto ffield = [&](const wxString& label, const char *value, const wxString& tip) {
    fg->Add(new wxStaticText(lp, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    wxTextCtrl *t = new wxTextCtrl(lp, wxID_ANY, value, wxDefaultPosition, wxSize(80, -1));
    t->SetToolTip(tip);
    fg->Add(t, 0);
    return t;
  };
  fg->Add(new wxStaticText(lp, wxID_ANY, U("Start angle φ (°)")), 0, wxALIGN_CENTER_VERTICAL);
  p_fphi = new wxSlider(lp, wxID_ANY, 0, 0, 359, wxDefaultPosition, wxSize(160, -1),
                        wxSL_HORIZONTAL | wxSL_LABELS);
  p_fphi->SetToolTip(U("Crystal orientation at the start of the exposure: the crystal turns about the vertical axis from φ to φ + Oscillation, and the spots are the reflections whose planes pass through the diffracting position (the Ewald sphere) in that range. Changing φ shows another part of the crystal's reciprocal lattice."));
  fg->Add(p_fphi, 0);
  p_fosc = ffield(U("Oscillation (°)"), "10", "Range turned during the exposure");
  p_fdist = ffield("Distance (mm)", "90", "Crystal to detector");
  p_fsource = new wxChoice(lp, wxID_ANY);
  for (const char *c : {"Mo", "Cu", "Co"}) p_fsource->Append(c);
  p_fsource->SetSelection(0);
  fg->Add(new wxStaticText(lp, wxID_ANY, "Source"), 0, wxALIGN_CENTER_VERTICAL);
  fg->Add(p_fsource, 0);
  frameBox->Add(fg, 0, wxALL, 6);
  p_fnoise = new wxCheckBox(lp, wxID_ANY, "Noise");
  p_fnoise->SetValue(true);
  p_flabel = new wxCheckBox(lp, wxID_ANY, "Label spots");
  p_flabel->SetToolTip("hkl next to the 12 strongest spots");
  p_frings = new wxCheckBox(lp, wxID_ANY, "Resolution rings");
  p_frings->SetToolTip(U("Circles at d = 3.0, 2.0, 1.5 and 1.0 Å, labelled with d and 2θ"));
  for (wxCheckBox *c : {p_fnoise, p_flabel, p_frings}) frameBox->Add(c, 0, wxLEFT | wxRIGHT | wxTOP, 6);
  left->Add(frameBox, 0);

  // Exposure and colour scheme of the image views (#985).
  wxStaticBoxSizer *disp = new wxStaticBoxSizer(wxVERTICAL, lp, "Image");
  p_displayItems = disp;
  disp->Add(new wxStaticText(lp, wxID_ANY, "Exposure"), 0, wxLEFT | wxTOP, 6);
  p_exposure = new wxSlider(lp, wxID_ANY, 0, -20, 20, wxDefaultPosition, wxSize(200, -1));
  p_exposure->SetToolTip("Intensity gain before saturation, in tenths of a stop; the data are unchanged");
  disp->Add(p_exposure, 0, wxLEFT | wxRIGHT, 6);
  p_scheme = new wxChoice(lp, wxID_ANY);
  p_scheme->Append("Film (dark on light)");
  p_scheme->Append("Dark (black, blue, orange, white)");
  p_scheme->SetSelection(0);
  disp->Add(p_scheme, 0, wxALL, 6);
  left->Add(disp, 0, wxEXPAND | wxALL, 4);

  wxButton *calc = new wxButton(lp, wxID_ANY, "Calculate");
  left->Add(calc, 0, wxALL, 6);
  wxButton *png = new wxButton(lp, wxID_ANY, "Save PNG...");
  wxButton *csv = new wxButton(lp, wxID_ANY, "Save CSV...");
  wxBoxSizer *save = new wxBoxSizer(wxHORIZONTAL);
  save->Add(png, 0, wxALL, 4);
  save->Add(csv, 0, wxALL, 4);
  left->Add(save, 0, wxALL, 2);
  left->AddStretchSpacer(1);
  left->Add(new wxButton(lp, wxID_CLOSE, "Close"), 0, wxALIGN_RIGHT | wxALL, 6);
  p_leftWin = lp;
  lp->SetSizer(left);
  left->Fit(lp);
  lp->SetMinSize(wxSize(lp->GetBestSize().x + wxSystemSettings::GetMetric(wxSYS_VSCROLL_X), 100));
  top->Add(lp, 0, wxEXPAND);

  wxBoxSizer *right = new wxBoxSizer(wxVERTICAL);
  p_panel = new DiffractionPanel(this, this);
  right->Add(p_panel, 1, wxEXPAND | wxALL, 4);
  p_plot = new PowderPanel(this, this);
  right->Add(p_plot, 2, wxEXPAND | wxALL, 4);
  p_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(560, 150), wxLC_REPORT | wxLC_SINGLE_SEL);
  const char *cols[] = {"h k l", "d (Å)", "2θ (°)", "|F|", "Multiplicity", "I (rel.)"};
  for (int i = 0; i < 6; i++) p_list->InsertColumn(i, U(cols[i]), wxLIST_FORMAT_LEFT, i ? 78 : 70);
  right->Add(p_list, 1, wxEXPAND | wxALL, 4);
  // The pointer and the status each have a panel of their own.
  wxPanel *pp = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN);
  wxBoxSizer *ps = new wxBoxSizer(wxVERTICAL);
  p_pointer = new wxStaticText(pp, wxID_ANY, "", wxDefaultPosition, wxSize(460, -1));
  ps->Add(p_pointer, 0, wxALL, 3);
  p_measure = new wxStaticText(pp, wxID_ANY, "", wxDefaultPosition, wxSize(460, -1));
  ps->Add(p_measure, 0, wxLEFT | wxRIGHT | wxBOTTOM, 3);
  pp->SetSizer(ps);
  right->Add(pp, 0, wxEXPAND | wxALL, 4);
  wxPanel *sp = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN);
  wxBoxSizer *ss = new wxBoxSizer(wxVERTICAL);
  p_status = new wxStaticText(sp, wxID_ANY, "Press Calculate.", wxDefaultPosition, wxSize(460, -1));
  ss->Add(p_status, 0, wxALL, 3);
  sp->SetSizer(ss);
  right->Add(sp, 0, wxEXPAND | wxALL, 4);
  top->Add(right, 1, wxEXPAND);
  SetSizer(top);
  for (wxWindow *w : {(wxWindow *)p_panel, (wxWindow *)p_plot, (wxWindow *)p_list}) w->SetMinSize(wxSize(200, 140));
  SetMinSize(wxSize(640, 480));
  {
    const wxRect area = wxDisplay(this).GetClientArea();
    SetSize(std::min(1060, area.width * 8 / 10), std::min(780, area.height * 8 / 10));
  }
  onPattern(p_periodic ? 1 : 0);
  // A view shows its own result at once: switching recalculates, so no view
  // is left showing another's picture or spots.
  powder->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent& e) { onPattern(1); onCalculate(e); });
  fibre->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent& e) { onPattern(0); onCalculate(e); });
  single->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent& e) { onPattern(2); onCalculate(e); });
  frame->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent& e) { onPattern(3); onCalculate(e); });
  p_rbFrame = frame;
  auto again = [this](wxCommandEvent&) { string why; if (p_frame && p_calc.IsOk()) calculate(why); };
  p_fnoise->Bind(wxEVT_CHECKBOX, again);
  p_plabel->Bind(wxEVT_SPINCTRL, [this](wxCommandEvent&) { p_plot->Refresh(); });
  p_exposure->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) { updateImage(); });
  p_scheme->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { updateImage(); });
  for (wxCheckBox *c : {p_absent, p_lhkl, p_laxes})
    c->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { if (p_single) p_panel->Refresh(); });
  for (wxCheckBox *c : {p_flabel, p_frings}) c->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { p_panel->Refresh(); });
  p_fsource->Bind(wxEVT_CHOICE, again);
  p_fphi->Bind(wxEVT_SCROLL_CHANGED, [this](wxScrollEvent&) { string why; if (p_frame) calculate(why); });
  meas->Bind(wxEVT_BUTTON, &DiffractionDialog::onMeasured, this);
  p_source->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { onSource(); });

  calc->Bind(wxEVT_BUTTON, &DiffractionDialog::onCalculate, this);
  back->Bind(wxEVT_BUTTON, &DiffractionDialog::onBackdrop, this);
  png->Bind(wxEVT_BUTTON, &DiffractionDialog::onSavePng, this);
  csv->Bind(wxEVT_BUTTON, &DiffractionDialog::onSaveCsv, this);
  p_opacity->Bind(wxEVT_SLIDER, &DiffractionDialog::onOpacity, this);
  FindWindow(wxID_CLOSE)->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Destroy(); });
  Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { Destroy(); });
}

DiffractionDialog::~DiffractionDialog()
{
  std::error_code ec;
  if (!p_dir.empty()) fs::remove_all(p_dir, ec);
}

void DiffractionDialog::status(const wxString& text) { p_status->SetLabel(text); }

double DiffractionDialog::number(wxTextCtrl *c) const
{
  return parse(c->GetValue());
}

string DiffractionDialog::run(const vector<string>& args, int& status)
{
  string cmd;
#ifdef _WIN32
  fs::path py = fs::path(Ecce::ecceHome()) / "python" / "python.exe";
  cmd = "\"" + (fs::exists(py) ? py.string() : string("python.exe")) + "\" ";
#endif
  cmd += "\"" + tool() + "\"";
  for (const string& a : args) cmd += " \"" + a + "\"";

  Child *child = new Child;
  const long pid = wxExecute(wxString::FromUTF8(cmd.c_str()), wxEXEC_ASYNC, child);
  if (pid == 0) { delete child; status = -1; return "cannot start " + tool(); }
  string out;
  wxStopWatch watch;
  wxProgressDialog *pd = 0;
  bool aborted = false;
  while (!child->done) {
    out += drain(child->GetInputStream());
    out += drain(child->GetErrorStream());
    if (!pd && watch.Time() > 1000)
      pd = new wxProgressDialog("Diffraction", "Calculating the pattern...", 100, this,
                                wxPD_CAN_ABORT | wxPD_APP_MODAL);
    if (pd && !pd->Pulse() && !aborted) { aborted = true; wxProcess::Kill(pid, wxSIGTERM); }
    wxMilliSleep(20);
    wxTheApp->Yield(true);
  }
  out += drain(child->GetInputStream());
  out += drain(child->GetErrorStream());
  delete pd;
  status = aborted ? -2 : child->code;
  return out;
}

void DiffractionDialog::onPattern(int mode)
{
  if (!p_periodic) mode = 0;
  p_powder = mode == 1;
  p_single = mode == 2;
  p_frame = mode == 3;
  p_left->Show(p_fibreItems, mode == 0, true);
  p_left->Show(p_powderItems, p_powder, true);
  p_left->Show(p_layerItems, p_single, true);
  p_left->Show(p_frameItems, p_frame, true);
  p_left->Show(p_displayItems, !p_powder, true);
  p_panel->Show(!p_powder);
  p_plot->Show(p_powder);
  p_list->Show(p_powder);
  p_pointer->SetLabel("");
  p_measure->SetLabel("");
  p_pick = 0;
  p_level.clear();
  p_levelN = 0;
  p_calc = wxImage();
  p_spots.clear();
  p_layerReady = false;
  p_panel->setImage(wxImage());
  p_left->FitInside(p_leftWin);
  Layout();
}

wxString DiffractionDialog::sizeText() const
{
  const wxRect area = wxDisplay(this).GetClientArea();
  return wxString::Format("window %dx%d, display client area %dx%d", GetSize().x, GetSize().y, area.width, area.height);
}

void DiffractionDialog::onSource()
{
  static const char *w[] = {"1.5406", "0.7093", "1.7890"};
  if (p_source->GetSelection() < 3) p_pwave->SetValue(w[p_source->GetSelection()]);
  p_doublet->Enable(p_source->GetSelection() < 3);
}

wxString DiffractionDialog::setSource(int index, int doublet)
{
  p_source->SetSelection(index);
  onSource();
  if (doublet >= 0) p_doublet->SetValue(doublet != 0);
  return wxString::Format("doublet=%d enabled=%d", p_doublet->GetValue() ? 1 : 0, p_doublet->IsEnabled() ? 1 : 0);
}

bool DiffractionDialog::setMeasured(const string& path, string& why)
{
  std::ifstream f(path);
  if (!f) { why = "cannot read " + path; return false; }
  p_measured = path;
  why.clear();
  return true;
}

void DiffractionDialog::onMeasured(wxCommandEvent&)
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
  string w2;
  if (!calculatePowder(w2)) {
    p_measured.clear();
    wxMessageBox(wxString::FromUTF8(w2.c_str()), "Experimental pattern", wxOK | wxICON_ERROR, this);
  }
}

wxString DiffractionDialog::reflectionText(double tt, double tol) const
{
  const Line *best = 0;
  for (const Line& l : p_lines)
    if (l.rel >= 0.5 && std::fabs(l.tt - tt) <= tol && (!best || l.rel > best->rel)) best = &l;
  if (!best) return "";
  return wxString::Format(U("(%s)   d = %.3f Å   2θ = %.2f°   I = %.1f"), best->hkl, best->d, best->tt, best->rel);
}

bool DiffractionDialog::calculatePowder(string& why)
{
  static const char *srcs[] = {"cu", "mo", "co"};
  const int src = p_source->GetSelection();
  const bool doublet = src >= 0 && src < 3 && p_doublet->GetValue();
  vector<string> a = {"powder", p_structure,
                      "--wavelength", num(number(p_pwave)),
                      "--tt-min", num(number(p_ptmin)), "--tt-max", num(number(p_ptmax)),
                      "--fwhm", num(number(p_pfwhm)), "--B", num(number(p_pb)),
                      "--out", p_png, "--out", p_csv, "--table", p_tab};
  if (!p_measured.empty()) { a.push_back("--measured"); a.push_back(p_measured); }
  if (doublet) { a.push_back("--source"); a.push_back(srcs[src]); }
  status("Calculating...");
  int st = 0;
  const string out = run(a, st);
  if (st != 0) {
    const size_t e = out.find("error: ");
    why = st == -2 ? "cancelled" :
        (e == string::npos ? "ecce-xrd failed: " + out.substr(0, 300)
                           : out.substr(e + 7, out.find('\n', e) - e - 7));
    status(wxString::FromUTF8(why.c_str()));
    return false;
  }
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
  std::ifstream g(p_tab);
  std::getline(g, line);
  while (std::getline(g, line)) {
    int h, k, l, mult;
    double d, tt, fa, rel;
    char which[8] = "";
    const int nf = sscanf(line.c_str(), "%d,%d,%d,%lf,%lf,%lf,%d,%lf,%7s", &h, &k, &l, &d, &tt, &fa, &mult, &rel, which);
    if (nf < 8) continue;
    Line ln = {tt, d, rel, wxString::Format("%d %d %d", h, k, l) + (nf == 9 ? U((string("  K\xCE\xB1") + (string(which) == "Ka2" ? "2" : "1")).c_str()) : wxString()),
               h, k, l, nf == 9 && string(which) == "Ka2"};
    p_lines.push_back(ln);
    const long row = p_list->InsertItem(p_list->GetItemCount(), ln.hkl);
    p_list->SetItem(row, 1, wxString::Format("%.4f", d));
    p_list->SetItem(row, 2, wxString::Format("%.3f", tt));
    p_list->SetItem(row, 3, wxString::Format("%.2f", fa));
    p_list->SetItem(row, 4, wxString::Format("%d", mult));
    p_list->SetItem(row, 5, wxString::Format("%.2f", rel));
  }
  p_plot->Refresh();
  status(wxString::FromUTF8(out.substr(0, out.find('\n')).c_str()));
  return true;
}

bool DiffractionDialog::setLayer(const string& zone, int layer, double dmin, bool showAbsent,
                                 string& why)
{
  const int z = zone == "l" ? 0 : zone == "k" ? 1 : zone == "h" ? 2 : -1;
  if (!p_periodic) { why = "single crystal needs a periodic structure"; return false; }
  if (z < 0) { why = "zone is h, k or l"; return false; }
  p_zone->SetSelection(z);
  p_layerNo->SetValue(wxString::Format("%d", layer));
  p_lres->SetValue(wxString(num(dmin)));
  p_absent->SetValue(showAbsent);
  p_rbSingle->SetValue(true);
  onPattern(2);
  why.clear();
  return true;
}

wxString DiffractionDialog::spotText(int h, int k, int l) const
{
  for (const Spot& s : p_spots)
    if (s.h == h && s.k == k && s.l == l)
      return wxString::Format(U("(%d %d %d)   d = %.3f Å   |F| = %.2f   I = %.4g%s"), h, k, l, s.d, s.F,
                              s.I, s.absent ? "   (systematically absent)" : "");
  return "";
}

bool DiffractionDialog::setFrame(double phi, double osc, bool labels, bool rings, string& why)
{
  if (!p_periodic) { why = "detector frame needs a periodic structure"; return false; }
  p_fphi->SetValue((int)std::lround(phi));
  p_fosc->SetValue(wxString(num(osc)));
  p_flabel->SetValue(labels);
  p_frings->SetValue(rings);
  p_rbFrame->SetValue(true);
  onPattern(3);
  why.clear();
  return true;
}

wxString DiffractionDialog::framePointerText(int x, int y) const
{
  const Spot *best = 0;
  double bd = 10.0 * 10.0;
  for (const Spot& s : p_spots) {
    const double d2 = (s.px - x) * (s.px - x) + (s.py - y) * (s.py - y);
    if (d2 < bd) { bd = d2; best = &s; }
  }
  return best ? wxString::Format(U("(%d %d %d)   d = %.3f Å   2θ = %.2f°   I = %.4g"), best->h,
                                 best->k, best->l, best->d, best->F, best->I)
              : wxString();
}

bool DiffractionDialog::calculateFrame(string& why)
{
  vector<string> a = {"frame", p_structure,
                      "--source", p_fsource->GetStringSelection().Lower().ToStdString(),
                      "--phi", std::to_string(p_fphi->GetValue()),
                      "--osc", num(number(p_fosc)), "--distance", num(number(p_fdist)),
                      "--pixels", std::to_string(p_layerSize),
                      "--out", p_png, "--out", p_csv, "--geometry", p_dir + "/geometry.txt"};
  if (!p_fnoise->IsChecked()) a.push_back("--no-noise");
  status("Calculating...");
  int st = 0;
  const string out = run(a, st);
  if (st != 0) {
    const size_t e = out.find("error: ");
    why = st == -2 ? "cancelled" :
        (e == string::npos ? "ecce-xrd failed: " + out.substr(0, 300)
                           : out.substr(e + 7, out.find('\n', e) - e - 7));
    status(wxString::FromUTF8(why.c_str()));
    return false;
  }
  if (!p_calc.LoadFile(wxString::FromUTF8(p_png.c_str()), wxBITMAP_TYPE_PNG)) {
    why = "cannot read the frame image";
    return false;
  }
  setLevels(p_calc, true);
  std::ifstream g(p_dir + "/geometry.txt");
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
    double x, y, phi, tt;
    if (sscanf(line.c_str(), "%d,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf", &s.h, &s.k, &s.l, &x, &y, &phi,
               &tt, &s.d, &s.I) != 9) continue;
    s.px = p_layerSize / 2.0 + x * p_layerScale;
    s.py = p_layerSize / 2.0 - y * p_layerScale;
    s.F = tt;
    s.absent = false;
    s.rel = 0;
    p_spots.push_back(s);
  }
  p_back = wxImage();
  p_pick = 0;
  p_measure->SetLabel("");
  updateImage();
  status(wxString::FromUTF8(out.substr(0, out.find('\n')).c_str()));
  return true;
}

wxString DiffractionDialog::layerPointerText(int x, int y) const
{
  const Spot *best = 0;
  double bd = 10.0 * 10.0;
  for (const Spot& s : p_spots) {
    const double d2 = (s.px - x) * (s.px - x) + (s.py - y) * (s.py - y);
    if (d2 < bd) { bd = d2; best = &s; }
  }
  return best ? spotText(best->h, best->k, best->l) : wxString();
}

bool DiffractionDialog::calculateLayer(string& why)
{
  static const char *zones[] = {"l", "k", "h"};
  const int layerNo = (int)std::lround(number(p_layerNo));
  vector<string> a = {"layer", p_structure,
                      "--zone", string(zones[std::max(0, p_zone->GetSelection())]) + "=" + std::to_string(layerNo),
                      "--resolution", num(number(p_lres)), "--B", num(number(p_lb)),
                      "--size", std::to_string(p_layerSize),
                      "--out", p_csv, "--geometry", p_dir + "/geometry.txt"};
  if (p_absent->IsChecked()) a.push_back("--show-absent");
  status("Calculating...");
  int st = 0;
  const string out = run(a, st);
  if (st != 0) {
    const size_t e = out.find("error: ");
    why = st == -2 ? "cancelled" :
        (e == string::npos ? "ecce-xrd failed: " + out.substr(0, 300)
                           : out.substr(e + 7, out.find('\n', e) - e - 7));
    status(wxString::FromUTF8(why.c_str()));
    return false;
  }
  std::ifstream g(p_dir + "/geometry.txt");
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
  status(wxString::FromUTF8(out.substr(0, out.find('\n')).c_str()));
  return true;
}

bool DiffractionDialog::calculate(string& why)
{
  if (p_single) return calculateLayer(why);
  if (p_frame) return calculateFrame(why);
  if (p_powder) return calculatePowder(why);
  vector<string> a = {"fibre", p_structure,
                      "--wavelength", num(number(p_wavelength)),
                      "--distance", num(number(p_distance)),
                      "--film-size", num(number(p_film)),
                      "--pixels", std::to_string(p_pixels),
                      "--B", num(number(p_b)),
                      "--length-broadening", num(number(p_length)),
                      "--misalignment", num(number(p_mis)),
                      "--scale", p_scale->GetStringSelection().ToStdString(),
                      "--decades", "3.5",
                      "--no-invert",
                      "--gamma", num(number(p_gamma)),
                      "--out", p_png, "--out", p_csv};
  if (p_selectedOnly->IsChecked() && !p_selectionText.empty()) {
    a.push_back("--select");
    a.push_back(p_selectionText);
  }
  if (p_axis->GetSelection() == 1) {
    a.push_back("--axis");
    a.push_back("selection");
  }
  status("Calculating...");
  int st = 0;
  const string out = run(a, st);
  if (st != 0) {
    const size_t e = out.find("error: ");
    why = st == -2 ? "cancelled" :
        (e == string::npos ? "ecce-xrd failed: " + out.substr(0, 300)
                           : out.substr(e + 7, out.find('\n', e) - e - 7));
    status(wxString::FromUTF8(why.c_str()));
    return false;
  }
  if (!p_calc.LoadFile(wxString::FromUTF8(p_png.c_str()), wxBITMAP_TYPE_PNG)) {
    why = "cannot read the pattern image";
    return false;
  }
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
  updateImage();
  status(wxString::FromUTF8(out.substr(0, out.find('\n')).c_str()));
  return true;
}

void DiffractionDialog::onOpacity(wxCommandEvent&)
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

bool DiffractionDialog::setBackdrop(const string& path, string& why)
{
  wxImage im;
  if (!im.LoadFile(wxString::FromUTF8(path.c_str()))) { why = "cannot read " + path; return false; }
  p_back = im.Scale(p_pixels, p_pixels, wxIMAGE_QUALITY_BILINEAR);
  p_opacity->Enable(true);
  wxCommandEvent none;
  onOpacity(none);
  return true;
}

void DiffractionDialog::onBackdrop(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Backdrop image", "", "", "Images (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg",
                   wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  string why;
  if (dlg.ShowModal() == wxID_OK && !setBackdrop(dlg.GetPath().ToStdString(wxConvUTF8), why))
    status(wxString::FromUTF8(why.c_str()));
}

wxString DiffractionDialog::pointerText(int x, int y) const
{
  if (p_single) return layerPointerText(x, y);
  if (p_frame) return framePointerText(x, y);
  const size_t k = (size_t)y * p_pixels + x;
  if (k >= p_i.size()) return "";
  const double r = p_r[k], z = p_z[k], s = std::hypot(r, z);
  const double rep = parse(p_repeat->GetValue());
  wxString t;
  if (rep > 0) t += wxString::Format("l = %.2f   ", z * rep);
  t += wxString::Format(U("R = %.4f   Z = %.4f Å⁻¹"), r, z);
  if (s > 0) t += wxString::Format(U("   d = %.2f Å"), 1.0 / s);
  t += wxString::Format("   I = %.3g", p_i[k]);
  return t;
}

bool DiffractionDialog::savePng(const string& path, string& why) const
{
  const bool ready = p_powder ? !p_tt.empty() : p_single ? p_layerReady : p_calc.IsOk();
  if (!ready) { why = "nothing calculated yet"; return false; }
  const wxBitmap bmp = p_powder ? render(900, p_meas.empty() ? 560 : 700, 2) : render(760, 820, 2);
  if (!bmp.SaveFile(wxString::FromUTF8(path.c_str()), wxBITMAP_TYPE_PNG)) {
    why = "cannot write " + path;
    return false;
  }
  return true;
}

bool DiffractionDialog::saveCsv(const string& path, string& why) const
{
  std::error_code ec;
  if (!fs::exists(p_csv)) { why = "nothing calculated yet"; return false; }
  fs::copy_file(p_csv, path, fs::copy_options::overwrite_existing, ec);
  if (ec) { why = "cannot write " + path + ": " + ec.message(); return false; }
  return true;
}

void DiffractionDialog::onSavePng(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Save the pattern", "", "diffraction.png", "PNG (*.png)|*.png",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  string why;
  if (dlg.ShowModal() == wxID_OK && !savePng(dlg.GetPath().ToStdString(wxConvUTF8), why))
    status(wxString::FromUTF8(why.c_str()));
}

void DiffractionDialog::onSaveCsv(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Save the pattern as CSV", "", "diffraction.csv", "CSV (*.csv)|*.csv",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  string why;
  if (dlg.ShowModal() == wxID_OK && !saveCsv(dlg.GetPath().ToStdString(wxConvUTF8), why))
    status(wxString::FromUTF8(why.c_str()));
}

void DiffractionDialog::onCalculate(wxCommandEvent&)
{
  string why;
  if (!calculate(why))
    wxMessageBox(wxString::FromUTF8(why.c_str()), "Diffraction", wxOK | wxICON_ERROR, this);
}

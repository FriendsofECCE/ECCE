#include "wxgui/DiffractionDialog.H"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <locale>
#include <sstream>

#include <algorithm>
#include <memory>

#include <wx/dcbuffer.h>
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
      t = p_d->reflectionText(tt, 4 * perPx);
      if (t.empty()) t = wxString::Format("2theta = %.2f", tt);
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
        if (!l.a2 && l.rel >= 8.0 && l.tt >= x0 && l.tt <= x1) order.push_back(&l);
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
  const wxString xt = wxString::FromUTF8("2\xCE\xB8 (degrees)    \xCE\xBB = ") +
                      wxString::Format("%.4f ", parse(p_pwave->GetValue())) + wxString::FromUTF8("\xC3\x85");
  text(gc, xt, L + pw / 2 - extent(gc, xt).x / 2.0, bot + 28, INK);
  const wxString yt = "Relative intensity";
  text(gc, yt, 8, T + (bot - T) / 2 + extent(gc, yt).x / 2.0, INK, 1.0, false, M_PI / 2);
}

void DiffractionDialog::paintImage(wxGraphicsContext *gc, double W, double H) const
{
  const wxRect a = imageArea(wxSize((int)W, (int)H));
  const double f = (double)a.width / imagePixels();
  const bool dark = p_frame && !p_fdark->IsChecked();
  wxString title, caption;
  if (p_single) {
    static const char *fmt[] = {"hk%d", "h%dl", "%dkl"};
    title = wxString::Format(fmt[std::max(0, p_zone->GetSelection())], (int)std::lround(number(p_layerNo))) +
            " layer (reciprocal lattice, simulated)";
    caption = wxString::Format("Disc area follows I_rel (%s scale); I_rel = 100 is the strongest reflection of the whole crystal",
                               p_lscale->GetStringSelection());
  } else if (p_frame) {
    title = wxString::Format("Rotating crystal, phi %d to %g degrees (simulated)", p_fphi->GetValue(),
                             p_fphi->GetValue() + number(p_fosc));
    caption = wxString::Format("Detector %.0f mm from the crystal, %s radiation",
                               number(p_fdist), p_fsource->GetStringSelection());
  } else {
    title = "Fibre diffraction pattern (simulated)";
    caption = wxString::Format("Flat film %g mm square, %g mm from the sample, logarithmic grey scale",
                               number(p_film), number(p_distance)) + wxString::FromUTF8(", \xCE\xBB = ") +
              wxString::Format("%g ", number(p_wavelength)) + wxString::FromUTF8("\xC3\x85");
  }
  text(gc, title, a.x, 6, INK, 1.1, true);
  if (p_single) {
    if (!p_layerReady) return;
    gc->SetPen(wxPen(wxColour(170, 170, 170), 1));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRectangle(a.x, a.y, a.width, a.height);
    const wxString mode = p_lscale->GetStringSelection();
    for (int pass = 0; pass < 2; pass++)
      for (const Spot& s : p_spots) {
        const double x = a.x + s.px * f, y = a.y + s.py * f;
        if (x < a.x || y < a.y || x > a.x + a.width || y > a.y + a.height) continue;
        if (s.absent) {
          if (pass == 0 && p_absent->IsChecked()) {
            gc->SetPen(wxPen(wxColour(150, 150, 150), 1));
            gc->DrawEllipse(x - 3.5 * f, y - 3.5 * f, 7 * f, 7 * f);
          }
          continue;
        }
        if (pass == 0) continue;
        const double q = std::min(1.0, std::max(s.rel / 100.0, 1e-300));
        double ar = mode == "linear" ? q : mode == "sqrt" ? std::sqrt(q)
                  : std::min(1.0, std::max(0.0, 1 + std::log10(q) / 3.0));
        const double r = std::max(p_layerRmax * std::sqrt(ar), 1.2) * f;
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(wxColour(25, 25, 25)));
        gc->DrawEllipse(x - r, y - r, 2 * r, 2 * r);
      }
    const double ox = a.x + p_layerSize / 2.0 * f, oy = a.y + p_layerSize / 2.0 * f;
    gc->SetPen(wxPen(wxColour(200, 40, 40), 1));
    gc->StrokeLine(ox - 5, oy, ox + 5, oy);
    gc->StrokeLine(ox, oy - 5, ox, oy + 5);
    static const char *names[] = {"a*", "b*", "c*"};
    for (int k = 0; k < 3; k++) {
      const double vx = p_axes[k][0], vy = p_axes[k][1], n = std::hypot(vx, vy);
      if (n < 1e-6) continue;
      const double ex = ox + vx * p_layerScale * f, ey = oy - vy * p_layerScale * f;
      const double ux = vx / n, uy = -vy / n;
      gc->SetPen(wxPen(CALC, 1.5));
      gc->StrokeLine(ox, oy, ex, ey);
      gc->StrokeLine(ex, ey, ex - 9 * ux + 4 * uy, ey - 9 * uy - 4 * ux);
      gc->StrokeLine(ex, ey, ex - 9 * ux - 4 * uy, ey - 9 * uy + 4 * ux);
      text(gc, names[k], ex + 8 * ux - 6, ey + 8 * uy - 8, CALC, 1.0, true);
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
    p_layerReady(false), p_layerRmax(8), p_geomDistance(0), p_geomLambda(0)
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
  wxBoxSizer *left = new wxBoxSizer(wxVERTICAL);

  wxStaticBoxSizer *type = new wxStaticBoxSizer(wxVERTICAL, this, "Pattern");
  wxRadioButton *fibre = new wxRadioButton(this, wxID_ANY, "Fibre", wxDefaultPosition,
                                           wxDefaultSize, wxRB_GROUP);
  wxRadioButton *powder = new wxRadioButton(this, wxID_ANY, "Powder (periodic structures only)");
  wxRadioButton *single = new wxRadioButton(this, wxID_ANY, "Single crystal (periodic structures only)");
  wxRadioButton *frame = new wxRadioButton(this, wxID_ANY, "Detector frame, rotating crystal (periodic structures only)");
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
  auto field = [&](const char *label, const char *value, const char *tip) {
    grid->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    wxTextCtrl *t = new wxTextCtrl(this, wxID_ANY, value, wxDefaultPosition, wxSize(80, -1));
    t->SetToolTip(tip);
    grid->Add(t, 0);
    return t;
  };
  p_wavelength = field("Wavelength (A)", "1.5418", "Cu K-alpha is 1.5418");
  p_distance = field("Film distance (mm)", "30", "Sample to flat film");
  p_film = field("Film size (mm)", "40", "Edge of the square film");
  p_b = field("B (A^2)", "20", "Isotropic temperature factor");
  p_length = field("Fibre length (A)", "100", "Gaussian broadening along the layer lines; 0 = none");
  p_mis = field("Misalignment (deg)", "8", "Spread of the fibre axes: arcs");
  p_repeat = field("Repeat (A)", "0", "Axial repeat; the pointer then reports the layer line l = Z * repeat");
  p_gamma = field("Gamma", "1", "Display exponent");
  grid->Add(new wxStaticText(this, wxID_ANY, "Axis"), 0, wxALIGN_CENTER_VERTICAL);
  p_axis = new wxChoice(this, wxID_ANY);
  p_axis->Append("Auto (principal axis)");
  p_axis->Append("First to last selected atom");
  p_axis->SetSelection(0);
  grid->Add(p_axis, 0);
  grid->Add(new wxStaticText(this, wxID_ANY, "Scaling"), 0, wxALIGN_CENTER_VERTICAL);
  p_scale = new wxChoice(this, wxID_ANY);
  for (const char *s : {"log", "sqrt", "linear"}) p_scale->Append(s);
  p_scale->SetSelection(0);
  grid->Add(p_scale, 0);
  fibreBox->Add(grid, 0, wxALL, 6);
  p_invert = new wxCheckBox(this, wxID_ANY, "Dark spots on white");
  p_invert->SetValue(true);
  fibreBox->Add(p_invert, 0, wxLEFT | wxRIGHT, 6);
  p_selectedOnly = new wxCheckBox(this, wxID_ANY, "Selected atoms only");
  p_selectedOnly->SetValue(!selection.empty());
  p_selectedOnly->Enable(!selection.empty());
  fibreBox->Add(p_selectedOnly, 0, wxALL, 6);

  wxButton *back = new wxButton(this, wxID_ANY, "Load Backdrop...");
  fibreBox->Add(back, 0, wxLEFT | wxRIGHT | wxTOP, 6);
  fibreBox->Add(new wxStaticText(this, wxID_ANY, "Calculated pattern opacity"), 0, wxLEFT | wxTOP, 6);
  p_opacity = new wxSlider(this, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxSize(200, -1));
  fibreBox->Add(p_opacity, 0, wxLEFT | wxRIGHT, 6);
  left->Add(fibreBox, 0);

  wxBoxSizer *powderBox = new wxBoxSizer(wxVERTICAL);
  p_powderItems = powderBox;
  wxFlexGridSizer *pg = new wxFlexGridSizer(2, 4, 6);
  auto pfield = [&](const char *label, const char *value, const char *tip) {
    pg->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    wxTextCtrl *t = new wxTextCtrl(this, wxID_ANY, value, wxDefaultPosition, wxSize(80, -1));
    t->SetToolTip(tip);
    pg->Add(t, 0);
    return t;
  };
  pg->Add(new wxStaticText(this, wxID_ANY, "Source"), 0, wxALIGN_CENTER_VERTICAL);
  p_source = new wxChoice(this, wxID_ANY);
  for (const char *c : {"Cu Ka (1.5406)", "Mo Ka (0.7093)", "Co Ka (1.7890)", "Other (type the wavelength)"})
    p_source->Append(c);
  p_source->SetSelection(0);
  pg->Add(p_source, 0);
  pg->AddSpacer(0);
  p_doublet = new wxCheckBox(this, wxID_ANY, "K\u03b11/K\u03b12 doublet");
  p_doublet->SetValue(true);
  p_doublet->SetToolTip("Draw every reflection at the K-alpha1 and K-alpha2 wavelengths (K-alpha2 at half the intensity); Cu, Mo and Co sources only");
  pg->Add(p_doublet, 0);
  p_pwave = pfield("Wavelength (A)", "1.5406", "Used for the pattern; the Source choice fills it");
  p_ptmin = pfield("2theta from", "5", "Degrees");
  p_ptmax = pfield("2theta to", "90", "Degrees");
  p_pfwhm = pfield("Peak width (deg)", "0.1", "FWHM in 2theta; pseudo-Voigt with equal Gauss and Lorentz parts");
  p_pb = pfield("B (A^2)", "0", "Isotropic temperature factor");
  powderBox->Add(pg, 0, wxALL, 6);
  wxButton *meas = new wxButton(this, wxID_ANY, "Load measured pattern...");
  powderBox->Add(meas, 0, wxLEFT | wxRIGHT | wxTOP, 6);
  left->Add(powderBox, 0);

  wxBoxSizer *layerBox = new wxBoxSizer(wxVERTICAL);
  p_layerItems = layerBox;
  wxFlexGridSizer *lg = new wxFlexGridSizer(2, 4, 6);
  auto lfield = [&](const char *label, const char *value, const char *tip) {
    lg->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    wxTextCtrl *t = new wxTextCtrl(this, wxID_ANY, value, wxDefaultPosition, wxSize(80, -1));
    t->SetToolTip(tip);
    lg->Add(t, 0);
    return t;
  };
  lg->Add(new wxStaticText(this, wxID_ANY, "Zone"), 0, wxALIGN_CENTER_VERTICAL);
  p_zone = new wxChoice(this, wxID_ANY);
  for (const char *c : {"hk(l): l is the layer", "h(k)l: k is the layer", "(h)kl: h is the layer"})
    p_zone->Append(c);
  p_zone->SetSelection(0);
  lg->Add(p_zone, 0);
  p_layerNo = lfield("Layer number", "0", "0 gives hk0, h0l or 0kl; 1, 2 ... the layers above it");
  p_lres = lfield("Resolution d_min (A)", "1", "Spots with d below this are left out");
  p_lb = lfield("B (A^2)", "0", "Isotropic temperature factor");
  lg->Add(new wxStaticText(this, wxID_ANY, "Spot area"), 0, wxALIGN_CENTER_VERTICAL);
  p_lscale = new wxChoice(this, wxID_ANY);
  for (const char *c : {"log", "sqrt", "linear"}) p_lscale->Append(c);
  p_lscale->SetSelection(0);
  lg->Add(p_lscale, 0);
  layerBox->Add(lg, 0, wxALL, 6);
  p_absent = new wxCheckBox(this, wxID_ANY, "Show absent reflections");
  p_absent->SetToolTip("Open circles where the structure factor is zero by symmetry");
  layerBox->Add(p_absent, 0, wxALL, 6);
  left->Add(layerBox, 0);

  // Detector frame (#985): a crystal turned through an oscillation range.
  wxBoxSizer *frameBox = new wxBoxSizer(wxVERTICAL);
  p_frameItems = frameBox;
  wxFlexGridSizer *fg = new wxFlexGridSizer(2, 4, 6);
  auto ffield = [&](const char *label, const char *value, const char *tip) {
    fg->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    wxTextCtrl *t = new wxTextCtrl(this, wxID_ANY, value, wxDefaultPosition, wxSize(80, -1));
    t->SetToolTip(tip);
    fg->Add(t, 0);
    return t;
  };
  fg->Add(new wxStaticText(this, wxID_ANY, "Start angle phi (deg)"), 0, wxALIGN_CENTER_VERTICAL);
  p_fphi = new wxSlider(this, wxID_ANY, 0, 0, 359, wxDefaultPosition, wxSize(160, -1),
                        wxSL_HORIZONTAL | wxSL_LABELS);
  p_fphi->SetToolTip("Where the oscillation starts, about the vertical axis");
  fg->Add(p_fphi, 0);
  p_fosc = ffield("Oscillation (deg)", "10", "Range turned during the exposure");
  p_fdist = ffield("Distance (mm)", "90", "Crystal to detector");
  p_fsource = new wxChoice(this, wxID_ANY);
  for (const char *c : {"Mo", "Cu", "Co"}) p_fsource->Append(c);
  p_fsource->SetSelection(0);
  fg->Add(new wxStaticText(this, wxID_ANY, "Source"), 0, wxALIGN_CENTER_VERTICAL);
  fg->Add(p_fsource, 0);
  frameBox->Add(fg, 0, wxALL, 6);
  p_fdark = new wxCheckBox(this, wxID_ANY, "Dark spots on light");
  p_fdark->SetValue(true);
  p_fnoise = new wxCheckBox(this, wxID_ANY, "Noise");
  p_fnoise->SetValue(true);
  p_flabel = new wxCheckBox(this, wxID_ANY, "Label spots");
  p_flabel->SetToolTip("hkl next to the 12 strongest spots");
  p_frings = new wxCheckBox(this, wxID_ANY, "Resolution rings");
  p_frings->SetToolTip("Circles at d = 3.0, 2.0, 1.5 and 1.0 A, labelled with d and 2 theta");
  for (wxCheckBox *c : {p_fdark, p_fnoise, p_flabel, p_frings}) frameBox->Add(c, 0, wxLEFT | wxRIGHT | wxTOP, 6);
  left->Add(frameBox, 0);

  wxButton *calc = new wxButton(this, wxID_ANY, "Calculate");
  left->Add(calc, 0, wxALL, 6);
  wxButton *png = new wxButton(this, wxID_ANY, "Save PNG...");
  wxButton *csv = new wxButton(this, wxID_ANY, "Save CSV...");
  wxBoxSizer *save = new wxBoxSizer(wxHORIZONTAL);
  save->Add(png, 0, wxALL, 4);
  save->Add(csv, 0, wxALL, 4);
  left->Add(save, 0, wxALL, 2);
  left->AddStretchSpacer(1);
  left->Add(new wxButton(this, wxID_CLOSE, "Close"), 0, wxALIGN_RIGHT | wxALL, 6);
  top->Add(left, 0, wxEXPAND);

  wxBoxSizer *right = new wxBoxSizer(wxVERTICAL);
  p_panel = new DiffractionPanel(this, this);
  right->Add(p_panel, 1, wxEXPAND | wxALL, 4);
  p_plot = new PowderPanel(this, this);
  right->Add(p_plot, 2, wxEXPAND | wxALL, 4);
  p_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(560, 150), wxLC_REPORT | wxLC_SINGLE_SEL);
  const char *cols[] = {"h k l", "d (A)", "2theta", "|F|", "Mult.", "I rel"};
  for (int i = 0; i < 6; i++) p_list->InsertColumn(i, cols[i], wxLIST_FORMAT_LEFT, i ? 78 : 70);
  right->Add(p_list, 1, wxEXPAND | wxALL, 4);
  // The pointer and the status each have a panel of their own.
  wxPanel *pp = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN);
  wxBoxSizer *ps = new wxBoxSizer(wxVERTICAL);
  p_pointer = new wxStaticText(pp, wxID_ANY, "", wxDefaultPosition, wxSize(460, -1));
  ps->Add(p_pointer, 0, wxALL, 3);
  pp->SetSizer(ps);
  right->Add(pp, 0, wxEXPAND | wxALL, 4);
  wxPanel *sp = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN);
  wxBoxSizer *ss = new wxBoxSizer(wxVERTICAL);
  p_status = new wxStaticText(sp, wxID_ANY, "Press Calculate.", wxDefaultPosition, wxSize(460, -1));
  ss->Add(p_status, 0, wxALL, 3);
  sp->SetSizer(ss);
  right->Add(sp, 0, wxEXPAND | wxALL, 4);
  top->Add(right, 1, wxEXPAND);
  SetSizerAndFit(top);
  onPattern(p_periodic ? 1 : 0);
  powder->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { onPattern(1); });
  fibre->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { onPattern(0); });
  single->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { onPattern(2); });
  frame->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { onPattern(3); });
  p_rbFrame = frame;
  auto again = [this](wxCommandEvent&) { string why; if (p_frame && p_calc.IsOk()) calculate(why); };
  for (wxCheckBox *c : {p_fdark, p_fnoise}) c->Bind(wxEVT_CHECKBOX, again);
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
  p_panel->Show(!p_powder);
  p_plot->Show(p_powder);
  p_list->Show(p_powder);
  p_pointer->SetLabel("");
  Layout();
  GetSizer()->Fit(this);
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
  wxFileDialog dlg(this, "Measured pattern (two columns: 2theta, intensity)", "", "",
                   "Text (*.txt;*.csv;*.xy;*.dat)|*.txt;*.csv;*.xy;*.dat|All files|*",
                   wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  string why;
  if (dlg.ShowModal() != wxID_OK) return;
  if (!setMeasured(dlg.GetPath().ToStdString(wxConvUTF8), why)) { status(wxString::FromUTF8(why.c_str())); return; }
  string w2;
  if (!p_tt.empty() && !calculatePowder(w2)) status(wxString::FromUTF8(w2.c_str()));
}

wxString DiffractionDialog::reflectionText(double tt, double tol) const
{
  const Line *best = 0;
  for (const Line& l : p_lines)
    if (l.rel >= 0.5 && std::fabs(l.tt - tt) <= tol && (!best || l.rel > best->rel)) best = &l;
  if (!best) return "";
  return wxString::Format("(%s)   d = %.3f A   2theta = %.2f   I = %.1f", best->hkl, best->d, best->tt, best->rel);
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
    Line ln = {tt, d, rel, wxString::Format("%d %d %d", h, k, l) + (nf == 9 ? wxString::Format("  %s", which) : wxString()),
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
      return wxString::Format("(%d %d %d)   d = %.3f A   |F| = %.2f   I = %.4g%s", h, k, l, s.d, s.F,
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
  return best ? wxString::Format("(%d %d %d)   d = %.3f A   2theta = %.2f deg   I = %.4g", best->h,
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
  if (!p_fdark->IsChecked()) a.push_back("--dark");
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
  p_panel->setImage(p_calc);
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
                      "--scale", p_lscale->GetStringSelection().ToStdString(),
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
  p_panel->Refresh();
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
                      p_invert->IsChecked() ? "--invert" : "--no-invert",
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
  p_r.clear(); p_z.clear(); p_i.clear();
  std::ifstream f(p_csv);
  string line;
  std::getline(f, line);
  while (std::getline(f, line)) {
    double x, y, r, z, i;
    if (sscanf(line.c_str(), "%lf,%lf,%lf,%lf,%lf", &x, &y, &r, &z, &i) != 5) continue;
    p_r.push_back((float)r); p_z.push_back((float)z); p_i.push_back((float)i);
  }
  wxCommandEvent none;
  onOpacity(none);
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
  t += wxString::Format("R = %.4f   Z = %.4f /A", r, z);
  if (s > 0) t += wxString::Format("   d = %.2f A", 1.0 / s);
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

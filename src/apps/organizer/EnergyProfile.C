#include "EnergyProfile.H"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <wx/clipbrd.h>
#include <wx/dc.h>
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>
#include <wx/dcmemory.h>
#include <wx/menu.h>

using std::string;
using std::vector;

namespace {

const char *DELTA = "\xCE\x94";
const char *DAGGER = "\xE2\x80\xA1";

string fmt(const char *f, double v)
{
  char b[64];
  snprintf(b, sizeof b, f, v);
  return b;
}

wxString wx(const string& s) { return wxString::FromUTF8(s.c_str()); }

// Liang-Barsky clip: does the segment touch the closed rectangle?
bool segHits(const EnergyProfileLayout::Seg& s, const wxRect& r)
{
  const double x0 = r.x, y0 = r.y, x1 = r.x + r.width, y1 = r.y + r.height;
  const double dx = s.x2 - s.x1, dy = s.y2 - s.y1;
  const double p[4] = {-dx, dx, -dy, dy};
  const double q[4] = {s.x1 - x0, x1 - s.x1, s.y1 - y0, y1 - s.y1};
  double t0 = 0, t1 = 1;
  for (int i = 0; i < 4; i++) {
    if (p[i] == 0) { if (q[i] < 0) return false; continue; }
    const double t = q[i] / p[i];
    if (p[i] < 0) t0 = std::max(t0, t); else t1 = std::min(t1, t);
    if (t0 > t1) return false;
  }
  return true;
}

// A label may go where it keeps 3 px from every line and every other label.
bool isFree(const EnergyProfileLayout& L, const wxRect& r, const wxRect& bounds)
{
  if (!bounds.Contains(r)) return false;
  wxRect p = r;
  p.Inflate(3);
  for (const EnergyProfileLayout::Seg& s : L.segs)
    if (segHits(s, p)) return false;
  for (const EnergyProfileLayout::Text& t : L.texts)
    if (p.Intersects(t.r)) return false;
  return true;
}

// Drops UTF-8 characters from the end until the text fits.
string elide(wxDC& dc, const string& s, int maxw)
{
  if (dc.GetTextExtent(wx(s)).x <= maxw) return s;
  string t = s;
  while (!t.empty()) {
    while (!t.empty() && (t.back() & 0xC0) == 0x80) t.pop_back();
    if (!t.empty()) t.pop_back();   // one whole character
    if (dc.GetTextExtent(wx(t + "\xE2\x80\xA6")).x <= maxw) return t + "\xE2\x80\xA6";
  }
  return "\xE2\x80\xA6";
}

}  // namespace

string profileNum(double v, const char *f)
{
  string s = fmt(f, v);
  if (s[0] == '-') {
    if (s.find_first_not_of("-0.") == string::npos) return s.substr(1);   // no "-0.0"
    s = "\xE2\x88\x92" + s.substr(1);
  }
  return s;
}

string EnergyProfileData::text() const
{
  string o = title + "\n";
  o += "kJ/mol relative to the reactants, G at " + fmt("%.2f", kelvin) + " K\n";
  o += "level     dE       dG       calculation\n";
  const char *lv[3] = {"reactant", "TS      ", "product "};
  for (int i = 0; i < 3; i++)
    o += string(lv[i]) + fmt(" %8.2f", E[i]) + fmt(" %8.2f", G[i]) + "  " + names[i] + "\n";
  o += string(DELTA) + "E" + DAGGER + " " + fmt("%.2f", E[1]) + "  " + DELTA + "G" + DAGGER + " " +
       fmt("%.2f", G[1]) + "  " + DELTA + "E " + fmt("%.2f", E[2]) + "  " + DELTA + "G " +
       fmt("%.2f", G[2]) + " kJ/mol\n";
  for (const string& l : rateLines) o += l + "\n";
  return o;
}

string EnergyProfileLayout::dump() const
{
  string o = "size " + std::to_string(size.x) + " " + std::to_string(size.y) + "\n";
  for (const Text& t : texts)
    o += "text " + std::to_string(t.r.x) + " " + std::to_string(t.r.y) + " " +
         std::to_string(t.r.width) + " " + std::to_string(t.r.height) + " " + t.what + ": " +
         t.text + "\n";
  for (const Seg& s : segs)
    o += "seg " + fmt("%.1f", s.x1) + " " + fmt("%.1f", s.y1) + " " + fmt("%.1f", s.x2) + " " +
         fmt("%.1f", s.y2) + " " + std::to_string(s.width) + " " + s.what + "\n";
  return o;
}

EnergyProfileLayout layoutEnergyProfile(wxDC& dc, const EnergyProfileData& d, const wxSize& size,
                                        const PlotPalette& pal)
{
  dc.SetFont(*wxNORMAL_FONT);
  auto ext = [&](const string& s) { return dc.GetTextExtent(wx(s)); };
  const int lh = ext("Ag").y;
  const wxColour cG = pal.series(1), cE = pal.series(0);

  double lo = 0, hi = 0;
  for (int i = 0; i < 3; i++) {
    lo = std::min(lo, std::min(d.E[i], d.G[i]));
    hi = std::max(hi, std::max(d.E[i], d.G[i]));
  }
  const double span = std::max(hi - lo, 1.0);
  lo -= 0.08 * span;
  hi += 0.08 * span;

  vector<string> block(1, string(DELTA) + "G" + DAGGER + " " + profileNum(d.G[1]) + " kJ/mol");
  for (const string& l : d.rateLines) block.push_back(l);
  int bw = 0;
  for (const string& l : block) bw = std::max(bw, ext(l).x);
  const int bh = (int)block.size() * (lh + 1);
  const string rxn = string(DELTA) + "G " + profileNum(d.G[2]) + " kJ/mol";
  const wxSize rs = ext(rxn);

  // Labels that find no free place make room: more empty space above (and
  // below) the levels, so the next pass has a free band to put them in.
  int extraTop = 0, extraBottom = 0;
  EnergyProfileLayout L;
  for (int attempt = 0; attempt < 24; attempt++) {
    L = EnergyProfileLayout();
    L.size = size;
    const int titleY = 8, legendY = titleY + lh + 6;
    const int plotTop = legendY + lh + 14;
    const int plotBottom = size.y - lh - 14;
    const int dataTop = plotTop + extraTop, dataBottom = plotBottom - extraBottom;
    if (dataBottom - dataTop < 40) { L.placed = false; break; }
    auto ymap = [&](double v) { return dataBottom - (v - lo) / (hi - lo) * (dataBottom - dataTop); };
    auto vat = [&](double y) { return lo + (dataBottom - y) / (double)(dataBottom - dataTop) * (hi - lo); };

    // Ticks at least 2.2 lines apart.
    const double vTop = vat(plotTop), vBot = vat(plotBottom);
    const double raw = (vTop - vBot) * 2.2 * lh / (plotBottom - plotTop);
    const double mag = std::pow(10.0, std::floor(std::log10(raw)));
    double step = 10 * mag;
    for (double m : {1.0, 2.0, 5.0})
      if (m * mag >= raw) { step = m * mag; break; }
    const int dec = std::max(0, (int)-std::floor(std::log10(step) + 1e-9));
    const string tf = "%." + std::to_string(dec) + "f";
    vector<std::pair<double, string> > ticks;
    int tickW = 0;
    for (double v = std::ceil(vBot / step) * step; v <= vTop + 1e-9 * step; v += step) {
      const double vv = std::fabs(v) < 1e-6 * step ? 0 : v;
      ticks.push_back({vv, profileNum(vv, tf.c_str())});
      tickW = std::max(tickW, ext(ticks.back().second).x);
    }
    const int left = 8 + lh + 8 + tickW + 8 + 5, right = size.x - 12;
    const int pw = right - left;

    // Fixed parts: title, legend, axis, ticks, names.
    auto text = [&](int x, int y, const string& s, const wxColour& c, const string& what) {
      const wxSize e = ext(s);
      L.texts.push_back({wxRect(x, y, e.x, e.y), s, c, false, what});
    };
    auto seg = [&](double x1, double y1, double x2, double y2, const wxColour& c, int w, bool dot,
                   const string& what) { L.segs.push_back({x1, y1, x2, y2, c, w, dot, what}); };
    const string title = elide(dc, d.title, size.x - 16);
    text((size.x - ext(title).x) / 2, titleY, title, pal.text, "title");
    const string lgG = string(DELTA) + "G (" + fmt("%.2f", d.kelvin) + " K)";
    const string lgE = string(DELTA) + "E (electronic)";
    const int lgW = 22 + 5 + ext(lgG).x + 20 + 22 + 5 + ext(lgE).x;
    int lx = (size.x - lgW) / 2;
    const double ly = legendY + lh / 2.0;
    seg(lx, ly, lx + 22, ly, cG, 3, false, "legend");
    text(lx + 27, legendY, lgG, pal.text, "legend");
    lx += 27 + ext(lgG).x + 20;
    seg(lx, ly, lx + 22, ly, cE, 3, false, "legend");
    text(lx + 27, legendY, lgE, pal.text, "legend");

    seg(left, plotTop, left, plotBottom, pal.axis, 1, false, "axis");
    for (const auto& t : ticks) {
      const double y = ymap(t.first);
      seg(left - 5, y, left, y, pal.axis, 1, false, "tick");
      const wxSize e = ext(t.second);
      text(left - 8 - e.x, (int)std::lround(y - e.y / 2.0), t.second, pal.text, "tick");
    }
    {
      const wxSize e = ext("kJ/mol");
      L.texts.push_back({wxRect(8, (plotTop + plotBottom - e.x) / 2, e.y, e.x), "kJ/mol", pal.text,
                         true, "axis-label"});
    }
    const double slot = pw / 3.0, hw = slot * 0.32;
    double cx[3];
    for (int i = 0; i < 3; i++) {
      cx[i] = left + slot * (i + 0.5);
      const string n = elide(dc, d.names[i], (int)slot - 10);
      text((int)std::lround(cx[i] - ext(n).x / 2.0), plotBottom + 6, n, pal.text, "name");
    }

    // Levels; dE and dG a few pixels apart when they would coincide.
    double yE[3], yG[3];
    for (int i = 0; i < 3; i++) {
      yE[i] = ymap(d.E[i]);
      yG[i] = ymap(d.G[i]);
      if (std::fabs(yE[i] - yG[i]) < 4) {
        const double m = (yE[i] + yG[i]) / 2, s = d.G[i] >= d.E[i] ? -2 : 2;
        yG[i] = m + s;
        yE[i] = m - s;
      }
    }
    for (int series = 0; series < 2; series++) {
      const double *y = series ? yG : yE;
      const wxColour& c = series ? cG : cE;
      const string nm = series ? "G" : "E";
      for (int i = 0; i < 3; i++)
        seg(cx[i] - hw, y[i], cx[i] + hw, y[i], c, 3, false, "level-" + nm);
      for (int i = 0; i < 2; i++)
        seg(cx[i] + hw, y[i], cx[i + 1] - hw, y[i + 1], c, 1, true, "link-" + nm);
    }
    // The dG barrier as a double arrow at the TS, from the reactants' zero.
    const double ax = cx[1], y0 = ymap(0), y1 = yG[1];
    seg(ax, y0, ax, y1, cG, 1, false, "arrow");
    if (std::fabs(y1 - y0) > 16) {
      const double dir = y1 > y0 ? 1 : -1;
      seg(ax, y1, ax - 4, y1 - dir * 7, cG, 1, false, "arrow");
      seg(ax, y1, ax + 4, y1 - dir * 7, cG, 1, false, "arrow");
      seg(ax, y0, ax - 4, y0 + dir * 7, cG, 1, false, "arrow");
      seg(ax, y0, ax + 4, y0 + dir * 7, cG, 1, false, "arrow");
    }

    const wxRect bounds(left + 4, plotTop, right - left - 4, plotBottom - plotTop);
    auto putBlock = [&](int x, int y) {
      for (size_t i = 0; i < block.size(); i++)
        text(x, y + (int)i * (lh + 1), block[i], cG, "barrier");
    };
    // Barrier label: beside the arrow, near its middle, then above or below the TS.
    bool blockOk = false;
    {
      const int amin = (int)std::min(y0, y1), amax = (int)std::max(y0, y1);
      const int ymid = (amin + amax) / 2 - bh / 2;
      vector<wxPoint> c;
      for (int k = 0; k <= 60; k++)
        for (int sgn : {1, -1}) {
          if (k == 0 && sgn < 0) continue;
          const int yy = ymid + sgn * k * (lh / 2);
          if (yy + bh < amin - bh || yy > amax + bh) continue;
          c.push_back(wxPoint((int)ax + 8, yy));
          c.push_back(wxPoint((int)ax - 8 - bw, yy));
        }
      const int tsTop = (int)std::min(yE[1], yG[1]), tsBot = (int)std::max(yE[1], yG[1]);
      c.push_back(wxPoint((int)(ax - bw / 2.0), tsTop - 6 - bh));
      c.push_back(wxPoint((int)(ax - bw / 2.0), tsBot + 6));
      c.push_back(wxPoint((int)(ax - bw / 2.0), plotTop + 4));
      for (const wxPoint& p : c)
        if (isFree(L, wxRect(p.x, p.y, bw, bh), bounds)) { putBlock(p.x, p.y); blockOk = true; break; }
    }
    // Reaction dG: above or below the product levels, then shifted sideways.
    bool rxnOk = false;
    {
      const int pTop = (int)std::min(yE[2], yG[2]), pBot = (int)std::max(yE[2], yG[2]);
      vector<wxPoint> c;
      for (int dy = 0; dy <= 3; dy++)
        for (int k : {0, -1, 1, -2, 2, -3}) {
          const int x = (int)(cx[2] - rs.x / 2.0 + k * slot * 0.12);
          c.push_back(wxPoint(x, pTop - 5 - rs.y - dy * lh));
          c.push_back(wxPoint(x, pBot + 5 + dy * lh));
        }
      for (const wxPoint& p : c)
        if (isFree(L, wxRect(p.x, p.y, rs.x, rs.y), bounds)) {
          text(p.x, p.y, rxn, cG, "reaction");
          rxnOk = true;
          break;
        }
    }
    if (blockOk && rxnOk) return L;
    if (!blockOk) extraTop += bh / 2 + 6;
    if (!rxnOk) {
      if (attempt % 2) extraBottom += rs.y + 4; else extraTop += rs.y + 4;
    }
  }
  L.placed = false;
  return L;
}

void paintEnergyProfile(wxDC& dc, const EnergyProfileLayout& L, const PlotPalette& pal)
{
  dc.SetBackground(wxBrush(pal.background));
  dc.Clear();
  dc.SetFont(*wxNORMAL_FONT);
  for (const EnergyProfileLayout::Seg& s : L.segs) {
    wxPen pen(s.colour, s.width, s.dotted ? wxPENSTYLE_DOT : wxPENSTYLE_SOLID);
    pen.SetCap(wxCAP_BUTT);
    dc.SetPen(pen);
    dc.DrawLine(wxRound(s.x1), wxRound(s.y1), wxRound(s.x2), wxRound(s.y2));
  }
  for (const EnergyProfileLayout::Text& t : L.texts) {
    dc.SetTextForeground(t.colour);
    if (t.vertical) dc.DrawRotatedText(wx(t.text), t.r.x, t.r.y + t.r.height, 90);
    else dc.DrawText(wx(t.text), t.r.x, t.r.y);
  }
}

EnergyProfileCanvas::EnergyProfileCanvas(wxWindow *parent)
  : wxPanel(parent, wxID_ANY)
{
  SetBackgroundStyle(wxBG_STYLE_PAINT);
  Bind(wxEVT_PAINT, &EnergyProfileCanvas::onPaint, this);
  Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { Refresh(); e.Skip(); });
  Bind(wxEVT_CONTEXT_MENU, [this](wxContextMenuEvent&) {
    wxMenu menu;
    const int id = wxWindow::NewControlId();
    menu.Append(id, _("Copy values"));
    menu.Bind(wxEVT_MENU, [this](wxCommandEvent&) {
      if (wxTheClipboard->Open()) {
        wxTheClipboard->SetData(new wxTextDataObject(wx(p_data.text())));
        wxTheClipboard->Close();
      }
    }, id);
    PopupMenu(&menu);
  });
}

void EnergyProfileCanvas::setProfile(const EnergyProfileData& d)
{
  p_data = d;
  Refresh();
}

void EnergyProfileCanvas::onPaint(wxPaintEvent&)
{
  wxAutoBufferedPaintDC pdc(this);
  wxGCDC dc(pdc);
  const PlotPalette pal = PlotPalette::system();
  paintEnergyProfile(dc, layoutEnergyProfile(dc, p_data, GetClientSize(), pal), pal);
}

EnergyProfileLayout exportLayout(const EnergyProfileData& d)
{
  wxBitmap bmp(ENERGY_PROFILE_PNG.x, ENERGY_PROFILE_PNG.y);
  wxMemoryDC mdc(bmp);
  wxGCDC dc(mdc);
  return layoutEnergyProfile(dc, d, ENERGY_PROFILE_PNG, PlotPalette::light());
}

bool saveEnergyProfilesPng(const vector<EnergyProfileData>& profiles, const string& file)
{
  if (profiles.empty()) return false;
  const wxSize one = ENERGY_PROFILE_PNG;
  wxBitmap all(one.x, one.y * (int)profiles.size());
  {
    wxMemoryDC adc(all);
    for (size_t i = 0; i < profiles.size(); i++) {
      wxBitmap bmp(one.x, one.y);
      {
        wxMemoryDC mdc(bmp);
        wxGCDC dc(mdc);
        const PlotPalette pal = PlotPalette::light();
        paintEnergyProfile(dc, layoutEnergyProfile(dc, profiles[i], one, pal), pal);
      }
      adc.DrawBitmap(bmp, 0, one.y * (int)i);
    }
  }
  return all.SaveFile(wx(file), wxBITMAP_TYPE_PNG);
}

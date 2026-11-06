#include "FragOrbDiagram.H"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include <wx/dc.h>
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>
#include <wx/dcmemory.h>

using std::string;
using std::vector;

namespace {

const char *MINUS = "\xE2\x88\x92";
const char *NDASH = "\xE2\x80\x93";
const int BAR_H = 5;
const int FO_W = 64, MO_W = 132;

string fmt(const char *f, double v)
{
  char b[64];
  snprintf(b, sizeof b, f, v);
  return b;
}

// A number with a real minus sign and no "-0.0".
string num(double v, const char *f = "%.3f")
{
  string s = fmt(f, v);
  if (s[0] == '-') {
    if (s.find_first_not_of("-0.") == string::npos) return s.substr(1);
    s = MINUS + s.substr(1);
  }
  return s;
}

string signedNum(double v, const char *f = "%.4f")
{
  const string s = num(v, f);
  return s.compare(0, 3, MINUS) == 0 ? s : "+" + s;
}

wxString wx(const string& s) { return wxString::FromUTF8(s.c_str()); }

string range(const vector<int>& n)
{
  if (n.empty()) return "";
  if (n.size() == 1) return std::to_string(n[0]);
  return std::to_string(n.front()) + NDASH + std::to_string(n.back());
}

string fragName(const FragDiagramResult& d, int f)
{
  return f >= 0 && f < (int)d.fragments.size() ? d.fragments[f] : "?";
}

string levelLabel(const FragDiagramResult& d, const FragDiagramLevel& l)
{
  if (l.fragment >= 0) return "FO " + range(l.numbers) + " " + l.position;
  string s = "MO " + range(l.numbers);
  if (l.kind == "bonding" || l.kind == "antibonding") s += " " + l.kind;
  else if (l.kind == "fragment" && !l.fragments.empty()) s += " " + fragName(d, l.fragments[0]);
  return s;
}

// Liang-Barsky clip: does the segment touch the closed rectangle?
bool segHits(double sx1, double sy1, double sx2, double sy2, const wxRect& r)
{
  const double x0 = r.x, y0 = r.y, x1 = r.x + r.width, y1 = r.y + r.height;
  const double dx = sx2 - sx1, dy = sy2 - sy1;
  const double p[4] = {-dx, dx, -dy, dy};
  const double q[4] = {sx1 - x0, x1 - sx1, sy1 - y0, y1 - sy1};
  double t0 = 0, t1 = 1;
  for (int i = 0; i < 4; i++) {
    if (p[i] == 0) { if (q[i] < 0) return false; continue; }
    const double t = q[i] / p[i];
    if (p[i] < 0) t0 = std::max(t0, t); else t1 = std::min(t1, t);
    if (t0 > t1) return false;
  }
  return true;
}

// A label keeps 3 px from every line, bar and other label (a line counts
// with its drawn width, as in tests/apps/profile_layout.py).
bool isFree(const FragDiagramLayout& L, const wxRect& r, const wxRect& bounds)
{
  if (!bounds.Contains(r)) return false;
  for (const FragDiagramLayout::Seg& s : L.segs) {
    wxRect p = r;
    p.Inflate(3 + (s.width + 1) / 2);
    if (segHits(s.x1, s.y1, s.x2, s.y2, p)) return false;
  }
  wxRect p = r;
  p.Inflate(3);
  for (const FragDiagramLayout::Text& t : L.texts)
    if (p.Intersects(t.r)) return false;
  return true;
}

bool leaderFree(const FragDiagramLayout& L, double x1, double y1, double x2, double y2)
{
  for (const FragDiagramLayout::Text& t : L.texts) {
    wxRect p = t.r;
    p.Inflate(2);
    if (segHits(x1, y1, x2, y2, p)) return false;
  }
  return true;
}

string elide(wxDC& dc, const string& s, int maxw)
{
  if (dc.GetTextExtent(wx(s)).x <= maxw) return s;
  string t = s;
  while (!t.empty()) {
    while (!t.empty() && (t.back() & 0xC0) == 0x80) t.pop_back();
    if (!t.empty()) t.pop_back();
    if (dc.GetTextExtent(wx(t + "\xE2\x80\xA6")).x <= maxw) return t + "\xE2\x80\xA6";
  }
  return "\xE2\x80\xA6";
}

// Words onto lines no wider than maxw.
vector<string> wrap(wxDC& dc, const string& s, int maxw)
{
  vector<string> out;
  string line, word;
  auto flush = [&]() {
    if (word.empty()) return;
    const string t = line.empty() ? word : line + " " + word;
    if (!line.empty() && dc.GetTextExtent(wx(t)).x > maxw) { out.push_back(line); line = word; }
    else line = t;
    word.clear();
  };
  for (char c : s) {
    if (c == ' ') flush();
    else word += c;
  }
  flush();
  if (!line.empty()) out.push_back(line);
  for (string& l : out) l = elide(dc, l, maxw);
  return out;
}

wxFont smallFont()
{
  wxFont f = *wxNORMAL_FONT;
  f.SetPointSize(std::max(7, f.GetPointSize() - 1));
  return f;
}

// Energy axis: shortened where nothing is drawn for more than breakEv, but
// never between the HOMO and LUMO levels, whose gap is the frontier.
struct Axis
{
  struct Part { double lo, hi; int bottom, top; };
  vector<Part> parts;
  double scale = 1;   // px per eV
  double y(double e) const
  {
    for (const Part& p : parts)
      if (e >= p.lo - 1e-9 && e <= p.hi + 1e-9) return p.bottom - (e - p.lo) * scale;
    return parts.empty() ? 0 : parts.front().bottom;
  }
};

Axis makeAxis(vector<double> e, double homo, double lumo, bool frontier, double breakEv,
              int bottom, int top, int breakPx)
{
  Axis a;
  std::sort(e.begin(), e.end());
  const double pad = 0.4;
  vector<std::pair<double, double> > runs;
  for (size_t i = 0; i < e.size(); i++) {
    const bool inGap = frontier && e[i - (i ? 1 : 0)] >= homo - 1e-6 && e[i] <= lumo + 1e-6;
    if (i == 0 || (e[i] - e[i - 1] > breakEv && !inGap)) runs.push_back({e[i], e[i]});
    else runs.back().second = e[i];
  }
  double span = 0;
  for (auto& r : runs) { r.first -= pad; r.second += pad; span += r.second - r.first; }
  const int nb = (int)runs.size() - 1;
  a.scale = std::max(1.0, (bottom - top - nb * breakPx) / std::max(span, 0.1));
  double y = bottom;
  for (const auto& r : runs) {
    Axis::Part p;
    p.lo = r.first;
    p.hi = r.second;
    p.bottom = (int)std::lround(y);
    y -= (r.second - r.first) * a.scale;
    p.top = (int)std::lround(y);
    y -= breakPx;
    a.parts.push_back(p);
  }
  return a;
}

}  // namespace


int FragDiagramLayout::levelAt(const wxPoint& p) const
{
  for (const auto& h : hits)
    if (h.first.Contains(p)) return h.second;
  return -1;
}


string FragDiagramLayout::dump(const FragDiagramResult& d) const
{
  string o = "size " + std::to_string(size.x) + " " + std::to_string(size.y) + "\n";
  for (const Text& t : texts)
    o += "text " + std::to_string(t.r.x) + " " + std::to_string(t.r.y) + " " +
         std::to_string(t.r.width) + " " + std::to_string(t.r.height) + " " + t.what + ": " +
         t.text + "\n";
  for (const Seg& s : segs)
    o += "seg " + fmt("%.1f", s.x1) + " " + fmt("%.1f", s.y1) + " " + fmt("%.1f", s.x2) + " " +
         fmt("%.1f", s.y2) + " " + std::to_string(s.width) + " " + s.what + "\n";
  if (channel < d.channels.size()) {
    const FragDiagramChannel& ch = d.channels[channel];
    vector<int> drawn;
    for (const Bar& b : bars)
      if (std::find(drawn.begin(), drawn.end(), b.level) == drawn.end()) drawn.push_back(b.level);
    std::sort(drawn.begin(), drawn.end());
    for (int id : drawn) {
      const FragDiagramLevel& l = ch.levels[id];
      string n;
      for (size_t i = 0; i < l.numbers.size(); i++) n += (i ? "," : "") + std::to_string(l.numbers[i]);
      int nb = 0;
      for (const Bar& b : bars) nb += b.level == id;
      o += "level " + std::to_string(id) + " " + std::to_string(l.column) + " " +
           std::to_string(l.fragment) + " " + fmt("%.6f", l.energy) + " " + n + " bars=" +
           std::to_string(nb) + "\n";
    }
    for (const Seg& s : segs)
      if (s.what.compare(0, 5, "line ") == 0) o += s.what + "\n";
  }
  if (!placed) o += "unplaced\n";
  return o;
}


FragDiagramLayout layoutFragDiagram(wxDC& dc, const FragDiagramResult& d, size_t channel,
                                    const FragDiagramView& v, const wxSize& size,
                                    const PlotPalette& pal)
{
  FragDiagramLayout L;
  L.size = size;
  L.channel = (int)channel;
  if (!d.ok || channel >= d.channels.size()) return L;
  const FragDiagramChannel& ch = d.channels[channel];
  const wxFont normal = *wxNORMAL_FONT, small = smallFont();
  auto ext = [&](const string& s, bool sm) {
    dc.SetFont(sm ? small : normal);
    return dc.GetTextExtent(wx(s));
  };
  const int lh = ext("Ag", false).y, sh = ext("Ag", true).y;
  const double maxOcc = d.channels.size() > 1 ? 1.0 : 2.0;
  const size_t nf = d.fragments.size();

  // Fixed texts.
  string head = v.heading.empty() ? "" : v.heading + ", ";
  head += (d.method == "scpa" ? "SCPA" : "Mulliken") + string(" shares; a line for a share of ") +
          fmt("%.0f", 100 * d.threshold) + " % or more";
  if (d.channels.size() > 1) head += ch.spin ? "; \xCE\xB2 orbitals" : "; \xCE\xB1 orbitals";
  string gap;
  if (ch.homo >= 0 && ch.lumo >= 0)
    gap = "HOMO" + string(NDASH) + "LUMO orbital-energy gap " + num(ch.gap, "%.2f") + " eV (MO " +
          range(ch.levels[ch.homo].numbers) + " to MO " + range(ch.levels[ch.lumo].numbers) + ")";
  vector<string> footer;
  dc.SetFont(small);
  footer.push_back("Bars: one per orbital; filled occupied, open empty, half filled singly occupied. "
                   "Line width: share under 25 %, 25" + string(NDASH) + "50 %, 50 % or more.");
  for (const string& n : ch.notes) footer.push_back(n);
  vector<string> footLines;
  for (const string& f : footer)
    for (const string& l : wrap(dc, f, size.x - 16)) footLines.push_back(l);
  dc.SetFont(normal);
  const vector<string> verdict = wrap(dc, ch.verdict, size.x - 16);

  // Label texts and lane widths.
  vector<string> label(ch.levels.size());
  vector<int> laneW(std::max<size_t>(nf, 1), 0);
  for (const FragDiagramLevel& l : ch.levels) {
    label[l.id] = levelLabel(d, l);
    if (l.fragment >= 0 && l.fragment < (int)nf)
      laneW[l.fragment] = std::max(laneW[l.fragment], ext(label[l.id], true).x);
  }

  vector<double> energies;
  for (const FragDiagramLevel& l : ch.levels) energies.push_back(l.energy);
  const bool frontier = ch.homo >= 0 && ch.lumo >= 0;
  const double eH = frontier ? ch.levels[ch.homo].energy : 0, eL = frontier ? ch.levels[ch.lumo].energy : 0;

  const double growth = 1.15;
  int height = size.y;
  for (int attempt = 0; attempt < 16; attempt++) {
    const bool last = attempt == 15;
    L = FragDiagramLayout();
    L.channel = (int)channel;
    L.size = wxSize(size.x, height);
    auto text = [&](int x, int y, const string& s, const wxColour& c, bool sm, const string& what,
                    int level = -1) {
      const wxSize e = ext(s, sm);
      FragDiagramLayout::Text t;
      t.r = wxRect(x, y, e.x, e.y);
      t.text = s;
      t.colour = c;
      t.small = sm;
      t.what = what;
      t.level = level;
      L.texts.push_back(t);
    };
    auto seg = [&](double x1, double y1, double x2, double y2, const wxColour& c, int w,
                   const string& what, bool dashed = false) {
      FragDiagramLayout::Seg s;
      s.x1 = x1; s.y1 = y1; s.x2 = x2; s.y2 = y2;
      s.colour = c; s.width = w; s.what = what; s.dashed = dashed;
      L.segs.push_back(s);
    };

    int y = 8;
    text(8, y, elide(dc, head, size.x - 16), pal.text, false, "title");
    y += lh + 2;
    for (const string& l : verdict) { text(8, y, l, pal.text, false, "verdict"); y += lh + 2; }
    if (!gap.empty()) { text(8, y, gap, pal.text, false, "gap"); y += lh + 2; }
    const int plotTop = y + 12;
    const int footH = 8 + lh + 8 + (int)footLines.size() * (sh + 2) + 6;
    const int plotBottom = height - footH;
    if (plotBottom - plotTop < 80) { height = (int)(height * growth); continue; }

    const Axis ax = makeAxis(energies, eH, eL, frontier, v.breakEv, plotBottom - 4, plotTop + 4, 26);

    // Ticks at least 2.2 lines apart; a part without one gets its own.
    const double raw = 2.2 * lh / ax.scale;
    const double mag = std::pow(10.0, std::floor(std::log10(raw)));
    double step = 10 * mag;
    for (double m : {1.0, 2.0, 5.0})
      if (m * mag >= raw) { step = m * mag; break; }
    const int dec = std::max(0, (int)-std::floor(std::log10(step) + 1e-9));
    const string tf = "%." + std::to_string(dec) + "f";
    vector<std::pair<double, string> > ticks;
    int tickW = 0;
    for (const Axis::Part& p : ax.parts) {
      size_t before = ticks.size();
      for (double t = std::ceil(p.lo / step) * step; t <= p.hi + 1e-9; t += step)
        ticks.push_back({t, num(std::fabs(t) < 1e-9 ? 0 : t, tf.c_str())});
      if (ticks.size() == before) {
        const double t = 0.5 * (p.lo + p.hi);
        ticks.push_back({t, num(t, "%.1f")});
      }
    }
    for (const auto& t : ticks) tickW = std::max(tickW, ext(t.second, false).x);
    const int axisX = 8 + lh + 8 + tickW + 8;

    // Columns: first fragment, complex, then one lane per other fragment.
    const int lw0 = nf ? laneW[0] : 0;
    int rightW = 0;
    for (size_t f = 1; f < nf; f++) rightW += FO_W + 8 + laneW[f] + 14;
    int gapW = 90;
    const int need = axisX + 12 + lw0 + 8 + FO_W + 2 * gapW + MO_W + rightW + 8;
    if (need < size.x) gapW += (size.x - need) / 2;
    const int x0 = axisX + 12 + lw0 + 8;          // left FO bars
    const int xc = x0 + FO_W + gapW;               // complex bars
    vector<int> xr(nf, 0);
    int xx = xc + MO_W + gapW;
    for (size_t f = 1; f < nf; f++) { xr[f] = xx; xx += FO_W + 8 + laneW[f] + 14; }
    if (nf) xr[0] = x0;
    L.size.x = std::max(size.x, xx);

    // Axis with its breaks.
    for (size_t i = 0; i < ax.parts.size(); i++) {
      seg(axisX, ax.parts[i].bottom, axisX, ax.parts[i].top, pal.axis, 1, "axis");
      if (i + 1 < ax.parts.size()) {
        const double yb = ax.parts[i].top - 9, yt = ax.parts[i + 1].bottom + 9;
        seg(axisX - 5, yb + 3, axisX + 5, yb - 3, pal.axis, 1, "break");
        seg(axisX - 5, yt + 3, axisX + 5, yt - 3, pal.axis, 1, "break");
      }
    }
    for (const auto& t : ticks) {
      const double ty = ax.y(t.first);
      seg(axisX - 5, ty, axisX, ty, pal.axis, 1, "tick");
      const wxSize e = ext(t.second, false);
      text(axisX - 8 - e.x, (int)std::lround(ty - e.y / 2.0), t.second, pal.text, false, "tick");
    }
    {
      const string al = "Orbital energy / eV";
      const wxSize e = ext(al, false);
      FragDiagramLayout::Text t;
      t.r = wxRect(8, (plotTop + plotBottom - e.x) / 2, e.y, e.x);
      t.text = al;
      t.colour = pal.text;
      t.vertical = true;
      t.what = "axis-label";
      L.texts.push_back(t);
    }

    // Bars (segments of their height, for the collision checks) and lines.
    auto groupX = [&](const FragDiagramLevel& l, int& a, int& w) {
      if (l.fragment < 0) { a = xc; w = MO_W; }
      else { a = l.fragment < (int)nf ? xr[l.fragment] : x0; w = FO_W; }
    };
    vector<double> ly(ch.levels.size());
    for (const FragDiagramLevel& l : ch.levels) {
      ly[l.id] = std::round(ax.y(l.energy)) + 0.5;
      int a, w;
      groupX(l, a, w);
      const int n = std::max<int>(1, (int)l.numbers.size());
      const double bw = (w - (n - 1) * 3.0) / n;
      const wxColour c = l.fragment < 0 ? pal.text : pal.series(l.fragment);
      for (int i = 0; i < n; i++) {
        const double bx = a + i * (bw + 3);
        FragDiagramLayout::Bar b;
        b.r = wxRect((int)std::lround(bx), (int)ly[l.id] - BAR_H / 2, std::max(2, (int)std::lround(bw)), BAR_H);
        b.colour = c;
        const double occ = i < (int)l.occupation.size() ? l.occupation[i] : 0;
        b.fill = std::min(1.0, std::max(0.0, occ / maxOcc));
        b.dashed = l.fragment < 0 && !l.representative;
        b.level = l.id;
        L.bars.push_back(b);
        seg(b.r.x, ly[l.id], b.r.x + b.r.width, ly[l.id], c, BAR_H, "bar " + std::to_string(l.id));
      }
      L.hits.push_back({wxRect(a - 2, (int)ly[l.id] - 5, w + 4, 10), l.id});
    }
    for (const FragDiagramLine& ln : ch.lines) {
      if (ln.fo < 0 || ln.mo < 0 || ln.fo >= (int)ch.levels.size() || ln.mo >= (int)ch.levels.size()) continue;
      const FragDiagramLevel& f = ch.levels[ln.fo];
      int a, w;
      groupX(f, a, w);
      const bool left = f.column == 0;
      const double fx = left ? a + w + 2 : a - 2;
      const double mx = left ? xc - 2 : xc + MO_W + 2;
      const int width = ln.share >= 0.5 ? 3 : ln.share >= 0.25 ? 2 : 1;
      seg(fx, ly[ln.fo], mx, ly[ln.mo], pal.series(ln.fragment), width,
          "line " + std::to_string(ln.fo) + " " + std::to_string(ln.mo) + " " + fmt("%.6f", ln.share));
    }

    // Column names under the plot.
    const int namesY = plotBottom + 8;
    auto name = [&](int cx, int w, const string& s, const wxColour& c) {
      dc.SetFont(normal);
      const string t = elide(dc, s, w);
      text(cx - ext(t, false).x / 2, namesY, t, c, false, "column");
    };
    if (nf) name(x0 + FO_W / 2, lw0 + FO_W + 40, fragName(d, 0), pal.series(0));
    name(xc + MO_W / 2, MO_W + 60, "complex", pal.text);
    for (size_t f = 1; f < nf; f++) name(xr[f] + FO_W / 2, FO_W + laneW[f], fragName(d, (int)f), pal.series(f));
    int fy = namesY + lh + 8;
    for (const string& l : footLines) { text(8, fy, l, pal.text, true, "note"); fy += sh + 2; }

    // Level labels, lowest energy first, each at the nearest free place.
    const wxRect bounds(axisX + 4, plotTop, L.size.x - axisX - 8, plotBottom - plotTop);
    vector<int> order;
    for (const FragDiagramLevel& l : ch.levels) order.push_back(l.id);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return ly[a] > ly[b]; });
    bool ok = true;
    for (int id : order) {
      const FragDiagramLevel& l = ch.levels[id];
      const wxSize e = ext(label[id], true);
      int a, w;
      groupX(l, a, w);
      const int cy = (int)ly[id];
      bool done = false;
      auto put = [&](int x, int y0, bool leader, double lx1, double lx2) {
        text(x, y0, label[id], l.fragment < 0 ? pal.text : pal.series(l.fragment), true,
             "label " + std::to_string(id), id);
        if (leader) seg(lx1, cy, lx2, y0 + e.y / 2.0, pal.axis, 1, "leader " + std::to_string(id));
        L.hits.push_back({L.texts.back().r, id});
        done = true;
      };
      if (l.fragment < 0) {
        // Under or over the bars (no line ever enters the column between
        // them), else beside the bars where no line passes.
        const int cx = a + (w - e.x) / 2;
        vector<wxPoint> c = {wxPoint(cx, cy + BAR_H / 2 + 5), wxPoint(cx, cy - BAR_H / 2 - 5 - e.y)};
        for (int k : {0, -1, 1})
          for (int side : {1, -1})
            c.push_back(wxPoint(side > 0 ? a + w + 8 : a - 8 - e.x, cy - e.y / 2 + k * (e.y / 2)));
        for (const wxPoint& p : c)
          if (!done && isFree(L, wxRect(p, e), bounds)) put(p.x, p.y, false, 0, 0);
      } else {
        const bool left = l.column == 0;
        const int x = left ? a - 10 - e.x : a + w + 10;
        const double edge = left ? a - 2 : a + w + 2, end = left ? x + e.x + 2 : x - 2;
        for (int k = 0; k <= 40 && !done; k++)
          for (int sgn : {1, -1}) {
            if (done || (k == 0 && sgn < 0)) continue;
            const int y0 = cy - e.y / 2 + sgn * k * (e.y / 2 + 1);
            if (!isFree(L, wxRect(x, y0, e.x, e.y), bounds)) continue;
            if (k > 0 && !leaderFree(L, edge, cy, end, y0 + e.y / 2.0)) continue;
            put(x, y0, k > 0, edge, end);
          }
      }
      if (!done) {
        ok = false;
        if (last) {
          const int x = l.fragment < 0 ? a + (w - e.x) / 2 : l.column == 0 ? a - 10 - e.x : a + w + 10;
          text(x, l.fragment < 0 ? cy + 6 : cy - e.y / 2, label[id], pal.text, true,
               "label " + std::to_string(id), id);
        }
      }
    }
    if (ok) return L;
    if (last) { L.placed = false; return L; }
    height = (int)(height * growth);
  }
  L.placed = false;
  return L;
}


void paintFragDiagram(wxDC& dc, const FragDiagramLayout& L, const PlotPalette& pal, int highlight)
{
  dc.SetBackground(wxBrush(pal.background));
  dc.Clear();
  const wxFont normal = *wxNORMAL_FONT, small = smallFont();
  for (const FragDiagramLayout::Seg& s : L.segs) {
    if (s.what.compare(0, 4, "bar ") == 0) continue;
    int w = s.width;
    if (highlight >= 0 && s.what.compare(0, 5, "line ") == 0) {
      int fo = -1, mo = -1;
      sscanf(s.what.c_str() + 5, "%d %d", &fo, &mo);
      if (fo == highlight || mo == highlight) w += 2;
    }
    wxPen pen(s.colour, w, s.dashed ? wxPENSTYLE_SHORT_DASH : wxPENSTYLE_SOLID);
    pen.SetCap(wxCAP_BUTT);
    dc.SetPen(pen);
    dc.DrawLine(wxRound(s.x1), wxRound(s.y1), wxRound(s.x2), wxRound(s.y2));
  }
  for (const FragDiagramLayout::Bar& b : L.bars) {
    wxPen pen(b.colour, b.level == highlight ? 2 : 1, b.dashed ? wxPENSTYLE_SHORT_DASH : wxPENSTYLE_SOLID);
    dc.SetPen(pen);
    dc.SetBrush(wxBrush(pal.background));
    dc.DrawRectangle(b.r);
    if (b.fill > 0.01) {
      dc.SetPen(*wxTRANSPARENT_PEN);
      dc.SetBrush(wxBrush(b.colour, b.dashed ? wxBRUSHSTYLE_BDIAGONAL_HATCH : wxBRUSHSTYLE_SOLID));
      dc.DrawRectangle(b.r.x, b.r.y, std::max(1, (int)std::lround(b.r.width * b.fill)), b.r.height);
    }
  }
  for (const FragDiagramLayout::Text& t : L.texts) {
    dc.SetFont(t.small ? small : normal);
    dc.SetTextForeground(t.colour);
    if (t.vertical) dc.DrawRotatedText(wx(t.text), t.r.x, t.r.y + t.r.height, 90);
    else dc.DrawText(wx(t.text), t.r.x, t.r.y);
  }
}


string fragDiagramLevelText(const FragDiagramResult& d, size_t channel, int level)
{
  if (channel >= d.channels.size()) return "";
  const FragDiagramChannel& ch = d.channels[channel];
  if (level < 0 || level >= (int)ch.levels.size()) return "";
  const FragDiagramLevel& l = ch.levels[level];
  const size_t n = l.numbers.size();
  string occ;
  bool same = true;
  for (double o : l.occupation) same = same && std::fabs(o - l.occupation[0]) < 1e-6;
  if (!l.occupation.empty() && same) occ = "occupation " + fmt("%.3f", l.occupation[0]) + (n > 1 ? " each" : "");
  else {
    occ = "occupations";
    for (double o : l.occupation) occ += " " + fmt("%.3f", o);
  }
  string t;
  const string orbs = n > 1 ? " (" + std::to_string(n) + " degenerate orbitals)" : "";
  const string pct = fmt("%.0f %%", 100 * d.threshold);
  if (l.fragment < 0) {
    t += "MO " + range(l.numbers) + orbs + ", " + l.position + ", " + num(l.energy) + " eV, " + occ + "\n";
    string frs;
    for (size_t i = 0; i < l.fragments.size(); i++)
      frs += (i ? (i + 1 == l.fragments.size() ? " and " : ", ") : "") + fragName(d, l.fragments[i]);
    const string op = signedNum(l.overlapPopulation);
    if (l.kind == "bonding" || l.kind == "antibonding")
      t += "Lines from " + frs + ": " + l.kind + "; the overlap population between the fragments in this MO is " +
           op + " per electron (its OPDOS contribution; " + (l.overlapPopulation > 0 ? "positive" : "negative") + ").\n";
    else if (l.kind == "fragment")
      t += "Lines from " + frs + " only; overlap population between the fragments " + op + " per electron.\n";
    else
      t += "No FO level has a share of " + pct + " or more; overlap population between the fragments " + op +
           " per electron.\n";
    t += "Sum of the shares of its lines " + fmt("%.1f %%", 100 * l.covered) + "; outside all FOs " +
         fmt("%.1f %%", std::fabs(l.remainder) < 5e-4 ? 0.0 : 100 * l.remainder) + ". " +
         (l.representative ? "Represented by the diagram (" + fmt("%.0f %%", 100 * d.covered) + " or more)."
                           : "Not represented by the diagram: under " + fmt("%.0f %%", 100 * d.covered) +
                             ", drawn dashed.") + "\n";
    t += l.partner ? "Outside the window: drawn because it shares an FO level with a drawn occupied MO level.\n"
                   : "In the window of MO levels around the HOMO and LUMO.\n";
    t += "FO levels with 1 % or more (Mulliken shares can be negative or exceed 100 %):\n";
    for (const FragDiagramShare& s : l.shares)
      t += "  " + fmt("%6.1f %%", 100 * s.share) + "  " + fragName(d, s.fragment) + " FO " + range(s.numbers) +
           " " + s.position + "  " + num(s.energy) + " eV" + (s.share >= d.threshold ? "  (line)" : "") +
           (s.level < 0 ? "  (not drawn)" : "") + "\n";
  } else {
    t += fragName(d, l.fragment) + " FO " + range(l.numbers) + orbs + ", " + l.position + " of the fragment, " +
         num(l.energy) + " eV, " + occ + "\n";
    t += "Its share in each complex MO level, 1 % or more (a line where " + pct + " or more):\n";
    for (const FragDiagramShare& s : l.shares)
      t += "  " + fmt("%6.1f %%", 100 * s.share) + "  MO " + range(s.numbers) + " " + s.position + "  " +
           num(s.energy) + " eV" + (s.level < 0 ? "  (not drawn)" : "") + "\n";
  }
  return t;
}


FragDiagramLayout exportFragDiagram(const FragDiagramResult& d, size_t channel, const FragDiagramView& v)
{
  wxBitmap bmp(FRAG_DIAGRAM_PNG.x, FRAG_DIAGRAM_PNG.y);
  wxMemoryDC mdc(bmp);
  wxGCDC dc(mdc);
  return layoutFragDiagram(dc, d, channel, v, FRAG_DIAGRAM_PNG, PlotPalette::light());
}


bool saveFragDiagramPng(const FragDiagramResult& d, size_t channel, const FragDiagramView& v,
                        const string& file)
{
  const FragDiagramLayout l = exportFragDiagram(d, channel, v);
  wxBitmap bmp(l.size.x, l.size.y);
  {
    wxMemoryDC mdc(bmp);
    wxGCDC dc(mdc);
    const PlotPalette pal = PlotPalette::light();
    const FragDiagramLayout again = layoutFragDiagram(dc, d, channel, v, FRAG_DIAGRAM_PNG, pal);
    paintFragDiagram(dc, again, pal, -1);
  }
  return bmp.SaveFile(wx(file), wxBITMAP_TYPE_PNG);
}


FragDiagramCanvas::FragDiagramCanvas(wxWindow *parent)
  : wxScrolledCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxHSCROLL)
{
  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetScrollRate(10, 10);
  Bind(wxEVT_PAINT, &FragDiagramCanvas::onPaint, this);
  Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { relayout(); e.Skip(); });
  auto at = [this](wxMouseEvent& e) {
    return p_layout.levelAt(CalcUnscrolledPosition(e.GetPosition()));
  };
  Bind(wxEVT_MOTION, [this, at](wxMouseEvent& e) {
    const int l = at(e);
    if (l != p_hover) {
      p_hover = l;
      Refresh();
      if (l >= 0 && p_onLevel) p_onLevel(l, false);
    }
    e.Skip();
  });
  Bind(wxEVT_LEFT_DOWN, [this, at](wxMouseEvent& e) {
    const int l = at(e);
    if (l >= 0 && p_onLevel) p_onLevel(l, true);
    e.Skip();
  });
}


void FragDiagramCanvas::setDiagram(const FragDiagramResult& d, size_t channel, const FragDiagramView& v)
{
  p_data = d;
  p_channel = channel;
  p_view = v;
  p_hover = -1;
  relayout();
}


void FragDiagramCanvas::relayout()
{
  wxClientDC cdc(this);
  wxGCDC dc(cdc);
  const wxSize s = GetClientSize();
  p_layout = layoutFragDiagram(dc, p_data, p_channel, p_view, wxSize(std::max(s.x, 500), std::max(s.y, 400)),
                               PlotPalette::system());
  SetVirtualSize(p_layout.size);
  Refresh();
}


void FragDiagramCanvas::onPaint(wxPaintEvent&)
{
  wxAutoBufferedPaintDC pdc(this);
  DoPrepareDC(pdc);
  wxGCDC dc(pdc);
  paintFragDiagram(dc, p_layout, PlotPalette::system(), p_hover);
}

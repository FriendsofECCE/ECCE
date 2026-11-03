/**
 * @file
 *
 * The MO Diagram panel's picture of a single atom: the aufbau view.
 *
 * There is nothing to correlate for one atom, so the panel draws its
 * subshells (1s, 2s, 2p, ...) with their electrons instead.  Two drawings:
 * boxes per subshell in energy order and not to scale (B, the default), and
 * orbital energies to scale on an eV axis with the core below a break (A),
 * where the splitting of a partly filled subshell in a ROKS/ROHF atom shows.
 */
#ifndef MOAUFBAUCANVAS_HH
#define MOAUFBAUCANVAS_HH

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <wx/bitmap.h>
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/panel.h>

#include "MoAufbauModel.H"
#include "MoDiagramCanvas.H"

using std::string;
using std::vector;

class MoAufbauCanvas : public wxPanel
{
  public:
    MoAufbauCanvas(wxWindow *parent)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxFULL_REPAINT_ON_RESIZE), p_owner(0), p_scale(false), p_tip(-1)
    {
      SetBackgroundStyle(wxBG_STYLE_PAINT);
      SetBackgroundColour(*wxWHITE);
      Bind(wxEVT_PAINT, &MoAufbauCanvas::onPaint, this);
      Bind(wxEVT_LEFT_DOWN, &MoAufbauCanvas::onClick, this);
      Bind(wxEVT_MOTION, &MoAufbauCanvas::onMotion, this);
    }

    void setClickHandler(MoDiagramClickTarget *owner) { p_owner = owner; }
    void setModel(const string& symbol, const vector<AufbauShell>& s,
                  const string& note = string())
    { p_symbol = symbol; p_shells = s; p_note = note; Refresh(); }
    void setToScale(bool scale) { p_scale = scale; Refresh(); }
    bool toScale() const { return p_scale; }
    bool empty() const { return p_shells.empty(); }
    const vector<AufbauShell>& model() const { return p_shells; }
    string symbol() const { return p_symbol; }

    /** What was drawn, one line per orbital, for scripts and tests. */
    string describe() const
    {
      string r = "config " + aufbau::configuration(p_symbol, p_shells, true) + "\n";
      for (const AufbauShell& s : p_shells) {
        char buf[200];
        snprintf(buf, sizeof buf, "shell %s core=%d electrons=%d mean=%.2f\n",
                 s.label.c_str(), s.core ? 1 : 0, s.electrons, s.mean);
        r += buf;
        for (const AufbauOrbital& o : s.orbs) {
          snprintf(buf, sizeof buf, "  orbital %d eV=%.2f up=%d down=%d\n",
                   o.index, o.eV, o.up, o.down);
          r += buf;
        }
      }
      return r;
    }

    bool savePng(const string& path, bool scale, int w, int h)
    {
      const bool was = p_scale;
      p_scale = scale;
      wxBitmap bmp(w, h);
      {
        wxMemoryDC dc(bmp);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();
        paintOnto(dc, wxSize(w, h));
      }
      p_scale = was;
      p_hits.clear();
      return bmp.SaveFile(wxString(path.c_str(), wxConvUTF8), wxBITMAP_TYPE_PNG);
    }

    void paintOnto(wxDC& dc, const wxSize& size)
    {
      p_hits.clear();
      dc.SetFont(*wxNORMAL_FONT);
      dc.SetTextForeground(wxColour(30, 30, 30));
      if (p_shells.empty()) {
        dc.DrawText(wxString((p_note.empty() ? "No orbital shells to draw." : p_note).c_str(),
                             wxConvUTF8), 20, 20);
        return;
      }
      const int lineH = dc.GetTextExtent(wxT("Xg")).y;
      //  The configuration is the panel's text: the top line.
      text(dc, aufbau::configuration(p_symbol, p_shells), 12, 8);
      text(dc, p_scale ? "orbital energies to scale (eV)" : "not to scale", 12,
           size.y - lineH - 6);
      const wxRect plot(0, 8 + lineH + 14, size.x, size.y - (8 + lineH + 14) - lineH - 14);
      if (p_scale) drawScaled(dc, plot); else drawBoxes(dc, plot);
    }

  private:
    struct Hit { wxRect box; int orbital; string tip; };

    static void text(wxDC& dc, const string& s, int x, int y)
    { dc.DrawText(wxString(s.c_str(), wxConvUTF8), x, y); }

    static void arrow(wxDC& dc, int x, int yTop, int yBot, bool up)
    {
      dc.DrawLine(x, yTop, x, yBot);
      const int tip = up ? yTop : yBot, dir = up ? 1 : -1;
      dc.DrawLine(x, tip, x - 3, tip + dir * 4);
      dc.DrawLine(x, tip, x + 3, tip + dir * 4);
    }

    void addHit(const wxRect& box, const AufbauShell& s, const AufbauOrbital& o)
    {
      char buf[120];
      snprintf(buf, sizeof buf, "%s, orbital %d: %.2f eV", s.label.c_str(),
               o.index + 1, o.eV);
      Hit h; h.box = box; h.orbital = o.index; h.tip = buf;
      p_hits.push_back(h);
    }

    //  Drawing B: a row per subshell, lowest at the bottom.
    void drawBoxes(wxDC& dc, const wxRect& plot)
    {
      const int n = (int)p_shells.size();
      const int rowH = std::min(64, plot.height / std::max(n, 1));
      const int boxW = 36, boxH = std::min(34, rowH - 6);
      for (int r = 0; r < n; r++) {
        const AufbauShell& s = p_shells[(size_t)r];
        const int yb = plot.GetBottom() - r * rowH;
        const int top = yb - boxH - 2;
        dc.SetTextForeground(wxColour(30, 30, 30));
        text(dc, s.label, 14, top + (boxH - dc.GetTextExtent(wxT("Xg")).y) / 2);
        //  Ordered like the textbook: the doubly filled first.
        vector<const AufbauOrbital*> order;
        for (const AufbauOrbital& o : s.orbs) order.push_back(&o);
        std::stable_sort(order.begin(), order.end(),
            [](const AufbauOrbital *a, const AufbauOrbital *b)
            { return a->up + a->down > b->up + b->down; });
        for (size_t k = 0; k < order.size(); k++) {
          const AufbauOrbital& o = *order[k];
          const wxRect box(110 + (int)k * (boxW + 4), top, boxW, boxH);
          dc.SetPen(wxPen(wxColour(60, 60, 60), 1));
          dc.SetBrush(*wxTRANSPARENT_BRUSH);
          dc.DrawRectangle(box);
          drawArrows(dc, box.x + boxW / 2, top + 3, top + boxH - 3, o);
          addHit(box, s, o);
        }
      }
    }

    static void drawArrows(wxDC& dc, int cx, int yTop, int yBot, const AufbauOrbital& o)
    {
      dc.SetPen(wxPen(*wxBLUE, 2));
      if (o.up) arrow(dc, cx - 5, yTop, yBot, true);
      if (o.down) arrow(dc, cx + 5, yTop, yBot, false);
    }

    //  Drawing A: eV axis, valence to scale, the core in a band below a break.
    void drawScaled(wxDC& dc, const wxRect& plot)
    {
      double lo = 1e9, hi = -1e9;
      int cores = 0;
      for (const AufbauShell& s : p_shells) {
        if (s.core) { cores++; continue; }
        for (const AufbauOrbital& o : s.orbs) { lo = std::min(lo, o.eV); hi = std::max(hi, o.eV); }
      }
      if (hi < lo) { lo = -1; hi = 1; }
      const double pad = std::max(1.0, 0.08 * (hi - lo));
      lo -= pad; hi += pad;
      const int coreH = cores ? std::min(40 + 30 * cores, plot.height / 3) : 0;
      const int axisX = 64;
      const int valTop = plot.y + 6, valBot = plot.GetBottom() - coreH - (cores ? 18 : 0);
      auto yFor = [&](double eV) {
        return valBot - (int)((eV - lo) / (hi - lo) * (valBot - valTop));
      };
      dc.SetPen(wxPen(wxColour(60, 60, 60), 1));
      dc.DrawLine(axisX, valTop, axisX, valBot);
      //  Ticks at a round step of eV.
      double step;
      step = (hi - lo) / 9 > 5 ? 10 : (hi - lo) / 9 > 2 ? 5 : (hi - lo) / 9 > 1 ? 2 : 1;
      for (double v = std::ceil(lo / step) * step; v <= hi; v += step) {
        const int y = yFor(v);
        dc.DrawLine(axisX - 4, y, axisX, y);
        char buf[32]; snprintf(buf, sizeof buf, "%g", v);
        text(dc, buf, axisX - 8 - dc.GetTextExtent(wxString(buf)).x, y - 8);
      }
      if (cores) {
        const int brk = valBot + 9;
        dc.DrawLine(axisX, valBot, axisX, brk - 3);
        dc.DrawLine(axisX - 6, brk - 1, axisX + 6, brk - 5);
        dc.DrawLine(axisX - 6, brk + 3, axisX + 6, brk - 1);
        dc.DrawLine(axisX, brk + 3, axisX, plot.GetBottom());
        text(dc, "core levels, not to scale", axisX + 12, brk + 2);
      }

      //  One column per angular type, shells of that type stacked in it.
      int maxL = 0;
      for (const AufbauShell& s : p_shells) maxL = std::max(maxL, s.l);
      vector<int> colX((size_t)maxL + 1, 0);
      int x = axisX + 100;
      for (int l = 0; l <= maxL; l++) {
        int slots = 0;
        for (const AufbauShell& s : p_shells) if (s.l == l) slots = std::max(slots, (int)s.orbs.size());
        colX[(size_t)l] = x;
        x += slots * 34 + (slots ? 70 : 0);
      }
      int coreSeen = 0;
      const int coreTop = valBot + 46;
      for (const AufbauShell& s : p_shells) {
        int yLevel[16];
        for (size_t k = 0; k < s.orbs.size() && k < 16; k++)
          yLevel[k] = s.core ? coreTop + (coreSeen) * std::max(24, (plot.GetBottom() - coreTop) / std::max(cores, 1))
                             : yFor(s.orbs[k].eV);
        if (s.core) coreSeen++;
        int yMin = 1 << 30;
        for (size_t k = 0; k < s.orbs.size() && k < 16; k++) {
          const AufbauOrbital& o = s.orbs[k];
          const wxRect bar(colX[(size_t)s.l] + (int)k * 34, yLevel[k] - 1, 30, 3);
          dc.SetPen(wxPen(wxColour(60, 60, 60), 2));
          dc.DrawLine(bar.x, yLevel[k], bar.x + 30, yLevel[k]);
          drawArrows(dc, bar.x + 15, yLevel[k] - 17, yLevel[k] - 2, o);
          addHit(wxRect(bar.x, yLevel[k] - 19, 30, 22), s, o);
          yMin = std::min(yMin, yLevel[k]);
        }
        dc.SetTextForeground(wxColour(30, 30, 30));
        const int yMid = s.core ? yLevel[0] : yFor(s.mean);
        text(dc, s.label, colX[(size_t)s.l] - 6 - dc.GetTextExtent(wxString::FromUTF8(s.label.c_str())).x, yMid - 9);
        if (s.core) {
          char buf[40]; snprintf(buf, sizeof buf, "%.1f eV", s.mean);
          text(dc, buf, colX[(size_t)s.l] + (int)s.orbs.size() * 34 + 4, yMid - 9);
        }
      }
    }

    void onPaint(wxPaintEvent&)
    {
      wxAutoBufferedPaintDC dc(this);
      dc.SetBackground(*wxWHITE_BRUSH);
      dc.Clear();
      paintOnto(dc, GetClientSize());
    }

    int hitAt(const wxPoint& p) const
    {
      for (size_t i = 0; i < p_hits.size(); i++)
        if (p_hits[i].box.Contains(p)) return (int)i;
      return -1;
    }

    void onClick(wxMouseEvent& event)
    {
      event.Skip();
      const int h = hitAt(event.GetPosition());
      if (h >= 0 && p_owner != 0) p_owner->orbitalClicked(p_hits[(size_t)h].orbital);
    }

    void onMotion(wxMouseEvent& event)
    {
      event.Skip();
      const int h = hitAt(event.GetPosition());
      SetCursor(h >= 0 ? wxCursor(wxCURSOR_HAND) : wxNullCursor);
      if (h == p_tip) return;
      p_tip = h;
      if (h < 0) UnsetToolTip();
      else SetToolTip(wxString::FromUTF8(p_hits[(size_t)h].tip.c_str()));
    }

    MoDiagramClickTarget *p_owner;
    bool p_scale;
    string p_symbol, p_note;
    vector<AufbauShell> p_shells;
    vector<Hit> p_hits;
    int p_tip;
};

#endif

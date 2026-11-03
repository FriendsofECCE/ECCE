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
    { p_symbol = symbol; p_shells = s; p_note = note; p_exportLayout.clear(); Refresh(); }
    void setToScale(bool scale) { p_scale = scale; p_exportLayout.clear(); Refresh(); }
    bool toScale() const { return p_scale; }
    bool empty() const { return p_shells.empty(); }
    const vector<AufbauShell>& model() const { return p_shells; }
    string symbol() const { return p_symbol; }

    /** What was drawn, one line per orbital, for scripts and tests. */
    string describe()
    {
      if (p_exportLayout.empty()) {
        const wxSize size = GetClientSize();
        if (size.x > 0 && size.y > 0) {
          wxBitmap bmp(size.x, size.y);
          wxMemoryDC dc(bmp);
          paintOnto(dc, size);
        }
      }
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
      return r + (p_exportLayout.empty() ? p_layout : p_exportLayout);
    }

    bool savePng(const string& path, bool scale, int w, int h)
    {
      const bool was = p_scale;
      const vector<Hit> hits = p_hits;
      const string layout = p_layout;
      p_scale = scale;
      wxBitmap bmp(w, h);
      {
        wxMemoryDC dc(bmp);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();
        paintOnto(dc, wxSize(w, h));
      }
      p_exportLayout = p_layout;
      p_layout = layout;
      p_scale = was;
      p_hits = hits;
      return bmp.SaveFile(wxString(path.c_str(), wxConvUTF8), wxBITMAP_TYPE_PNG);
    }

    void paintOnto(wxDC& dc, const wxSize& size)
    {
      p_hits.clear();
      p_layout = "layout " + std::to_string(size.x) + " " + std::to_string(size.y) + "\n";
      dc.SetFont(*wxNORMAL_FONT);
      dc.SetTextForeground(wxColour(30, 30, 30));
      if (p_shells.empty()) {
        dc.DrawText(wxString((p_note.empty() ? "No orbital shells to draw." : p_note).c_str(),
                             wxConvUTF8), 20, 20);
        return;
      }
      wxFont configFont = dc.GetFont();
      configFont.SetPointSize(configFont.GetPointSize() + 3);
      dc.SetFont(configFont);
      const string config = aufbau::configuration(p_symbol, p_shells);
      const wxSize extent = dc.GetTextExtent(wxString::FromUTF8(config.c_str()));
      text(dc, config, std::max(8, (size.x - extent.x) / 2), 8);
      dc.SetFont(*wxNORMAL_FONT);
      const int lineH = dc.GetTextExtent(wxT("Xg")).y;
      text(dc, p_scale ? "orbital energies to scale (eV)" : "not to scale", 12,
           size.y - lineH - 6);
      const int top = 8 + extent.y + 14;
      const wxRect plot(12, top, size.x - 24, size.y - top - lineH - 18);
      if (p_scale) drawScaled(dc, plot); else drawBoxes(dc, plot);
    }

  private:
    struct Hit { wxRect box; int orbital; string tip; };

    void rectangle(const string& kind, const string& label, const wxRect& r)
    {
      p_layout += "rect " + kind + " " + std::to_string(r.x) + " " +
          std::to_string(r.y) + " " + std::to_string(r.width) + " " +
          std::to_string(r.height) + " " + label + "\n";
    }

    void text(wxDC& dc, const string& s, int x, int y)
    {
      const wxString value = wxString::FromUTF8(s.c_str());
      rectangle("text", s, wxRect(wxPoint(x, y), dc.GetTextExtent(value)));
      dc.DrawText(value, x, y);
    }

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
      const int picked = hitAt(wxPoint(box.x + box.width/2, box.y + box.height/2));
      p_layout += "hit " + std::to_string(o.index) + " " +
          std::to_string(p_hits[picked].orbital) + "\n";
    }

    // Shell columns share energy ranks; measured labels reserve their own space.
    void drawBoxes(wxDC& dc, const wxRect& plot)
    {
      const int rows = (int)p_shells.size();
      const int lineH = dc.GetTextExtent(wxT("Xg")).y;
      int maxN = 1, labelW = 0;
      for (const AufbauShell& s : p_shells) {
        maxN = std::max(maxN, s.n);
        labelW = std::max(labelW, dc.GetTextExtent(wxString::FromUTF8(s.label.c_str())).x);
      }
      const int gap = 3, labelGap = 4, colGap = 10;
      const int arrowW = lineH + 28;
      auto widths = [&](int box) {
        vector<int> widths(maxN, dc.GetTextExtent(wxT("n = 1")).x);
        for (const AufbauShell& s : p_shells) {
          int width = 0;
          for (int l = 0; l <= s.l; ++l)
            width += labelW + labelGap + (2*l+1)*(box+gap) + colGap;
          widths[s.n-1] = std::max(widths[s.n-1], width - colGap);
        }
        return widths;
      };
      int box = std::min(40, (plot.height - lineH - 16) / std::max(1, rows) - 8);
      box = std::max(14, box);
      vector<int> colW;
      int total = 0;
      for (;;) {
        colW = widths(box);
        total = arrowW + colGap * (maxN - 1);
        for (int w : colW) total += w;
        if (total <= plot.width || box == 14) break;
        --box;
      }
      const int rowH = std::max(std::max(box, lineH) + 8,
          std::min(64, (plot.height - lineH - 16) / std::max(1, rows)));
      const int height = lineH + 16 + rows * rowH;
      const int left = plot.x + (plot.width - total) / 2;
      const int top = plot.y + std::max(0, (plot.height - height) / 2);
      vector<int> colX(maxN, left);
      for (int n = 1; n < maxN; ++n) colX[n] = colX[n-1] + colW[n-1] + colGap;
      for (int n = 1; n <= maxN; ++n) {
        const string heading = "n = " + std::to_string(n);
        const int w = dc.GetTextExtent(wxString::FromUTF8(heading.c_str())).x;
        text(dc, heading, colX[n-1] + (colW[n-1] - w)/2, top);
      }
      for (int r = 0; r < rows; ++r) {
        const AufbauShell& s = p_shells[r];
        int x = colX[s.n-1];
        for (int l = 0; l < s.l; ++l)
          x += labelW + labelGap + (2*l+1)*(box+gap) + colGap;
        const int y = top + lineH + 16 + (rows - 1 - r)*rowH;
        const wxColour colour = s.electrons ? wxColour(30,30,30) : wxColour(145,145,145);
        dc.SetTextForeground(colour);
        const int w = dc.GetTextExtent(wxString::FromUTF8(s.label.c_str())).x;
        text(dc, s.label, x + labelW - w, y + (box-lineH)/2);
        x += labelW + labelGap;
        vector<const AufbauOrbital*> order;
        for (const AufbauOrbital& o : s.orbs) order.push_back(&o);
        std::stable_sort(order.begin(), order.end(),
            [](const AufbauOrbital *a, const AufbauOrbital *b)
            { return a->up + a->down > b->up + b->down; });
        for (size_t k = 0; k < order.size(); ++k) {
          const wxRect rect(x + (int)k*(box+gap), y, box, box);
          dc.SetPen(wxPen(colour, 1, s.electrons ? wxPENSTYLE_SOLID : wxPENSTYLE_SHORT_DASH));
          dc.SetBrush(*wxTRANSPARENT_BRUSH);
          dc.DrawRectangle(rect);
          rectangle("box", s.label, rect);
          drawArrows(dc, rect.x + box/2, y+3, y+box-3, *order[k], box);
          addHit(rect, s, *order[k]);
        }
      }
      dc.SetTextForeground(wxColour(80,80,80));
      dc.SetPen(wxPen(wxColour(80,80,80), 1));
      const int ax = left + total - arrowW + 8;
      const int ay = top + lineH + 16;
      arrow(dc, ax, ay, ay + rows*rowH - 8, true);
      const wxString caption = wxT("energy (order)");
      const wxSize e = dc.GetTextExtent(caption);
      const int cy = ay + (rows*rowH + e.x)/2;
      rectangle("text", "energy (order)", wxRect(ax+8, cy-e.x, e.y, e.x));
      dc.DrawRotatedText(caption, ax+8, cy, 90);
    }

    static void drawArrows(wxDC& dc, int cx, int yTop, int yBot, const AufbauOrbital& o, int width)
    {
      const int offset = std::min(5, std::max(2, (width - 8)/2));
      dc.SetPen(wxPen(*wxBLUE, 2));
      if (o.up) arrow(dc, cx - offset, yTop, yBot, true);
      if (o.down) arrow(dc, cx + offset, yTop, yBot, false);
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
      const int lineH = dc.GetTextExtent(wxT("Xg")).y;
      const int coreRow = std::max(26, lineH + 8);
      const int coreH = cores ? 30 + cores * coreRow : 0;
      const int axisX = plot.x + 46;
      const int valTop = plot.y + 24, valBot = plot.GetBottom() - coreH - (cores ? 18 : 0);
      if (valBot - valTop < lineH) return;
      auto yFor = [&](double eV) {
        return valBot - (int)((eV - lo) / (hi - lo) * (valBot - valTop));
      };
      dc.SetPen(wxPen(wxColour(60, 60, 60), 1));
      dc.DrawLine(axisX, valTop, axisX, valBot);
      //  Ticks at a round step of eV.
      double step;
      step = (hi - lo) / 9 > 5 ? 10 : (hi - lo) / 9 > 2 ? 5 : (hi - lo) / 9 > 1 ? 2 : 1;
      while ((valBot-valTop)*step/(hi-lo) < lineH+4) step *= 2;
      for (double v = std::ceil(lo / step) * step; v <= hi; v += step) {
        const int y = yFor(v);
        dc.DrawLine(axisX - 4, y, axisX, y);
        char buf[32]; snprintf(buf, sizeof buf, "%g", v);
        text(dc, buf, axisX - 8 - dc.GetTextExtent(wxString(buf)).x, y - lineH/2);
      }
      if (cores) {
        const int brk = valBot + 9;
        rectangle("break", "axis", wxRect(axisX-6, brk-5, 13, 9));
        dc.DrawLine(axisX, valBot, axisX, brk - 3);
        dc.DrawLine(axisX - 6, brk - 1, axisX + 6, brk - 5);
        dc.DrawLine(axisX - 6, brk + 3, axisX + 6, brk - 1);
        dc.DrawLine(axisX, brk + 3, axisX, plot.GetBottom());
        text(dc, "core levels, not to scale", axisX + 20, brk + 8);
      }

      //  One column per angular type, shells of that type stacked in it.
      int maxL = 0;
      for (const AufbauShell& s : p_shells) maxL = std::max(maxL, s.l);
      vector<int> labelW(maxL+1, 0), slots(maxL+1, 0), colX(maxL+1, 0);
      auto caption = [](const AufbauShell& s) {
        if (!s.core) return s.label;
        char buf[64]; snprintf(buf, sizeof buf, " (%.1f eV)", s.mean);
        return s.label + buf;
      };
      for (const AufbauShell& s : p_shells) {
        labelW[s.l] = std::max(labelW[s.l], dc.GetTextExtent(wxString::FromUTF8(caption(s).c_str())).x);
        slots[s.l] = std::max(slots[s.l], (int)s.orbs.size());
      }
      int fixed = 0, count = 0;
      for (int l = 0; l <= maxL; ++l) {
        fixed += labelW[l] + 16;
        count += slots[l];
      }
      const int available = plot.GetRight() - axisX - 16;
      const int pitch = std::max(14, std::min(34, (available-fixed)/std::max(1,count)));
      const int spare = std::max(0, available-fixed-count*pitch);
      int x = axisX + 16;
      for (int l = 0; l <= maxL; ++l) {
        colX[l] = x + labelW[l] + 6;
        x += labelW[l] + 16 + slots[l]*pitch + spare/std::max(1,maxL);
      }
      int coreSeen = 0;
      vector<int> previousLabel(maxL+1, plot.GetBottom()+lineH+4);
      // The model is sorted from low to high energy, including the core.
      for (const AufbauShell& s : p_shells) {
        const int coreY = plot.GetBottom() - 4 - coreSeen*coreRow;
        if (s.core) ++coreSeen;
        for (size_t k = 0; k < s.orbs.size(); ++k) {
          const AufbauOrbital& o = s.orbs[k];
          const int y = s.core ? coreY : yFor(o.eV);
          const wxRect bar(colX[s.l] + (int)k*pitch, y-1, pitch-4, 3);
          dc.SetPen(wxPen(wxColour(60,60,60), 2));
          dc.DrawLine(bar.x, y, bar.x+bar.width, y);
          rectangle("level", s.label, bar);
          drawArrows(dc, bar.x+bar.width/2, y-19, y-3, o, bar.width);
          addHit(wxRect(bar.x, y-21, bar.width, 24), s, o);
        }
        dc.SetTextForeground(wxColour(30,30,30));
        const string label = caption(s);
        const int w = dc.GetTextExtent(wxString::FromUTF8(label.c_str())).x;
        const int desired = (s.core ? coreY : yFor(s.mean)) - lineH/2;
        const int y = std::min(desired, previousLabel[s.l] - lineH - 4);
        text(dc, label, colX[s.l]-6-w, y);
        previousLabel[s.l] = y;
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
    string p_layout, p_exportLayout;
};

#endif

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
#include <sstream>
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
      // Keep every subshell in the configuration even when the drawing folds a core.
      std::istringstream words(config);
      string word, line;
      vector<string> lines;
      while (words >> word) {
        const string next = line.empty() ? word : line + " " + word;
        if (!line.empty() && dc.GetTextExtent(wxString::FromUTF8(next.c_str())).x > size.x-16) {
          lines.push_back(line);
          line = word;
        } else line = next;
      }
      if (!line.empty()) lines.push_back(line);
      int top = 8;
      for (const string& value : lines) {
        const wxSize extent = dc.GetTextExtent(wxString::FromUTF8(value.c_str()));
        text(dc, value, std::max(8, (size.x - extent.x) / 2), top);
        top += extent.y;
      }
      dc.SetFont(*wxNORMAL_FONT);
      const int lineH = dc.GetTextExtent(wxT("Xg")).y;
      if (!p_scale) text(dc, "not to scale", 12, size.y - lineH - 6);
      top += 14;
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

    // Share energy ranks across wraps; reserve each column's entire vertical span
    // because a lower-n d shell can lie above the next shell's s level.
    void drawBoxes(wxDC& dc, const wxRect& plot)
    {
      const int lineH = dc.GetTextExtent(wxT("Xg")).y;
      const int gap = 3, labelGap = 4, colGap = 10, rowGap = 2;
      const int arrowW = lineH + 28;
      int maxN = 1, labelW = 0;
      for (const AufbauShell& s : p_shells) {
        maxN = std::max(maxN, s.n);
        labelW = std::max(labelW, dc.GetTextExtent(wxString::FromUTF8(s.label.c_str())).x);
      }
      struct Column { int n, first, last; wxRect bounds; };
      vector<Column> columns;
      vector<int> shown;
      string core;
      int box = 18, rowH = 22, height = 0, width = 0, wraps = 0;
      auto layout = [&](int first, int candidate, bool wrap) {
        columns.clear(); shown.clear(); wraps = 0;
        box = candidate;
        rowH = std::max(box, lineH) + rowGap;
        for (int r = first; r < (int)p_shells.size(); ++r) shown.push_back(r);
        const int rows = (int)shown.size();
        height = rows*rowH + lineH + 4 + (core.empty() ? 0 : lineH+12);
        if (height > plot.height) return false;
        int nextX = 0;
        width = 0;
        for (int n = 1; n <= maxN; ++n) {
          Column c; c.n = n; c.first = rows; c.last = -1;
          int w = dc.GetTextExtent(wxString::FromUTF8(("n = " + std::to_string(n)).c_str())).x;
          for (int r = 0; r < rows; ++r) {
            const AufbauShell& s = p_shells[shown[r]];
            if (s.n != n) continue;
            c.first = std::min(c.first, r); c.last = r;
            w = std::max(w, labelW + labelGap + (int)s.orbs.size()*(box+gap)-gap);
          }
          if (c.last < 0) continue;
          c.bounds = wxRect(nextX, (rows-1-c.last)*rowH, w,
                            (c.last-c.first)*rowH + box + 4 + lineH);
          if (c.bounds.GetRight()+1 > plot.width-arrowW && wrap) {
            c.bounds.x = 0;
            ++wraps;
          }
          // A wrap may cross an earlier column whose levels extend higher.
          bool collision;
          do {
            collision = false;
            for (const Column& old : columns) {
              if (!c.bounds.Intersects(old.bounds)) continue;
              c.bounds.x = old.bounds.GetRight()+1+colGap;
              collision = true;
            }
          } while (collision);
          if (c.bounds.GetRight()+1 > plot.width-arrowW) return false;
          nextX = c.bounds.GetRight()+1+colGap;
          width = std::max(width, nextX-colGap);
          columns.push_back(c);
        }
        if (!core.empty()) width = std::max(width, dc.GetTextExtent(wxString::FromUTF8(core.c_str())).x+16);
        width += arrowW;
        return width <= plot.width;
      };
      auto fit = [&](int first) {
        for (bool wrap : {false, true})
          for (int candidate = 40; candidate >= 18; --candidate)
            if (layout(first, candidate, wrap)) return true;
        return false;
      };
      bool fitted = fit(0);
      if (!fitted) {
        const char *names[] = {"He", "Ne", "Ar", "Kr", "Xe"};
        const char *filled[] = {"1s", "2s 2p", "3s 3p", "4s 3d 4p", "5s 4d 5p"};
        vector<string> labels;
        for (int k = 0; k < 5 && !fitted; ++k) {
          std::istringstream tokens(filled[k]);
          string label;
          while (tokens >> label) labels.push_back(label);
          int first = 0;
          for (const AufbauShell& s : p_shells) {
            if (std::find(labels.begin(), labels.end(), s.label) == labels.end() ||
                (int)s.orbs.size() != 2*s.l+1 || s.electrons != 2*(2*s.l+1)) break;
            ++first;
          }
          if (first != (int)labels.size() || first == (int)p_shells.size()) continue;
          core = "[" + string(names[k]) + "]";
          fitted = fit(first);
        }
      }
      if (!fitted) return;
      p_layout += "boxes size=" + std::to_string(box) + " wraps=" + std::to_string(wraps) +
          " core=" + (core.empty() ? "none" : core) + "\n";
      const int left = plot.x + (plot.width-width)/2;
      const int top = plot.y + (plot.height-height)/2;
      const int rows = (int)shown.size();
      for (const Column& c : columns) {
        const string heading = "n = " + std::to_string(c.n);
        const int w = dc.GetTextExtent(wxString::FromUTF8(heading.c_str())).x;
        dc.SetTextForeground(wxColour(30,30,30));
        text(dc, heading, left+c.bounds.x+(c.bounds.width-w)/2,
             top+(rows-1-c.first)*rowH+box+4);
        for (int r = c.first; r <= c.last; ++r) {
          const AufbauShell& s = p_shells[shown[r]];
          if (s.n != c.n) continue;
          int x = left+c.bounds.x;
          const int y = top+(rows-1-r)*rowH;
          const wxColour colour = s.electrons ? wxColour(30,30,30) : wxColour(145,145,145);
          dc.SetTextForeground(colour);
          const int w = dc.GetTextExtent(wxString::FromUTF8(s.label.c_str())).x;
          text(dc, s.label, x+labelW-w, y+(box-lineH)/2);
          x += labelW+labelGap;
          vector<const AufbauOrbital*> order;
          for (const AufbauOrbital& o : s.orbs) order.push_back(&o);
          std::stable_sort(order.begin(), order.end(),
              [](const AufbauOrbital *a, const AufbauOrbital *b)
              { return a->up+a->down > b->up+b->down; });
          for (size_t k = 0; k < order.size(); ++k) {
            const wxRect rect(x+(int)k*(box+gap), y, box, box);
            dc.SetPen(wxPen(colour, 1, s.electrons ? wxPENSTYLE_SOLID : wxPENSTYLE_SHORT_DASH));
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(rect);
            rectangle("box", s.label, rect);
            drawArrows(dc, rect.x+box/2, y+3, y+box-3, *order[k], box);
            addHit(rect, s, *order[k]);
          }
        }
      }
      dc.SetTextForeground(wxColour(80,80,80));
      dc.SetPen(wxPen(wxColour(80,80,80), 1));
      if (!core.empty()) {
        const wxSize e = dc.GetTextExtent(wxString::FromUTF8(core.c_str()));
        const wxRect rect(left, top+rows*rowH+lineH+4, e.x+16, lineH+8);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(rect);
        // Record the label's enclosing box once, so overlap checks include its border.
        rectangle("core", core, rect);
        dc.DrawText(wxString::FromUTF8(core.c_str()), rect.x+8, rect.y+4);
      }
      const int ax = left+width-arrowW+8;
      arrow(dc, ax, top, top+rows*rowH-4, true);
      const wxString caption = wxT("energy (order)");
      const wxSize e = dc.GetTextExtent(caption);
      const int cy = top+(rows*rowH+e.x)/2;
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
      const int coreRow = std::max(23, lineH + 4);
      const int coreH = cores ? 18 + cores * coreRow : 0;
      wxFont valueFont = dc.GetFont();
      valueFont.SetPointSize(std::max(8, valueFont.GetPointSize()-2));
      dc.SetFont(valueFont);
      const string coreTitle = "core (not to scale)";
      const int coreTitleW = dc.GetTextExtent(wxString::FromUTF8(coreTitle.c_str())).x;
      dc.SetFont(*wxNORMAL_FONT);
      const int axisX = plot.x + std::max(46, cores ? coreTitleW+14 : 46);
      text(dc, "Energy (eV)", axisX-24, plot.y);
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
        if (y-lineH/2 >= valTop)
          text(dc, buf, axisX - 8 - dc.GetTextExtent(wxString(buf)).x, y - lineH/2);
      }
      if (cores) {
        const int brk = valBot + 9;
        rectangle("break", "axis", wxRect(axisX-6, brk-5, 13, 9));
        dc.DrawLine(axisX, valBot, axisX, brk - 3);
        dc.DrawLine(axisX - 6, brk - 1, axisX + 6, brk - 5);
        dc.DrawLine(axisX - 6, brk + 3, axisX + 6, brk - 1);
        dc.DrawLine(axisX, brk + 3, axisX, plot.GetBottom());
        dc.SetFont(valueFont);
        text(dc, coreTitle, plot.x, brk+8);
        dc.SetFont(*wxNORMAL_FONT);
      }

      //  One column per angular type, shells of that type stacked in it.
      int maxL = 0;
      for (const AufbauShell& s : p_shells) maxL = std::max(maxL, s.l);
      auto energyText = [](double value) {
        char buf[64];
        snprintf(buf, sizeof buf, "%.1f eV", std::fabs(value) < 0.05 ? 0.0 : std::fabs(value));
        return (value <= -0.05 ? string("\xE2\x88\x92") : string()) + buf;
      };
      vector<int> labelW(maxL+1, 0), valueW(maxL+1, 0), slots(maxL+1, 0), colX(maxL+1, 0);
      for (const AufbauShell& s : p_shells) {
        labelW[s.l] = std::max(labelW[s.l], dc.GetTextExtent(wxString::FromUTF8(s.label.c_str())).x);
        slots[s.l] = std::max(slots[s.l], (int)s.orbs.size());
        dc.SetFont(valueFont);
        for (const AufbauOrbital& o : s.orbs)
          valueW[s.l] = std::max(valueW[s.l], dc.GetTextExtent(wxString::FromUTF8(energyText(o.eV).c_str())).x);
        dc.SetFont(*wxNORMAL_FONT);
      }
      const int available = plot.GetRight() - axisX - 16;
      int pitch = 28, gap = 0, groupW = 0;
      bool reserveValues = true;
      // Reserve annotations within each column, then centre the whole group.
      // A narrow panel reduces the gap and level width rather than separating sets.
      for (; pitch >= 12; --pitch) {
        int fixed = 0;
        for (int l = 0; l <= maxL; ++l)
          fixed += labelW[l]+6+slots[l]*pitch+4+valueW[l];
        gap = maxL ? std::max(0, std::min({160, 2*(pitch-4), (available-fixed)/maxL})) : 0;
        groupW = fixed+maxL*gap;
        if (groupW <= available) break;
      }
      if (groupW > available) {
        // Values are optional; keep every labelled angular column inside the panel.
        reserveValues = false;
        std::fill(valueW.begin(), valueW.end(), 0);
        pitch = 12;
        groupW = 0;
        for (int l = 0; l <= maxL; ++l)
          groupW += labelW[l]+6+slots[l]*pitch+4;
      }
      pitch = std::max(12, pitch);
      int x = axisX + 16 + std::max(0, (available-groupW)/2);
      for (int l = 0; l <= maxL; ++l) {
        colX[l] = x + labelW[l] + 6;
        x += labelW[l]+6+slots[l]*pitch+4+valueW[l]+gap;
      }
      struct Value { string label; wxRect bounds; };
      vector<Value> values;
      vector<wxRect> occupied;
      int coreSeen = 0;
      vector<int> labelY(p_shells.size());
      for (int l = 0; l <= maxL; ++l) {
        int previous = valTop-lineH-4;
        for (int j = (int)p_shells.size()-1; j >= 0; --j) {
          const AufbauShell& s = p_shells[j];
          if (s.l != l || s.core) continue;
          labelY[j] = std::max(yFor(s.mean)-lineH/2, previous+lineH+4);
          previous = labelY[j];
        }
      }
      // The model is sorted from low to high energy, including the core.
      for (const AufbauShell& s : p_shells) {
        const int coreY = plot.GetBottom() - lineH/2 - 2 - coreSeen*coreRow;
        if (s.core) ++coreSeen;
        for (size_t k = 0; k < s.orbs.size(); ++k) {
          const AufbauOrbital& o = s.orbs[k];
          const int y = s.core ? coreY : yFor(o.eV);
          const wxRect bar(colX[s.l] + (int)k*pitch, y-1, pitch-4, 3);
          dc.SetPen(wxPen(wxColour(60,60,60), 2));
          dc.DrawLine(bar.x, y, bar.x+bar.width, y);
          rectangle("level", s.label, bar);
          drawArrows(dc, bar.x+bar.width/2, y-19, y-3, o, bar.width);
          const wxRect hit(bar.x, y-10, bar.width, 21);
          addHit(hit, s, o);
          occupied.push_back(hit);
          // Degenerate orbitals share one value after their last line.
          if (reserveValues && (k+1 == s.orbs.size() || std::fabs(s.orbs[k+1].eV-o.eV) > 0.0005)) {
            const string value = energyText(o.eV);
            dc.SetFont(valueFont);
            const wxSize extent = dc.GetTextExtent(wxString::FromUTF8(value.c_str()));
            values.push_back({value, wxRect(bar.GetRight()+7, y-extent.y/2, extent.x, extent.y)});
            dc.SetFont(*wxNORMAL_FONT);
          }
        }
        dc.SetTextForeground(wxColour(30,30,30));
        const string label = s.label;
        const int w = dc.GetTextExtent(wxString::FromUTF8(label.c_str())).x;
        const int y = s.core ? coreY-lineH/2 : labelY[&s-p_shells.data()];
        text(dc, label, colX[s.l]-6-w, y);
        occupied.push_back(wxRect(colX[s.l]-6-w, y, w, lineH));
      }
      dc.SetFont(valueFont);
      dc.SetTextForeground(wxColour(110,110,110));
      for (const Value& value : values) {
        if (!plot.Contains(value.bounds)) continue;
        bool collision = false;
        for (const wxRect& r : occupied)
          if (value.bounds.Intersects(r)) { collision = true; break; }
        if (collision) continue;
        text(dc, value.label, value.bounds.x, value.bounds.y);
        wxRect reserved = value.bounds;
        reserved.Inflate(2, 2);
        occupied.push_back(reserved);
      }
      dc.SetFont(*wxNORMAL_FONT);
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
      int nearest = -1;
      int distance = 0;
      // Close energy levels share a forgiving hit area; select the nearest centre.
      for (size_t i = 0; i < p_hits.size(); i++) {
        const wxRect& box = p_hits[i].box;
        if (!box.Contains(p)) continue;
        const int d = std::abs(p.y-(box.y+box.height/2));
        if (nearest < 0 || d < distance) { nearest = (int)i; distance = d; }
      }
      return nearest;
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

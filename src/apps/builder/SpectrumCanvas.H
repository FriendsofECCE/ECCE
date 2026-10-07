/**
 * @file
 *
 * The surface an IR / Raman spectrum is drawn on (#214).
 *
 * Draws a VibSpectrum and nothing else: what the sticks are, what the
 * broadening does and which wavenumbers are on screen are decided in
 * tdat/VibSpectrum.H.  Defined entirely inline, like MoDiagramCanvas.H,
 * so tools/spectrum/render can paint the real canvas onto a bitmap and
 * the layout can be looked at without a desktop session.
 */

#ifndef SPECTRUMCANVAS_H_
#define SPECTRUMCANVAS_H_

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/panel.h>
#include <wx/settings.h>

#include "tdat/VibSpectrum.H"
#include "wxgui/PlotStyle.H"

/** Told when a stick is clicked. */
class SpectrumClickTarget
{
  public:
    virtual ~SpectrumClickTarget() {}
    /** `mode` is the zero-based row of the frequency table. */
    virtual void modeClicked(int mode) = 0;
};

class SpectrumCanvas;

/**
 * What several views of one spectrum share: the model, the zoom, the
 * selected mode.  A canvas made with shareWith() draws and edits the same
 * one, so the docked panel and the pop-out window cannot disagree.
 */
struct SpectrumShared
{
  VibSpectrum  spec;
  SpectrumAxis axis;
  bool         showSticks;
  int          selected;
  int          version;                  // bumped when the data is replaced
  std::vector<SpectrumCanvas*> views;
  SpectrumShared() : showSticks(true), selected(-1), version(0) {}
};

class SpectrumCanvas : public wxPanel
{
  public:

    SpectrumCanvas(wxWindow *parent)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxWANTS_CHARS),
        p_owner(0), p_palette(SpectrumPalette::system()),
        p_s(new SpectrumShared), p_kindShown(VIB_IR), p_compactBelow(320),
        p_hoverKind(VIB_IR),
        p_hoverMode(-1), p_hoverAt(0, 0), p_pressed(false),
        p_dragging(false), p_panning(false), p_band(false),
        p_bandLo(0), p_bandHi(0), p_cacheValid(false)
    {
      p_s->views.push_back(this);
      SetBackgroundStyle(wxBG_STYLE_PAINT);
      SetMinSize(wxSize(240, 220));
      Bind(wxEVT_PAINT, &SpectrumCanvas::onPaint, this);
      Bind(wxEVT_LEFT_DOWN, &SpectrumCanvas::onLeftDown, this);
      Bind(wxEVT_LEFT_UP, &SpectrumCanvas::onLeftUp, this);
      Bind(wxEVT_LEFT_DCLICK, &SpectrumCanvas::onDoubleClick, this);
      Bind(wxEVT_RIGHT_DOWN, &SpectrumCanvas::onPanDown, this);
      Bind(wxEVT_MIDDLE_DOWN, &SpectrumCanvas::onPanDown, this);
      Bind(wxEVT_RIGHT_UP, &SpectrumCanvas::onPanUp, this);
      Bind(wxEVT_MIDDLE_UP, &SpectrumCanvas::onPanUp, this);
      Bind(wxEVT_MOTION, &SpectrumCanvas::onMotion, this);
      Bind(wxEVT_MOUSEWHEEL, &SpectrumCanvas::onWheel, this);
      Bind(wxEVT_KEY_DOWN, &SpectrumCanvas::onKey, this);
      Bind(wxEVT_LEAVE_WINDOW, &SpectrumCanvas::onLeave, this);
      Bind(wxEVT_MOUSE_CAPTURE_LOST, &SpectrumCanvas::onCaptureLost, this);
    }

    ~SpectrumCanvas()
    {
      std::vector<SpectrumCanvas*>& v = p_s->views;
      v.erase(std::remove(v.begin(), v.end(), this), v.end());
    }

    /** Draw and edit the same model, zoom and selection as `other`. */
    void shareWith(SpectrumCanvas& other)
    {
      std::vector<SpectrumCanvas*>& v = p_s->views;
      v.erase(std::remove(v.begin(), v.end(), this), v.end());
      p_s = other.p_s;
      p_s->views.push_back(this);
      p_cacheValid = false;
      Refresh();
    }

    /** Every view of the shared state repaints. */
    virtual void Refresh(bool erase = true, const wxRect *rect = NULL)
    {
      wxPanel::Refresh(erase, rect);
      for (size_t i = 0; i < p_s->views.size(); i++)
        if (p_s->views[i] != this) p_s->views[i]->wxPanel::Refresh(erase);
    }

    /**
     * Below this canvas height one pane is drawn, with a toggle when there
     * are two, and no envelope axis.  0 means always the full view.
     */
    void setCompactBelow(int height) { p_compactBelow = height; Refresh(); }
    /** Which pane the compact layout shows. */
    void setCompactPane(VibKind kind) { p_kindShown = kind; Refresh(); }

    void setClickHandler(SpectrumClickTarget *owner) { p_owner = owner; }

    void setPalette(const SpectrumPalette& palette)
    {
      p_palette = palette;
      Refresh();
    }

    /** Replace the data.  Keeps the current FWHM and scale. */
    void setSpectrum(const VibSpectrum& spectrum)
    {
      const double fwhm = p_s->spec.fwhm(), scale = p_s->spec.scale();
      const LineShape shape = p_s->spec.lineShape();
      p_s->spec = spectrum;
      p_s->spec.setFwhm(fwhm);
      p_s->spec.setScale(scale);
      p_s->spec.setLineShape(shape);
      p_s->selected = -1;
      p_s->version++;
      p_hoverMode = -1;
      fitRange();
    }
    const VibSpectrum& spectrum() const { return p_s->spec; }

    void setFwhm(double fwhm)
    {
      p_s->spec.setFwhm(fwhm);
      if (!p_s->axis.zoomed()) fitRange();
      p_cacheValid = false;
      Refresh();
    }
    void setScale(double scale)
    {
      p_s->spec.setScale(scale);
      fitRange();
    }
    void setLineShape(LineShape shape)
    {
      p_s->spec.setLineShape(shape);
      p_cacheValid = false;
      Refresh();
    }

    /** Whether sticks are drawn under the broadened curve. */
    void setShowSticks(bool show) { p_s->showSticks = show; Refresh(); }
    bool showSticks() const { return p_s->showSticks; }

    /** High wavenumbers on the left (the default) or on the right. */
    void setReversed(bool reversed)
    {
      p_s->axis.setReversed(reversed);
      Refresh();
    }
    bool reversed() const { return p_s->axis.reversed(); }

    SpectrumAxis& axis() { return p_s->axis; }
    void resetZoom() { p_s->axis.reset(); p_cacheValid = false; Refresh(); }

    /** The mode (table row) drawn highlighted, or -1. */
    void setSelected(int mode) { p_s->selected = mode; Refresh(); }
    int  selected() const { return p_s->selected; }

    //  For the offscreen render: state that otherwise comes from the mouse.
    void setHover(VibKind kind, int mode, const wxPoint& at)
    {
      p_hoverKind = kind; p_hoverMode = mode; p_hoverAt = at;
    }
    void setBand(double lo, double hi)
    {
      p_band = true; p_bandLo = lo; p_bandHi = hi;
    }

    /** Where a stick was drawn at the last paint; for tests and render. */
    bool stickPosition(VibKind kind, int mode, wxPoint *top) const
    {
      for (size_t i = 0; i < p_hits.size(); i++)
        if (p_hits[i].kind == kind && p_hits[i].mode == mode) {
          *top = wxPoint(p_hits[i].x, p_hits[i].top);
          return true;
        }
      return false;
    }

    void paintOnto(wxDC& dc, const wxSize& size)
    {
      p_hits.clear();
      p_panes.clear();
      dc.SetBackground(wxBrush(p_palette.background));
      dc.Clear();
      dc.SetFont(*wxSMALL_FONT);
      dc.SetTextForeground(p_palette.text);

      const bool haveIr = p_s->spec.has(VIB_IR), haveRaman = p_s->spec.has(VIB_RAMAN);
      const bool any = (haveIr || haveRaman) && !p_s->spec.sticks(
                         haveIr ? VIB_IR : VIB_RAMAN).empty();
      if (!any) {
        const wxString msg = wxString::FromUTF8(
            "This calculation has no infrared or Raman intensities.");
        dc.SetFont(*wxNORMAL_FONT);
        wxSize e = dc.GetTextExtent(msg);
        dc.DrawText(msg, (size.x - e.x) / 2, (size.y - e.y) / 2);
        return;
      }

      std::vector<VibKind> kinds;
      if (haveIr) kinds.push_back(VIB_IR);
      if (haveRaman) kinds.push_back(VIB_RAMAN);

      //  A short canvas (the docked panel) shows one pane, chosen with a
      //  toggle, and no envelope axis; the pop-out window shows it all.
      const bool compact = p_compactBelow > 0 && size.y < p_compactBelow;
      const bool toggle = compact && kinds.size() > 1;
      p_toggle[0] = p_toggle[1] = wxRect();
      if (compact && kinds.size() > 1) {
        const VibKind keep = p_kindShown;
        kinds.assign(1, keep == VIB_RAMAN && haveRaman ? VIB_RAMAN : VIB_IR);
        if (kinds[0] == VIB_IR && !haveIr) kinds[0] = VIB_RAMAN;
        p_kindShown = kinds[0];
      }

      const bool envelope = p_s->spec.fwhm() > 0;
      const int ch = dc.GetCharHeight();
      const int gap = ch + 6;

      //  Y scales first: the margins depend on how wide the labels are.
      std::vector<PaneScale> scales;
      for (size_t k = 0; k < kinds.size(); k++)
        scales.push_back(scaleFor(kinds[k], envelope, compact));

      int leftLabel = 0, rightLabel = 0;
      for (size_t k = 0; k < scales.size(); k++) {
        leftLabel = std::max(leftLabel, labelWidth(dc, scales[k].left));
        if (scales[k].hasRight)
          rightLabel = std::max(rightLabel, labelWidth(dc, scales[k].right));
      }
      //  The rotated titles are drawn in the larger font.
      dc.SetFont(*wxNORMAL_FONT);
      const int th = dc.GetCharHeight();
      dc.SetFont(*wxSMALL_FONT);
      const int left = th + 16 + leftLabel;
      const int right = scales[0].hasRight ? th + 16 + rightLabel : 14;
      const int top = compact ? 4 : 10;
      const bool imaginary = hasImaginary() && !compact;
      const bool xTitle = !compact || size.y >= 140;
      const int bottom = compact
          ? ch + 12 + (xTitle ? ch + 6 : 0)
          : 2 * ch + 20 + (imaginary ? ch + 4 : 0);

      const int plotW = std::max(40, size.x - left - right);
      const int plotH = size.y - top - bottom;
      const int n = (int)kinds.size();
      const int paneH = std::max(30, (plotH - gap * (n - 1)) / n);

      p_plotLeft = left;
      p_plotWidth = plotW;

      const SpectrumTicks xt = niceTicks(p_s->axis.lo(), p_s->axis.hi(),
                                         std::max(3, plotW / (compact ? 110 : 70)));

      for (int k = 0; k < n; k++) {
        const wxRect pane(left, top + k * (paneH + gap), plotW, paneH);
        p_panes.push_back(pane);
        paintPane(dc, pane, kinds[k], scales[k], xt, k == n - 1);
      }

      //  X axis title under the bottom pane.
      const wxRect last = p_panes.back();
      wxString xtitle = wxString::FromUTF8("Wavenumber (cm\xE2\x81\xBB\xC2\xB9");
      if (std::fabs(p_s->spec.scale() - 1.0) > 1e-9)
        xtitle += wxString::Format(", scaled by %g", p_s->spec.scale());
      xtitle += ")";
      dc.SetFont(*wxNORMAL_FONT);
      dc.SetTextForeground(p_palette.text);
      wxSize te = dc.GetTextExtent(xtitle);
      if (xTitle)
        dc.DrawText(xtitle, last.x + (last.width - te.x) / 2,
                    last.GetBottom() + ch + (compact ? 6 : 10));
      if (toggle) paintToggle(dc, last, haveIr, haveRaman);
      if (imaginary) {
        dc.SetFont(*wxSMALL_FONT);
        dc.SetTextForeground(p_palette.imaginary);
        dc.DrawText(wxString::FromUTF8("Imaginary frequencies are drawn at "
                                       "their negative wavenumber."),
                    last.x, last.GetBottom() + 2 * ch + 14);
        dc.SetTextForeground(p_palette.text);
      }

      if (p_hoverMode >= 0) paintReadout(dc, size);
    }

    /** What a click at this point does: tell the owner which mode. */
    bool clickAt(const wxPoint& p)
    {
      const Hit *h = hitAt(p);
      if (h == 0 || p_owner == 0) return false;
      p_owner->modeClicked(h->mode);
      return true;
    }

  private:

    //  One y axis: its top, ticks and title.
    struct Axis
    {
      SpectrumYAxis y;
      wxString      title;
    };
    struct PaneScale
    {
      Axis left, right;
      bool hasRight;      // envelope on its own axis, sticks on the left
      bool sticksOnLeft;  // false: the left axis is the envelope's
    };
    struct Hit { VibKind kind; int mode; int x, top, base; };

    static wxString utf8(const char *s) { return wxString::FromUTF8(s); }

    static int labelWidth(wxDC& dc, const Axis& a)
    {
      int w = 0;
      for (size_t i = 0; i < a.y.ticks.values.size(); i++)
        w = std::max(w, dc.GetTextExtent(
            wxString::Format("%.*f", a.y.ticks.decimals,
                             a.y.ticks.values[i])).x);
      return w;
    }

    bool hasImaginary() const
    {
      for (int k = 0; k < 2; k++) {
        const std::vector<VibStick> s = p_s->spec.sticks((VibKind)k);
        for (size_t i = 0; i < s.size(); i++)
          if (s[i].imaginary && s[i].wavenumber >= p_s->axis.lo() &&
              s[i].wavenumber <= p_s->axis.hi()) return true;
      }
      return false;
    }

    void fitRange()
    {
      double lo, hi;
      p_s->spec.defaultRange(&lo, &hi);
      p_s->axis.setFull(lo, hi);
      p_cacheValid = false;
      p_band = false;
      Refresh();
    }

    //  The y scales follow the visible window, so a band zoomed into is
    //  never a flat line next to a tall one that is off screen.  The
    //  envelope search is the expensive part; it is redone only when the
    //  window or the line changes.
    PaneScale scaleFor(VibKind kind, bool envelope, bool compact)
    {
      const double key[7] = { p_s->axis.lo(), p_s->axis.hi(), p_s->spec.fwhm(),
                              p_s->spec.scale(), (double)p_s->spec.lineShape(),
                              (double)p_s->showSticks, (double)p_s->version };
      if (!p_cacheValid || !std::equal(key, key + 7, p_cacheKey)) {
        std::copy(key, key + 7, p_cacheKey);
        for (int k = 0; k < 2; k++) {
          p_maxStick[k] = p_s->spec.maxStick((VibKind)k, p_s->axis.lo(), p_s->axis.hi());
          p_maxEnv[k] = p_s->spec.maxEnvelope((VibKind)k, p_s->axis.lo(), p_s->axis.hi());
        }
        p_cacheValid = true;
      }
      PaneScale s;
      const wxString name = kind == VIB_IR ? "IR" : "Raman";
      const wxString unit = utf8(p_s->spec.unitLabel(kind).c_str());
      const wxString inv = utf8(" per cm\xE2\x81\xBB\xC2\xB9");
      const wxString rel = p_s->spec.relative(kind) ? wxString("relative")
                                                 : unit;
      s.hasRight = envelope && p_s->showSticks && !compact;
      s.sticksOnLeft = !envelope || p_s->showSticks;
      Axis sticks, env;
      sticks.y = niceYAxis(p_maxStick[kind], compact ? 3 : 4);
      sticks.title = utf8(p_s->spec.axisLabel(kind).c_str());
      env.y = niceYAxis(p_maxEnv[kind], compact ? 3 : 4);
      env.title = name + " envelope (" + rel + inv + ")";
      if (s.sticksOnLeft) { s.left = sticks; s.right = env; }
      else { s.left = env; s.right = sticks; }
      return s;
    }

    static void drawVerticalText(wxDC& dc, const wxString& text, int cx,
                                 int cy, int maxLength, bool leftSide)
    {
      wxString t = text;
      wxSize e = dc.GetTextExtent(t);
      //  A title longer than the pane would run off its ends; the unit
      //  is the part worth keeping.
      int open = t.Find('(');
      if (e.x > maxLength && open != wxNOT_FOUND) {
        t = t.Mid(open + 1, t.Length() - open - 2);
        e = dc.GetTextExtent(t);
      }
      (void)leftSide;
      if (e.x > maxLength) return;       // no room: better none than overlap
      dc.DrawRotatedText(t, cx, cy + e.x / 2, 90);
    }

    void paintPane(wxDC& dc, const wxRect& pane, VibKind kind,
                   const PaneScale& sc, const SpectrumTicks& xt, bool bottom)
    {
      const int ch = dc.GetCharHeight();
      const wxColour series = kind == VIB_IR ? p_palette.ir : p_palette.raman;
      const bool envelope = p_s->spec.fwhm() > 0;
      const int base = pane.GetBottom();
      const double lo = p_s->axis.lo(), hi = p_s->axis.hi();

      //  Grid and frame.
      dc.SetBrush(*wxTRANSPARENT_BRUSH);
      dc.SetPen(wxPen(p_palette.grid, 1));
      for (size_t i = 0; i < xt.values.size(); i++) {
        const int x = xOf(pane, xt.values[i]);
        dc.DrawLine(x, pane.y, x, base);
      }
      for (size_t i = 1; i < sc.left.y.ticks.values.size(); i++) {
        const int y = yOf(pane, sc.left.y.ticks.values[i], sc.left.y.top);
        dc.DrawLine(pane.x, y, pane.GetRight(), y);
      }

      //  Envelope: filled, then its outline on top of the sticks.
      std::vector<wxPoint> curve;
      const Axis& envAxis = sc.sticksOnLeft ? sc.right : sc.left;
      if (envelope) {
        for (int px = 0; px < pane.width; px++) {
          const double wn = p_s->axis.fromFraction(
              (px + 0.5) / (double)pane.width);
          const double v = p_s->spec.envelope(kind, wn);
          curve.push_back(wxPoint(pane.x + px,
                                  yOf(pane, v, envAxis.y.top)));
        }
        std::vector<wxPoint> poly = curve;
        poly.push_back(wxPoint(pane.GetRight(), base));
        poly.push_back(wxPoint(pane.x, base));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(SpectrumPalette::mix(series, p_palette.background,
                                                 0.78)));
        dc.SetClippingRegion(pane);
        dc.DrawPolygon((int)poly.size(), &poly[0]);
        dc.DestroyClippingRegion();
      }

      //  Band being dragged out.
      if (p_band) {
        const int a = xOf(pane, p_bandLo), b = xOf(pane, p_bandHi);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(SpectrumPalette::mix(p_palette.text,
                                                 p_palette.background, 0.85)));
        dc.DrawRectangle(std::min(a, b), pane.y, std::abs(a - b) + 1,
                         pane.height);
      }

      //  Sticks are hit-tested wherever they are drawn; they are painted
      //  after the envelope so the fill and its outline never cover them.
      const std::vector<VibStick> sticks = p_s->spec.sticks(kind);
      const bool drawSticks = sc.sticksOnLeft;
      dc.SetClippingRegion(pane.x - 3, pane.y - 2, pane.width + 6,
                           pane.height + 4);
      if (envelope && curve.size() > 1) {
        dc.SetPen(wxPen(series, 2));
        dc.DrawLines((int)curve.size(), &curve[0]);
      }
      if (drawSticks) {
        const double top = sc.left.y.top;
        int selX = -1, selY = 0;
        for (size_t i = 0; i < sticks.size(); i++) {
          const VibStick& s = sticks[i];
          if (s.wavenumber < lo || s.wavenumber > hi) continue;
          const int x = xOf(pane, s.wavenumber);
          const int y = yOf(pane, s.intensity, top);
          Hit h = { kind, s.mode, x, y, base };
          p_hits.push_back(h);
          if (s.mode == p_s->selected) { selX = x; selY = y; continue; }
          dc.SetPen(wxPen(s.imaginary ? p_palette.imaginary : series, 2));
          dc.DrawLine(x, base, x, y);
        }
        if (selX >= 0) paintSelection(dc, pane, selX, selY, p_s->selected);
      } else {
        //  Sticks hidden: peaks are still clickable, at their centres.
        for (size_t i = 0; i < sticks.size(); i++) {
          const VibStick& s = sticks[i];
          if (s.wavenumber < lo || s.wavenumber > hi || s.imaginary) continue;
          const int x = xOf(pane, s.wavenumber);
          const int y = yOf(pane, p_s->spec.envelope(kind, s.wavenumber),
                            envAxis.y.top);
          Hit h = { kind, s.mode, x, y, base };
          p_hits.push_back(h);
          if (s.mode == p_s->selected) paintSelection(dc, pane, x, y, s.mode);
        }
      }
      dc.DestroyClippingRegion();

      //  Frame, ticks and labels.
      dc.SetBrush(*wxTRANSPARENT_BRUSH);
      dc.SetPen(wxPen(p_palette.axis, 1));
      dc.DrawRectangle(pane);
      dc.SetTextForeground(p_palette.text);
      dc.SetFont(*wxSMALL_FONT);
      for (size_t i = 0; i < sc.left.y.ticks.values.size(); i++) {
        const double v = sc.left.y.ticks.values[i];
        const int y = yOf(pane, v, sc.left.y.top);
        dc.DrawLine(pane.x - 4, y, pane.x, y);
        const wxString t = wxString::Format("%.*f", sc.left.y.ticks.decimals, v);
        const wxSize e = dc.GetTextExtent(t);
        dc.DrawText(t, pane.x - 7 - e.x, y - e.y / 2);
      }
      if (sc.hasRight) {
        dc.SetTextForeground(series);
        for (size_t i = 0; i < sc.right.y.ticks.values.size(); i++) {
          const double v = sc.right.y.ticks.values[i];
          const int y = yOf(pane, v, sc.right.y.top);
          dc.SetPen(wxPen(series, 1));
          dc.DrawLine(pane.GetRight(), y, pane.GetRight() + 4, y);
          const wxString t = wxString::Format("%.*f", sc.right.y.ticks.decimals,
                                              v);
          dc.DrawText(t, pane.GetRight() + 7, y - ch / 2);
        }
      }
      for (size_t i = 0; i < xt.values.size(); i++) {
        const int x = xOf(pane, xt.values[i]);
        dc.SetPen(wxPen(p_palette.axis, 1));
        dc.DrawLine(x, base, x, base + 4);
        if (bottom) {
          const wxString t = wxString::Format("%.*f", xt.decimals,
                                              xt.values[i]);
          const wxSize e = dc.GetTextExtent(t);
          dc.SetTextForeground(p_palette.text);
          dc.DrawText(t, x - e.x / 2, base + 6);
        }
      }

      dc.SetFont(*wxNORMAL_FONT);
      dc.SetTextForeground(p_palette.text);
      drawVerticalText(dc, sc.left.title, 6, pane.y + pane.height / 2,
                       pane.height, true);
      if (sc.hasRight) {
        dc.SetTextForeground(series);
        drawVerticalText(dc, sc.right.title,
                         pane.GetRight() + 12 + labelWidthOf(dc, sc.right),
                         pane.y + pane.height / 2, pane.height, false);
      }
      dc.SetFont(*wxSMALL_FONT);
      dc.SetTextForeground(p_palette.text);
    }

    static int labelWidthOf(wxDC& dc, const Axis& a)
    {
      return labelWidth(dc, a);
    }

    //  The selected mode: the stick in the text colour, with its number
    //  above it, so it can be found against the table row.
    void paintSelection(wxDC& dc, const wxRect& pane, int x, int top,
                        int mode)
    {
      dc.SetPen(wxPen(p_palette.text, 4));
      dc.DrawLine(x, pane.GetBottom(), x, top);
      dc.SetFont(*wxSMALL_FONT);
      dc.SetTextForeground(p_palette.text);
      const wxString t = wxString::Format("%d", mode + 1);
      const wxSize e = dc.GetTextExtent(t);
      dc.DrawText(t, std::min(std::max(x - e.x / 2, pane.x),
                              pane.GetRight() - e.x),
                  std::max(pane.y, top - e.y - 2));
    }

    //  IR | Raman, at the top right of the plot: which pane the compact
    //  layout shows.
    void paintToggle(wxDC& dc, const wxRect& pane, bool haveIr, bool haveRaman)
    {
      (void)haveIr; (void)haveRaman;
      dc.SetFont(*wxSMALL_FONT);
      const wxString names[2] = { "IR", "Raman" };
      const int pad = 6, h = dc.GetCharHeight() + 4;
      int x = pane.GetRight() - 3;
      for (int k = 1; k >= 0; k--) {
        const int w = dc.GetTextExtent(names[k]).x + 2 * pad;
        x -= w;
        const wxRect r(x, pane.y + 3, w, h);
        p_toggle[k] = r;
        const wxColour c = k == VIB_IR ? p_palette.ir : p_palette.raman;
        const bool on = p_kindShown == (VibKind)k;
        dc.SetPen(wxPen(c, 1));
        dc.SetBrush(wxBrush(on ? c : p_palette.background));
        dc.DrawRectangle(r);
        dc.SetTextForeground(on ? p_palette.background : p_palette.text);
        dc.DrawText(names[k], r.x + pad, r.y + 2);
      }
      dc.SetTextForeground(p_palette.text);
    }

    int xOf(const wxRect& pane, double wn) const
    {
      return pane.x + (int)std::floor(p_s->axis.toFraction(wn) *
                                      (pane.width - 1) + 0.5);
    }

    static int yOf(const wxRect& pane, double v, double top)
    {
      const double f = top > 0 ? std::min(1.0, std::max(0.0, v / top)) : 0.0;
      return pane.GetBottom() - (int)std::floor(f * (pane.height - 2) + 0.5);
    }

    void paintReadout(wxDC& dc, const wxSize& size)
    {
      std::vector<wxString> lines;
      const std::vector<VibStick> s = p_s->spec.sticks(p_hoverKind);
      const VibStick *at = 0;
      for (size_t i = 0; i < s.size(); i++)
        if (s[i].mode == p_hoverMode) at = &s[i];
      if (at == 0) return;
      wxString head = wxString::Format("Mode %d", at->mode + 1);
      if (!at->irrep.empty()) head += "  " + utf8(at->irrep.c_str());
      lines.push_back(head);
      lines.push_back(wxString::Format("%.1f", at->wavenumber) +
                      utf8(" cm\xE2\x81\xBB\xC2\xB9") +
                      (at->imaginary ? wxString(" (imaginary)") : wxString()));
      for (int k = 0; k < 2; k++) {
        const std::vector<VibStick> ks = p_s->spec.sticks((VibKind)k);
        for (size_t i = 0; i < ks.size(); i++)
          if (ks[i].mode == p_hoverMode)
            lines.push_back(wxString(k == VIB_IR ? "IR  " : "Raman  ") +
                            wxString::Format("%.2f ", ks[i].intensity) +
                            utf8(p_s->spec.unitLabel((VibKind)k).c_str()));
      }
      dc.SetFont(*wxSMALL_FONT);
      int w = 0;
      const int ch = dc.GetCharHeight();
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
      dc.SetTextForeground(at->imaginary ? p_palette.imaginary : p_palette.text);
      for (size_t i = 0; i < lines.size(); i++)
        dc.DrawText(lines[i], x + 6, y + 4 + (int)i * (ch + 1));
      dc.SetTextForeground(p_palette.text);
    }

    // ---- mouse ----------------------------------------------------------

    void onPaint(wxPaintEvent&)
    {
      wxAutoBufferedPaintDC dc(this);
      paintOnto(dc, GetClientSize());
    }

    //  The pane under a point, or -1.
    int paneAt(const wxPoint& p) const
    {
      for (size_t i = 0; i < p_panes.size(); i++)
        if (p_panes[i].Contains(p)) return (int)i;
      return -1;
    }

    //  Nearest stick within a few pixels of the pointer, in the pane it
    //  is over.  Distance to the drawn line, so the top of a tall stick
    //  and a short one are equally easy to find.
    const Hit *hitAt(const wxPoint& p) const
    {
      const Hit *best = 0;
      double bestD = 8.0;
      const int pane = paneAt(wxPoint(std::max(p_plotLeft,
                          std::min(p.x, p_plotLeft + p_plotWidth - 1)), p.y));
      if (pane < 0) return 0;
      for (size_t i = 0; i < p_hits.size(); i++) {
        const Hit& h = p_hits[i];
        if (!p_panes[pane].Contains(wxPoint(h.x, h.base))) continue;
        const double dx = p.x - h.x;
        const double dy = p.y < h.top ? h.top - p.y
                                      : (p.y > h.base ? p.y - h.base : 0);
        const double d = std::sqrt(dx * dx + dy * dy);
        if (d < bestD) { bestD = d; best = &h; }
      }
      return best;
    }

    double wavenumberAt(int px) const
    {
      const double f = p_plotWidth > 1
          ? (px - p_plotLeft) / (double)(p_plotWidth - 1) : 0.5;
      return p_s->axis.fromFraction(std::min(1.0, std::max(0.0, f)));
    }

    void onLeftDown(wxMouseEvent& e)
    {
      SetFocus();
      for (int k = 0; k < 2; k++)
        if (p_toggle[k].Contains(e.GetPosition())) {
          p_kindShown = (VibKind)k;
          Refresh();
          return;
        }
      if (paneAt(e.GetPosition()) < 0) return;
      p_pressed = true;
      p_dragging = false;
      p_pressAt = e.GetPosition();
      if (!HasCapture()) CaptureMouse();
    }

    void onMotion(wxMouseEvent& e)
    {
      const wxPoint p = e.GetPosition();
      if (p_panning) {
        const double was = wavenumberAt(p_lastPan.x), now = wavenumberAt(p.x);
        p_s->axis.pan(was - now);
        p_lastPan = p;
        p_cacheValid = false;
        Refresh();
        return;
      }
      if (p_pressed && e.LeftIsDown()) {
        if (!p_dragging && std::abs(p.x - p_pressAt.x) > 4) p_dragging = true;
        if (p_dragging) {
          p_band = true;
          p_bandLo = wavenumberAt(p_pressAt.x);
          p_bandHi = wavenumberAt(p.x);
          Refresh();
        }
        return;
      }
      const Hit *h = hitAt(p);
      const int mode = h ? h->mode : -1;
      if (mode != p_hoverMode || h) {
        p_hoverMode = mode;
        if (h) p_hoverKind = h->kind;
        p_hoverAt = p;
        Refresh();
      }
      SetCursor(h ? wxCursor(wxCURSOR_HAND) : wxNullCursor);
    }

    void onLeftUp(wxMouseEvent& e)
    {
      if (!p_pressed) return;
      p_pressed = false;
      if (HasCapture()) ReleaseMouse();
      if (p_dragging) {
        p_dragging = false;
        p_band = false;
        if (std::abs(e.GetPosition().x - p_pressAt.x) >= 6) {
          p_s->axis.zoomTo(wavenumberAt(p_pressAt.x),
                        wavenumberAt(e.GetPosition().x));
          p_cacheValid = false;
        }
        Refresh();
        return;
      }
      clickAt(e.GetPosition());
    }

    void onDoubleClick(wxMouseEvent& e)
    {
      if (paneAt(e.GetPosition()) >= 0 && hitAt(e.GetPosition()) == 0)
        resetZoom();
    }

    void onPanDown(wxMouseEvent& e)
    {
      if (paneAt(e.GetPosition()) < 0) return;
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
      p_panning = false;
      p_pressed = false;
      p_dragging = false;
      p_band = false;
      Refresh();
    }

    void onWheel(wxMouseEvent& e)
    {
      const int delta = e.GetWheelDelta() > 0 ? e.GetWheelDelta() : 120;
      const double notches = e.GetWheelRotation() / (double)delta;
      if (e.ShiftDown()) {
        p_s->axis.pan(-notches * 0.1 * p_s->axis.span() *
                   (p_s->axis.reversed() ? -1 : 1));
      } else {
        //  About the pointer, so the band under it stays put.
        p_s->axis.zoomAbout(wavenumberAt(e.GetPosition().x),
                         std::pow(0.8, notches));
      }
      p_cacheValid = false;
      Refresh();
    }

    void onKey(wxKeyEvent& e)
    {
      const double step = 0.1 * p_s->axis.span() * (p_s->axis.reversed() ? 1 : -1);
      const double mid = 0.5 * (p_s->axis.lo() + p_s->axis.hi());
      switch (e.GetKeyCode()) {
        case WXK_HOME:  resetZoom(); return;
        case WXK_LEFT:  p_s->axis.pan(step); break;
        case WXK_RIGHT: p_s->axis.pan(-step); break;
        case '+': case '=': case WXK_NUMPAD_ADD:
          p_s->axis.zoomAbout(mid, 0.8); break;
        case '-': case WXK_NUMPAD_SUBTRACT:
          p_s->axis.zoomAbout(mid, 1.25); break;
        default: e.Skip(); return;
      }
      p_cacheValid = false;
      Refresh();
    }

    void onLeave(wxMouseEvent&)
    {
      if (p_hoverMode >= 0 && !p_pressed) {
        p_hoverMode = -1;
        Refresh();
      }
    }

    SpectrumClickTarget *p_owner;
    SpectrumPalette p_palette;
    std::shared_ptr<SpectrumShared> p_s;   // model, zoom, selection: shared
    VibKind         p_kindShown;           // the one pane in compact layout
    int             p_compactBelow;
    wxRect          p_toggle[2];
    VibKind         p_hoverKind;
    int             p_hoverMode;
    wxPoint         p_hoverAt;
    bool            p_pressed, p_dragging, p_panning, p_band;
    wxPoint         p_pressAt, p_lastPan;
    double          p_bandLo, p_bandHi;

    std::vector<Hit>    p_hits;
    std::vector<wxRect> p_panes;
    int p_plotLeft = 0, p_plotWidth = 1;

    bool   p_cacheValid;
    double p_cacheKey[7];
    double p_maxStick[2], p_maxEnv[2];
};

#endif

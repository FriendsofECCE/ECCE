/**
 * @file
 *
 * The surface an MO correlation diagram is drawn on.
 *
 * SEPARATED FROM THE PANEL SO IT CAN BE DRAWN WITHOUT ONE.
 *
 * This is the only part of the diagram that could not be checked
 * without a person looking at a screen: the model is tested against
 * the character tables and against the course's own worked answers,
 * but the layout was reachable only by building a package, installing
 * it, starting the services and opening a calculation.  So a second
 * implementation of the layout grew up alongside it in
 * tools/modiagram, and two implementations of the same picture drift.
 *
 * As a class of its own it can be given a model and asked to paint
 * itself onto a bitmap, which tests/symmetry does.  Nothing about the
 * class changed in the move; it was defined entirely inline inside
 * MoDiagramPanel.C and is defined entirely inline here.
 */

#ifndef MODIAGRAMCANVAS_H_
#define MODIAGRAMCANVAS_H_

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

#include <wx/dcbuffer.h>
#include <wx/panel.h>
#include <wx/sizer.h>

#include "tdat/MoDiagram.H"

using std::max;
using std::min;
using std::ostringstream;
using std::vector;

/**
 * Something that can be told a level was clicked.
 *
 * An interface rather than the panel itself, so the canvas does not
 * depend on it.  The dependency ran the wrong way: the canvas is the
 * general thing and the panel is one user of it, and with the panel
 * named directly the canvas could not be compiled -- or drawn -- on
 * its own.
 */
class MoDiagramClickTarget
{
  public:
    virtual ~MoDiagramClickTarget() {}
    virtual void orbitalClicked(int orbengIndex) = 0;

    /** Composition text for these ORBENG indices (one level), or "". */
    virtual std::string orbitalComposition(const std::vector<int>&)
    { return std::string(); }
};

/**
 * The drawing surface.
 *
 * Nothing here decides anything -- MoDiagram has already worked out
 * which levels exist, what is folded away and what connects to what.
 * This turns that into pixels.
 */
class MoDiagramCanvas : public wxPanel
{
  public:

    MoDiagramCanvas(wxWindow *parent)
      //  Drawn to the whole size: on MSW a resize would otherwise repaint
      //  only the new strip (see SpectrumCanvas).
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxFULL_REPAINT_ON_RESIZE), p_owner(0), p_haveFragments(false),
        p_showEnergies(true),
        p_showDescription(true),
        p_placed(), p_notes(), p_sketchColumn(0), p_outward(-1)
    {
      SetBackgroundStyle(wxBG_STYLE_PAINT);   // needed for buffered paint
      SetBackgroundColour(*wxWHITE);
      Bind(wxEVT_PAINT, &MoDiagramCanvas::onPaint, this);
      Bind(wxEVT_LEFT_DOWN, &MoDiagramCanvas::onClick, this);
      Bind(wxEVT_MOTION, &MoDiagramCanvas::onMotion, this);
    }

    /** Called with an ORBENG index when a level is clicked. */
    void setClickHandler(MoDiagramClickTarget *owner) { p_owner = owner; }

    void setGroup(const string& group) { p_group = group; }

    /** The molecule's formula, from MoDiagram::formula(). */
    void setFormula(const string& formula) { p_formula = formula; }

    /**
     * Whether each level's energy is written beside its label.
     *
     * Twelve levels with a label and a number each is a lot of text on
     * one picture, and the numbers are the part a reader of a
     * QUALITATIVE diagram needs least -- the point of it is which
     * orbital is above which, not by how much.  Off, the labels alone
     * are much easier to read; on, the exact value is there.
     */
    void setShowEnergies(bool show) { p_showEnergies = show; }
    /** The clicked level's description under the diagram (the tooltip stays). */
    void setShowDescription(bool show) { p_showDescription = show; }
    /** Which of the two model-level labels are drawn: construction (left), symmetry (right). */
    void setLabelSides(bool construction, bool symmetry) { p_showConstruct = construction; p_showSymmetry = symmetry; }
    /** 0 both parts of the construction label, 1 without the parents, 2 none (narrow panels). */
    int constructMode() const { return p_cMode; }
    bool showEnergies() const { return p_showEnergies; }

    void setDiagram(const MoColumn& left, const MoColumn& centre,
                    const MoColumn& right,
                    const vector<MoConnection>& links,
                    bool haveFragments, const string& note,
                    const string& headline = "")
    {
      p_left = left;
      p_centre = centre;
      p_right = right;
      p_links = links;
      p_haveFragments = haveFragments;
      p_note = note;
      p_summary.clear();
      p_verdict.clear();
      p_warnings.clear();
      p_about.clear();
      p_headline = headline;
      p_info.clear();
      p_tips.clear();
      p_tipLevel = -1;
      Refresh();
    }

    /**
     * The text under the diagram in tiers (#301 follow-up).  `summary`
     * is the always-shown second line (with <sub>..</sub> allowed),
     * `warnings` are drawn in orange only when something is wrong, and
     * `about` is the longer text behind the panel's "About this
     * diagram" button.  Without a summary, the plain note is drawn.
     */
    void setNotes(const string& summary, const vector<string>& warnings,
                  const string& about)
    {
      p_summary = summary;
      p_warnings = warnings;
      p_about = about;
      Refresh();
    }
    /**
     * The About text with a "This diagram" section in front: what used
     * to be written under the diagram (verdict, source, folded levels,
     * electron count, warnings), one bullet each.
     */
    string aboutText() const
    {
      const string dot = "\xE2\x80\xA2 ";
      const string sep = " \xC2\xB7 ";
      string out = "**This diagram**\n" + dot +
          "Experimental: check the orbitals themselves before relying on "
          "this diagram.\n";
      if (!p_verdict.empty()) out += dot + p_verdict + "\n";
      size_t at = 0;
      while (!p_summary.empty()) {
        const size_t k = p_summary.find(sep, at);
        out += dot + p_summary.substr(at, k == string::npos ? k : k - at) + "\n";
        if (k == string::npos) break;
        at = k + sep.size();
      }
      for (size_t i = 0; i < p_warnings.size(); i++) out += dot + p_warnings[i] + "\n";
      return p_about.empty() ? out : out + "\n" + p_about;
    }

    /** The calculation column's heading: the method, e.g. "BP86/def2-SVP". */
    void setCalcHeader(const string& text)
    {
      p_calcHeader = text.empty() ? string("This calculation") : text;
      Refresh();
    }

    /** The model-against-calculation line of the model-first diagram. */
    void setVerdict(const string& text, bool ok)
    {
      p_verdict = text;
      p_verdictOk = ok;
      Refresh();
    }

  private:

    //  Where each centre level was last drawn, so a click can be
    //  turned back into an orbital.  Recorded during the paint rather
    //  than recomputed on the click: the layout depends on the window
    //  size, the folded bands and the heading, and a second copy of
    //  that arithmetic would drift from the one that draws.
    struct Hit { wxRect box; int orbital; int level; };

    /**
     * The colour of a level: one per bonding/antibonding pair.
     *
     * Seeing which antibonding orbital belongs to which bonding one is
     * most of what the diagram is for, and it cannot be read off the
     * order.  Non-bonding levels, and anything unclassified, stay
     * black.  The palette avoids the red/green pair that colour-blind
     * readers cannot separate.
     */
    static wxColour levelColour(const MoLevel& level)
    {
      static const unsigned char rgb[8][3] = {
        { 31,119,180}, {255,127, 14}, {148,103,189}, {140, 86, 75},
        { 23,190,207}, {227,119,194}, {127,127,127}, {188,189, 34}};
      if (level.pairing < 0) return wxColour(0, 0, 0);
      const int k = level.pairing % 8;
      return wxColour(rgb[k][0], rgb[k][1], rgb[k][2]);
    }

    /**
     * A term symbol broken into upright, subscript and superscript.
     *
     * A diagram that writes "1b2" and "2t2*" flat, where a chemist
     * writes 1b<sub>2</sub> and 2t<sub>2</sub><sup>*</sup>, looks like
     * it was drawn by something that does not know what it is drawing.
     * The same applies to a linear molecule, whose labels arrive from
     * the code as "sig", "siu", "piu" and mean sigma-g, sigma-u,
     * pi-u.
     *
     * The shape is always: an optional orbital number, a symmetry
     * symbol, its subscript (the order, and the g/u parity, and any
     * primes), an optional asterisk, and then whatever else was
     * appended -- " nb", or a shell in brackets.
     */
    struct Piece { wxString text; int kind; };   // 0 upright 1 sub 2 super

    static bool startsWith(const string& s, size_t at, const char* what)
    {
      const size_t n = strlen(what);
      if (at + n > s.size()) return false;
      for (size_t i = 0; i < n; i++) {
        if (tolower((unsigned char)s[at + i]) != what[i]) return false;
      }
      return true;
    }

    static void splitLabel(const string& label, vector<Piece>& out)
    {
      out.clear();
      size_t i = 0;

      //  The orbital number, if there is one.
      string lead;
      while (i < label.size() && isdigit((unsigned char)label[i])) {
        lead += label[i++];
      }
      if (!lead.empty()) {
        Piece p; p.text = wxString(lead.c_str(), wxConvUTF8); p.kind = 0;
        out.push_back(p);
      }

      //  The symmetry symbol.  A spelled-out Greek letter first, since
      //  its first character would otherwise be taken for a Mulliken
      //  symbol and the rest of its name for a subscript.
      static const char* const names[]  =
          {"sigma", "delta", "dlta", "phi", "pi", "si", "sg", "de", 0};
      //  "dlta" and "sg" are Gaussian's own spellings (SGG, PIU, DLTG...).
      static const char* const glyphs[] =
          {"\xcf\x83", "\xce\xb4", "\xce\xb4", "\xcf\x86",
           "\xcf\x80", "\xcf\x83", "\xcf\x83", "\xce\xb4"};
      bool greek = false;
      for (int k = 0; names[k] != 0 && !greek; k++) {
        if (!startsWith(label, i, names[k])) continue;
        Piece p; p.text = wxString::FromUTF8(glyphs[k]); p.kind = 0;
        out.push_back(p);
        i += strlen(names[k]);
        greek = true;
      }
      if (!greek && i < label.size() && isalpha((unsigned char)label[i])) {
        Piece p;
        p.text = wxString(label.substr(i, 1).c_str(), wxConvUTF8);
        p.kind = 0;
        out.push_back(p);
        i++;
      }

      //  Its subscript: the order and the parity.  NOT the primes.
      string sub;
      while (i < label.size()) {
        const char c = label[i];
        if (isdigit((unsigned char)c) ||
            tolower((unsigned char)c) == 'g' ||
            tolower((unsigned char)c) == 'u') {
          sub += c; i++;
        } else if (c == '-' && sub.empty()) {
          i++;                       // "sigma-g" as some codes write it
        } else {
          break;
        }
      }
      if (!sub.empty()) {
        Piece p; p.text = wxString(sub.c_str(), wxConvUTF8); p.kind = 1;
        out.push_back(p);
      }

      //  THE PRIMES ARE SUPERSCRIPT, and they decide the whole label.
      //
      //  A' and A'' are the only two irreps Cs has, so the prime is
      //  the entire distinction between them.  Set as a subscript in a
      //  small font, both collapsed into something that reads as a
      //  dot: every level in a Cs diagram came out as "A.", which made
      //  the diagram unreadable rather than merely ugly. Drawn with
      //  the proper prime and double-prime glyphs, at the height the
      //  notation actually uses.
      string primes;
      while (i < label.size() && (label[i] == '\'' || label[i] == '"')) {
        primes += (label[i] == '"') ? "\xe2\x80\xb3"   // double prime
                                    : "\xe2\x80\xb2";  // prime
        i++;
      }
      //  Two apostrophes are how most codes write a double prime.
      if (primes == "\xe2\x80\xb2\xe2\x80\xb2") primes = "\xe2\x80\xb3";
      if (!primes.empty()) {
        Piece p; p.text = wxString::FromUTF8(primes.c_str()); p.kind = 2;
        out.push_back(p);
      }

      if (i < label.size() && label[i] == '*') {
        Piece p; p.text = wxT("*"); p.kind = 2;
        out.push_back(p);
        i++;
      }

      if (i < label.size()) {
        Piece p;
        p.text = wxString(label.substr(i).c_str(), wxConvUTF8);
        p.kind = 0;
        out.push_back(p);
      }
    }

    /** A formula: digit runs subscript, anything after '^' superscript. */
    static void splitFormula(const string& text, vector<Piece>& out)
    {
      out.clear();
      size_t i = 0;
      while (i < text.size() && text[i] != '^') {
        const bool digits = isdigit((unsigned char)text[i]) != 0;
        string run;
        while (i < text.size() && text[i] != '^' &&
               (isdigit((unsigned char)text[i]) != 0) == digits) {
          run += text[i++];
        }
        Piece p;
        p.text = wxString(run.c_str(), wxConvUTF8);
        p.kind = digits ? 1 : 0;
        out.push_back(p);
      }
      if (i < text.size() && text[i] == '^') {
        //  A real minus sign, not a hyphen: it is a charge.
        string rest;
        for (size_t k = i + 1; k < text.size(); k++) {
          if (text[k] == '-') rest += "\xe2\x88\x92";
          else                rest += text[k];
        }
        Piece p;
        p.text = wxString::FromUTF8(rest.c_str());
        p.kind = 2;
        out.push_back(p);
      }
    }

    /** The width and height a split label will occupy. */
    static wxSize measurePieces(wxDC& dc, const vector<Piece>& pieces,
                                const wxFont& base, const wxFont& small)
    {
      wxSize total(0, 0);
      for (size_t i = 0; i < pieces.size(); i++) {
        dc.SetFont(pieces[i].kind == 0 ? base : small);
        const wxSize one = dc.GetTextExtent(pieces[i].text);
        total.x += one.x;
        if (one.y > total.y) total.y = one.y;
      }
      dc.SetFont(base);
      if (total.y == 0) total.y = dc.GetTextExtent(wxT("Xg")).y;
      return total;
    }

    static void drawPieces(wxDC& dc, const vector<Piece>& pieces,
                           int x, int y, const wxFont& base,
                           const wxFont& small, int lineHeight)
    {
      for (size_t i = 0; i < pieces.size(); i++) {
        if (pieces[i].kind == 0) {
          dc.SetFont(base);
          dc.DrawText(pieces[i].text, x, y);
        } else {
          dc.SetFont(small);
          const int drop = (pieces[i].kind == 1) ? lineHeight/3
                                                 : -lineHeight/4;
          dc.DrawText(pieces[i].text, x, y + drop);
        }
        x += dc.GetTextExtent(pieces[i].text).x;
      }
      dc.SetFont(base);
    }

    /**
     * Energy to pixels.  With a broken axis the levels above the break
     * (MoLevel::clipped) share a short strip at the top, in order but not
     * to scale, and the valence region below it gets all the rest.
     */
    int yFor(double energy, const wxRect& plot) const
    {
      if (p_hi <= p_lo) return plot.y + plot.height/2;
      if (p_stripH > 0) {
        if (energy >= p_stripLo - 1.0e-9) {
          const double f = p_stripHi > p_stripLo
                         ? (p_stripHi - energy)/(p_stripHi - p_stripLo) : 0.0;
          return plot.y + 4 + (int)(f*p_stripH);
        }
        energy = min(energy, p_hi);
        const int top = plot.y + p_stripH + 4 + p_breakGap;
        const double f = fracOf(energy);
        return plot.y + plot.height - (int)(f*(plot.y + plot.height - top));
      }
      const double f = fracOf(energy);
      return plot.y + plot.height - (int)(f*plot.height);
    }

    /**
     * Where an energy falls between p_lo and p_hi, 0..1.  Linear for the old
     * drawings; in the model diagram a monotone map through the energies drawn
     * (p_kE/p_kF), where a gap much larger than the typical spacing counts only
     * logarithmically, so the occupied valence levels keep a usable share of the
     * height.  The model is not to scale, so only the order and rough spacing
     * mean anything.
     */
    double fracOf(double energy) const
    {
      if (p_kE.size() < 2) return (energy - p_lo)/(p_hi - p_lo);
      if (energy <= p_kE.front()) return 0.0;
      if (energy >= p_kE.back()) return 1.0;
      size_t k = std::upper_bound(p_kE.begin(), p_kE.end(), energy) - p_kE.begin();
      const double t = (energy - p_kE[k-1])/(p_kE[k] - p_kE[k-1]);
      return p_kF[k-1] + t*(p_kF[k] - p_kF[k-1]);
    }

    void buildMap(const MoColumn* const cols[3])
    {
      p_kE.clear(); p_kF.clear();
      vector<double> e;
      e.push_back(p_lo); e.push_back(p_hi);
      for (int c = 0; c < 3; c++)
        for (size_t i = 0; i < cols[c]->levels.size(); i++) {
          const double v = cols[c]->levels[i].energy;
          if (v > p_lo && v < p_hi && !(p_stripRows > 0 && v >= p_stripLo - 1.0e-9)) e.push_back(v);
        }
      std::sort(e.begin(), e.end());
      vector<double> k;
      for (size_t i = 0; i < e.size(); i++)
        if (k.empty() || e[i] - k.back() > 1.0e-6) k.push_back(e[i]);
      if (k.size() < 3) return;
      vector<double> gaps;
      for (size_t i = 1; i < k.size(); i++) gaps.push_back(k[i] - k[i-1]);
      vector<double> sorted = gaps;
      std::sort(sorted.begin(), sorted.end());
      const double cap = 2.0*sorted[sorted.size()/2];
      double total = 0.0;
      vector<double> cum(1, 0.0);
      for (size_t i = 0; i < gaps.size(); i++) {
        total += gaps[i] <= cap ? gaps[i] : cap*(1.0 + 0.5*std::log(gaps[i]/cap));
        cum.push_back(total);
      }
      p_kE = k;
      for (size_t i = 0; i < cum.size(); i++) p_kF.push_back(cum[i]/total);
    }

    //  A calculated level in the calculation column: where, how wide, which lane.
    struct CalcItem
    {
      int level;                 // the centre level it is matched to (click, hover, colour)
      int only;                  // >= 0: index into calcOnly (no model partner)
      double e;                  // eV
      int deg;
      bool strip, loose;
      int y, top, bottom, w, lane, x;
      vector<Piece> pieces;
      wxString energy;
    };

    //  The calculation column's own scale: p_clo..p_chi to scale, and the
    //  levels in p_cstripLo..p_cstripHi above a break, not to scale.
    int calcYFor(double e, const wxRect& plot) const
    {
      if (p_cstripH > 0) {
        if (e >= p_cstripLo - 1.0e-9) {
          const double f = p_cstripHi > p_cstripLo ? (p_cstripHi - e)/(p_cstripHi - p_cstripLo) : 0.0;
          return plot.y + 4 + (int)(f*p_cstripH);
        }
        e = min(e, p_chi);
        const int top = plot.y + p_cstripH + 4 + p_breakGap;
        return plot.y + plot.height - (int)((e - p_clo)/(p_chi - p_clo)*(plot.y + plot.height - top));
      }
      return plot.y + plot.height - (int)((e - p_clo)/(p_chi - p_clo)*plot.height);
    }

    static const int calcSlot = 14;
    static constexpr double HARTREE_EV = 27.211386;
    //  Half the height an electron-arrow glyph (18 px, 2 px pen) needs between rows.
    static constexpr int ARROW_HALF = 12;

    //  Hover keys: a model level is its index; a calculation-column level
    //  carries one of these bits (matched: centre index; no partner: calcOnly index).
    static const int CALC_BIT = 1 << 20;
    static const int ONLY_BIT = 1 << 21;

    const MoLevel& itemLevel(const CalcItem& it) const
    {
      return it.only >= 0 ? p_centre.calcOnly[it.only] : p_centre.levels[it.level];
    }

    /**
     * Lays out the calculation column: one item per distinct calculated
     * level, at its own energy.  Items whose boxes would overlap vertically
     * go side by side in lanes.  Returns the width the column needs
     * (lanes, gaps and the axis on the right); `axisX` is the axis line.
     */
    int layoutCalc(wxDC& dc, const wxRect& plot, int canvasWidth,
                   vector<CalcItem>& items, int& axisX)
    {
      items.clear();
      for (size_t i = 0; i < p_centre.levels.size(); i++) {
        const MoLevel& l = p_centre.levels[i];
        if (!l.hasCalc) continue;
        vector<int> key = l.orbitals;
        bool dup = false;
        for (size_t k = 0; k < items.size(); k++) {
          const MoLevel& o = p_centre.levels[items[k].level];
          vector<int> ok = o.orbitals;
          if (ok != key) continue;
          dup = true;
          if (l.calcOverlap > o.calcOverlap) items[k].level = (int)i;
        }
        if (dup) continue;
        CalcItem it;
        it.level = (int)i; it.only = -1; it.e = l.calcEnergy*HARTREE_EV; it.strip = l.clipped;
        it.deg = 1; it.loose = false; it.y = it.top = it.bottom = it.w = it.lane = it.x = 0;
        items.push_back(it);
      }
      for (size_t i = 0; i < p_centre.calcOnly.size(); i++) {
        CalcItem it;
        it.level = -1; it.only = (int)i; it.e = p_centre.calcOnly[i].calcEnergy*HARTREE_EV; it.strip = false;
        it.deg = 1; it.loose = false; it.y = it.top = it.bottom = it.w = it.lane = it.x = 0;
        items.push_back(it);
      }

      //  The broken axis: only where the model has one, and only levels
      //  that really lie above everything on the scale.
      bool again = p_stripH > 0;
      while (again) {
        again = false;
        double maxV = -1e30;
        for (size_t k = 0; k < items.size(); k++) if (!items[k].strip) maxV = max(maxV, items[k].e);
        for (size_t k = 0; k < items.size(); k++)
          if (items[k].strip && items[k].e <= maxV + 1.0e-9) { items[k].strip = false; again = true; }
      }
      double lo = 1e30, hi = -1e30;
      p_cstripLo = 1e30; p_cstripHi = -1e30;
      p_cstripH = 0;
      for (size_t k = 0; k < items.size(); k++) {
        if (p_stripH > 0 && items[k].strip) {
          p_cstripLo = min(p_cstripLo, items[k].e); p_cstripHi = max(p_cstripHi, items[k].e);
        } else {
          items[k].strip = false;
          lo = min(lo, items[k].e); hi = max(hi, items[k].e);
        }
      }
      if (p_cstripHi >= p_cstripLo) p_cstripH = p_stripH;
      if (lo > hi) { lo = 0.0; hi = 0.1; }
      if (hi - lo < 1.0e-6) hi = lo + 0.1;
      const double pad = 0.06*(hi - lo);
      p_clo = lo - pad; p_chi = hi + pad;

      wxFont label = *wxNORMAL_FONT;
      wxFont sub = label;
      sub.SetPointSize(max(5, label.GetPointSize() - 3));
      dc.SetFont(label);
      const int labelH = dc.GetTextExtent(wxT("Xg")).y;
      dc.SetFont(*wxSMALL_FONT);
      const int smallH = dc.GetTextExtent(wxT("Xg")).y;

      for (size_t k = 0; k < items.size(); k++) {
        CalcItem& it = items[k];
        const MoLevel& l = itemLevel(it);
        it.deg = min(max(l.calcDegeneracy, 1), 5);
        it.loose = it.only < 0 && l.calcOverlap < 0.80;
        splitLabel(l.calcLabel, it.pieces);
        const double e = fabs(it.e) < 0.005 ? 0.0 : it.e;
        it.energy = wxString::Format(wxT("%s%.2f"), e < 0 ? wxT("−") : wxT("+"), fabs(e));
        if (!p_showEnergies) it.energy.clear();
        if (it.only < 0) {
          if (!it.energy.empty()) it.energy += wxT("  ");
          it.energy += wxString::Format(wxT("overlap %.2f"), l.calcOverlap);
        }
        it.y = calcYFor(it.e, plot);
        const int lw = measurePieces(dc, it.pieces, label, sub).x;
        dc.SetFont(*wxSMALL_FONT);
        const int ew = dc.GetTextExtent(it.energy).x;
        it.w = it.deg*calcSlot + 6 + lw + (ew ? 6 + ew : 0) + 8;
        it.top = it.y - max(9, labelH/2);
        it.bottom = it.y + max(9, labelH/2);
      }
      (void)smallH;

      //  One row per level, in energy order, never beside another.
      {
        vector<int> natural, y;
        int widest = 0;
        for (size_t k = 0; k < items.size(); k++) {
          natural.push_back(items[k].y);
          widest = max(widest, items[k].w);
        }
        y = natural;
        vector<int> halves;
        for (size_t k = 0; k < items.size(); k++) {
          const MoLevel& l = itemLevel(items[k]);
          halves.push_back(l.calcOccupancy > 0.0 || l.calcOccupancyBeta > 0.0
                           ? (int)ARROW_HALF : max(9, labelH/2));
        }
        openRows(y, [&](size_t a, size_t b) { return halves[a] + halves[b]; },
                 plot.y + ARROW_HALF, plot.y + plot.height - ARROW_HALF);
        p_calcSpans.clear();
        stretchedSpans(natural, y, p_calcSpans);
        for (size_t k = 0; k < items.size(); k++) {
          items[k].y = y[k];
          items[k].top = y[k] - halves[k];
          items[k].bottom = y[k] + halves[k];
          items[k].lane = 0;
        }
        const int total = widest;
      dc.SetFont(*wxNORMAL_FONT);
      const int header = dc.GetTextExtent(wxString::FromUTF8(p_calcHeader.c_str())).x;
      const int axisW = 44;
      int width = max(total + 20, header + 20) + axisW;
      width = max(width, 150);
      (void)canvasWidth;
      const int x0 = canvasWidth - width + 8 + 6;
      for (size_t k = 0; k < items.size(); k++) items[k].x = x0;
      axisX = canvasWidth - axisW + 8;
      p_calcRows = items;
      return width;
      }
    }

    void drawCalc(wxDC& dc, const wxRect& plot, const vector<CalcItem>& items, int axisX)
    {
      wxFont label = *wxNORMAL_FONT;
      wxFont sub = label;
      sub.SetPointSize(max(5, label.GetPointSize() - 3));

      //  The axis, in eV, with the same kind of break as the model's.
      dc.SetPen(wxPen(wxColour(120, 120, 120), 1));
      if (p_cstripH > 0) {
        const int valenceTop = plot.y + p_cstripH + 4 + p_breakGap;
        const int mid = valenceTop - p_breakGap/2;
        dc.DrawLine(axisX, plot.y + plot.height, axisX, mid + 5);
        dc.DrawLine(axisX, mid - 5, axisX, plot.y);
        dc.DrawLine(axisX - 6, mid + 6, axisX + 6, mid + 1);
        dc.DrawLine(axisX - 6, mid - 1, axisX + 6, mid - 6);
      } else {
        dc.DrawLine(axisX, plot.y, axisX, plot.y + plot.height);
      }
      dc.DrawLine(axisX, plot.y, axisX - 4, plot.y + 7);
      dc.DrawLine(axisX, plot.y, axisX + 4, plot.y + 7);
      dc.SetFont(*wxSMALL_FONT);
      dc.SetTextForeground(wxColour(110, 110, 110));
      static const double steps[] = { 0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50 };
      double step = steps[8];
      for (int i = 0; i < 9; i++)
        if ((p_chi - p_clo)/steps[i] <= 10) { step = steps[i]; break; }
      for (double v = ceil(p_clo/step)*step; v <= p_chi; v += step) {
        const int y = calcYFor(v, plot);
        if (inSpans(p_calcSpans, y)) continue;
        dc.DrawLine(axisX - 3, y, axisX + 3, y);
        const double shown = fabs(v) < 1.0e-9 ? 0.0 : floor(v/step + 0.5)*step;
        const wxString text = wxString::Format(wxT("%g"), shown);
        dc.DrawText(text, axisX + 6, y - dc.GetTextExtent(text).y/2);
      }
      drawStretchMarks(dc, axisX, p_calcSpans);
      dc.DrawText(wxT("eV"), axisX + 7, plot.y - dc.GetTextExtent(wxT("eV")).y - 2);

      //  The text of a crowded column is spread on its own, a line apart,
      //  with a leader to its level; the levels stay where they are.
      dc.SetFont(label);
      const int textH = dc.GetTextExtent(wxT("Xg")).y;
      vector<int> textY;
      for (size_t k = 0; k < items.size(); k++) textY.push_back(items[k].y);
      openRows(textY, [&](size_t, size_t) { return textH; },
               plot.y + textH/2, plot.y + plot.height - textH/2);

      //  When the column is too short for a line each, the labels that
      //  would print over another are left off (occupied levels keep
      //  theirs first); the level itself is still drawn, hovered and clicked.
      vector<bool> labelled(items.size(), true);
      {
        vector<int> keep;
        for (int pass = 0; pass < 2; pass++)
          for (size_t k = 0; k < items.size(); k++) {
            const MoLevel& l = itemLevel(items[k]);
            const bool occupied = l.calcOccupancy > 0.0 || l.calcOccupancyBeta > 0.0;
            if (occupied == (pass == 0)) {
              bool clear = true;
              for (size_t m = 0; m < keep.size() && clear; m++)
                clear = abs(textY[keep[m]] - textY[k]) >= textH;
              labelled[k] = clear;
              if (clear) keep.push_back((int)k);
            }
          }
      }

      for (size_t k = 0; k < items.size(); k++) {
        const CalcItem& it = items[k];
        const MoLevel& l = itemLevel(it);
        const wxColour colour = it.only >= 0 ? wxColour(130, 130, 130) : levelColour(l);
        dc.SetPen(wxPen(colour, 2));
        for (int d = 0; d < it.deg; d++)
          dc.DrawLine(it.x + d*calcSlot, it.y, it.x + d*calcSlot + calcSlot - 4, it.y);
        MoLevel occ;
        occ.degeneracy = it.deg;
        occ.occupancy = l.calcOccupancy;
        occ.occupancyBeta = l.calcOccupancyBeta;
        drawElectrons(dc, occ, it.x, calcSlot, calcSlot - 4, it.y, 0, plot);

        //  One hit box per bar, each with its own orbital.
        vector<int> orbs = l.orbitals;
        for (int d = 0; d < it.deg && !orbs.empty(); d++) {
          Hit hit;
          hit.box = wxRect(it.x + d*calcSlot - 2, it.y - 8, calcSlot, 17);
          hit.orbital = orbs[(size_t)d < orbs.size() ? (size_t)d : orbs.size() - 1];
          hit.level = it.only >= 0 ? (ONLY_BIT | it.only) : (CALC_BIT | it.level);
          p_hits.push_back(hit);
        }

        const int tx = it.x + it.deg*calcSlot + 6;
        dc.SetTextForeground(colour);
        dc.SetFont(label);
        const int labelH = dc.GetTextExtent(wxT("Xg")).y;
        const int ty = textY[k];
        if (!labelled[k]) continue;
        if (abs(ty - it.y) > 2) {
          dc.SetPen(wxPen(wxColour(170, 170, 170), 1));
          dc.DrawLine(it.x + it.deg*calcSlot - 3, it.y, tx - 1, ty);
        }
        drawPieces(dc, it.pieces, tx, ty - labelH/2, label, sub, labelH);
        const int lw = measurePieces(dc, it.pieces, label, sub).x;
        dc.SetFont(*wxSMALL_FONT);
        dc.SetTextForeground(wxColour(110, 110, 110));
        dc.DrawText(it.energy, tx + lw + 6, ty - dc.GetTextExtent(it.energy).y/2);
      }
    }

    /**
     * One column's levels.
     *
     * A DEGENERATE LEVEL IS DRAWN AS SEPARATE LINES, one per orbital,
     * side by side.  It is three orbitals, and one long line labelled
     * "(3)" both looks like one orbital and puts the count somewhere
     * the eye does not connect to it.
     *
     * Levels that share an energy are laid out side by side too, for
     * the same reason and a sharper one: oxygen's 2p spans a1, b1 and
     * b2 at one energy, and drawn on top of each other their three
     * labels overprint into an unreadable smudge.
     */
    /**
     * Where each level of a column is drawn, and which share a row.
     *
     * Separated out because the CORRELATION LINES need the same
     * answer.  They are drawn before the columns and used to compute
     * their endpoints from the energy directly; once fragment columns
     * could be opened out for legibility, the lines went on ending
     * where the levels used to be and stopped short of them -- a
     * diagram whose lines point at nothing.
     *
     * ROOM FOR THE LABELS.  Nitrite's oxygen set has six symmetry
     * orbitals inside a tenth of a Hartree, so drawn strictly to
     * scale their rows overlap and their labels and phase sketches
     * print on top of one another.  A fragment level is not AT its
     * drawn energy anyway: it is placed at the mean of the orbitals
     * it became.  The energy is printed beside every level, so a row
     * moved off its height still says its true value.
     */
    void columnGeometry(wxDC& dc, const MoColumn& column,
                        const wxRect& plot, bool spread,
                        vector< std::pair<size_t,size_t> >& groups,
                        vector<int>& groupY)
    {
      groups.clear();
      groupY.clear();

      //  A ROW IS ONE LEVEL.  SIDE BY SIDE MEANS DEGENERATE.
      //
      //  Levels that merely land on the same pixel were being merged
      //  into one row and drawn beside each other -- which is how a
      //  degenerate set is drawn, and so says they are one.  In D2h
      //  nothing can be: it has eight irreps and every one of them is
      //  one-dimensional.  Ethene's three highest orbitals are three
      //  thousandths of a Hartree apart, and drawn as a row of three
      //  they claimed a degeneracy the group forbids.
      //
      //  So a row is a level, and a level draws as many lines as its
      //  irrep has dimensions.  Labels that collide are moved instead
      //  -- the picture must not assert what the arithmetic does not.
      for (size_t a = 0; a < column.levels.size(); ) {
        size_t b = a;
        while (b + 1 < column.levels.size() &&
               fabs(column.levels[b+1].energy - column.levels[a].energy)
                   < 1.0e-9) {
          b++;
        }
        groups.push_back(std::make_pair(a, b));
        a = b + 1;
      }

      for (size_t g = 0; g < groups.size(); g++) {
        groupY.push_back(yFor(column.levels[groups[g].first].energy, plot));
      }
      if (!spread || groupY.size() < 2) return;

      //  A phase sketch is taller than a line of text, so a column
      //  that has them needs more room than one that does not --
      //  chlorine's 3p symmetry orbitals were overlapping each other.
      //  WHAT EACH LEVEL ACTUALLY NEEDS, NOT WHAT THE WORST ONE
      //  NEEDS.
      //
      //  An annotation is written under its level's label and is
      //  drawn once per shell, so in water exactly one of the
      //  oxygen's three 2p levels carries one.  Demanding two rows
      //  between every pair on that account opened the set out by
      //  three times what it needed and pushed B1 2p away from the
      //  non-bonding orbital it is identical to.  A gap has to clear
      //  the level ABOVE it, and nothing more.
      dc.SetFont(*wxSMALL_FONT);
      const int row = dc.GetTextExtent(wxT("Xg")).y;

      vector<int> needs(groups.size(), row + 8), tall(groups.size(), 0);
      for (size_t g = 0; g < groups.size(); g++) {
        bool sketch = false, note = false;
        for (size_t i = groups[g].first; i <= groups[g].second; i++) {
          if (!column.levels[i].phases.empty())     sketch = true;
          //  Only when it will actually be drawn, so turning the
          //  energies off reclaims the row they needed rather than
          //  leaving the levels spread as if they were still there.
          if (p_showEnergies && !column.levels[i].annotation.empty()) {
            note = true;
          }
        }
        if (note)   needs[g] = 2*row + 10;
        if (sketch && needs[g] < 46) needs[g] = 46;
        //  The octahedron or tetrahedron is taller than a flat sketch.
        if (sketch && column.sketchView == 1) {
          tall[g] = 2*sketchHalfHeight(column) + 4;
          if (needs[g] < tall[g]) needs[g] = tall[g];
        }
      }

      //  In the order they are DRAWN: a fragment column is not sorted by
      //  energy (placeFragments() moves each level to the mean of the
      //  orbitals it became), so array order would stack it wrongly.
      vector<double> en;
      vector<bool> occ;
      for (size_t g = 0; g < groups.size(); g++) {
        en.push_back(column.levels[groups[g].first].energy);
        bool o = false;
        for (size_t i = groups[g].first; i <= groups[g].second; i++)
          if (column.levels[i].occupancy > 0.0 || column.levels[i].occupancyBeta > 0.0) o = true;
        occ.push_back(o);
      }
      openRows(groupY, [&](size_t upper, size_t lower) {
                 const int g0 = (occ[upper] && occ[lower]) ? max(needs[upper], 2*(int)ARROW_HALF) : needs[upper];
                 return max(g0, (tall[upper] + tall[lower])/2); },
               plot.y + 6, plot.y + plot.height - 6, &en);
    }

    /**
     * Opens a column out vertically: every row its own height, in the order
     * of `y`, at least gap(upper, lower) apart, the stack kept centred on
     * where the rows really are and inside [lo, hi] (squeezed in order when
     * there is not room).  One implementation for every column, so the
     * correlation lines and the levels get the same answer.
     */
    static void openRows(vector<int>& y,
                         const std::function<int(size_t, size_t)>& gap,
                         int lo, int hi, const vector<double>* energy = 0)
    {
      if (y.size() < 2) return;
      //  Rows on one pixel keep the order of their energies.
      vector< std::pair<int,size_t> > byHeight;
      for (size_t g = 0; g < y.size(); g++) byHeight.push_back(std::make_pair(y[g], g));
      std::sort(byHeight.begin(), byHeight.end(),
                [&](const std::pair<int,size_t>& a, const std::pair<int,size_t>& b) {
                  if (a.first != b.first) return a.first < b.first;
                  if (energy != 0 && (*energy)[a.second] != (*energy)[b.second])
                    return (*energy)[a.second] > (*energy)[b.second];
                  return a.second < b.second; });

      double before = 0.0;
      for (size_t g = 0; g < y.size(); g++) before += y[g];

      for (size_t k = 1; k < byHeight.size(); k++) {
        const size_t up = byHeight[k-1].second, dn = byHeight[k].second;
        const int need = gap(up, dn);
        if (y[dn] - y[up] < need) y[dn] = y[up] + need;
      }

      //  Pushing down from the top accumulates downward; moving the stack
      //  back onto its own centre keeps every row as near its energy as
      //  the spacing allows.
      double after = 0.0;
      for (size_t g = 0; g < y.size(); g++) after += y[g];
      const int recentre = (int)((before - after)/(double)y.size());
      if (recentre != 0) for (size_t g = 0; g < y.size(); g++) y[g] += recentre;

      //  Inside [lo, hi]: only the rows that are in the way move, each keeping
      //  its gap; shifting the whole stack would squeeze a spread-out column.
      const size_t first = byHeight.front().second, last = byHeight.back().second;
      for (size_t k = byHeight.size(); k-- > 0; ) {
        const size_t g = byHeight[k].second;
        int limit = hi;
        if (k + 1 < byHeight.size()) limit = y[byHeight[k+1].second] - gap(g, byHeight[k+1].second);
        if (y[g] > limit) y[g] = limit;
      }
      for (size_t k = 0; k < byHeight.size(); k++) {
        const size_t g = byHeight[k].second;
        int limit = lo;
        if (k > 0) limit = y[byHeight[k-1].second] + gap(byHeight[k-1].second, g);
        if (y[g] < limit) y[g] = limit;
      }

      if (y[last] - y[first] > hi - lo) {
        const int y0 = y[first];
        const double span = y[last] - y0;
        for (size_t g = 0; g < y.size(); g++)
          y[g] = lo + (int)((y[g] - y0)*(hi - lo)/span);
      }
    }

    /**
     * The stretches of the axis where rows were pushed further apart than
     * their energies are: where two neighbours (in true order) ended up more
     * than 2 px further apart than `natural` puts them.  Each span is in
     * drawn coordinates, a little beyond its first and last row.
     */
    static void stretchedSpans(const vector<int>& natural, const vector<int>& shown,
                               vector< std::pair<int,int> >& spans)
    {
      vector< std::pair<int,size_t> > order;
      for (size_t g = 0; g < natural.size(); g++) order.push_back(std::make_pair(natural[g], g));
      std::sort(order.begin(), order.end());
      bool open = false;
      for (size_t k = 0; k + 1 < order.size(); k++) {
        const int a = shown[order[k].second], b = shown[order[k+1].second];
        const bool wide = (b - a) > (order[k+1].first - order[k].first) + 2;
        if (wide && !open) { spans.push_back(std::make_pair(a - 6, b + 6)); open = true; }
        else if (wide) spans.back().second = b + 6;
        else open = false;
      }
    }

    static bool inSpans(const vector< std::pair<int,int> >& spans, int y)
    {
      for (size_t i = 0; i < spans.size(); i++)
        if (y >= spans[i].first && y <= spans[i].second) return true;
      return false;
    }

    /**
     * The "not to scale" mark of a stretched stretch of axis: the axis goes
     * dotted and is closed by the same two slanted strokes as the broken axis.
     */
    static void drawStretchMarks(wxDC& dc, int axisX,
                                 const vector< std::pair<int,int> >& spans)
    {
      for (size_t i = 0; i < spans.size(); i++) {
        const int y0 = spans[i].first, y1 = spans[i].second;
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawRectangle(axisX - 5, y0, 11, y1 - y0 + 1);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(wxPen(wxColour(120, 120, 120), 1, wxPENSTYLE_DOT));
        dc.DrawLine(axisX, y0, axisX, y1);
        dc.SetPen(wxPen(wxColour(120, 120, 120), 1));
        dc.DrawLine(axisX - 6, y0 + 1, axisX + 6, y0 - 4);
        dc.DrawLine(axisX - 6, y1 + 4, axisX + 6, y1 - 1);
      }
    }

    /** Merged non-bonding levels: one grey block per band. */
    void drawBands(wxDC& dc, const MoColumn& column, int x, int width,
                   const wxRect& plot)
    {
      for (size_t i = 0; i < column.bands.size(); i++) {
        const MoBand& b = column.bands[i];
        int top = yFor(b.high, plot), bottom = yFor(b.low, plot);
        dc.SetFont(*wxSMALL_FONT);
        const int row = dc.GetTextExtent(wxT("Xg")).y;
        if (bottom - top < row + 6) {
          const int mid = (top + bottom)/2;
          top = mid - row/2 - 3;
          bottom = mid + row/2 + 3;
        }
        dc.SetBrush(wxBrush(wxColour(222, 222, 222)));
        dc.SetPen(wxPen(wxColour(160, 160, 160), 1));
        dc.DrawRectangle(x, top, width, bottom - top);
        const wxString text(b.label.c_str(), wxConvUTF8);
        const wxSize ext = dc.GetTextExtent(text);
        dc.SetTextForeground(wxColour(70, 70, 70));
        dc.DrawText(text, x + (width - ext.x)/2, top + 2);   // top edge: a level may cross the middle
      }
    }

    /** The row a single level sits on, for a line to end at. */
    int levelYIn(wxDC& dc, const MoColumn& column, const wxRect& plot,
                 bool spread, int which)
    {
      vector< std::pair<size_t,size_t> > groups;
      vector<int> groupY;
      columnGeometry(dc, column, plot, spread, groups, groupY);
      for (size_t g = 0; g < groups.size(); g++) {
        if ((size_t)which >= groups[g].first &&
            (size_t)which <= groups[g].second) {
          return groupY[g];
        }
      }
      return yFor(column.levels[which].energy, plot);
    }

    /** "A1  (2p)" as its symmetry part and its shell part. */
    static void splitShellLabel(const string& label, string& irrep,
                                string& shell)
    {
      irrep = label;
      shell.clear();
      const string::size_type open = label.find('(');
      if (open == string::npos) return;
      const string::size_type close = label.find(')', open);
      if (close == string::npos) return;

      shell = label.substr(open + 1, close - open - 1);
      irrep = label.substr(0, open);
      while (!irrep.empty() && isspace((unsigned char)irrep[irrep.size()-1])) {
        irrep.erase(irrep.size() - 1);
      }
    }

    /** The most lines any one row of either fragment column holds. */
    static int widestRow(const MoColumn& column)
    {
      int widest = 1;
      const vector<MoLevel>& levels = column.levels;
      for (size_t a = 0; a < levels.size(); ) {
        size_t b = a;
        int lines = levels[a].degeneracy > 0 ? levels[a].degeneracy : 1;
        while (b + 1 < levels.size() &&
               fabs(levels[b+1].energy - levels[a].energy) < 1.0e-9) {
          b++;
          lines += levels[b].degeneracy > 0 ? levels[b].degeneracy : 1;
        }
        if (lines > widest) widest = lines;
        a = b + 1;
      }
      return widest;
    }


    //  THE MODEL'S MIDDLE COLUMN.
    //
    //  Every level has its own row, in energy order, one label height apart at
    //  least, and the label is on the row of its line.  A crowded range is
    //  opened out vertically, never sideways: a level drawn beside another
    //  reads as a separate group.
    struct CentreLane
    {
      int lane;        // always 0: one lane
      int lineX;       // left of the lines, from the column's start
      int y;           // the level
      int slotsW;      // width of the lines
      int labelY;      // centre of the label's row
      int labelW;      // width of the label's row
      int labelH;      // height of the label's row
      int top, bottom; // the arrows
      int linkL, linkR;  // where the links from the left and right fragment columns meet the level
    };
    vector<CentreLane> p_cLanes;
    int p_cLaneTotal = 0;
    int p_cLabelRight = 0;          // right edge of the label column, from the column's start
    vector< std::pair<int,int> > p_calcSpans;     // ... by the calculation column
    int p_zeroY = -1;                             // 0 eV on the model's arrow, -1 if none
    vector<CalcItem> p_calcRows;
    vector<int> p_fragSketch[2];              // half height of the sketch box of each row, 0 for none
    vector< std::pair<int,bool> > p_fragRows[2];  // fragment columns as drawn: (y, has arrows)                  // the calculation column as laid out

    int centreSlot() const { return 24; }

    /**
     * The label row of a centre level: the construction label (left) and the
     * symmetry label (right).  `conText` is the construction label as the mode
     * gives it, `wCon` its measured width.
     */
    void centreRow(wxDC& dc, const MoLevel& l, vector<Piece>& pieces, int& wLabel,
                   int& wCon, string& conText, int mode) const
    {
      wxFont base = *wxSMALL_FONT, small = base;
      small.SetPointSize(base.GetPointSize() - 2 > 4 ? base.GetPointSize() - 2 : 5);
      pieces.clear();
      wLabel = 0;
      if (p_showSymmetry) {
        splitLabel(l.symLabel.empty() ? l.label : l.symLabel, pieces);
        wLabel = measurePieces(dc, pieces, base, small).x;
      }
      conText = !p_showConstruct || mode >= 2 ? string() : (mode == 0 ? l.construct : l.constructShort);
      dc.SetFont(base);
      wCon = conText.empty() ? 0 : dc.GetTextExtent(wxString(conText.c_str(), wxConvUTF8)).x;
    }

    /** Every centre level's row, and the width the column needs. */
    int layoutCentreLanes(wxDC& dc, const wxRect& plot, int maxWidth)
    {
      const int n = (int)p_centre.levels.size();
      p_cLanes.assign(n, CentreLane());
      dc.SetFont(*wxSMALL_FONT);
      const int row = dc.GetTextExtent(wxT("Xg")).y;
      const int R = row + 2;

      int conCol = 0, symCol = 0, slotsMax = 0;
      vector<int> natural(n), half(n);
      for (int i = 0; i < n; i++) {
        const int count = p_centre.levels[i].degeneracy > 0 ? p_centre.levels[i].degeneracy : 1;
        slotsMax = max(slotsMax, count*centreSlot());
      }
      //  The construction label gives way first (parents, then the whole label)
      //  when the measured rows do not fit between the fragment columns.
      const int budget = maxWidth - slotsMax - 16;
      p_cMode = 2;
      for (int mode = 0; mode < 3; mode++) {
        int wcMax = 0, wlMax = 0;
        for (int i = 0; i < n; i++) {
          vector<Piece> pieces;
          int wl, wc;
          string text;
          centreRow(dc, p_centre.levels[i], pieces, wl, wc, text, mode);
          wcMax = max(wcMax, wc);
          wlMax = max(wlMax, wl);
        }
        if (wcMax + wlMax + 16 <= budget || mode == 2) { p_cMode = mode; conCol = wcMax; symCol = wlMax; break; }
      }
      slotsMax = 0;
      for (int i = 0; i < n; i++) {
        const MoLevel& l = p_centre.levels[i];
        CentreLane& c = p_cLanes[i];
        c.labelH = row;
        const int count = l.degeneracy > 0 ? l.degeneracy : 1;
        c.slotsW = count*centreSlot();
        slotsMax = max(slotsMax, c.slotsW);
        natural[i] = yFor(l.energy, plot);
        //  Electron arrows are 18 px tall; an empty level needs only its label.
        half[i] = (l.occupancy > 0.0 || l.occupancyBeta > 0.0) ? max(R/2, (int)ARROW_HALF) : R/2;
      }

      vector<int> y = natural;
      vector<double> en;
      for (int i = 0; i < n; i++) en.push_back(p_centre.levels[i].energy);
      openRows(y, [&](size_t a, size_t b) { return half[a] + half[b]; },
               plot.y + R/2, plot.y + plot.height - R/2, &en);

      //  0 eV between the highest negative level and the lowest positive one as
      //  drawn (bound levels below, antibonding EHT levels above).
      p_zeroY = -1;
      if (n > 0) {
        int negTop = 1 << 29, posBottom = -(1 << 29);
        for (int i = 0; i < n; i++) {
          if (p_centre.levels[i].energy < 0.0) negTop = min(negTop, y[i]);
          else posBottom = max(posBottom, y[i]);
        }
        const int lim0 = plot.y + 6, lim1 = plot.y + plot.height - 6;
        if (negTop < (1 << 29) && posBottom > -(1 << 29)) p_zeroY = (negTop + posBottom)/2;
        else if (negTop < (1 << 29)) p_zeroY = max(lim0, negTop - R);
        else p_zeroY = min(lim1, posBottom + R);
      }

      //  construction label | lines | symmetry label; the right-hand links start after the latter.
      const int lineX = conCol + (conCol ? 12 : 6);
      for (int i = 0; i < n; i++) {
        const int count = p_centre.levels[i].degeneracy > 0 ? p_centre.levels[i].degeneracy : 1;
        p_cLanes[i].labelW = conCol + symCol;
        p_cLanes[i].linkL = lineX - 2 - (conCol ? conCol + 8 : 0);
        p_cLanes[i].linkR = lineX + count*centreSlot() - 2 + (symCol ? symCol + 8 : 0);
      }
      p_cLabelRight = lineX;
      for (int i = 0; i < n; i++) {
        CentreLane& c = p_cLanes[i];
        c.lane = 0;
        c.lineX = lineX;
        c.y = y[i];
        c.labelY = y[i];
        c.top = y[i] - half[i];
        c.bottom = y[i] + half[i];
      }
      p_cLaneTotal = lineX + slotsMax + (symCol ? symCol + 8 : 0);
      return p_cLaneTotal;
    }

    /** The model's middle column: lines, electrons, labels and hit boxes. */
    void drawCentreLanes(wxDC& dc, int x0, const wxRect& plot, vector<Hit>* hits)
    {
      p_outward = -1;
      p_sketchColumn = &p_centre;
      const int slot = centreSlot();
      wxFont base = *wxSMALL_FONT, small = base;
      small.SetPointSize(base.GetPointSize() - 2 > 4 ? base.GetPointSize() - 2 : 5);
      wxFont tagFont = base;
      tagFont.SetPointSize(base.GetPointSize() - 1 > 5 ? base.GetPointSize() - 1 : 5);
      for (size_t i = 0; i < p_centre.levels.size(); i++) {
        const MoLevel& level = p_centre.levels[i];
        const CentreLane& c = p_cLanes[i];
        const int count = level.degeneracy > 0 ? level.degeneracy : 1;
        const wxColour colour = levelColour(level);
        dc.SetPen(wxPen(colour, 2));
        for (int d = 0; d < count; d++) {
          const int lx = x0 + c.lineX + d*slot;
          dc.DrawLine(lx, c.y, lx + slot - 4, c.y);
        }
        drawElectrons(dc, level, x0 + c.lineX, slot, slot - 4, c.y, 0, plot);

        //  The label row, right-aligned at the label column's edge; lines
        //  run behind the text, not through it.
        vector<Piece> pieces;
        int wl, wc;
        string conText;
        centreRow(dc, level, pieces, wl, wc, conText, p_cMode);
        dc.SetFont(base);
        const int row = dc.GetTextExtent(wxT("Xg")).y;
        const int top = c.labelY - row/2;
        const int xCon = x0 + c.lineX - 6 - wc;
        const int xSym = x0 + c.lineX + count*slot + 4;
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(*wxWHITE_BRUSH);
        if (wc) dc.DrawRectangle(xCon - 2, top, wc + 4, row);
        if (wl) dc.DrawRectangle(xSym - 2, top, wl + 4, row);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetTextForeground(colour);
        if (wc) dc.DrawText(wxString(conText.c_str(), wxConvUTF8), xCon, top);
        if (wl) drawPieces(dc, pieces, xSym, top, base, small, row);
        if (abs(c.labelY - c.y) > 3 || c.lane > 0) {
          dc.SetPen(wxPen(wxColour(200, 200, 200), 1));
          dc.DrawLine(x0 + p_cLabelRight + 3, c.labelY, x0 + c.lineX - 3, c.y);
        }
        if (hits != 0 && !level.orbitals.empty()) {
          for (int d = 0; d < count; d++) {
            const size_t oi = (size_t)d < level.orbitals.size()
                              ? (size_t)d : level.orbitals.size() - 1;
            Hit hit;
            hit.box = wxRect(x0 + c.lineX + d*slot - 6, c.y - 7, slot + 8, 15);
            hit.orbital = level.orbitals[oi];
            hit.level = (int)i;
            hits->push_back(hit);
          }
        }
      }
    }

  public:
    /** 1 when the 0 eV tick is not between the negative and the positive model levels. */
    int zeroTickProblems() const
    {
      if (p_zeroY < 0) return 0;
      for (size_t i = 0; i < p_cLanes.size() && i < p_centre.levels.size(); i++) {
        const bool neg = p_centre.levels[i].energy < 0.0;
        if (neg ? p_cLanes[i].y <= p_zeroY : p_cLanes[i].y >= p_zeroY) return 1;
      }
      return 0;
    }

    /**
     * Test hook: problems in the model and calculation columns.  Counts
     * pairs of levels whose arrows or label boxes overlap, and pairs that
     * are not in one lane (drawn sideways of each other).
     */
    int centreOverlaps() const
    {
      //  Measured from the drawn heights and the arrow glyph's real size,
      //  not from the boxes the layout reserved.
      int n = 0;
      for (size_t a = 0; a < p_cLanes.size(); a++)
        for (size_t b = a + 1; b < p_cLanes.size(); b++) {
          const CentreLane &u = p_cLanes[a], &v = p_cLanes[b];
          const int d = abs(u.y - v.y);
          if (u.lineX != v.lineX) n++;
          if (hasArrows(p_centre.levels[a].occupancy, p_centre.levels[a].occupancyBeta) &&
              hasArrows(p_centre.levels[b].occupancy, p_centre.levels[b].occupancyBeta) &&
              d < 2*ARROW_HALF) n++;
          if (d < u.labelH) n++;
        }
      for (size_t a = 0; a < p_calcRows.size(); a++)
        for (size_t b = a + 1; b < p_calcRows.size(); b++) {
          const CalcItem &u = p_calcRows[a], &v = p_calcRows[b];
          const MoLevel &lu = itemLevel(u), &lv = itemLevel(v);
          const int d = abs(u.y - v.y);
          if (u.x != v.x) n++;
          if (hasArrows(lu.calcOccupancy, lu.calcOccupancyBeta) &&
              hasArrows(lv.calcOccupancy, lv.calcOccupancyBeta) && d < 2*ARROW_HALF) n++;
          if (d < 14) n++;
        }
      for (int c = 0; c < 2; c++)
        for (size_t a = 0; a < p_fragRows[c].size(); a++)
          for (size_t b = a + 1; b < p_fragRows[c].size(); b++) {
            const int d = abs(p_fragRows[c][a].first - p_fragRows[c][b].first);
            if (p_fragRows[c][a].second && p_fragRows[c][b].second && d < 2*ARROW_HALF) n++;
            if (d < 12) n++;
            //  Sketch boxes, each centred on its row.
            if (b < p_fragSketch[c].size() && a < p_fragSketch[c].size() &&
                p_fragSketch[c][a] > 0 && p_fragSketch[c][b] > 0 &&
                d < p_fragSketch[c][a] + p_fragSketch[c][b]) n++;
          }
      n += zeroTickProblems();
      return n;
    }

    static bool hasArrows(double a, double b) { return a > 0.0 || b > 0.0; }

  private:

    void drawColumn(wxDC& dc, const MoColumn& column, int x, int width,
                    const wxRect& plot, bool withOccupancy,
                    vector<Hit>* hits, int outward = -1,
                    bool spread = false)
    {
      //  Which side this column's labels and sketches go.  Outward, so
      //  nothing is written over the correlation lines in the middle --
      //  the right-hand column was labelling leftward like every other,
      //  which put its phase sketches in the centre of the diagram.
      p_outward = outward;

      //  The column being drawn, so drawLabel can reach its sketch
      //  positions without them being threaded through every call.
      p_sketchColumn = &column;

      const int lineGap = 4;

      vector<int> groupY;
      vector< std::pair<size_t,size_t> > groups;
      columnGeometry(dc, column, plot, spread, groups, groupY);

      for (size_t gi = 0; gi < groups.size(); gi++) {

        const size_t first = groups[gi].first;
        const size_t last  = groups[gi].second;
        int lines = 0;
        for (size_t i = first; i <= last; i++) {
          lines += column.levels[i].degeneracy > 0
                   ? column.levels[i].degeneracy : 1;
        }

        const int y = groupY[gi];
        const int slot = (lines > 0) ? (width + lineGap)/lines : width;
        const int lineWidth = max(8, slot - lineGap);

        vector<string> rowNames;
        int drawn = 0;
        for (size_t i = first; i <= last; i++) {
          const MoLevel& level = column.levels[i];
          const int count = level.degeneracy > 0 ? level.degeneracy : 1;
          const int groupLeft = x + drawn*slot;

          const wxColour colour = levelColour(level);
          dc.SetPen(wxPen(colour, 2));

          //  A DEGENERATE SET WHOSE ORBITALS ARE NOT AT ONE ENERGY is
          //  drawn at their own heights and bracketed with a brace.
          //  They are one level because their irrep is one irrep, and
          //  averaging them into a single line claims a number that
          //  none of them has.  Only where the difference can be seen:
          //  finite precision spreads every degenerate set a little,
          //  and bracketing all of them would be noise.
          const bool split = spreadIsVisible(level, plot);
          int lowest = y, highest = y;

          for (int d = 0; d < count; d++) {
            const int lx = x + (drawn + d)*slot;
            const int ly = split ? yFor(level.energies[d], plot) : y;
            dc.DrawLine(lx, ly, lx + lineWidth, ly);
            if (ly < lowest)  lowest = ly;
            if (ly > highest) highest = ly;
          }
          if (split) drawBrace(dc, x + drawn*slot + count*slot - lineGap + 4,
                               lowest, highest);

          const int centreY = split ? (lowest + highest)/2 : y;

          if (withOccupancy) {
            drawElectrons(dc, level, x + drawn*slot, slot, lineWidth,
                          centreY, split ? &level : 0, plot);
          }

          //  A ROW OF SEVERAL SYMMETRY COMPONENTS IS LABELLED ONCE.
          //
          //  A shell's components are drawn side by side on one row,
          //  so each of them carrying the full "A1  (2p)" printed
          //  three labels over one another in a slot a third as wide.
          //  The shell is named once, at the left of the row; each
          //  line is marked with its own symmetry, which is how the
          //  set is drawn by hand.
          if (first != last) {
            MoLevel marked = level;
            string irrep, shell;
            splitShellLabel(level.label, irrep, shell);
            marked.label = irrep;
            marked.annotation.clear();

            //  ONLY IF IT FITS IN ITS OWN SLOT.
            //
            //  A row of several levels gives each one a slot and puts
            //  its label at the left of it, which works while the
            //  labels are short. Benzene's degenerate pairs are not:
            //  "2A''" and "3A''" are each wider than the slot, so
            //  they ran into one another and printed as one unreadable
            //  run -- 2A''nb3A''nb. Where they do not fit, the row is
            //  named once below, and each line is left to the brace
            //  and the row's own label.
            dc.SetFont(*wxSMALL_FONT);
            const int room = count*slot - lineGap;
            if (dc.GetTextExtent(wxString(marked.label.c_str(),
                                          wxConvUTF8)).x <= room) {
              drawLabel(dc, marked, groupLeft, room, centreY,
                        false, plot, colour);
            } else {
              rowNames.push_back(marked.label);
            }
          } else {
            drawLabel(dc, level, groupLeft, count*slot - lineGap, centreY,
                      true, plot, colour);
          }

          //  ONE HIT BOX PER DRAWN COMPONENT, NOT ONE FOR THE WHOLE ROW.
          //
          //  A degenerate level's `count` components are drawn side by
          //  side, each in its own slot (see the DrawLine loop above,
          //  lx = x + (drawn+d)*slot) -- but until now the click region
          //  covering all of them resolved to the single hardcoded
          //  level.orbitals[0], so clicking anywhere on a degenerate
          //  row always showed the SAME member's 3-D MO, never the
          //  others, which is exactly why every orbital of a degenerate
          //  set looked identical when clicked (found live, 2026-10-02).
          //  Each component gets its own box over its own slot and its
          //  own orbital index, split's own y where the level is drawn
          //  spread out.
          if (hits != 0 && !level.orbitals.empty()) {
            for (int d = 0; d < count; d++) {
              const int lx = x + (drawn + d)*slot;
              const int ly = split ? yFor(level.energies[d], plot) : y;
              const size_t oi = (size_t)d < level.orbitals.size()
                                ? (size_t)d : level.orbitals.size() - 1;
              Hit hit;
              hit.box = wxRect(lx - 6, ly - 7, slot + 12, 15);
              hit.orbital = level.orbitals[oi];
              hit.level = (int)i;
              hits->push_back(hit);
            }
          }
          drawn += count;
        }

        if (first != last) {
          string irrep, shell;
          splitShellLabel(column.levels[first].label, irrep, shell);
          //  Only a fragment column names a shell; a molecular level
          //  is its own label and has nothing to factor out.
          if (!shell.empty()) {
            MoLevel row;
            row.label      = shell;
            row.annotation = column.levels[first].annotation;
            row.degeneracy = 1;
            drawLabel(dc, row, x, width, y, true, plot,
                      wxColour(0, 0, 0));
          } else if (!rowNames.empty()) {
            //  The names that would not fit over their own lines,
            //  written once for the row instead.
            string joined;
            for (size_t k = 0; k < rowNames.size(); k++) {
              if (k) joined += " ";
              joined += rowNames[k];
            }
            MoLevel row;
            row.label      = joined;
            row.degeneracy = 1;
            drawLabel(dc, row, x, width, y, true, plot,
                      wxColour(0, 0, 0));
          }
        }
      }
    }

  private:

    /**
     * A level's label, left of its lines, and its annotation right.
     *
     * The first level at a given energy takes the left-hand position;
     * any others sharing that energy are labelled above their own
     * lines, since there is only one left-hand position and they would
     * otherwise all claim it.
     */
    void drawLabel(wxDC& dc, const MoLevel& level, int x, int width,
                   int y, bool leftHand, const wxRect& plot,
                   const wxColour& colour)
    {
      if (level.label.empty() || level.label == "?") return;

      dc.SetFont(*wxSMALL_FONT);
      dc.SetTextForeground(colour);

      wxFont base = *wxSMALL_FONT;
      wxFont small = base;
      small.SetPointSize(base.GetPointSize() - 2 > 4
                             ? base.GetPointSize() - 2 : 5);

      vector<Piece> pieces;
      splitLabel(level.label, pieces);
      const wxSize extent = measurePieces(dc, pieces, base, small);

      if (leftHand) {
        //  Nudged aside when the level above it took the same row, so
        //  two close-but-distinct levels do not overprint their labels
        //  -- which is what A' and A'' did all the way down methanol.
        //  Pushed out one notch at a time until it clears every label
        //  already placed at a similar height, rather than once past
        //  the previous one: a crowded spectrum has several levels
        //  within a few pixels and one nudge leaves them piled up.
        //  At most two notches out.  A spectrum can put four levels
        //  within a pixel of each other, and pushing a label further
        //  for every one of them marched them into the next column --
        //  "5a1 nb" ended up printed over the carbon's "T2 (2p)".
        //  Beyond two they overlap, which is honest about the levels
        //  being that close and does not damage another column.
        //  A FREE ROW, TESTED AGAINST WHERE LABELS ACTUALLY WENT.
        //
        //  Counting notches does not work: the levels are a few
        //  pixels apart to begin with, so "one row up from mine" and
        //  "two rows down from yours" can land on the same pixel.
        //  CCl4 has 1t1, 3t2 and 1e within two hundredths of a
        //  Hartree and two of the three printed on top of each other
        //  whatever the step arithmetic said.
        //
        //  So try this level's own row first, then a row above, a row
        //  below, two above, two below, and take the first that
        //  clears every label already placed.  Vertical, because
        //  pushing them sideways walks them into the next column.
        const string annotation = p_showEnergies ? level.annotation : string();
        //  The character tag (grey, a size smaller) is a line of its own under
        //  the energy; the block is as wide as its widest line, measured.
        wxFont tagFont = base;
        tagFont.SetPointSize(base.GetPointSize() - 1 > 5 ? base.GetPointSize() - 1 : 5);
        const wxString tagText(level.tag.c_str(), wxConvUTF8);
        int tagW = 0;
        if (!level.tag.empty()) {
          dc.SetFont(tagFont);
          tagW = dc.GetTextExtent(tagText).x;
          dc.SetFont(base);
        }
        const int blockW = max(extent.x, tagW);
        const int lines = 1 + (annotation.empty() ? 0 : 1) + (level.tag.empty() ? 0 : 1);
        const int tall = lines*extent.y + 2;
        const int home = y - extent.y/2;

        //  SIDEWAYS FIRST, THEN A FREE ROW.
        //
        //  Two levels a pixel apart need their labels apart, and
        //  beside each other is what a person writes -- stacking them
        //  vertically puts a label where no level is.  Capped at two
        //  steps, because three marched "3t2 nb" into the chlorine
        //  column; past that, a row above or below.
        //  Past that, a free row at the first position, nearest first,
        //  joined to its level by a leader; labels may not overlap.
        int step = 0, ly = home;
        bool found = false;
        auto free = [&](int cand, int sideways) {
          for (size_t k = 0; k < p_placed.size(); k++)
            if (p_placed[k].second == sideways &&
                abs(cand - p_placed[k].first) < tall) return false;
          return true;
        };
        for (int attempt = 0; attempt < 3 && !found; attempt++)
          if (free(home, attempt)) { step = attempt; ly = home; found = true; }
        for (int k = 1; k <= 14 && !found; k++)
          for (int sign = 1; sign >= -1 && !found; sign -= 2) {
            const int cand = home + sign*k*tall;
            if (free(cand, 0)) { step = 0; ly = cand; found = true; }
          }
        if (!found) return;   // no room: the level itself is still drawn

        p_placed.push_back(std::make_pair(ly, step));

        //  Clear of the line by more than a couple of pixels: at
        //  eight it was printing over the left-hand end of the level
        //  it names.
        const int lx = (p_outward < 0)
            ? x - blockW - 14 - step*(blockW + 10)
            : x + width + 14 + step*(blockW + 10);
        //  Right-aligned block on the left of a column, left-aligned on the right.
        const int ox = (p_outward < 0) ? lx + blockW - extent.x : lx;

        if (abs(ly + extent.y/2 - y) > 3) {
          dc.SetPen(wxPen(wxColour(170, 170, 170), 1));
          dc.DrawLine((p_outward < 0) ? x - 2 : x + width + 2, y,
                      (p_outward < 0) ? lx + blockW + 2 : lx - 2, ly + extent.y/2);
        }
        drawPieces(dc, pieces, ox, ly, base, small, extent.y);

        //  Clear of the annotation, which is written under the label
        //  and is about as wide again: at thirty the chlorine 3p
        //  sketch was drawn over its own "free atom" line.
        drawSketch(dc, level, *p_sketchColumn,
                   (p_outward < 0) ? ox - sketchOffset(*p_sketchColumn)
                                   : ox + extent.x + sketchOffset(*p_sketchColumn) + 20,
                   ly + extent.y/2);

        //  ONCE PER SHELL.  Oxygen's b2, a1 and b1 2p levels all
        //  carry the same free-atom energy, and printing it beside
        //  each of the three said nothing three times and collided
        //  with the axis doing it.  It belongs to the shell, so it is
        //  drawn for the first level of the shell only.
        bool saidAlready = false;
        for (size_t k = 0; k < p_notes.size(); k++) {
          if (p_notes[k] == annotation) saidAlready = true;
        }
        if (!annotation.empty() && !saidAlready) {
          p_notes.push_back(annotation);
          dc.SetTextForeground(wxColour(130, 130, 130));
          const wxString extra(annotation.c_str(), wxConvUTF8);
          dc.DrawText(extra,
                      (p_outward < 0)
                          ? ox + extent.x - dc.GetTextExtent(extra).x : ox,
                      ly + extent.y);
        }
        if (!level.tag.empty()) {
          dc.SetFont(tagFont);
          dc.SetTextForeground(wxColour(130, 130, 130));
          dc.DrawText(tagText,
                      (p_outward < 0) ? ox + extent.x - tagW : ox,
                      ly + (annotation.empty() ? 1 : 2)*extent.y);
          dc.SetFont(base);
        }
      } else {
        drawPieces(dc, pieces, x + (width - extent.x)/2, y - extent.y - 6,
                   base, small, extent.y);

        //  Above its own lines.  The left-hand branch puts a sketch
        //  beside the label, but a component of a multi-component row
        //  has no left-hand position -- and chlorine's 3p symmetry
        //  orbitals are exactly the case the sketches exist for.
        drawSketch(dc, level, *p_sketchColumn, x + width/2,
                   y - extent.y - 26);
      }

    }

    /** Are this level's orbitals far enough apart to draw separately? */
    bool spreadIsVisible(const MoLevel& level, const wxRect& plot) const
    {
      if ((int)level.energies.size() != level.degeneracy ||
          level.degeneracy < 2) {
        return false;
      }
      int lowest = plot.height, highest = -plot.height;
      for (size_t i = 0; i < level.energies.size(); i++) {
        const int y = yFor(level.energies[i], plot);
        if (y < lowest)  lowest = y;
        if (y > highest) highest = y;
      }
      return (highest - lowest) >= 6;         // pixels
    }

    /** A curly brace bracketing one level's orbitals. */
    void drawBrace(wxDC& dc, int x, int top, int bottom)
    {
      const int w = 5;
      const int mid = (top + bottom)/2;
      dc.SetPen(wxPen(wxColour(90, 90, 90), 1));

      dc.DrawLine(x, top, x + w, top + 1);
      dc.DrawLine(x + w, top + 1, x + w, mid - 2);
      dc.DrawLine(x + w, mid - 2, x + 2*w, mid);
      dc.DrawLine(x + 2*w, mid, x + w, mid + 2);
      dc.DrawLine(x + w, mid + 2, x + w, bottom - 1);
      dc.DrawLine(x + w, bottom - 1, x, bottom);
    }

    /**
     * The phase pattern beside a symmetry orbital.
     *
     * Circles at the terminal atoms, filled for one sign and open for
     * the other, sized by weight.  It is what makes a TASO a picture
     * rather than a label: "a1" and "t2" both say how many orbitals,
     * and neither says which combination.
     */
    void drawSketch(wxDC& dc, const MoLevel& level, const MoColumn& column,
                    int cx, int cy)
    {
      if (level.phases.empty()) return;
      if (column.sketchView == 1) {
        drawSketch3D(dc, level, column, cx, cy);
        return;
      }

      const size_t atoms = column.sketchX.size();

      //  A P SYMMETRY ORBITAL POINTS SOMEWHERE, so three numbers an
      //  atom rather than one, and the sketch has to say where.  The
      //  component along the axis flattened away making these
      //  positions is perpendicular to the page -- the pi case -- and
      //  is drawn as a circle, filled or open, exactly as an s
      //  combination is.  The other two lie in the page and are drawn
      //  as a line through the atom, which is the orbital's axis.
      if (level.phases.size() == 3*atoms) {
        drawVectorSketch(dc, level, column, cx, cy);
        return;
      }
      if (level.phases.size() != atoms) return;

      double biggest = 0.0;
      for (size_t i = 0; i < level.phases.size(); i++) {
        if (fabs(level.phases[i]) > biggest) biggest = fabs(level.phases[i]);
      }
      if (biggest <= 0.0) return;

      const int reach = 11;          // pixels from the sketch's centre
      dc.SetPen(wxPen(wxColour(60, 60, 60), 1));

      for (size_t i = 0; i < level.phases.size(); i++) {
        const double w = level.phases[i];
        if (fabs(w) < 1.0e-6) continue;

        const int r = 2 + (int)(3.0*sqrt(fabs(w)/biggest));
        const int x = cx + (int)(column.sketchX[i]*reach);
        const int y = cy - (int)(column.sketchY[i]*reach);

        dc.SetBrush(w > 0.0 ? wxBrush(wxColour(60, 60, 60))
                            : *wxWHITE_BRUSH);
        dc.DrawCircle(x, y, r);
      }
      dc.SetBrush(*wxTRANSPARENT_BRUSH);
    }

    //  Oblique view of an octahedral or tetrahedral ligand set: the frame's
    //  Y is up, X across, and Z (depth) runs up and to the right.
    static constexpr double OBLIQUE_X = 0.45, OBLIQUE_Y = 0.25;
    static constexpr int    REACH_3D = 16;

    /** Half the height of the box a column's sketch needs, from the centre of its row. */
    static int sketchHalfHeight(const MoColumn& column)
    {
      double top = 0.0;
      const bool oblique = column.sketchView == 1;
      for (size_t i = 0; i < column.sketchY.size(); i++) {
        double y = fabs(column.sketchY[i]);
        if (oblique && i < column.sketchZ.size())
          y = fabs(column.sketchY[i] + OBLIQUE_Y*column.sketchZ[i]);
        if (y > top) top = y;
      }
      return (int)(top*(oblique ? REACH_3D : 11)) + 6;
    }

    /** A drawn sketch is centred this far out from its row's label block. */
    static int sketchOffset(const MoColumn& column) { return column.sketchView == 1 ? 46 : 34; }

    /** The octahedron or tetrahedron of a ligand set, signs on the donor atoms. */
    void drawSketch3D(wxDC& dc, const MoLevel& level, const MoColumn& column,
                      int cx, int cy)
    {
      const size_t n = column.sketchX.size();
      if (column.sketchZ.size() != n) return;
      const bool vec = level.phases.size() == 3*n;
      if (!vec && level.phases.size() != n) return;
      double biggest = 0.0;
      for (size_t i = 0; i < level.phases.size(); i++)
        if (fabs(level.phases[i]) > biggest) biggest = fabs(level.phases[i]);
      if (biggest <= 0.0) return;

      vector<int> sx(n), sy(n), order(n);
      for (size_t i = 0; i < n; i++) {
        sx[i] = cx + (int)((column.sketchX[i] + OBLIQUE_X*column.sketchZ[i])*REACH_3D);
        sy[i] = cy - (int)((column.sketchY[i] + OBLIQUE_Y*column.sketchZ[i])*REACH_3D);
        order[i] = (int)i;
      }
      //  Far ligands first, so the near ones are drawn over them.
      std::sort(order.begin(), order.end(), [&](int a, int b) {
        return column.sketchZ[a] > column.sketchZ[b]; });

      dc.SetPen(wxPen(wxColour(205, 205, 205), 1));
      for (size_t i = 0; i < n; i++) dc.DrawLine(cx, cy, sx[i], sy[i]);
      dc.SetBrush(wxBrush(wxColour(205, 205, 205)));
      dc.DrawCircle(cx, cy, 2);

      const wxColour ink(60, 60, 60);
      for (size_t o = 0; o < n; o++) {
        const size_t i = order[o];
        if (!vec) {
          const double w = level.phases[i];
          if (fabs(w) < 1.0e-6) continue;
          const int r = 2 + (int)(3.0*sqrt(fabs(w)/biggest));
          dc.SetPen(wxPen(ink, 1));
          dc.SetBrush(w > 0.0 ? wxBrush(ink) : *wxWHITE_BRUSH);
          dc.DrawCircle(sx[i], sy[i], r);
          continue;
        }
        const double vx = level.phases[3*i], vy = level.phases[3*i + 1], vz = level.phases[3*i + 2];
        const double len = sqrt(vx*vx + vy*vy + vz*vz);
        if (len < 0.08*biggest) continue;
        const double k = 8.0*(len/biggest)/len;
        const int dx = (int)((vx + OBLIQUE_X*vz)*k), dy = (int)((vy + OBLIQUE_Y*vz)*k);
        dc.SetPen(wxPen(ink, 2));
        dc.SetBrush(wxBrush(ink));
        dc.DrawLine(sx[i] - dx, sy[i] + dy, sx[i] + dx, sy[i] - dy);
        dc.DrawCircle(sx[i] + dx, sy[i] - dy, 2);
      }
      dc.SetBrush(*wxTRANSPARENT_BRUSH);
    }

    /** The same, for a symmetry orbital built from p shells. */
    void drawVectorSketch(wxDC& dc, const MoLevel& level,
                          const MoColumn& column, int cx, int cy)
    {
      const size_t atoms = column.sketchX.size();
      const int normal = (column.sketchNormal >= 0 &&
                          column.sketchNormal < 3) ? column.sketchNormal : 2;
      int axes[2], n = 0;
      for (int a = 0; a < 3; a++) if (a != normal) axes[n++] = a;

      double biggest = 0.0;
      for (size_t i = 0; i < level.phases.size(); i++) {
        if (fabs(level.phases[i]) > biggest) biggest = fabs(level.phases[i]);
      }
      if (biggest <= 0.0) return;

      const int reach = 11;
      for (size_t i = 0; i < atoms; i++) {
        const double out = level.phases[3*i + normal];
        const double inA = level.phases[3*i + axes[0]];
        const double inB = level.phases[3*i + axes[1]];

        const int x = cx + (int)(column.sketchX[i]*reach);
        const int y = cy - (int)(column.sketchY[i]*reach);

        if (fabs(out) > 0.08*biggest) {
          const int r = 2 + (int)(3.0*sqrt(fabs(out)/biggest));
          dc.SetPen(wxPen(wxColour(60, 60, 60), 1));
          dc.SetBrush(out > 0.0 ? wxBrush(wxColour(60, 60, 60))
                                : *wxWHITE_BRUSH);
          dc.DrawCircle(x, y, r);
        }

        const double length = sqrt(inA*inA + inB*inB);
        if (length > 0.08*biggest) {
          const double scale = 9.0*(length/biggest)/length;
          const int dx = (int)(inA*scale);
          const int dy = (int)(inB*scale);
          dc.SetPen(wxPen(wxColour(60, 60, 60), 2));
          dc.DrawLine(x - dx, y + dy, x + dx, y - dy);
          //  A head on the positive end, so the sign is readable.
          dc.DrawCircle(x + dx, y - dy, 2);
        }
      }
      dc.SetBrush(*wxTRANSPARENT_BRUSH);
    }

    /** Electrons as arrows, one pair of slots per orbital. */
    void drawElectrons(wxDC& dc, const MoLevel& level, int x, int slot,
                       int lineWidth, int y, const MoLevel* split,
                       const wxRect& plot)
    {
      const int count = level.degeneracy > 0 ? level.degeneracy : 1;

      //  MERGED OPEN-SHELL LEVEL: the two spins are drawn independently,
      //  each by its own Hund's-rule fill, rather than as a single
      //  combined count -- see MoLevel::occupancyBeta's header comment.
      //  This is what lets O2's 1pi-g* come out as two unpaired arrows
      //  (two alpha electrons, no beta) instead of one up-down pair.
      if (level.occupancyBeta >= 0.0) {
        const int totalA = (int)(level.occupancy + 0.5);
        const int totalB = (int)(level.occupancyBeta + 0.5);
        if (totalA <= 0 && totalB <= 0) return;
        dc.SetPen(wxPen(*wxBLUE, 2));
        for (int d = 0; d < count; d++) {
          const bool up   = d < totalA;
          const bool down = d < totalB;
          if (!up && !down) continue;
          int yy = y;
          if (split != 0 && d < (int)split->energies.size())
            yy = yFor(split->energies[d], plot);
          const int cx = x + d*slot + lineWidth/2;
          if (up) {
            const int ax = cx - 4;
            dc.DrawLine(ax, yy - 9, ax, yy + 9);
            dc.DrawLine(ax, yy - 9, ax - 3, yy - 9 + 4);
            dc.DrawLine(ax, yy - 9, ax + 3, yy - 9 + 4);
          }
          if (down) {
            const int ax = cx + 4;
            dc.DrawLine(ax, yy - 9, ax, yy + 9);
            dc.DrawLine(ax, yy + 9, ax - 3, yy + 9 - 4);
            dc.DrawLine(ax, yy + 9, ax + 3, yy + 9 - 4);
          }
        }
        return;
      }

      const int total = (int)(level.occupancy + 0.5);
      if (total <= 0) return;

      //  HUND, NOT A DIVISION.  Dividing the occupancy by the
      //  degeneracy and rounding drew carbon's two 2p electrons as
      //  three arrows: 2/3 rounds to one, and one went onto each of
      //  the three lines.  One electron into every orbital of the set
      //  before any of them takes a second, which is what a person
      //  draws and what fillColumn() already computes.
      const int singles = (total < count) ? total : count;
      const int pairs   = total - singles;

      dc.SetPen(wxPen(*wxBLUE, 2));
      for (int d = 0; d < count; d++) {
        const int perOrbital = (d < singles ? 1 : 0) + (d < pairs ? 1 : 0);
        if (perOrbital <= 0) continue;
        //  On its own orbital's line where they were drawn apart.
        if (split != 0 && d < (int)split->energies.size()) {
          y = yFor(split->energies[d], plot);
        }
        const int cx = x + d*slot + lineWidth/2;
        for (int e = 0; e < perOrbital && e < 2; e++) {
          const int ax = cx + (e == 0 ? -4 : 4);
          const int dir = (e == 0) ? -1 : 1;      // up then down
          dc.DrawLine(ax, y - 9, ax, y + 9);
          dc.DrawLine(ax, y + dir*9, ax - 3, y + dir*9 - dir*4);
          dc.DrawLine(ax, y + dir*9, ax + 3, y + dir*9 - dir*4);
        }
      }
    }

  public:

    void onPaint(wxPaintEvent&)
    {
      wxAutoBufferedPaintDC dc(this);
      dc.SetBackground(*wxWHITE_BRUSH);
      dc.Clear();
      paintOnto(dc, GetClientSize());
    }

    /**
     * Draw onto any device context, at any size.
     *
     * Separated from the paint handler so the drawing can be sent to a
     * bitmap instead of a window, which is what makes it checkable
     * without a person looking at a screen.  The handler does nothing
     * but supply the window's own context and size.
     */
    void paintOnto(wxDC& dc, const wxSize& size)
    {
      if (p_centre.levels.empty()) {
        int y = 20;
        if (!p_headline.empty()) {
          wxFont banner = dc.GetFont();
          banner.SetPointSize(banner.GetPointSize() + 3);
          banner.SetWeight(wxFONTWEIGHT_BOLD);
          dc.SetFont(banner);
          dc.SetTextForeground(wxColour(140, 90, 0));
          dc.DrawText(wxString(p_headline.c_str(), wxConvUTF8), 20, y);
          y += dc.GetTextExtent(wxT("Xg")).y + 12;
          dc.SetTextForeground(wxColour(30, 30, 30));
        }
        dc.SetFont(*wxNORMAL_FONT);
        dc.DrawText(wxString(p_note.c_str(), wxConvUTF8), 20, y);
        return;
      }

      //  The model-first diagram: model energies are drawn in eV and the
      //  calculation is a narrow column of text on the right.
      const bool model = p_centre.levels[0].model;
      const double unit = model ? 27.211386245988 : 1.0;

      //  Energy range over everything that will be drawn; levels above an
      //  axis break are laid out in a strip of their own.
      p_lo = p_hi = p_centre.levels[0].energy;
      p_stripH = 0;
      p_stripRows = 0;
      p_stripLo = 1e30; p_stripHi = -1e30;
      const MoColumn* cols[3] = { &p_left, &p_centre, &p_right };
      {
        bool any = false;
        for (size_t i = 0; i < p_centre.levels.size(); i++) {
          if (!p_centre.levels[i].clipped) continue;
          any = true;
          p_stripLo = min(p_stripLo, p_centre.levels[i].energy);
          p_stripHi = max(p_stripHi, p_centre.levels[i].energy);
        }
        bool valence = false;
        for (int c = 0; c < 3; c++) {
          for (size_t i = 0; i < cols[c]->levels.size(); i++) {
            if (any && cols[c]->levels[i].energy >= p_stripLo - 1.0e-9) continue;
            if (!valence) { p_lo = p_hi = cols[c]->levels[i].energy; valence = true; }
            p_lo = min(p_lo, cols[c]->levels[i].energy);
            p_hi = max(p_hi, cols[c]->levels[i].energy);
          }
        }
        if (any && valence) {
          //  One label row (44 pixels) per distinct strip energy, at most a third of the plot.
          int rows = 1;
          vector<double> seen;
          for (size_t i = 0; i < p_centre.levels.size(); i++) {
            if (!p_centre.levels[i].clipped) continue;
            bool dup = false;
            for (size_t k = 0; k < seen.size(); k++) if (fabs(seen[k] - p_centre.levels[i].energy) < 1e-9) dup = true;
            if (!dup) { seen.push_back(p_centre.levels[i].energy); }
          }
          rows = (int)seen.size();
          p_stripRows = rows;
          p_breakGap = 26;
        } else if (any) {
          //  Nothing below the strip: no break to draw.
          p_stripLo = 1e30; p_stripHi = -1e30;
          for (int c = 0; c < 3; c++)
            for (size_t i = 0; i < cols[c]->levels.size(); i++) {
              p_lo = min(p_lo, cols[c]->levels[i].energy);
              p_hi = max(p_hi, cols[c]->levels[i].energy);
            }
        }
      }
      if (p_hi - p_lo < 1e-6) { p_hi = p_lo + 1.0; }
      const double pad = 0.06*(p_hi - p_lo);
      p_lo -= pad;
      p_hi += pad;
      p_kE.clear(); p_kF.clear();

      //  THE POINT GROUP IS THE DIAGRAM'S PREMISE, so it is a heading
      //  and not a footnote: every label below it -- a1, t2, eg -- is a
      //  name in that group and means nothing without it, and the same
      //  molecule in a lower group gives a different diagram entirely.
      //  THE FOOTER IS MEASURED FIRST AND STACKED, bottom upward.
      //
      //  Three separate things are written down there -- the wrapped
      //  note, the hint, and the count of folded core orbitals -- and
      //  each was placed at its own fixed offset from the bottom of
      //  the window.  They overlapped.  Moving one clear of another
      //  fixed one pair and left the third, which is what happens when
      //  three things share a space and none of them knows about the
      //  others.
      dc.SetFont(*wxSMALL_FONT);
      const int lineHeight = dc.GetTextExtent(wxT("Xg")).y + 1;
      const vector<wxString> noteLines = wrapNote(dc, size.x - 16);
      const vector<wxString> infoLines = p_showDescription
          ? wrapText(dc, p_info, size.x - 16) : vector<wxString>();
      vector<wxString> warnLines;
      for (size_t i = 0; i < p_warnings.size(); i++) {
        const vector<wxString> w = wrapText(dc, p_warnings[i], size.x - 16);
        warnLines.insert(warnLines.end(), w.begin(), w.end());
      }
      const wxString hintSentence = wxT("Click a level to show its orbital.");
      const bool hint = p_owner != 0;
      //  Only the click hint is permanent; the rest is in the About text.
      //  A diagram that could not be drawn keeps its reasons on screen.
      const bool refused = !p_headline.empty();
      if (!refused) warnLines.clear();
      int footerLines = (int)infoLines.size() + (int)warnLines.size() + (hint ? 1 : 0);
      if (p_summary.empty()) footerLines += (int)noteLines.size();
      if (p_centre.hiddenCount > 0)  footerLines += 2;      // band and text

      const int footerHeight = footerLines*lineHeight + 6;

      int top = 46;
      if (!p_group.empty()) {
        wxFont heading = *wxNORMAL_FONT;
        heading.SetWeight(wxFONTWEIGHT_BOLD);
        heading.SetPointSize(heading.GetPointSize() + 2);
        dc.SetFont(heading);
        dc.SetTextForeground(wxColour(30, 30, 30));

        //  Spelled the way a chemist writes it -- Td, C2v, D4h -- not
        //  the uppercase the generator's tables use.  First letter
        //  capital and the rest lower is right for all 46 of them.
        string pretty = p_group;
        for (string::size_type i = 0; i < pretty.size(); i++) {
          pretty[i] = (i == 0) ? toupper(pretty[i]) : tolower(pretty[i]);
        }
        //  The two infinite ones, which have no finite spelling.
        if (pretty == "Dinfh") pretty = "D\xe2\x88\x9eh";
        if (pretty == "Cinfv") pretty = "C\xe2\x88\x9ev";

        //  WITH A REAL SUBSCRIPT.  A point group is written with the
        //  order and the element as a subscript -- T_d, C_2v, D_4h --
        //  and printing "Td" flat is the sort of thing that makes a
        //  diagram look like it was drawn by something that does not
        //  know what it is drawing.  The Schoenflies letter is the
        //  first character; everything after it is the subscript.
        const wxString stem = wxString(pretty.substr(0, 1).c_str(),
                                       wxConvUTF8);
        const wxString sub  = wxString(pretty.substr(1).c_str(),
                                       wxConvUTF8);

        wxFont small = heading;
        small.SetPointSize(heading.GetPointSize() - 3);

        const wxSize stemSize = dc.GetTextExtent(stem);
        dc.SetFont(small);
        const wxSize subSize = dc.GetTextExtent(sub);
        dc.SetFont(heading);

        //  THE FORMULA FIRST, BECAUSE OF THE CHARGE.  Nitrite and
        //  nitrogen dioxide differ by one electron and their diagrams
        //  look nearly the same; the formula is where a reader checks
        //  which one is on the screen.
        vector<Piece> formula;
        splitFormula(p_formula, formula);
        const wxSize formulaSize =
            formula.empty() ? wxSize(0, 0)
                            : measurePieces(dc, formula, heading, small);
        const int gap = formula.empty() ? 0 : 18;

        const wxString tag = wxString::FromUTF8("\xC2\xB7 experimental");
        dc.SetFont(*wxSMALL_FONT);
        const wxSize tagSize = dc.GetTextExtent(tag);
        dc.SetFont(heading);
        const int tagGap = 12;
        const int wide = formulaSize.x + gap + stemSize.x + subSize.x +
                         tagGap + tagSize.x;
        int titleX = (size.x - wide)/2;

        if (!formula.empty()) {
          dc.SetTextForeground(wxColour(30, 30, 30));
          drawPieces(dc, formula, titleX, 6, heading, small,
                     formulaSize.y);
          titleX += formulaSize.x + gap;
        }

        dc.SetFont(heading);
        dc.DrawText(stem, titleX, 6);
        dc.SetFont(small);
        dc.DrawText(sub, titleX + stemSize.x,
                    6 + stemSize.y - subSize.y + 1);
        dc.SetFont(*wxSMALL_FONT);
        dc.SetTextForeground(wxColour(190, 120, 40));
        dc.DrawText(tag, titleX + stemSize.x + subSize.x + tagGap,
                    6 + stemSize.y - tagSize.y - 1);
        dc.SetTextForeground(wxColour(30, 30, 30));
        dc.SetFont(heading);
        top += 22;
      }
      //  THE REASON THE DIAGRAM IS MISSING, WHERE IT WILL BE READ.
      //
      //  Drawn across the width, above the levels, in the colour of a
      //  warning -- not appended to the footnote under the picture,
      //  which is where it used to be and where nobody looked.
      if (!p_headline.empty()) {
        wxFont banner = dc.GetFont();
        banner.SetPointSize(banner.GetPointSize() + 3);
        banner.SetWeight(wxFONTWEIGHT_BOLD);
        dc.SetFont(banner);

        const wxString text(p_headline.c_str(), wxConvUTF8);
        const wxSize extent = dc.GetTextExtent(text);

        //  A panel behind it, so it reads as a notice rather than as
        //  a stray caption over an empty diagram.
        const int pad = 8;
        const wxRect box((size.x - extent.x)/2 - 2*pad, top,
                         extent.x + 4*pad, extent.y + 2*pad);
        dc.SetBrush(wxBrush(wxColour(255, 244, 214)));
        dc.SetPen(wxPen(wxColour(214, 170, 60), 1));
        dc.DrawRectangle(box);

        dc.SetTextForeground(wxColour(140, 90, 0));
        dc.DrawText(text, (size.x - extent.x)/2, top + pad);
        dc.SetTextForeground(wxColour(30, 30, 30));

        top += box.height + 10;
        dc.SetFont(*wxNORMAL_FONT);
      }

      const wxRect plot(0, top, size.x,
                        max(40, size.y - top - footerHeight - 6));
      if (p_stripRows > 0) p_stripH = max(1, min(plot.height/3, 44*(p_stripRows - 1)));
      if (model) buildMap(cols);

      const int levelWidth = p_haveFragments ? 70 : 120;

      //  A FRAGMENT COLUMN IS AS WIDE AS ITS WIDEST ROW NEEDS.
      //
      //  A shell's symmetry components are drawn side by side on one
      //  row -- which is how the set is drawn by hand -- and chlorine
      //  brings four of them from its 3p, each wanting a symmetry
      //  label and a phase sketch of its own.  At one level width
      //  they were packed into seventeen pixels apiece and nothing
      //  could be read.  Bounded, so a column cannot eat the diagram.
      //  Each column on its own account: carbon brings one 2p row of
      //  three, chlorine brings a 3p row of twelve, and widening both
      //  to suit the wider one leaves the narrow column's lines
      //  stretched across empty space.
      const int leftWidth  = max(levelWidth,
                                 min(3*levelWidth, 34*widestRow(p_left)));
      const int rightWidth = max(levelWidth,
                                 min(3*levelWidth, 34*widestRow(p_right)));

      //  The molecular column too: a row of near-degenerate levels
      //  drawn side by side needs room for all of their lines, and
      //  CCl4 puts 3t2 and 1t1 -- six lines -- on one row.
      int centreWidth = max(levelWidth,
                            min(3*levelWidth, 26*widestRow(p_centre)));

      vector<CalcItem> calcItems;
      int calcAxisX = 0;
      const int calcWidth = model ? layoutCalc(dc, plot, size.x, calcItems, calcAxisX) : 0;
      const int usable  = size.x - calcWidth;
      const int xLeft   = usable/6 - (leftWidth - levelWidth)/2;
      const int xRight  = (5*usable)/6 - levelWidth
                          - (rightWidth - levelWidth)/2;
      //  The model's middle column (lanes) takes what lies between the
      //  fragment columns, but never less than a few lanes' worth.
      if (model) {
        const int between = xRight - (xLeft + leftWidth) - 80;
        centreWidth = layoutCentreLanes(dc, plot,
                                        max(100, min((int)(0.55*size.x), between)));
      }
      const int xCentre = usable/2 - centreWidth/2;
      const int xCalc   = size.x - calcWidth + 8;

      //  The energy axis.  A correlation diagram without one is a set
      //  of floating dashes: the reader has to be told which way is up
      //  and roughly how far apart the levels are, and the scale is no
      //  longer arbitrary now that every column is in eV.
      {
        const int ax = 22;
        dc.SetPen(wxPen(wxColour(120, 120, 120), 1));
        dc.SetFont(*wxSMALL_FONT);
        dc.SetTextForeground(wxColour(110, 110, 110));
        if (model) {
          //  Positions are monotone in energy but not on a scale, so the
          //  model side has an arrow and no numbers; each level prints its own.
          dc.DrawLine(ax, plot.y, ax, plot.y + plot.height);
          dc.DrawLine(ax, plot.y, ax - 4, plot.y + 7);
          dc.DrawLine(ax, plot.y, ax + 4, plot.y + 7);
          dc.DrawText(wxT("E"), 4, plot.y - 16);
          if (p_zeroY >= 0) {
            dc.DrawLine(ax - 4, p_zeroY, ax + 4, p_zeroY);
            dc.DrawText(wxT("0 eV"), ax + 7, p_zeroY - dc.GetTextExtent(wxT("X")).y/2);
          }
        } else {
        if (p_stripH > 0) {
          //  Broken axis: the strip above holds the high levels in order,
          //  not to scale; the break is two slanted strokes.
          const int valenceTop = plot.y + p_stripH + 4 + p_breakGap;
          const int mid = valenceTop - p_breakGap/2;
          dc.DrawLine(ax, plot.y + plot.height, ax, mid + 5);
          dc.DrawLine(ax, mid - 5, ax, plot.y);
          dc.DrawLine(ax - 6, mid + 6, ax + 6, mid + 1);
          dc.DrawLine(ax - 6, mid - 1, ax + 6, mid - 6);
          dc.SetFont(*wxSMALL_FONT);
          dc.SetTextForeground(wxColour(110, 110, 110));
          dc.DrawText(wxT("not to scale"), ax + 8, mid - dc.GetTextExtent(wxT("X")).y/2);
        } else {
          dc.DrawLine(ax, plot.y, ax, plot.y + plot.height);
        }
        dc.DrawLine(ax, plot.y, ax - 4, plot.y + 7);
        dc.DrawLine(ax, plot.y, ax + 4, plot.y + 7);

        //  Ticks on round numbers, at whatever spacing keeps them from
        //  colliding, so the spacing follows the spectrum rather than
        //  assuming a range.
        static const double steps[] = { 0.02, 0.05, 0.1, 0.2, 0.5,
                                        1, 2, 5, 10, 20, 50 };
        const double dLo = p_lo*unit, dHi = p_hi*unit;
        double step = steps[10];
        for (int i = 0; i < 11; i++) {
          if ((dHi - dLo)/steps[i] <= 12) { step = steps[i]; break; }
        }

        const double first = ceil(dLo/step)*step;
        for (double v = first; v <= dHi; v += step) {
          const int y = yFor(v/unit, plot);
          dc.DrawLine(ax - 3, y, ax + 3, y);
          //  Rounded to the step, so a tick that should read 0 does
          //  not come out as -5.55112e-17.  The offline renderer was
          //  fixed for this and the canvas was not: two implementations
          //  of one picture, and only one of them corrected.
          const double shown = (fabs(v) < 1.0e-9) ? 0.0
                                                  : floor(v/step + 0.5)*step;
          wxString text = wxString::Format(wxT("%g"), shown);
          dc.DrawText(text, ax + 6, y - dc.GetTextExtent(text).y/2);
        }
        dc.DrawText(wxT("E / Hartree"), 4, plot.y - 16);
        }
      }


      //  Column headings.
      dc.SetFont(*wxNORMAL_FONT);
      dc.SetTextForeground(wxColour(60, 60, 60));
      //  A COLUMN LEFT EMPTY (pi-only, with nothing of that fragment's
      //  in the visible pi system -- benzene's H column) IS DROPPED
      //  ENTIRELY, title included, rather than shown as a heading over
      //  blank space with nothing under it.
      //  Measured, not assumed: docked narrow, the right heading started
      //  inside "Molecular orbitals".  Abbreviate the centre one first,
      //  then push the outer two apart, inside the window.
      {
        const bool haveL = p_haveFragments && !p_left.levels.empty();
        const bool haveR = p_haveFragments && !p_right.levels.empty();
        const wxString tl(p_left.title.c_str(), wxConvUTF8);
        const wxString tr(p_right.title.c_str(), wxConvUTF8);
        wxString tc(p_centre.title.c_str(), wxConvUTF8);
        const int gap = 10;
        int xl = xLeft, xr = xRight;
        const int wl = haveL ? dc.GetTextExtent(tl).x : 0;
        const int wr = haveR ? dc.GetTextExtent(tr).x : 0;
        int wc = dc.GetTextExtent(tc).x;
        const bool crowded = (haveL && xl + wl + gap > xCentre) ||
                             (haveR && xCentre + wc + gap > xr);
        if (crowded && tc == wxT("Molecular orbitals")) {
          tc = wxT("MOs");
          wc = dc.GetTextExtent(tc).x;
        }
        if (haveR) {
          if (xr < xCentre + wc + gap) xr = xCentre + wc + gap;
          if (xr + wr > usable - 4) xr = usable - 4 - wr;
        }
        if (haveL && xl + wl + gap > xCentre) xl = xCentre - gap - wl;
        if (haveL && xl < 2) xl = 2;
        if (haveL) dc.DrawText(tl, xl, top - 38);
        if (haveR) dc.DrawText(tr, xr, top - 38);
        //  Not over the banner of a diagram that could not be drawn.
        if (p_headline.empty() || p_haveFragments) dc.DrawText(tc, xCentre, top - 38);
        if (model) {
          dc.DrawText(wxString::FromUTF8(p_calcHeader.c_str()), xCalc, top - 38);
          dc.SetPen(wxPen(wxColour(210, 210, 210), 1));
          dc.DrawLine(xCalc - 12, top - 40, xCalc - 12, plot.y + plot.height);
        }
      }

      for (int c = 0; c < 2; c++) {
        const MoColumn& col = c == 0 ? p_left : p_right;
        vector< std::pair<size_t,size_t> > groups;
        vector<int> gy;
        columnGeometry(dc, col, plot, true, groups, gy);
        p_fragRows[c].clear();
        p_fragSketch[c].clear();
        for (size_t g = 0; g < groups.size(); g++) {
          bool o = false, sk = false;
          for (size_t i = groups[g].first; i <= groups[g].second; i++) {
            o = o || hasArrows(col.levels[i].occupancy, col.levels[i].occupancyBeta);
            sk = sk || !col.levels[i].phases.empty();
          }
          p_fragRows[c].push_back(std::make_pair(gy[g], o));
          p_fragSketch[c].push_back(sk ? sketchHalfHeight(col) : 0);
        }
      }

      //  Correlation lines first, so the levels sit on top of them.
      if (p_haveFragments) {
        for (size_t i = 0; i < p_links.size(); i++) {
          const MoConnection& link = p_links[i];
          if (link.centreLevel < 0 ||
              link.centreLevel >= (int)p_centre.levels.size()) continue;

          //  The line takes its molecular level's colour, so a
          //  bonding orbital and its antibonding partner can be
          //  followed back to the same pair of fragment orbitals.
          const MoLevel& reached = p_centre.levels[link.centreLevel];
          wxColour tint = levelColour(reached);
          if (reached.pairing < 0) tint = wxColour(150, 150, 150);
          else tint = wxColour((tint.Red()   + 255)/2,
                               (tint.Green() + 255)/2,
                               (tint.Blue()  + 255)/2);
          //  Weighted lines grow from a 3 % share to 50 %, in width and
          //  strength, but the weakest must still show on an ordinary LCD
          //  seen at an angle (#574): never thinner than 2 px nor paler
          //  than 60 % of the full tint.
          int penWidth = 2;
          if (link.weight >= 0.0) {
            const double t = max(0.0, min(1.0, (link.weight - 0.03)/0.47));
            penWidth = 2 + (int)floor(t + 0.5);
            const double keep = 0.6 + 0.4*t;
            tint = wxColour((int)(255 - keep*(255 - tint.Red())),
                            (int)(255 - keep*(255 - tint.Green())),
                            (int)(255 - keep*(255 - tint.Blue())));
          }
          dc.SetPen(wxPen(tint, penWidth, wxPENSTYLE_SHORT_DASH));

          //  WHERE THE LEVEL IS DRAWN, not where its energy puts it.
          //
          //  The centre column is drawn with spreading on, so a
          //  degenerate or near-degenerate set is nudged apart to stay
          //  legible -- and this aimed every correlation line at the
          //  un-nudged position instead, so the lines stopped short of
          //  the levels they belong to. The fragment ends below were
          //  always spread-aware; only the molecular end was not.
          const int cy = (model && (size_t)link.centreLevel < p_cLanes.size())
              ? p_cLanes[link.centreLevel].y
              : levelYIn(dc, p_centre, plot, true, link.centreLevel);

          //  In lanes the connector meets the level's own lines.
          int cxL = xCentre, cxR = xCentre + centreWidth;
          if (model && (size_t)link.centreLevel < p_cLanes.size()) {
            const CentreLane& c = p_cLanes[link.centreLevel];
            const int count = max(1, p_centre.levels[link.centreLevel].degeneracy);
            (void)count;
            cxL = xCentre + c.linkL;
            cxR = xCentre + c.linkR;
          }
          if (link.leftLevel >= 0 && link.leftLevel < (int)p_left.levels.size()) {
            dc.DrawLine(xLeft + leftWidth,
                        levelYIn(dc, p_left, plot, true, link.leftLevel),
                        cxL, cy);
          }
          if (link.rightLevel >= 0 &&
              link.rightLevel < (int)p_right.levels.size()) {
            dc.DrawLine(cxR, cy,
                        xRight,
                        levelYIn(dc, p_right, plot, true, link.rightLevel));
          }
        }
      }

      p_placed.clear();
      p_notes.clear();
      if (p_haveFragments) {
        //  WITH their electrons.  A fragment level carries an
        //  occupancy now, and a diagram that does not draw it does not
        //  say which combinations are filled -- which is half of what
        //  it is for.  The offline renderer draws them; this did not.
        //  The model-first diagram counts its ligand-internal levels in
        //  the note; nothing is drawn over levels there.
        if (!model) {
          drawBands(dc, p_left, xLeft, leftWidth, plot);
          drawBands(dc, p_right, xRight, rightWidth, plot);
        }
        drawColumn(dc, p_left, xLeft, leftWidth, plot, true, 0, -1, true);
        p_placed.clear();
      p_notes.clear();
        drawColumn(dc, p_right, xRight, rightWidth, plot, true, 0, +1, true);
        p_placed.clear();
      p_notes.clear();
      }

      p_hits.clear();
      drawBands(dc, p_centre, xCentre, centreWidth, plot);
      //  SPREAD THE MOLECULAR ORBITALS TOO.
      //
      //  The fragment columns have always been drawn with a minimum
      //  gap between rows; the centre column was not, so two levels a
      //  few thousandths of a Hartree apart drew their lines, their
      //  labels, their energies and their electron arrows on top of
      //  each other.  NO2- has 1b1 at -0.2020 and 3b2 at -0.2065 and
      //  they came out as one unreadable smear.
      //
      //  This does move a line off its exact height on the axis, by a
      //  few pixels, which the fragment columns can afford more easily
      //  than this one can.  It is the lesser fault: the energy is
      //  printed beside every level, so the number is still exact,
      //  whereas two overprinted labels are not readable at all.  A
      //  qualitative diagram is what this is for.
      if (model) drawCentreLanes(dc, xCentre, plot, &p_hits);
      else drawColumn(dc, p_centre, xCentre, centreWidth, plot, true, &p_hits,
                      -1, true);

      //  THE CALCULATION: its own column, axis and scale (see layoutCalc).
      if (model) drawCalc(dc, plot, calcItems, calcAxisX);

      //  The stack, from the bottom of the window upward: the reasons
      //  of a refused diagram, the click hint, what was clicked.
      int line = size.y - 4;
      dc.SetFont(*wxSMALL_FONT);
      dc.SetTextForeground(wxColour(110, 110, 110));

      const wxColour warn(170, 80, 0);
      for (size_t i = warnLines.size(); i > 0; i--) {
        line -= lineHeight;
        dc.SetTextForeground(warn);
        dc.DrawText(warnLines[i-1], 8, line);
      }
      dc.SetTextForeground(wxColour(110, 110, 110));

      if (p_summary.empty()) {
        for (size_t i = noteLines.size(); i > 0; i--) {
          line -= lineHeight;
          dc.DrawText(noteLines[i-1], 8, line);
        }
      }

      if (hint) {
        line -= lineHeight;
        dc.DrawText(hintSentence, 8, line);
      }

      //  What the last clicked level is, in the diagram's own words.
      dc.SetTextForeground(wxColour(30, 30, 30));
      for (size_t i = infoLines.size(); i > 0; i--) {
        line -= lineHeight;
        dc.DrawText(infoLines[i-1], 8, line);
      }
      //  The folded core, said out loud rather than silently dropped:
      //  an absence the reader cannot see is worse than a cluttered
      //  diagram.
      if (p_centre.hiddenCount > 0 || p_centre.hiddenSemicoreCount > 0) {
        line -= lineHeight;
        ostringstream text;
        //  Semicore counted under its own name rather than folded
        //  silently into "core" -- it is hidden for the same reason
        //  (it takes no part in bonding) but is a different chemical
        //  claim, and a reader comparing against a textbook diagram
        //  needs to be able to tell the two apart (#175).
        if (p_centre.hiddenCount > 0 && p_centre.hiddenSemicoreCount > 0) {
          text << p_centre.hiddenCount << " core and "
               << p_centre.hiddenSemicoreCount << " semicore orbitals below "
               << p_centre.hiddenMaxEnergy << " Hartree, not shown";
        } else if (p_centre.hiddenSemicoreCount > 0) {
          text << p_centre.hiddenSemicoreCount << " semicore orbital"
               << (p_centre.hiddenSemicoreCount == 1 ? "" : "s") << " below "
               << p_centre.hiddenMaxEnergy << " Hartree, not shown";
        } else {
          text << p_centre.hiddenCount << " core orbital"
               << (p_centre.hiddenCount == 1 ? "" : "s") << " below "
               << p_centre.hiddenMaxEnergy << " Hartree, not shown";
        }
        dc.DrawText(wxString(text.str().c_str(), wxConvUTF8),
                    xCentre - 30, line);

        line -= lineHeight;
        dc.SetPen(wxPen(wxColour(150, 150, 150), 1, wxPENSTYLE_DOT));
        dc.DrawLine(xCentre - 30, line + lineHeight/2,
                    xCentre + centreWidth + 30, line + lineHeight/2);
      }
    }

    /** Text with <sub>..</sub> as pieces. */
    static void splitMarkup(const string& text, vector<Piece>& out)
    {
      out.clear();
      size_t i = 0;
      while (i < text.size()) {
        const bool sub = text.compare(i, 5, "<sub>") == 0;
        size_t end;
        string run;
        if (sub) {
          end = text.find("</sub>", i + 5);
          if (end == string::npos) end = text.size();
          run = text.substr(i + 5, end - i - 5);
          i = std::min(text.size(), end + 6);
        } else {
          end = text.find("<sub>", i);
          if (end == string::npos) end = text.size();
          run = text.substr(i, end - i);
          i = end;
        }
        Piece p;
        p.text = wxString::FromUTF8(run.c_str());
        p.kind = sub ? 1 : 0;
        out.push_back(p);
      }
    }

    /** The summary broken into lines at its " . " separators. */
    vector< vector<Piece> > layoutSummary(wxDC& dc, int width)
    {
      vector< vector<Piece> > lines;
      if (p_summary.empty()) return lines;
      wxFont base = *wxSMALL_FONT;
      wxFont sub = base;
      sub.SetPointSize(max(5, base.GetPointSize() - 2));
      const string sep = " \xC2\xB7 ";
      vector<string> parts;
      size_t at = 0;
      for (;;) {
        const size_t k = p_summary.find(sep, at);
        if (k == string::npos) { parts.push_back(p_summary.substr(at)); break; }
        parts.push_back(p_summary.substr(at, k - at));
        at = k + sep.size();
      }
      string current;
      for (size_t i = 0; i < parts.size(); i++) {
        const string candidate = current.empty() ? parts[i]
                                                 : current + sep + parts[i];
        vector<Piece> pieces;
        splitMarkup(candidate, pieces);
        if (!current.empty() &&
            measurePieces(dc, pieces, base, sub).x > width) {
          splitMarkup(current, pieces);
          lines.push_back(pieces);
          current = parts[i];
        } else {
          current = candidate;
        }
      }
      vector<Piece> pieces;
      splitMarkup(current, pieces);
      lines.push_back(pieces);
      return lines;
    }

    /** The note, broken into lines that fit. */
    vector<wxString> wrapNote(wxDC& dc, int width)
    {
      return wrapText(dc, p_note, width);
    }

    vector<wxString> wrapText(wxDC& dc, const string& text, int width)
    {
      vector<wxString> lines;
      if (text.empty()) return lines;

      //  Explicit line breaks are kept.
      const string::size_type nl = text.find('\n');
      if (nl != string::npos) {
        lines = wrapText(dc, text.substr(0, nl), width);
        const vector<wxString> rest = wrapText(dc, text.substr(nl + 1), width);
        lines.insert(lines.end(), rest.begin(), rest.end());
        return lines;
      }

      wxString line, word;
      const wxString all(text.c_str(), wxConvUTF8);
      for (size_t i = 0; i <= all.length(); i++) {
        const wxUniChar c = (i < all.length()) ? all[i]
                                               : wxUniChar(wxT(' '));
        if (c != wxT(' ')) { word += c; continue; }
        if (word.IsEmpty()) continue;

        const wxString candidate = line.IsEmpty() ? word
                                                  : line + wxT(" ") + word;
        if (dc.GetTextExtent(candidate).x > width && !line.IsEmpty()) {
          lines.push_back(line);
          line = word;
        } else {
          line = candidate;
        }
        word.Clear();
      }
      if (!line.IsEmpty()) lines.push_back(line);
      return lines;
    }


    /** Which level, if any, is under a point. */
    int orbitalAt(const wxPoint& at) const
    {
      for (size_t i = 0; i < p_hits.size(); i++) {
        if (p_hits[i].box.Contains(at)) return p_hits[i].orbital;
      }
      return -1;
    }

    int levelAt(const wxPoint& at) const
    {
      for (size_t i = 0; i < p_hits.size(); i++) {
        if (p_hits[i].box.Contains(at)) return p_hits[i].level;
      }
      return -1;
    }

  public:
    /**
     * What a molecular level is: MO number(s), energy, irrep, occupancy,
     * then the owner's composition.  Cached, since the composition may
     * need the overlap integrals.
     */
    /** The text of the last clicked level, shown under the diagram. */
    /** Hover key of calculation-column level n (matched levels, then the unmatched). */
    int calcKey(int n) const
    {
      const int nl = (int)p_centre.levels.size();
      return n < nl ? (CALC_BIT | n) : (ONLY_BIT | (n - nl));
    }
    const string& infoText() const { return p_info; }

    string describeLevel(int key)
    {
      const bool only = (key & ONLY_BIT) != 0;
      const bool calc = only || (key & CALC_BIT) != 0;
      const int level = key & ~(CALC_BIT | ONLY_BIT);
      if (level < 0 || level >= (int)(only ? p_centre.calcOnly.size() : p_centre.levels.size())) return "";
      std::map<int,string>::const_iterator hit = p_tips.find(key);
      if (hit != p_tips.end()) return hit->second;

      const MoLevel& l = only ? p_centre.calcOnly[level] : p_centre.levels[level];
      ostringstream out;
      //  A model level shows the model's own composition (fragment
      //  orbitals) and the calculated orbitals it matches.
      if (l.model && !calc) {
        if (!l.tag.empty()) out << l.tag << "\n";
        out << l.label << ": " << l.detail;
        p_tips[key] = out.str();
        return p_tips[key];
      }
      if (l.orbitals.size() > 1) {
        out << "MOs ";
        for (size_t k = 0; k < l.orbitals.size(); k++)
          out << (k ? ", " : "") << l.orbitals[k] + 1;
      } else if (!l.orbitals.empty()) {
        out << "MO " << l.orbitals[0] + 1;
      }
      char e[64];
      if (calc) snprintf(e, sizeof e, "%.2f", l.calcEnergy*HARTREE_EV);
      else snprintf(e, sizeof e, "%.4f", l.energy);
      out << ", E = " << e << (calc ? " eV" : " Hartree");
      if (calc && !l.calcLabel.empty()) out << ", " << l.calcLabel;
      else if (!l.irrep.empty()) out << ", " << l.irrep;
      const double oa = calc ? l.calcOccupancy : l.occupancy;
      const double ob = calc ? l.calcOccupancyBeta : l.occupancyBeta;
      if (ob >= 0.0) out << ", occupancy " << oa << " alpha, " << ob << " beta";
      else out << ", occupancy " << oa;
      if (p_owner != 0 && !l.orbitals.empty()) {
        const string comp = p_owner->orbitalComposition(l.orbitals);
        if (!comp.empty()) out << "\n" << comp;
      }
      if (calc && !only) {
        char o[32];
        snprintf(o, sizeof o, "%.2f", l.calcOverlap);
        out << "\nThis level matches model " << l.label << ", overlap " << o << ".";
      } else if (only) {
        out << "\n" << l.detail;
      } else if (!l.detail.empty()) {
        out << "\n" << l.detail;
      }
      p_tips[key] = out.str();
      return p_tips[key];
    }

  private:

    void onClick(wxMouseEvent& event)
    {
      event.Skip();
      const int orbital = orbitalAt(event.GetPosition());
      if (orbital >= 0 && p_owner != 0) {
        p_info = describeLevel(levelAt(event.GetPosition()));
        Refresh();
        p_owner->orbitalClicked(orbital);
      }
    }

    /** A hand cursor over a level, so the levels look clickable. */
    void onMotion(wxMouseEvent& event)
    {
      event.Skip();
      const int level = levelAt(event.GetPosition());
      SetCursor(level >= 0 ? wxCursor(wxCURSOR_HAND) : wxNullCursor);
      if (level != p_tipLevel) {
        p_tipLevel = level;
        if (level < 0 || p_owner == 0) UnsetToolTip();
        else SetToolTip(wxString(describeLevel(level).c_str(), wxConvUTF8));
      }
    }

    MoDiagramClickTarget *p_owner;
    MoColumn p_left, p_centre, p_right;
    vector<MoConnection> p_links;

    /**
     * A reason the diagram is missing, said where it will be read.
     *
     * The explanation used to be a sentence in the footnote under the
     * picture, in small grey text, below a spectrum that looks like a
     * diagram with its sides missing. Nobody reads a footnote to find
     * out why the thing they asked for is not there -- reported as
     * "the MO diagram for water is broken" when the calculation had
     * simply been run with symmetry switched off, which the footnote
     * said in its second sentence.
     */
    string p_headline;
    bool p_haveFragments;
    bool p_showEnergies;
    bool p_showDescription;
    bool p_showConstruct = true, p_showSymmetry = true;
    int  p_cMode = 0;
    string p_group;
    string p_formula;
    string p_calcHeader = "This calculation";

    vector<Hit> p_hits;
    string p_info;                       // the clicked level, in the footer
    std::map<int,string> p_tips;         // level -> text, see describeLevel()
    int p_tipLevel = -1;
    string p_note;
    string p_summary, p_about;
    string p_verdict;
    bool p_verdictOk = true;
    vector<string> p_warnings;
    mutable double p_lo, p_hi;
    mutable double p_stripLo, p_stripHi;   // the levels above the axis break
    mutable int p_stripH = 0, p_breakGap = 0;   // pixels; 0 = no break
    mutable int p_stripRows = 0;
    vector<double> p_kE, p_kF;   // the model's monotone energy map (knots)
    mutable double p_clo = 0, p_chi = 1, p_cstripLo = 1e30, p_cstripHi = -1e30;   // the calculation column's scale
    mutable int p_cstripH = 0;
    //  Every label placed in the column being drawn, as (y, notch), so
    //  a new one can be pushed clear of all of them.
    vector< std::pair<int,int> > p_placed;
    vector<string> p_notes;       // annotations already drawn
    const MoColumn *p_sketchColumn;
    int p_outward;
};

#endif // MODIAGRAMCANVAS_H_

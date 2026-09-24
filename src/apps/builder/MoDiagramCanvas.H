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
      : wxPanel(parent, wxID_ANY), p_owner(0), p_haveFragments(false),
        p_placed(), p_sketchColumn(0), p_outward(-1)
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

    void setDiagram(const MoColumn& left, const MoColumn& centre,
                    const MoColumn& right,
                    const vector<MoConnection>& links,
                    bool haveFragments, const string& note)
    {
      p_left = left;
      p_centre = centre;
      p_right = right;
      p_links = links;
      p_haveFragments = haveFragments;
      p_note = note;
      Refresh();
    }

  private:

    //  Where each centre level was last drawn, so a click can be
    //  turned back into an orbital.  Recorded during the paint rather
    //  than recomputed on the click: the layout depends on the window
    //  size, the folded bands and the heading, and a second copy of
    //  that arithmetic would drift from the one that draws.
    struct Hit { wxRect box; int orbital; };

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

    /** Energy to pixels, with the occupied region given the room. */
    int yFor(double energy, const wxRect& plot) const
    {
      if (p_hi <= p_lo) return plot.y + plot.height/2;
      const double f = (energy - p_lo)/(p_hi - p_lo);
      return plot.y + plot.height - (int)(f*plot.height);
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
    void drawColumn(wxDC& dc, const MoColumn& column, int x, int width,
                    const wxRect& plot, bool withOccupancy,
                    vector<Hit>* hits, int outward = -1)
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

      for (size_t first = 0; first < column.levels.size(); ) {

        //  How many levels share this energy?
        size_t last = first;
        int lines = column.levels[first].degeneracy > 0
                    ? column.levels[first].degeneracy : 1;
        while (last + 1 < column.levels.size() &&
               fabs(column.levels[last+1].energy
                    - column.levels[first].energy) < 1.0e-9) {
          last++;
          lines += column.levels[last].degeneracy > 0
                   ? column.levels[last].degeneracy : 1;
        }

        const int y = yFor(column.levels[first].energy, plot);
        const int slot = (lines > 0) ? (width + lineGap)/lines : width;
        const int lineWidth = max(8, slot - lineGap);

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

          drawLabel(dc, level, groupLeft, count*slot - lineGap, centreY,
                    first == last, plot, colour);

          if (hits != 0) {
            Hit hit;
            hit.box = wxRect(groupLeft - 6, y - 7, count*slot + 12, 15);
            hit.orbital = level.orbitals.empty() ? -1 : level.orbitals[0];
            hits->push_back(hit);
          }
          drawn += count;
        }
        first = last + 1;
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

      const wxString text(level.label.c_str(), wxConvUTF8);
      const wxSize extent = dc.GetTextExtent(text);

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
        int step = 0;
        bool clash = true;
        while (clash && step < 2) {
          clash = false;
          for (size_t k = 0; k < p_placed.size(); k++) {
            if (p_placed[k].second == step &&
                abs(y - p_placed[k].first) < extent.y + 2) {
              clash = true;
              break;
            }
          }
          if (clash) step++;
        }
        p_placed.push_back(std::make_pair(y, step));

        //  Clear of the line by more than a couple of pixels: at
        //  eight it was printing over the left-hand end of the level
        //  it names.
        const int lx = (p_outward < 0)
            ? x - extent.x - 14 - step*(extent.x + 12)
            : x + width + 14 + step*(extent.x + 12);
        dc.DrawText(text, lx, y - extent.y/2);

        drawSketch(dc, level, *p_sketchColumn,
                   (p_outward < 0) ? lx - 30 : lx + extent.x + 30, y);

        if (!level.annotation.empty()) {
          dc.SetTextForeground(wxColour(130, 130, 130));
          const wxString extra(level.annotation.c_str(), wxConvUTF8);
          dc.DrawText(extra,
                      (p_outward < 0)
                          ? lx + extent.x - dc.GetTextExtent(extra).x : lx,
                      y + extent.y/2);
        }
      } else {
        dc.DrawText(text, x + (width - extent.x)/2, y - extent.y - 6);
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
        dc.SetFont(*wxNORMAL_FONT);
        dc.DrawText(wxString(p_note.c_str(), wxConvUTF8), 20, 20);
        return;
      }

      //  Energy range over everything that will be drawn.
      p_lo = p_hi = p_centre.levels[0].energy;
      const MoColumn* cols[3] = { &p_left, &p_centre, &p_right };
      for (int c = 0; c < 3; c++) {
        for (size_t i = 0; i < cols[c]->levels.size(); i++) {
          p_lo = min(p_lo, cols[c]->levels[i].energy);
          p_hi = max(p_hi, cols[c]->levels[i].energy);
        }
      }
      if (p_hi - p_lo < 1e-6) { p_hi = p_lo + 1.0; }
      const double pad = 0.06*(p_hi - p_lo);
      p_lo -= pad;
      p_hi += pad;

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

      int footerLines = (int)noteLines.size();
      if (!p_centre.levels.empty()) footerLines++;          // the hint
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

        const wxString title = wxString(pretty.c_str(), wxConvUTF8);
        dc.DrawText(title, (size.x - dc.GetTextExtent(title).x)/2, 6);
        top += 22;
      }
      const wxRect plot(0, top, size.x,
                        max(40, size.y - top - footerHeight - 6));

      const int levelWidth = p_haveFragments ? 70 : 120;
      const int xLeft   = size.x/6;
      const int xCentre = size.x/2 - levelWidth/2;
      const int xRight  = (5*size.x)/6 - levelWidth;

      //  The energy axis.  A correlation diagram without one is a set
      //  of floating dashes: the reader has to be told which way is up
      //  and roughly how far apart the levels are, and the scale is no
      //  longer arbitrary now that every column is in eV.
      {
        const int ax = 22;
        dc.SetPen(wxPen(wxColour(120, 120, 120), 1));
        dc.DrawLine(ax, plot.y, ax, plot.y + plot.height);
        dc.DrawLine(ax, plot.y, ax - 4, plot.y + 7);
        dc.DrawLine(ax, plot.y, ax + 4, plot.y + 7);

        dc.SetFont(*wxSMALL_FONT);
        dc.SetTextForeground(wxColour(110, 110, 110));

        //  Ticks on round numbers, at whatever spacing keeps them from
        //  colliding, so the spacing follows the spectrum rather than
        //  assuming a range.
        static const double steps[] = { 0.02, 0.05, 0.1, 0.2, 0.5,
                                        1, 2, 5, 10, 20, 50 };
        double step = steps[10];
        for (int i = 0; i < 11; i++) {
          if ((p_hi - p_lo)/steps[i] <= 12) { step = steps[i]; break; }
        }

        const double first = ceil(p_lo/step)*step;
        for (double v = first; v <= p_hi; v += step) {
          const int y = yFor(v, plot);
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

      //  Column headings.
      dc.SetFont(*wxNORMAL_FONT);
      dc.SetTextForeground(wxColour(60, 60, 60));
      if (p_haveFragments) {
        dc.DrawText(wxString(p_left.title.c_str(), wxConvUTF8),
                    xLeft, top - 38);
        dc.DrawText(wxString(p_right.title.c_str(), wxConvUTF8),
                    xRight, top - 38);
      }
      dc.DrawText(wxString(p_centre.title.c_str(), wxConvUTF8),
                  xCentre, top - 38);

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
          if (reached.pairing < 0) tint = wxColour(165, 165, 165);
          else tint = wxColour((tint.Red()   + 2*255)/3,
                               (tint.Green() + 2*255)/3,
                               (tint.Blue()  + 2*255)/3);
          dc.SetPen(wxPen(tint, 1, wxPENSTYLE_SHORT_DASH));

          const int cy = yFor(reached.energy, plot);

          if (link.leftLevel >= 0 && link.leftLevel < (int)p_left.levels.size()) {
            dc.DrawLine(xLeft + levelWidth,
                        yFor(p_left.levels[link.leftLevel].energy, plot),
                        xCentre, cy);
          }
          if (link.rightLevel >= 0 &&
              link.rightLevel < (int)p_right.levels.size()) {
            dc.DrawLine(xCentre + levelWidth, cy,
                        xRight, yFor(p_right.levels[link.rightLevel].energy,
                                     plot));
          }
        }
      }

      p_placed.clear();
      if (p_haveFragments) {
        //  WITH their electrons.  A fragment level carries an
        //  occupancy now, and a diagram that does not draw it does not
        //  say which combinations are filled -- which is half of what
        //  it is for.  The offline renderer draws them; this did not.
        drawColumn(dc, p_left, xLeft, levelWidth, plot, true, 0);
        p_placed.clear();
        drawColumn(dc, p_right, xRight, levelWidth, plot, true, 0, +1);
        p_placed.clear();
      }

      p_hits.clear();
      drawColumn(dc, p_centre, xCentre, levelWidth, plot, true, &p_hits);

      //  The stack, from the bottom of the window upward.
      int line = size.y - 4;
      dc.SetFont(*wxSMALL_FONT);
      dc.SetTextForeground(wxColour(110, 110, 110));

      for (size_t i = noteLines.size(); i > 0; i--) {
        line -= lineHeight;
        dc.DrawText(noteLines[i-1], 8, line);
      }

      if (!p_hits.empty()) {
        line -= lineHeight;
        dc.DrawText(wxT("Click a level to show that orbital"), 8, line);
      }

      //  The folded core, said out loud rather than silently dropped:
      //  an absence the reader cannot see is worse than a cluttered
      //  diagram.
      if (p_centre.hiddenCount > 0) {
        line -= lineHeight;
        ostringstream text;
        text << p_centre.hiddenCount << " core orbital"
             << (p_centre.hiddenCount == 1 ? "" : "s") << " below "
             << p_centre.hiddenMaxEnergy << " Hartree, not shown";
        dc.DrawText(wxString(text.str().c_str(), wxConvUTF8),
                    xCentre - 30, line);

        line -= lineHeight;
        dc.SetPen(wxPen(wxColour(150, 150, 150), 1, wxPENSTYLE_DOT));
        dc.DrawLine(xCentre - 30, line + lineHeight/2,
                    xCentre + levelWidth + 30, line + lineHeight/2);
      }
    }

    /** The note, broken into lines that fit. */
    vector<wxString> wrapNote(wxDC& dc, int width)
    {
      vector<wxString> lines;
      if (p_note.empty()) return lines;

      wxString line, word;
      const wxString all(p_note.c_str(), wxConvUTF8);
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

    void onClick(wxMouseEvent& event)
    {
      event.Skip();
      const int orbital = orbitalAt(event.GetPosition());
      if (orbital >= 0 && p_owner != 0) p_owner->orbitalClicked(orbital);
    }

    /** A hand cursor over a level, so the levels look clickable. */
    void onMotion(wxMouseEvent& event)
    {
      event.Skip();
      SetCursor(orbitalAt(event.GetPosition()) >= 0
                ? wxCursor(wxCURSOR_HAND) : wxNullCursor);
    }

    MoDiagramClickTarget *p_owner;
    MoColumn p_left, p_centre, p_right;
    vector<MoConnection> p_links;
    bool p_haveFragments;
    string p_group;

    vector<Hit> p_hits;
    string p_note;
    mutable double p_lo, p_hi;
    //  Every label placed in the column being drawn, as (y, notch), so
    //  a new one can be pushed clear of all of them.
    vector< std::pair<int,int> > p_placed;
    const MoColumn *p_sketchColumn;
    int p_outward;
};

#endif // MODIAGRAMCANVAS_H_

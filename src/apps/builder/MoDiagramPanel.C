#include <algorithm>
#include <cmath>
#include <ios>
#include <sstream>

#include <wx/dcbuffer.h>
#include <wx/link.h>
#include <wx/sizer.h>

#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"
#include "tdat/MoFragments.H"
#include "tdat/PropTable.H"
#include "tdat/TGBSAngFunc.H"
#include "dsm/TGBSConfig.H"
#include "dsm/ICalculation.H"
#include "dsm/ICalcUtils.H"
#include "dsm/JCode.H"
#include "tdat/CharacterTable.H"
#include "tdat/SymmetryOps.H"
#include "tdat/TAtm.H"

#include "dsm/IPropCalculation.H"

#include "viz/SGContainer.H"
#include "viz/SGFragment.H"
#include "wxviz/WxVizToolFW.H"

#include "MoPanel.H"
#include "MoDiagramPanel.H"

using std::max;
using std::min;
using std::ostringstream;

wxFORCE_LINK_THIS_MODULE(MoDiagramPanel)


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
        p_placed(), p_sketchColumn(0)
    {
      SetBackgroundStyle(wxBG_STYLE_PAINT);   // needed for buffered paint
      SetBackgroundColour(*wxWHITE);
      Bind(wxEVT_PAINT, &MoDiagramCanvas::onPaint, this);
      Bind(wxEVT_LEFT_DOWN, &MoDiagramCanvas::onClick, this);
      Bind(wxEVT_MOTION, &MoDiagramCanvas::onMotion, this);
    }

    /** Called with an ORBENG index when a level is clicked. */
    void setClickHandler(MoDiagramPanel *owner) { p_owner = owner; }

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
                    vector<Hit>* hits)
    {
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
          for (int d = 0; d < count; d++) {
            const int lx = x + (drawn + d)*slot;
            dc.DrawLine(lx, y, lx + lineWidth, y);
          }

          if (withOccupancy) {
            drawElectrons(dc, level, x + drawn*slot, slot, lineWidth, y);
          }

          drawLabel(dc, level, groupLeft, count*slot - lineGap, y,
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
        int step = 0;
        bool clash = true;
        while (clash && step < 4) {
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

        const int lx = x - extent.x - 6 - step*(extent.x + 10);
        dc.DrawText(text, lx, y - extent.y/2);

        drawSketch(dc, level, *p_sketchColumn, lx - extent.x/2 - 28, y);

        if (!level.annotation.empty()) {
          dc.SetTextForeground(wxColour(130, 130, 130));
          const wxString extra(level.annotation.c_str(), wxConvUTF8);
          dc.DrawText(extra, lx + extent.x - dc.GetTextExtent(extra).x,
                      y + extent.y/2);
        }
      } else {
        dc.DrawText(text, x + (width - extent.x)/2, y - extent.y - 6);
      }

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
      if (level.phases.empty() ||
          level.phases.size() != column.sketchX.size()) return;

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

    /** Electrons as arrows, one pair of slots per orbital. */
    void drawElectrons(wxDC& dc, const MoLevel& level, int x, int slot,
                       int lineWidth, int y)
    {
      const int count = level.degeneracy > 0 ? level.degeneracy : 1;
      const int perOrbital = (int)(level.occupancy/count + 0.5);
      if (perOrbital <= 0) return;

      dc.SetPen(wxPen(*wxBLUE, 2));
      for (int d = 0; d < count; d++) {
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

      const wxSize size = GetClientSize();
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
      const int bottom = p_centre.hiddenCount > 0 ? 58 : 26;
      const wxRect plot(0, top, size.x, max(40, size.y - top - bottom));

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
          wxString text = wxString::Format(wxT("%g"), v);
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
        drawColumn(dc, p_left, xLeft, levelWidth, plot, false, 0);
        p_placed.clear();
        drawColumn(dc, p_right, xRight, levelWidth, plot, false, 0);
        p_placed.clear();
      }

      p_hits.clear();
      drawColumn(dc, p_centre, xCentre, levelWidth, plot, true, &p_hits);

      //  The folded core, said out loud rather than silently dropped.
      if (p_centre.hiddenCount > 0) {
        const int y = size.y - 34;
        dc.SetPen(wxPen(wxColour(150, 150, 150), 1, wxPENSTYLE_DOT));
        dc.DrawLine(xCentre - 30, y, xCentre + levelWidth + 30, y);
        dc.SetFont(*wxSMALL_FONT);
        dc.SetTextForeground(wxColour(110, 110, 110));
        ostringstream note;
        note << p_centre.hiddenCount << " core orbital"
             << (p_centre.hiddenCount == 1 ? "" : "s") << " below "
             << p_centre.hiddenMaxEnergy << " Hartree, not shown";
        dc.DrawText(wxString(note.str().c_str(), wxConvUTF8),
                    xCentre - 30, y + 4);
      }

      if (!p_hits.empty()) {
        dc.SetFont(*wxSMALL_FONT);
        dc.SetTextForeground(wxColour(110, 110, 110));
        dc.DrawText(wxT("Click a level to show that orbital"),
                    8, size.y - 30);
      }

      if (!p_note.empty()) {
        //  WRAPPED, because it is a paragraph and the window is not as
        //  wide as it is.  Drawn as one line, most of it was simply off
        //  the right-hand edge -- including the part that says why the
        //  fragment columns are missing, which is the only place that
        //  is explained.
        dc.SetFont(*wxSMALL_FONT);
        dc.SetTextForeground(wxColour(110, 110, 110));

        vector<wxString> lines;
        wxString line, word;
        const wxString all(p_note.c_str(), wxConvUTF8);
        for (size_t i = 0; i <= all.length(); i++) {
          const wxUniChar c = (i < all.length()) ? all[i]
                                                 : wxUniChar(wxT(' '));
          if (c != wxT(' ')) { word += c; continue; }
          if (word.IsEmpty()) continue;

          const wxString candidate = line.IsEmpty() ? word
                                                    : line + wxT(" ") + word;
          if (dc.GetTextExtent(candidate).x > size.x - 16 && !line.IsEmpty()) {
            lines.push_back(line);
            line = word;
          } else {
            line = candidate;
          }
          word.Clear();
        }
        if (!line.IsEmpty()) lines.push_back(line);

        const int lineHeight = dc.GetTextExtent(wxT("Xg")).y + 1;
        for (size_t i = 0; i < lines.size(); i++) {
          dc.DrawText(lines[i], 8,
                      size.y - 4 - (int)(lines.size() - i)*lineHeight);
        }
      }
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

    MoDiagramPanel *p_owner;
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
};


BEGIN_EVENT_TABLE( MoDiagramPanel, VizPropertyPanel )
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(MoDiagramPanel, VizPropertyPanel)


MoDiagramPanel::MoDiagramPanel()
  : p_canvas(0)
{
}


MoDiagramPanel::MoDiagramPanel(IPropCalculation *calculation,
      wxWindow *parent, wxWindowID id, const wxPoint& pos,
      const wxSize& size, long style, const wxString& name)
  : p_canvas(0)
{
  Create(calculation, parent, id, pos, size, style, name);
}


MoDiagramPanel::~MoDiagramPanel()
{
}


/////////////////////////////////////////////////////////////////////////////
// Description
//   Show a clicked level's orbital in the viewer.
//
//   Handed to the MOs panel rather than reimplemented here: that panel
//   owns the grid, the basis set, the isovalue and the whole compute
//   path, and a second way of putting an orbital on screen would be a
//   second thing to keep working.
//
//   The MOs panel may not be open.  That is not an error -- the diagram
//   is available for a calculation whose orbitals cannot be rendered at
//   all, which is why it claims ORBENG and not MO -- so it says so
//   instead of failing silently.
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Description
//   How many basis functions each atom carries.
//
//   The molecular orbital coefficients are stored atom by atom in this
//   order, so this is what says which coefficients belong to which
//   fragment -- and therefore how much of an orbital sits on each side
//   of the diagram.
//
//   The count is checked against the coefficient table's own width by
//   the caller.  It has to be: a mapping that is off by one atom
//   produces a perfectly plausible population rather than an error,
//   and would quietly connect the wrong levels.
/////////////////////////////////////////////////////////////////////////////
static bool functionsPerAtom(IPropCalculation *expt, SGFragment *sgfrag,
                             vector<int>& counts, vector<int>& shellOf)
{
  counts.clear();
  shellOf.clear();

  ICalculation *escalc = dynamic_cast<ICalculation*>(expt);
  if (escalc == 0 || sgfrag == 0) return false;

  TGBSConfig *config = escalc->gbsConfig();

  //  A semiempirical code writes no basis set; rebuild one from the
  //  Slater exponents it did report, as MoPanel and ComputeMoCmd both
  //  do.  Each fetches the config independently.
  if (config == 0 || config->empty()) {
    TGBSConfig *slater = ICalcUtils::slaterBasisConfig(expt);
    if (slater != 0) { delete config; config = slater; }
  }
  if (config == 0 || config->empty()) { delete config; return false; }

  const JCode *cap = escalc->application();
  TGBSAngFunc *angfunc = (cap == 0) ? 0 : cap->getAngFunc(config->coordsys());
  const bool cartesian =
      (angfunc != 0 && angfunc->basisType() == TGBSAngFunc::Cartesian);
  delete angfunc;

  vector<TAtm*> *atoms = sgfrag->atoms();
  if (atoms == 0) { delete config; return false; }

  bool ok = true;
  for (size_t a = 0; a < atoms->size(); a++) {
    const string symbol = (*atoms)[a]->atomicSymbol();
    int here = 0;

    vector<const TGaussianBasisSet*> list = config->getGBSList(symbol);
    for (size_t g = 0; g < list.size(); g++) {
      const TGaussianBasisSet *gbs = list[g];
      if (gbs == 0) continue;
      const int sets = gbs->num_contracted_sets(symbol.c_str());
      for (int ics = 0; ics < sets; ics++) {
        vector<TGaussianBasisSet::AngularMomentum> types =
            gbs->func_types(symbol.c_str(), ics);
        for (size_t t = 0; t < types.size(); t++) {
          const int l = (int)types[t];
          //  (l+1)(l+2)/2 Cartesian functions in a shell, 2l+1
          //  spherical ones -- six Cartesian d against five spherical.
          const int inShell = cartesian ? ((l+1)*(l+2))/2 : (2*l + 1);
          here += inShell;
          //  The angular momentum of each function, in the order the
          //  coefficients are stored.  A diatomic can be connected by
          //  nothing else: both its atoms carry exactly half of every
          //  orbital, so which SHELL a sigma-g came from is the only
          //  question with an answer.
          for (int f = 0; f < inShell; f++) shellOf.push_back(l);
        }
      }
    }
    if (here == 0) ok = false;
    counts.push_back(here);
  }

  delete atoms;
  delete config;
  return ok && !counts.empty();
}


void MoDiagramPanel::orbitalClicked(int orbengIndex)
{
  IPropCalculation *calc = getCalculation();
  if (calc == 0) return;

  set<PropertyPanel*> panels =
      PropertyPanel::getPanels(calc->getURL().toString(), "MOs");

  for (set<PropertyPanel*>::iterator it = panels.begin();
       it != panels.end(); ++it) {
    MoPanel *mo = dynamic_cast<MoPanel*>(*it);
    if (mo == 0) continue;

    if (mo->showOrbital(orbengIndex)) return;

    getFW().showMessage("That orbital cannot be computed: the MOs panel "
                        "has no basis set or no coefficients for it.",
                        false/*warning*/);
    return;
  }

  getFW().showMessage("Open the MOs panel (Properties menu) to show an "
                      "orbital from the diagram.", false/*warning*/);
}


bool MoDiagramPanel::Create(IPropCalculation *calculation,
      wxWindow *parent, wxWindowID id, const wxPoint& pos,
      const wxSize& size, long style, const wxString& name)
{
  VizPropertyPanel::Create(calculation, parent, id, pos, size, style, name);

  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  p_canvas = new MoDiagramCanvas(this);
  p_canvas->setClickHandler(this);
  sizer->Add(p_canvas, 1, wxEXPAND|wxALL, 2);
  SetSizer(sizer);

  return true;
}


bool MoDiagramPanel::prefersFloating() const
{
  return true;
}


wxSize MoDiagramPanel::preferredFloatingSize() const
{
  return wxSize(860, 720);
}


bool MoDiagramPanel::isRelevant(IPropCalculation *propCalc) const
{
  if (propCalc == 0) return false;
  //  Energies are the one thing without which there is nothing to draw.
  //  Symmetry labels and fragment analysis are both optional: without
  //  them this degrades to a labelled level diagram, which is still
  //  worth having.
  return propCalc->getProperty("ORBENG") != 0;
}


void MoDiagramPanel::initialize()
{
  build();
}


void MoDiagramPanel::refresh()
{
  build();
}


void MoDiagramPanel::build()
{
  if (p_canvas == 0) return;

  IPropCalculation *calc = getCalculation();
  if (calc == 0) return;

  PropVector    *energies = (PropVector*)calc->getProperty("ORBENG");
  PropVector    *occs     = (PropVector*)calc->getProperty("ORBOCC");
  PropVecString *syms     = (PropVecString*)calc->getProperty("ORBSYM");

  MoColumn left, centre, right;
  vector<MoConnection> links;

  if (energies == 0) {
    p_canvas->setDiagram(left, centre, right, links, false,
                         "This calculation has no orbital energies.");
    return;
  }

  vector<double> e, o;
  vector<string> s;
  for (int i = 0; i < energies->rows(); i++) e.push_back(energies->value(i));
  if (occs != 0 && occs->rows() == energies->rows()) {
    for (int i = 0; i < occs->rows(); i++) o.push_back(occs->value(i));
  }
  //  Only used when it lines up.  MoPanel::fillTable() reverses these
  //  vectors independently and indexes them by the same row, which is
  //  how a length mismatch mislabels every row rather than running out
  //  at the end; the same care is worth taking here.
  if (syms != 0 && syms->rows() == energies->rows()) {
    for (int i = 0; i < syms->rows(); i++) s.push_back(syms->value(i));
  }

  //  --- the molecule's symmetry --------------------------------------
  //
  //  Needed BEFORE the orbitals are grouped, because the grouping uses
  //  the irreps' dimensions.
  //
  //  THE ANALYSIS RUNS ON THE SYMMETRISED GEOMETRY, not the one on
  //  screen.  gensym's operation matrices are written in each group's
  //  standard frame, so atoms only map onto atoms if the molecule is in
  //  that frame.  An optimised structure is neither aligned to it nor
  //  exactly symmetric, and SymmetryOps::find() fixes both: autosym
  //  writes the cleaned, reoriented coordinates back onto the fragment
  //  it was given, which is why it takes a non-const reference.
  //
  //  On a COPY, because that fragment is the one the viewer is
  //  displaying, and nudging every atom of it is not something opening
  //  a diagram should do.  Reading the coordinates back off the copy is
  //  then not an extra step, it is the point.
  //
  //  0.01 Angstrom, which is what the Symmetry panel's own field
  //  defaults to.  The threshold does NOT behave the way it reads: a
  //  LOOSER one finds LOWER symmetry.  Measured against autosym
  //  directly, on exact water and exact methane:
  //
  //      threshold   H2O      CH4
  //      0           C1       C1
  //      0.001       C2v      Td
  //      0.01        C2v      Td
  //      0.05        Cs       Td
  //
  //  0.05 was the first value tried here and it quietly cost water its
  //  C2 axis -- a correlation diagram in Cs instead of C2v, with no
  //  error and no way to tell from the picture.
  string group, why;
  vector<double> coords;
  vector<string> elements;

  WxVizToolFW& fw = getFW();
  SGFragment *sgfrag = fw.getSceneGraph().getFragment();

  if (sgfrag == 0 || sgfrag->numAtoms() == 0) {
    why = "No structure is loaded, so there is no symmetry to use.";
  } else {
    Fragment probe(*sgfrag);
    try {
      group = SymmetryOps::find(probe, 0.01);
    } catch (...) {
      group.clear();
      why = "The symmetry search could not be run.";
    }

    double *xyz = probe.coordinates();
    if (xyz != 0) {
      for (unsigned long a = 0; a < probe.numAtoms(); a++) {
        TAtm *atom = probe.atomRef((int)a);
        if (atom == 0) { elements.clear(); coords.clear(); break; }
        elements.push_back(atom->atomicSymbol());
        for (int k = 0; k < 3; k++) coords.push_back(xyz[a*3 + k]);
      }
    }
  }

  //  --- the molecular orbitals ---------------------------------------
  //
  //  Grouped by irrep DIMENSION where the group is known.  A degenerate
  //  set is degenerate because its irrep is, not because two energies
  //  came out close: methane from a semiempirical run spreads its 1t2
  //  over 2e-4 Hartree, which any tolerance tight enough to separate
  //  real levels splits into a doublet and a singlet.
  map<string,int> dimensions;
  const CharacterTable *table = CharacterTable::lookup(group);
  if (table != 0) {
    const vector<string>& irreps = table->irreps();
    for (size_t i = 0; i < irreps.size(); i++) {
      dimensions[MoDiagram::canonicalIrrep(irreps[i])] =
          table->dimension(irreps[i]);
    }
  }
  MoDiagram::groupByIrrep(e, o, s, dimensions, 1.0e-4, centre.levels);
  centre.title = "Molecular orbitals";

  //  Both ends of the spectrum are folded away, not just the core, and
  //  both cutoffs report what they hid so an absence the reader cannot
  //  see does not pass for a complete diagram.
  MoDiagram::hideBelow(centre, MoDiagram::suggestCoreCutoff(centre.levels));
  MoDiagram::hideAbove(centre,
                       MoDiagram::suggestVirtualCutoff(centre.levels));

  for (size_t i = 0; i < centre.levels.size(); i++) {
    ostringstream text;
    text.setf(std::ios::fixed);
    text.precision(4);
    text << centre.levels[i].energy;
    centre.levels[i].annotation = text.str();
  }

  ostringstream note;
  if (s.empty()) {
    note << "This calculation reports no orbital symmetry labels "
            "(ORBSYM), so the levels are unlabelled and cannot be "
            "correlated with the fragment orbitals.";
  } else {
    note << "Energies in Hartree.";
  }

  //  --- the fragment columns -----------------------------------------
  //
  //  A correlation diagram is a symmetry argument, so these depend on
  //  the geometry and the point group and not on the calculation.  The
  //  molecular orbitals are still drawn when this cannot be made: a
  //  level diagram on its own is useful, and refusing to draw anything
  //  would be worse than saying why the rest is missing.
  bool haveFragments = false;
  if (!elements.empty()) {
    //  The charge, so the fragment levels carry the right electron
    //  count -- NO2- has one more than its atoms bring.
    int charge = 0;
    {
      Fragment *frag = fw.getSceneGraph().getFragment();
      if (frag != 0) charge = (int)frag->charge();
    }
    haveFragments = MoFragments::build(coords, elements, group, charge,
                                       left, right, why);
  }

  if (haveFragments) {
    //  Reconcile the axis conventions BEFORE anything is matched on
    //  the names: in C2v the character table and the code need not
    //  agree on which mirror is sigma-v, and water comes out inside
    //  out if they are compared as they stand.
    string mismatch;
    if (!MoDiagram::reconcile(left.levels, right.levels, centre.levels,
                              mismatch)) {
      haveFragments = false;
      why = mismatch;
    }
    //  classify, then connect, then place: a fragment level's energy
    //  is the mean of the orbitals it connects to, so the connections
    //  have to exist first.
    MoDiagram::classify(left.levels, centre.levels, right.levels);

    //  DO THE TWO SIDES EVEN SPEAK THE SAME LANGUAGE?
    //
    //  The centre labels come from the code, which may have run the job
    //  in a lower group than the structure actually has -- ORCA with no
    //  symmetry reports every orbital as "A", and in C2v there is no
    //  such irrep.  connect() would then find no partner for anything
    //  and the diagram would come out with three columns and not one
    //  line between them, looking like a result.
    int matched = 0;
    for (size_t i = 0; i < centre.levels.size(); i++) {
      for (size_t j = 0; j < left.levels.size(); j++) {
        if (centre.levels[i].irrep == left.levels[j].irrep) { matched++; break; }
      }
    }
    if (matched == 0 && !centre.levels.empty()) {
      note << "  The calculation's orbital labels are not irreps of "
           << group << " -- it was probably run without symmetry, or in "
              "a lower group -- so nothing can be correlated.";
    } else {
      MoDiagram::connect(left.levels, centre.levels, right.levels, links);
      MoDiagram::placeFragments(centre, left, right, links);
      note << "  Molecular levels are the calculation's own orbital "
              "energies in Hartree. Fragment levels are placed by their "
              "valence ionisation energies (shown in eV), in order and "
              "spacing but not on this axis.";
    }
  } else if (!why.empty()) {
    note << "  " << why;
  }

  p_canvas->setGroup(group);
  p_canvas->setDiagram(left, centre, right, links, haveFragments, note.str());
}

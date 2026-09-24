#include <algorithm>
#include <cmath>
#include <sstream>

#include <wx/dcbuffer.h>
#include <wx/link.h>
#include <wx/sizer.h>

#include "tdat/PropVector.H"
#include "tdat/PropVecString.H"
#include "tdat/MoFragments.H"
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
      : wxPanel(parent, wxID_ANY), p_owner(0), p_haveFragments(false)
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

    /** Energy to pixels, with the occupied region given the room. */
    int yFor(double energy, const wxRect& plot) const
    {
      if (p_hi <= p_lo) return plot.y + plot.height/2;
      const double f = (energy - p_lo)/(p_hi - p_lo);
      return plot.y + plot.height - (int)(f*plot.height);
    }

    void drawLevel(wxDC& dc, const MoLevel& level, int x, int width,
                   const wxRect& plot, bool withOccupancy)
    {
      const int y = yFor(level.energy, plot);

      dc.SetPen(wxPen(*wxBLACK, 2));
      dc.DrawLine(x, y, x + width, y);

      //  The label sits left of the line.  The energy is NOT written
      //  beside each level: the axis carries it now, and a number on
      //  every level in a spectrum this dense is clutter rather than
      //  information.
      dc.SetFont(*wxSMALL_FONT);
      dc.SetTextForeground(*wxBLACK);
      if (!level.label.empty() && level.label != "?") {
        wxString text(level.label.c_str(), wxConvUTF8);

        //  Only for the molecular orbitals, whose label is a bare
        //  irrep.  A fragment label already carries its own count
        //  ("2x T2  (2p)"), and appending a second one gives
        //  "2x T2  (2p) (6)".
        if (withOccupancy && level.degeneracy > 1) {
          text += wxString::Format(" (%d)", level.degeneracy);
        }
        const wxSize extent = dc.GetTextExtent(text);
        dc.DrawText(text, x - extent.x - 4, y - extent.y/2);
      }

      if (!withOccupancy) return;

      //  Electrons as arrows, paired where the level holds two.  Drawn
      //  per orbital rather than per level, so a degenerate set shows
      //  its own filling.
      const int perOrbital = (level.degeneracy > 0)
                             ? (int)(level.occupancy/level.degeneracy + 0.5) : 0;
      const int slotWidth = (level.degeneracy > 0)
                            ? width/level.degeneracy : width;
      dc.SetPen(wxPen(*wxBLUE, 2));

      for (int d = 0; d < level.degeneracy; d++) {
        const int cx = x + d*slotWidth + slotWidth/2;
        for (int e = 0; e < perOrbital && e < 2; e++) {
          const int ax = cx + (e == 0 ? -4 : 4);
          const int dir = (e == 0) ? -1 : 1;      // up then down
          dc.DrawLine(ax, y - 9, ax, y + 9);
          dc.DrawLine(ax, y + dir*9, ax - 3, y + dir*9 - dir*4);
          dc.DrawLine(ax, y + dir*9, ax + 3, y + dir*9 - dir*4);
        }
      }
    }

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
        static const double steps[] = { 1, 2, 5, 10, 20, 50, 100, 200, 500 };
        double step = steps[8];
        for (int i = 0; i < 9; i++) {
          if ((p_hi - p_lo)/steps[i] <= 12) { step = steps[i]; break; }
        }

        const double first = ceil(p_lo/step)*step;
        for (double v = first; v <= p_hi; v += step) {
          const int y = yFor(v, plot);
          dc.DrawLine(ax - 3, y, ax + 3, y);
          wxString text = wxString::Format(wxT("%g"), v);
          dc.DrawText(text, ax + 6, y - dc.GetTextExtent(text).y/2);
        }
        dc.DrawText(wxT("E / eV"), 4, plot.y - 16);
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
        dc.SetPen(wxPen(wxColour(160, 160, 160), 1, wxPENSTYLE_SHORT_DASH));
        for (size_t i = 0; i < p_links.size(); i++) {
          const MoConnection& link = p_links[i];
          if (link.centreLevel < 0 ||
              link.centreLevel >= (int)p_centre.levels.size()) continue;
          const int cy = yFor(p_centre.levels[link.centreLevel].energy, plot);

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

      if (p_haveFragments) {
        for (size_t i = 0; i < p_left.levels.size(); i++) {
          drawLevel(dc, p_left.levels[i], xLeft, levelWidth, plot, false);
        }
        for (size_t i = 0; i < p_right.levels.size(); i++) {
          drawLevel(dc, p_right.levels[i], xRight, levelWidth, plot, false);
        }
      }
      p_hits.clear();
      for (size_t i = 0; i < p_centre.levels.size(); i++) {
        drawLevel(dc, p_centre.levels[i], xCentre, levelWidth, plot, true);

        //  ONE ORBITAL PER LEVEL, even where the level is degenerate.
        //  They are degenerate: their shapes are related by the
        //  group's own operations, so any one of them represents the
        //  set, and offering three near-identical choices would be
        //  clutter rather than information.
        Hit hit;
        hit.box = wxRect(xCentre - 40, yFor(p_centre.levels[i].energy, plot) - 7,
                         levelWidth + 80, 15);
        hit.orbital = p_centre.levels[i].orbitals.empty()
                      ? -1 : p_centre.levels[i].orbitals[0];
        p_hits.push_back(hit);
      }

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
             << (int)(p_centre.hiddenMaxEnergy + 0.5) << " eV, not shown";
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
        dc.SetFont(*wxSMALL_FONT);
        dc.SetTextForeground(wxColour(110, 110, 110));
        dc.DrawText(wxString(p_note.c_str(), wxConvUTF8), 8, size.y - 16);
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

    //  Where each centre level was last drawn, so a click can be
    //  turned back into an orbital.  Recorded during the paint rather
    //  than recomputed on the click: the layout depends on the window
    //  size, the folded bands and the heading, and a second copy of
    //  that arithmetic would drift from the one that draws.
    struct Hit { wxRect box; int orbital; };
    vector<Hit> p_hits;
    string p_note;
    mutable double p_lo, p_hi;
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

  //  EVERY COLUMN IS DRAWN ON ONE ENERGY AXIS, so every column has to
  //  be in one unit.  The calculation reports orbital energies in
  //  Hartree and the fragment table is in eV, and the first version of
  //  this put both on the same axis unconverted -- which silently
  //  stretched the fragment levels over a range 27 times too large and
  //  made the correlation lines meaningless.
  //
  //  eV is the unit to convert TO, not from: a valence orbital
  //  ionisation energy and a Koopmans orbital energy are the same kind
  //  of quantity, so on one eV axis the two columns can honestly be
  //  compared.  (They will not agree closely for a DFT calculation,
  //  whose occupied levels come out too shallow.  That is a real
  //  property of the method and not a fault in the drawing.)
  const double hartreeToEv = 27.211386245988;
  for (size_t i = 0; i < e.size(); i++) e[i] *= hartreeToEv;

  //  A tolerance in eV now, converted from the Hartree one it was
  //  chosen as.  Degenerate partners agree to far better
  //  than this; distinct levels in a valence spectrum are further apart.
  MoDiagram::group(e, o, s, 1.0e-4*hartreeToEv, centre.levels);
  centre.title = "Molecular orbitals";

  //  Both ends of the spectrum are folded away, not just the core.
  //  "We don't need all virtual orbitals, just the bonding and
  //  anti-bonding ones" -- and a def2-SVP calculation on a small
  //  molecule carries four or five times as many virtuals as occupied
  //  orbitals, nearly all of them basis-set artefacts rather than
  //  chemistry.  Both cutoffs are suggested from the spectrum itself
  //  and both report what they hid, so the reader can see that
  //  something was left out.
  MoDiagram::hideBelow(centre, MoDiagram::suggestCoreCutoff(centre.levels));
  MoDiagram::hideAbove(centre,
                       MoDiagram::suggestVirtualCutoff(centre.levels));

  ostringstream note;
  if (s.empty()) {
    note << "This calculation reports no orbital symmetry labels "
            "(ORBSYM), so the levels are unlabelled and cannot be "
            "correlated with the fragment orbitals.";
  } else {
    note << "Energies in eV.";
  }

  //  --- the two fragment columns -----------------------------------
  //
  //  A correlation diagram is a symmetry argument, so these depend on
  //  the geometry and the point group and not on the calculation.  The
  //  molecular orbitals are still drawn when this cannot be made: a
  //  level diagram on its own is useful, and refusing to draw anything
  //  because the molecule has no unique central atom would be worse
  //  than saying so.
  bool haveFragments = false;
  string why;
  string group;

  WxVizToolFW& fw = getFW();
  SGFragment *sgfrag = fw.getSceneGraph().getFragment();

  if (sgfrag == 0 || sgfrag->numAtoms() == 0) {
    why = "No structure is loaded.";
  } else {
    //  THE ANALYSIS RUNS ON THE SYMMETRISED GEOMETRY, not the one on
    //  screen.
    //
    //  gensym's operation matrices are written in each group's
    //  standard frame, so atoms only map onto atoms if the molecule is
    //  in that frame.  An optimised structure is neither aligned to it
    //  nor exactly symmetric, and SymmetryOps::find() fixes both --
    //  autosym writes the cleaned, reoriented coordinates back onto the
    //  fragment it was given, which is the whole reason it takes a
    //  non-const reference.
    //
    //  On a COPY, because that fragment is the one the viewer is
    //  displaying and nudging every atom of it is not something opening
    //  a diagram should do.  Reading the coordinates back off the copy
    //  is then not an extra step, it is the point.
    vector<double> coords;
    vector<string> elements;

    //  0.01 Angstrom, which is what the Symmetry panel's own field
    //  defaults to.  The threshold does NOT behave the way it reads:
    //  a LOOSER one finds LOWER symmetry, not higher.  Measured
    //  against autosym directly, on exact water and exact methane:
    //
    //      threshold   H2O      CH4
    //      0           C1       C1
    //      0.001       C2v      Td
    //      0.01        C2v      Td
    //      0.05        Cs       Td
    //
    //  0.05 was the first value tried here and it quietly cost water
    //  its C2 axis -- a correlation diagram in Cs instead of C2v, with
    //  no error and no way to tell from the picture.
    Fragment probe(*sgfrag);
    try {
      group = SymmetryOps::find(probe, 0.01);
    } catch (...) {
      group.clear();
    }

    double *xyz = probe.coordinates();
    if (xyz != 0) {
      for (unsigned long a = 0; a < probe.numAtoms(); a++) {
        TAtm *atom = probe.atomRef((int)a);
        if (atom == 0) { elements.clear(); break; }
        elements.push_back(atom->atomicSymbol());
        for (int k = 0; k < 3; k++) coords.push_back(xyz[a*3 + k]);
      }
    }

    haveFragments = MoFragments::build(coords, elements, group,
                                       left, right, why);
    if (haveFragments) {
      //  DO THE TWO SIDES EVEN SPEAK THE SAME LANGUAGE?
      //
      //  The centre labels come from the code, which may have run the
      //  job in a lower group than the structure actually has -- ORCA
      //  with no symmetry reports every orbital as "A", and in C2v
      //  there is no such irrep.  connect() would then find no partner
      //  for anything and the diagram would come out with three
      //  columns and not one line between them, looking like a result.
      int matched = 0;
      for (size_t i = 0; i < centre.levels.size(); i++) {
        for (size_t j = 0; j < left.levels.size(); j++) {
          if (centre.levels[i].irrep == left.levels[j].irrep) { matched++; break; }
        }
      }
      if (matched == 0 && !centre.levels.empty()) {
        note << "  The calculation's orbital labels are not irreps of "
             << group << " -- it was probably run without symmetry, or "
                "in a lower group -- so nothing can be correlated.";
      } else {
        MoDiagram::connect(left.levels, centre.levels, right.levels, links);
      }
      note << "  Fragment levels are valence orbital ionisation "
              "energies; molecular levels are the calculation's own "
              "orbital energies. Both in eV.";
    }
  }

  if (!haveFragments && !why.empty()) note << "  " << why;

  p_canvas->setGroup(group);
  p_canvas->setDiagram(left, centre, right, links, haveFragments, note.str());
}


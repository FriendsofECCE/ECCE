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
      : wxPanel(parent, wxID_ANY), p_haveFragments(false)
    {
      SetBackgroundStyle(wxBG_STYLE_PAINT);   // needed for buffered paint
      SetBackgroundColour(*wxWHITE);
      Bind(wxEVT_PAINT, &MoDiagramCanvas::onPaint, this);
    }

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

      //  The label sits left of the line, the energy right of it, so a
      //  crowded diagram stays readable in one direction at least.
      dc.SetFont(*wxSMALL_FONT);
      dc.SetTextForeground(*wxBLACK);
      if (!level.label.empty() && level.label != "?") {
        wxString text(level.label.c_str(), wxConvUTF8);
        if (level.degeneracy > 1) {
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
      for (size_t i = 0; i < p_centre.levels.size(); i++) {
        drawLevel(dc, p_centre.levels[i], xCentre, levelWidth, plot, true);
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
             << p_centre.hiddenMaxEnergy << " Hartree, not shown";
        dc.DrawText(wxString(note.str().c_str(), wxConvUTF8),
                    xCentre - 30, y + 4);
      }

      if (!p_note.empty()) {
        dc.SetFont(*wxSMALL_FONT);
        dc.SetTextForeground(wxColour(110, 110, 110));
        dc.DrawText(wxString(p_note.c_str(), wxConvUTF8), 8, size.y - 16);
      }
    }

    MoColumn p_left, p_centre, p_right;
    vector<MoConnection> p_links;
    bool p_haveFragments;
    string p_group;
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


bool MoDiagramPanel::Create(IPropCalculation *calculation,
      wxWindow *parent, wxWindowID id, const wxPoint& pos,
      const wxSize& size, long style, const wxString& name)
{
  VizPropertyPanel::Create(calculation, parent, id, pos, size, style, name);

  wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
  p_canvas = new MoDiagramCanvas(this);
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

  //  A tolerance in Hartree.  Degenerate partners agree to far better
  //  than this; distinct levels in a valence spectrum are further apart.
  MoDiagram::group(e, o, s, 1.0e-4, centre.levels);
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
    note << "No symmetry labels for this calculation, so the levels are "
            "unlabelled and nothing is correlated.";
  } else {
    note << "Energies in Hartree.";
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
    vector<double> coords;
    vector<string> elements;
    vector<TAtm*> *atoms = sgfrag->atoms();
    double *xyz = sgfrag->coordinates();

    if (atoms != 0 && xyz != 0) {
      for (unsigned long a = 0; a < sgfrag->numAtoms(); a++) {
        elements.push_back((*atoms)[a]->atomicSymbol());
        for (int k = 0; k < 3; k++) coords.push_back(xyz[a*3 + k]);
      }
    }
    delete atoms;

    //  Detected on a COPY.  SymmetryOps::find() writes the group onto
    //  the fragment it is given, and the fragment here is the one the
    //  viewer is displaying.
    group = sgfrag->pointGroup();
    if (group.empty()) {
      try {
        Fragment probe(*sgfrag);
        group = SymmetryOps::find(probe, 0.05);
      } catch (...) {
        group.clear();
      }
    }

    haveFragments = MoFragments::build(coords, elements, group,
                                       left, right, why);
    if (haveFragments) {
      MoDiagram::connect(left.levels, centre.levels, right.levels, links);
      note << "  Fragment orbitals on valence ionisation energies (eV), "
              "molecular orbitals on the calculation's own energies "
              "(Hartree): the two scales are different and only the "
              "ordering is comparable.";
    }
  }

  if (!haveFragments && !why.empty()) note << "  " << why;

  p_canvas->setGroup(group);
  p_canvas->setDiagram(left, centre, right, links, haveFragments, note.str());
}


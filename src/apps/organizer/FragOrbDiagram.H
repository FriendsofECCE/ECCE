/**
 * @file
 *
 * The fragment-orbital interaction diagram of the Fragment orbitals Details
 * window (#201): the first fragment's FO levels on the left, the complex's
 * MO levels in the centre, the other fragments' FO levels on the right, and a
 * line wherever `ecce-fragorb diagram` found a share at or above the
 * threshold.  Which levels and lines exist is the CLI's answer
 * (FragDiagramResult); this file only places them.  As in EnergyProfile.C,
 * the layout is computed first, every label by a collision check against the
 * drawn lines, bars and other labels, and painting only draws it, so tests
 * read the same layout through dump().
 */

#ifndef FRAGORBDIAGRAM_HH
#define FRAGORBDIAGRAM_HH

#include <functional>
#include <string>
#include <vector>

#include <wx/colour.h>
#include <wx/gdicmn.h>
#include <wx/scrolwin.h>

#include "util/FragOrbRun.H"
#include "wxgui/PlotStyle.H"

class wxDC;

struct FragDiagramLayout
{
  struct Seg
  {
    double x1, y1, x2, y2;
    wxColour colour;
    int width = 1;
    bool dashed = false;
    std::string what;
  };
  /// One orbital of a level: a short bar, filled by its occupation.
  struct Bar
  {
    wxRect r;
    wxColour colour;
    double fill = 0;                 ///< 0 empty, 0.5 singly occupied (restricted), 1 full
    bool dashed = false;             ///< level not representative
    int level = -1;
  };
  struct Text
  {
    wxRect r;
    std::string text;
    wxColour colour;
    bool vertical = false;
    bool small = false;
    std::string what;
    int level = -1;
  };
  std::vector<Seg> segs;
  std::vector<Bar> bars;
  std::vector<Text> texts;
  std::vector<std::pair<wxRect, int> > hits;   ///< level id under a rectangle
  wxSize size;
  bool placed = true;                ///< false when a label found no free place
  int channel = 0;

  /** The level id at p, or -1. */
  int levelAt(const wxPoint& p) const;
  /**
   * "size W H", "text x y w h what: text", "seg x1 y1 x2 y2 width what"
   * (bars as segments of their height), then the drawn data:
   * "level ID column fragment energy n1,n2,..", "line FO MO share".
   */
  std::string dump(const FragDiagramResult& d) const;
};

/** Display options; the CLI decides what is drawn. */
struct FragDiagramView
{
  std::string heading;               ///< "Cr | C₆O₆"
  double breakEv = 5.0;              ///< empty stretches longer than this are shortened
};

FragDiagramLayout layoutFragDiagram(wxDC& dc, const FragDiagramResult& d, size_t channel,
                                    const FragDiagramView& v, const wxSize& size,
                                    const PlotPalette& pal);
/** highlight: a level id whose bars and lines are drawn heavier, or -1. */
void paintFragDiagram(wxDC& dc, const FragDiagramLayout& l, const PlotPalette& pal, int highlight = -1);

/** "2t2g" as "2t₂g"; "?" for "" (not determined). */
std::string fragIrrep(const std::string& irrep);

/** A level's composition and numbers as plain text (hover, click, test hook). */
std::string fragDiagramLevelText(const FragDiagramResult& d, size_t channel, int level);

/** The size of an exported diagram before it grows to fit its labels. */
const wxSize FRAG_DIAGRAM_PNG(1100, 820);
/** The layout at the export size on white, and the PNG of it. */
FragDiagramLayout exportFragDiagram(const FragDiagramResult& d, size_t channel,
                                    const FragDiagramView& v);
bool saveFragDiagramPng(const FragDiagramResult& d, size_t channel, const FragDiagramView& v,
                        const std::string& file);

/**
 * The diagram on screen.  It grows taller than the window, and scrolls,
 * when the labels need more room; hovering or clicking a level calls the
 * callback with its id.
 */
class FragDiagramCanvas : public wxScrolledCanvas
{
public:
  explicit FragDiagramCanvas(wxWindow *parent);
  void setDiagram(const FragDiagramResult& d, size_t channel, const FragDiagramView& v);
  void onLevel(std::function<void(int level, bool click)> f) { p_onLevel = f; }
  const FragDiagramLayout& layout() const { return p_layout; }

private:
  void relayout();
  void onPaint(wxPaintEvent&);
  FragDiagramResult p_data;
  size_t p_channel = 0;
  FragDiagramView p_view;
  FragDiagramLayout p_layout;
  int p_hover = -1;
  std::function<void(int, bool)> p_onLevel;
};

#endif

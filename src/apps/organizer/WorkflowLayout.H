/**
 * @file
 *
 * The workflow view's layout of boxes and right-angled links, apart from
 * what the boxes are: the canvas lays out the steps of a Session with it and
 * the recipe set-up panel lays out its preview with the same code (#710).
 */

#ifndef WORKFLOWLAYOUT_HH
#define WORKFLOWLAYOUT_HH

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <string>
#include <utility>
#include <vector>

/// A box: its column, the one box it comes from (-1 for none or several) and
/// every source with the text of the arrow from it.
struct WfLayoutNode {
  int depth = 0;
  int source = -1;
  std::vector<std::pair<int, std::string> > in;
};

/// One link: its elbow, arrow and label, in unscrolled coordinates.
struct WfLayoutLink {
  wxPoint p0, p1, p2, p3;    // p0 source edge, p3 target edge
  wxRect label;
  std::string text;
};

struct WfLayoutResult {
  std::vector<wxRect> boxes;
  std::vector<double> rows;
  std::vector<WfLayoutLink> links;
  int maxX = 0, maxY = 0;     ///< right and bottom edge of the boxes
};

/// Boxes flow left to right or top to bottom, with one slot per incoming
/// arrow in a joined box.  `dc` has the font the labels are drawn in.
WfLayoutResult layoutWorkflow(wxDC& dc, const std::vector<WfLayoutNode>& nodes,
                              bool vertical = false);

/// Labels that touch a box, a line, an arrow or another label.
int layoutOverlaps(const WfLayoutResult& layout);

const int kWfMargin = 24;

#endif

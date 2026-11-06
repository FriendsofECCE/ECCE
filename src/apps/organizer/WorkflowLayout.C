#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <algorithm>

#include "WorkflowLayout.H"

using std::vector;

namespace {
const int kBoxW = 230, kBoxH = 62, kGapX = 170, kGapY = 44;
const int kDrop = 40;      // keep the first elbow clear of the source box
}


WfLayoutResult layoutWorkflow(wxDC &dc, const vector<WfLayoutNode> &nodes,
                              bool vertical)
{
  WfLayoutResult res;
  const size_t n = nodes.size();
  res.boxes.resize(n);
  res.rows.assign(n, -1);

  // Rows by a walk in tree order so siblings stay in creation order.
  double nextRow = 0;
  struct R {
    const vector<WfLayoutNode> &nodes; vector<double> &row; double &next;
    double go(size_t i) {
      vector<double> kr;
      for (size_t k = 0; k < nodes.size(); k++)
        if (nodes[k].source == (int)i) kr.push_back(go(k));
      if (kr.empty()) return row[i] = next++;
      return row[i] = (kr.front() + kr.back()) / 2;
    }
  } r = {nodes, res.rows, nextRow};
  for (size_t i = 0; i < n; i++)
    if (nodes[i].source < 0) r.go(i);

  wxSize th = dc.GetTextExtent("geometry");
  int maxLabel = 0;
  for (size_t i = 0; i < n; i++)
    for (size_t k = 0; k < nodes[i].in.size(); k++)
      maxLabel = std::max(maxLabel, dc.GetTextExtent(
          wxString::FromUTF8(nodes[i].in[k].second.c_str())).x);
  const int gapX = std::max(kGapX, maxLabel + kDrop + 50);

  // One more line than before: the result line under the name (#810).
  const int boxH = kBoxH + th.y;
  vector<int> width(n, kBoxW), height(n, boxH);
  int maxW = kBoxW;
  int maxH = boxH;
  for (size_t i = 0; i < n; i++) {
    if (nodes[i].in.size() > 1) {
      // Vertical arrows need a lane each so their horizontal labels stay clear.
      if (vertical)
        width[i] = std::max(kBoxW, (int)(nodes[i].in.size() + 1) * (maxLabel + 30));
      else
        height[i] = std::max(boxH, (int)(nodes[i].in.size() + 1) * (th.y + 10));
    }
    maxW = std::max(maxW, width[i]);
    maxH = std::max(maxH, height[i]);
  }
  const int left = kWfMargin + (vertical ? std::max(0, maxLabel + 6 - kBoxW / 2) : 0);
  for (size_t i = 0; i < n; i++) {
    res.boxes[i] = wxRect(kWfMargin + nodes[i].depth * (kBoxW + gapX),
                          kWfMargin + (int)(res.rows[i] * (maxH + kGapY)) +
                              (maxH - height[i]) / 2,
                          kBoxW, height[i]);
    if (vertical)
      res.boxes[i] = wxRect(left + (int)(res.rows[i] * (maxW + kGapY)) +
                              (maxW - width[i]) / 2,
                          kWfMargin + nodes[i].depth * (maxH + kGapX),
                          width[i], height[i]);
    res.maxX = std::max(res.maxX, res.boxes[i].GetRight());
    res.maxY = std::max(res.maxY, res.boxes[i].GetBottom());
  }
  for (size_t i = 0; i < n; i++) {
    const vector<std::pair<int, std::string> > &in = nodes[i].in;
    // Top to bottom by where the source is, so the arrows do not cross.
    vector<size_t> order(in.size());
    for (size_t k = 0; k < order.size(); k++) order[k] = k;
    std::stable_sort(order.begin(), order.end(), [&](size_t x, size_t y) {
      return vertical ? res.boxes[in[x].first].x < res.boxes[in[y].first].x
                      : res.boxes[in[x].first].y < res.boxes[in[y].first].y;
    });
    const wxRect &t = res.boxes[i];
    for (size_t slot = 0; slot < order.size(); slot++) {
      const std::pair<int, std::string> &inc = in[order[slot]];
      const wxRect &s = res.boxes[inc.first];
      WfLayoutLink l;
      l.p0 = wxPoint(s.GetRight() + 1, s.y + s.height / 2);
      l.p1 = wxPoint(l.p0.x + kDrop, l.p0.y);
      l.p2 = wxPoint(l.p1.x, order.size() == 1
                                 ? t.y + t.height / 2
                                 : t.y + (int)(slot + 1) * t.height /
                                             (int)(order.size() + 1));
      l.p3 = wxPoint(t.x, l.p2.y);
      l.text = inc.second;
      // Above the segment that enters the target, just before the arrow.
      const int w = dc.GetTextExtent(wxString::FromUTF8(l.text.c_str())).x;
      l.label = wxRect(l.p3.x - 14 - w, l.p3.y - 5 - th.y, w, th.y);
      if (vertical) {
        l.p0 = wxPoint(s.x + s.width / 2, s.GetBottom() + 1);
        l.p1 = wxPoint(l.p0.x, l.p0.y + kDrop);
        l.p2 = wxPoint(order.size() == 1 ? t.x + t.width / 2
                          : t.x + (int)(slot + 1) * t.width / (int)(order.size() + 1),
                       l.p1.y);
        l.p3 = wxPoint(l.p2.x, t.y);
        // Keep text horizontal, beside the segment entering the target.
        l.label = wxRect(l.p3.x - 6 - w, l.p3.y - 14 - th.y, w, th.y);
      }
      res.links.push_back(l);
    }
  }
  return res;
}


int layoutOverlaps(const WfLayoutResult &res)
{
  int n = 0;
  vector<wxRect> lines;
  for (const WfLayoutLink &l : res.links) {
    wxPoint pts[4] = {l.p0, l.p1, l.p2, l.p3};
    for (int k = 0; k < 3; k++) {
      wxRect r(pts[k], pts[k + 1]);
      r.Inflate(2, 2);
      lines.push_back(r);
    }
    lines.push_back(l.p2.x == l.p3.x
        ? wxRect(l.p3.x - 5, l.p3.y - 10, 10, 10)
        : wxRect(l.p3.x - 10, l.p3.y - 5, 10, 10));
  }
  for (size_t i = 0; i < res.links.size(); i++) {
    wxRect lab = res.links[i].label;
    for (const wxRect &b : res.boxes) if (lab.Intersects(b)) n++;
    for (const wxRect &r : lines) if (lab.Intersects(r)) n++;
    for (size_t k = 0; k < res.links.size(); k++)
      if (k != i && lab.Intersects(res.links[k].label)) n++;
  }
  return n;
}

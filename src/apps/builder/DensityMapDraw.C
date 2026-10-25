#include <algorithm>
#include <cmath>

#include <wx/brush.h>
#include <wx/font.h>
#include <wx/pen.h>

#include "DensityMapDraw.H"

using namespace DensityPlane;

namespace {

//  Okabe-Ito blue and vermillion: positive and negative lobes differ in
//  colour AND in line style, so the map survives colour-blindness and a
//  black-and-white printout.
const wxColour POSITIVE(0, 114, 178), NEGATIVE(213, 94, 0);

//  viridis, 0 (dark purple) to 0.85 (green-yellow); its bright yellow end
//  is not drawn, it vanishes on white.
wxColour viridis(double t)
{
  static const double stops[5][3] = {
    { 68, 1, 84 }, { 59, 82, 139 }, { 33, 145, 140 }, { 94, 201, 98 },
    { 253, 231, 37 } };
  t = std::max(0.0, std::min(1.0, t));
  const double x = t * 0.85 * 4.0;
  const int i = std::min(3, (int)x);
  const double f = x - i;
  return wxColour((unsigned char)(stops[i][0] + f*(stops[i+1][0] - stops[i][0])),
                  (unsigned char)(stops[i][1] + f*(stops[i+1][1] - stops[i][1])),
                  (unsigned char)(stops[i][2] + f*(stops[i+1][2] - stops[i][2])));
}

wxString levelText(double v)
{
  return wxString::Format("%g", v);
}

}  // namespace


void paintDensityMap(wxDC& dc, const wxSize& size, bool have,
                     const Map& map, const wxString& message)
{
  dc.SetBackground(*wxWHITE_BRUSH);
  dc.Clear();
  dc.SetTextForeground(*wxBLACK);
  if (!have) {
    dc.DrawLabel(message, wxRect(size), wxALIGN_CENTER);
    return;
  }
  const Map& m = map;
  //  The legend gives way before the plot does in a narrow pane.
  const int left = 62, right = std::max(92, std::min(150, size.x * 3 / 10)),
            top = size.y < 300 ? 26 : 34, bottom = size.y < 300 ? 26 : 62;
  const bool roomy = size.y >= 300;   // axis title and plane text need it
  const double w = m.nu - 1, h = m.nv - 1;       // in steps
  const double avail = std::max(50, size.x - left - right);
  const double availH = std::max(50, size.y - top - bottom);
  const double scale = std::min(avail / (w*m.step), availH / (h*m.step));  // px per A
  const double pw = w*m.step*scale, ph = h*m.step*scale;
  const double ox = left + 0.5*(avail - pw), oy = top + 0.5*(availH - ph);
  auto X = [&](double u) { return ox + (u - m.u0)*scale; };
  auto Y = [&](double v) { return oy + ph - (v - m.v0)*scale; };

  wxFont base = dc.GetFont();
  wxFont bold = base; bold.SetWeight(wxFONTWEIGHT_BOLD);
  dc.SetFont(bold);
  dc.DrawText(wxString::FromUTF8(m.title.c_str()), left, 8);
  dc.SetFont(base);

  //  Frame and Angstrom axes.
  dc.SetPen(wxPen(wxColour(120, 120, 120), 1));
  dc.SetBrush(*wxTRANSPARENT_BRUSH);
  dc.DrawRectangle(wxRound(ox), wxRound(oy), wxRound(pw), wxRound(ph));
  const double uEnd = m.u(m.nu - 1), vEnd = m.v(m.nv - 1);
  for (int t = (int)ceil(m.u0); t <= (int)floor(uEnd); t++) {
    const int x = wxRound(X(t));
    dc.DrawLine(x, wxRound(oy + ph), x, wxRound(oy + ph) + 4);
    dc.DrawLabel(wxString::Format("%d", t), wxRect(x - 15, wxRound(oy + ph) + 5, 30, 14), wxALIGN_CENTER);
  }
  for (int t = (int)ceil(m.v0); t <= (int)floor(vEnd); t++) {
    const int y = wxRound(Y(t));
    dc.DrawLine(wxRound(ox) - 4, y, wxRound(ox), y);
    dc.DrawLabel(wxString::Format("%d", t), wxRect(wxRound(ox) - 30, y - 7, 24, 14), wxALIGN_RIGHT);
  }
  if (roomy) dc.DrawLabel(wxString::FromUTF8("distance in the plane (\xC3\x85)"),
               wxRect(wxRound(ox), wxRound(oy + ph) + 22, wxRound(pw), 16),
               wxALIGN_CENTER);

  //  Contours, clipped to the frame.
  dc.SetClippingRegion(wxRound(ox), wxRound(oy), wxRound(pw) + 1, wxRound(ph) + 1);
  struct Line { double value; wxColour colour; wxPenStyle style; };
  vector<Line> legend;
  bool scaled = false;
  const vector<double> lv = mapLevels(m, scaled);
  if (m.quantity != ORBITAL) {
    for (size_t k = 0; k < lv.size(); k++) {
      Line l = { lv[k], viridis(lv.size() > 1 ? double(k)/(lv.size()-1) : 0.0), wxPENSTYLE_SOLID };
      legend.push_back(l);
    }
  } else {
    for (size_t k = lv.size(); k-- > 0;) {
      Line l = { lv[k], POSITIVE, wxPENSTYLE_SOLID };
      legend.push_back(l);
    }
    for (size_t k = 0; k < lv.size(); k++) {
      Line l = { -lv[k], NEGATIVE, wxPENSTYLE_SHORT_DASH };
      legend.push_back(l);
    }
  }
  for (size_t k = 0; k < legend.size(); k++) {
    dc.SetPen(wxPen(legend[k].colour, 1, legend[k].style));
    const vector<Segment> seg = contour(m, legend[k].value);
    for (size_t s = 0; s < seg.size(); s++)
      dc.DrawLine(wxRound(X(seg[s].u0)), wxRound(Y(seg[s].v0)),
                  wxRound(X(seg[s].u1)), wxRound(Y(seg[s].v1)));
  }

  //  Atoms: filled when in the plane, open when out of it.
  for (size_t a = 0; a < m.atoms.size(); a++) {
    const Mark& k = m.atoms[a];
    const bool inPlane = fabs(k.off) < 0.3;
    //  Off-plane atoms are faint and unlabelled: they only locate the cut.
    dc.SetPen(wxPen(inPlane ? *wxBLACK : wxColour(190, 190, 190), 1));
    dc.SetBrush(inPlane ? wxBrush(*wxBLACK) : wxBrush(wxColour(235, 235, 235)));
    dc.DrawCircle(wxRound(X(k.u)), wxRound(Y(k.v)), inPlane ? 4 : 3);
    if (inPlane)
      dc.DrawText(wxString::FromUTF8(k.label.c_str()), wxRound(X(k.u)) + 6,
                  wxRound(Y(k.v)) - 16);
  }
  dc.DestroyClippingRegion();

  //  Legend: every drawn level.
  int ly = wxRound(oy);
  const int lx = wxRound(ox + pw) + 16;
  dc.SetBrush(*wxTRANSPARENT_BRUSH);
  dc.SetPen(*wxBLACK_PEN);
  dc.DrawText(m.quantity == DENSITY ? wxString::FromUTF8("e / bohr\xC2\xB3")
                  : m.quantity == ORBITAL_SET ? wxString::FromUTF8("\xCF\x88\xC2\xB2 / bohr\xC2\xB3")
                                     : wxString::FromUTF8("\xCF\x88 / bohr\xE2\x81\xBB\xC2\xB3\xE2\x81\x84\xC2\xB2"),
              lx, ly - 18);
  for (size_t k = 0; k < legend.size(); k++, ly += 17) {
    dc.SetPen(wxPen(legend[k].colour, 2, legend[k].style));
    dc.DrawLine(lx, ly + 8, lx + 26, ly + 8);
    dc.SetTextForeground(*wxBLACK);
    wxString t = levelText(legend[k].value);
    if (m.quantity == ORBITAL && legend[k].value > 0) t = "+" + t;
    dc.DrawText(t, lx + 32, ly);
  }
  if (legend.empty()) {
    dc.DrawText(fabs(m.zmax) + fabs(m.zmin) > 0 ? "no level reached" : "zero in this plane", lx, ly);
    ly += 17;
  }
  dc.SetTextForeground(wxColour(80, 80, 80));
  if (scaled) {
    dc.DrawText("levels scaled", lx, ly);
    dc.DrawText("to the plane's max", lx, ly + 15);
    ly += 33;
  }
  dc.DrawText(wxString::Format("max %.3g", m.zmax), lx, ly + 8);
  if (m.quantity == ORBITAL)
    dc.DrawText(wxString::Format("min %.3g", m.zmin), lx, ly + 25);
  if (roomy)
    dc.DrawText(wxString::FromUTF8(m.plane.description.c_str()), left, size.y - 18);
}

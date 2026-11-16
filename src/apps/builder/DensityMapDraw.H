// The density map's drawing, shared by the Builder panel and the headless
// picture tool so that what is checked is what is shown (#215).
#ifndef DENSITYMAPDRAW__H
#define DENSITYMAPDRAW__H

#include <string>
#include <vector>
#include <wx/dc.h>
#include <wx/string.h>

#include "tdat/DensityPlane.H"

/** Paints `map` (or `message` when there is no map) into dc.  `fill`
 *  colours the plane between the contour lines (#1077). */
void paintDensityMap(wxDC& dc, const wxSize& size, bool have,
                     const DensityPlane::Map& map, const wxString& message,
                     bool fill = false);

/** The fill colour at value z for contour levels lv (ascending, lv[0]
 *  outermost): signed maps two-sign blue/orange, others viridis; white
 *  below lv[0]. */
wxColour densityMapFillColour(const DensityPlane::Map& map,
                              const std::vector<double>& lv, double z);

/** The atoms named in the picture: every one within 1.5 A of the plane. */
std::vector<std::string> densityMapLabelled(const DensityPlane::Map& map);

/** Plane coordinates (A) under pixel (px, py); false outside the plot. */
bool densityMapPointAt(const wxSize& size, const DensityPlane::Map& map,
                       int px, int py, double& u, double& v);

#endif

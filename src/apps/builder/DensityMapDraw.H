// The density map's drawing, shared by the Builder panel and the headless
// picture tool so that what is checked is what is shown (#215).
#ifndef DENSITYMAPDRAW__H
#define DENSITYMAPDRAW__H

#include <string>
#include <vector>
#include <wx/dc.h>
#include <wx/string.h>

#include "tdat/DensityPlane.H"

/** Paints `map` (or `message` when there is no map) into dc. */
void paintDensityMap(wxDC& dc, const wxSize& size, bool have,
                     const DensityPlane::Map& map, const wxString& message);

/** The atoms named in the picture: every one within 1.5 A of the plane. */
std::vector<std::string> densityMapLabelled(const DensityPlane::Map& map);

/** Plane coordinates (A) under pixel (px, py); false outside the plot. */
bool densityMapPointAt(const wxSize& size, const DensityPlane::Map& map,
                       int px, int py, double& u, double& v);

#endif

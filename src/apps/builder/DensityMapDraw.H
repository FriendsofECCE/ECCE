// The density map's drawing, shared by the Builder panel and the headless
// picture tool so that what is checked is what is shown (#215).
#ifndef DENSITYMAPDRAW__H
#define DENSITYMAPDRAW__H

#include <wx/dc.h>
#include <wx/string.h>

#include "tdat/DensityPlane.H"

/** Paints `map` (or `message` when there is no map) into dc. */
void paintDensityMap(wxDC& dc, const wxSize& size, bool have,
                     const DensityPlane::Map& map, const wxString& message);

#endif

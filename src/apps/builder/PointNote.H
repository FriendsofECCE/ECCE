#ifndef POINTNOTE__H
#define POINTNOTE__H

#include <string>

class IPropCalculation;
class wxSizer;
class wxStaticText;
class wxWindow;

/**
 * Orbital-based panels (Density Map, Density Profile, MOs, MO Diagram) read
 * the calculation's orbitals.  Most parsers keep only the last computed
 * point of a scan or IRC; ECCE-QM stores every point's and the panels follow
 * the point selected in the scan or IRC panel.  The text saying which point
 * the orbitals belong to; empty for any other calculation.
 */
std::string orbitalPointNote(IPropCalculation *calc);

/** Puts the note as a line of its own at the top of <sizer>; 0 if there is none. */
wxStaticText *addOrbitalPointNote(wxWindow *parent, wxSizer *sizer,
                                  IPropCalculation *calc);

/**
 * Makes the scan/IRC point <tableIndex> (shown as row <row>, 0-based, of the
 * panel's table) the one whose orbitals the calculation returns, then has
 * the open orbital panels of the calculation re-read them and their notes
 * updated.  Does nothing unless the calculation stores per-point orbitals
 * and the point differs from the current one.
 */
void selectOrbitalPoint(IPropCalculation *calc, int tableIndex, int row);

#endif

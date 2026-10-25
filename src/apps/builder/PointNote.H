#ifndef POINTNOTE__H
#define POINTNOTE__H

#include <string>

class IPropCalculation;
class wxSizer;
class wxStaticText;
class wxWindow;

/**
 * Orbital-based panels (Density Map, Density Profile, MOs, MO Diagram) read
 * the calculation's one set of orbitals, which every parser keeps from the
 * last computed point of a scan or IRC.  The text saying so; empty for any
 * other calculation.
 */
std::string orbitalPointNote(IPropCalculation *calc);

/** Puts the note as a line of its own at the top of <sizer>; 0 if there is none. */
wxStaticText *addOrbitalPointNote(wxWindow *parent, wxSizer *sizer,
                                  IPropCalculation *calc);

#endif

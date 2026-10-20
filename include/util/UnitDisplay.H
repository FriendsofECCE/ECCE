#ifndef UNITDISPLAY_H
#define UNITDISPLAY_H

#include <string>

// How a unit name is shown to the user.  The stored names ("kJoule/Mole")
// are also the keys of the converters, unit families and saved settings,
// so they stay; only what is drawn on screen changes.
std::string unitLabel(const std::string& units);

// A value printed with up to `decimals` decimals and no trailing zeros
// (-385274.0, not -385274.000000), but at least one decimal so a count
// still reads as a measurement.  Hartree energies keep their precision.
std::string valueLabel(double value, int decimals);

#endif

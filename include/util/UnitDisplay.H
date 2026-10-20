#ifndef UNITDISPLAY_H
#define UNITDISPLAY_H

#include <string>

// How a unit name is shown to the user.  The stored names ("kJoule/Mole")
// are also the keys of the converters, unit families and saved settings,
// so they stay; only what is drawn on screen changes.
std::string unitLabel(const std::string& units);

#endif

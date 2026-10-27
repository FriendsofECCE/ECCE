#ifndef ENERGYFORMAT_HH
#define ENERGYFORMAT_HH

#include <string>

/**
 * Energy units and the digits that are shown (#612).  Absolute energies in
 * hartree get 6 decimals; differences get what the methods carry: 1 decimal
 * in kJ/mol and kcal/mol, 2 in eV.  Copy/export use full precision instead.
 */
enum { EU_HARTREE = 0, EU_KJ = 1, EU_KCAL = 2, EU_EV = 3 };

double hartreeTo(double hartree, int unit);
const char *energyUnitName(int unit);

/** An absolute energy in hartree, 6 decimals. */
std::string fmtAbsolute(double hartree);
/** A difference given in hartree, shown in the unit with the agreed digits. */
std::string fmtDifference(double hartree, int unit);

#endif

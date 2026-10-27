#ifndef COMPAREENERGIES_HH
#define COMPAREENERGIES_HH

#include <string>
#include <utility>
#include <vector>

#include "tdat/EnergySums.H"

class PropertyTask;

/**
 * The final energy of a calculation as the Organizer's summary shows it
 * (#352).  No wx: the dialog only gathers the terms of the two sides and
 * shows what sumEnergies() (tdat/EnergySums) makes of them (#573, #612).
 */
struct FinalEnergy
{
   bool found = false;
   double value = 0;
   std::string units;     ///< as stored ("Hartree", "kcal/Mole", "NA")
   bool scanned = false;  ///< last point of a relaxed scan
   bool heat = false;     ///< MOPAC heat of formation, not a total energy
};

/**
 * Calculation names in the order the dialog lists them: alphabetical, case
 * ignored, a run of digits compared as a number (C2 before C10).
 */
bool calcNameLess(const std::string& a, const std::string& b);

/** The energy the summary names the final energy; found=false if none. */
FinalEnergy finalEnergyOf(PropertyTask *task);

/** Hartree for a stored energy; false if the unit is not one we know. */
bool finalEnergyHartree(const FinalEnergy& e, double *hartree);

/**
 * One calculation as a term of a side: energy, thermochemistry, settings and
 * atoms.  state.problem says why there is no energy (cannot be read, none).
 */
EnergyTerm gatherEnergyTerm(const std::string& url, int coeff);

/** The calculations (name, url) directly in a project. */
std::vector<std::pair<std::string, std::string> >
calculationsIn(const std::string& projectUrl);

/** A side as typed in the script hook: "[-][N*]url,[-][N*]url". */
std::vector<std::pair<std::string, int> > parseSide(const std::string& spec);
std::vector<EnergyTerm> gatherSide(const std::string& spec);

#endif

#ifndef COMPAREENERGIES_HH
#define COMPAREENERGIES_HH

#include <string>
#include <utility>
#include <vector>

class PropertyTask;

/**
 * The final energy of a calculation as the Organizer's summary shows it
 * (#352), and the difference of two of them (#573).  No wx: the dialog only
 * gathers the two calculations and shows the result.
 */
struct FinalEnergy
{
   bool found = false;
   double value = 0;
   std::string units;     ///< as stored ("Hartree", "kcal/Mole", "NA")
   bool scanned = false;  ///< last point of a relaxed scan
   bool heat = false;     ///< MOPAC heat of formation, not a total energy
};

/** The energy the summary names the final energy; found=false if none. */
FinalEnergy finalEnergyOf(PropertyTask *task);

struct EnergyEntry
{
   std::string name, url, code, method, basis;
   bool exists = false;
   FinalEnergy energy;
};

/** Reads a calculation by URL; exists=false if it cannot be opened. */
EnergyEntry gatherEnergyEntry(const std::string& url);

/** The calculations (name, url) directly in a project. */
std::vector<std::pair<std::string, std::string> >
calculationsIn(const std::string& projectUrl);

struct EnergyComparison
{
   bool ok = false;            ///< both have a usable energy
   double a = 0, b = 0;        ///< hartree
   double delta = 0;           ///< E(B) - E(A), hartree
   std::vector<std::string> notes;
};

EnergyComparison compareEnergies(const EnergyEntry& a, const EnergyEntry& b);

/** 0 hartree, 1 kJ/mol, 2 kcal/mol, 3 eV. */
double hartreeTo(double hartree, int unit);
const char *energyUnitName(int unit);

/** The energies block and the notes, as the two text areas show them. */
std::string comparisonResults(const EnergyEntry& a, const EnergyEntry& b,
                              const EnergyComparison& c);
std::string comparisonNotes(const EnergyComparison& c);

/** Both in one text, as the hook prints it and the dialog shows it. */
std::string comparisonText(const EnergyEntry& a, const EnergyEntry& b,
                           const EnergyComparison& c);

#endif

#ifndef REACTIONENERGIES__H
#define REACTIONENERGIES__H

#include <string>
#include <vector>

/**
 * Barrier and reaction energy from three calculations: reactant,
 * transition state, product (#419).  No wx; the panel only gathers the
 * RxnState of each calculation and draws the result.
 */
struct RxnState
{
   std::string name;
   /** Non-empty: this calculation gives no energy, and why. */
   std::string problem;
   double energy = 0;          ///< final total energy, hartree
   bool thermo = false;        ///< zpe, enthalpy and gibbs are set
   double zpe = 0;             ///< zero-point correction, hartree
   double enthalpy = 0;        ///< absolute, hartree
   double gibbs = 0;           ///< absolute, hartree
   double temperature = 0;     ///< K, 0 if not printed
   std::string method, basis, solvent, multiplicity;
   int charge = 0;
};

enum { RXN_REACTANT = 0, RXN_TS = 1, RXN_PRODUCT = 2 };

/** One energy quantity for the three states, absolute in hartree. */
struct RxnLevel
{
   std::string label;    ///< "E", "E + ZPE", "H" or "G"
   double value[3] = {0, 0, 0};
   double barrier() const { return value[RXN_TS] - value[RXN_REACTANT]; }
   double reaction() const { return value[RXN_PRODUCT] - value[RXN_REACTANT]; }
};

struct RxnResult
{
   /** Why no energies were made (one per calculation); empty if all fine. */
   std::vector<std::string> problems;
   /** The numbers are shown but the calculations differ in what is listed. */
   std::vector<std::string> warnings;
   /** Notes that are not warnings (e.g. no thermochemistry). */
   std::vector<std::string> notes;
   std::vector<RxnLevel> levels;
   std::string names[3];
   bool ok() const { return problems.empty() && !levels.empty(); }
};

/** states[RXN_REACTANT..RXN_PRODUCT]. */
RxnResult rxnAnalyse(const RxnState states[3]);

double rxnToUnit(double hartree, bool kcal);

/** CSV: '#' header lines, then one row per quantity.  C locale. */
std::string rxnCsv(const RxnResult& r, bool kcal, const std::string& calculation);

#endif

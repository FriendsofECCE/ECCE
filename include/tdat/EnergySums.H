#ifndef ENERGYSUMS_HH
#define ENERGYSUMS_HH

#include <map>
#include <string>
#include <vector>

#include "tdat/ReactionEnergies.H"

/**
 * Tools > Compare Energies (#612): side A and side B are each a sum of
 * calculations with integer coefficients (a negative one is a "-" entry).
 * No wx; the dialog only gathers the terms and shows the result.
 */
struct EnergyTerm
{
   RxnState state;               ///< name, energy, thermochemistry, settings
   std::string code, dispersion;
   std::map<std::string, int> atoms;   ///< element -> count; empty if unknown
   int coeff = 1;                ///< signed stoichiometric coefficient
   bool heat = false;            ///< the energy is a heat of formation (MOPAC)
   bool scanned = false;         ///< the energy is a relaxed scan's last point
};

/** One row of the table: A and B sums in hartree, or why it is not there. */
struct SumRow
{
   std::string label;            ///< "E", "E + ZPE", "H (298 K)", "G (298 K)"
   bool available = false;
   std::string reason;           ///< when not available
   double a = 0, b = 0;          ///< hartree
   double delta() const { return b - a; }
};

struct SumResult
{
   bool ok = false;              ///< every calculation has an energy
   std::vector<SumRow> rows;
   /** The numbers are shown but may not mean what they seem to. */
   std::vector<std::string> warnings;
   /** Notes that are not warnings. */
   std::vector<std::string> notes;
   std::string solventA, solventB;   ///< "none" or the model name(s)
   /** "solvation energy" for a gas phase / solvent pair of one molecule. */
   std::string deltaLabel;
};

SumResult sumEnergies(const std::vector<EnergyTerm>& a,
                      const std::vector<EnergyTerm>& b);

/** The table, the warnings and notes, and both, as shown or (full) for Copy. */
std::string sumTable(const SumResult& r, bool full);
std::string sumNotes(const SumResult& r);
/** Table and notes together. */
std::string sumText(const SumResult& r, bool full);

/** "C6H10" style formula of the signed sum, e.g. for the unbalanced note. */
std::string atomFormula(const std::map<std::string, int>& atoms);

#endif

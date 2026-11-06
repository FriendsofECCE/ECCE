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
   std::string xc;                     ///< density functional as stored, "" if none
   std::map<std::string, int> atoms;   ///< element -> count; empty if unknown
   int coeff = 1;                ///< signed stoichiometric coefficient
   std::string url;              ///< the calculation, for recomputing G (#692)
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
   bool hasSpread = false;       ///< max - min of delta() over the treatments (#692)
   double spread = 0;            ///< hartree
};

/**
 * The 1 M standard-state correction of the G row and log K.  The stored G of
 * the codes is at 1 atm; for a solution-phase reaction with change in the
 * number of species dn, dn*RT*ln(R'T/p0) (R' in L atm/K/mol, p0 = 1 atm)
 * refers the delta G to 1 mol/L.
 */
struct StdState
{
   bool available = false;       ///< the G row has a value
   bool applied = false;         ///< the correction is in the G row
   int dn = 0;                   ///< coefficients of B minus those of A
   double lnFactor = 0;          ///< ln(R'T/p0)
   double correction = 0;        ///< kJ/mol, dn*RT*lnFactor (added when applied)
   double deltaG = 0;            ///< kJ/mol as shown, with the correction if applied
   double logK = 0;              ///< -deltaG/(RT ln 10)
};

/** The correction in kJ/mol for dn and T in K. */
double standardStateCorrection(int dn, double T);
/** log K from delta G (kJ/mol) at T (K). */
double logKFromDeltaG(double deltaG, double T);

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
   double gTemperature = 0;      ///< K, of the G row; 0 if none
   StdState stdState;
};

SumResult sumEnergies(const std::vector<EnergyTerm>& a,
                      const std::vector<EnergyTerm>& b);

/**
 * Fill r.stdState from the G row and, if apply, add the 1 M correction to B
 * of the G row.  Call after the G row is final (the #692 treatments).
 */
void applyStandardState(SumResult& r, const std::vector<EnergyTerm>& a,
                        const std::vector<EnergyTerm>& b, bool apply);

/** The table, the warnings and notes, and both, as shown or (full) for Copy. */
std::string sumTable(const SumResult& r, bool full);
std::string sumNotes(const SumResult& r);
/** Table and notes together. */
std::string sumText(const SumResult& r, bool full);

/** "C6H10" style formula of the signed sum, e.g. for the unbalanced note. */
std::string atomFormula(const std::map<std::string, int>& atoms);

#endif

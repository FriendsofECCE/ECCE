/**
 * @file
 * The structures behind Gaussian's Opt=QST2/QST3 (#419): which calculations
 * of the project can be picked, what their final structure is, and the
 * files handed to the input generator.  No wx here.
 */
#ifndef QSTSTRUCTURES_HH
#define QSTSTRUCTURES_HH

#include <string>
#include <vector>

class ICalculation;
class PropertyTask;
class Fragment;

struct QstEntry
{
   std::string name;
   std::string url;
   std::string kind;   // "optimised" or "input"
};

/** The one definition of "a calculation's final structure": its optimised
 *  geometry when it has one, else its input structure.  false when the
 *  calculation has no structure. */
bool qstFinalStructure(PropertyTask* calc, std::vector<std::string>& elements,
                       std::vector<double>& xyz, bool& optimised);

/** A copy of a calculation's final structure, for a transition-state job to
 *  store as its own.  Caller deletes; 0 when it has none. */
Fragment* qstReactantFragment(const std::string& url);

/** Other calculations in self's project that have a structure. */
std::vector<QstEntry> qstCandidates(ICalculation* self);

/** Writes the list the Runtype Details dialog reads (name, URL, kind per
 *  line) and returns its path, "" on failure. */
std::string qstWriteCandidateFile(const std::vector<QstEntry>& entries);

/** Writes <name>.qst1.frag ... for the reactant, product and optional TS
 *  guess picked in self's runtype settings, each mapped and aligned onto
 *  self's structure, in the order Gaussian wants.  status is the line shown
 *  to the user (one line), detail the long form for a tooltip; warning is non-empty when the structures do not fit
 *  together (never blocking).  Returns false only on a file error. */
bool qstWriteStructureFiles(ICalculation* self, const std::string& baseName,
                            std::string& status, std::string& warning,
                            std::string* detail = 0,
                            std::string* table = 0);

/** ORCA NEB-TS: writes neb_product.xyz (and neb_guess.xyz when picked) in
 *  the working directory, each mapped and aligned onto self's structure,
 *  which is the reactant.  Same status/warning contract as above. */
bool nebWriteStructureFiles(ICalculation* self, std::string& status,
                            std::string& warning, std::string* detail = 0,
                            std::string* table = 0);

/** The same matching as the two functions above, from the reactant, product
 *  and TS guess URLs as picked, with nothing written: the mapping needs only
 *  the structures, not a saved deck or a basis set (#464). */
bool tsMatchStructures(ICalculation* self, bool neb, const std::string picks[3],
                       std::string& status, std::string& warning,
                       std::string* detail = 0, std::string* table = 0,
                       std::string* pathWarning = 0);

/** The level of theory a calculation was set up with (#468). */
struct TsLevel
{
   std::string theory;       // "DFT", "RHF", ...
   std::string functional;   // only for DFT
   std::string basis;
   bool optimised;           // its structure is an optimised one
   TsLevel() : optimised(false) {}
   bool known() const { return !theory.empty() || !basis.empty(); }
   /** "B3LYP/def2-SVP", "HF/6-31G*"; "" when nothing is known. */
   std::string text() const;
};

/** The stored level of the calculation at url; empty when it has none. */
TsLevel tsLevelOfCalculation(const std::string& url);

/** Warning that the end point `role` was made at a different level than
 *  `mine`, "" when the levels agree or one is unknown.  Components missing on
 *  either side are not compared. */
std::string tsLevelWarning(const char* role, const TsLevel& other,
                           const TsLevel& mine);
#endif

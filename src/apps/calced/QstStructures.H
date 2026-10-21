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
                            std::string* detail = 0);

/** ORCA NEB-TS: writes neb_product.xyz (and neb_guess.xyz when picked) in
 *  the working directory, each mapped and aligned onto self's structure,
 *  which is the reactant.  Same status/warning contract as above. */
bool nebWriteStructureFiles(ICalculation* self, std::string& status,
                            std::string& warning, std::string* detail = 0);

#endif

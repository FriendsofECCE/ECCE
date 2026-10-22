#ifndef BONDORDERRULE_HH
#define BONDORDERRULE_HH

#include <string>
#include <utility>
#include <vector>

/**
 * Bond orders as bookkeeping for Force Field Clean, derived from the atoms'
 * elements and neighbour counts only (a Nub or hydrogen counts as a
 * neighbour). No wx or fragment dependency, so a CLI test can drive it.
 *
 *   free valence = usual valence (C 4, N 3, O 2, S 2) - neighbours
 *
 * Bonds are then raised (at most to triple) until every atom's free valence
 * is used up; ring double bonds alternate (Kekule). Anything left over stays
 * single and the atom is reported as unresolved.
 */
namespace BondOrderRule {

struct Result {
  std::vector<double> orders;        // one per input bond: 1, 1.5, 2 or 3
  std::vector<int>    unresolved;    // atoms left with free valence, or
                                     //   hypervalent S/P
  std::vector<std::string> notes;    // "C1=C2 double (both trigonal)", ...
};

//  elements[i] is atom i's symbol ("Nub" for a nub); bonds are covalent
//  bonds only (callers leave metal-pi bonds out of the list).
Result perceive(const std::vector<std::string>& elements,
                const std::vector<std::pair<int,int> >& bonds);

//  Typical length (Angstrom) of a bond of this order between two elements;
//  0 if the pair is not covered (metals, noble gases).
double typicalLength(const std::string& e1, const std::string& e2,
                     double order);

}

#endif

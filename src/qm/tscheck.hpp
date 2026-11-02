// Checks on a transition-state search that ended converged: does the saddle
// found belong to the reaction the search was started for?
#pragma once
#include <string>
#include <vector>
#include "molecule.hpp"
#include "optimizer.hpp"
#include "vibration.hpp"

namespace qm {

// Warnings, empty when the saddle looks right.  start and end have no dummy
// atoms and the same atom order; vib belongs to end (imaginary mode first).
//  * the imaginary mode must move the followed coordinate: ts_follow when
//    given, else the bonds whose length changed most between start and end;
//    "move" = the mode's overlap with the coordinate's unit gradient exceeds
//    min_overlap;
//  * no bond that started shorter than 1.5x the covalent sum (a TS bond is stretched) may end longer
//    than 2x it.
std::vector<std::string> check_saddle(const Molecule& start, const Molecule& end, const VibResult& vib,
                                      const OptOptions& opt, double min_overlap = 0.15);

}  // namespace qm

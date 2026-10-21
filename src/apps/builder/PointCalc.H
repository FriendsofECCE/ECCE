#ifndef POINTCALC__H
#define POINTCALC__H

#include <string>
#include <vector>

class Fragment;
class IPropCalculation;

/**
 * Put the structure of one scan/IRC point (x y z per atom, Angstrom) onto a
 * fragment.  Dummy atoms ("X") are skipped when the structure has no rows for
 * them.  False, with the reason in why, if the atom count fits neither way;
 * the fragment is left unchanged then.
 */
bool applyPointStructure(Fragment *frag, const std::vector<double>& xyz,
                         std::string& why);

/**
 * "New calculation from this point" (#410): a copy of the calculation in
 * its project (as Duplicate in the Organizer makes it: same code, theory,
 * basis and run type) whose structure is frame <frame> of property <key>
 * (SCANGEOM or IRCGEOM), reset to the created state so it can be edited and
 * run.  Named <original>_point_<point>; resource names cannot hold spaces.
 * Returns the new calculation's URL, or "" with the reason in why.
 */
std::string newCalcFromPoint(IPropCalculation *calc, const std::string& key,
                             int frame, int point, std::string& why);

#endif

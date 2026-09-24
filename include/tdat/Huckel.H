/**
 * @file
 *
 * Extended Huckel: molecular orbitals from a structure alone.
 *
 * WHY THIS EXISTS.  A correlation diagram's two fragment columns come
 * from geometry and group theory and need no calculation at all.  Its
 * middle column does -- the molecular orbitals, ordered and to scale.
 * Requiring a real calculation for that means a teaching tool cannot
 * answer "draw me the diagram for ammonia" without a code installed,
 * an input file written and output parsed.
 *
 * Extended Huckel answers it from the structure.  H_ii is the valence
 * orbital potential energy the diagram's own fragment levels are
 * annotated with; H_ij is the Wolfsberg-Helmholz form
 *
 *     H_ij = K S_ij (H_ii + H_jj) / 2,    K = 1.75
 *
 * and the orbitals fall out of the generalised eigenvalue problem
 * H C = S C E.  Nothing else: no self-consistency, no two-electron
 * integrals, no iteration to converge or fail to.
 *
 * WHAT IT IS AND IS NOT.  It gets the ORDERING of orbitals right, and
 * their symmetries exactly, and their energies to a few electron
 * volts.  That is the whole content of a qualitative diagram and it
 * is the wrong tool for anything quantitative.  It must not be used
 * where a real calculation exists -- and the caller, not this code,
 * is what has to know that.
 */

#ifndef HUCKEL_HH
#define HUCKEL_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

class Huckel
{
  public:

    /**
     * Solve a molecule.
     *
     * @param coords     3N, atom-major, Angstrom
     * @param elements   N atomic symbols
     * @param charge     molecular charge, for the electron count
     * @param energies   filled with one orbital energy per basis
     *                   function, Hartree, ascending
     * @param occupancies  filled to match, aufbau with pairing
     * @param coefficients filled with one row per orbital, in the same
     *                   order the basis functions are in
     * @param perAtom    filled with the number of functions on each
     *                   atom
     * @param shellOf    filled with each function's angular momentum,
     *                   in order -- so the caller can hand all of this
     *                   straight to the diagram machinery
     * @param why        set when it declines
     *
     * @return false if any element has no parameters, or if the
     *         eigenproblem does not solve.  It declines rather than
     *         returning something shaped like an answer.
     */
    static bool solve(const vector<double>& coords,
                      const vector<string>& elements,
                      int charge,
                      vector<double>& energies,
                      vector<double>& occupancies,
                      vector< vector<double> >& coefficients,
                      vector<int>& perAtom,
                      vector<int>& shellOf,
                      string& why);

    /** Are there parameters for every element here? */
    static bool canSolve(const vector<string>& elements, string& missing);
};

#endif

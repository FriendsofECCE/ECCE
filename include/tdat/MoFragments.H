/**
 * @file
 *
 * The two outer columns of a qualitative MO correlation diagram: the
 * central atom's valence orbitals on one side, the terminal atoms'
 * symmetry orbitals (TASOs) on the other.
 *
 * Separate from anything that draws, and from the calculation itself:
 * what goes in these columns depends only on the molecule's geometry
 * and its point group, which is the point.  A correlation diagram is a
 * symmetry argument, and it should come out the same whichever code
 * produced the orbitals it is being compared against.
 *
 * Everything here routes through SymmetryAnalysis::basisCharacter() and
 * CharacterTable::reduce(), both of which tests/symmetry already checks
 * against the great orthogonality theorem and against the labels the
 * codes themselves report.  There is deliberately no second
 * implementation of the angular characters living in here.
 */

#ifndef MOFRAGMENTS_HH
#define MOFRAGMENTS_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

#include "tdat/MoDiagram.H"

struct SymOp;


class MoFragments
{
  public:

    /**
     * The operations of a named point group.
     *
     * Shells out to the `symops` helper, which wraps gensym -- the same
     * generator cleansym, genmol and getfrag have always built
     * structures from, so the operations cannot drift from the ones the
     * rest of ECCE uses.
     *
     * @return false if the group is not recognised or the helper could
     *         not be run; the caller should say so rather than draw a
     *         diagram with no symmetry in it.
     */
    static bool symmetryOperations(const string& group, vector<SymOp>& ops);

    /**
     * A valence orbital ionisation energy, in eV, negative.
     *
     * @param l  0 for the valence s, 1 for the valence p
     * @return false if the element is not in the table, which is the
     *         honest answer for a transition metal: these are main
     *         group parameters and extending them to d orbitals needs
     *         a different table, not an extrapolation of this one.
     */
    static bool valenceEnergy(const string& element, int l, double& eV);

    /** How many valence electrons an element brings; 0 if unknown. */
    static int valenceElectrons(const string& element);

    /**
     * Split the atoms into a central atom and its terminal orbit.
     *
     * The central atom is one the whole group leaves in place -- an
     * orbit of size one -- and where several qualify, the heaviest.
     * The terminal atoms are the largest remaining orbit.
     *
     * @return false when the molecule has no such atom (ethane,
     *         benzene, anything with no unique centre), which is not a
     *         failure so much as a molecule this kind of diagram does
     *         not describe.  Say so rather than nominate an arbitrary
     *         atom.
     */
    static bool partition(const vector< vector<int> >& orbits,
                          const vector<string>& elements,
                          int& central,
                          vector<int>& terminal);

    /**
     * Build both outer columns.
     *
     * @param coords     3N, atom-major, Angstrom
     * @param elements   N atomic symbols
     * @param group      the point group name, as SymmetryOps::find()
     *                   returns it
     * @param charge     the molecule's charge, so the fragment levels
     *                   carry the right number of electrons
     * @param left       filled with the central atom's valence orbitals
     * @param right      filled with the terminal atoms' symmetry
     *                   orbitals
     * @param note       filled with why the columns are empty, when
     *                   they are
     *
     * @return false if no symmetry argument could be made, with `note`
     *         explaining which step gave out.
     */
    static bool build(const vector<double>& coords,
                      const vector<string>& elements,
                      const string& group,
                      int charge,
                      MoColumn& left,
                      MoColumn& right,
                      string& note);
};

#endif

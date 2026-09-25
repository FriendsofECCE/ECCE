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
     * Where to draw the terminal atoms when sketching a phase pattern.
     *
     * The atoms' own positions flattened onto the two directions they
     * spread out in most, scaled into -1..1.  Flattened rather than
     * drawn in perspective because the sketch answers "which of these
     * are in phase with which", and a perspective view of four
     * tetrahedral hydrogens hides exactly that.
     */
    static void sketchPositions(const vector<double>& coords,
                                const vector<int>& atoms,
                                vector<double>& x, vector<double>& y,
                                int* normal = 0);

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
     * How many orbitals a minimal valence basis puts on this element.
     *
     * One for hydrogen and helium, four for a main-group element
     * (s and three p), nine for a transition metal (five d as well).
     * This is the size of the picture a qualitative MO diagram draws:
     * a split-valence or larger basis produces further empty levels
     * that are artefacts of the basis and belong to no orbital anyone
     * would put on such a diagram.
     */
    static int valenceOrbitals(const string& element);

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
     * Does the basis span what the code says the orbitals span?
     *
     * THE ONE CHECK THAT DOES NOT NEED TO KNOW THE ANSWER.
     *
     * Every molecular orbital is a combination of the basis functions,
     * so the irreps the whole basis spans and the irreps the code
     * reports for its orbitals are the same multiset.  One is computed
     * here from the geometry and the group; the other is read from the
     * calculation's own output.  Two programs, different data,
     * different routes.
     *
     * When they agree, the symmetry detection, the frame, the class
     * matching, the angular characters and the shell bookkeeping are
     * all right together -- on a molecule nobody wrote an expected
     * answer for.  When they disagree, something is wrong and this
     * says which molecule and how.
     *
     * An exchange of two irreps is allowed and reported, because that
     * is the axis convention and not an error: see
     * MoDiagram::reconcile().
     *
     * @param shellsPerAtom  the angular momentum of each SHELL on each
     *                       atom, in coefficient order
     * @param reported       the code's own orbital labels, one per
     *                       orbital, in any spelling
     * @param detail         filled with both reductions, agreeing or
     *                       not, for a person to read
     *
     * @return false if they disagree, or if the check could not be
     *         made -- which `detail` distinguishes, because "no
     *         disagreement found" and "nothing was compared" must not
     *         look alike
     */
    static bool basisSpansReported(const vector<double>& coords,
                                   const vector<string>& elements,
                                   const string& group,
                                   const vector< vector<int> >& shellsPerAtom,
                                   const vector<string>& reported,
                                   bool cartesian,
                                   string& detail);

    /**
     * How much of one molecular orbital sits on a set of atoms.
     *
     * A Mulliken population restricted to those atoms:
     *
     *     q_A = sum_{mu in A} sum_nu c_mu c_nu S_{mu,nu}
     *
     * divided by the orbital's total, so it comes out between 0 and 1.
     * With no overlap matrix the basis is taken as orthonormal and it
     * reduces to the sum of c^2, which is a Lowdin population and is
     * the right answer for a semiempirical calculation, whose
     * coefficients are in an orthonormalised basis to begin with.
     *
     * @param coefficients     one per basis function
     * @param functionsPerAtom one per atom, in the same order the
     *                         coefficients are; the caller must have
     *                         checked they sum to the basis size,
     *                         because a mapping that is off by one
     *                         atom produces a plausible number rather
     *                         than an error
     * @param atoms            which atoms the share is wanted for
     * @param overlap          nbas*nbas, row major, or empty
     *
     * @return the share, or -1 if the arguments do not line up
     */
    static double share(const vector<double>& coefficients,
                        const vector<int>& functionsPerAtom,
                        const vector<int>& atoms,
                        const vector<double>& overlap);

    /**
     * The sets of symmetry-equivalent atoms, for choosing a split.
     *
     * A FRAGMENT HAS TO BE A UNION OF THESE.  For a fragment's
     * symmetry orbitals to exist at all the group must map it onto
     * itself; if an operation takes one of its atoms outside it, there
     * is nothing to project and no symmetry orbitals to draw.  So
     * "which atoms are the fragment?" is not a question about picking
     * atoms, it is a question about grouping orbits -- and a caller
     * offering the choice can only offer it that way without being
     * able to construct an invalid answer.
     *
     * Ferrocene in D5d comes out {Fe} {10 C} {10 H}, so metal against
     * both rings is one tick.  Ethene as two CH2 units does NOT: each
     * holds one carbon and two hydrogens and the group maps one onto
     * the other, so it is no orbit of D2h and that diagram needs a
     * subgroup rather than a different grouping.
     */
    static bool orbitsOf(const vector<double>& coords,
                         const vector<string>& elements,
                         const string& group,
                         vector< vector<int> >& orbits,
                         string& why);

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
     * @param leftAtoms  optional: the atoms the left column is built
     *                   from, so a caller can ask how much of each
     *                   molecular orbital sits there without repeating
     *                   the orbit analysis
     * @param rightAtoms optional: the same for the right column
     * @param sideOfOrbit optional: one entry per orbit as orbitsOf()
     *                   returns them -- 0 for the left column, 1 for
     *                   the right, anything else to leave that orbit
     *                   out.  Without it the split is chosen
     *                   automatically, which handles AXn molecules and
     *                   diatomics and refuses everything else.
     *
     * @return false if no symmetry argument could be made, with `note`
     *         explaining which step gave out.
     */
    /**
     * The Schoenflies name of a linear molecule's group, if it is one.
     *
     * The symmetry generator has no infinite groups, so it returns a
     * finite stand-in -- N2 comes back as D4h.  Its operations are a
     * genuine subgroup of the real one and the fragment machinery is
     * right to use them, but the NAME is wrong on the page, and a
     * chemist reading "D4h" over a diatomic stops trusting the rest of
     * the diagram.
     *
     * Returns "DINFH" or "CINFV" in name, for display only: pass the
     * generator's own name to everything that computes.
     */
    /**
     * The coordination skeleton: a metal and the atoms bonded to it.
     *
     * Empty unless this really is a complex with polyatomic ligands.
     *
     * A CALLER MUST SEARCH SYMMETRY ON THIS, NOT ON THE MOLECULE.
     * Hexammine cobalt's donor set is a perfect octahedron and the
     * complex is not -- six ammonia rotors cannot all be -- so a
     * whole-molecule search returns C1 and the diagram is drawn in
     * no symmetry at all.  What a ligand field diagram classifies is
     * the skeleton, which is why every course draws the thing in Oh.
     *
     * @param keep  indices into elements, central atom first
     */
    static bool coordinationSkeleton(const vector<double>& coords,
                                     const vector<string>& elements,
                                     vector<int>& keep);

    static bool linearGroupName(const vector<double>& coords,
                                const vector<string>& elements,
                                string& name);

    static bool build(const vector<double>& coords,
                      const vector<string>& elements,
                      const string& group,
                      int charge,
                      MoColumn& left,
                      MoColumn& right,
                      string& note,
                      vector<int>* leftAtoms = 0,
                      vector<int>* rightAtoms = 0,
                      const vector<int>* sideOfOrbit = 0);
};

#endif

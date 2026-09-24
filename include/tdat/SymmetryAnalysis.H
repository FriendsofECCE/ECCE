/**
 * @file
 *
 * Symmetry analysis of a molecule's atoms: orbits, conjugacy classes,
 * and the representations a set of atomic orbitals spans.
 *
 * This is the step between "the molecule is Td" and "these four
 * hydrogens span a1 + t2" -- the decomposition an MO correlation
 * diagram is drawn from (#132).
 *
 * Deliberately takes plain coordinates and matrices rather than a
 * Fragment or a point group name, so it can be tested against the
 * textbook answers with no ECCE runtime, no detector and no files.
 */

#ifndef SYMMETRYANALYSIS_HH
#define SYMMETRYANALYSIS_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

class CharacterTable;

/** One symmetry operation: a 3x3 orthogonal matrix. */
struct SymOp
{
   double m[3][3];

   double trace(void) const   { return m[0][0] + m[1][1] + m[2][2]; }
   double determinant(void) const;
};


class SymmetryAnalysis
{
  public:

    /**
     * Where each atom goes under each operation.
     *
     * @param coords     3N, atom-major
     * @param elements   N, so an operation cannot map an atom onto a
     *                   different element
     * @param ops        the group's operations
     * @param tolerance  in the same units as coords
     * @param images     filled with ops.size() vectors of N indices;
     *                   images[o][a] is where atom a lands under op o
     *
     * @return false if any operation maps an atom onto nothing -- which
     *         means the operations and the coordinates are not in the
     *         same frame, the failure this analysis is most exposed to.
     */
    static bool atomImages(const vector<double>& coords,
                           const vector<string>& elements,
                           const vector<SymOp>& ops,
                           double tolerance,
                           vector< vector<int> >& images);

    /** Partition the atoms into orbits, from their images. */
    static void orbits(const vector< vector<int> >& images,
                       int numAtoms,
                       vector< vector<int> >& orbits);

    /** Conjugacy classes of the operations, as index sets. */
    static void conjugacyClasses(const vector<SymOp>& ops,
                                 vector< vector<int> >& classes);

    /**
     * Match computed conjugacy classes onto a character table's
     * classes, so an operation can be given its irreps' characters.
     *
     * Matched by (determinant, trace, class size), which identifies the
     * operation type and how many there are.  Where that is not unique
     * -- C2v's two mirrors, D4h's two sets of C2 axes -- the axis the
     * operation acts on decides, which is why the table's class names
     * carry it.
     *
     * @param classOfOp  filled with one table-class index per operation
     *
     * @return false if the classes cannot be matched one to one, which
     *         means the operations are not this group's.
     */
    static bool matchClasses(const vector<SymOp>& ops,
                             const vector< vector<int> >& classes,
                             const CharacterTable& table,
                             vector<int>& classOfOp);

    /**
     * The character of the representation spanned by one s orbital on
     * each atom of an orbit.
     *
     * For s orbitals this is just how many atoms each operation leaves
     * in place, since an s orbital carries no sign under rotation.
     *
     * @param chi  filled with one character per TABLE class
     */
    static bool orbitalCharacter(const vector<int>& orbit,
                                 const vector< vector<int> >& images,
                                 const vector<int>& classOfOp,
                                 int numClasses,
                                 vector<double>& chi);

    /**
     * The character of the whole atomic orbital basis.
     *
     * An operation contributes only through the atoms it leaves in
     * place, and each shell on such an atom contributes the character
     * of its angular momentum:
     *
     *     proper rotation by t    chi_l = 1 + 2 sum_{m=1..l} cos(m t)
     *     improper rotation by t  chi_l = (-1)^l [that, with t -> t-pi]
     *
     * Written as those sums rather than the usual closed form
     * sin((l+1/2)t)/sin(t/2), which is 0/0 for the identity and for
     * inversion and would need both special-cased.
     *
     * Reducing the result gives the irreps the basis spans, which must
     * match the multiset of labels the code itself reports in ORBSYM --
     * a check on the detection, the frame, the class matching and the
     * shell bookkeeping together, using no machinery beyond the atom
     * permutations already computed.
     *
     * @param shells     per atom, the angular momentum of each shell
     * @param cartesian  true if d and higher shells are the Cartesian
     *                   sets rather than the spherical ones: six
     *                   Cartesian d functions span D2 + D0, not D2, and
     *                   counting them as five would be wrong by an s
     * @param chi        filled with one character per class
     */
    static bool basisCharacter(const vector< vector<int> >& shells,
                               const vector< vector<int> >& images,
                               const vector<int>& classOfOp,
                               const vector<SymOp>& ops,
                               int numClasses,
                               bool cartesian,
                               vector<double>& chi);

    /**
     * The symmetry orbitals of one irrep, over an orbit's s orbitals.
     *
     * The projection operator, P = (l/h) sum_R chi(R)* R, applied to
     * each orbital of the orbit in turn.  That gives as many vectors as
     * there are orbitals, most of them repeats or zero, so the
     * independent ones are picked out by Gram-Schmidt and normalised.
     *
     * These are the terminal-atom symmetry orbitals -- the in-phase and
     * out-of-phase combinations a diagram draws on one side.
     *
     * @param table   the group's character table
     * @param irrep   which irrep to project onto
     * @param vectors filled with one coefficient vector per independent
     *                symmetry orbital, each over the orbit's atoms in
     *                orbit order
     *
     * @return false if the arguments do not describe this group.  The
     *         number of vectors returned is the irrep's multiplicity
     *         times its dimension, and a caller that knows the expected
     *         reduction should check it.
     */
    /**
     * The symmetry orbitals of one irrep, over an orbit's p orbitals.
     *
     * projectOrbit() handles one orbital per atom, which is what an s
     * shell is.  A p shell puts three on each, and they are not
     * carried along unchanged: an operation takes p_x on one atom to a
     * COMBINATION of p_x, p_y and p_z on its image, by the same 3x3
     * matrix that moves the atom.  So the projection runs over 3N
     * basis functions and mixes the components as it goes.
     *
     * This is what nitrite's pi system needs.  Its oxygen 2p set spans
     * 2a1 + a2 + 2b1 + b2, and the a2 in particular is a combination
     * of p orbitals perpendicular to the molecular plane that no s
     * projection can express.
     *
     * @param vectors  one per independent symmetry orbital, each of
     *                 length 3*orbit.size(), ordered atom-major:
     *                 (atom 0 x, atom 0 y, atom 0 z, atom 1 x, ...)
     *
     * @return false if the arguments do not describe this group.  The
     *         number of vectors is the irrep's multiplicity times its
     *         dimension, which a caller that has reduced the character
     *         already knows and should check.
     */
    static bool projectVectorOrbit(const vector<int>& orbit,
                                   const vector< vector<int> >& images,
                                   const vector<int>& classOfOp,
                                   const vector<SymOp>& ops,
                                   const CharacterTable& table,
                                   const string& irrep,
                                   vector< vector<double> >& vectors);

    /**
     * Which irrep a molecular orbital spans, from its coefficients.
     *
     * DERIVED IN THIS FRAME, rather than taken from the code's own
     * labels.  Two sources do not share an axis convention -- in C2v
     * which mirror is sigma-v decides b1 from b2, and in D2h the
     * disagreement needs two pairs relabelled at once, which no
     * single swap can undo.  Doing the classification here removes
     * the question instead of trying to reverse it afterwards, and it
     * also labels the very common calculation that reports no
     * symmetry at all.
     *
     * An operation carries a basis function on one atom to functions
     * on that atom's image: an s function unchanged, and the three p
     * functions mixed by the operation's own matrix, since they
     * transform as the Cartesian coordinates do.  The character is
     * then the overlap of the orbital with its own image, summed over
     * a degenerate set.
     *
     * @param orbitals  one coefficient vector per orbital; a
     *                  degenerate set is passed together, because
     *                  only the set has a character
     * @param perAtom   basis functions on each atom
     * @param shellOf   the angular momentum of each function, in the
     *                  same order the coefficients are in
     *
     * @return false if the basis contains anything beyond s and p --
     *         a d shell needs its own five-by-five transformation and
     *         has none here -- or if the character does not reduce to
     *         exactly one irrep of the right dimension, which means an
     *         assumption failed and the honest answer is no answer.
     */
    static bool orbitalIrrep(const vector< vector<double> >& orbitals,
                             const vector<int>& perAtom,
                             const vector<int>& shellOf,
                             const vector< vector<int> >& images,
                             const vector<int>& classOfOp,
                             const vector<SymOp>& ops,
                             const CharacterTable& table,
                             string& irrep);

    static bool projectOrbit(const vector<int>& orbit,
                             const vector< vector<int> >& images,
                             const vector<int>& classOfOp,
                             const CharacterTable& table,
                             const string& irrep,
                             vector< vector<double> >& vectors);
};

#endif

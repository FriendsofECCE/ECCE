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
class TGBSAngFunc;

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
    /**
     * Label a whole spectrum, choosing the axis convention that fits.
     *
     * WHERE DIFFERENT AXES CAN BE CHOSEN, TRY THEM AND KEEP THE ONE
     * THAT MATCHES -- and stop, because the set is finite.  The three
     * p components can be written in any of six orders and a code
     * need not use the one this analysis assumes; getting it wrong
     * does not fail loudly, it returns orbitals that are fractional
     * mixtures of two irreps, which is what water's b1 and b2 did.
     *
     * Each candidate is scored against the labels the calculation
     * reported, and the best is kept only if it labels more orbitals
     * than any other and agrees with what was reported.  With no
     * reported labels there is nothing to choose by, so the assumed
     * order is used and the caller is told the labels are derived.
     *
     * @param reported  the code's own labels, or empty
     * @param derived   filled with one irrep per orbital, empty where
     *                  no label could be established
     *
     * @return the number of orbitals labelled
     */
    static int labelSpectrum(const vector< vector<double> >& orbitals,
                             const vector<double>& energies,
                             const vector<string>& reported,
                             const vector<int>& perAtom,
                             const vector<int>& shellOf,
                             const vector< vector<int> >& images,
                             const vector<int>& classOfOp,
                             const vector<SymOp>& ops,
                             const CharacterTable& table,
                             vector<string>& derived);

    static bool orbitalIrrep(const vector< vector<double> >& orbitals,
                             const vector<int>& perAtom,
                             const vector<int>& shellOf,
                             const vector< vector<int> >& images,
                             const vector<int>& classOfOp,
                             const vector<SymOp>& ops,
                             const CharacterTable& table,
                             string& irrep,
                             const int* componentOrder = 0);

    /**
     * Which irrep a molecular orbital spans in the FULL point group,
     * from its coefficients, the real overlap matrix and ECCE's real
     * angular table -- the #147/#132 whole-AO-basis ORBSYM oracle.
     *
     * orbitalIrrep() above handles s and p only, assumes an
     * orthonormal basis, and searches six guessed p-orderings because
     * it has no way to derive the true one.  This instead:
     *
     *   - uses ShellRotation::buildD() to get each shell's REAL matrix
     *     under each operation, for any shell type the angular table
     *     describes (s, p, d, ...; Cartesian or spherical) -- no
     *     guessed component order, because it is read off the table
     *     itself, and no s/p ceiling;
     *   - weights the character by the real overlap S rather than
     *     assuming orthonormality, since a contracted Gaussian basis
     *     never is one: chi(R) = sum_k c_k^T S D(R) c_k / sum_k c_k^T S c_k;
     *   - takes operations ALREADY CONJUGATED into the coefficients'
     *     own frame (SymmetryAnalysis::conjugate(), with the rotation
     *     from alignFrames()) -- a code's SCF runs in whatever
     *     orientation it likes, not necessarily autosym's standard one
     *     the operations/images/classOfOp were built in.
     *
     * Declines (false) rather than labelling wrongly: if any shell a
     * used atom carries cannot be rotated to within shellResidualTol
     * (ShellRotation::buildD() failing, e.g. a shell type the angular
     * table does not describe), if the reduction is not cleanly one
     * irrep of the right dimension, or if any of the bookkeeping
     * (basis size, atom layout under the operations) does not match.
     *
     * @param perAtom     basis functions on each atom, in flattened
     *                    (BasisFlatten::flatten()) order
     * @param shellTypeOf the TGaussianBasisSet::AngularMomentum shell
     *                    type (0=s, 1=p, 2=d, ...) of each function,
     *                    same order -- ALL of one shell's functions
     *                    carry the same value, angfunc->numFuncs()
     *                    wide, consecutively, exactly as flatten()
     *                    emits them
     * @param overlap     the nbasis x nbasis overlap matrix S, same
     *                    flattened order (EspField::overlapOf() over
     *                    all pairs)
     * @param opsInCoeffFrame  the group's operations, conjugated into
     *                    the SAME frame the coefficients (and images/
     *                    classOfOp, built from atomImages() in that
     *                    same frame) are in
     * @param angfunc     the real angular function table
     *                    (JCode::getAngFunc()) the calculation's code
     *                    actually uses
     * @param shellResidualTol  passed through to ShellRotation::buildD()
     */
    static bool fullOrbitalIrrep(const vector< vector<double> >& orbitals,
                                 const vector<int>& perAtom,
                                 const vector<int>& shellTypeOf,
                                 const vector< vector<double> >& overlap,
                                 const vector< vector<int> >& images,
                                 const vector<int>& classOfOp,
                                 const vector<SymOp>& opsInCoeffFrame,
                                 TGBSAngFunc *angfunc,
                                 const CharacterTable& table,
                                 double shellResidualTol,
                                 string& irrep);

    /**
     * fullOrbitalIrrep(), over a whole spectrum grouped into degenerate
     * sets by energy (the same 1e-3 Hartree grouping labelSpectrum()
     * uses) -- no candidate search needed, unlike labelSpectrum(),
     * because the component order comes from the real table rather
     * than being guessed.
     *
     * @return the number of orbitals labelled
     */
    static int fullLabelSpectrum(const vector< vector<double> >& orbitals,
                                 const vector<double>& energies,
                                 const vector<int>& perAtom,
                                 const vector<int>& shellTypeOf,
                                 const vector< vector<double> >& overlap,
                                 const vector< vector<int> >& images,
                                 const vector<int>& classOfOp,
                                 const vector<SymOp>& opsInCoeffFrame,
                                 TGBSAngFunc *angfunc,
                                 const CharacterTable& table,
                                 double shellResidualTol,
                                 vector<string>& derived);

    static bool projectOrbit(const vector<int>& orbit,
                             const vector< vector<int> >& images,
                             const vector<int>& classOfOp,
                             const CharacterTable& table,
                             const string& irrep,
                             vector< vector<double> >& vectors);

    /**
     * The rigid rotation (+ centroid shift) that best carries one set
     * of coordinates onto another -- the orthogonal Procrustes/Kabsch
     * fit (#151).
     *
     * Needed because a code's MO coefficients are reported in ITS OWN
     * frame (whatever orientation its SCF happened to run in), while
     * the symmetry operations this analysis works with are in autosym's
     * STANDARD frame (SymmetryOps::find() reorients and symmetrises).
     * The two agree only by accident, so an operation built in one
     * frame cannot be applied directly to coefficients reported in the
     * other -- see conjugate() below, which is what actually makes the
     * two usable together.
     *
     * Solved via the eigendecomposition of H^T H (H = fromCentered^T *
     * toCentered), a 3x3 symmetric matrix -- plain Jacobi rotations,
     * no external linear algebra needed for a matrix this size.
     *
     * @param from   3N atom-major coordinates, in the frame R maps FROM
     * @param to     3N atom-major coordinates, SAME ATOM ORDER, in the
     *               frame R maps TO
     * @param R      filled with the rotation: R*(from_i - centroid(from))
     *               best approximates (to_i - centroid(to))
     * @param rmsd   filled with the residual RMS distance, in the same
     *               units as the coordinates, AFTER applying R -- the
     *               number a caller should refuse a large value of
     *               rather than trust a bad fit silently
     *
     * @return false if from/to are not the same size, empty, or fewer
     *         than 3 atoms (nothing to determine an orientation from)
     */
    static bool alignFrames(const vector<double>& from,
                            const vector<double>& to,
                            SymOp& R,
                            double& rmsd);

    /**
     * Conjugate a symmetry operation from the frame it was built in
     * into another frame related to it by a known rotation:
     * R_s = Q^T * R * Q, where Q is alignFrames()'s result carrying the
     * SECOND frame onto the FIRST.  (Q rotates op's own operand into
     * the frame R_s will be applied in, so Q^T undoes that afterwards.)
     */
    static SymOp conjugate(const SymOp& R, const SymOp& Q);
};

#endif

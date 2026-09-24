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
    static bool projectOrbit(const vector<int>& orbit,
                             const vector< vector<int> >& images,
                             const vector<int>& classOfOp,
                             const CharacterTable& table,
                             const string& irrep,
                             vector< vector<double> >& vectors);
};

#endif

/**
 * @file
 *
 * The atomic-orbital basis behind a calculation's MO coefficients, and
 * the composition of one MO in it: how much of the orbital sits on each
 * basis function, each (atom, shell) and each atom (#161).
 *
 * The default population is Mulliken's, w_mu = c_mu (S c)_mu, divided by
 * c.S.c so the parts always sum to 100%.  It is the one the MO diagram's
 * fragment shares use (MoComposition, MoFragments::shareOfIndices), so
 * an orbital reads the same here as in the diagram, and it is the one
 * ORCA, Multiwfn and most textbooks mean by "composition".  Individual
 * Mulliken shares are not bounded: an antibonding orbital can put more
 * than 100% on one atom and a negative share on another.  The
 * alternative, plain c^2 (also normalised), is bounded and
 * overlap-free, but depends on how diffuse the basis is.
 *
 * Nothing here depends on wx.  The basis comes from a calculation's
 * stored basis set and is flattened exactly as the MO display does
 * (BasisFlatten), so Cartesian and spherical sets, ECPs and the
 * per-code MO column orders are handled in one place.
 */

#ifndef MOAOBASIS_HH
#define MOAOBASIS_HH

#include <functional>
#include <string>
#include <vector>
using std::string;
using std::vector;

class Fragment;
class IPropCalculation;
struct EspBasisFunction;

class MoAoBasis
{
  public:

    /** One basis function: one column of the MO coefficient table. */
    struct Function
    {
      int    atom;       // index into `elements`
      int    l;          // 0=s, 1=p, 2=d, ...
      int    shellSeq;   // 1 for the atom's first shell of this l, 2 ...
      string component;  // "s", "x", "z2", "xy", ... (see componentName)

      Function() : atom(-1), l(0), shellSeq(1) {}
    };

    /** How a function's share of an orbital is weighted. */
    enum Method { MULLIKEN, COEFFICIENT_SQUARED };

    /** A group of functions' share of one orbital, as a fraction. */
    struct Share
    {
      int    atom;
      int    l;          // -1 for a whole atom
      int    shellSeq;   // 0 = every shell of this l on the atom
      string label;      // "O1", "O1 p", "O1 p2"
      double share;

      Share() : atom(-1), l(-1), shellSeq(0), share(0.0) {}
    };

    /** The composition of one orbital. */
    struct Composition
    {
      Method         method;
      double         norm;         // c.S.c (c.c for COEFFICIENT_SQUARED)
      vector<double> perFunction;  // one per column; sums to 1
      vector<Share>  atoms;        // one per atom
      vector<Share>  types;        // (atom, l), all shells of that l
      vector<Share>  shells;       // (atom, l, nth shell of that l)

      Composition() : method(MULLIKEN), norm(0.0) {}
    };

    vector<string>   elements;     // per atom, in coefficient order
    vector<int>      perAtom;      // functions per atom
    vector<Function> functions;    // one per coefficient column
    vector<double>   overlap;      // nbas*nbas row major; empty if the
                                   // basis is orthonormal (semiempirical)
    bool             orthonormal;
    string           complaint;    // why build() returned false

    MoAoBasis() : orthonormal(false) {}

    /**
     * Build the basis of a calculation, in the STORED frame its
     * coefficients are in.
     *
     * @param width    the coefficient table's column count; it decides
     *                 Cartesian against spherical (see the .C file)
     * @param coordsAng atom-major x,y,z in Angstrom, stored frame
     */
    bool build(IPropCalculation *calc, const vector<string>& symbols,
               const vector<double>& coordsAng, int width,
               std::function<void(double)> onRow = nullptr);

    /**
     * The fractions of one orbital (a row of the coefficient table in
     * this basis's column order).  False if the size does not match or
     * the orbital has no norm.
     */
    bool analyse(const vector<double>& coefficients, Method method,
                 Composition& out) const;

    /**
     * One orbital's rows of the calculation's "MO" or "MOBETA" table,
     * with the per-code column order undone (MoAoOrder), so they line
     * up with functions[].
     */
    static bool coefficients(IPropCalculation *calc,
                             const vector<string>& symbols, bool beta,
                             vector< vector<double> >& rows);

    /** Element symbols and Angstrom coordinates of a fragment's atoms. */
    static bool fragmentGeometry(Fragment *frag, vector<string>& symbols,
                                 vector<double>& coordsAng);

    //  The two pieces the MO diagram also needs, kept here so there is
    //  one copy of the walk that decides what a coefficient column is.

    /**
     * Functions per atom and the angular momentum of each function.
     * `shellSeq` and `component` (optional) get the nth-shell number and
     * the component index within the shell, per function.
     */
    static bool functionCounts(IPropCalculation *calc,
                               const vector<string>& symbols, int width,
                               vector<int>& counts, vector<int>& shellOf,
                               bool *semiempirical = 0,
                               vector<int> *shellSeq = 0,
                               vector<int> *component = 0);

    /** The real overlap matrix and per-atom counts (see build()). */
    static bool overlapMatrix(IPropCalculation *calc,
                              const vector<string>& symbols,
                              const vector<double>& coordsAng, int width,
                              vector<int>& perAtom, vector<double>& Sflat,
                              std::function<void(double)> onRow = nullptr,
                              vector<EspBasisFunction> *basisOut = 0);

    /** "s", "x", "xx" (Cartesian d), "z2", "x2-y2" (spherical d), "f#3" ... */
    static string componentName(const EspBasisFunction& f, int l,
                                int indexInShell);

    static const char* methodName(Method m);
};

#endif

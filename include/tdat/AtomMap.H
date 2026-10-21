/**
 * @file
 *
 * Atom mapping and alignment of two structures of one reaction (#419).
 *
 * QST2/QST3 and NEB-TS want reactant, product and guess with the atoms in
 * the same order and in the same orientation.  No wx here: the Calculation
 * Editor only shows what this decides, and tests/atommap drives it from the
 * command line.
 *
 * Coordinates are flat x,y,z in Angstrom; elements are symbols, one per atom.
 * A mapping is indexed by the MOVING structure's atom (product, guess) and
 * holds the matching atom of the REFERENCE structure (reactant).
 */

#ifndef ATOMMAP_HH
#define ATOMMAP_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

/** Rotation + translation: x' = R x + t, R row-major and proper. */
struct AtomAlignment
{
   double R[9];
   double t[3];
   double rmsdBefore;   // over the mapped atoms, as given
   double rmsdAfter;    // after applying R, t
   AtomAlignment();
};

struct AtomMapOptions
{
   /** Same element order and no atom further than this (A) after
    *  superposition: the order is kept.  Bond lengths change by well under
    *  this in a reaction; two swapped atoms are always further apart. */
   double identityMaxDisp;
   /** A mapped pair further apart than this (A) after alignment is flagged. */
   double farThreshold;
   AtomMapOptions() : identityMaxDisp(0.5), farThreshold(1.0) {}
};

struct AtomMapResult
{
   bool ok;
   string error;              // set when !ok (counts or elements differ)
   vector<int> map;           // moving atom -> reference atom
   bool identity;             // order kept
   int nChanged;              // atoms with map[i] != i
   AtomAlignment alignment;   // moving onto reference, over the mapping
   vector<double> displacement;   // per moving atom, after alignment (A)
   vector<int> far;           // moving atoms over farThreshold
   AtomMapResult() : ok(false), identity(false), nChanged(0) {}
};

class AtomMap
{
 public:
   /** Map `moving` onto `reference`; fills the mapping, the alignment and the
    *  report.  Differing atom count or element multiset gives ok=false and a
    *  plain error text, nothing else is filled. */
   static AtomMapResult map(const vector<string>& refElements,
                            const vector<double>& refCoords,
                            const vector<string>& movElements,
                            const vector<double>& movCoords,
                            const AtomMapOptions& options = AtomMapOptions());

   /** Kabsch superposition of moving onto reference over the pairs
    *  moving i <-> reference map[i] (Horn's quaternion form: never a
    *  reflection). */
   static AtomAlignment align(const vector<double>& refCoords,
                              const vector<double>& movCoords,
                              const vector<int>& map);

   /** The fit the report uses: heavy (non-H) atoms only when there are at
    *  least 3, since H atoms are what moves in a reaction; otherwise all
    *  atoms, mass-weighted.  movElements are the moving structure's symbols.
    *  rmsdBefore/After still run over all mapped atoms. */
   static AtomAlignment alignHeavy(const vector<double>& refCoords,
                                   const vector<double>& movCoords,
                                   const vector<int>& map,
                                   const vector<string>& movElements);

   static void apply(const AtomAlignment& a, vector<double>& coords);

   /** The moving structure in reference order: out[map[i]] = in[i]. */
   static vector<double> renumber(const vector<double>& movCoords,
                                  const vector<int>& map);
   static vector<string> renumber(const vector<string>& movElements,
                                  const vector<int>& map);

   /** Optimal assignment (Hungarian, O(n^3)) of a square cost matrix,
    *  row-major n*n; result[row] = column. */
   static vector<int> assign(const vector<double>& cost, int n);
};

#endif

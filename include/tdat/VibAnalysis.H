/**
 * @file
 *
 * What each normal mode is: its irrep in the molecule's point group, whether
 * that makes it IR and Raman active, and a short description of the motion
 * (#367).  Wx-free; the Vibrational Frequencies panel only shows the result.
 *
 * Symmetry comes from ECCE's own operations and character tables, so every
 * code gets labels, not only those that print them.  The description is a
 * projection of each mode onto internal coordinates (stretches, bends,
 * out-of-plane bends, torsions) in mass-weighted Cartesian space, not a
 * potential energy distribution: it needs no force constants.
 */
#ifndef VIBANALYSIS_HH
#define VIBANALYSIS_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

struct SymOp;
class Fragment;

struct VibBond
{
   int    a, b;
   double order;
};

struct VibInput
{
   vector<string> elements;           // N chemical symbols
   vector<double> coords;             // 3N, Angstrom, the frame of the modes
   vector<VibBond> bonds;             // empty: guessed from covalent radii
   vector<double> freqs;              // cm-1, one per mode
   vector< vector<double> > modes;    // per mode 3N Cartesian displacements
};

struct VibModeInfo
{
   string irrep;         // "" not assigned (rotation/translation), "?" unclear
   bool   activityKnown; // ir/raman are only meaningful when true
   bool   ir;
   bool   raman;
   string label;         // short form, UTF-8: nu_as(OH), delta(HOH); Tx..Rz for rigid motion
   string detail;        // long form with percentages, UTF-8
   bool   rigid;         // a translation or rotation of the whole molecule
   string remark;        // how a rigid label was found, when it is not certain
};

struct VibResult
{
   string group;                 // point group the labels are in
   string note;                  // why symmetry or description is missing
   vector<VibModeInfo> modes;
};

/** One internal coordinate chosen by atoms (0-based): 2 atoms a stretch,
    3 a bend about the middle one, 4 a torsion (a-b-c-d). */
struct VibCoordSpec
{
   int n;
   int at[4];
};

/**
 * Mode composition (#486): how much of each mode lies along each chosen
 * internal coordinate.  Not a potential energy distribution (that needs a
 * complete non-redundant set and the force constants).
 */
struct VibComposition
{
   vector<VibCoordSpec> coords;
   vector<string> names;                // "C2-O3 stretch", "H9-N4-C2 bend"
   vector< vector<double> > percent;    // per mode, per coordinate, sums to 100
};

class VibAnalysis
{
  public:
   /**
    * Composition of every mode over `chosen`; when that is empty, the
    * automatic set: every bond as a stretch, plus every bend whose centre
    * is in `centres` (all non-hydrogen atoms if `centres` is empty).
    * Rigid-motion rows get all zeros.  False without a usable input.
    */
   static bool composition(const VibInput& in, const vector<VibCoordSpec>& chosen,
                           const vector<int>& centres, VibComposition& out);

   /** Symmetry and description both; false if neither could be produced. */
   static bool analyse(const VibInput& in, VibResult& out);

   /**
    * How much of each mode lies in the part of the molecule `atoms`
    * (0-based): two atoms are one stretch; more, every stretch, bend and
    * torsion that lies entirely within them.  Shares are percent of each
    * mode over the automatic set plus those coordinates, so they are
    * comparable between selections.  `label` names what was measured.
    */
   static bool selectionShare(const VibInput& in, const vector<int>& atoms,
                              vector<double>& share, string& label);

   /** The input for a calculation's fragment and its modes. */
   static bool input(Fragment& frag, const vector<double>& freqs,
                     const vector< vector<double> >& modes, VibInput& in);

   /** Same for a calculation's fragment, to be given the modes. */
   static bool analyse(Fragment& frag, const vector<double>& freqs,
                       const vector< vector<double> >& modes, VibResult& out);

   /**
    * Irrep and activity of each mode, given the operations of `group` in
    * its standard frame and the molecule's coordinates in that frame.
    * Fills irrep/ir/raman of out.modes.
    */
   static bool symmetry(const VibInput& in, const string& group,
                        const vector<double>& stdCoords,
                        const vector<SymOp>& ops, VibResult& out);

   /** Fills label/detail of out.modes from geometry alone. */
   static void describe(const VibInput& in, VibResult& out);

   /** Linear molecules are labelled with the group's sigma/pi names. */
   static string linearName(const string& irrep, const string& group);
};

#endif

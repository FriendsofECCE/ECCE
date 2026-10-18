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
   string label;         // short form, UTF-8: nu_as(OH), delta(HOH)
   string detail;        // long form with percentages, UTF-8
};

struct VibResult
{
   string group;                 // point group the labels are in
   string note;                  // why symmetry or description is missing
   vector<VibModeInfo> modes;
};

class VibAnalysis
{
  public:
   /** Symmetry and description both; false if neither could be produced. */
   static bool analyse(const VibInput& in, VibResult& out);

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

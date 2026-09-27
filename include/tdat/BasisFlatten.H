/**
 * @file
 *
 * The atomic-orbital basis flattening shared by the ESP field evaluator
 * (src/viz/propsgcommands/ComputeMoCmd.C) and the full-point-group MO
 * symmetry projection (SymmetryAnalysis, #147/#151).
 *
 * Moved here verbatim from ComputeMoCmd::normalize(), ::getoddNormalize()
 * and the body of ::buildEspBasis() -- ONE implementation, not a copy,
 * so a diagram built from it and an ESP surface built from it are
 * guaranteed to agree on what a basis function is, to the digit, and so
 * this can be linked into a plain g++ test (tests/symmetry) with no wx
 * or Open Inventor dependency the way ComputeMoCmd itself has.
 *
 * ComputeMoCmd's own methods of these names now just forward here; see
 * that file for why each step (NORMP, NORMF, the odd angular factor) is
 * what it is -- unchanged by the move.
 */

#ifndef BASISFLATTEN_HH
#define BASISFLATTEN_HH

#include <string>
#include <vector>
using std::string;
using std::vector;

class TGaussianBasisSet;
class TGBSConfig;
class JCode;
class TGBSAngFunc;
struct EspBasisFunction;

namespace BasisFlatten
{
  /**
   * Optimises one atom's basis sets and renormalises their primitives
   * from the NORMS convention (contraction coefficients given for
   * normalised primitives) to unnormalised ones, exactly as
   * ComputeMoCmd::normalize() always has.  Caller owns the returned
   * TGaussianBasisSet objects.
   */
  vector<TGaussianBasisSet*> normalize(
      const string& atomID,
      const vector<const TGaussianBasisSet*>& gbslist,
      TGBSConfig& gbsConfig,
      const JCode* code);

  /** The odd angular normalisation factor for one shell/component --
   *  1.0 for a Cartesian basis, and for spherical d/f components whose
   *  monomial expansion is not already unit-normalised. */
  double getoddNormalize(int shell_type, int index, TGBSAngFunc *angfunc);

  /**
   * Flatten a molecule's basis into one EspBasisFunction per atomic
   * orbital, atom-major, in exactly the MO coefficient column order --
   * the same walk (atoms, basis sets, contracted sets, shell columns,
   * degeneracies) ComputeMoCmd's field evaluation makes, and it MUST
   * stay in step with that.
   *
   * @param atomSymbols    element symbol per atom
   * @param atomCoordsAng  atom-major x,y,z, in ANGSTROM (converted to
   *                       bohr internally, as ComputeMoCmd::buildEspBasis
   *                       always has)
   * @param maxShell       shells beyond this are skipped, leaving an
   *                       empty entry so later indices still line up
   * @param length_shell   how many degenerate functions per shell type
   */
  bool flatten(const vector<string>& atomSymbols,
               const vector<double>& atomCoordsAng,
               TGBSConfig *gbsConfig,
               const JCode *code,
               TGBSAngFunc *angfunc,
               int maxShell, const int *length_shell,
               vector<EspBasisFunction>& basis);
}

#endif

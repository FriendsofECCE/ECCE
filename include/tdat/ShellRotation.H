/**
 * @file
 *
 * How a symmetry operation acts on one shell of a Gaussian basis --
 * s, p, d, ... , Cartesian or spherical -- expressed as a matrix over
 * the shell's own functions (#151).
 *
 * SymmetryAnalysis::orbitalIrrep() already knows how an operation moves
 * an s or a p function (identity, or the operation's own 3x3 matrix,
 * because p transforms exactly as x,y,z do).  A d shell -- or higher,
 * or a spherical one whose components are not single monomials -- needs
 * its own transformation matrix, and this is where it is DERIVED rather
 * than assumed: substitute the operation's inverse into each function's
 * monomial expansion, and re-express the result in the shell's own
 * basis by least squares.  The shell's span is rotation-invariant (a
 * standard fact of representation theory -- a Cartesian shell of
 * degree l spans a sum of SO(3) irreps closed under rotation, and a
 * spherical shell IS one of those irreps on its own), so the fit should
 * be EXACT; the residual this reports is therefore not a tolerance to
 * shrug at but a check on everything upstream of it -- the frame
 * alignment, the class match, the angular table itself.
 */

#ifndef SHELLROTATION_HH
#define SHELLROTATION_HH

#include <vector>
using std::vector;

class TGBSAngFunc;
struct SymOp;

namespace ShellRotation
{
  /**
   * The matrix of one shell (of a given TGaussianBasisSet::AngularMomentum
   * value -- 0=s, 1=p, 2=d, ...) under one symmetry operation, in the
   * SAME FRAME the operation's own matrix is expressed in (the caller's
   * job to have conjugated it there first, e.g. with
   * SymmetryAnalysis::conjugate()).
   *
   * Atom-independent and radial-part-independent: the radial part of a
   * Gaussian shell is a function of |r| alone, so it is untouched by a
   * rotation, and every atom's shell of the same shell_type shares the
   * same angular definition (one TGBSAngFunc table for the whole
   * calculation) -- so this needs nothing about which atom or which
   * contracted set the shell belongs to, only its TYPE and the
   * operation.
   *
   * @param shell_type  TGaussianBasisSet::AngularMomentum value (0=s,
   *                    1=p, 2=d, ...)
   * @param angfunc     the angular function table (JCode::getAngFunc()),
   *                    Cartesian or spherical -- this is oblivious to
   *                    which, since it works from whatever getFunc()
   *                    and the odd-normalisation factor actually report
   * @param R           the operation, in the frame the shell's own
   *                    coefficients are in
   * @param residualTol the least-squares residual (RMS over the
   *                    degree-l monomial basis) above which the fit is
   *                    declined rather than trusted
   * @param D           filled with an m x m matrix (m = the shell's
   *                    function count), D[row][col]: rotating function
   *                    "col" gives sum_row D[row][col] * function[row]
   *
   * @return false if the fit's residual exceeds residualTol, or the
   *         shell/table is malformed (an r^k radial factor in the
   *         angular table, which nothing shipped uses) -- meaning
   *         DECLINE, not "assume identity" or any other guess.
   */
  bool buildD(int shell_type, TGBSAngFunc *angfunc, const SymOp& R,
             double residualTol, vector< vector<double> >& D);
}

#endif

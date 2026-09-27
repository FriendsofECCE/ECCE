///////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: BasisAngularNorm.C
//
// BasisFlatten::getoddNormalize(), split out of BasisFlatten.C into its
// own translation unit on purpose: it only needs the angular table
// (TGBSAngFunc) and TGaussianBasisSet's shell-type ENUM (a compile-time
// constant, no linkage), while normalize()/flatten() in BasisFlatten.C
// need the full TGaussianBasisSet/TGBSConfig/JCode machinery (XML, DAV).
//
// ShellRotation::buildD() calls only this function, not normalize() or
// flatten() -- and several of tests/symmetry's standalone checks
// (testSymmetryAnalysis, testMoFragments, testOracle, testHuckel, ...)
// link SymmetryAnalysis.C, which now calls ShellRotation::buildD(),
// without a build tree. Keeping getoddNormalize() in its own light file
// means those checks still need no XML/DAV dependency to build, even
// though SymmetryAnalysis.C's full-group projection path exists in the
// same object file they link.
///////////////////////////////////////////////////////////////////////////

#include "tdat/BasisFlatten.H"
#include "tdat/TGBSAngFunc.H"
#include "dsm/TGaussianBasisSet.H"
#include "util/ErrMsg.H"

#include <cmath>

namespace BasisFlatten {

double getoddNormalize(int shell_type, int index, TGBSAngFunc *angfunc)
{
   int l, m, n;
   if (angfunc->basisType() == TGBSAngFunc::Cartesian) {
      return 1.0;
   } else {
      // Evaluate numerical prefactor for spherical basis functions
      AngMomFunc func;
      switch (shell_type) {
         case TGaussianBasisSet::s_shell:
         case TGaussianBasisSet::p_shell:
            return 1.0;
            break;
         case TGaussianBasisSet::d_shell:
            angfunc->getMaxExponents(shell_type, index, l, m, n);
            if (n == 2) return 1.0;
            return sqrt(3.0);
            break;
         case TGaussianBasisSet::f_shell:
            angfunc->getMaxExponents(shell_type, index, l, m, n);
            func = angfunc->getFunc(shell_type, index);
            if (func.size() == 1) return sqrt(15.0);
            if (func.size() == 2) {
               if (l == 3 || m == 3) return sqrt(2.5);
               return sqrt(15.0);
            }
            if (func.size() == 3) {
               if (l == 3 || m == 3) return sqrt(1.5);
               return 1.0;
            }
            break;
            // This will need to be fixed when h and i are added. - TLW
         case TGaussianBasisSet::g_shell:
         case TGaussianBasisSet::h_shell:
         case TGaussianBasisSet::i_shell:
            return 1.0;
            break;
         default:
            EE_RT_ASSERT(0, EE_FATAL,
                  "Unrecognized funcType type");
            break;
      }
      // should never get here--makes the compiler happy to return a value
      return 1.0;
   }
}

}  // namespace BasisFlatten

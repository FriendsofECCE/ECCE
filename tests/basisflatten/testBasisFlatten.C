//  Is BasisFlatten's move from ComputeMoCmd byte-for-byte the same
//  arithmetic it always was?
//
//  ComputeMoCmd::normalize()/getoddNormalize()/buildEspBasis() were
//  moved verbatim into tdat/BasisFlatten.[HC] so the MO symmetry
//  projection (SymmetryAnalysis, #147/#151) and the ESP field evaluator
//  share ONE implementation rather than a second copy. A pure text
//  move should reproduce the exact same numbers -- checked here against
//  an INDEPENDENT closed-form oracle, not against the moved code's own
//  intermediate steps: the standard normalisation constant of a single
//  primitive Cartesian s Gaussian,
//
//      N(alpha) = (2 alpha / pi)^(3/4)
//
//  which every physical chemistry textbook gives directly from
//  <g|g> = (pi/(2 alpha))^(3/2) = 1/N(alpha)^2. A single-primitive
//  contraction written in the library convention (coefficient 1.0,
//  meaning "the whole, already-unit-normalised primitive") must come
//  out of BasisFlatten::flatten() with contraction[0] == N(alpha),
//  since NORMP (undo the library's own-primitive normalisation) and
//  NORMF (renormalise the contracted function to unit self-overlap)
//  cancel back to exactly that for a contraction of size 1.
//
//  Needs a real JCode (Gaussian-16's .edml, for GeneralContractions and
//  for its real, shipped MOOrdering table -- the actual Cartesian s/p
//  monomial order ECCE uses, not one invented for this test) and a real
//  TGBSConfig, built the same way tests/slater/testSlaterBasisSet.C
//  does: through ICalcUtils::importConfig() on NumericalBasis text, not
//  by hand-poking TGaussianBasisSet's internals.
//
//  Run with ECCE_HOME set to the repo root (tests/basisflatten/run_tests.py
//  does this); needs no build tree -- see that script for the g++ line.

#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "tdat/BasisFlatten.H"
#include "tdat/EspField.H"
#include "tdat/TGBSAngFunc.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TGaussianBasisSet.H"
#include "dsm/ICalcUtils.H"
#include "dsm/JCode.H"
#include "dsm/CodeFactory.H"

static int failures = 0;

static void check(bool ok, const string& what)
{
  printf("  %-70s %s\n", what.c_str(), ok ? "ok" : "FAIL");
  if (!ok) failures++;
}

int main()
{
  const JCode* code = CodeFactory::lookup("Gaussian-16");
  check(code != 0, "Gaussian-16 JCode loaded (needs ECCE_HOME set to the repo)");
  if (code == 0) return 1;

  //  A single Cartesian s primitive, library convention (coefficient 1.0
  //  == the primitive is already unit-normalised on its own).
  const double alpha = 1.3;
  ostringstream gbs;
  gbs << "NumericalBasis\n"
      << "basis \"ao basis\" cartesian print\n"
      << "H S\n"
      << "      " << alpha << "              1.0000000\n"
      << "END\n"
      << "EndNumericalBasis";
  istringstream in(gbs.str());
  TGBSConfig* cfg = ICalcUtils::importConfig(in);
  check(cfg != 0, "TGBSConfig parsed from NumericalBasis text");
  if (cfg == 0) return 1;

  TGBSAngFunc* angfunc = code->getAngFunc(TGaussianBasisSet::Cartesian);
  check(angfunc != 0, "Gaussian-16's real Cartesian MOOrdering table loaded");
  if (angfunc == 0) return 1;
  check(angfunc->basisType() == TGBSAngFunc::Cartesian,
        "angular table reports Cartesian");

  int maxShell = angfunc->maxShells();
  //  Same table ComputeMoCmd's execute() builds this from (Cartesian:
  //  d=6, f=10, ...).
  int length_shell[7] = { 1, 3, 6, 10, 15, 21, 28 };

  vector<string> atomSymbols(1, "H");
  vector<double> atomCoordsAng(3, 0.0);
  vector<EspBasisFunction> basis;

  bool ok = BasisFlatten::flatten(atomSymbols, atomCoordsAng, cfg, code,
                                  angfunc, maxShell, length_shell, basis);
  check(ok, "flatten() succeeded");
  check(basis.size() == 1, "exactly one flattened function (one s shell)");

  if (ok && basis.size() == 1) {
    const EspBasisFunction& fn = basis[0];
    check(fn.exponent.size() == 1 && fn.contraction.size() == 1,
          "one primitive in the flattened function");

    const double expected = pow(2.0*alpha/M_PI, 0.75);
    const double got = fn.contraction.empty() ? 0.0 : fn.contraction[0];
    char msg[160];
    snprintf(msg, sizeof(msg),
             "NORMP+NORMF coefficient %.10f matches N(alpha)=%.10f "
             "(closed form, independent of BasisFlatten)", got, expected);
    check(fabs(got - expected) < 1.0e-6, msg);

    check(fn.exponent.size() == 1 && fabs(fn.exponent[0] - alpha) < 1.0e-12,
          "exponent passed through unchanged");
    check(fn.angularCoef.size() == 1 && fabs(fn.angularCoef[0] - 1.0) < 1.0e-12,
          "Cartesian s angular coefficient is 1 (getoddNormalize==1 for "
          "Cartesian, real monomial table)");
    check(fn.powerX.size() == 1 && fn.powerX[0] == 0 &&
          fn.powerY[0] == 0 && fn.powerZ[0] == 0,
          "s shell is the bare monomial (0,0,0)");
  }

  //  A shell beyond maxShell must leave an EMPTY placeholder, not shrink
  //  the vector -- buildEspBasis()'s index-alignment contract with the MO
  //  coefficient columns, preserved unchanged by the move.
  {
    vector<EspBasisFunction> basis2;
    bool ok2 = BasisFlatten::flatten(atomSymbols, atomCoordsAng, cfg, code,
                                     angfunc, /*maxShell=*/0, length_shell,
                                     basis2);
    check(!ok2 || (basis2.size() == 1 && basis2[0].empty()),
          "a shell past maxShell becomes an empty placeholder, index preserved");
  }

  delete angfunc;
  delete cfg;

  //  Cartesian d self-overlap, on/off-axis component (issue: getoddNormalize
  //  Cartesian branch was a flat 1.0 for every code, so a Cartesian
  //  d_xy/d_xz/d_yz came out with self-overlap 1/3 relative to d_xx --
  //  Gaussian actually normalizes each Cartesian component individually
  //  (6D/10F) and needs that corrected; NWChem does not and must stay
  //  exactly as it was. One shared single-primitive d shell, checked
  //  against both codes' real MOOrdering tables.
  {
    const double alpha = 0.8;
    ostringstream gbs;
    gbs << "NumericalBasis\n"
        << "basis \"ao basis\" cartesian print\n"
        << "H D\n"
        << "      " << alpha << "              1.0000000\n"
        << "END\n"
        << "EndNumericalBasis";
    istringstream in(gbs.str());
    TGBSConfig* dcfg = ICalcUtils::importConfig(in);
    check(dcfg != 0, "d-shell TGBSConfig parsed");

    vector<string> atomSymbols(1, "H");
    vector<double> atomCoordsAng(3, 0.0);
    int length_shell[7] = { 1, 3, 6, 10, 15, 21, 28 };

    struct { const char* codeName; bool normalized; } cases[] = {
      { "Gaussian-16", true },
      { "NWChem", false },
    };
    for (const auto& c : cases) {
      const JCode* dcode = CodeFactory::lookup(c.codeName);
      TGBSAngFunc* dang = dcode ? dcode->getAngFunc(TGaussianBasisSet::Cartesian) : 0;
      char what[160];
      snprintf(what, sizeof(what), "%s Cartesian MOOrdering loaded", c.codeName);
      check(dang != 0, what);
      if (dang == 0) continue;

      vector<EspBasisFunction> dbasis;
      bool dok = BasisFlatten::flatten(atomSymbols, atomCoordsAng, dcfg, dcode,
                                       dang, dang->maxShells(), length_shell,
                                       dbasis);
      snprintf(what, sizeof(what), "%s: d shell flattened (6 Cartesian components)", c.codeName);
      check(dok && dbasis.size() == 6, what);
      if (!dok || dbasis.size() != 6) continue;

      //  Find an on-axis (xx) and an off-axis (xy) component by their
      //  actual powers rather than assuming an index -- Gaussian's and
      //  NWChem's real MOOrdering tables list the six d components in
      //  different orders (see each .edml).
      int onAxisIdx = -1, offAxisIdx = -1;
      for (int deg = 0; deg < 6 && (onAxisIdx < 0 || offAxisIdx < 0); deg++) {
        int lx, ly, lz;
        dang->getMaxExponents(TGaussianBasisSet::d_shell, deg, lx, ly, lz);
        if (lx == 2) onAxisIdx = deg;
        else if (lx == 1 && ly == 1) offAxisIdx = deg;
      }
      snprintf(what, sizeof(what), "%s: found both xx and xy components", c.codeName);
      check(onAxisIdx >= 0 && offAxisIdx >= 0, what);
      if (onAxisIdx < 0 || offAxisIdx < 0) continue;

      const double onAxis = EspField::overlapOf(dbasis[onAxisIdx], dbasis[onAxisIdx]);
      const double offAxis = EspField::overlapOf(dbasis[offAxisIdx], dbasis[offAxisIdx]);
      snprintf(what, sizeof(what),
               "%s: on-axis (xx) self-overlap is 1", c.codeName);
      check(fabs(onAxis - 1.0) < 1.0e-8, what);

      const double expectedOffAxis = c.normalized ? 1.0 : (1.0/3.0);
      snprintf(what, sizeof(what),
               "%s: off-axis (xy) self-overlap is %s",
               c.codeName, c.normalized ? "1 (component-normalized)"
                                         : "1/3 (unchanged NWChem convention)");
      check(fabs(offAxis - expectedOffAxis) < 1.0e-8, what);

      delete dang;
    }
    delete dcfg;
  }

  printf("%s\n", failures ? "FAIL" : "PASS");
  return failures ? 1 : 0;
}

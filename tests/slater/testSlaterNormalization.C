// Regression test for #128: SlaterBasisSet::numericalBasis() must write
// coefficients that survive ComputeMoCmd::normalize()'s NORMP step (now
// moved, verbatim, to BasisFlatten::normalize()) unchanged in SHAPE.
//
// SlaterExpansion::fit() returns coefficients for UNNORMALISED primitives
// (r^l exp(-a r^2)).  NORMP, applied to every NumericalBasis entry
// downstream, treats the coefficient as belonging to the library
// convention (a NORMALISED primitive) and divides it by the primitive's
// own norm to undo that.  numericalBasis() must multiply by that same
// norm first, or the division happens twice and tight primitives get
// hugely over-weighted (~137x for C 2p, per the bug report) while the
// diffuse ones barely move -- a basis function that still looks smooth
// and plausible while being the wrong shape.
//
// This test does NOT use the "fit quality" SlaterExpansion::fit() already
// reports (see testSlaterExpansion.C) -- that number is computed from the
// RAW fit coefficients and can't see a bug introduced between fit() and
// what numericalBasis() actually writes to the file.  Instead it:
//
//   1. Calls the real SlaterBasisSet::numericalBasis() for C and H and
//      parses the NumericalBasis text it produces (this is the file
//      ECCE's importer round-trips through TGBSConfig at runtime).
//   2. Applies the library-convention transform EXACTLY as
//      BasisFlatten::normalize() does: NORMP (divide by the primitive's
//      own norm) then NORMF (renormalise the contracted function to unit
//      self-overlap) -- reimplemented here rather than linked, so this
//      stays the independent oracle the CLAUDE.md rule about verification
//      asks for, not a check that only re-confirms the production code
//      agrees with itself.
//   3. Evaluates the resulting radial function against the EXACT,
//      normalised Slater radial function r^(n-1) exp(-zeta r) (the same
//      target SlaterExpansion::fitFixed() fits against) on a grid, and
//      requires overlap >= 0.9999.
//
// Compiled and run directly with g++ against include/ + the two .C files
// it needs -- no wx, no DAV, no XML, so no build tree required (mirrors
// testSlaterExpansion.C / testSlaterBasisSet.C in this directory).

#include <cstdio>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#include "tdat/SlaterBasisSet.H"

using namespace std;

namespace {

const double PI = 3.14159265358979323846;
const int GRID_POINTS = 4000;
const double GRID_EXTENT = 12.0;   // in units of 1/zeta, matches fitFixed()

struct Shell {
  string element;
  int l;
  vector<double> exponents, coefficients;
};

//  Parse the "NumericalBasis" text numericalBasis() wrote: blocks of
//  "<Elem>    <S|P|D>" followed by "<exponent> <coefficient>" rows.
vector<Shell> parseNumericalBasis(const string& text)
{
  vector<Shell> shells;
  istringstream in(text);
  string line;
  Shell* cur = 0;
  while (getline(in, line)) {
    istringstream ls(line);
    string a, b;
    if (!(ls >> a)) continue;
    if (a == "NumericalBasis" || a == "END" || a == "EndNumericalBasis")
      continue;
    if (a == "basis") continue;
    if (ls >> b && (b == "S" || b == "P" || b == "D") &&
        !isdigit((unsigned char)a[0]) && a[0] != '-') {
      shells.push_back(Shell());
      cur = &shells.back();
      cur->element = a;
      cur->l = (b == "S") ? 0 : (b == "P") ? 1 : 2;
      continue;
    }
    //  A data row: "<exponent> <coefficient>".
    if (cur) {
      istringstream ds(line);
      double e, c;
      if (ds >> e >> c) {
        cur->exponents.push_back(e);
        cur->coefficients.push_back(c);
      }
    }
  }
  return shells;
}

//  BasisFlatten::normalize()'s NORMP step, reimplemented: undo the
//  "coefficient belongs to a normalised primitive" assumption.
double primitiveNorm(double ee, int l)
{
  const double facs = pow(PI, 1.5) / (ee * sqrt(ee));
  switch (l) {
    case 0: return sqrt(facs);
    case 1: return sqrt(0.5 * facs / ee);
    case 2: return sqrt(0.75 * facs / (ee * ee));
    default: return sqrt(facs);   // not needed for this test
  }
}

//  Apply NORMP then NORMF to a shell's coefficients, exactly as
//  BasisFlatten::normalize() does downstream of the .gbs/NumericalBasis
//  text at runtime.
vector<double> libraryConventionRoundTrip(const Shell& shell)
{
  const size_t n = shell.exponents.size();
  vector<double> c(n);
  for (size_t i = 0; i < n; i++) {
    const double ee = 2.0 * shell.exponents[i];
    c[i] = shell.coefficients[i] / primitiveNorm(ee, shell.l);
  }
  //  NORMF: renormalise the contracted function to unit self-overlap.
  double s = 0.0;
  for (size_t i = 0; i < n; i++) {
    for (size_t j = 0; j < n; j++) {
      const double e = shell.exponents[i] + shell.exponents[j];
      const double f = e * sqrt(e);
      double d = c[i] * c[j] / f;
      if (shell.l == 1) d /= (2.0 * e);
      s += d;
    }
  }
  const double snorm = 1.0 / sqrt(s * pow(PI, 1.5));
  for (size_t i = 0; i < n; i++) c[i] *= snorm;
  return c;
}

//  Overlap of the round-tripped contraction against the exact, normalised
//  Slater radial function r^(n-1) exp(-zeta r) -- the same target
//  SlaterExpansion::fitFixed() fits against (see SlaterExpansion.C).
double overlapWithExact(const Shell& shell, const vector<double>& c,
                        int n, double zeta)
{
  const double rmax = GRID_EXTENT / zeta;
  const double dr = rmax / GRID_POINTS;
  double num = 0.0, fitNorm = 0.0, targetNorm = 0.0;
  for (int p = 1; p <= GRID_POINTS; p++) {
    const double r = p * dr;
    const double r2 = r * r;
    const double w = r2 * dr;

    double rl = 1.0;
    for (int k = 0; k < shell.l; k++) rl *= r;
    double rn = 1.0;
    for (int k = 0; k < n - 1; k++) rn *= r;

    double value = 0.0;
    for (size_t i = 0; i < c.size(); i++)
      value += c[i] * rl * exp(-shell.exponents[i] * r2);

    const double target = rn * exp(-zeta * r);
    num += value * target * w;
    fitNorm += value * value * w;
    targetNorm += target * target * w;
  }
  if (fitNorm <= 0.0 || targetNorm <= 0.0) return -1.0;
  return num / sqrt(fitNorm * targetNorm);
}

}  // namespace

int main()
{
  vector<SlaterBasisSet::Element> els;
  SlaterBasisSet::Element c; c.symbol = "C"; c.atomicNumber = 6;
  c.zetaS = 1.9422440; c.zetaP = 1.7087230; c.zetaD = 0.0;
  els.push_back(c);
  SlaterBasisSet::Element h; h.symbol = "H"; h.atomicNumber = 1;
  h.zetaS = 1.2602370; h.zetaP = 0.0; h.zetaD = 0.0;
  els.push_back(h);

  string text;
  if (!SlaterBasisSet::numericalBasis(els, 6, text)) {
    printf("FAIL: numericalBasis() build failed\n");
    return 1;
  }

  vector<Shell> shells = parseNumericalBasis(text);

  struct Want { const char* element; int l; int n; double zeta;
               const char* label; };
  Want wants[] = {
    { "H", 0, 1, 1.2602370, "H  1s" },
    { "C", 0, 2, 1.9422440, "C  2s" },
    { "C", 1, 2, 1.7087230, "C  2p" },
  };

  int bad = 0;
  printf("  shell   n l   zeta      overlap after library-convention "
        "round-trip\n");
  for (size_t w = 0; w < sizeof(wants)/sizeof(wants[0]); w++) {
    const Shell* found = 0;
    for (size_t s = 0; s < shells.size(); s++) {
      if (shells[s].element == wants[w].element &&
          shells[s].l == wants[w].l) {
        found = &shells[s];
        break;
      }
    }
    if (!found) {
      printf("  %-8s FAIL: shell not found in generated NumericalBasis\n",
            wants[w].label);
      bad++;
      continue;
    }
    vector<double> c = libraryConventionRoundTrip(*found);
    double ov = overlapWithExact(*found, c, wants[w].n, wants[w].zeta);
    printf("  %-8s %d %d %7.4f   %.6f%s\n", wants[w].label, wants[w].n,
          wants[w].l, wants[w].zeta, ov, ov < 0.9999 ? "   <-- FAIL" : "");
    if (ov < 0.9999) bad++;
  }

  printf("  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}

//  #170 -- Lowdin (symmetrically-orthogonalised) per-atom shares,
//  against a Mulliken share's known failure mode.
//
//  A Mulliken per-atom share is not bounded to [0,1]: for an
//  antibonding orbital one atom's share can exceed 1 while another
//  goes negative (confirmed against ORCA's own Mulliken printout for
//  CO's 6-sigma*, #170). MoDiagram::classify() compares this share
//  against a fixed threshold to decide "this level is a lone pair",
//  so an unbounded measure can trip that threshold for an orbital
//  that plainly is not localised. This checks the replacement
//  (MoComposition::lowdinShares(), built from MoComposition::
//  buildSqrtOverlap()) against hand-derived numbers for a simple
//  two-function system, and separately reproduces the Mulliken
//  failure mode via the existing MoComposition::compute() to prove
//  the two measures really do disagree the way #170 says.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
using namespace std;

#include "tdat/MoComposition.H"
#include "tdat/MoFragments.H"

static int bad = 0;

static void check(const char* what, bool ok, const char* detail = "")
{
  printf("  %-58s %s %s\n", what, ok ? "ok" : "FAIL", detail);
  if (!ok) bad++;
}

static bool near(double a, double b, double tol = 1.0e-6)
{
  return fabs(a - b) < tol;
}

int main(void)
{
  printf("\n  Lowdin composition (#170)\n");

  //  --- the moved eigensolver, on a textbook 2x2 -----------------------
  //  [[2,1],[1,2]] has eigenvalues 3 and 1 (eigenvectors (1,1)/sqrt2
  //  and (1,-1)/sqrt2) -- independent of anything else here, just
  //  confirming the moved routine still does what Huckel.C relies on.
  {
    vector< vector<double> > a(2, vector<double>(2));
    a[0][0] = 2; a[0][1] = 1; a[1][0] = 1; a[1][1] = 2;
    vector<double> values;
    vector< vector<double> > vectors;
    MoComposition::jacobiEigen(a, values, vectors);

    const bool haveThree = near(values[0], 3.0) || near(values[1], 3.0);
    const bool haveOne = near(values[0], 1.0) || near(values[1], 1.0);
    check("jacobiEigen: [[2,1],[1,2]] eigenvalues are 3 and 1",
         haveThree && haveOne);
  }

  //  --- two functions, two atoms, s = 0.4 ------------------------------
  //  Bonding c=(1,1)/sqrt(2+2s) and antibonding c=(1,-1)/sqrt(2-2s) are
  //  both EIGENVECTORS of S (eigenvalues 1+s and 1-s respectively), so
  //  S^{1/2} acting on either just rescales it -- c' is proportional to
  //  c itself, giving exactly 0.5/0.5 either way. Derived independently
  //  here (not via buildSqrtOverlap) as the oracle for the next block.
  {
    const double s = 0.4;
    const int nbasis = 2;
    vector<double> S(4);
    S[0] = 1.0; S[1] = s; S[2] = s; S[3] = 1.0;
    vector<int> perAtom(2, 1);

    vector<double> sqrtS;
    const bool built = MoComposition::buildSqrtOverlap(S, nbasis, sqrtS);
    check("buildSqrtOverlap succeeds for a well-conditioned 2x2", built);

    if (built) {
      //  S^{1/2} itself, from the closed form (eigenvalues 1+-s,
      //  eigenvectors (1,1)/sqrt2 and (1,-1)/sqrt2) -- the independent
      //  check that buildSqrtOverlap computed the right matrix, not
      //  just SOME symmetric matrix.
      const double sp = sqrt(1.0+s), sm = sqrt(1.0-s);
      const double expect00 = 0.5*(sp+sm);
      const double expect01 = 0.5*(sp-sm);
      check("S^1/2 diagonal matches the closed form",
           near(sqrtS[0], expect00) && near(sqrtS[3], expect00));
      check("S^1/2 off-diagonal matches the closed form",
           near(sqrtS[1], expect01) && near(sqrtS[2], expect01));

      const double normBond = sqrt(2.0+2.0*s);
      vector<double> cBond(2);
      cBond[0] = 1.0/normBond; cBond[1] = 1.0/normBond;
      const vector<double> shareBond =
          MoComposition::lowdinShares(cBond, perAtom, sqrtS);
      check("bonding: Lowdin shares are 0.5/0.5",
           shareBond.size() == 2 && near(shareBond[0], 0.5) &&
           near(shareBond[1], 0.5));

      const double normAnti = sqrt(2.0-2.0*s);
      vector<double> cAnti(2);
      cAnti[0] = 1.0/normAnti; cAnti[1] = -1.0/normAnti;
      const vector<double> shareAnti =
          MoComposition::lowdinShares(cAnti, perAtom, sqrtS);
      check("antibonding: Lowdin shares are ALSO 0.5/0.5",
           shareAnti.size() == 2 && near(shareAnti[0], 0.5) &&
           near(shareAnti[1], 0.5));
    }
  }

  //  --- the asymmetric antibonding case: Mulliken exceeds 1, ----------
  //  Lowdin does not. c = (1.0, -0.3), s = 0.5, hand-derived:
  //    Mulliken atom0 share = 0.85/0.79 = 1.0759... (> 1)
  //    Mulliken atom1 share = -0.06/0.79 = -0.0759... (< 0)
  //    Lowdin shares = 0.99879 / 0.00121 (both in [0,1], sum to 1)
  {
    const double s = 0.5;
    const int nbasis = 2;
    vector<double> S(4);
    S[0] = 1.0; S[1] = s; S[2] = s; S[3] = 1.0;
    vector<int> perAtom(2, 1);
    vector<int> noShellSplit;
    vector<string> elements(2, "X");

    vector<double> c(2);
    c[0] = 1.0; c[1] = -0.3;

    //  MoFragments::shareOfIndices() directly, not MoComposition::
    //  compute() -- compute() silently DROPS a negative share
    //  (`if (s < 0.0) continue;`), which is itself part of what makes
    //  a Mulliken share unsafe to build a bounded measure from, but
    //  it means compute() can't be used to exhibit the negative value.
    vector<int> onAtom0(1, 0), onAtom1(1, 1);
    const double atom0 = MoFragments::shareOfIndices(c, onAtom0, S);
    const double atom1 = MoFragments::shareOfIndices(c, onAtom1, S);
    char detail[128];
    snprintf(detail, sizeof(detail), "(atom0=%.4f atom1=%.4f)", atom0, atom1);
    check("Mulliken: atom0 share exceeds 1.0", atom0 > 1.0, detail);
    check("Mulliken: atom1 share is negative", atom1 < 0.0, detail);
    check("Mulliken: hand-derived value (0.85/0.79)",
         near(atom0, 0.85/0.79, 1.0e-4));

    //  And confirm MoComposition::compute() -- what the diagram panel
    //  actually called before #170 -- really does drop that negative
    //  share rather than reporting it, so a caller reading its output
    //  alone would never even see the sign that shows the measure is
    //  unbounded.
    const vector<MoComposition::Share> mulliken =
        MoComposition::compute(c, perAtom, noShellSplit, elements, S);
    check("MoComposition::compute() drops the negative share",
         mulliken.size() == 1 && mulliken[0].atom == 0);

    vector<double> sqrtS;
    const bool built = MoComposition::buildSqrtOverlap(S, nbasis, sqrtS);
    check("buildSqrtOverlap succeeds for this S too", built);
    if (built) {
      const vector<double> lowdin =
          MoComposition::lowdinShares(c, perAtom, sqrtS);
      check("Lowdin returns two shares", lowdin.size() == 2);
      if (lowdin.size() == 2) {
        char detail[128];
        snprintf(detail, sizeof(detail), "(atom0=%.4f atom1=%.4f)",
                lowdin[0], lowdin[1]);
        check("Lowdin: both shares stay in [0,1]",
             lowdin[0] >= 0.0 && lowdin[0] <= 1.0 &&
             lowdin[1] >= 0.0 && lowdin[1] <= 1.0, detail);
        check("Lowdin: shares sum to 1", near(lowdin[0]+lowdin[1], 1.0));
        check("Lowdin: hand-derived value (~0.9988/0.0012)",
             near(lowdin[0], 0.99879, 1.0e-4) &&
             near(lowdin[1], 0.00121, 1.0e-4), detail);
      }
    }
  }

  //  --- degenerate/ill-conditioned overlap: buildSqrtOverlap declines --
  {
    vector<double> S(4);
    S[0] = 1.0; S[1] = 1.0; S[2] = 1.0; S[3] = 1.0;  // singular, s=1
    vector<double> sqrtS;
    const bool built = MoComposition::buildSqrtOverlap(S, 2, sqrtS);
    check("buildSqrtOverlap declines a linearly dependent basis", !built);
  }

  printf("  %d failed\n", bad);
  return bad == 0 ? 0 : 1;
}

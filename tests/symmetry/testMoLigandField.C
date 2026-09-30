//  The ligand-field model's own linear algebra, checked against
//  synthetic cases with a KNOWN answer -- not against Cr(CO)6, which
//  tests/modiagram's capture.py covers separately against a real
//  calculation. Two things, per the brief that asked for this file:
//
//    1. Fock reconstruction: F = S C diag(eps) C^T S must satisfy
//       C^T F C = diag(eps) for the very C/eps it was built from, and
//       (when C is a COMPLETE S-orthonormal basis, not just some of
//       it) must reproduce the original F exactly.
//    2. generalizedEigen() against a textbook two-site Huckel problem,
//       whose bonding/antibonding energies have a closed form.
#include <cstdio>
#include <cmath>
#include <vector>
using namespace std;

#include "tdat/MoLigandField.H"

static int bad = 0;

static void checkd(const char* what, double got, double want, double tol) {
  const bool ok = fabs(got - want) <= tol;
  printf("  %-46s %-14.6f %-14.6f %s\n", what, got, want, ok ? "ok" : "FAIL");
  if (!ok) bad++;
}


int main() {
  printf("  %-46s %-14s %-14s\n", "", "got", "expected");

  //  --- 1. Fock reconstruction, then C^T F C == diag(eps) -------------
  //
  //  A non-orthogonal 3-function basis (S is not the identity, the case
  //  that matters: a Gaussian AO basis never is one) with an arbitrary
  //  symmetric F0. generalizedEigen() itself is not under test here --
  //  it is only used to produce a genuine S-orthonormal eigenbasis of
  //  F0 to reconstruct FROM, so that "reproduces F0 exactly" is a
  //  meaningful check and not circular.
  {
    const int n = 3;
    const double S[9] = { 1.0, 0.2, 0.1,
                          0.2, 1.0, 0.15,
                          0.1, 0.15, 1.0 };
    const double F0[9] = { -1.0,  0.3, 0.05,
                            0.3, -0.5, 0.2,
                           0.05,  0.2, 0.1 };
    vector<double> Sv(S, S + 9), F0v(F0, F0 + 9);

    vector<double> eps;
    vector< vector<double> > vecs;
    MoLigandField::generalizedEigen(F0v, Sv, n, eps, vecs);
    printf("  eigenpairs found: %d (want 3)\n", (int)eps.size());
    if ((int)eps.size() != 3) bad++;

    //  Orthonormality under S -- the property buildFock()'s exactness
    //  relies on, checked independently before trusting the rest.
    for (size_t i = 0; i < vecs.size(); i++) {
      for (size_t j = 0; j < vecs.size(); j++) {
        double sij = 0.0;
        for (int mu = 0; mu < n; mu++)
          for (int nu = 0; nu < n; nu++) sij += vecs[i][mu]*Sv[mu*n+nu]*vecs[j][nu];
        checkd((i == j) ? "C^T S C diagonal == 1" : "C^T S C off-diagonal == 0",
               sij, (i == j) ? 1.0 : 0.0, 1.0e-8);
      }
    }

    vector<double> fock;
    MoLigandField::buildFock(vecs, eps, Sv, n, fock);

    //  A COMPLETE S-orthonormal basis (n MOs for an n-function basis):
    //  buildFock() must reproduce F0 exactly, not just be consistent
    //  with eps on this basis -- this is the real invariant, C^T F C
    //  == diag(eps) alone would also hold for a wrong F that happened
    //  to share eigenvectors.
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        char label[64];
        snprintf(label, sizeof(label), "F reconstructed [%d][%d]", i, j);
        checkd(label, fock[i*n+j], F0v[i*n+j], 1.0e-8);
      }
    }

    //  C^T F C == diag(eps), the check the header promises -- run
    //  against the RECONSTRUCTED fock, independently of the exact-F0
    //  check above (a transcription slip in buildFock() that still
    //  happened to diagonalise correctly would not be caught by the
    //  first check alone if eps/vecs were wrong in a compensating way).
    for (size_t i = 0; i < vecs.size(); i++) {
      for (size_t j = 0; j < vecs.size(); j++) {
        double fij = 0.0;
        for (int mu = 0; mu < n; mu++)
          for (int nu = 0; nu < n; nu++) fij += vecs[i][mu]*fock[mu*n+nu]*vecs[j][nu];
        checkd((i == j) ? "C^T F C diagonal == eps" : "C^T F C off-diagonal == 0",
               fij, (i == j) ? eps[i] : 0.0, 1.0e-8);
      }
    }
  }

  //  --- 1b. Incomplete C (nMO < nbasis): still valid on that span -----
  //
  //  Only the lowest of the 3 eigenvectors above.  buildFock() cannot
  //  reproduce F0 now (it has no information at all about the
  //  directions the other two eigenvectors covered), but restricted to
  //  the ONE orbital it was given, C^T F C == eps must still hold
  //  exactly -- "still valid on the MO span" is the header's own claim
  //  and this is what it means.
  {
    const int n = 3;
    const double S[9] = { 1.0, 0.2, 0.1,
                          0.2, 1.0, 0.15,
                          0.1, 0.15, 1.0 };
    const double F0[9] = { -1.0,  0.3, 0.05,
                            0.3, -0.5, 0.2,
                           0.05,  0.2, 0.1 };
    vector<double> Sv(S, S + 9), F0v(F0, F0 + 9);
    vector<double> eps;
    vector< vector<double> > vecs;
    MoLigandField::generalizedEigen(F0v, Sv, n, eps, vecs);

    vector<double> oneEps(1, eps[0]);
    vector< vector<double> > oneVec(1, vecs[0]);
    vector<double> fock;
    MoLigandField::buildFock(oneVec, oneEps, Sv, n, fock);

    double fii = 0.0;
    for (int mu = 0; mu < n; mu++)
      for (int nu = 0; nu < n; nu++) fii += vecs[0][mu]*fock[mu*n+nu]*vecs[0][nu];
    checkd("incomplete C: C^T F C == eps on its own span", fii, eps[0], 1.0e-8);
  }

  //  --- 2. generalizedEigen() against a textbook two-site problem -----
  //
  //  Two equivalent sites (a1 == a2 == a), coupling f, mutual overlap
  //  s: the bonding/antibonding energies of a symmetric Huckel-style
  //  2x2 problem have the closed form e = (a +/- f) / (1 +/- s) --
  //  any general chemistry text's treatment of a homonuclear diatomic
  //  with overlap (e.g. H2+) gives this. f < 0 (attractive, as usual)
  //  puts the bonding (+f, lower denominator 1+s) level below a.
  {
    const int n = 2;
    const double a = -0.5, f = -0.3, s = 0.25;
    const double S[4] = { 1.0, s, s, 1.0 };
    const double F[4] = { a, f, f, a };
    vector<double> Sv(S, S + 4), Fv(F, F + 4);

    vector<double> eps;
    vector< vector<double> > vecs;
    const int kept = MoLigandField::generalizedEigen(Fv, Sv, n, eps, vecs);
    printf("  modes kept: %d (want 2)\n", kept);
    if (kept != 2) bad++;

    const double bonding = (a + f)/(1.0 + s);
    const double antibonding = (a - f)/(1.0 - s);
    checkd("bonding energy (lower)", eps[0], bonding, 1.0e-8);
    checkd("antibonding energy (upper)", eps[1], antibonding, 1.0e-8);

    //  The bonding eigenvector is the SYMMETRIC combination (equal
    //  sign on both sites); check the ratio, not the raw components,
    //  since the overall sign/normalisation is arbitrary.
    checkd("bonding mode is symmetric (c1/c2 == 1)",
           vecs[0][0]/vecs[0][1], 1.0, 1.0e-6);
    checkd("antibonding mode is antisymmetric (c1/c2 == -1)",
           vecs[1][0]/vecs[1][1], -1.0, 1.0e-6);
  }

  //  --- 2b. A near-singular overlap is handled, not blown up ----------
  //
  //  Two basis functions almost parallel (S close to singular): the
  //  linear-dependence floor must drop the redundant mode rather than
  //  dividing by ~0 in S^{-1/2} and returning garbage.
  {
    const int n = 2;
    const double S[4] = { 1.0, 0.999999999, 0.999999999, 1.0 };
    const double F[4] = { -0.5, -0.3, -0.3, -0.5 };
    vector<double> Sv(S, S + 4), Fv(F, F + 4);
    vector<double> eps;
    vector< vector<double> > vecs;
    const int kept = MoLigandField::generalizedEigen(Fv, Sv, n, eps, vecs, 1.0e-6);
    printf("  modes kept from a near-singular S: %d (want 1)\n", kept);
    if (kept != 1) bad++;
    //  The redundant (near-singular) direction is dropped, leaving only
    //  the well-conditioned SYMMETRIC combination: e = (F11+F12)/(1+s),
    //  s approx 1 here, so approx (-0.5-0.3)/2 = -0.4.
    if (!eps.empty()) checkd("the one kept mode == (F11+F12)/(1+s)", eps[0], -0.4, 1.0e-6);
  }

  if (bad) {
    printf("\nFAILED  %d check(s)\n", bad);
    return 1;
  }
  printf("\nPASSED\n");
  return 0;
}

//  The ESP assembly checked against cases whose answer is known.
//
//  The integrals themselves are covered by testCoulombIntegrals.  What
//  is checked here is the assembly around them: the SIGN of the
//  electronic term, the factor of two on off-diagonal density matrix
//  elements, and the basis normalisation.  Get any of those wrong and
//  the surface is smooth, plausible, and wrong -- there is nothing
//  downstream that would notice.
//
//  The anchor is charge neutrality.  Far from a molecule the potential
//  must approach (total nuclear charge - number of electrons)/R,
//  whatever the shape of the density, so it is an independent check
//  that the two terms are on the same scale and opposite in sign.
#include <cstdio>
#include <cmath>
#include <vector>
#include "tdat/EspField.H"
using namespace std;

static const double PI = 3.14159265358979323846;
static int bad = 0;

static void check(const char* what, double got, double want, double tol) {
  bool ok = fabs(got-want) <= tol*(1.0 + fabs(want));
  printf("  %-54s %14.9f %14.9f  %s\n", what, got, want, ok ? "ok" : "FAIL");
  if (!ok) bad++;
}

/** A normalised 1s Gaussian: integral of its square is one. */
static EspBasisFunction s1(double alpha, double x, double y, double z) {
  EspBasisFunction f;
  f.center[0] = x; f.center[1] = y; f.center[2] = z;
  f.powerX.push_back(0); f.powerY.push_back(0); f.powerZ.push_back(0);
  f.angularCoef.push_back(1.0);
  f.exponent.push_back(alpha);
  f.contraction.push_back(pow(2.0*alpha/PI, 0.75));
  return f;
}

int main() {
  const double alpha = 0.9;

  printf("  %-54s %14s %14s\n", "", "got", "expected");

  //  ---- one atom, one s function ----
  vector<EspBasisFunction> basis;
  basis.push_back(s1(alpha, 0, 0, 0));

  vector<EspNucleus> nuclei(1);
  nuclei[0].center[0] = nuclei[0].center[1] = nuclei[0].center[2] = 0.0;

  const double R = 60.0;
  double far[3] = {R, 0, 0};

  //  Neutral: one proton, one electron.  The potential must vanish.
  {
    vector<double> P(1, 1.0);
    EspField::Pairs pairs;
    EspField::selectPairs(basis, P, 1e-12, pairs);
    nuclei[0].charge = 1.0;
    check("neutral H atom, far field -> 0",
          EspField::potential(basis, pairs, nuclei, far), 0.0, 1e-9);
    //  The electronic term alone is one electron's worth, and negative.
    check("  its electronic term alone is -1/R",
          EspField::electronicPotential(basis, pairs, far), -1.0/R, 1e-9);
  }
  //  A cation: no electrons at all.
  {
    vector<double> P(1, 0.0);
    EspField::Pairs pairs;
    EspField::selectPairs(basis, P, 1e-12, pairs);
    nuclei[0].charge = 1.0;
    check("bare proton, far field -> +1/R",
          EspField::potential(basis, pairs, nuclei, far), 1.0/R, 1e-9);
  }
  //  An anion: one proton, two electrons.
  {
    vector<double> P(1, 2.0);
    EspField::Pairs pairs;
    EspField::selectPairs(basis, P, 1e-12, pairs);
    nuclei[0].charge = 1.0;
    check("hydride, one proton two electrons -> -1/R",
          EspField::potential(basis, pairs, nuclei, far), -1.0/R, 1e-9);
  }
  //  Helium: two protons, two electrons -- neutral, like H, but
  //  reaching zero by cancelling larger numbers.
  {
    vector<double> P(1, 2.0);
    EspField::Pairs pairs;
    EspField::selectPairs(basis, P, 1e-12, pairs);
    nuclei[0].charge = 2.0;
    check("helium, two protons two electrons -> 0",
          EspField::potential(basis, pairs, nuclei, far), 0.0, 1e-9);
  }
  //  He+: two protons, one electron.
  {
    vector<double> P(1, 1.0);
    EspField::Pairs pairs;
    EspField::selectPairs(basis, P, 1e-12, pairs);
    nuclei[0].charge = 2.0;
    check("He+, two protons one electron -> +1/R",
          EspField::potential(basis, pairs, nuclei, far), 1.0/R, 1e-9);
  }

  //  ---- two atoms: the off-diagonal factor of two ----
  //
  //  Two normalised s functions 1.4 bohr apart with density matrix
  //  [[1,1],[1,1]].  The electron count is trace(P S) = 2 + 2S, where S
  //  is their overlap -- so if the off-diagonal weight were counted
  //  once instead of twice, the far field would be off by exactly S/R.
  {
    vector<EspBasisFunction> h2;
    h2.push_back(s1(alpha, 0, 0, 0));
    h2.push_back(s1(alpha, 1.4, 0, 0));

    vector<double> P(4, 1.0);
    EspField::Pairs pairs;
    EspField::selectPairs(h2, P, 1e-12, pairs);

    //  Overlap of two normalised s Gaussians, in closed form.
    const double d2 = 1.4*1.4;
    const double S = pow(PI/(2.0*alpha), 1.5)
                     * exp(-alpha*alpha/(2.0*alpha)*d2)
                     * pow(2.0*alpha/PI, 1.5);
    const double electrons = 2.0 + 2.0*S;

    vector<EspNucleus> two(2);
    two[0].center[0] = 0.0; two[0].center[1] = two[0].center[2] = 0.0;
    two[1].center[0] = 1.4; two[1].center[1] = two[1].center[2] = 0.0;
    two[0].charge = two[1].charge = 1.0;

    double veryFar[3] = {4000.0, 0, 0};
    check("H2-like, far field -> (2 - trace(PS))/R",
          EspField::potential(h2, pairs, two, veryFar),
          (2.0 - electrons)/4000.0, 1e-6);

    //  And the pair list must actually have folded the two off-diagonal
    //  terms into one doubled entry.
    check("three pairs kept, not four", (double)pairs.size(), 3.0, 1e-12);
    double total = 0.0;
    for (size_t i = 0; i < pairs.size(); i++) total += pairs.weight[i];
    check("  their weights sum to 4", total, 4.0, 1e-12);
  }

  //  ---- the density helper must agree with the same pair list ----
  {
    vector<EspBasisFunction> h2;
    h2.push_back(s1(alpha, 0, 0, 0));
    h2.push_back(s1(alpha, 1.4, 0, 0));
    vector<double> P(4, 1.0);
    EspField::Pairs pairs;
    EspField::selectPairs(h2, P, 1e-12, pairs);

    //  Directly, without the pair machinery.
    double direct = 0.0;
    for (int mu = 0; mu < 2; mu++)
      for (int nu = 0; nu < 2; nu++)
        direct += P[mu*2+nu]*h2[mu].value(0.7, 0.2, 0.0)
                            *h2[nu].value(0.7, 0.2, 0.0);
    check("density from pairs matches the plain double sum",
          EspField::density(h2, pairs, 0.7, 0.2, 0.0), direct, 1e-12);
  }

  //  ---- the cutoff must drop pairs, and keep the diagonal ----
  {
    vector<EspBasisFunction> h2;
    h2.push_back(s1(alpha, 0, 0, 0));
    h2.push_back(s1(alpha, 40.0, 0, 0));
    vector<double> P(4, 0.0);
    P[0] = 1.0; P[3] = 1.0; P[1] = P[2] = 1e-14;
    EspField::Pairs pairs;
    EspField::selectPairs(h2, P, 1e-8, pairs);
    check("a negligible off-diagonal is dropped", (double)pairs.size(), 2.0, 1e-12);
  }

  printf("\n  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}

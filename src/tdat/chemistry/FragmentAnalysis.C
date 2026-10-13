#include <algorithm>
#include <cmath>
#include <cstdio>

#include "tdat/FragmentAnalysis.H"

typedef vector<double> Vec;

static string fmt(const char *f, double a, double b = 0.0, double c = 0.0)
{
  char buf[256];
  snprintf(buf, sizeof buf, f, a, b, c);
  return buf;
}


bool FragmentAnalysis::embed(const FragmentParent& parent,
                             const FragmentOrbitalSet& frag, Vec& columns,
                             string& problem)
{
  const int n = parent.nAO;
  const int norb = (int)frag.coefficients.size();
  if (!frag.sameBasis) {
    problem = frag.name + ": a fragment in a different basis than the parent "
              "needs the projection through the parent overlap, which is not "
              "implemented";
    return false;
  }
  if ((int)frag.parentIndex.size() != frag.nAO) {
    problem = frag.name + ": no mapping of its basis functions onto the "
              "parent's";
    return false;
  }
  for (int k = 0; k < frag.nAO; k++)
    if (frag.parentIndex[k] < 0 || frag.parentIndex[k] >= n) {
      problem = frag.name + ": basis function mapped outside the parent basis";
      return false;
    }
  columns.assign((size_t)n * norb, 0.0);
  for (int j = 0; j < norb; j++) {
    if ((int)frag.coefficients[j].size() != frag.nAO) {
      problem = frag.name + ": an orbital has the wrong number of coefficients";
      return false;
    }
    for (int k = 0; k < frag.nAO; k++)
      columns[(size_t)frag.parentIndex[k] * norb + j] = frag.coefficients[j][k];
  }
  return true;
}


//  In-place Cholesky factor G = L L'; false if not positive definite.
static bool cholesky(Vec& g, int n)
{
  for (int j = 0; j < n; j++) {
    double d = g[(size_t)j*n + j];
    for (int k = 0; k < j; k++) d -= g[(size_t)j*n + k] * g[(size_t)j*n + k];
    if (d <= 1e-12) return false;
    d = sqrt(d);
    g[(size_t)j*n + j] = d;
    for (int i = j + 1; i < n; i++) {
      double s = g[(size_t)i*n + j];
      for (int k = 0; k < j; k++) s -= g[(size_t)i*n + k] * g[(size_t)j*n + k];
      g[(size_t)i*n + j] = s / d;
    }
  }
  return true;
}

static void cholSolve(const Vec& l, int n, Vec& x)
{
  for (int i = 0; i < n; i++) {
    double s = x[i];
    for (int k = 0; k < i; k++) s -= l[(size_t)i*n + k] * x[k];
    x[i] = s / l[(size_t)i*n + i];
  }
  for (int i = n - 1; i >= 0; i--) {
    double s = x[i];
    for (int k = i + 1; k < n; k++) s -= l[(size_t)k*n + i] * x[k];
    x[i] = s / l[(size_t)i*n + i];
  }
}


bool FragmentAnalysis::run(const FragmentParent& parent,
                           const vector<FragmentOrbitalSet>& frags,
                           double tol, string& problem)
{
  orbitals.clear(); coefficient.clear(); mulliken.clear(); scpa.clear();
  capture.clear(); perFragment.clear(); checks = Checks();
  allSameBasis = true;
  problem.clear();

  const int n = parent.nAO;
  const size_t nmo = parent.coefficients.size();
  if (n <= 0 || (int)parent.overlap.size() != n * n || nmo == 0) {
    problem = "the parent has no overlap matrix or no orbitals";
    return false;
  }
  for (size_t m = 0; m < nmo; m++)
    if ((int)parent.coefficients[m].size() != n) {
      problem = "a parent orbital has the wrong number of coefficients";
      return false;
    }
  if (frags.empty()) { problem = "no fragments"; return false; }

  //  T: parent AO x all fragment orbitals.
  vector<int> start;
  int nfo = 0;
  for (size_t f = 0; f < frags.size(); f++) {
    start.push_back(nfo);
    nfo += (int)frags[f].coefficients.size();
    if (!frags[f].sameBasis) allSameBasis = false;
  }
  start.push_back(nfo);
  Vec T((size_t)n * nfo, 0.0);
  vector<bool> claimed((size_t)n, false);
  for (size_t f = 0; f < frags.size(); f++) {
    Vec cols;
    if (!embed(parent, frags[f], cols, problem)) return false;
    if (frags[f].sameBasis)
      for (int k = 0; k < frags[f].nAO; k++) {
        if (claimed[frags[f].parentIndex[k]]) {
          problem = frags[f].name + ": shares parent atoms with an earlier "
                    "fragment; fragments must not overlap";
          return false;
        }
        claimed[frags[f].parentIndex[k]] = true;
      }
    const int no = start[f+1] - start[f];
    for (int mu = 0; mu < n; mu++)
      for (int j = 0; j < no; j++)
        T[(size_t)mu*nfo + start[f] + j] = cols[(size_t)mu*no + j];
    const FragmentOrbitalSet& fs = frags[f];
    for (int j = 0; j < no; j++) {
      Orbital o;
      o.fragment = (int)f; o.index = j;
      o.energy = j < (int)fs.energies.size() ? fs.energies[j] : 0.0;
      o.occupation = j < (int)fs.occupations.size() ? fs.occupations[j] : 0.0;
      orbitals.push_back(o);
    }
  }

  const Vec& S = parent.overlap;
  Vec ST((size_t)n * nfo, 0.0);                 // S T
  for (int mu = 0; mu < n; mu++)
    for (int nu = 0; nu < n; nu++) {
      const double s = S[(size_t)mu*n + nu];
      if (s == 0.0) continue;
      for (int j = 0; j < nfo; j++)
        ST[(size_t)mu*nfo + j] += s * T[(size_t)nu*nfo + j];
    }
  Vec G((size_t)nfo * nfo, 0.0);                // T' S T
  for (int mu = 0; mu < n; mu++)
    for (int i = 0; i < nfo; i++) {
      const double t = T[(size_t)mu*nfo + i];
      if (t == 0.0) continue;
      for (int j = 0; j < nfo; j++)
        G[(size_t)i*nfo + j] += t * ST[(size_t)mu*nfo + j];
    }

  //  Each fragment's orbitals orthonormal in the embedded basis, and (same
  //  basis) its own overlap equal to the parent's block: a mismatch is a
  //  different geometry or basis, which no orbital check would reveal.
  bool ok = true;
  for (size_t f = 0; f < frags.size(); f++) {
    double worst = 0.0;
    for (int i = start[f]; i < start[f+1]; i++)
      for (int j = start[f]; j < start[f+1]; j++)
        worst = std::max(worst, fabs(G[(size_t)i*nfo + j] - (i == j ? 1.0 : 0.0)));
    checks.orthonormality.push_back(worst);
    if (worst > tol) {
      problem += frags[f].name + ": orbitals not orthonormal in the parent "
                 "basis, worst " + fmt("%.2e", worst) + " (limit " +
                 fmt("%.0e", tol) + ")\n";
      ok = false;
    }
    double mis = -1.0;
    const FragmentOrbitalSet& fs = frags[f];
    if (fs.sameBasis && (int)fs.selfOverlap.size() == fs.nAO * fs.nAO) {
      mis = 0.0;
      for (int a = 0; a < fs.nAO; a++)
        for (int b = 0; b < fs.nAO; b++)
          mis = std::max(mis, fabs(fs.selfOverlap[(size_t)a*fs.nAO + b] -
              S[(size_t)fs.parentIndex[a]*n + fs.parentIndex[b]]));
      if (mis > 100.0 * tol) {
        problem += frags[f].name + ": its own overlap matrix differs from the "
                   "parent's block by " + fmt("%.2e", mis) +
                   " (different geometry or basis?)\n";
        ok = false;
      }
    }
    checks.overlapMismatch.push_back(mis);
  }

  Vec L = G;
  if (!cholesky(L, nfo)) {
    problem += "the fragment orbitals are linearly dependent in the parent "
               "basis (overlapping fragments?)\n";
    return false;
  }

  for (size_t m = 0; m < nmo; m++) {
    const Vec& c = parent.coefficients[m];
    Vec Sc((size_t)n, 0.0);
    double norm = 0.0;
    for (int mu = 0; mu < n; mu++) {
      double s = 0.0;
      for (int nu = 0; nu < n; nu++) s += S[(size_t)mu*n + nu] * c[nu];
      Sc[mu] = s;
      norm += c[mu] * s;
    }
    checks.maxNormError = std::max(checks.maxNormError, fabs(norm - 1.0));

    Vec a((size_t)nfo, 0.0);                    // T' S c
    for (int mu = 0; mu < n; mu++)
      for (int j = 0; j < nfo; j++) a[j] += T[(size_t)mu*nfo + j] * Sc[mu];
    cholSolve(L, nfo, a);

    Vec mul((size_t)nfo, 0.0), sc((size_t)nfo, 0.0);
    double sumA2 = 0.0, cap = 0.0;
    for (int i = 0; i < nfo; i++) {
      double ga = 0.0;
      for (int j = 0; j < nfo; j++) ga += G[(size_t)i*nfo + j] * a[j];
      mul[i] = a[i] * ga;
      cap += mul[i];
      sumA2 += a[i] * a[i];
    }
    for (int i = 0; i < nfo; i++) sc[i] = a[i] * a[i] / sumA2;
    Vec per(frags.size(), 0.0);
    for (size_t f = 0; f < frags.size(); f++)
      for (int j = start[f]; j < start[f+1]; j++) per[f] += mul[j];

    coefficient.push_back(a);
    mulliken.push_back(mul);
    scpa.push_back(sc);
    perFragment.push_back(per);
    capture.push_back(cap / norm);
    if (allSameBasis)
      checks.maxCaptureError = std::max(checks.maxCaptureError,
                                        fabs(cap / norm - 1.0));
  }
  if (checks.maxNormError > tol) {
    problem += "a parent MO is not normalised in the stored basis, worst " +
               fmt("%.2e", checks.maxNormError) + " (limit " +
               fmt("%.0e", tol) + "): coefficients, basis order or geometry "
               "do not match\n";
    ok = false;
  }
  if (allSameBasis && checks.maxCaptureError > tol) {
    problem += "the fragment orbitals do not reproduce the parent MOs: "
               "summed composition off by " + fmt("%.2e", checks.maxCaptureError) +
               " (limit " + fmt("%.0e", tol) + "); the fragments do not cover "
               "all parent atoms, or the basis differs\n";
    ok = false;
  }
  return ok;
}

#include "tdat/MoLigandField.H"
#include "tdat/MoComposition.H"
#include "tdat/MoFragments.H"

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <numeric>
#include <sstream>
using std::map;
using std::ostringstream;


//  --- small linear-algebra helpers, all row-major flat matrices ------
namespace {

vector< vector<double> > toMatrix(const vector<double>& flat, int n)
{
   vector< vector<double> > m(n, vector<double>(n, 0.0));
   for (int i = 0; i < n; i++)
      for (int j = 0; j < n; j++) m[i][j] = flat[(size_t)i*n + j];
   return m;
}

/** M * v, M flat n*n row major. */
vector<double> matVec(const vector<double>& flat, int n, const vector<double>& v)
{
   vector<double> out(n, 0.0);
   for (int i = 0; i < n; i++) {
      double sum = 0.0;
      for (int j = 0; j < n; j++) sum += flat[(size_t)i*n + j]*v[j];
      out[i] = sum;
   }
   return out;
}

/** a^T M b, all length n. */
double bilinear(const vector<double>& a, const vector<double>& flat, int n,
                const vector<double>& b)
{
   double sum = 0.0;
   const vector<double> Mb = matVec(flat, n, b);
   for (int i = 0; i < n; i++) sum += a[i]*Mb[i];
   return sum;
}

/** A * B, both flat n*n row major. */
vector<double> matMul(const vector<double>& A, const vector<double>& B, int n)
{
   vector<double> out((size_t)n*n, 0.0);
   for (int i = 0; i < n; i++) {
      for (int k = 0; k < n; k++) {
         const double aik = A[(size_t)i*n + k];
         if (aik == 0.0) continue;
         for (int j = 0; j < n; j++) out[(size_t)i*n + j] += aik*B[(size_t)k*n + j];
      }
   }
   return out;
}

/** One atom's basis functions: [offsets[a], offsets[a+1]). */
vector<int> atomOffsets(const vector<int>& perAtom)
{
   vector<int> offsets(perAtom.size() + 1, 0);
   for (size_t a = 0; a < perAtom.size(); a++) offsets[a+1] = offsets[a] + perAtom[a];
   return offsets;
}

vector<int> gatherIndices(const vector<int>& atoms, const vector<int>& offsets)
{
   vector<int> idx;
   for (size_t k = 0; k < atoms.size(); k++) {
      const int a = atoms[k];
      if (a < 0 || a + 1 >= (int)offsets.size()) continue;
      for (int mu = offsets[a]; mu < offsets[a+1]; mu++) idx.push_back(mu);
   }
   return idx;
}

vector<double> subMatrix(const vector<double>& flat, int n, const vector<int>& idx)
{
   const int m = (int)idx.size();
   vector<double> out((size_t)m*m, 0.0);
   for (int i = 0; i < m; i++) {
      for (int j = 0; j < m; j++) out[(size_t)i*m + j] = flat[(size_t)idx[i]*n + idx[j]];
   }
   return out;
}

/** A sub-block vector embedded into the full nbasis space, zero elsewhere. */
vector<double> embed(const vector<double>& sub, const vector<int>& idx, int nbasis)
{
   vector<double> out(nbasis, 0.0);
   for (size_t i = 0; i < idx.size(); i++) out[idx[i]] = sub[i];
   return out;
}

/** Group ascending-sorted (value, tag) pairs into clusters within `tol`
 *  of their neighbour -- used for both the metal atomic-like shells and
 *  the drawn interaction-model levels.  Consecutive-neighbour grouping,
 *  not "within tol of the group's first member": a chain of levels each
 *  close to the next but the ends far apart is still one physically
 *  connected near-degenerate set (the same choice MoDiagram::group()
 *  makes, at a much larger energy tolerance). */
vector< vector<int> > clusterAscending(const vector<double>& sortedValues, double tol)
{
   vector< vector<int> > groups;
   for (size_t i = 0; i < sortedValues.size(); i++) {
      if (i > 0 && sortedValues[i] - sortedValues[i-1] <= tol) {
         groups.back().push_back((int)i);
      } else {
         groups.push_back(vector<int>(1, (int)i));
      }
   }
   return groups;
}

}  // namespace


//  --- Fock reconstruction and the generalised eigensolver ------------

void
MoLigandField::buildFock(const vector< vector<double> >& coefficients,
                         const vector<double>& energies,
                         const vector<double>& overlap,
                         int nbasis,
                         vector<double>& fock)
{
   fock.assign((size_t)nbasis*nbasis, 0.0);
   const int nMO = (int)coefficients.size();
   if (nMO == 0 || (int)energies.size() != nMO ||
       overlap.size() != (size_t)nbasis*nbasis) {
      return;
   }

   //  W = S * C, one column per orbital: W[mu][m] = sum_nu S[mu,nu]*C[m][nu].
   //  F = W * diag(eps) * W^T -- see the header for why this is exactly
   //  F on the orbitals' own span.
   vector< vector<double> > W(nMO, vector<double>(nbasis, 0.0));
   for (int m = 0; m < nMO; m++) {
      if ((int)coefficients[m].size() != nbasis) continue;
      W[m] = matVec(overlap, nbasis, coefficients[m]);
   }

   for (int mu = 0; mu < nbasis; mu++) {
      for (int nu = mu; nu < nbasis; nu++) {
         double sum = 0.0;
         for (int m = 0; m < nMO; m++) sum += W[m][mu]*energies[m]*W[m][nu];
         fock[(size_t)mu*nbasis + nu] = sum;
         fock[(size_t)nu*nbasis + mu] = sum;
      }
   }
}


int
MoLigandField::generalizedEigen(const vector<double>& fock,
                                const vector<double>& overlap,
                                int n,
                                vector<double>& values,
                                vector< vector<double> >& vectors,
                                double linearDependenceTol)
{
   values.clear();
   vectors.clear();
   if (n <= 0 || fock.size() != (size_t)n*n || overlap.size() != (size_t)n*n)
      return 0;

   //  S^{-1/2} in S's own eigenbasis, dropping any mode at or below the
   //  linear-dependence floor rather than dividing by ~0 -- the same
   //  convention MoComposition::buildSqrtOverlap() uses for S^{+1/2},
   //  just the reciprocal power and (unlike that function) tolerant of
   //  the near-singular case instead of declining outright, because a
   //  ligand's own small AO block is far more likely to be close to
   //  linearly dependent than a whole-molecule basis is.
   vector< vector<double> > Sa = toMatrix(overlap, n);
   vector<double> sVals;
   vector< vector<double> > sVecs;
   MoComposition::jacobiEigen(Sa, sVals, sVecs);

   vector<double> invSqrt(sVals.size());
   int kept = 0;
   for (size_t k = 0; k < sVals.size(); k++) {
      if (sVals[k] > linearDependenceTol) { invSqrt[k] = 1.0/sqrt(sVals[k]); kept++; }
      else invSqrt[k] = 0.0;
   }

   vector<double> Sinv((size_t)n*n, 0.0);
   for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
         double sum = 0.0;
         for (int k = 0; k < n; k++) sum += sVecs[i][k]*sVecs[j][k]*invSqrt[k];
         Sinv[(size_t)i*n + j] = sum;
      }
   }

   //  F' = Sinv * F * Sinv, a plain symmetric eigenproblem (Sinv is
   //  symmetric, so this is unambiguous as a matrix product).
   const vector<double> FSinv = matMul(fock, Sinv, n);
   const vector<double> FpFlat = matMul(Sinv, FSinv, n);
   vector< vector<double> > Fp = toMatrix(FpFlat, n);

   vector<double> eVals;
   vector< vector<double> > eVecs;
   MoComposition::jacobiEigen(Fp, eVals, eVecs);

   //  Sort ascending, drop the linearly-dependent modes (their "energy"
   //  is meaningless -- Fp's row/column for them is exactly zero by
   //  construction of Sinv, so they come out at 0.0 and would otherwise
   //  masquerade as a real non-bonding level at the origin).
   vector<int> order;
   for (size_t k = 0; k < eVals.size(); k++) order.push_back((int)k);
   std::sort(order.begin(), order.end(),
             [&](int a, int b) { return eVals[a] < eVals[b]; });

   values.clear();
   vectors.clear();
   for (size_t oi = 0; oi < order.size(); oi++) {
      const int k = order[oi];
      //  A dropped S-mode makes every Sinv column touching it zero, so
      //  the corresponding Fp eigenvector's transform back is exactly
      //  the zero vector -- that IS how a linearly-dependent mode is
      //  recognised here, rather than re-deriving it from sVals again.
      vector<double> c(n, 0.0);
      double norm = 0.0;
      for (int i = 0; i < n; i++) {
         double sum = 0.0;
         for (int j = 0; j < n; j++) sum += Sinv[(size_t)i*n + j]*eVecs[j][k];
         c[i] = sum;
         norm += c[i]*c[i];
      }
      if (norm < 1.0e-10) continue;
      values.push_back(eVals[k]);
      vectors.push_back(c);
   }
   return kept;
}


//  --- ligand fragments: connected components of the non-metal atoms --
namespace {

int findRoot(vector<int>& parent, int x)
{
   while (parent[x] != x) { parent[x] = parent[parent[x]]; x = parent[x]; }
   return x;
}

void unite(vector<int>& parent, int a, int b)
{
   a = findRoot(parent, a);
   b = findRoot(parent, b);
   if (a != b) parent[a] = b;
}

/** Non-metal atoms, split by covalent bonds that do not touch the
 *  metal -- a metal-ligand "bond" would otherwise merge every ligand
 *  into one component through the metal atom itself. */
vector< vector<int> > ligandGroups(const vector<double>& coords,
                                   const vector<string>& elements,
                                   int metalAtomIndex)
{
   vector< std::pair<int,int> > bonds;
   MoFragments::covalentBonds(coords, elements, bonds);

   vector<int> parent(elements.size());
   for (size_t a = 0; a < elements.size(); a++) parent[a] = (int)a;
   for (size_t k = 0; k < bonds.size(); k++) {
      const int a = bonds[k].first, b = bonds[k].second;
      if (a == metalAtomIndex || b == metalAtomIndex) continue;
      unite(parent, a, b);
   }

   map<int, vector<int> > byRoot;
   for (size_t a = 0; a < elements.size(); a++) {
      if ((int)a == metalAtomIndex) continue;
      byRoot[findRoot(parent, (int)a)].push_back((int)a);
   }
   vector< vector<int> > groups;
   for (map<int, vector<int> >::iterator it = byRoot.begin(); it != byRoot.end(); ++it)
      groups.push_back(it->second);
   return groups;
}


//  --- the metal's own valence orbitals --------------------------------
//
//  RULE (documented here because it needs one -- see MoLigandField.H
//  and tests/modiagram's Cr(CO)6 capture for the values it produces):
//
//  Solve F_MM c = e S_MM c over the WHOLE metal atom's own basis (every
//  shell it carries, core included) -- one small generalised
//  eigenproblem, not one per shell, so nothing has to assume the atom's
//  s/p/d blocks are exactly decoupled (they are, for a real Gaussian
//  basis on one centre -- different l shells are orthogonal by the
//  angular integral alone -- but solving the whole block costs nothing
//  extra and does not rely on that being exact).
//
//  Each resulting orbital is tagged by which l it is BUILT FROM:
//  weight_l = c^T (S restricted to l's own rows/cols) c, exact because
//  of that same orthogonality. A core shell of a given l and the true
//  valence one of the SAME l both come out "dominant l", so the count
//  alone cannot separate them -- what does is keeping only the
//  HIGHEST-ENERGY orbitals of each l: a split-valence-or-larger basis
//  puts its most diffuse, least-bound function last, and Cr has no
//  core d at all, so this keeps the whole d manifold unconditionally.
//  A sanity ceiling excludes a function so diffuse it is a basis
//  artefact rather than anything chemical -- checked not to fire at
//  all on Cr(CO)6/def2-SVP.
//
//  The five d-dominant orbitals kept are then split into an eg-like
//  (2-fold) and a t2g-like (3-fold) cluster by the SINGLE LARGEST gap
//  among their four consecutive energy differences: Oh forces each
//  sub-cluster to be exactly degenerate and the two clusters apart by
//  the real crystal-field splitting, so the biggest gap is that one
//  splitting, not noise inside either cluster (checked against the
//  data, not assumed -- a clean 2/3 split is required or the d set is
//  left undifferentiated, noted rather than guessed at).
struct MetalOrbital { string tag; vector<double> vecFull; double energy; };

const double VALENCE_SANITY_CEILING = 2.0;   // Hartree

vector<MetalOrbital> metalValenceOrbitals(const vector<double>& fock,
                                          const vector<double>& overlap,
                                          int nbasis,
                                          const vector<int>& shellOf,
                                          const vector<int>& offsets,
                                          int metalAtomIndex,
                                          string& note)
{
   vector<MetalOrbital> out;
   const vector<int> idx = gatherIndices(vector<int>(1, metalAtomIndex), offsets);
   const int n = (int)idx.size();
   if (n == 0) { note = "The metal atom has no basis functions."; return out; }

   vector<int> lOf(n);
   for (int i = 0; i < n; i++) lOf[i] = shellOf[idx[i]];

   const vector<double> Fmm = subMatrix(fock, nbasis, idx);
   const vector<double> Smm = subMatrix(overlap, nbasis, idx);

   vector<double> vals;
   vector< vector<double> > vecs;
   MoLigandField::generalizedEigen(Fmm, Smm, n, vals, vecs);

   const int nOrb = (int)vals.size();
   vector<int> dominantL(nOrb, -1);
   vector<double> dominantWeight(nOrb, 0.0);
   for (int k = 0; k < nOrb; k++) {
      map<int,double> byL;
      const vector<double> Sc = matVec(Smm, n, vecs[k]);
      for (int i = 0; i < n; i++) byL[lOf[i]] += vecs[k][i]*Sc[i];
      for (map<int,double>::iterator it = byL.begin(); it != byL.end(); ++it) {
         if (it->second > dominantWeight[k]) {
            dominantWeight[k] = it->second;
            dominantL[k] = it->first;
         }
      }
   }

   static const char *tagOf[3] = { "metal:a1g", "metal:t1u", 0 /* d: split below */ };

   for (int l = 0; l <= 2; l++) {
      const int want = 2*l + 1;
      vector<int> candidates;
      for (int k = 0; k < nOrb; k++) {
         if (dominantL[k] == l && dominantWeight[k] >= 0.7 &&
             vals[k] < VALENCE_SANITY_CEILING) {
            candidates.push_back(k);
         }
      }
      std::sort(candidates.begin(), candidates.end(),
                [&](int a, int b) { return vals[a] > vals[b]; });   // highest first
      if ((int)candidates.size() < want) {
         ostringstream msg;
         msg << "Only " << candidates.size() << " of the expected " << want
             << " l=" << l << " metal valence orbital(s) were found "
                "(below " << VALENCE_SANITY_CEILING
             << " Ha, >=70% l-weight); the model uses what there is.";
         if (!note.empty()) note += "  ";
         note += msg.str();
      }
      const int take = std::min(want, (int)candidates.size());
      vector<int> kept;
      for (int c = 0; c < take; c++) kept.push_back(candidates[c]);
      std::sort(kept.begin(), kept.end(),
                [&](int a, int b) { return vals[a] < vals[b]; });   // ascending

      if (l < 2) {
         for (size_t c = 0; c < kept.size(); c++) {
            MetalOrbital orb;
            orb.tag = tagOf[l];
            orb.energy = vals[kept[c]];
            orb.vecFull = embed(vecs[kept[c]], idx, nbasis);
            out.push_back(orb);
         }
         continue;
      }

      //  l == 2: split by the largest of the (kept.size()-1) gaps.
      if (kept.size() < 2) {
         for (size_t c = 0; c < kept.size(); c++) {
            MetalOrbital orb;
            orb.tag = "metal:d";
            orb.energy = vals[kept[c]];
            orb.vecFull = embed(vecs[kept[c]], idx, nbasis);
            out.push_back(orb);
         }
         continue;
      }
      int splitAt = -1;
      double biggest = -1.0;
      for (size_t g = 1; g < kept.size(); g++) {
         const double gap = vals[kept[g]] - vals[kept[g-1]];
         if (gap > biggest) { biggest = gap; splitAt = (int)g; }
      }
      const size_t lowerCount = (size_t)splitAt;
      const size_t upperCount = kept.size() - lowerCount;
      const bool clean = (lowerCount == 2 && upperCount == 3) ||
                         (lowerCount == 3 && upperCount == 2);
      for (size_t c = 0; c < kept.size(); c++) {
         MetalOrbital orb;
         if (clean) {
            const bool lower = (int)c < splitAt;
            const size_t thisCount = lower ? lowerCount : upperCount;
            orb.tag = (thisCount == 2) ? "metal:eg" : "metal:t2g";
         } else {
            orb.tag = "metal:d";
         }
         orb.energy = vals[kept[c]];
         orb.vecFull = embed(vecs[kept[c]], idx, nbasis);
         out.push_back(orb);
      }
      if (!clean) {
         if (!note.empty()) note += "  ";
         note += "The 5 metal d orbitals did not split cleanly into a "
                 "2/3 (eg/t2g) pattern by their largest energy gap; "
                 "drawn as an undifferentiated d set instead of guessing "
                 "which is which.";
      }
   }
   return out;
}


//  --- the ligand fragments' own orbitals ------------------------------
//
//  Solved as F_LL c = e S_LL c in EACH ligand's own AO block (step 2 of
//  the model, MoLigandField.H) -- one small generalised eigenproblem
//  per ligand, embedded back into the full AO space with zeros on
//  every other atom. Limited to a generous valence-plus-low-virtual
//  window (LIGAND_WINDOW_LOW..LIGAND_WINDOW_HIGH): a carbonyl's own
//  1s core sits far below it and never needs to be considered here.
//
//  Tagged "sigma"/"pi"/"other" by the WITHIN-LIGAND degeneracy pattern
//  alone (a size-1 energy cluster is sigma-like, size-2 is pi-like,
//  anything else is left as "other<n>") -- a lightweight, honest stand-
//  in for a real symmetry label: it needs no frame, no character
//  table, and is exactly the shape a linear/near-linear diatomic
//  ligand's own orbitals come in. It is NOT a claim that these are the
//  molecule's own Oh SALCs -- see MoLigandField.H's header for why the
//  interaction model does not need that claim to be numerically
//  correct, only the qualitative naming here is coarser for it.
struct LigandOrbital
{
   string tag;          // "ligand:sigma1", "ligand:pi2", "ligand:other1", ...
   int    group;         // which ligand, 0-based
   vector<double> vecFull;
   double energy;
   double maxCoupling;
   bool   interacting;
};

const double LIGAND_WINDOW_LOW = -3.0;    // Hartree
const double LIGAND_WINDOW_HIGH = 1.0;    // Hartree
const double LIGAND_DEGENERACY_TOL = 1.0e-3;   // Hartree

vector<LigandOrbital> ligandFragmentOrbitals(
    const vector<double>& fock, const vector<double>& overlap, int nbasis,
    const vector< vector<int> >& groups, const vector<int>& offsets,
    double& ligandAsymmetry, string& note)
{
   vector<LigandOrbital> out;
   vector< vector<double> > keptEnergiesByGroup;   // for the equivalence check

   for (size_t g = 0; g < groups.size(); g++) {
      const vector<int> idx = gatherIndices(groups[g], offsets);
      const int n = (int)idx.size();
      if (n == 0) continue;
      const vector<double> Fll = subMatrix(fock, nbasis, idx);
      const vector<double> Sll = subMatrix(overlap, nbasis, idx);

      vector<double> vals;
      vector< vector<double> > vecs;
      MoLigandField::generalizedEigen(Fll, Sll, n, vals, vecs);

      vector<double> windowed;
      vector<int> windowedIdx;
      for (size_t k = 0; k < vals.size(); k++) {
         if (vals[k] >= LIGAND_WINDOW_LOW && vals[k] <= LIGAND_WINDOW_HIGH) {
            windowed.push_back(vals[k]);
            windowedIdx.push_back((int)k);
         }
      }
      keptEnergiesByGroup.push_back(windowed);

      const vector< vector<int> > clusters =
          clusterAscending(windowed, LIGAND_DEGENERACY_TOL);
      int sigmaRank = 0, piRank = 0, otherRank = 0;
      for (size_t c = 0; c < clusters.size(); c++) {
         string type;
         if (clusters[c].size() == 1) type = "sigma" + std::to_string(++sigmaRank);
         else if (clusters[c].size() == 2) type = "pi" + std::to_string(++piRank);
         else type = "other" + std::to_string(++otherRank);
         for (size_t m = 0; m < clusters[c].size(); m++) {
            const int w = clusters[c][m];
            LigandOrbital orb;
            orb.tag = "ligand:" + type;
            orb.group = (int)g;
            orb.energy = windowed[w];
            orb.vecFull = embed(vecs[windowedIdx[w]], idx, nbasis);
            orb.maxCoupling = 0.0;
            orb.interacting = false;
            out.push_back(orb);
         }
      }
   }

   //  Equivalence check (step 2): same tag, across groups, should be
   //  the same energy if the ligands really are related by symmetry.
   ligandAsymmetry = 0.0;
   map<string, vector<double> > byTag;
   for (size_t i = 0; i < out.size(); i++) byTag[out[i].tag].push_back(out[i].energy);
   for (map<string, vector<double> >::iterator it = byTag.begin();
        it != byTag.end(); ++it) {
      double lo = it->second[0], hi = it->second[0];
      for (size_t k = 0; k < it->second.size(); k++) {
         lo = std::min(lo, it->second[k]);
         hi = std::max(hi, it->second[k]);
      }
      ligandAsymmetry = std::max(ligandAsymmetry, hi - lo);
   }
   if (groups.size() > 1) {
      ostringstream msg;
      msg << groups.size() << " ligand fragments, largest same-type "
             "energy spread " << ligandAsymmetry << " Ha.";
      if (!note.empty()) note += "  ";
      note += msg.str();
   }
   return out;
}


//  --- coupling: does a ligand orbital feel a metal valence orbital? --
//
//  |<chi|F - e_chi S|phi>|, chi a metal valence orbital that is (by
//  construction) an eigenvector of its OWN block (F_MM chi = e_chi
//  S_MM chi restricted to the metal AOs) -- so this is exactly the
//  residual Fock matrix element left once chi's own diagonal energy is
//  subtracted out, i.e. the standard non-degenerate-perturbation-theory
//  off-diagonal coupling in a non-orthogonal basis. Using e_chi rather
//  than phi's own energy or some average of the two is a choice, made
//  because it asks the physically relevant question -- "how much does
//  this ligand orbital perturb THIS metal orbital" -- and needs nothing
//  beyond what step 3 already computed.
double couplingElement(const vector<double>& chi, double eChi,
                       const vector<double>& phi,
                       const vector<double>& fock, const vector<double>& overlap,
                       int nbasis)
{
   const double f = bilinear(chi, fock, nbasis, phi);
   const double s = bilinear(chi, overlap, nbasis, phi);
   return fabs(f - eChi*s);
}

void computeCouplings(vector<LigandOrbital>& ligandOrbitals,
                      const vector<MetalOrbital>& metalOrbitals,
                      const vector<double>& fock, const vector<double>& overlap,
                      int nbasis)
{
   for (size_t p = 0; p < ligandOrbitals.size(); p++) {
      double best = 0.0;
      for (size_t m = 0; m < metalOrbitals.size(); m++) {
         const double c = couplingElement(metalOrbitals[m].vecFull,
             metalOrbitals[m].energy, ligandOrbitals[p].vecFull, fock, overlap, nbasis);
         if (c > best) best = c;
      }
      ligandOrbitals[p].maxCoupling = best;
   }
}

/**
 * The data-driven threshold (step 4/6): the largest RATIO gap in the
 * sorted couplings, the same "biggest gap decides" idea
 * MoDiagram::suggestCoreCutoff() uses for energies, applied on a log
 * scale because a coupling that is genuinely "not interacting" is
 * usually orders of magnitude smaller than one that is, not merely
 * somewhat smaller (checked against the real Cr(CO)6 numbers -- see
 * the capture's own report for what the gap actually looks like).
 */
double suggestCouplingThreshold(const vector<LigandOrbital>& ligandOrbitals,
                                string& why)
{
   vector<double> v;
   for (size_t i = 0; i < ligandOrbitals.size(); i++) {
      if (ligandOrbitals[i].maxCoupling > 1.0e-12) v.push_back(ligandOrbitals[i].maxCoupling);
   }
   std::sort(v.begin(), v.end(), std::greater<double>());
   if (v.size() < 2) {
      why = "Too few nonzero couplings to find a gap; using a fixed "
            "fallback of 0.001 Ha.";
      return 1.0e-3;
   }
   int splitAt = 0;
   double bestRatio = -1.0;
   for (size_t i = 0; i + 1 < v.size(); i++) {
      const double ratio = v[i]/v[i+1];
      if (ratio > bestRatio) { bestRatio = ratio; splitAt = (int)i; }
   }
   const double threshold = sqrt(v[splitAt]*v[splitAt+1]);
   ostringstream msg;
   msg << "Largest ratio gap in the sorted couplings is " << bestRatio
       << "x, between " << v[splitAt] << " and " << v[splitAt+1]
       << " Ha; threshold set to their geometric mean.";
   why = msg.str();
   return threshold;
}


//  --- the interaction model itself ------------------------------------

string metalTagShort(const string& tag)
{
   const size_t colon = tag.find(':');
   return colon == string::npos ? tag : tag.substr(colon + 1);
}

struct SmallMember
{
   string tag;
   vector<double> vecFull;
   double energy;
};

struct RawLevel
{
   double energy;
   vector<double> vecFull;
   map<string,double> weightByTag;   // sums to ~1 over all tags
};

/**
 * Diagonalise the metal valence orbitals plus whichever ligand orbitals
 * clear `threshold` (step 5): S/F for this small basis are built
 * directly from the real AO S/F via bilinear forms on the members' own
 * (already embedded, full-AO-space) vectors, symmetrically
 * orthogonalised, and diagonalised -- generalizedEigen() again, just at
 * this basis' own small size.
 *
 * WEIGHTS ARE LOWDIN, NOT PLAIN c^2: the small-basis members are not
 * mutually orthogonal (a metal d orbital and a ligand sigma donor
 * plainly overlap), so a raw eigenvector component squared is not a
 * bounded population the way it would be in an orthonormal basis --
 * exactly the same reason MoComposition::lowdinShares() transforms by
 * S^{1/2} before squaring, done here at this basis' own size.
 */
vector<RawLevel> solveInteractionModel(const vector<MetalOrbital>& metalOrbitals,
                                       vector<LigandOrbital>& ligandOrbitals,
                                       double threshold,
                                       const vector<double>& fock,
                                       const vector<double>& overlap, int nbasis)
{
   vector<SmallMember> members;
   for (size_t i = 0; i < metalOrbitals.size(); i++) {
      SmallMember m;
      m.tag = metalOrbitals[i].tag;
      m.vecFull = metalOrbitals[i].vecFull;
      m.energy = metalOrbitals[i].energy;
      members.push_back(m);
   }
   for (size_t i = 0; i < ligandOrbitals.size(); i++) {
      ligandOrbitals[i].interacting = ligandOrbitals[i].maxCoupling > threshold;
      if (!ligandOrbitals[i].interacting) continue;
      SmallMember m;
      m.tag = ligandOrbitals[i].tag;
      m.vecFull = ligandOrbitals[i].vecFull;
      m.energy = ligandOrbitals[i].energy;
      members.push_back(m);
   }

   vector<RawLevel> out;
   const int n = (int)members.size();
   if (n == 0) return out;

   vector<double> Ssmall((size_t)n*n, 0.0), Fsmall((size_t)n*n, 0.0);
   for (int i = 0; i < n; i++) {
      for (int j = i; j < n; j++) {
         const double s = bilinear(members[i].vecFull, overlap, nbasis, members[j].vecFull);
         const double f = bilinear(members[i].vecFull, fock, nbasis, members[j].vecFull);
         Ssmall[(size_t)i*n + j] = Ssmall[(size_t)j*n + i] = s;
         Fsmall[(size_t)i*n + j] = Fsmall[(size_t)j*n + i] = f;
      }
   }

   vector<double> vals;
   vector< vector<double> > vecs;
   MoLigandField::generalizedEigen(Fsmall, Ssmall, n, vals, vecs);

   vector<double> sqrtSmall;
   const bool haveSqrt = MoComposition::buildSqrtOverlap(Ssmall, n, sqrtSmall);

   for (size_t k = 0; k < vals.size(); k++) {
      RawLevel lvl;
      lvl.energy = vals[k];
      lvl.vecFull.assign(nbasis, 0.0);
      for (int i = 0; i < n; i++) {
         if (vecs[k][i] == 0.0) continue;
         for (int mu = 0; mu < nbasis; mu++) lvl.vecFull[mu] += vecs[k][i]*members[i].vecFull[mu];
      }
      const vector<double> d = haveSqrt ? MoComposition::transform(vecs[k], sqrtSmall) : vecs[k];
      double norm = 0.0;
      for (int i = 0; i < n; i++) norm += d[i]*d[i];
      for (int i = 0; i < n; i++) {
         lvl.weightByTag[members[i].tag] += (norm > 0.0) ? d[i]*d[i]/norm : 0.0;
      }
      out.push_back(lvl);
   }
   return out;
}

typedef MoLigandField::Level Level;
typedef MoLigandField::Level::Character Character;

/** Group same-tag orbitals into one display level each -- the metal and
 *  ligand-interacting "parent" columns, unlike the interaction column,
 *  need no energy clustering: the tag itself already says which
 *  orbitals belong together. */
Level groupByTag(const string& tag, const vector<double>& energies,
                 bool isMetal)
{
   Level lvl;
   lvl.label = isMetal ? metalTagShort(tag) : tag.substr(tag.find(':') + 1);
   lvl.degeneracy = (int)energies.size();
   double sum = 0.0;
   for (size_t i = 0; i < energies.size(); i++) sum += energies[i];
   lvl.energy = energies.empty() ? 0.0 : sum/energies.size();
   lvl.metalWeight = isMetal ? 1.0 : 0.0;
   lvl.ligandWeight = isMetal ? 0.0 : 1.0;
   return lvl;
}

vector<Level> buildParentColumn(const vector<MetalOrbital>& orbitals)
{
   map<string, vector<double> > byTag;
   for (size_t i = 0; i < orbitals.size(); i++) byTag[orbitals[i].tag].push_back(orbitals[i].energy);
   vector<Level> out;
   for (map<string, vector<double> >::iterator it = byTag.begin(); it != byTag.end(); ++it)
      out.push_back(groupByTag(it->first, it->second, true));
   std::sort(out.begin(), out.end(), [](const Level& a, const Level& b) { return a.energy < b.energy; });
   return out;
}

vector<Level> buildLigandParentColumn(const vector<LigandOrbital>& orbitals, bool interacting)
{
   map<string, vector<double> > byTag;
   for (size_t i = 0; i < orbitals.size(); i++) {
      if (orbitals[i].interacting != interacting) continue;
      byTag[orbitals[i].tag].push_back(orbitals[i].energy);
   }
   vector<Level> out;
   for (map<string, vector<double> >::iterator it = byTag.begin(); it != byTag.end(); ++it)
      out.push_back(groupByTag(it->first, it->second, false));
   std::sort(out.begin(), out.end(), [](const Level& a, const Level& b) { return a.energy < b.energy; });
   return out;
}

/**
 * Group the raw interaction-model eigenlevels into drawn levels, label
 * and classify them, and build the correlation links -- the heart of
 * step 5/6 of the model (MoLigandField.H).
 *
 * CHARACTER IS POSITION VS PARENTS (the "physically cleaner option",
 * Andy 2026-09-28's item 5): a level below both the metal and ligand
 * energies it is built from is bonding, above both is antibonding, by
 * definition of the words -- the same rule MoDiagram::classifyByEnergy()
 * already uses for the canonical diagram, applied here to THIS model's
 * own parents (a tag's mean energy in the metal/ligand parent columns)
 * rather than to a fragment column built a different way. The
 * alternative offered in the brief -- the sign of the metal-ligand part
 * of an energy decomposition -- needs a partition of the Fock energy
 * into pairwise atom/fragment contributions that nothing here computes
 * and that is itself model-dependent; position-vs-parents needs only
 * numbers this function already has.
 */
void groupInteraction(const vector<RawLevel>& raw,
                      const vector<Level>& metalColumn,
                      const vector<Level>& ligandColumn,
                      double tol,
                      vector<Level>& levels,
                      vector<MoLigandField::Link>& links,
                      vector< vector<int> >& rawIndexOfLevel)
{
   levels.clear();
   links.clear();
   rawIndexOfLevel.clear();
   if (raw.empty()) return;

   map<string,int> metalIndexOf, ligandIndexOf;
   for (size_t i = 0; i < metalColumn.size(); i++) metalIndexOf[metalColumn[i].label] = (int)i;
   for (size_t i = 0; i < ligandColumn.size(); i++) ligandIndexOf[ligandColumn[i].label] = (int)i;

   vector<int> order;
   for (size_t i = 0; i < raw.size(); i++) order.push_back((int)i);
   std::sort(order.begin(), order.end(),
             [&](int a, int b) { return raw[a].energy < raw[b].energy; });

   vector<double> sortedEnergies;
   for (size_t i = 0; i < order.size(); i++) sortedEnergies.push_back(raw[order[i]].energy);
   const vector< vector<int> > clusters = clusterAscending(sortedEnergies, tol);

   map<string, double> metalEnergyOf, ligandEnergyOf;
   for (size_t i = 0; i < metalColumn.size(); i++) metalEnergyOf[metalColumn[i].label] = metalColumn[i].energy;
   for (size_t i = 0; i < ligandColumn.size(); i++) ligandEnergyOf[ligandColumn[i].label] = ligandColumn[i].energy;

   for (size_t c = 0; c < clusters.size(); c++) {
      Level lvl;
      double energySum = 0.0;
      map<string,double> weightSum;
      for (size_t m = 0; m < clusters[c].size(); m++) {
         const RawLevel& r = raw[order[clusters[c][m]]];
         energySum += r.energy;
         for (map<string,double>::const_iterator it = r.weightByTag.begin();
              it != r.weightByTag.end(); ++it) {
            weightSum[it->first] += it->second;
         }
      }
      const double n = (double)clusters[c].size();
      lvl.energy = energySum/n;
      lvl.degeneracy = (int)clusters[c].size();

      string dominantMetalTag, dominantLigandTag;
      double dominantMetalWeight = 0.0, dominantLigandWeight = 0.0;
      lvl.metalWeight = lvl.ligandWeight = 0.0;
      for (map<string,double>::iterator it = weightSum.begin(); it != weightSum.end(); ++it) {
         const double w = it->second/n;
         const bool isMetal = it->first.compare(0, 6, "metal:") == 0;
         if (isMetal) {
            lvl.metalWeight += w;
            if (w > dominantMetalWeight) { dominantMetalWeight = w; dominantMetalTag = it->first; }
         } else {
            lvl.ligandWeight += w;
            if (w > dominantLigandWeight) { dominantLigandWeight = w; dominantLigandTag = it->first; }
         }
      }

      const bool haveMetalParent = lvl.metalWeight >= 0.05 && !dominantMetalTag.empty() &&
          metalEnergyOf.count(metalTagShort(dominantMetalTag));
      const bool haveLigandParent = lvl.ligandWeight >= 0.05 && !dominantLigandTag.empty() &&
          ligandEnergyOf.count(dominantLigandTag.substr(dominantLigandTag.find(':') + 1));
      double metalParentE = haveMetalParent ? metalEnergyOf[metalTagShort(dominantMetalTag)] : 0.0;
      double ligandParentE = haveLigandParent ? ligandEnergyOf[dominantLigandTag.substr(dominantLigandTag.find(':') + 1)] : 0.0;

      const double MARGIN = 0.01;   // Hartree
      if (haveMetalParent && haveLigandParent) {
         const double lo = std::min(metalParentE, ligandParentE);
         const double hi = std::max(metalParentE, ligandParentE);
         if (lvl.energy < lo - MARGIN) lvl.character = Level::BONDING;
         else if (lvl.energy > hi + MARGIN) lvl.character = Level::ANTIBONDING;
         else lvl.character = Level::NONBONDING;
      } else {
         lvl.character = Level::NONBONDING;
      }

      const bool metalDominant = lvl.metalWeight >= 0.5;
      lvl.label = metalDominant ? metalTagShort(dominantMetalTag)
                                : (dominantLigandTag.empty() ? "?" : dominantLigandTag);
      if (lvl.character == Level::ANTIBONDING) lvl.label += "*";

      const int levelIndex = (int)levels.size();
      for (map<string,double>::iterator it = weightSum.begin(); it != weightSum.end(); ++it) {
         const double w = it->second/n;
         if (w < 0.05) continue;
         const bool isMetal = it->first.compare(0, 6, "metal:") == 0;
         MoLigandField::Link link;
         link.modelIndex = levelIndex;
         link.weight = w;
         if (isMetal) {
            map<string,int>::iterator mi = metalIndexOf.find(metalTagShort(it->first));
            link.metalIndex = (mi != metalIndexOf.end()) ? mi->second : -1;
         } else {
            const string ligLabel = it->first.substr(it->first.find(':') + 1);
            map<string,int>::iterator li = ligandIndexOf.find(ligLabel);
            link.ligandIndex = (li != ligandIndexOf.end()) ? li->second : -1;
         }
         if (link.metalIndex >= 0 || link.ligandIndex >= 0) links.push_back(link);
      }

      levels.push_back(lvl);
      vector<int> origIdx;
      for (size_t m = 0; m < clusters[c].size(); m++) origIdx.push_back(order[clusters[c][m]]);
      rawIndexOfLevel.push_back(origIdx);
   }

   //  Pairing: only when a tag's own levels split into exactly one
   //  bonding and one antibonding -- Cr(CO)6's t2g set has a THIRD,
   //  non-bonding member between them (the metal-heavy HOMO), which is
   //  deliberately left unpaired rather than forced onto either side.
   map<string, vector<int> > byBaseLabel;
   for (size_t i = 0; i < levels.size(); i++) {
      string base = levels[i].label;
      if (!base.empty() && base.back() == '*') base.pop_back();
      byBaseLabel[base].push_back((int)i);
   }
   for (map<string, vector<int> >::iterator it = byBaseLabel.begin();
        it != byBaseLabel.end(); ++it) {
      int bonding = -1, antibonding = -1, others = 0;
      for (size_t k = 0; k < it->second.size(); k++) {
         const int i = it->second[k];
         if (levels[i].character == Level::BONDING) { if (bonding < 0) bonding = i; else others++; }
         else if (levels[i].character == Level::ANTIBONDING) { if (antibonding < 0) antibonding = i; else others++; }
      }
      if (bonding >= 0 && antibonding >= 0 && others == 0) {
         levels[bonding].pairing = antibonding;
         levels[antibonding].pairing = bonding;
      }
   }
}

/**
 * The three robustness questions Andy asked for (2026-09-28 item 3),
 * read off a finished interaction column: the pi-acceptor three-level
 * t2g pattern (targets doc #7), Delta_o's sign, and whether the eg*-
 * like level is still above the t2g-like HOMO -- the last two are the
 * SAME comparison by this model's own definition of Delta_o (the eg*
 * energy minus the t2g HOMO energy), which is stated rather than
 * pretended to be two independent checks.
 */
void extractPiAcceptorAndDeltaO(const vector<Level>& interaction,
                                double& deltaO, bool& piAcceptorPattern,
                                bool& eg2AboveT2g)
{
   deltaO = 0.0;
   piAcceptorPattern = false;
   eg2AboveT2g = false;

   vector<int> t2gIdx, egIdx;
   for (size_t i = 0; i < interaction.size(); i++) {
      string base = interaction[i].label;
      if (!base.empty() && base[base.size()-1] == '*') base.erase(base.size()-1);
      if (base == "t2g") t2gIdx.push_back((int)i);
      else if (base == "eg") egIdx.push_back((int)i);
   }

   if (t2gIdx.size() >= 3) {
      bool sawBonding = false, sawNonbonding = false, sawAntibonding = false;
      for (size_t k = 0; k < t2gIdx.size(); k++) {
         switch (interaction[t2gIdx[k]].character) {
            case Level::BONDING:     sawBonding = true; break;
            case Level::NONBONDING:  sawNonbonding = true; break;
            case Level::ANTIBONDING: sawAntibonding = true; break;
            default: break;
         }
      }
      piAcceptorPattern = sawBonding && sawNonbonding && sawAntibonding;
   }

   int homoT2g = -1;
   for (size_t k = 0; k < t2gIdx.size(); k++) {
      if (interaction[t2gIdx[k]].character == Level::NONBONDING) homoT2g = t2gIdx[k];
   }
   if (homoT2g < 0) {
      for (size_t k = 0; k < t2gIdx.size(); k++) {
         if (homoT2g < 0 || interaction[t2gIdx[k]].energy > interaction[homoT2g].energy)
            homoT2g = t2gIdx[k];
      }
   }

   int egStar = -1;
   for (size_t k = 0; k < egIdx.size(); k++) {
      if (interaction[egIdx[k]].character == Level::ANTIBONDING &&
          (egStar < 0 || interaction[egIdx[k]].energy > interaction[egStar].energy)) {
         egStar = egIdx[k];
      }
   }

   if (homoT2g >= 0 && egStar >= 0) {
      deltaO = interaction[egStar].energy - interaction[homoT2g].energy;
      eg2AboveT2g = deltaO > 0.0;
   }
}

const char* characterName(Level::Character c)
{
   switch (c) {
      case Level::BONDING:     return "BONDING";
      case Level::NONBONDING:  return "NONBONDING";
      case Level::ANTIBONDING: return "ANTIBONDING";
      default:                 return "UNKNOWN";
   }
}

/** Match each base-run level to the nearest-energy same-label level of
 *  a rescanned run, and report every character change -- item 3's
 *  "report whether each qualitative claim changes", read off the
 *  labels rather than a level index that a different threshold can
 *  change the meaning of. */
vector<string> compareCharacters(const vector<Level>& base, const vector<Level>& scan)
{
   vector<string> changes;
   for (size_t i = 0; i < base.size(); i++) {
      string baseLabel = base[i].label;
      if (!baseLabel.empty() && baseLabel[baseLabel.size()-1] == '*') baseLabel.erase(baseLabel.size()-1);
      int best = -1;
      double bestDE = 1.0e30;
      for (size_t j = 0; j < scan.size(); j++) {
         string sLabel = scan[j].label;
         if (!sLabel.empty() && sLabel[sLabel.size()-1] == '*') sLabel.erase(sLabel.size()-1);
         if (sLabel != baseLabel) continue;
         const double de = fabs(scan[j].energy - base[i].energy);
         if (de < bestDE) { bestDE = de; best = (int)j; }
      }
      if (best >= 0 && scan[best].character != base[i].character) {
         ostringstream m;
         m << base[i].label << ": " << characterName(base[i].character)
           << " -> " << characterName(scan[best].character);
         changes.push_back(m.str());
      }
   }
   return changes;
}

}  // namespace


void
MoLigandField::build(const vector<double>& coords,
                     const vector<string>& elements,
                     int metalAtomIndex,
                     const vector<double>& moEnergies,
                     const vector<double>& moOccupancies,
                     const vector< vector<double> >& moCoefficients,
                     const vector<int>& perAtom,
                     const vector<int>& shellOf,
                     const vector<double>& overlap,
                     const vector<double>& sqrtOverlap,
                     bool openShell,
                     Result& result)
{
   result = Result();
   (void)moOccupancies;
   (void)sqrtOverlap;   // reconstructed from `overlap` directly (buildFock());
                        // kept in the signature for interface symmetry with
                        // the caller's own already-built S^{1/2} and in case
                        // a future caller needs it without recomputing.

   const int nbasis = (int)shellOf.size();
   const int nMO = (int)moCoefficients.size();

   if (elements.empty() || metalAtomIndex < 0 ||
       (size_t)metalAtomIndex >= elements.size() || nMO == 0 ||
       (int)moEnergies.size() != nMO ||
       overlap.size() != (size_t)nbasis*nbasis ||
       perAtom.size() != elements.size()) {
      result.note = "The ligand-field model needs a metal atom, MO "
                    "coefficients and a real overlap matrix; one of "
                    "those is missing.";
      return;
   }

   result.method = openShell
       ? "canonical alpha-spin HF/DFT orbitals of an open-shell "
         "calculation (beta orbitals are not modelled)"
       : "canonical closed-shell HF/DFT orbitals";
   result.energyScaleNote =
       "Every energy and gap here is a canonical-orbital ENERGY GAP "
       "(Koopmans-like at best), never a computed excitation energy.";

   vector<double> fock;
   buildFock(moCoefficients, moEnergies, overlap, nbasis, fock);

   const vector<int> offsets = atomOffsets(perAtom);
   if (offsets.back() != nbasis) {
      result.note = "Basis functions per atom do not sum to the basis size.";
      return;
   }

   const vector< vector<int> > groups = ligandGroups(coords, elements, metalAtomIndex);
   if (groups.empty()) {
      result.note = "No ligand fragments were found bonded to the metal atom.";
      return;
   }

   string ligandNote;
   vector<LigandOrbital> ligandOrbitals = ligandFragmentOrbitals(
       fock, overlap, nbasis, groups, offsets, result.ligandAsymmetry, ligandNote);

   string metalNote;
   const vector<MetalOrbital> metalOrbitals = metalValenceOrbitals(
       fock, overlap, nbasis, shellOf, offsets, metalAtomIndex, metalNote);
   if (metalOrbitals.empty()) {
      result.note = "No metal valence orbitals could be identified.";
      return;
   }

   computeCouplings(ligandOrbitals, metalOrbitals, fock, overlap, nbasis);
   string thresholdWhy;
   const double baseThreshold = suggestCouplingThreshold(ligandOrbitals, thresholdWhy);
   result.couplingThreshold = baseThreshold;
   result.thresholdWhy = thresholdWhy;

   //  --- the real model, at the data-chosen threshold -----------------
   vector<LigandOrbital> ligandsAtBase = ligandOrbitals;
   vector<RawLevel> raw = solveInteractionModel(metalOrbitals, ligandsAtBase,
                                                baseThreshold, fock, overlap, nbasis);

   result.metal = buildParentColumn(metalOrbitals);
   result.ligandInteracting = buildLigandParentColumn(ligandsAtBase, true);
   result.ligandInternal = buildLigandParentColumn(ligandsAtBase, false);

   for (size_t i = 0; i < ligandsAtBase.size(); i++) {
      MoLigandField::Coupling c;
      ostringstream label;
      label << "ligand#" << ligandsAtBase[i].group << " " << ligandsAtBase[i].tag;
      c.label = label.str();
      c.energy = ligandsAtBase[i].energy;
      c.maxCoupling = ligandsAtBase[i].maxCoupling;
      c.interacting = ligandsAtBase[i].interacting;
      result.couplings.push_back(c);
   }

   const double LEVEL_CLUSTER_TOL = 1.0e-4;   // Hartree
   vector< vector<int> > rawIndexOfLevel;
   groupInteraction(raw, result.metal, result.ligandInteracting, LEVEL_CLUSTER_TOL,
                    result.interaction, result.links, rawIndexOfLevel);

   bool piAcceptorPattern = false, eg2AboveT2g = false;
   extractPiAcceptorAndDeltaO(result.interaction, result.deltaO,
                              piAcceptorPattern, eg2AboveT2g);

   result.note = ligandNote;
   if (!metalNote.empty()) { if (!result.note.empty()) result.note += "  "; result.note += metalNote; }

   if (raw.empty()) {
      if (!result.note.empty()) result.note += "  ";
      result.note += "The interaction model is empty: no ligand orbital "
                     "cleared the coupling threshold.";
      result.ok = false;
      return;
   }
   result.ok = true;

   //  --- validation (Andy, 2026-09-28): capture and energy agreement --
   //
   //  CAPTURE is the projector onto the model's own span (the
   //  interaction column's eigenvectors ARE a complete orthonormal
   //  basis of that span -- see solveInteractionModel()'s header)
   //  applied to each canonical valence MO: sum_k |<raw_k|S|MO_m>|^2
   //  over EVERY raw eigenvector, not just the ones grouped into one
   //  drawn level -- Parseval's identity over that orthonormal set, so
   //  this is exact for the model as built, not an approximation of it.
   const double VALENCE_WINDOW_LOW = -3.0, VALENCE_WINDOW_HIGH = 2.0;   // Hartree
   vector<int> windowMO;
   for (int m = 0; m < nMO; m++) {
      if (moEnergies[m] >= VALENCE_WINDOW_LOW && moEnergies[m] <= VALENCE_WINDOW_HIGH)
         windowMO.push_back(m);
   }
   vector< vector<double> > overlapRawMO(raw.size(), vector<double>(windowMO.size(), 0.0));
   for (size_t k = 0; k < raw.size(); k++) {
      for (size_t im = 0; im < windowMO.size(); im++) {
         const double ov = bilinear(raw[k].vecFull, overlap, nbasis, moCoefficients[windowMO[im]]);
         overlapRawMO[k][im] = ov*ov;
      }
   }
   vector<double> captureOf(windowMO.size(), 0.0);
   for (size_t im = 0; im < windowMO.size(); im++) {
      double sum = 0.0;
      for (size_t k = 0; k < raw.size(); k++) sum += overlapRawMO[k][im];
      captureOf[im] = sum;
   }

   double worstCapture = 1.0;
   string worstCaptureLabel;
   for (size_t lv = 0; lv < result.interaction.size(); lv++) {
      double bestOv = 0.0;
      int bestIm = -1;
      for (size_t im = 0; im < windowMO.size(); im++) {
         double ov = 0.0;
         for (size_t c = 0; c < rawIndexOfLevel[lv].size(); c++)
            ov += overlapRawMO[rawIndexOfLevel[lv][c]][im];
         if (ov > bestOv) { bestOv = ov; bestIm = (int)im; }
      }
      if (bestIm < 0 || bestOv < MATCH_OVERLAP_MIN) continue;
      Level& lvl = result.interaction[lv];
      lvl.canonicalMOs.push_back(windowMO[bestIm]);
      lvl.canonicalOverlap.push_back(bestOv);
      lvl.capture = captureOf[bestIm];
      lvl.energyDeviation = fabs(lvl.energy - moEnergies[windowMO[bestIm]]);
      if (lvl.capture < worstCapture) { worstCapture = lvl.capture; worstCaptureLabel = lvl.label; }
   }

   //  Energy agreement: flag any adjacent pair of drawn levels whose
   //  ORDER the diagram asserts (i.e. every adjacent pair, since the
   //  diagram draws them top to bottom) where the gap is smaller than
   //  twice the larger of the two levels' own energy deviation from
   //  its canonical match -- the deviation is exactly the model's own
   //  uncertainty on that level's placement, so a gap that small is not
   //  a claim the model can actually support.
   {
      vector<int> order;
      for (size_t i = 0; i < result.interaction.size(); i++) order.push_back((int)i);
      std::sort(order.begin(), order.end(), [&](int a, int b) {
         return result.interaction[a].energy < result.interaction[b].energy; });
      for (size_t i = 0; i + 1 < order.size(); i++) {
         const Level& a = result.interaction[order[i]];
         const Level& b = result.interaction[order[i+1]];
         if (a.energyDeviation < 0.0 || b.energyDeviation < 0.0) continue;
         const double gap = b.energy - a.energy;
         const double worstDev = std::max(a.energyDeviation, b.energyDeviation);
         if (gap < 2.0*worstDev) {
            ostringstream msg;
            msg << "Order claim " << a.label << " < " << b.label
                << " (gap " << gap << " Ha) is UNRESOLVED: it is less than "
                   "twice the larger energy deviation from a canonical "
                   "match (" << worstDev << " Ha).";
            result.disagreements.push_back(msg.str());
         }
      }
   }

   //  --- robustness scan: rebuild at 0.5x and 2x the threshold --------
   for (int p = 0; p < 2; p++) {
      const double scale = (p == 0) ? 0.5 : 2.0;
      MoLigandField::RobustnessPoint pt;
      pt.scale = scale;

      vector<LigandOrbital> ligandsScan = ligandOrbitals;
      vector<RawLevel> rawScan = solveInteractionModel(metalOrbitals, ligandsScan,
          baseThreshold*scale, fock, overlap, nbasis);
      const vector<Level> metalScan = buildParentColumn(metalOrbitals);
      const vector<Level> ligandScanColumn = buildLigandParentColumn(ligandsScan, true);
      vector<Level> interactionScan;
      vector<Link> linksScan;
      vector< vector<int> > rawIndexScan;
      groupInteraction(rawScan, metalScan, ligandScanColumn, LEVEL_CLUSTER_TOL,
                       interactionScan, linksScan, rawIndexScan);
      double deltaOScan = 0.0;
      extractPiAcceptorAndDeltaO(interactionScan, deltaOScan,
                                 pt.piAcceptorPattern, pt.eg2AboveT2g);
      pt.deltaOSign = deltaOScan > 0.0;
      pt.characterChanges = compareCharacters(result.interaction, interactionScan);
      result.robustness.push_back(pt);
   }

   //  --- verdict (Andy, 2026-09-28) -------------------------------------
   //
   //  Thresholds in one place, with why: 0.8 and 0.5 are the same
   //  "mostly right" / "recognisable but not trustworthy in detail"
   //  split used elsewhere for a share-based confidence call (see
   //  MoComposition::LOCALISED_SHARE_THRESHOLD's own header for the
   //  same style of judgement call) -- there is no first-principles
   //  number here, only the requirement that it be stated once and
   //  checked against real data rather than tuned per molecule; see
   //  the capture's own report for what Cr(CO)6 actually gives.
   const bool unstable = result.robustness.size() == 2 &&
       (!result.robustness[0].characterChanges.empty() ||
        !result.robustness[1].characterChanges.empty() ||
        result.robustness[0].deltaOSign != eg2AboveT2g ||
        result.robustness[1].deltaOSign != eg2AboveT2g ||
        result.robustness[0].piAcceptorPattern != piAcceptorPattern ||
        result.robustness[1].piAcceptorPattern != piAcceptorPattern ||
        result.robustness[0].eg2AboveT2g != eg2AboveT2g ||
        result.robustness[1].eg2AboveT2g != eg2AboveT2g);
   const bool orderUnresolved = !result.disagreements.empty();

   if (worstCapture < CAPTURE_APPROXIMATE) {
      result.verdict = Result::NOT_REPRESENTATIVE;
   } else if (worstCapture < CAPTURE_REPRESENTATIVE || orderUnresolved || unstable) {
      result.verdict = Result::APPROXIMATE;
   } else {
      result.verdict = Result::REPRESENTATIVE;
   }
   {
      ostringstream r;
      r << "Worst per-level capture: " << worstCapture << " (" << worstCaptureLabel << ")";
      result.verdictReasons.push_back(r.str());
   }
   if (orderUnresolved) {
      result.verdictReasons.push_back("One or more order claims are unresolved (see disagreements).");
   }
   if (unstable) {
      result.verdictReasons.push_back("A qualitative claim changed under the 0.5x/2x threshold scan.");
   }
}

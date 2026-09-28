#include <cmath>
#include <map>
#include <utility>

#include "tdat/MoComposition.H"
#include "tdat/MoFragments.H"

using std::map;
using std::pair;


vector<MoComposition::Share>
MoComposition::compute(const vector<double>& coefficients,
                       const vector<int>& functionsPerAtom,
                       const vector<int>& shellOf,
                       const vector<string>& elements,
                       const vector<double>& overlap)
{
   vector<Share> out;

   const size_t nbas = coefficients.size();
   if (nbas == 0 || functionsPerAtom.size() != elements.size()) return out;

   //  Same check share() makes: the mapping has to account for every
   //  function, or a mismatch produces a plausible-looking answer
   //  instead of an error.
   int total = 0;
   for (size_t i = 0; i < functionsPerAtom.size(); i++) total += functionsPerAtom[i];
   if (total != (int)nbas) return out;

   const bool haveShell = (shellOf.size() == nbas);

   //  Which atom each basis function belongs to, and (when given)
   //  which shell -- so the functions can be grouped by (atom, shell)
   //  regardless of where in the row they fall.
   vector<int> atomOf(nbas, -1);
   {
      int at = 0;
      for (size_t a = 0; a < functionsPerAtom.size(); a++) {
         for (int f = 0; f < functionsPerAtom[a]; f++) atomOf[at + f] = (int)a;
         at += functionsPerAtom[a];
      }
   }

   //  Grouped in (atom, shell) order of first appearance -- a plain
   //  map<pair<int,int>, ...> would reorder by shell number first,
   //  which reads oddly for a per-atom breakdown (2p before 2s).
   vector< pair<int,int> > keys;             // (atom, shell) as encountered
   map< pair<int,int>, vector<int> > groups;
   for (size_t mu = 0; mu < nbas; mu++) {
      const int a = atomOf[mu];
      if (a < 0) continue;
      const int shell = haveShell ? shellOf[mu] : -1;
      const pair<int,int> key(a, shell);
      if (groups.find(key) == groups.end()) keys.push_back(key);
      groups[key].push_back((int)mu);
   }

   out.reserve(keys.size());
   for (size_t i = 0; i < keys.size(); i++) {
      const int a = keys[i].first;
      const int shell = keys[i].second;
      const double s = MoFragments::shareOfIndices(coefficients,
                                                    groups[keys[i]], overlap);
      if (s < 0.0) continue;

      Share entry;
      entry.atom    = a;
      entry.element = (a >= 0 && a < (int)elements.size()) ? elements[a] : string();
      entry.shell   = shell;
      entry.share   = s;
      out.push_back(entry);
   }

   return out;
}


double
MoComposition::overlapPopulation(const vector<double>& coefficients,
                                 const vector<int>& functionsPerAtom,
                                 const vector<double>& overlap,
                                 const vector< pair<int,int> >& bonds)
{
   const size_t nbas = coefficients.size();
   if (nbas == 0 || overlap.size() != nbas*nbas) return 0.0;

   int total = 0;
   for (size_t i = 0; i < functionsPerAtom.size(); i++) total += functionsPerAtom[i];
   if (total != (int)nbas) return 0.0;

   //  Same atom-boundary walk compute() uses.
   vector<int> atomOf(nbas, -1);
   {
      int at = 0;
      for (size_t a = 0; a < functionsPerAtom.size(); a++) {
         for (int f = 0; f < functionsPerAtom[a]; f++) atomOf[at + f] = (int)a;
         at += functionsPerAtom[a];
      }
   }

   //  Which basis functions belong to which atom, so each bonded pair
   //  is a small nested loop over its own two atoms' functions rather
   //  than a full nbas*nbas scan repeated per bond.
   vector< vector<int> > functionsOf(functionsPerAtom.size());
   for (size_t mu = 0; mu < nbas; mu++) {
      if (atomOf[mu] >= 0) functionsOf[atomOf[mu]].push_back((int)mu);
   }

   double op = 0.0;
   for (size_t b = 0; b < bonds.size(); b++) {
      const int A = bonds[b].first, B = bonds[b].second;
      if (A < 0 || B < 0 || A >= (int)functionsOf.size() ||
          B >= (int)functionsOf.size()) continue;
      const vector<int>& onA = functionsOf[A];
      const vector<int>& onB = functionsOf[B];
      for (size_t i = 0; i < onA.size(); i++) {
         const int mu = onA[i];
         for (size_t j = 0; j < onB.size(); j++) {
            const int nu = onB[j];
            op += 2.0*coefficients[mu]*coefficients[nu]*overlap[mu*nbas + nu];
         }
      }
   }

   return op;
}


//  --- a symmetric eigensolver ---------------------------------------
//
//  Moved from Huckel.C (#170): rotate away the largest off-diagonal
//  element, repeat. Slow for a big matrix and entirely adequate for
//  one the size of a molecule's valence basis, with no failure mode
//  more exotic than not converging in the sweep limit -- not observed
//  at the sizes this is used at.
bool
MoComposition::jacobiEigen(vector< vector<double> >& a,
                           vector<double>& values,
                           vector< vector<double> >& vectors,
                           std::function<void(double)> onSweep)
{
   const size_t n = a.size();
   vectors.assign(n, vector<double>(n, 0.0));
   for (size_t i = 0; i < n; i++) vectors[i][i] = 1.0;

   //  off0: the first sweep's off-diagonal norm, so later sweeps can be
   //  reported on a log scale -- off falls geometrically, so a linear
   //  fraction would sit near 0% for nearly the whole run.
   double off0 = -1.0;
   for (int sweep = 0; sweep < 100; sweep++) {
      double off = 0.0;
      for (size_t i = 0; i < n; i++) {
         for (size_t j = i + 1; j < n; j++) off += a[i][j]*a[i][j];
      }
      if (sweep == 0) off0 = off;
      if (onSweep) {
         double fraction = 1.0;
         if (off > 0.0 && off0 > 0.0 && off0 > off) {
            fraction = log(off0/off) / log(off0/1.0e-22);
            if (fraction < 0.0) fraction = 0.0;
            if (fraction > 1.0) fraction = 1.0;
         } else if (off0 <= 0.0) {
            fraction = 0.0;
         }
         onSweep(fraction);
      }
      if (off < 1.0e-22) break;

      for (size_t p = 0; p < n; p++) {
         for (size_t q = p + 1; q < n; q++) {
            if (fabs(a[p][q]) < 1.0e-18) continue;

            const double theta = (a[q][q] - a[p][p])/(2.0*a[p][q]);
            const double t = (theta >= 0.0 ? 1.0 : -1.0)
                           / (fabs(theta) + sqrt(theta*theta + 1.0));
            const double c = 1.0/sqrt(t*t + 1.0);
            const double s = t*c;

            for (size_t k = 0; k < n; k++) {
               const double akp = a[k][p], akq = a[k][q];
               a[k][p] = c*akp - s*akq;
               a[k][q] = s*akp + c*akq;
            }
            for (size_t k = 0; k < n; k++) {
               const double apk = a[p][k], aqk = a[q][k];
               a[p][k] = c*apk - s*aqk;
               a[q][k] = s*apk + c*aqk;
            }
            for (size_t k = 0; k < n; k++) {
               const double vkp = vectors[k][p], vkq = vectors[k][q];
               vectors[k][p] = c*vkp - s*vkq;
               vectors[k][q] = s*vkp + c*vkq;
            }
         }
      }
   }

   values.assign(n, 0.0);
   for (size_t i = 0; i < n; i++) values[i] = a[i][i];
   return true;
}


bool
MoComposition::buildSqrtOverlap(const vector<double>& overlap, int nbasis,
                                vector<double>& sqrtOverlap,
                                std::function<void(double)> onSweep)
{
   sqrtOverlap.clear();
   if (nbasis <= 0 || overlap.size() != (size_t)nbasis*(size_t)nbasis) {
      return false;
   }

   const size_t n = (size_t)nbasis;
   vector< vector<double> > a(n, vector<double>(n, 0.0));
   for (size_t i = 0; i < n; i++) {
      for (size_t j = 0; j < n; j++) a[i][j] = overlap[i*n + j];
   }

   vector<double> values;
   vector< vector<double> > vectors;
   jacobiEigen(a, values, vectors, onSweep);

   for (size_t i = 0; i < n; i++) {
      if (values[i] <= 1.0e-8) return false;
   }

   //  S^{1/2} = V * diag(sqrt(lambda)) * V^T, in the eigenvectors S
   //  itself was just diagonalised in -- the POSITIVE power, unlike
   //  Huckel's X = S^-1/2 which uses 1/sqrt(lambda) for the same
   //  eigensystem.
   sqrtOverlap.assign(n*n, 0.0);
   for (size_t i = 0; i < n; i++) {
      for (size_t j = 0; j < n; j++) {
         double sum = 0.0;
         for (size_t k = 0; k < n; k++) {
            sum += vectors[i][k]*vectors[j][k]*sqrt(values[k]);
         }
         sqrtOverlap[i*n + j] = sum;
      }
   }
   return true;
}


vector<double>
MoComposition::lowdinShares(const vector<double>& coefficients,
                            const vector<int>& functionsPerAtom,
                            const vector<double>& sqrtOverlap)
{
   vector<double> out;

   const size_t nbas = coefficients.size();
   if (nbas == 0 || sqrtOverlap.size() != nbas*nbas) return out;

   int total = 0;
   for (size_t i = 0; i < functionsPerAtom.size(); i++) total += functionsPerAtom[i];
   if (total != (int)nbas) return out;

   vector<int> atomOf(nbas, -1);
   {
      int at = 0;
      for (size_t a = 0; a < functionsPerAtom.size(); a++) {
         for (int f = 0; f < functionsPerAtom[a]; f++) atomOf[at + f] = (int)a;
         at += functionsPerAtom[a];
      }
   }

   //  c' = S^{1/2} c
   vector<double> cprime(nbas, 0.0);
   for (size_t mu = 0; mu < nbas; mu++) {
      double sum = 0.0;
      for (size_t nu = 0; nu < nbas; nu++) {
         sum += sqrtOverlap[mu*nbas + nu]*coefficients[nu];
      }
      cprime[mu] = sum;
   }

   out.assign(functionsPerAtom.size(), 0.0);
   double whole = 0.0;
   for (size_t mu = 0; mu < nbas; mu++) {
      const double sq = cprime[mu]*cprime[mu];
      whole += sq;
      if (atomOf[mu] >= 0) out[atomOf[mu]] += sq;
   }
   if (whole <= 0.0) { out.clear(); return out; }

   for (size_t a = 0; a < out.size(); a++) out[a] /= whole;
   return out;
}

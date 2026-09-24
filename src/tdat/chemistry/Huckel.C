//  Extended Huckel: molecular orbitals from a structure alone.
//
//  See include/tdat/Huckel.H for what this is for and, more
//  importantly, what it is not for.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

#include "util/Ecce.H"
#include "tdat/Huckel.H"
#include "tdat/MoFragments.H"

namespace {

const double BOHR = 0.529177210903;          // Angstrom
const double HARTREE = 27.211386245988;      // eV
const double WOLFSBERG = 1.75;

struct Slater
{
   int    n;
   double zetaS;
   double zetaP;
   bool   haveP;

   Slater() : n(0), zetaS(0.0), zetaP(0.0), haveP(false) {}
};

map<string, Slater> s_slater;
bool s_loaded = false;

void loadSlater(void)
{
   if (s_loaded) return;
   s_loaded = true;

   const char* home = getenv("ECCE_HOME");
   string path = string(home ? home : ".") +
                 "/data/client/config/HuckelParameters";

   ifstream in(path.c_str());
   if (!in) return;

   string line;
   while (getline(in, line)) {
      const string::size_type hash = line.find('#');
      if (hash != string::npos) line.erase(hash);

      istringstream parse(line);
      string symbol, sText, pText;
      int n = 0;
      if (!(parse >> symbol >> n >> sText)) continue;

      Slater entry;
      entry.n = n;
      entry.zetaS = atof(sText.c_str());
      if ((parse >> pText) && pText != "-") {
         entry.zetaP = atof(pText.c_str());
         entry.haveP = true;
      }
      s_slater[symbol] = entry;
   }
}


//  --- Gaussian expansions of Slater orbitals ------------------------
//
//  The overlap of two Slater functions has a closed form, and it is
//  long.  Expanding each as three Gaussians instead -- the STO-3G fit,
//  which exists precisely because Gaussian integrals are elementary --
//  gives the same overlaps to well inside what this method's other
//  approximations cost.
//
//  For zeta = 1; every exponent scales as zeta squared.  Only the
//  first two rows: a 3s or 3p needs its own fit, with its own radial
//  node, and using a 2s fit for one would be a quiet lie about the
//  shape of the orbital.  Elements past neon are declined instead.
struct Primitive { double alpha; double coefficient; };

const Primitive STO3G_1S[3] = {
   { 2.22766000, 0.15432897 },
   { 0.40577100, 0.53532814 },
   { 0.10981800, 0.44463454 }
};
const Primitive STO3G_2S[3] = {
   { 0.99420200, -0.09996723 },
   { 0.23103100,  0.39951283 },
   { 0.07513860,  0.70011547 }
};
const Primitive STO3G_2P[3] = {
   { 0.99420200, 0.15591627 },
   { 0.23103100, 0.60768372 },
   { 0.07513860, 0.39195739 }
};


double doubleFactorial(int n)
{
   double out = 1.0;
   for (int k = n; k > 1; k -= 2) out *= k;
   return out;
}

double binomial(int n, int k)
{
   if (k < 0 || k > n) return 0.0;
   double out = 1.0;
   for (int i = 0; i < k; i++) out = out*(n - i)/(i + 1);
   return out;
}

//  The binomial expansion coefficient that Taketa, Huzinaga and
//  O-ohata's overlap formula sums over.
double expansion(int j, int l1, int l2, double a, double b)
{
   double out = 0.0;
   for (int i = 0; i <= l1; i++) {
      const int k = j - i;
      if (k < 0 || k > l2) continue;
      out += binomial(l1, i)*binomial(l2, k)
           * pow(a, l1 - i)*pow(b, l2 - k);
   }
   return out;
}

//  One Cartesian direction of the overlap of two primitive Gaussians.
double overlap1D(int l1, int l2, double pa, double pb, double gamma)
{
   double out = 0.0;
   for (int i = 0; i <= (l1 + l2)/2; i++) {
      out += expansion(2*i, l1, l2, pa, pb)
           * doubleFactorial(2*i - 1)/pow(2.0*gamma, (double)i);
   }
   return out;
}

double primitiveNorm(double alpha, int l, int m, int n)
{
   const double total = l + m + n;
   return pow(2.0*alpha/M_PI, 0.75)
        * pow(4.0*alpha, total/2.0)
        / sqrt(doubleFactorial(2*l - 1)*doubleFactorial(2*m - 1)
               *doubleFactorial(2*n - 1));
}

//  Overlap of two primitive Cartesian Gaussians, normalised.
double primitiveOverlap(double a, const double* A, int la, int ma, int na,
                        double b, const double* B, int lb, int mb, int nb)
{
   const double gamma = a + b;
   double R2 = 0.0, P[3];
   for (int k = 0; k < 3; k++) {
      const double d = A[k] - B[k];
      R2 += d*d;
      P[k] = (a*A[k] + b*B[k])/gamma;
   }

   const double pre = exp(-a*b*R2/gamma)*pow(M_PI/gamma, 1.5);

   const double sx = overlap1D(la, lb, P[0]-A[0], P[0]-B[0], gamma);
   const double sy = overlap1D(ma, mb, P[1]-A[1], P[1]-B[1], gamma);
   const double sz = overlap1D(na, nb, P[2]-A[2], P[2]-B[2], gamma);

   return pre*sx*sy*sz
        * primitiveNorm(a, la, ma, na)*primitiveNorm(b, lb, mb, nb);
}


/** One basis function: which atom, which shell, which component. */
struct Function
{
   int    atom;
   int    l;                  // 0 or 1
   int    component;          // 0,1,2 = x,y,z for l == 1
   double zeta;
   int    n;
   double energy;             // H_ii, Hartree
};


double contractedOverlap(const Function& f, const double* A,
                         const Function& g, const double* B)
{
   const Primitive *pf = (f.l == 1) ? STO3G_2P
                                    : ((f.n == 1) ? STO3G_1S : STO3G_2S);
   const Primitive *pg = (g.l == 1) ? STO3G_2P
                                    : ((g.n == 1) ? STO3G_1S : STO3G_2S);

   int la[3] = { 0, 0, 0 }, lb[3] = { 0, 0, 0 };
   if (f.l == 1) la[f.component] = 1;
   if (g.l == 1) lb[g.component] = 1;

   double out = 0.0;
   for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
         const double a = pf[i].alpha*f.zeta*f.zeta;
         const double b = pg[j].alpha*g.zeta*g.zeta;
         out += pf[i].coefficient*pg[j].coefficient
              * primitiveOverlap(a, A, la[0], la[1], la[2],
                                 b, B, lb[0], lb[1], lb[2]);
      }
   }
   return out;
}


//  --- a symmetric eigensolver ---------------------------------------
//
//  Jacobi: rotate away the largest off-diagonal element, repeat.  Slow
//  for a big matrix and entirely adequate for one the size of a
//  molecule's valence basis, with the advantage of being short enough
//  to read and having no failure mode more exotic than not converging,
//  which is reported.
bool jacobi(vector< vector<double> >& a, vector<double>& values,
            vector< vector<double> >& vectors)
{
   const size_t n = a.size();
   vectors.assign(n, vector<double>(n, 0.0));
   for (size_t i = 0; i < n; i++) vectors[i][i] = 1.0;

   for (int sweep = 0; sweep < 100; sweep++) {
      double off = 0.0;
      for (size_t i = 0; i < n; i++) {
         for (size_t j = i + 1; j < n; j++) off += a[i][j]*a[i][j];
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

}  // namespace


bool Huckel::canSolve(const vector<string>& elements, string& missing)
{
   loadSlater();
   missing.clear();

   for (size_t i = 0; i < elements.size(); i++) {
      map<string, Slater>::const_iterator it = s_slater.find(elements[i]);
      double unused;
      if (it == s_slater.end() ||
          !MoFragments::valenceEnergy(elements[i], 0, unused)) {
         if (!missing.empty()) missing += ", ";
         missing += elements[i];
      } else if (it->second.n > 2) {
         //  A 3s or 3p needs its own Gaussian fit, with its own radial
         //  node.  Using a 2s fit for one would be a quiet lie about
         //  the shape of the orbital, so it declines.
         if (!missing.empty()) missing += ", ";
         missing += elements[i] + " (past the second row)";
      }
   }
   return missing.empty();
}


bool Huckel::solve(const vector<double>& coords,
                   const vector<string>& elements,
                   int charge,
                   vector<double>& energies,
                   vector<double>& occupancies,
                   vector< vector<double> >& coefficients,
                   vector<int>& perAtom,
                   vector<int>& shellOf,
                   string& why,
                   vector<double>* overlap)
{
   if (overlap != 0) overlap->clear();
   why.clear();
   energies.clear();
   occupancies.clear();
   coefficients.clear();
   perAtom.clear();
   shellOf.clear();

   loadSlater();

   string missing;
   if (!canSolve(elements, missing)) {
      why = "No extended Huckel parameters for " + missing + ".";
      return false;
   }
   if (coords.size() != 3*elements.size() || elements.empty()) {
      why = "The coordinates and the elements do not describe one molecule.";
      return false;
   }

   //  --- the basis ---------------------------------------------------
   vector<Function> basis;
   vector<double> centres;                  // bohr, one per function

   for (size_t a = 0; a < elements.size(); a++) {
      const Slater& slater = s_slater[elements[a]];
      perAtom.push_back(slater.haveP ? 4 : 1);

      double eV = 0.0;
      MoFragments::valenceEnergy(elements[a], 0, eV);

      Function s;
      s.atom = (int)a; s.l = 0; s.component = 0;
      s.zeta = slater.zetaS; s.n = slater.n;
      s.energy = eV/HARTREE;
      basis.push_back(s);
      shellOf.push_back(0);
      for (int k = 0; k < 3; k++) centres.push_back(coords[3*a+k]/BOHR);

      if (slater.haveP) {
         double pEv = 0.0;
         MoFragments::valenceEnergy(elements[a], 1, pEv);
         for (int comp = 0; comp < 3; comp++) {
            Function p;
            p.atom = (int)a; p.l = 1; p.component = comp;
            p.zeta = slater.zetaP; p.n = slater.n;
            p.energy = pEv/HARTREE;
            basis.push_back(p);
            shellOf.push_back(1);
            for (int k = 0; k < 3; k++) {
               centres.push_back(coords[3*a+k]/BOHR);
            }
         }
      }
   }

   const size_t n = basis.size();
   if (n == 0) { why = "No basis functions."; return false; }

   //  --- overlap and Hamiltonian --------------------------------------
   vector< vector<double> > S(n, vector<double>(n, 0.0));
   for (size_t i = 0; i < n; i++) {
      for (size_t j = i; j < n; j++) {
         const double v = contractedOverlap(basis[i], &centres[3*i],
                                            basis[j], &centres[3*j]);
         S[i][j] = v;
         S[j][i] = v;
      }
   }

   //  Normalised, since the contraction coefficients are for the
   //  Slater function they fit and not for this combination of them.
   vector<double> scale(n, 1.0);
   for (size_t i = 0; i < n; i++) {
      if (S[i][i] <= 0.0) { why = "A basis function has no norm."; return false; }
      scale[i] = 1.0/sqrt(S[i][i]);
   }
   for (size_t i = 0; i < n; i++) {
      for (size_t j = 0; j < n; j++) S[i][j] *= scale[i]*scale[j];
   }

   vector< vector<double> > H(n, vector<double>(n, 0.0));
   for (size_t i = 0; i < n; i++) {
      H[i][i] = basis[i].energy;
      for (size_t j = i + 1; j < n; j++) {
         //  Wolfsberg-Helmholz.  The off-diagonal is the mean of the
         //  two atomic levels, scaled by how much the orbitals
         //  actually overlap -- which is the entire physical content
         //  of the method.
         const double v = WOLFSBERG*S[i][j]
                        * (basis[i].energy + basis[j].energy)/2.0;
         H[i][j] = v;
         H[j][i] = v;
      }
   }

   //  --- H C = S C E, by symmetric orthogonalisation -------------------
   vector< vector<double> > work = S;
   vector<double> sValues;
   vector< vector<double> > sVectors;
   jacobi(work, sValues, sVectors);

   for (size_t i = 0; i < n; i++) {
      if (sValues[i] < 1.0e-8) {
         why = "The basis is linearly dependent at this geometry.";
         return false;
      }
   }

   //  X = S^-1/2, in the basis its own eigenvectors define.
   vector< vector<double> > X(n, vector<double>(n, 0.0));
   for (size_t i = 0; i < n; i++) {
      for (size_t j = 0; j < n; j++) {
         double sum = 0.0;
         for (size_t k = 0; k < n; k++) {
            sum += sVectors[i][k]*sVectors[j][k]/sqrt(sValues[k]);
         }
         X[i][j] = sum;
      }
   }

   //  H' = X H X
   vector< vector<double> > tmp(n, vector<double>(n, 0.0));
   for (size_t i = 0; i < n; i++) {
      for (size_t j = 0; j < n; j++) {
         double sum = 0.0;
         for (size_t k = 0; k < n; k++) sum += X[i][k]*H[k][j];
         tmp[i][j] = sum;
      }
   }
   vector< vector<double> > Hp(n, vector<double>(n, 0.0));
   for (size_t i = 0; i < n; i++) {
      for (size_t j = 0; j < n; j++) {
         double sum = 0.0;
         for (size_t k = 0; k < n; k++) sum += tmp[i][k]*X[k][j];
         Hp[i][j] = sum;
      }
   }

   vector<double> eValues;
   vector< vector<double> > eVectors;
   jacobi(Hp, eValues, eVectors);

   //  C = X C', and in ascending energy.
   vector<size_t> order;
   for (size_t i = 0; i < n; i++) order.push_back(i);
   for (size_t i = 1; i < n; i++) {
      for (size_t j = i; j > 0 && eValues[order[j]] < eValues[order[j-1]]; j--) {
         const size_t swap = order[j];
         order[j] = order[j-1];
         order[j-1] = swap;
      }
   }

   for (size_t k = 0; k < n; k++) {
      const size_t which = order[k];
      energies.push_back(eValues[which]);

      //  IN THE ORTHOGONALISED BASIS, WHICH IS WHAT CALLERS ASSUME.
      //
      //  The eigenvectors of H' are coefficients on Lowdin-orthogonalised
      //  functions; multiplying by X would give coefficients on the raw
      //  atomic orbitals, which overlap each other.  Everything that
      //  reads these treats a coefficient squared as a population and
      //  a dot product as an overlap, and both of those are only true
      //  in an orthonormal basis -- MOPAC reports its vectors this way
      //  for the same reason.
      //
      //  Taking the non-orthogonal ones put ammonia's nodeless a1
      //  combination of three hydrogens ABOVE its e pair, which is
      //  backwards: an in-phase combination of neighbours cannot lie
      //  above one with a node in it.
      //  On the atomic orbitals themselves.  They are not orthogonal
      //  to each other, so a caller forming an overlap with some
      //  combination of them needs S as well -- which is why it is
      //  handed back.
      vector<double> row(n, 0.0);
      for (size_t i = 0; i < n; i++) {
         double sum = 0.0;
         for (size_t j = 0; j < n; j++) sum += X[i][j]*eVectors[j][which];
         row[i] = sum*scale[i];
      }

      //  Unit norm, which is what anything reading these expects.
      double norm = 0.0;
      for (size_t i = 0; i < n; i++) norm += row[i]*row[i];
      if (norm > 0.0) {
         norm = sqrt(norm);
         for (size_t i = 0; i < n; i++) row[i] /= norm;
      }
      coefficients.push_back(row);
   }

   if (overlap != 0) {
      //  In terms of the functions the caller is told about, which
      //  the working matrix was rescaled away from.
      overlap->assign(n*n, 0.0);
      for (size_t i = 0; i < n; i++) {
         for (size_t j = 0; j < n; j++) {
            (*overlap)[i*n + j] = S[i][j]/(scale[i]*scale[j]);
         }
      }
   }

   //  --- electrons -----------------------------------------------------
   int electrons = 0;
   for (size_t a = 0; a < elements.size(); a++) {
      electrons += MoFragments::valenceElectrons(elements[a]);
   }
   electrons -= charge;

   for (size_t k = 0; k < n; k++) {
      const double take = (electrons >= 2) ? 2.0
                                           : ((electrons == 1) ? 1.0 : 0.0);
      occupancies.push_back(take);
      electrons -= (int)take;
   }

   return true;
}

//  Fragment orbitals from extended Hueckel and MO composition in them.
//  See include/tdat/FragmentOrbitals.H.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

#include "tdat/FragmentOrbitals.H"
#include "tdat/MoComposition.H"
#include "tdat/MoFragments.H"
#include "tdat/SlaterExpansion.H"

using namespace std;

namespace {

const double BOHR = 0.529177210903;          // Angstrom
const double HARTREE = 27.211386245988;      // eV
const double KAPPA = 1.75;                   // YAeHMOP THE_CONST
const int    STO_GAUSSIANS = 6;
const double MIN_FIT = 0.999;                // refuse a poorer STO fit

struct Shell
{
   int n, l;
   double hii;                // eV
   double z1, z2, c1, c2;
};

map< string, vector<Shell> > s_params;
bool s_loaded = false;

const char* const SYMBOLS[] = {
   "", "H","He","Li","Be","B","C","N","O","F","Ne","Na","Mg","Al","Si","P",
   "S","Cl","Ar","K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn",
   "Ga","Ge","As","Se","Br","Kr","Rb","Sr","Y","Zr","Nb","Mo","Tc","Ru",
   "Rh","Pd","Ag","Cd","In","Sn","Sb","Te","I","Xe","Cs","Ba","La","Ce",
   "Pr","Nd","Pm","Sm","Eu","Gd","Tb","Dy","Ho","Er","Tm","Yb","Lu","Hf",
   "Ta","W","Re","Os","Ir","Pt","Au","Hg","Tl","Pb","Bi","Po","At","Rn"
};
const int NSYMBOLS = (int)(sizeof(SYMBOLS)/sizeof(SYMBOLS[0]));

int atomicNumber(const string& el)
{
   for (int z = 1; z < NSYMBOLS; z++) if (el == SYMBOLS[z]) return z;
   return 0;
}

bool hasShell(const vector<Shell>& v, int l)
{
   for (size_t i = 0; i < v.size(); i++) if (v[i].l == l) return true;
   return false;
}

//  Valence electrons as the group number; a filled d shell that has no
//  parameters (Zn, Cd; Hg's 5d is in the file) is core.
int valenceCount(const string& el)
{
   const int z = atomicNumber(el);
   int group;
   if (z == 0) return 0;
   else if (z <= 2) return z;
   else if (z <= 10) group = z - 2;
   else if (z <= 18) group = z - 10;
   else if (z <= 36) group = z - 18;
   else if (z <= 54) group = z - 36;
   else if (z <= 57) group = z - 54;
   else if (z <= 71) return 0;                 // lanthanides: not handled
   else if (z <= 86) group = z - 68;
   else return 0;
   if (group >= 13) group -= 10;               // p block: s2 p(group-12)
   if (FragmentOrbitals::isTransitionMetal(el)) {
      //  Zn and Cd have no d parameters: the d10 shell is core.
      map< string, vector<Shell> >::const_iterator it = s_params.find(el);
      if (it == s_params.end() || !hasShell(it->second, 2)) return 2;
   }
   return group;
}

//  --- real functions -------------------------------------------------

struct Term { int px, py, pz; double c; };

vector<Term> angular(int l, int comp, string& name)
{
   vector<Term> t;
   const double r3 = sqrt(3.0);
   if (l == 0) { t.push_back({0,0,0,1.0}); name = "s"; }
   else if (l == 1) {
      const int k[3][3] = { {1,0,0}, {0,1,0}, {0,0,1} };
      t.push_back({k[comp][0], k[comp][1], k[comp][2], 1.0});
      name = comp == 0 ? "x" : (comp == 1 ? "y" : "z");
   } else {
      switch (comp) {
        case 0: t.push_back({0,0,2,1.0}); t.push_back({2,0,0,-0.5});
                t.push_back({0,2,0,-0.5}); name = "z2"; break;
        case 1: t.push_back({2,0,0,r3/2}); t.push_back({0,2,0,-r3/2});
                name = "x2-y2"; break;
        case 2: t.push_back({1,1,0,r3}); name = "xy"; break;
        case 3: t.push_back({1,0,1,r3}); name = "xz"; break;
        default: t.push_back({0,1,1,r3}); name = "yz"; break;
      }
   }
   return t;
}

void normalise(EspBasisFunction& f)
{
   const double s = EspField::overlapOf(f, f);
   if (s <= 0.0) return;
   const double k = 1.0/sqrt(s);
   for (size_t i = 0; i < f.contraction.size(); i++) f.contraction[i] *= k;
}

struct FitKey
{
   int n, l; double z;
   bool operator<(const FitKey& o) const {
      if (n != o.n) return n < o.n;
      if (l != o.l) return l < o.l;
      return z < o.z;
   }
};
struct FitVal { vector<double> a, c; double quality; };
map<FitKey, FitVal> s_fits;

const FitVal* fitFor(int n, int l, double z)
{
   FitKey k = { n, l, z };
   map<FitKey, FitVal>::iterator it = s_fits.find(k);
   if (it != s_fits.end()) return &it->second;
   FitVal v;
   v.quality = SlaterExpansion::fit(n, l, z, STO_GAUSSIANS, v.a, v.c);
   return &(s_fits[k] = v);
}

bool makeFunction(const Shell& sh, int comp, const double* centreBohr,
                  EspBasisFunction& f, string& name, double& quality)
{
   const vector<Term> terms = angular(sh.l, comp, name);
   const double zs[2] = { sh.z1, sh.z2 };
   const double cs[2] = { sh.c1, sh.c2 };
   const int nz = (sh.z2 > 0.0) ? 2 : 1;

   f = EspBasisFunction();
   for (int k = 0; k < 3; k++) f.center[k] = centreBohr[k];
   for (size_t t = 0; t < terms.size(); t++) {
      f.powerX.push_back(terms[t].px);
      f.powerY.push_back(terms[t].py);
      f.powerZ.push_back(terms[t].pz);
      f.angularCoef.push_back(terms[t].c);
   }
   for (int z = 0; z < nz; z++) {
      const FitVal* v = fitFor(sh.n, sh.l, zs[z]);
      if (v->quality < MIN_FIT) return false;
      quality = std::min(quality, v->quality);
      EspBasisFunction one = f;
      one.exponent = v->a;
      one.contraction = v->c;
      normalise(one);
      const double w = (nz == 2) ? cs[z] : 1.0;
      for (size_t i = 0; i < one.exponent.size(); i++) {
         f.exponent.push_back(one.exponent[i]);
         f.contraction.push_back(w*one.contraction[i]);
      }
   }
   normalise(f);
   return true;
}

//  --- linear algebra on small dense matrices ---------------------------

typedef vector< vector<double> > Mat;

//  S^(-1/2) from the eigen-decomposition; false when S is singular.
bool inverseSqrt(const Mat& S, Mat& X, double floor)
{
   const size_t n = S.size();
   Mat work = S, vecs;
   vector<double> vals;
   MoComposition::jacobiEigen(work, vals, vecs);
   for (size_t i = 0; i < n; i++) if (vals[i] < floor) return false;
   X.assign(n, vector<double>(n, 0.0));
   for (size_t i = 0; i < n; i++)
     for (size_t j = 0; j < n; j++) {
        double s = 0.0;
        for (size_t k = 0; k < n; k++) s += vecs[i][k]*vecs[j][k]/sqrt(vals[k]);
        X[i][j] = s;
     }
   return true;
}

Mat mul(const Mat& A, const Mat& B)
{
   const size_t n = A.size(), m = B[0].size(), p = B.size();
   Mat C(n, vector<double>(m, 0.0));
   for (size_t i = 0; i < n; i++)
     for (size_t k = 0; k < p; k++) {
        const double a = A[i][k];
        if (a == 0.0) continue;
        for (size_t j = 0; j < m; j++) C[i][j] += a*B[k][j];
     }
   return C;
}

//  H C = S C E; C columns normalised c.S.c = 1, ascending E.
bool generalised(const Mat& H, const Mat& S, vector<double>& E, Mat& C)
{
   const size_t n = S.size();
   Mat X;
   if (!inverseSqrt(S, X, 1.0e-8)) return false;
   Mat Hp = mul(mul(X, H), X);
   for (size_t i = 0; i < n; i++)
     for (size_t j = i + 1; j < n; j++) {
        const double m = 0.5*(Hp[i][j] + Hp[j][i]);
        Hp[i][j] = Hp[j][i] = m;
     }
   vector<double> vals; Mat vecs;
   MoComposition::jacobiEigen(Hp, vals, vecs);
   vector<size_t> order(n);
   for (size_t i = 0; i < n; i++) order[i] = i;
   std::stable_sort(order.begin(), order.end(),
                    [&](size_t a, size_t b) { return vals[a] < vals[b]; });
   Mat Cp(n, vector<double>(n, 0.0));
   E.assign(n, 0.0);
   for (size_t k = 0; k < n; k++) {
      E[k] = vals[order[k]];
      for (size_t i = 0; i < n; i++) Cp[i][k] = vecs[i][order[k]];
   }
   C = mul(X, Cp);
   return true;
}

//  w_k = |<FO~_k|psi>|^2 for psi = c normalised in the parent basis;
//  X = Sff^-1/2, F[k][mu] = <FO_k|chi_mu>.  False if c has no norm.
bool loewdinWeights(const Mat& F, const Mat& X, const vector<double>& S,
                    const vector<double>& c, vector<double>& w, double& norm)
{
   const size_t nP = c.size(), nk = F.size();
   norm = 0.0;
   for (size_t i = 0; i < nP; i++) {
      double sc = 0.0;
      for (size_t j = 0; j < nP; j++) sc += S[i*nP+j]*c[j];
      norm += c[i]*sc;
   }
   if (norm <= 1.0e-12) return false;
   const double inv = 1.0/sqrt(norm);
   vector<double> b(nk, 0.0);
   for (size_t k = 0; k < nk; k++) {
      double s = 0.0;
      for (size_t m = 0; m < nP; m++) s += F[k][m]*c[m];
      b[k] = s*inv;
   }
   w.assign(nk, 0.0);
   for (size_t k = 0; k < nk; k++) {
      double t = 0.0;
      for (size_t l = 0; l < nk; l++) t += X[k][l]*b[l];
      w[k] = t*t;
   }
   return true;
}

//  Weighted Wolfsberg-Helmholz (Ammeter, Buergi, Thibeault, Hoffmann,
//  JACS 100, 3686 (1978)), the form YAeHMOP uses by default.
double hij(double hi, double hj, double sij)
{
   const double sum = hi + hj;
   double d2 = 0.0;
   if (sum != 0.0) { const double d = (hi - hj)/sum; d2 = d*d; }
   const double K = KAPPA + d2 + d2*d2*(1.0 - KAPPA);
   return 0.5*K*sum*sij;
}

string dShellName(int n, int l)
{
   char b[8];
   snprintf(b, sizeof b, "%d%c", n, "spdf"[l]);
   return b;
}

}  // namespace


bool FragmentOrbitals::loadParameters(const string& pathIn)
{
   s_params.clear();
   s_loaded = true;
   string path = pathIn;
   if (path.empty()) {
      const char* over = getenv("ECCE_EHT_PARAMETERS");
      const char* home = getenv("ECCE_HOME");
      path = over ? over : string(home ? home : ".")
                           + "/data/client/config/EHTParameters";
   }
   ifstream in(path.c_str());
   if (!in) return false;
   string line;
   while (getline(in, line)) {
      const string::size_type h = line.find('#');
      if (h != string::npos) line.erase(h);
      istringstream p(line);
      string sym; Shell s;
      if (!(p >> sym >> s.n >> s.l >> s.hii >> s.z1 >> s.z2 >> s.c1 >> s.c2))
         continue;
      s_params[sym].push_back(s);
   }
   return !s_params.empty();
}

bool FragmentOrbitals::isTransitionMetal(const string& el)
{
   const int z = atomicNumber(el);
   return (z >= 21 && z <= 30) || (z >= 39 && z <= 48) || (z >= 72 && z <= 80)
          || z == 57;
}

bool FragmentOrbitals::canSolve(const vector<string>& elements, string& missing)
{
   if (!s_loaded) loadParameters();
   missing.clear();
   for (size_t i = 0; i < elements.size(); i++) {
      map< string, vector<Shell> >::const_iterator it =
          s_params.find(elements[i]);
      const bool tm = isTransitionMetal(elements[i]);
      const bool ok = it != s_params.end() && hasShell(it->second, 0) &&
          (atomicNumber(elements[i]) <= 2 || hasShell(it->second, 1)) &&
          (!tm || hasShell(it->second, 2));
      if (!ok && missing.find(elements[i]) == string::npos) {
         if (!missing.empty()) missing += ", ";
         missing += elements[i];
      }
   }
   return missing.empty();
}


bool FragmentOrbitals::autoFragments(const vector<string>& symbols,
                                     const vector<double>& coordsAng,
                                     int totalCharge, int multiplicity,
                                     vector<Fragment>& out, string& why)
{
   out.clear();
   why.clear();
   const size_t n = symbols.size();
   if (n == 0 || coordsAng.size() != 3*n) {
      why = "No atoms.";
      return false;
   }
   if (!s_loaded) loadParameters();

   vector<int> metals;
   for (size_t i = 0; i < n; i++)
      if (isTransitionMetal(symbols[i])) metals.push_back((int)i);
   if (metals.empty()) {
      why = "Not applicable: no transition metal.";
      return false;
   }

   vector< std::pair<int,int> > bonds;
   MoFragments::covalentBonds(coordsAng, symbols, bonds);

   vector<int> comp(n, -1);
   int ncomp = 0;
   vector<bool> isMetal(n, false);
   for (size_t k = 0; k < metals.size(); k++) isMetal[metals[k]] = true;
   for (size_t s = 0; s < n; s++) {
      if (isMetal[s] || comp[s] >= 0) continue;
      vector<int> stack(1, (int)s);
      comp[s] = ncomp;
      while (!stack.empty()) {
         const int a = stack.back(); stack.pop_back();
         for (size_t b = 0; b < bonds.size(); b++) {
            int o = -1;
            if (bonds[b].first == a) o = bonds[b].second;
            else if (bonds[b].second == a) o = bonds[b].first;
            if (o < 0 || isMetal[o] || comp[o] >= 0) continue;
            comp[o] = ncomp;
            stack.push_back(o);
         }
      }
      ncomp++;
   }

   //  Formula of a ligand in Hill-ish order of appearance, for its name.
   vector<Fragment> ligands;
   int ligandCharge = 0;
   for (int c = 0; c < ncomp; c++) {
      Fragment f;
      f.kind = LIGAND;
      map<string, int> count;
      vector<string> order;
      int electrons = 0;
      for (size_t i = 0; i < n; i++) {
         if (comp[i] != c) continue;
         f.atoms.push_back((int)i);
         if (!count[symbols[i]]++) order.push_back(symbols[i]);
         electrons += atomicNumber(symbols[i]);
      }
      f.charge = (electrons % 2) ? -1 : 0;
      ligandCharge += f.charge;
      for (size_t k = 0; k < order.size(); k++) {
         f.name += order[k];
         if (count[order[k]] > 1) f.name += std::to_string(count[order[k]]);
      }
      ligands.push_back(f);
   }
   //  Same-formula ligands are told apart by a running number.
   map<string, int> seen, total;
   for (size_t i = 0; i < ligands.size(); i++) total[ligands[i].name]++;
   for (size_t i = 0; i < ligands.size(); i++)
      if (total[ligands[i].name] > 1)
         ligands[i].name += " (" + std::to_string(++seen[ligands[i].name]) + ")";

   for (size_t k = 0; k < metals.size(); k++) {
      Fragment f;
      f.kind = METAL;
      f.name = symbols[metals[k]];
      f.atoms.push_back(metals[k]);
      f.charge = (k == 0) ? totalCharge - ligandCharge : 0;
      f.multiplicity = multiplicity;
      out.push_back(f);
   }
   for (size_t i = 0; i < ligands.size(); i++) out.push_back(ligands[i]);

   if (!ligands.empty()) {
      Fragment shell;
      shell.kind = SHELL;
      shell.name = "ligands";
      shell.charge = ligandCharge;
      for (size_t i = 0; i < n; i++) if (!isMetal[i]) shell.atoms.push_back((int)i);
      out.push_back(shell);
   }
   return true;
}


bool FragmentOrbitals::build(const vector<string>& symbols,
                             const vector<double>& coordsAng,
                             const vector<Fragment>& fragments,
                             LigandSet ligands, FoSet& out, string& why)
{
   out = FoSet();
   why.clear();
   if (!s_loaded) loadParameters();
   if (s_params.empty()) {
      why = "The extended Hueckel parameter file could not be read.";
      return false;
   }
   if (coordsAng.size() != 3*symbols.size()) {
      why = "The coordinates and the elements do not match.";
      return false;
   }
   string missing;
   if (!canSolve(symbols, missing)) {
      why = "No extended Hueckel parameters for " + missing + ".";
      return false;
   }

   //  Fragments used: metals always; ligands per-ligand or as one shell.
   for (size_t i = 0; i < fragments.size(); i++) {
      const Fragment& f = fragments[i];
      if (f.kind == METAL ||
          (f.kind == LIGAND && ligands == PER_LIGAND) ||
          (f.kind == SHELL && ligands == LIGAND_SHELL))
         out.fragments.push_back(f);
   }
   if (out.fragments.empty()) {
      why = "No fragments.";
      return false;
   }

   //  Functions, fragment by fragment.
   vector< vector<int> > fragFuncs(out.fragments.size());
   for (size_t fi = 0; fi < out.fragments.size(); fi++) {
      const Fragment& frag = out.fragments[fi];
      for (size_t k = 0; k < frag.atoms.size(); k++) {
         const int a = frag.atoms[k];
         if (a < 0 || a >= (int)symbols.size()) {
            why = "A fragment names an atom that does not exist.";
            return false;
         }
         const vector<Shell>& shells = s_params[symbols[a]];
         double centre[3];
         for (int q = 0; q < 3; q++) centre[q] = coordsAng[3*a+q]/BOHR;
         for (size_t s = 0; s < shells.size(); s++) {
            const int nc = 2*shells[s].l + 1;
            for (int c = 0; c < nc; c++) {
               Function fn;
               fn.atom = a; fn.element = symbols[a];
               fn.n = shells[s].n; fn.l = shells[s].l;
               fn.hii = shells[s].hii/HARTREE;
               if (!makeFunction(shells[s], c, centre, fn.fn, fn.component,
                                 out.worstFit)) {
                  why = "The Slater fit for " + symbols[a] + " " +
                        dShellName(fn.n, fn.l) + " is too poor to use.";
                  return false;
               }
               fragFuncs[fi].push_back((int)out.funcs.size());
               out.funcs.push_back(fn);
            }
         }
      }
   }

   const size_t nf = out.funcs.size();
   out.overlap.assign(nf*nf, 0.0);
   for (size_t i = 0; i < nf; i++)
     for (size_t j = i; j < nf; j++) {
        const double v = EspField::overlapOf(out.funcs[i].fn, out.funcs[j].fn);
        out.overlap[i*nf + j] = out.overlap[j*nf + i] = v;
     }

   //  Fragment orbitals.
   for (size_t fi = 0; fi < out.fragments.size(); fi++) {
      const Fragment& frag = out.fragments[fi];
      const vector<int>& idx = fragFuncs[fi];
      const size_t m = idx.size();

      vector<double> E;
      Mat C;
      if (frag.kind == METAL) {
         //  The atom's own AOs, each its own FO, energy H_ii.
         E.resize(m);
         C.assign(m, vector<double>(m, 0.0));
         for (size_t k = 0; k < m; k++) {
            E[k] = out.funcs[idx[k]].hii;
            C[k][k] = 1.0;
         }
         vector<size_t> order(m);
         for (size_t i = 0; i < m; i++) order[i] = i;
         std::stable_sort(order.begin(), order.end(),
             [&](size_t a, size_t b) { return E[a] < E[b]; });
         vector<double> E2(m); Mat C2(m, vector<double>(m, 0.0));
         for (size_t k = 0; k < m; k++) {
            E2[k] = E[order[k]];
            C2[order[k]][k] = 1.0;
         }
         E = E2; C = C2;
      } else {
         Mat S(m, vector<double>(m)), H(m, vector<double>(m));
         for (size_t i = 0; i < m; i++)
           for (size_t j = 0; j < m; j++)
              S[i][j] = out.overlap[idx[i]*nf + idx[j]];
         for (size_t i = 0; i < m; i++)
           for (size_t j = 0; j < m; j++)
              H[i][j] = (i == j) ? out.funcs[idx[i]].hii
                  : hij(out.funcs[idx[i]].hii, out.funcs[idx[j]].hii, S[i][j]);
         if (!generalised(H, S, E, C)) {
            why = "The basis of fragment " + frag.name +
                  " is linearly dependent at this geometry.";
            return false;
         }
      }

      int electrons = -frag.charge;
      for (size_t k = 0; k < frag.atoms.size(); k++)
         electrons += valenceCount(symbols[frag.atoms[k]]);

      for (size_t k = 0; k < m; k++) {
         FO fo;
         fo.fragment = (int)fi;
         fo.index = (int)k + 1;
         fo.energy = E[k];
         fo.coef.assign(nf, 0.0);
         for (size_t i = 0; i < m; i++) fo.coef[idx[i]] = C[i][k];
         if (frag.kind == METAL) {
            fo.occupancy = 0.0;       // an atom's shells are not filled here
         } else {
            fo.occupancy = (electrons >= 2) ? 2.0 : (electrons == 1 ? 1.0 : 0.0);
            electrons -= (int)fo.occupancy;
         }

         //  Mulliken content by (element, n, l), largest two.
         map<string, double> by;
         double tot = 0.0;
         for (size_t i = 0; i < nf; i++) {
            if (fo.coef[i] == 0.0) continue;
            double sc = 0.0;
            for (size_t j = 0; j < nf; j++) sc += out.overlap[i*nf+j]*fo.coef[j];
            const double w = fo.coef[i]*sc;
            by[out.funcs[i].element + " " +
               dShellName(out.funcs[i].n, out.funcs[i].l)] += w;
            tot += w;
         }
         vector< std::pair<double,string> > rank;
         for (map<string,double>::iterator b = by.begin(); b != by.end(); ++b)
            rank.push_back(std::make_pair(-b->second, b->first));
         std::sort(rank.begin(), rank.end());
         for (size_t r = 0; r < rank.size() && r < 2; r++) {
            const double pct = 100.0*(-rank[r].first)/(tot > 0 ? tot : 1.0);
            if (r > 0 && pct < 15.0) break;
            char b[64];
            snprintf(b, sizeof b, "%s%s %.0f%%", r ? ", " : "",
                     rank[r].second.c_str(), pct);
            fo.character += b;
         }
         out.fos.push_back(fo);
      }
   }
   return true;
}


string FragmentOrbitals::classOf(double capture, double metal, double ligand)
{
   if (capture < CORE_CAPTURE) return "core";
   if (metal >= SIDE_FRACTION*capture) return "metal";
   if (metal >= CENTRED_FRACTION*capture) return "metal-centred";
   if (ligand >= SIDE_FRACTION*capture) return "ligand";
   return "mixed";
}


bool FragmentOrbitals::hii(const string& element, int l, double& eV)
{
   if (!s_loaded) loadParameters();
   map< string, vector<Shell> >::const_iterator it = s_params.find(element);
   if (it == s_params.end()) return false;
   for (size_t i = 0; i < it->second.size(); i++) {
      if (it->second[i].l == l) { eV = it->second[i].hii; return true; }
   }
   return false;
}


static string baseNameOf(const string& n)
{
   const string::size_type p = n.find(" (");
   return p == string::npos ? n : n.substr(0, p);
}


void FragmentOrbitals::groupFos(const FoSet& set, vector< vector<int> >& groups)
{
   groups.clear();
   const size_t nk = set.fos.size();
   for (size_t k = 0; k < nk; k++) {
      const FO& fk = set.fos[k];
      const Fragment& frk = set.fragments[fk.fragment];
      bool placed = false;
      for (size_t g = 0; g < groups.size() && !placed; g++) {
         const FO& ref = set.fos[groups[g][0]];
         const Fragment& frr = set.fragments[ref.fragment];
         const bool same = ref.fragment == fk.fragment ||
             (frr.kind != METAL && frk.kind != METAL &&
              baseNameOf(frr.name) == baseNameOf(frk.name));
         if (same && fabs(ref.energy - fk.energy) < DEGENERATE_EH) {
            groups[g].push_back((int)k);
            placed = true;
         }
      }
      if (!placed) groups.push_back(vector<int>(1, (int)k));
   }
}


FragmentOrbitals::LevelShare
FragmentOrbitals::describeGroup(const FoSet& set, const vector<int>& members)
{
   LevelShare ls;
   const FO& f0 = set.fos[members[0]];
   const Fragment& fr = set.fragments[f0.fragment];
   ls.fragment = f0.fragment;
   ls.energy = f0.energy*HARTREE;
   ls.share = 0.0;
   char buf[96];
   if (fr.kind == METAL) {
      //  The group's shell: any member's function names it.
      int fnIdx = 0;
      for (size_t a = 0; a < f0.coef.size(); a++)
         if (f0.coef[a] != 0.0) { fnIdx = (int)a; break; }
      snprintf(buf, sizeof buf, "%s %s", fr.name.c_str(),
               dShellName(set.funcs[fnIdx].n, set.funcs[fnIdx].l).c_str());
      ls.label = buf;
   } else {
      int i0 = f0.index, i1 = f0.index;
      for (size_t q = 1; q < members.size(); q++) {
         const FO& fq = set.fos[members[q]];
         i0 = std::min(i0, fq.index);
         i1 = std::max(i1, fq.index);
      }
      const string nm = baseNameOf(fr.name);
      const char* occ = f0.occupancy > 0.5 ? "occ" : "virt";
      if (i0 == i1)
         snprintf(buf, sizeof buf, "%s FO%d (%s)", nm.c_str(), i0, occ);
      else
         snprintf(buf, sizeof buf, "%s FO%d-%d (%s)", nm.c_str(), i0, i1, occ);
      ls.label = buf;
      ls.character = f0.character;
   }
   return ls;
}


bool FragmentOrbitals::compose(const FoSet& set,
                               const vector<EspBasisFunction>& parent,
                               const vector<double>& S,
                               const vector< vector<double> >& rows,
                               vector<MoShares>& out, string& why)
{
   out.clear();
   why.clear();
   const size_t nP = parent.size(), nf = set.funcs.size(), nk = set.fos.size();
   if (nP == 0 || nk == 0 || S.size() != nP*nP) {
      why = "The parent basis and its overlap matrix do not match.";
      return false;
   }
   for (size_t r = 0; r < rows.size(); r++) {
      if (rows[r].size() != nP) {
         why = "An MO row does not match the parent basis size.";
         return false;
      }
   }

   //  <f_a | chi_mu>
   Mat Smix(nf, vector<double>(nP, 0.0));
   for (size_t a = 0; a < nf; a++)
     for (size_t m = 0; m < nP; m++)
        Smix[a][m] = EspField::overlapOf(set.funcs[a].fn, parent[m]);

   //  FO overlaps and the FO-parent overlaps.
   Mat Sff(nk, vector<double>(nk, 0.0)), F(nk, vector<double>(nP, 0.0));
   Mat SF(nk, vector<double>(nf, 0.0));
   for (size_t k = 0; k < nk; k++)
     for (size_t a = 0; a < nf; a++) {
        double s = 0.0;
        for (size_t b = 0; b < nf; b++)
           s += set.overlap[a*nf+b]*set.fos[k].coef[b];
        SF[k][a] = s;
     }
   for (size_t k = 0; k < nk; k++)
     for (size_t l = k; l < nk; l++) {
        double s = 0.0;
        for (size_t a = 0; a < nf; a++) s += SF[k][a]*set.fos[l].coef[a];
        Sff[k][l] = Sff[l][k] = s;
     }
   for (size_t k = 0; k < nk; k++)
     for (size_t m = 0; m < nP; m++) {
        double s = 0.0;
        for (size_t a = 0; a < nf; a++) s += set.fos[k].coef[a]*Smix[a][m];
        F[k][m] = s;
     }

   Mat X;
   if (!inverseSqrt(Sff, X, 1.0e-8)) {
      why = "The fragment orbitals are linearly dependent.";
      return false;
   }

   vector< vector<int> > groups;
   groupFos(set, groups);

   for (size_t r = 0; r < rows.size(); r++) {
      const vector<double>& c = rows[r];
      MoShares ms;
      if (!loewdinWeights(F, X, S, c, ms.perFo, ms.norm)) {
         why = "An MO has no norm in the parent basis.";
         return false;
      }
      for (size_t k = 0; k < nk; k++) {
         ms.capture += ms.perFo[k];
         const Fragment& fr = set.fragments[set.fos[k].fragment];
         if (fr.kind == METAL) ms.metal += ms.perFo[k];
         else ms.ligand += ms.perFo[k];
      }

      ms.klass = classOf(ms.capture, ms.metal, ms.ligand);

      ms.perGroup.assign(groups.size(), 0.0);
      for (size_t g = 0; g < groups.size(); g++) {
         double w = 0.0;
         for (size_t q = 0; q < groups[g].size(); q++) w += ms.perFo[groups[g][q]];
         ms.perGroup[g] = w;
         if (w < TOP_MIN) continue;
         LevelShare ls = describeGroup(set, groups[g]);
         ls.share = w;
         ms.top.push_back(ls);
      }
      std::stable_sort(ms.top.begin(), ms.top.end(),
          [](const LevelShare& a, const LevelShare& b) { return a.share > b.share; });
      out.push_back(ms);
   }
   return true;
}


bool FragmentOrbitals::composeGaussian(const vector<Fragment>& fragments,
                                       const vector<int>& foFragment,
                                       const vector< vector<double> >& foRows,
                                       const vector<double>& S,
                                       const vector< vector<double> >& moRows,
                                       vector<MoShares>& out, string& why)
{
   out.clear();
   why.clear();
   const size_t nk = foRows.size();
   const size_t nP = nk ? foRows[0].size() : 0;
   if (nk == 0 || foFragment.size() != nk || S.size() != nP*nP) {
      why = "The fragment orbitals do not match the parent basis.";
      return false;
   }
   Mat F(nk, vector<double>(nP, 0.0)), Sff(nk, vector<double>(nk, 0.0));
   for (size_t k = 0; k < nk; k++)
     for (size_t m = 0; m < nP; m++) {
        double s = 0.0;
        for (size_t a = 0; a < nP; a++) s += foRows[k][a]*S[a*nP+m];
        F[k][m] = s;
     }
   for (size_t k = 0; k < nk; k++)
     for (size_t l = k; l < nk; l++) {
        double s = 0.0;
        for (size_t m = 0; m < nP; m++) s += F[k][m]*foRows[l][m];
        Sff[k][l] = Sff[l][k] = s;
     }
   Mat X;
   if (!inverseSqrt(Sff, X, 1.0e-8)) {
      why = "The fragment orbitals are linearly dependent.";
      return false;
   }
   for (size_t r = 0; r < moRows.size(); r++) {
      MoShares ms;
      if (moRows[r].size() != nP ||
          !loewdinWeights(F, X, S, moRows[r], ms.perFo, ms.norm)) {
         why = "An MO row does not match the parent basis or has no norm.";
         return false;
      }
      for (size_t k = 0; k < nk; k++) {
         ms.capture += ms.perFo[k];
         if (fragments[foFragment[k]].kind == METAL) ms.metal += ms.perFo[k];
         else ms.ligand += ms.perFo[k];
      }
      ms.klass = classOf(ms.capture, ms.metal, ms.ligand);
      out.push_back(ms);
   }
   return true;
}

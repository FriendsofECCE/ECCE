//  The fragment-orbital correlation diagram: see include/tdat/FoDiagram.H.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <map>
#include <sstream>
#include <tuple>

#include "tdat/CharacterTable.H"
#include "tdat/FoDiagram.H"
#include "tdat/MoFragments.H"
#include "tdat/SymmetryAnalysis.H"

namespace {

typedef vector< vector<double> > Mat;

const double HARTREE = 27.211386245988;      // eV

string percent(double x)
{
   char b[16];
   snprintf(b, sizeof b, "%.0f%%", 100.0*x);
   return b;
}


//  "75–98 %": the about text writes a space before the percent sign.
string spacedPercent(double x)
{
   char b[16];
   snprintf(b, sizeof b, "%.0f %%", 100.0*x);
   return b;
}

string countWord(int n)
{
   static const char* w[] = { "no", "one", "two", "three", "four", "five", "six",
                              "seven", "eight", "nine", "ten", "eleven", "twelve" };
   return (n >= 0 && n <= 12) ? w[n] : std::to_string(n);
}

string plural(int n, const char* one, const char* many)
{
   return std::to_string(n) + " " + (n == 1 ? one : many);
}

//  The panel's own cut-off removes the deepest orbitals before this is
//  called; say how many, in the words the diagram's band uses.
string coreSentence(const MoColumn& c)
{
   if (c.hiddenCount <= 0 && c.hiddenSemicoreCount <= 0) return "";
   char e[32];
   snprintf(e, sizeof e, "%s%.2f", c.hiddenMaxEnergy < 0 ? "\xE2\x88\x92" : "",
            fabs(c.hiddenMaxEnergy));
   string t = "Also not shown: ";
   if (c.hiddenCount > 0 && c.hiddenSemicoreCount > 0)
      t += std::to_string(c.hiddenCount) + " core and " +
           std::to_string(c.hiddenSemicoreCount) + " semicore orbitals";
   else if (c.hiddenSemicoreCount > 0)
      t += plural(c.hiddenSemicoreCount, "semicore orbital", "semicore orbitals");
   else
      t += plural(c.hiddenCount, "core orbital", "core orbitals");
   return t + " below " + e + " Hartree.";
}

//  The text before the first comma: a ligand FO's dominant character.
string firstCharacter(const string& c)
{
   const string::size_type comma = c.find(',');
   return comma == string::npos ? c : c.substr(0, comma);
}

//  Solve A x = b in place (Gaussian elimination, partial pivoting).
bool solve(Mat A, vector<double> b, vector<double>& x)
{
   const size_t n = b.size();
   for (size_t c = 0; c < n; c++) {
      size_t p = c;
      for (size_t r = c + 1; r < n; r++)
         if (fabs(A[r][c]) > fabs(A[p][c])) p = r;
      if (fabs(A[p][c]) < 1.0e-12) return false;
      std::swap(A[p], A[c]); std::swap(b[p], b[c]);
      for (size_t r = c + 1; r < n; r++) {
         const double f = A[r][c]/A[c][c];
         for (size_t k = c; k < n; k++) A[r][k] -= f*A[c][k];
         b[r] -= f*b[c];
      }
   }
   x.assign(n, 0.0);
   for (size_t i = n; i-- > 0;) {
      double s = b[i];
      for (size_t k = i + 1; k < n; k++) s -= A[i][k]*x[k];
      x[i] = s/A[i][i];
   }
   return true;
}

bool inverse(const Mat& A, Mat& inv)
{
   const size_t n = A.size();
   inv.assign(n, vector<double>(n, 0.0));
   for (size_t c = 0; c < n; c++) {
      vector<double> e(n, 0.0), x;
      e[c] = 1.0;
      if (!solve(A, e, x)) return false;
      for (size_t r = 0; r < n; r++) inv[r][c] = x[r];
   }
   return true;
}

//  "OH" -> "Oh", "D4H" -> "D4h": the spelling the rest of the panel uses.
string groupSpelling(const string& g)
{
   string r = g;
   for (size_t i = 1; i < r.size(); i++) r[i] = (char)tolower((unsigned char)r[i]);
   return r;
}

// ---- the coordination skeleton's point group, found from its atoms ----
//
//  No helper program: an operation is fixed by where it sends two
//  non-collinear bond vectors, so every candidate is tried against the
//  whole skeleton.  Works in the frame the basis functions are in.

struct Skeleton
{
   string group;
   const CharacterTable* table;
   vector<SymOp> ops;
   vector< vector<int> > images;     // [op][atom], -1 outside the skeleton
   vector<int> classOfOp;
   vector<bool> inSkeleton;
   Skeleton() : table(0) {}
};

void cross(const double* a, const double* b, double* c)
{
   c[0] = a[1]*b[2] - a[2]*b[1];
   c[1] = a[2]*b[0] - a[0]*b[2];
   c[2] = a[0]*b[1] - a[1]*b[0];
}

double norm3(const double* a) { return sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]); }

//  The operations that map the skeleton onto itself, found from two
//  non-collinear bond vectors.  `p` is every atom relative to the origin
//  atom; `tol` is how far (same units) an image may be from an atom.
//  Returns false when no frame can be made (linear skeleton).
bool collectOps(const vector<string>& symbols, const vector<int>& keep,
                int origin, const vector<double>& p, double tol,
                vector<SymOp>& opsOut, vector< vector<int> >& imagesOut)
{
   const int n = (int)symbols.size();
   opsOut.clear();
   imagesOut.clear();

   //  Two non-collinear bond vectors: a, then the b that makes the
   //  largest angle with it.
   int ia = -1, ib = -1;
   for (size_t i = 0; i < keep.size(); i++)
      if (keep[i] != origin && norm3(&p[3*keep[i]]) > 0.1) { ia = keep[i]; break; }
   if (ia < 0) return false;
   double best = 0.0;
   for (size_t i = 0; i < keep.size(); i++) {
      const int q = keep[i];
      if (q == origin) continue;
      double c[3];
      cross(&p[3*ia], &p[3*q], c);
      const double s = norm3(c)/(norm3(&p[3*ia])*norm3(&p[3*q]) + 1.0e-12);
      if (s > best) { best = s; ib = q; }
   }
   if (ib < 0 || best < 0.1) return false;      // linear: no labels

   const double *a = &p[3*ia], *b = &p[3*ib];
   double ca[3];
   cross(a, b, ca);
   const double Af[9] = { a[0], b[0], ca[0], a[1], b[1], ca[1], a[2], b[2], ca[2] };
   Mat Am(3, vector<double>(3)), Ainv;
   for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) Am[r][c] = Af[3*r+c];
   if (!inverse(Am, Ainv)) return false;

   for (size_t ci = 0; ci < keep.size(); ci++) {
      const int i = keep[ci];
      if (i == origin || symbols[i] != symbols[ia]) continue;
      if (fabs(norm3(&p[3*i]) - norm3(a)) > tol) continue;
      for (size_t cj = 0; cj < keep.size(); cj++) {
         const int j = keep[cj];
         if (j == origin || symbols[j] != symbols[ib]) continue;
         if (fabs(norm3(&p[3*j]) - norm3(b)) > tol) continue;
         double d1[3], d2[3];
         for (int k = 0; k < 3; k++) { d1[k] = a[k]-b[k]; d2[k] = p[3*i+k]-p[3*j+k]; }
         if (fabs(norm3(d1) - norm3(d2)) > tol) continue;
         for (int improper = 0; improper < 2; improper++) {
            double cb[3];
            cross(&p[3*i], &p[3*j], cb);
            const double sgn = improper ? -1.0 : 1.0;
            const double Bf[9] = { p[3*i], p[3*j], sgn*cb[0],
                                   p[3*i+1], p[3*j+1], sgn*cb[1],
                                   p[3*i+2], p[3*j+2], sgn*cb[2] };
            SymOp R;
            for (int r = 0; r < 3; r++)
               for (int c = 0; c < 3; c++) {
                  double s = 0.0;
                  for (int k = 0; k < 3; k++) s += Bf[3*r+k]*Ainv[k][c];
                  R.m[r][c] = s;
               }
            vector<int> img(n, -1);
            bool ok = true;
            vector<bool> used(n, false);
            for (size_t cq = 0; cq < keep.size() && ok; cq++) {
               const int q = keep[cq];
               double r[3];
               for (int rr = 0; rr < 3; rr++) {
                  r[rr] = 0.0;
                  for (int c = 0; c < 3; c++) r[rr] += R.m[rr][c]*p[3*q+c];
               }
               int hit = -1;
               for (size_t cw = 0; cw < keep.size(); cw++) {
                  const int w = keep[cw];
                  if (symbols[w] != symbols[q]) continue;
                  double d = 0.0;
                  for (int k = 0; k < 3; k++) d += (p[3*w+k]-r[k])*(p[3*w+k]-r[k]);
                  if (sqrt(d) < tol) { hit = w; break; }
               }
               if (hit < 0 || used[hit]) ok = false;
               else { used[hit] = true; img[q] = hit; }
            }
            if (!ok) continue;
            opsOut.push_back(R);
            imagesOut.push_back(img);
         }
      }
   }
   return !opsOut.empty();
}

//  How far (Angstrom) a non-metal skeleton's atoms may be from the ideal
//  ones of a larger group and still be labelled in it.
const double NEAR_TOL = 0.15;

//  The nearest orthogonal matrix (polar decomposition by Newton steps).
void snapOrthogonal(SymOp& R)
{
   for (int it = 0; it < 30; it++) {
      SymOp Q;
      //  Q = 0.5 (R + R^-T), R^-T from the adjugate.
      double inv[3][3];
      const double (*m)[3] = R.m;
      const double det = R.determinant();
      if (fabs(det) < 1e-9) return;
      inv[0][0] = (m[1][1]*m[2][2]-m[1][2]*m[2][1])/det;
      inv[0][1] = (m[0][2]*m[2][1]-m[0][1]*m[2][2])/det;
      inv[0][2] = (m[0][1]*m[1][2]-m[0][2]*m[1][1])/det;
      inv[1][0] = (m[1][2]*m[2][0]-m[1][0]*m[2][2])/det;
      inv[1][1] = (m[0][0]*m[2][2]-m[0][2]*m[2][0])/det;
      inv[1][2] = (m[0][2]*m[1][0]-m[0][0]*m[1][2])/det;
      inv[2][0] = (m[1][0]*m[2][1]-m[1][1]*m[2][0])/det;
      inv[2][1] = (m[0][1]*m[2][0]-m[0][0]*m[2][1])/det;
      inv[2][2] = (m[0][0]*m[1][1]-m[0][1]*m[1][0])/det;
      double change = 0.0;
      for (int r = 0; r < 3; r++)
         for (int c = 0; c < 3; c++) {
            Q.m[r][c] = 0.5*(R.m[r][c] + inv[c][r]);
            change += fabs(Q.m[r][c] - R.m[r][c]);
         }
      R = Q;
      if (change < 1e-14) break;
   }
}

//  The character table the operations make up, if any.
bool matchTable(Skeleton& sk)
{
   vector< vector<int> > classes;
   SymmetryAnalysis::conjugacyClasses(sk.ops, classes);
   const vector<string> names = CharacterTable::names();
   for (size_t i = 0; i < names.size(); i++) {
      const CharacterTable* t = CharacterTable::lookup(names[i]);
      if (t == 0 || t->order() != (int)sk.ops.size()) continue;
      vector<int> classOf;
      if (SymmetryAnalysis::matchClasses(sk.ops, classes, *t, classOf)) {
         sk.table = t; sk.classOfOp = classOf; sk.group = groupSpelling(t->name());
         return true;
      }
   }
   return false;
}

//  `centre` >= 0: a non-metal centre.  Its skeleton is the atom and its
//  bonded atoms; when they are only nearly symmetric the group is the
//  nearest one (the skeleton is idealised by averaging over the
//  operations found loosely, until they are exact) and the result says so.
bool findSkeleton(const vector<string>& symbols, const vector<double>& coords,
                  const vector<FragmentOrbitals::Fragment>& frags,
                  Skeleton& sk, int centre = -1)
{
   const int n = (int)symbols.size();
   int origin = -1, metals = 0;
   for (size_t i = 0; i < frags.size(); i++) {
      if (frags[i].kind == FragmentOrbitals::METAL && !frags[i].atoms.empty()) {
         origin = frags[i].atoms[0]; metals++;
      }
   }
   if (metals != 1) return false;

   vector<int> keep;
   if (centre >= 0) {
      keep.push_back(centre);
      vector< std::pair<int,int> > bonds;
      MoFragments::covalentBonds(coords, symbols, bonds);
      for (int i = 0; i < n; i++)
         for (size_t b = 0; b < bonds.size(); b++)
            if ((bonds[b].first == centre && bonds[b].second == i) ||
                (bonds[b].second == centre && bonds[b].first == i)) {
               keep.push_back(i);
               break;
            }
   } else if (!MoFragments::coordinationSkeleton(coords, symbols, keep)) {
      keep.clear();
      for (int i = 0; i < n; i++) keep.push_back(i);
   }
   if (std::find(keep.begin(), keep.end(), origin) == keep.end()) return false;

   sk.inSkeleton.assign(n, false);
   for (size_t i = 0; i < keep.size(); i++) sk.inSkeleton[keep[i]] = true;

   vector<double> p(3*n, 0.0);
   for (int i = 0; i < n; i++)
      for (int k = 0; k < 3; k++) p[3*i+k] = coords[3*i+k] - coords[3*origin+k];

   if (!collectOps(symbols, keep, origin, p, 0.05, sk.ops, sk.images))
      return false;
   if (centre < 0) {
      if (sk.ops.empty()) return false;
      return matchTable(sk);
   }
   //  A non-metal centre: the exact group if there is one, unless the
   //  skeleton is nearly a larger one (CH3 of methanol is C3v to within
   //  its torsional distortion, exactly only Cs).  A group of order one
   //  carries no labels.
   Skeleton exact = sk;
   const bool haveExact = sk.ops.size() > 1 && matchTable(exact);
   const size_t exactOrder = haveExact ? exact.ops.size() : 1;
   sk.ops.clear(); sk.images.clear(); sk.table = 0;
   for (int pass = 0; pass < 40; pass++) {
      vector<SymOp> ops;
      vector< vector<int> > images;
      if (!collectOps(symbols, keep, origin, p, pass == 0 ? NEAR_TOL : 0.1, ops, images))
         break;
      if (ops.size() <= exactOrder) break;
      for (size_t o = 0; o < ops.size(); o++) snapOrthogonal(ops[o]);
      vector<double> q(3*n, 0.0);
      for (size_t k = 0; k < keep.size(); k++) {
         const int a = keep[k];
         for (size_t o = 0; o < ops.size(); o++) {
            const int im = images[o][a];
            for (int r = 0; r < 3; r++)
               for (int c = 0; c < 3; c++)
                  q[3*a+r] += ops[o].m[c][r]*p[3*im+c]/ops.size();
         }
      }
      p = q;
      sk.ops.clear(); sk.images.clear();
      if (collectOps(symbols, keep, origin, p, 0.01, sk.ops, sk.images) &&
          sk.ops.size() > 1) {
         for (size_t o = 0; o < sk.ops.size(); o++) snapOrthogonal(sk.ops[o]);
         if (sk.ops.size() > exactOrder && matchTable(sk)) {
            //  The ideal geometry only found the group; the functions are
            //  rotated by its operations on the real atoms.
            return true;
         }
      }
   }
   if (!haveExact) return false;
   const vector<bool> in = sk.inSkeleton;
   sk = exact;
   sk.inSkeleton = in;
   return true;
}

//  Angular part of a function at a unit vector.
double angularAt(const EspBasisFunction& f, const double* u)
{
   double s = 0.0;
   for (size_t t = 0; t < f.angularCoef.size(); t++)
      s += f.angularCoef[t]*pow(u[0], f.powerX[t])*pow(u[1], f.powerY[t])
           *pow(u[2], f.powerZ[t]);
   return s;
}

//  The matrix of operation `op` on the FO set's functions: T[j][i] is
//  the coefficient on function j of the image of function i.  Functions
//  off the skeleton are dropped.
bool functionMatrix(const FragmentOrbitals::FoSet& set, const Skeleton& sk,
                    size_t op, Mat& T)
{
   const size_t nf = set.funcs.size();
   T.assign(nf, vector<double>(nf, 0.0));
   static vector< vector<double> > dirs;
   if (dirs.empty()) {
      for (int k = 0; k < 48; k++) {
         const double z = 1.0 - 2.0*(k + 0.5)/48.0, r = sqrt(1.0 - z*z);
         const double ph = k*2.399963229728653;
         dirs.push_back(vector<double>());
         dirs.back().push_back(r*cos(ph));
         dirs.back().push_back(r*sin(ph));
         dirs.back().push_back(z);
      }
   }
   const SymOp& R = sk.ops[op];
   vector< vector<size_t> > byAtom(sk.inSkeleton.size());
   for (size_t f = 0; f < nf; f++) byAtom[set.funcs[f].atom].push_back(f);

   for (size_t a = 0; a < byAtom.size(); a++) {
      if (!sk.inSkeleton[a] || byAtom[a].empty()) continue;
      const int b = sk.images[op][a];
      if (b < 0 || byAtom[b].size() != byAtom[a].size()) return false;
      size_t s0 = 0;
      while (s0 < byAtom[a].size()) {
         const FragmentOrbitals::Function& f0 = set.funcs[byAtom[a][s0]];
         size_t s1 = s0;
         while (s1 < byAtom[a].size() &&
                set.funcs[byAtom[a][s1]].n == f0.n &&
                set.funcs[byAtom[a][s1]].l == f0.l) s1++;
         const size_t nj = s1 - s0;
         Mat G(nj, vector<double>(nj, 0.0));
         vector< vector<double> > P(dirs.size(), vector<double>(nj));
         for (size_t p = 0; p < dirs.size(); p++)
            for (size_t j = 0; j < nj; j++)
               P[p][j] = angularAt(set.funcs[byAtom[b][s0+j]].fn, &dirs[p][0]);
         for (size_t j = 0; j < nj; j++)
            for (size_t k = 0; k < nj; k++)
               for (size_t p = 0; p < dirs.size(); p++) G[j][k] += P[p][j]*P[p][k];
         for (size_t i = 0; i < nj; i++) {
            vector<double> rhs(nj, 0.0), x;
            for (size_t p = 0; p < dirs.size(); p++) {
               double u[3];   // R^-1 u = R^T u
               for (int r = 0; r < 3; r++)
                  u[r] = R.m[0][r]*dirs[p][0] + R.m[1][r]*dirs[p][1]
                         + R.m[2][r]*dirs[p][2];
               const double t = angularAt(set.funcs[byAtom[a][s0+i]].fn, u);
               for (size_t j = 0; j < nj; j++) rhs[j] += P[p][j]*t;
            }
            if (!solve(G, rhs, x)) return false;
            for (size_t j = 0; j < nj; j++) T[byAtom[b][s0+j]][byAtom[a][s0+i]] = x[j];
         }
         s0 = s1;
      }
   }
   return true;
}

//  One fragment level: a group of degenerate FOs restricted to one irrep.
struct Sub
{
   int group;
   string irrep;
   vector<int> members;     // FO indices
   Mat P;                   // projector in the members' amplitude space
   bool metal;
};

}  // namespace


vector<FoDiagram::PairCount>
FoDiagram::countPairs(const vector<PairLevel>& levels)
{
   std::map<string, PairCount> by;
   vector<string> order;
   for (size_t i = 0; i < levels.size(); i++) {
      const PairLevel& l = levels[i];
      if (!by.count(l.irrep)) { by[l.irrep].irrep = l.irrep; order.push_back(l.irrep); }
      PairCount& c = by[l.irrep];
      const bool inPair = l.character == MoLevel::BONDING ||
                          l.character == MoLevel::ANTIBONDING;
      if (l.shown > 0) {
         if (l.character == MoLevel::BONDING) c.bonding += l.degeneracy;
         else if (l.character == MoLevel::ANTIBONDING) c.antibonding += l.degeneracy;
         else c.nonbonding += l.degeneracy;
      } else if (inPair) {
         (l.shown == 0 ? c.hiddenByCutoff : c.hiddenByFilter) += l.degeneracy;
      }
   }
   vector<PairCount> out;
   for (size_t i = 0; i < order.size(); i++) {
      PairCount c = by[order[i]];
      c.ok = c.bonding == c.antibonding;
      if (!c.ok) {
         std::ostringstream m;
         m << (c.irrep.empty() ? string("MOs") : c.irrep) << ": " << c.bonding
           << " bonding, " << c.antibonding << " antibonding: pairs incomplete";
         if (c.hiddenByCutoff > 0)
            m << "; " << c.hiddenByCutoff << " partner orbital"
              << (c.hiddenByCutoff > 1 ? "s are" : " is")
              << " beyond the levels cut-off";
         if (c.hiddenByFilter > 0)
            m << "; " << c.hiddenByFilter << " partner orbital"
              << (c.hiddenByFilter > 1 ? "s are" : " is")
              << " hidden (tick Show all levels)";
         c.message = m.str();
      }
      out.push_back(c);
   }
   return out;
}


string FoDiagram::coreSentence(const MoColumn& centre) { return ::coreSentence(centre); }

string FoDiagram::groupMarkup(const string& group)
{
   if (group.empty()) return "";
   string g = group;
   for (size_t i = 0; i < g.size(); i++)
      g[i] = i == 0 ? toupper((unsigned char)g[i]) : tolower((unsigned char)g[i]);
   if (g == "Dinfh") return "D<sub>\xE2\x88\x9Eh</sub>";
   if (g == "Cinfv") return "C<sub>\xE2\x88\x9Ev</sub>";
   if (g.size() == 1) return g;
   return g.substr(0, 1) + "<sub>" + g.substr(1) + "</sub>";
}

string FoDiagram::complexFormula(const vector<string>& symbols,
                                 const vector<double>& coordsAng, int charge)
{
   vector<FragmentOrbitals::Fragment> frags;
   string why;
   if (!FragmentOrbitals::autoFragments(symbols, coordsAng, charge, 1, frags, why))
      return string();

   std::map<string,int> metals;
   std::vector<int> metalAtoms;
   for (size_t i = 0; i < frags.size(); i++)
      if (frags[i].kind == FragmentOrbitals::METAL)
         for (size_t k = 0; k < frags[i].atoms.size(); k++) {
            metals[symbols[frags[i].atoms[k]]]++;
            metalAtoms.push_back(frags[i].atoms[k]);
         }
   if (metalAtoms.empty()) return string();

   auto count = [](const std::map<string,int>& c, const string& e) {
      std::ostringstream o;
      o << e;
      std::map<string,int>::const_iterator it = c.find(e);
      if (it != c.end() && it->second > 1) o << it->second;
      return o.str();
   };

   std::map<string,int> kinds;                       // formula -> number
   std::map<string,bool> isPoly;
   for (size_t i = 0; i < frags.size(); i++) {
      if (frags[i].kind != FragmentOrbitals::LIGAND) continue;
      const vector<int>& at = frags[i].atoms;
      string text;
      if (at.size() == 1) {
         text = symbols[at[0]];
      } else {
         //  The donor: the atom nearest a metal.
         int donor = at[0];
         double best = 1.0e30;
         for (size_t k = 0; k < at.size(); k++)
            for (size_t m = 0; m < metalAtoms.size(); m++) {
               double d = 0.0;
               for (int q = 0; q < 3; q++) {
                  const double t = coordsAng[3*at[k]+q] - coordsAng[3*metalAtoms[m]+q];
                  d += t*t;
               }
               if (d < best - 1.0e-9) { best = d; donor = at[k]; }
            }
         std::map<string,int> rest;
         for (size_t k = 0; k < at.size(); k++)
            if (at[k] != donor) rest[symbols[at[k]]]++;
         std::map<string,int> all = rest;
         all[symbols[donor]]++;
         if (all.size() == 2 && all["H"] == 2 && all["O"] == 1) {
            text = "H2O";
         } else {
            text = symbols[donor];
            //  Hill order for the rest: C, H, then alphabetical.
            const char* first[] = { "C", "H" };
            for (int f = 0; f < 2; f++)
               if (rest.count(first[f])) text += count(rest, first[f]);
            for (std::map<string,int>::const_iterator it = rest.begin();
                 it != rest.end(); ++it)
               if (it->first != "C" && it->first != "H") text += count(rest, it->first);
         }
         isPoly[text] = true;
      }
      kinds[text]++;
   }

   string out;
   for (std::map<string,int>::const_iterator it = metals.begin(); it != metals.end(); ++it)
      out += count(metals, it->first);
   for (std::map<string,int>::const_iterator it = kinds.begin(); it != kinds.end(); ++it) {
      if (isPoly[it->first]) out += "(" + it->first + ")";
      else out += it->first;
      if (it->second > 1) out += std::to_string(it->second);
   }
   if (charge != 0) {
      out = "[" + out + "]";
      const int size = charge < 0 ? -charge : charge;
      out += "^";
      if (size > 1) out += std::to_string(size);
      out += charge < 0 ? "-" : "+";
   }
   return out;
}


bool FoDiagram::applicable(const vector<string>& symbols,
                           const vector<double>& coordsAng, int charge,
                           string& why, int centre)
{
   vector<FragmentOrbitals::Fragment> frags;
   if (!FragmentOrbitals::autoFragments(symbols, coordsAng, charge, 1,
                                        frags, why, centre)) {
      return false;
   }
   bool ligand = false;
   for (size_t i = 0; i < frags.size(); i++) {
      if (frags[i].kind == FragmentOrbitals::SHELL) ligand = true;
   }
   if (!ligand) {
      why = centre >= 0 ? "Not applicable: the picked atom has no bonded atoms."
                        : "Not applicable: no ligand bonded to the metal.";
      return false;
   }
   string missing;
   if (!FragmentOrbitals::canSolve(symbols, missing)) {
      why = "No extended Hückel parameters for " + missing + ".";
      return false;
   }
   return true;
}


namespace {

//  A drawn-or-not MO level while the diagram is being decided.
struct Lv
{
   MoLevel level;
   string irrep;                  // skeleton irrep of its orbitals
   vector<double> share;          // per fragment set, mean over orbitals
   double capture, metal, ligand;
   string klass;
   bool inCentre;
   int sm, sl;                    // dominant metal / ligand set
   double fit;                    // fit to the local label (non-metal centre)
   double shM, shL;
   bool valence;                  // described by the valence FOs
   bool shown;
   int why;                       // 0 shown, 1 cut-off, 2 filtered
   Lv() : capture(0), metal(0), ligand(0), inCentre(true), sm(-1), sl(-1), fit(1.0),
          shM(0), shL(0), valence(false), shown(false), why(0) {}
};

//  Periodic-table group of a d-block element (La counts as 3), 0 otherwise.
int groupNumber(const string& el)
{
   static const char* rows[3][10] = {
      { "Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn" },
      { "Y","Zr","Nb","Mo","Tc","Ru","Rh","Pd","Ag","Cd" },
      { "Lu","Hf","Ta","W","Re","Os","Ir","Pt","Au","Hg" } };
   if (el == "La") return 3;
   for (int r = 0; r < 3; r++)
      for (int g = 0; g < 10; g++)
         if (el == rows[r][g]) return g + 3;
   return 0;
}

}  // namespace


bool FoDiagram::build(const vector<string>& symbols,
                      const vector<double>& coordsAng, int charge,
                      const vector<EspBasisFunction>& parentBasis,
                      const vector<double>& S,
                      const vector< vector<double> >& moRows,
                      const vector<double>& moEnergies,
                      MoColumn& centre, Result& out, string& why,
                      bool showAll, const vector<MoLevel>* whole,
                      int multiplicity, int pickAtom)
{
   out = Result();
   why.clear();
   if (moRows.size() != moEnergies.size()) {
      why = "The orbital energies and coefficients do not match.";
      return false;
   }

   vector<FragmentOrbitals::Fragment> frags;
   if (!FragmentOrbitals::autoFragments(symbols, coordsAng, charge, 1,
                                        frags, why, pickAtom)) {
      return false;
   }
   const bool atomCentre = pickAtom >= 0;
   out.centreAtom = atomCentre;
   for (size_t i = 0; i < frags.size(); i++) {
      if (frags[i].kind != FragmentOrbitals::LIGAND) continue;
      string nm = frags[i].name;
      const string::size_type pr = nm.find(" (");
      if (pr != string::npos) nm = nm.substr(0, pr);
      out.pieces += (out.pieces.empty() ? "" : ", ") + nm;
   }
   FragmentOrbitals::FoSet set;
   if (!FragmentOrbitals::build(symbols, coordsAng, frags,
                                FragmentOrbitals::LIGAND_SHELL, set, why)) {
      return false;
   }

   //  All the MOs are projected: the Fock operator is known on their span.
   const size_t nMO = moRows.size();
   vector<FragmentOrbitals::MoShares> sh;
   Mat amp;
   if (!FragmentOrbitals::compose(set, parentBasis, S, moRows, sh, why, &amp)) {
      return false;
   }
   vector< vector<int> > groups;
   //  A non-metal centre's skeleton is only nearly symmetric, so FOs a few
   //  tenths of an eV apart are one level.
   FragmentOrbitals::groupFos(set, groups,
                              atomCentre ? 0.01 : FragmentOrbitals::DEGENERATE_EH);
   const size_t ng = groups.size();

   //  --- the skeleton's symmetry, and the fragment sets --------------
   Skeleton sk;
   bool haveSym = findSkeleton(symbols, coordsAng, frags, sk, pickAtom);
   const size_t nf = set.funcs.size();
   vector<Sub> subs;
   //  A non-metal centre: every irrep a FO has any weight in, unrounded,
   //  to say how far the skeleton's label is from the truth.
   vector<Sub> rawSubs;
   for (size_t g = 0; g < ng; g++) {
      const vector<int>& K = groups[g];
      const size_t n = K.size();
      const bool metal =
          set.fragments[set.fos[K[0]].fragment].kind == FragmentOrbitals::METAL;
      vector<Sub> mine;
      if (haveSym) {
         //  The FOs restricted to the skeleton's atoms, and the operations
         //  on them: P_G = (chi(E)/sum chi^2) sum_R chi_G(R) N^-1 M(R).
         vector< vector<double> > v(n, vector<double>(nf, 0.0)), Sv(n);
         for (size_t k = 0; k < n; k++)
            for (size_t f = 0; f < nf; f++)
               if (sk.inSkeleton[set.funcs[f].atom]) v[k][f] = set.fos[K[k]].coef[f];
         for (size_t k = 0; k < n; k++) {
            Sv[k].assign(nf, 0.0);
            for (size_t a = 0; a < nf; a++) {
               double s = 0.0;
               for (size_t b = 0; b < nf; b++) s += set.overlap[a*nf+b]*v[k][b];
               Sv[k][a] = s;
            }
         }
         Mat N(n, vector<double>(n, 0.0)), Ninv;
         for (size_t k = 0; k < n; k++)
            for (size_t l = 0; l < n; l++)
               for (size_t f = 0; f < nf; f++) N[k][l] += v[k][f]*Sv[l][f];
         bool ok = inverse(N, Ninv);
         vector<Mat> D;
         for (size_t o = 0; o < sk.ops.size() && ok; o++) {
            Mat T;
            if (!functionMatrix(set, sk, o, T)) { ok = false; break; }
            Mat M(n, vector<double>(n, 0.0));
            for (size_t l = 0; l < n; l++) {
               vector<double> w(nf, 0.0);
               for (size_t a = 0; a < nf; a++) {
                  double s = 0.0;
                  for (size_t b = 0; b < nf; b++) s += T[a][b]*v[l][b];
                  w[a] = s;
               }
               for (size_t k = 0; k < n; k++) {
                  double s = 0.0;
                  for (size_t f = 0; f < nf; f++) s += Sv[k][f]*w[f];
                  M[k][l] = s;
               }
            }
            Mat d(n, vector<double>(n, 0.0));
            for (size_t k = 0; k < n; k++)
               for (size_t l = 0; l < n; l++)
                  for (size_t q = 0; q < n; q++) d[k][l] += Ninv[k][q]*M[q][l];
            D.push_back(d);
         }
         if (ok) {
            const vector<string>& irreps = sk.table->irreps();
            for (size_t r = 0; r < irreps.size(); r++) {
               const vector<double>* chi = sk.table->characters(irreps[r]);
               if (chi == 0) continue;
               double norm = 0.0, chiE = 0.0;
               for (size_t o = 0; o < sk.ops.size(); o++) {
                  const double c = (*chi)[sk.classOfOp[o]];
                  norm += c*c;
                  if (fabs(sk.ops[o].trace() - 3.0) < 1e-6) chiE = c;
               }
               if (norm <= 0.0 || chiE <= 0.0) continue;
               Mat P(n, vector<double>(n, 0.0));
               for (size_t o = 0; o < sk.ops.size(); o++) {
                  const double c = (*chi)[sk.classOfOp[o]]*chiE/norm;
                  for (size_t k = 0; k < n; k++)
                     for (size_t l = 0; l < n; l++) P[k][l] += c*D[o][k][l];
               }
               double tr = 0.0;
               for (size_t k = 0; k < n; k++) {
                  for (size_t l = k + 1; l < n; l++)
                     P[k][l] = P[l][k] = 0.5*(P[k][l] + P[l][k]);
                  tr += P[k][k];
               }
               if (atomCentre && tr > 0.02) {
                  Sub rs; rs.group = (int)g; rs.irrep = irreps[r]; rs.members = K;
                  rs.P = P; rs.metal = metal;
                  rawSubs.push_back(rs);
               }
               if (tr < 0.5) continue;
               Sub s; s.group = (int)g; s.irrep = irreps[r]; s.members = K;
               s.P = P; s.metal = metal;
               mine.push_back(s);
            }
            if (atomCentre && !mine.empty()) {
               //  The skeleton is only nearly symmetric: a FO is drawn whole
               //  in the irrep it is mostly in, a degenerate set with whole
               //  orbitals per irrep, so no level carries a fraction of an
               //  orbital.
               if (n == 1) {
                  size_t top = 0;
                  for (size_t i = 1; i < mine.size(); i++)
                     if (mine[i].P[0][0] > mine[top].P[0][0]) top = i;
                  Sub keep1 = mine[top];
                  keep1.P[0][0] = 1.0;
                  mine.assign(1, keep1);
               } else {
                  for (size_t i = 0; i < mine.size(); i++) {
                     double tr = 0.0;
                     for (size_t k = 0; k < n; k++) tr += mine[i].P[k][k];
                     const double f = std::max(1.0, std::floor(tr + 0.5))/tr;
                     for (size_t k = 0; k < n; k++)
                        for (size_t l = 0; l < n; l++) mine[i].P[k][l] *= f;
                  }
               }
            }
         }
      }
      if (mine.empty()) {
         Sub s; s.group = (int)g; s.members = K; s.metal = metal;
         s.P.assign(n, vector<double>(n, 0.0));
         for (size_t k = 0; k < n; k++) s.P[k][k] = 1.0;
         mine.push_back(s);
      }
      for (size_t i = 0; i < mine.size(); i++) subs.push_back(mine[i]);
   }
   if (haveSym) out.group = sk.group;
   const size_t ns = subs.size();

   //  --- shares and Fock-diagonal energies per set ---------------------
   //  The span the Fock diagonal is taken over is the valence MOs (those the
   //  FOs describe at all): an extended Hueckel STO is not orthogonal to the
   //  parent's cores, and a 0.5% overlap with a -7 Hartree 3s would pull a
   //  metal 4s down by 0.04 Hartree.
   vector<bool> spanMO(nMO);
   for (size_t m = 0; m < nMO; m++) spanMO[m] = sh[m].klass != "core";
   vector< vector<double> > share(nMO, vector<double>(ns, 0.0));
   out.sets.assign(ns, SetInfo());
   for (size_t s = 0; s < ns; s++) {
      const Sub& sub = subs[s];
      const size_t n = sub.members.size();
      Mat Fg(n, vector<double>(n, 0.0)), Cg(n, vector<double>(n, 0.0));
      for (size_t m = 0; m < nMO; m++) {
         vector<double> a(n);
         for (size_t k = 0; k < n; k++) a[k] = amp[m][sub.members[k]];
         double w = 0.0;
         for (size_t k = 0; k < n; k++)
            for (size_t l = 0; l < n; l++) {
               const double t = a[k]*a[l];
               w += sub.P[k][l]*t;
               if (!spanMO[m]) continue;
               Cg[k][l] += t;
               Fg[k][l] += moEnergies[m]*t;
            }
         share[m][s] = std::max(0.0, w);
      }
      double trP = 0.0, trPC = 0.0, trPF = 0.0, eht = 0.0;
      for (size_t k = 0; k < n; k++) {
         trP += sub.P[k][k];
         eht += set.fos[sub.members[k]].energy;
         for (size_t l = 0; l < n; l++) {
            trPC += sub.P[k][l]*Cg[l][k];
            trPF += sub.P[k][l]*Fg[l][k];
         }
      }
      SetInfo& si = out.sets[s];
      si.metal = sub.metal;
      si.irrep = sub.irrep;
      si.eEht = eht/n;
      si.capture = trPC/std::max(trP, 1.0e-12);
      si.eFock = trPC > 1.0e-9 ? trPF/trPC : si.eEht;
      si.degeneracy = (int)lround(trP);
      const FragmentOrbitals::LevelShare d =
          FragmentOrbitals::describeGroup(set, groups[sub.group]);
      if (sub.metal) {
         si.label = d.label;
      } else {
         const string::size_type fo = d.label.find("FO");
         const string::size_type sp = d.label.find(" (", fo);
         si.label = d.label.substr(fo, sp == string::npos ? string::npos : sp - fo);
         //  A degenerate set split across irreps (methanol's CH3 e pair is
         //  a' + a'' in Cs) must not give two levels one label.
         size_t same = 0;
         for (size_t q = 0; q < ns; q++) same += subs[q].group == sub.group;
         if (atomCentre && same > 1 && !sub.irrep.empty()) si.label += " " + sub.irrep;
      }
   }

   //  --- MO skeleton irreps, per orbital -------------------------------
   vector<string> moIrrep(nMO);
   //  How well each MO fits its local label (non-metal centre only): the
   //  share of its skeleton part in the label's irrep, times the weight
   //  of the MO on the skeleton's atoms (Mulliken, from the parent).  The
   //  skeleton's operations say nothing about atoms outside it.
   vector<double> moFit(nMO, 1.0);
   if (haveSym) {
      vector<int> atomOf;
      if (atomCentre) {
         const double bohr = 0.529177210903;
         for (size_t f = 0; f < parentBasis.size(); f++) {
            int near = 0; double dm = 1e30;
            for (size_t a = 0; a < symbols.size(); a++) {
               double d = 0.0;
               for (int k = 0; k < 3; k++) {
                  const double x = parentBasis[f].center[k] - coordsAng[3*a+k]/bohr;
                  d += x*x;
               }
               if (d < dm) { dm = d; near = (int)a; }
            }
            atomOf.push_back(near);
         }
      }
      const size_t nb = parentBasis.size();
      for (size_t m = 0; m < nMO; m++) {
         std::map<string, double> by;
         double all = 0.0;
         for (size_t s = 0; s < ns; s++) { by[subs[s].irrep] += share[m][s]; all += share[m][s]; }
         double top = 0.0;
         for (std::map<string,double>::const_iterator it = by.begin();
              it != by.end(); ++it)
            if (it->second > top) { top = it->second; moIrrep[m] = it->first; }
         if (top < 0.05) moIrrep[m].clear();
         if (!atomCentre) continue;
         if (moIrrep[m].empty() || all <= 0.0) { moFit[m] = 0.0; continue; }
         //  The label's weight among every irrep the FOs of this MO reach.
         {
            double rawAll = 0.0, rawTop = 0.0;
            for (size_t q = 0; q < rawSubs.size(); q++) {
               const Sub& rs = rawSubs[q];
               double w = 0.0;
               for (size_t k = 0; k < rs.members.size(); k++)
                  for (size_t l = 0; l < rs.members.size(); l++)
                     w += rs.P[k][l]*amp[m][rs.members[k]]*amp[m][rs.members[l]];
               w = std::max(0.0, w);
               rawAll += w;
               if (rs.irrep == moIrrep[m]) rawTop += w;
            }
            if (rawAll > 0.0) top = rawTop, all = rawAll;
         }
         double skel = 1.0;
         if (S.size() == nb*nb && moRows[m].size() == nb) {
            double tot = 0.0, on = 0.0;
            for (size_t i = 0; i < nb; i++) {
               double sc = 0.0;
               for (size_t j = 0; j < nb; j++) sc += S[i*nb+j]*moRows[m][j];
               const double w = moRows[m][i]*sc;
               tot += w;
               if (sk.inSkeleton[atomOf[i]]) on += w;
            }
            skel = tot > 0.0 ? std::min(1.0, std::max(0.0, on/tot)) : 0.0;
         }
         moFit[m] = (top/all)*skel;
      }
   }

   //  --- the levels: the whole spectrum, regrouped by skeleton irrep ----
   vector<MoLevel> input = whole ? *whole : centre.levels;
   std::sort(input.begin(), input.end(),
             [](const MoLevel& a, const MoLevel& b) { return a.energy < b.energy; });
   std::map<int,bool> drawnOrbital;
   for (size_t c = 0; c < centre.levels.size(); c++)
      for (size_t k = 0; k < centre.levels[c].orbitals.size(); k++)
         drawnOrbital[centre.levels[c].orbitals[k]] = true;

   //  Levels the panel grouped by energy alone (no labels) can hold two
   //  irreps that happen to be close; they are split into single orbitals
   //  and put back together by the irrep's own dimension below.
   if (haveSym) {
      vector<MoLevel> atoms;
      for (size_t i = 0; i < input.size(); i++) {
         const MoLevel& lv = input[i];
         bool mixed = lv.irrep.empty() && lv.degeneracy > 0;
         string first;
         for (size_t k = 0; k < lv.orbitals.size(); k++) {
            const int m = lv.orbitals[k];
            const string X = (m >= 0 && m < (int)nMO) ? moIrrep[m] : string();
            if (k == 0) first = X; else if (X != first) mixed = true;
         }
         if (!mixed || lv.orbitals.size() != (size_t)lv.degeneracy) { atoms.push_back(lv); continue; }
         for (size_t k = 0; k < lv.orbitals.size(); k++) {
            MoLevel a = lv;
            a.orbitals.assign(1, lv.orbitals[k]);
            a.degeneracy = 1;
            a.energy = k < lv.energies.size() ? lv.energies[k] : lv.energy;
            a.energies.assign(1, a.energy);
            a.occupancy = lv.occupancy/lv.degeneracy;
            if (lv.occupancyBeta >= 0) a.occupancyBeta = lv.occupancyBeta/lv.degeneracy;
            a.label = std::to_string(lv.orbitals[k] + 1); a.irrep.clear();
            atoms.push_back(a);
         }
      }
      input.swap(atoms);
      std::sort(input.begin(), input.end(),
                [](const MoLevel& a, const MoLevel& b) { return a.energy < b.energy; });
   }

   vector<MoLevel> merged;
   {
      std::vector<bool> taken(input.size(), false);
      for (size_t i = 0; i < input.size(); i++) {
         if (taken[i]) continue;
         MoLevel lv = input[i];
         taken[i] = true;
         string X;
         if (!lv.orbitals.empty() && lv.orbitals[0] >= 0 &&
             lv.orbitals[0] < (int)nMO) X = moIrrep[lv.orbitals[0]];
         const int dim = (haveSym && !X.empty()) ? sk.table->dimension(X) : 1;
         for (size_t j = i + 1; j < input.size() && lv.degeneracy < dim; j++) {
            if (taken[j]) continue;
            const MoLevel& nx = input[j];
            string Y;
            if (!nx.orbitals.empty() && nx.orbitals[0] >= 0 &&
                nx.orbitals[0] < (int)nMO) Y = moIrrep[nx.orbitals[0]];
            if (Y != X) continue;
            if (nx.energy - input[i].energy > 0.02) break;
            if (lv.degeneracy + nx.degeneracy > dim) continue;
            //  One level set: the irrep's own dimension decides, not a gap.
            const bool wasLabelled = !lv.irrep.empty();
            lv.orbitals.insert(lv.orbitals.end(), nx.orbitals.begin(), nx.orbitals.end());
            lv.energies.insert(lv.energies.end(), nx.energies.begin(), nx.energies.end());
            lv.occupancy += nx.occupancy;
            if (lv.occupancyBeta >= 0 && nx.occupancyBeta >= 0) lv.occupancyBeta += nx.occupancyBeta;
            else lv.occupancyBeta = -1.0;
            lv.degeneracy += nx.degeneracy;
            double e = 0; for (size_t q = 0; q < lv.energies.size(); q++) e += lv.energies[q];
            if (!lv.energies.empty()) lv.energy = e/lv.energies.size();
            (void)wasLabelled;
            taken[j] = true;
         }
         merged.push_back(lv);
      }
   }

   vector<Lv> L(merged.size());
   const size_t nL = merged.size();
   out.captureMin = 1.0; out.captureMax = 0.0;
   for (size_t c = 0; c < nL; c++) {
      Lv& v = L[c];
      v.level = merged[c];
      v.share.assign(ns, 0.0);
      int n = 0;
      for (size_t k = 0; k < v.level.orbitals.size(); k++) {
         const int m = v.level.orbitals[k];
         if (m < 0 || m >= (int)nMO) continue;
         for (size_t s = 0; s < ns; s++) v.share[s] += share[m][s];
         v.capture += sh[m].capture; v.metal += sh[m].metal;
         v.ligand += sh[m].ligand;
         if (drawnOrbital.count(m)) v.inCentre = true;
         n++;
      }
      if (whole) {
         v.inCentre = false;
         for (size_t k = 0; k < v.level.orbitals.size(); k++)
            if (drawnOrbital.count(v.level.orbitals[k])) v.inCentre = true;
      }
      if (n == 0) { v.klass = "core"; continue; }
      for (size_t s = 0; s < ns; s++) v.share[s] /= n;
      v.capture /= n; v.metal /= n; v.ligand /= n;
      v.klass = FragmentOrbitals::classOf(v.capture, v.metal, v.ligand);
      v.valence = v.klass != "core";
      v.irrep = moIrrep[v.level.orbitals[0]];
      if (atomCentre) {
         double f = 0.0; int k2 = 0;
         for (size_t k = 0; k < v.level.orbitals.size(); k++) {
            const int m = v.level.orbitals[k];
            if (m >= 0 && m < (int)nMO) { f += moFit[m]; k2++; }
         }
         v.fit = k2 > 0 ? f/k2 : 0.0;
      }
      for (size_t s = 0; s < ns; s++) {
         if (!v.irrep.empty() && subs[s].irrep != v.irrep) continue;
         if (subs[s].metal) { if (v.sm < 0 || v.share[s] > v.shM) { v.sm = (int)s; v.shM = v.share[s]; } }
         else               { if (v.sl < 0 || v.share[s] > v.shL) { v.sl = (int)s; v.shL = v.share[s]; } }
      }
   }

   //  --- bonding / antibonding pairs -------------------------------------
   //  A partner pair: two levels of one skeleton irrep and one degeneracy
   //  that mix the same metal set, the lower below that set's level and
   //  the higher above it, and share a ligand set of that irrep (at least
   //  PAIR_FLOOR in both).  Taken best first, so a ligand-internal level
   //  with a little metal in it does not displace the pair that carries
   //  the interaction.
   struct Cand { double score; size_t b, a; int sm, sl; };
   vector<Cand> cands;
   for (size_t b = 0; b < nL; b++) {
      Lv& vb = L[b];
      vb.level.character = MoLevel::NONBONDING;
      vb.level.pairing = -1;
      if (!vb.valence || vb.irrep.empty() && haveSym) continue;
      for (size_t a = 0; a < nL; a++) {
         const Lv& va = L[a];
         if (a == b || !va.valence || va.irrep != vb.irrep ||
             va.level.degeneracy != vb.level.degeneracy) continue;
         if (va.level.energy <= vb.level.energy) continue;
         for (size_t s = 0; s < ns; s++) {
            if (!subs[s].metal || (haveSym && subs[s].irrep != vb.irrep)) continue;
            const double eM = out.sets[s].eFock;
            if (!(vb.level.energy < eM && va.level.energy > eM)) continue;
            const double mm = std::min(vb.share[s], va.share[s]);
            if (mm < PAIR_FLOOR) continue;
            for (size_t l = 0; l < ns; l++) {
               if (subs[l].metal || (haveSym && subs[l].irrep != vb.irrep)) continue;
               const double ml = std::min(vb.share[l], va.share[l]);
               if (ml < PAIR_FLOOR) continue;
               //  the interaction has to be a real one on at least one side
               if (std::max(vb.share[s], va.share[s]) < LINK_MIN ||
                   std::max(vb.share[l], va.share[l]) < LINK_MIN) continue;
               Cand c; c.score = mm + ml; c.b = b; c.a = a; c.sm = (int)s; c.sl = (int)l;
               cands.push_back(c);
            }
         }
      }
   }
   std::sort(cands.begin(), cands.end(),
             [](const Cand& x, const Cand& y) { return x.score > y.score; });
   vector< std::pair<size_t,size_t> > pairList;       // (bonding, anti)
   vector< std::pair<int,int> > pairParents;          // (metal set, ligand set)
   int nextPair = 0;
   for (size_t i = 0; i < cands.size(); i++) {
      const Cand& c = cands[i];
      if (L[c.b].level.pairing >= 0 || L[c.a].level.pairing >= 0) continue;
      L[c.b].level.character = MoLevel::BONDING;
      L[c.a].level.character = MoLevel::ANTIBONDING;
      L[c.b].level.pairing = L[c.a].level.pairing = nextPair++;
      pairList.push_back(std::make_pair(c.b, c.a));
      pairParents.push_back(std::make_pair(c.sm, c.sl));
   }

   //  --- what is drawn ----------------------------------------------------
   int hiddenLigand = 0, hiddenCore = 0, hiddenCut = 0, coreEmpty = 0;
   for (size_t c = 0; c < nL; c++) {
      Lv& v = L[c];
      if (!v.valence) { v.why = 3; continue; }
      if (!v.inCentre) { v.why = 1; continue; }
      if (!showAll && v.klass == "ligand") {
         double toMetal = 0.0;
         for (size_t s = 0; s < ns; s++) if (subs[s].metal) toMetal = std::max(toMetal, v.share[s]);
         if (toMetal < LINK_MIN) { v.why = 2; continue; }
      }
      v.shown = true;
   }
   //  A pair is drawn whole or not at all (past the cut-off and the filter too).
   for (size_t p = 0; p < pairList.size(); p++) {
      Lv& b = L[pairList[p].first];
      Lv& a = L[pairList[p].second];
      if (b.shown || a.shown) b.shown = a.shown = true;
   }
   for (size_t c = 0; c < nL; c++) {
      if (L[c].shown) L[c].why = 0;
      else if (L[c].why == 3 && L[c].valence == false) {
         hiddenCore++;
         if (L[c].level.occupancy + std::max(L[c].level.occupancyBeta, 0.0) < 0.01) coreEmpty++;
      }
      else if (L[c].why == 2) hiddenLigand++;
      else if (L[c].why == 1) hiddenCut++;
   }
   (void)hiddenCut;

   if (getenv("FODIAGRAM_DEBUG")) {
      for (size_t c = 0; c < nL; c++)
         if (L[c].valence && L[c].level.character != MoLevel::NONBONDING)
            fprintf(stderr, "lvl %zu E=%.4f deg=%d %s char=%d pair=%d shown=%d inCentre=%d shM=%.3f shL=%.3f sm=%d sl=%d\n",
                    c, L[c].level.energy, L[c].level.degeneracy, L[c].irrep.c_str(),
                    (int)L[c].level.character, L[c].level.pairing, (int)L[c].shown,
                    (int)L[c].inCentre, L[c].shM, L[c].shL, L[c].sm, L[c].sl);
   }
   //  Pair count, over every level the valence FOs describe.
   {
      vector<PairLevel> pl;
      for (size_t c = 0; c < nL; c++) {
         if (!L[c].valence || L[c].irrep.empty()) continue;
         PairLevel q;
         q.irrep = L[c].irrep;
         q.degeneracy = L[c].level.degeneracy;
         q.character = L[c].level.character;
         q.shown = L[c].shown ? 1 : (L[c].why == 2 ? -1 : 0);
         pl.push_back(q);
      }
      out.pairs = countPairs(pl);
      for (size_t i = 0; i < out.pairs.size(); i++)
         if (!out.pairs[i].ok) out.pairMessages.push_back(out.pairs[i].message);
   }

   vector<int> newIndex(nL, -1);
   vector<size_t> shownIdx;
   for (size_t c = 0; c < nL; c++) {
      if (!L[c].shown) continue;
      newIndex[c] = (int)shownIdx.size();
      shownIdx.push_back(c);
   }
   if (shownIdx.empty()) {
      why = "Every drawn orbital is on the ligands alone (or core).";
      return false;
   }

   //  --- the fragment columns: sets with at least one line ---------------
   double lo = 1.0e30, hi = -1.0e30;
   for (size_t i = 0; i < shownIdx.size(); i++) {
      lo = std::min(lo, L[shownIdx[i]].level.energy);
      hi = std::max(hi, L[shownIdx[i]].level.energy);
   }
   const double margin = 0.25*(hi - lo);
   vector<bool> parent(ns, false);
   for (size_t p = 0; p < pairParents.size(); p++) {
      parent[pairParents[p].first] = true; parent[pairParents[p].second] = true;
   }
   vector<bool> linked(ns, false);
   int farAway = 0;
   for (size_t s = 0; s < ns; s++) {
      bool any = false;
      for (size_t i = 0; i < shownIdx.size(); i++)
         if (L[shownIdx[i]].share[s] >= LINK_MIN) any = true;
      if (parent[s]) {
         for (size_t p = 0; p < pairList.size(); p++)
            if ((size_t)pairParents[p].first == s || (size_t)pairParents[p].second == s)
               if (L[pairList[p].first].shown) any = true;
      }
      if (!any) continue;
      const double e = out.sets[s].eFock;
      if (!parent[s] && (e < lo - margin || e > hi + margin)) { farAway++; continue; }
      linked[s] = true;
   }

   vector< std::pair<double,size_t> > order[2];       // 0 metal, 1 ligand
   for (size_t s = 0; s < ns; s++) {
      if (!linked[s]) continue;
      order[subs[s].metal ? 0 : 1].push_back(std::make_pair(out.sets[s].eFock, s));
   }
   if (order[0].empty() || order[1].empty()) {
      why = "No metal orbital or no ligand orbital makes up as much as " +
            percent(LINK_MIN) + " of any drawn orbital.";
      return false;
   }

   //  --- electrons: the metal d count, placed; the bookkeeping ----------
   //  Ionic convention: the ligands carry their charges (autoFragments),
   //  the metal the rest, and n = group number - oxidation state.
   vector<double> dAlpha(ns, 0.0), dBeta(ns, 0.0);
   vector<bool> isD(ns, false);
   {
      ElectronCount& ec = out.electrons;
      int nMetal = 0;
      string metalSym;
      for (size_t i = 0; i < frags.size(); i++)
         if (frags[i].kind == FragmentOrbitals::METAL) {
            nMetal++;
            if (nMetal == 1) {
               ec.oxidation = frags[i].charge;
               metalSym = symbols[frags[i].atoms[0]];
               ec.metal = metalSym;
            }
         }
      double ligElectrons = 0.0;
      for (size_t k = 0; k < set.fos.size(); k++)
         if (set.fragments[set.fos[k].fragment].kind != FragmentOrbitals::METAL)
            ligElectrons += set.fos[k].occupancy;
      ec.ligands = (int)lround(ligElectrons);
      for (size_t c = 0; c < merged.size(); c++) {
         const MoLevel& lv = merged[c];
         bool valence = false;
         for (size_t k = 0; k < lv.orbitals.size(); k++) {
            const int m = lv.orbitals[k];
            if (m >= 0 && m < (int)nMO && sh[m].klass != "core") valence = true;
         }
         if (!valence) continue;
         ec.moElectrons += lv.occupancy + (lv.occupancyBeta >= 0 ? lv.occupancyBeta : 0.0);
      }
      ec.centreAtom = atomCentre;
      if (atomCentre) {
         //  Neutral pieces: the centre keeps its own valence electrons
         //  less its charge, put in its s and p levels by aufbau (the
         //  Fock diagonals order them) and Hund's rule, as the d
         //  electrons of a metal are.
         string reason;
         int c = FragmentOrbitals::valenceElectrons(metalSym) - ec.oxidation;
         ec.dCount = c;
         if (nMetal != 1 || c < 0 || c > 8)
            reason = "the centre's valence electron count is " + std::to_string(c);
         vector< std::pair<double,size_t> > sSets, pSets;
         if (reason.empty()) {
            for (size_t s = 0; s < ns; s++) {
               if (!subs[s].metal) continue;
               const vector<int>& K = groups[subs[s].group];
               size_t best = 0; double bc = -1.0;
               for (size_t f = 0; f < nf; f++) {
                  const double a = fabs(set.fos[K[0]].coef[f]);
                  if (a > bc) { bc = a; best = f; }
               }
               if (set.funcs[best].l == 0) sSets.push_back(std::make_pair(out.sets[s].eFock, s));
               if (set.funcs[best].l == 1) pSets.push_back(std::make_pair(out.sets[s].eFock, s));
            }
            int so = 0, po = 0;
            for (size_t i = 0; i < sSets.size(); i++) so += out.sets[sSets[i].second].degeneracy;
            for (size_t i = 0; i < pSets.size(); i++) po += out.sets[pSets[i].second].degeneracy;
            if (so != 1 || (po != 3 && symbols[pickAtom] != "H" && symbols[pickAtom] != "He"))
               reason = "the centre's s and p orbitals are not all found";
         }
         if (reason.empty()) {
            const int sE = std::min(c, 2), pE = c - sE;
            const int uS = sE % 2, uP = pE <= 3 ? pE : 6 - pE;
            ec.unpaired = uS + uP;
            std::sort(sSets.begin(), sSets.end());
            std::sort(pSets.begin(), pSets.end());
            const vector< std::pair<double,size_t> >* lists[2] = { &sSets, &pSets };
            const int doubles[2] = { (sE - uS)/2, (pE - uP)/2 };
            const int singles[2] = { uS, uP };
            for (int w = 0; w < 2; w++) {
               int doubled = doubles[w], single = singles[w];
               for (size_t i = 0; i < lists[w]->size(); i++) {
                  const size_t s = (*lists[w])[i].second;
                  int room = out.sets[s].degeneracy;
                  const int dbl = std::min(doubled, room);
                  doubled -= dbl; room -= dbl;
                  const int sgl = std::min(single, room);
                  single -= sgl;
                  dAlpha[s] = dbl + sgl;
                  dBeta[s] = dbl;
                  isD[s] = true;
               }
            }
            ec.placed = true;
         } else {
            ec.why = reason;
         }
      } else {
      const int group = groupNumber(metalSym);
      string reason;
      if (nMetal != 1 || group == 0) {
         reason = "the metal's d count is given for a single transition metal only";
      } else {
         ec.dCount = group - ec.oxidation;
         if (ec.dCount < 0 || ec.dCount > 10) {
            char b[160];
            snprintf(b, sizeof b, "the ionic d count is %d (group %d, oxidation state %+d), "
                     "outside 0-10", ec.dCount, group, ec.oxidation);
            reason = b;
         }
      }
      const int unpaired = multiplicity > 1 ? multiplicity - 1 : 0;
      ec.unpaired = unpaired;
      vector< std::pair<double,size_t> > dsets;
      if (reason.empty()) {
         for (size_t s = 0; s < ns; s++) {
            if (!subs[s].metal) continue;
            const vector<int>& K = groups[subs[s].group];
            size_t best = 0; double bc = -1.0;
            for (size_t f = 0; f < nf; f++) {
               const double a = fabs(set.fos[K[0]].coef[f]);
               if (a > bc) { bc = a; best = f; }
            }
            if (set.funcs[best].l == 2) {
               isD[s] = true;
               dsets.push_back(std::make_pair(out.sets[s].eFock, s));
            }
         }
         std::sort(dsets.begin(), dsets.end());
         int orbitals = 0;
         for (size_t i = 0; i < dsets.size(); i++) orbitals += out.sets[dsets[i].second].degeneracy;
         const int rest = ec.dCount - unpaired;
         if (orbitals != 5) reason = "the five d orbitals are not all found";
         else if (rest < 0 || rest % 2 != 0 || rest/2 + unpaired > 5)
            reason = "a d" + std::to_string(ec.dCount) + " ion cannot have multiplicity "
                     + std::to_string(multiplicity);
      }
      if (reason.empty()) {
         //  Pairs fill the lowest orbitals, the unpaired electrons the next
         //  ones up (Hund's rule); a level's degeneracy counts orbitals.
         int doubled = (ec.dCount - unpaired)/2, single = unpaired;
         for (size_t i = 0; i < dsets.size(); i++) {
            const size_t s = dsets[i].second;
            int room = out.sets[s].degeneracy;
            const int dbl = std::min(doubled, room);
            doubled -= dbl; room -= dbl;
            const int sgl = std::min(single, room);
            single -= sgl;
            dAlpha[s] = dbl + sgl;
            dBeta[s] = dbl;
         }
         ec.placed = true;
      } else {
         ec.why = reason;
         isD.assign(ns, false);
      }
      }
      const double diff = (ec.placed ? ec.dCount : 0) + ec.ligands - ec.moElectrons;
      ec.agree = ec.placed && fabs(diff) < 0.5;
      char b[400];
      if (ec.placed) {
         snprintf(b, sizeof b, "Electrons: %s%d + %s %d = %d in the drawn "
                  "valence orbitals",
                  atomCentre ? (ec.metal + " ").c_str() : "metal d", ec.dCount,
                  atomCentre ? "pieces" : "ligands", ec.ligands, ec.dCount + ec.ligands);
         ec.line = b;
         if (ec.agree) ec.line += ", the same as the " + std::to_string((int)lround(ec.moElectrons))
                                  + " in the valence MOs.";
         else {
            snprintf(b, sizeof b, "; the valence MOs the diagram considers (covered "
                     "%.0f%% or more) hold %.0f, so the two sides do not agree.",
                     100*FragmentOrbitals::CORE_CAPTURE, ec.moElectrons);
            ec.line += b;
         }
      } else {
         snprintf(b, sizeof b, "Electrons: no %s electrons drawn, %s.  %s "
                  "%d; the valence MOs hold %.0f.",
                  atomCentre ? "centre-atom" : "metal d", ec.why.c_str(),
                  atomCentre ? "Pieces" : "Ligands", ec.ligands, ec.moElectrons);
         ec.line = b;
      }
   }

   int lowCapture = 0;
   for (int side = 0; side < 2; side++) {
      std::sort(order[side].begin(), order[side].end());
      MoColumn& col = side == 0 ? out.left : out.right;
      for (size_t i = 0; i < order[side].size(); i++) {
         const size_t s = order[side][i].second;
         SetInfo& si = out.sets[s];
         si.column = (int)i;
         MoLevel lv;
         lv.energy = si.eFock;
         lv.tabulated = si.eEht*HARTREE;
         lv.degeneracy = si.degeneracy > 0 ? si.degeneracy : 1;
         lv.energies.assign(lv.degeneracy, lv.energy);
         lv.irrep = si.irrep;
         const FragmentOrbitals::LevelShare d =
             FragmentOrbitals::describeGroup(set, groups[subs[s].group]);
         lv.label = si.label;
         if (!si.irrep.empty()) lv.annotation = si.irrep;
         if (side == 0 && out.electrons.placed && isD[s]) {
            lv.occupancy = dAlpha[s] + dBeta[s];
            if (out.electrons.unpaired > 0) {
               lv.occupancy = dAlpha[s];
               lv.occupancyBeta = dBeta[s];
            }
         }
         if (side == 1) {
            const string c = firstCharacter(d.character);
            lv.annotation = si.irrep.empty() ? c : si.irrep + "  " + c;
            for (size_t q = 0; q < subs[s].members.size(); q++)
               lv.occupancy += set.fos[subs[s].members[q]].occupancy
                               *subs[s].P[q][q];
         }
         if (si.capture < CAPTURE_OK) {
            lv.annotation += (lv.annotation.empty() ? "" : "  ")
                             + string("covered ") + percent(si.capture);
            lowCapture++;
         }
         char eb[96];
         snprintf(eb, sizeof eb, "%s (qualitative): %.2f eV",
                  side == 0 ? "EHT H_ii" : "EHT FO energy", si.eEht*HARTREE);
         char fb[320];
         snprintf(fb, sizeof fb, "Fock diagonal %.4f Hartree, covered %s of the "
                  "orbital by the calculation's MOs.\n%s", si.eFock,
                  percent(si.capture).c_str(), eb);
         lv.detail = fb;
         col.levels.push_back(lv);
      }
   }
   out.left.title = atomCentre
       ? symbols[pickAtom] + " AOs (extended H\xC3\xBC" "ckel)"
       : string("Metal AOs (extended H\xC3\xBC" "ckel)");
   out.right.title = atomCentre
       ? string("Orbitals of the other atoms (extended H\xC3\xBC" "ckel)")
       : string("Ligand orbitals (extended H\xC3\xBC" "ckel)");

   //  --- the drawn levels, their text and their lines ---------------------
   vector<MoLevel> kept;
   out.mo.clear();
   for (size_t i = 0; i < shownIdx.size(); i++) {
      Lv& v = L[shownIdx[i]];
      MoLevel& lv = v.level;
      lv.shareLeft = v.metal;
      lv.shareRight = v.ligand;
      if (atomCentre) {
         //  The local skeleton's label, shown as far as it fits.
         lv.label = v.irrep.empty() || v.fit < FIT_LABEL_MIN ? string()
                  : (v.fit < FIT_LABEL_OK ? "~" + v.irrep : v.irrep);
      } else if (merged[shownIdx[i]].irrep.empty() && !v.irrep.empty()) {
         lv.label = v.irrep;
      }
      lv.irrep = v.irrep;
      if (lv.character == MoLevel::ANTIBONDING) lv.label += "*";
      else if (lv.character == MoLevel::NONBONDING) lv.label += lv.label.empty() ? "nb" : " nb";
      {
         char e4[32];
         snprintf(e4, sizeof e4, "%.4f", lv.energy);   // as the panel writes it
         lv.annotation = e4;
      }

      MoInfo mi;
      mi.irrep = v.irrep; mi.metalSet = v.sm; mi.ligandSet = v.sl;
      mi.fit = v.fit;
      vector< std::pair<double,int> > rank;
      for (size_t s = 0; s < ns; s++)
         if (v.share[s] >= FragmentOrbitals::TOP_MIN) rank.push_back(std::make_pair(-v.share[s], (int)s));
      std::sort(rank.begin(), rank.end());
      for (size_t q = 0; q < rank.size(); q++)
         mi.top.push_back(std::make_pair(rank[q].second, -rank[q].first));
      out.mo.push_back(mi);

      std::ostringstream t;
      const char* kind = lv.character == MoLevel::BONDING ? "bonding"
                       : lv.character == MoLevel::ANTIBONDING ? "antibonding"
                       : "non-bonding";
      t << "Fragment orbitals (extended H\xC3\xBC" "ckel): " << v.klass << ", " << kind
        << ", metal " << percent(v.metal) << ", ligands " << percent(v.ligand)
        << ", covered " << percent(v.capture) << " of the MO";
      if (lv.degeneracy > 1) t << " (mean over its orbitals)";
      t << ".";
      if (!v.irrep.empty()) {
         t << "  Irrep " << v.irrep << " of the skeleton's group " << sk.group;
         if (atomCentre) t << ", fit " << percent(v.fit) << " (local, approximate)";
         t << ".";
      }
      for (size_t q = 0; q < mi.top.size() && q < 4; q++) {
         const SetInfo& si = out.sets[mi.top[q].first];
         t << (q ? "; " : "\nLargest: ") << si.label;
         if (!si.irrep.empty()) t << " " << si.irrep;
         t << " " << percent(mi.top[q].second);
      }
      lv.detail = t.str();
      kept.push_back(lv);
   }
   for (size_t i = 0; i < shownIdx.size(); i++) {
      out.captureMin = std::min(out.captureMin, L[shownIdx[i]].capture);
      out.captureMax = std::max(out.captureMax, L[shownIdx[i]].capture);
   }
   centre.levels = kept;

   //  Partner parents, for the always-linked sets.
   vector< vector<bool> > forced(shownIdx.size(), vector<bool>(ns, false));
   for (size_t p = 0; p < pairList.size(); p++) {
      const int b = newIndex[pairList[p].first], a = newIndex[pairList[p].second];
      if (b < 0 || a < 0) continue;
      forced[b][pairParents[p].first] = forced[b][pairParents[p].second] = true;
      forced[a][pairParents[p].first] = forced[a][pairParents[p].second] = true;
   }
   for (size_t p = 0; p < pairList.size(); p++) {
      PairInfo pi;
      pi.bonding = newIndex[pairList[p].first];
      pi.antibonding = newIndex[pairList[p].second];
      pi.metalSet = pairParents[p].first;
      pi.ligandSet = pairParents[p].second;
      pi.irrep = L[pairList[p].first].irrep;
      out.pairInfo.push_back(pi);
   }
   for (size_t i = 0; i < shownIdx.size(); i++) {
      const Lv& v = L[shownIdx[i]];
      for (size_t s = 0; s < ns; s++) {
         if (!linked[s]) continue;
         if (v.share[s] < LINK_MIN && !forced[i][s]) continue;
         MoConnection link;
         link.leftLevel = link.rightLevel = -1;
         link.centreLevel = (int)i;
         link.weight = v.share[s];
         if (subs[s].metal) link.leftLevel = out.sets[s].column;
         else link.rightLevel = out.sets[s].column;
         out.links.push_back(link);
      }
   }

   std::ostringstream n;
   n << "Fragment levels: extended H\xC3\xBC" "ckel orbitals placed at the diagonal of the "
        "calculation's Fock operator in the orthogonalised fragment-orbital "
        "basis; qualitative.  That operator is known only on the span of the "
        "reported MOs, so the levels depend on which were reported; each is "
        "covered to " << percent(out.captureMin) << " - "
     << percent(out.captureMax) << " (drawn MOs)";
   if (lowCapture > 0)
      n << ", " << lowCapture << " fragment level" << (lowCapture > 1 ? "s" : "")
        << " below " << percent(CAPTURE_OK) << " are marked";
   n << ".  A line is drawn where a fragment orbital makes up at least "
     << percent(LINK_MIN) << " of an orbital, and from both members of a "
        "bonding/antibonding pair to both of their parents.";
   if (haveSym)
      n << "  Symmetry labels are of the coordination skeleton's group, "
        << sk.group << (atomCentre ? " (local, approximate: the rest of the molecule breaks it)."
                                   : " (the ligands' own atoms may lower the molecule's).");
   if (hiddenLigand > 0)
      n << "  " << hiddenLigand << " ligand-internal or low-metal-share level"
        << (hiddenLigand > 1 ? "s" : "") << " not drawn (tick Show all levels).";
   if (farAway > 0)
      n << "  " << farAway << " fragment level" << (farAway > 1 ? "s" : "")
        << " far outside the drawn orbitals not drawn.";
   if (hiddenCore > 0)
      n << "  " << hiddenCore << " level" << (hiddenCore > 1 ? "s" : "")
        << " the valence fragment orbitals do not describe (covered below "
        << percent(FragmentOrbitals::CORE_CAPTURE) << ") not drawn.";
   for (size_t i = 0; i < out.pairMessages.size(); i++)
      n << "\n" << out.pairMessages[i] << ".";
   n << "\n" << out.electrons.line;
   out.note = n.str();

   //  --- the text under the diagram, in three tiers -----------------------
   double covMin = 1.0, covMax = 0.0;
   for (size_t i = 0; i < out.sets.size(); i++) {
      if (out.sets[i].column < 0) continue;
      covMin = std::min(covMin, out.sets[i].capture);
      covMax = std::max(covMax, out.sets[i].capture);
   }
   const string groupText = haveSym ? groupMarkup(sk.group) : string();
   {
      string t = "Fragment levels from extended H\xC3\xBC" "ckel (qualitative) \xC2\xB7 energies in Hartree";
      if (haveSym) t += " \xC2\xB7 labels in " + groupText +
                        (atomCentre ? " (local, approximate)" : "");
      if (hiddenLigand > 0)
         t += " \xC2\xB7 " + plural(hiddenLigand, "level", "levels") + " hidden (Show all levels)";
      out.summary = t;
   }

   out.warnings.clear();
   for (size_t i = 0; i < out.pairMessages.size(); i++)
      out.warnings.push_back(out.pairMessages[i] + ".");
   //  Low coverage only concerns the marked levels, so it is explained in the
   //  About text, next to the percentages, not raised as a warning.
   {
      const ElectronCount& ec = out.electrons;
      char b[300];
      if (ec.placed && !ec.agree) {
         snprintf(b, sizeof b, "The electron count does not add up: %s%d + %s %d = %d, "
                  "but the valence orbitals the diagram considers hold %.0f.",
                  atomCentre ? (ec.metal + " ").c_str() : "metal d", ec.dCount,
                  atomCentre ? "pieces" : "ligands", ec.ligands,
                  ec.dCount + ec.ligands, ec.moElectrons);
         out.warnings.push_back(b);
      } else if (!ec.placed) {
         snprintf(b, sizeof b, "No %s electrons are drawn: %s.",
                  atomCentre ? "centre-atom" : "metal d", ec.why.c_str());
         out.warnings.push_back(b);
      }
   }

   {
      std::ostringstream a;
      if (atomCentre)
         a << "**Where the fragment levels come from.** The left column is the valence "
              "orbitals of the picked " << symbols[pickAtom] << " atom, the right column the orbitals of "
              "the pieces left when it is removed (" << out.pieces << "), each piece neutral, "
              "from a quick extended H\xC3\xBC" "ckel calculation. Each is placed at the energy it has inside "
              "the molecule before it mixes with anything. This puts every bonding level "
              "below the two levels it comes from and every antibonding level above them, "
              "as in a textbook diagram. Treat their positions as qualitative.\n\n";
      else
      a << "**Where the fragment levels come from.** The left and right columns are the "
           "orbitals of the metal atom and of the ligands on their own, from a quick "
           "extended H\xC3\xBC" "ckel calculation. Each is placed at the energy it has inside "
           "the molecule before it mixes with anything. This puts every bonding level "
           "below the two levels it comes from and every antibonding level above them, "
           "as in a textbook diagram. Treat their positions as qualitative.\n\n";
      {
         char lo[16], hi[16];
         snprintf(lo, sizeof lo, "%.0f", 100.0*covMin);
         snprintf(hi, sizeof hi, "%.0f", 100.0*covMax);
         a << "**How reliable each fragment level is: " << lo;
         if (string(lo) != hi) a << "\xE2\x80\x93" << hi;
         a << " %";
      }
      a << ".** A fragment level can only be placed using the orbitals the calculation "
           "reported. The percentage says how much of the fragment orbital those orbitals "
           "cover. Levels below " << spacedPercent(CAPTURE_OK)
        << " show their percentage next to them; their positions are less certain.\n\n";
      a << "**Lines.** A line joins a molecular orbital to a fragment orbital that makes up "
           "at least " << spacedPercent(LINK_MIN) << " of it. Both members of a "
           "bonding/antibonding pair are joined to both of their parents.\n\n";
      if (haveSym && atomCentre) {
         int donors = 0;
         for (size_t i = 0; i < sk.inSkeleton.size(); i++) if (sk.inSkeleton[i]) donors++;
         donors = std::max(donors - 1, 0);
         a << "**Symmetry labels** are those of the local skeleton, " << symbols[pickAtom]
           << " and its " << countWord(donors) << " bonded atom" << (donors == 1 ? "" : "s")
           << " (" << groupText << "), and are approximate: the rest of the molecule breaks "
              "that symmetry. Beside each orbital is how well it fits its label. A label is "
              "shown as it is when the fit is " << spacedPercent(FIT_LABEL_OK) << " or more, with a ~ in front "
              "from " << spacedPercent(FIT_LABEL_MIN) << ", and left off below that.\n\n";
      } else if (atomCentre) {
         a << "**Symmetry labels** are not given: the picked atom and the atoms bonded to it "
              "have no symmetry to label the orbitals by.\n\n";
      } else if (haveSym) {
         int donors = 0;
         for (size_t i = 0; i < sk.inSkeleton.size(); i++) if (sk.inSkeleton[i]) donors++;
         donors = std::max(donors - 1, 0);
         a << "**Symmetry labels** are in " << groupText << ", the symmetry of the "
           << (out.electrons.metal.empty() ? string("metal") : out.electrons.metal)
           << " and its " << countWord(donors) << " donor atom" << (donors == 1 ? "" : "s")
           << ". A ligand's own atoms can lower the symmetry of the whole molecule (the "
              "hydrogens of NH3, for example), so the labels may be higher than the "
              "molecule's own.\n\n";
      }
      const string core = coreSentence(centre);
      if (hiddenCore > 0 || hiddenLigand > 0 || farAway > 0 || !core.empty()) {
         a << "**Not shown:**";
         if (hiddenCore > 0) {
            a << " " << hiddenCore << " molecular orbital" << (hiddenCore == 1 ? "" : "s")
              << " that " << (hiddenCore == 1 ? "is" : "are")
              << " less than half made of the fragments' valence orbitals.";
            if (coreEmpty*5 >= hiddenCore*4)
               a << " " << (coreEmpty == hiddenCore ? "All" : "Nearly all") << " of them are high, empty orbitals.";
            else
               a << " " << coreEmpty << " of them " << (coreEmpty == 1 ? "is" : "are")
                 << " empty and " << hiddenCore - coreEmpty << " "
                 << (hiddenCore - coreEmpty == 1 ? "is" : "are") << " occupied.";
         }
         if (hiddenLigand > 0)
            a << " " << plural(hiddenLigand, "further level sits", "further levels sit")
              << " on the ligands alone or "
              << (hiddenLigand == 1 ? "has" : "have")
              << " little metal character; tick Show all levels to draw "
              << (hiddenLigand == 1 ? "it" : "them") << ".";
         if (farAway > 0)
            a << " " << plural(farAway, "fragment level lies", "fragment levels lie") << " far outside "
                 "the energy range of the drawn orbitals.";
         if (!core.empty()) a << " " << core;
         a << "\n\n";
      }
      {
         const ElectronCount& ec = out.electrons;
         char b[200];
         if (ec.placed) {
            if (atomCentre)
               snprintf(b, sizeof b, "**Total electrons: %d** (%s %d, pieces %d).",
                        ec.dCount + ec.ligands, ec.metal.c_str(), ec.dCount, ec.ligands);
            else
               snprintf(b, sizeof b, "**Total electrons: %d** (%s d%d, ligands %d).",
                        ec.dCount + ec.ligands, ec.metal.c_str(), ec.dCount, ec.ligands);
         } else {
            snprintf(b, sizeof b, "**Total electrons:** the %s electrons are not drawn "
                     "(%s). The %s bring %d; the valence orbitals hold %.0f.",
                     atomCentre ? "centre atom's" : "metal's d", ec.why.c_str(),
                     atomCentre ? "pieces" : "ligands", ec.ligands, ec.moElectrons);
         }
         a << b;
      }
      out.about = a.str();
   }
   return true;
}

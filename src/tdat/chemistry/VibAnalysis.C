#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>

#include "tdat/VibAnalysis.H"
#include "tdat/CharacterTable.H"
#include "tdat/Fragment.H"
#include "tdat/MoFragments.H"
#include "tdat/SymmetryAnalysis.H"
#include "tdat/SymmetryOps.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"
#include "tdat/TPerTab.H"

using std::map;
using std::set;
using std::pair;

namespace {

//  Below this a "mode" is a rotation or translation, which carries no
//  symmetry label of its own.
const double ZERO_FREQ = 10.0;

string upperCase(string s)
{
   for (size_t i = 0; i < s.size(); i++) s[i] = toupper((unsigned char)s[i]);
   return s;
}

//  ------------------------------------------------------------------
//  Symmetry
//  ------------------------------------------------------------------

typedef vector<double> Vec;

double dot(const Vec& a, const Vec& b)
{
   double s = 0.0;
   for (size_t i = 0; i < a.size(); i++) s += a[i]*b[i];
   return s;
}

//  R acting on a displacement field: atom a's vector goes to its image.
Vec applyOp(const SymOp& op, const vector<int>& image, const Vec& d)
{
   Vec out(d.size(), 0.0);
   for (size_t a = 0; a < image.size(); a++) {
      for (int r = 0; r < 3; r++) {
         double s = 0.0;
         for (int c = 0; c < 3; c++) s += op.m[r][c]*d[3*a + c];
         out[3*image[a] + r] = s;
      }
   }
   return out;
}

//  Multiplicity of each irrep in a representation with the given class
//  characters.  Accepts noise of 0.2 per irrep and insists the dimensions
//  add up; anything else is "not a representation of this group".
bool decompose(const CharacterTable& table, const vector<double>& chi,
               vector<int>& mult)
{
   const vector<string>& irreps = table.irreps();
   const vector<int>& counts = table.counts();
   mult.assign(irreps.size(), 0);
   double dims = 0.0;
   for (size_t i = 0; i < irreps.size(); i++) {
      const vector<double>* row = table.characters(irreps[i]);
      if (row == 0 || row->size() != chi.size()) return false;
      double t = 0.0;
      for (size_t c = 0; c < chi.size(); c++) t += counts[c]*(*row)[c]*chi[c];
      t /= table.order()*(table.separable(irreps[i]) ? 2 : 1);
      const double r = floor(t + 0.5);
      if (fabs(t - r) > 0.2 || r < 0.0) return false;
      mult[i] = (int)r;
      dims += r*table.dimension(irreps[i]);
   }
   return true;
}

bool isLinear(const vector<double>& x)
{
   const size_t n = x.size()/3;
   if (n < 2) return false;
   double u[3] = { x[3]-x[0], x[4]-x[1], x[5]-x[2] };
   const double un = sqrt(u[0]*u[0] + u[1]*u[1] + u[2]*u[2]);
   if (un < 1e-8) return false;
   for (size_t a = 2; a < n; a++) {
      double v[3] = { x[3*a]-x[0], x[3*a+1]-x[1], x[3*a+2]-x[2] };
      const double c[3] = { u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2],
                            u[0]*v[1]-u[1]*v[0] };
      if (sqrt(c[0]*c[0] + c[1]*c[1] + c[2]*c[2])/un > 0.02) return false;
   }
   return true;
}

//  ------------------------------------------------------------------
//  Internal coordinates
//  ------------------------------------------------------------------

enum CoordType { STRETCH, BEND, OOP, TORSION };

struct Coord
{
   CoordType type;
   int at[4];
   int n;
   bool linear;       // one of the two perpendicular components of a 180 bend
   Vec b;             // d(coordinate)/d(Cartesian), 3N
   double G;          // sum |b|^2/m
   double value;      // length / angle in degrees, for telling classes apart
   double order;      // stretch bond order
   int group;
};

double norm3(const double *v) { return sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]); }

void cross(const double *a, const double *b, double *c)
{
   c[0] = a[1]*b[2]-a[2]*b[1];
   c[1] = a[2]*b[0]-a[0]*b[2];
   c[2] = a[0]*b[1]-a[1]*b[0];
}

double evalCoord(const Coord& q, const Vec& x)
{
   const double *p[4];
   for (int i = 0; i < q.n; i++) p[i] = &x[3*q.at[i]];
   double u[3], v[3], w[3];
   switch (q.type) {
   case STRETCH:
      for (int k = 0; k < 3; k++) u[k] = p[0][k] - p[1][k];
      return norm3(u);
   case BEND: {
      for (int k = 0; k < 3; k++) { u[k] = p[0][k]-p[1][k]; v[k] = p[2][k]-p[1][k]; }
      double c = (u[0]*v[0]+u[1]*v[1]+u[2]*v[2])/(norm3(u)*norm3(v));
      c = std::max(-1.0, std::min(1.0, c));
      return acos(c);
   }
   case OOP: {
      //  Height of the centre over the plane of its three neighbours.
      double a[3], b[3], nrm[3];
      for (int k = 0; k < 3; k++) { a[k] = p[2][k]-p[1][k]; b[k] = p[3][k]-p[1][k]; }
      cross(a, b, nrm);
      const double l = norm3(nrm);
      if (l < 1e-12) return 0.0;
      double h = 0.0;
      for (int k = 0; k < 3; k++) {
         const double cen = (p[1][k]+p[2][k]+p[3][k])/3.0;
         h += nrm[k]/l*(p[0][k]-cen);
      }
      return h;
   }
   case TORSION: {
      double b1[3], b2[3], b3[3], n1[3], n2[3], m1[3];
      for (int k = 0; k < 3; k++) {
         b1[k] = p[1][k]-p[0][k]; b2[k] = p[2][k]-p[1][k]; b3[k] = p[3][k]-p[2][k];
      }
      cross(b1, b2, n1); cross(b2, b3, n2);
      const double l2 = norm3(b2);
      double b2n[3] = { b2[0]/l2, b2[1]/l2, b2[2]/l2 };
      cross(n1, b2n, m1);
      const double xx = n1[0]*n2[0]+n1[1]*n2[1]+n1[2]*n2[2];
      const double yy = m1[0]*n2[0]+m1[1]*n2[1]+m1[2]*n2[2];
      return atan2(yy, xx);
   }
   }
   return 0.0;
}

//  Numerical B row: the molecules are small and this has to be right for
//  four different coordinate types, so one derivative routine serves all.
void numericB(Coord& q, const Vec& x)
{
   const double h = 1e-4;
   q.b.assign(x.size(), 0.0);
   Vec y = x;
   for (int i = 0; i < q.n; i++) {
      for (int k = 0; k < 3; k++) {
         const int idx = 3*q.at[i] + k;
         y[idx] = x[idx] + h;
         const double fp = evalCoord(q, y);
         y[idx] = x[idx] - h;
         const double fm = evalCoord(q, y);
         y[idx] = x[idx];
         double d = fp - fm;
         if (q.type == TORSION) {
            while (d > M_PI) d -= 2*M_PI;
            while (d < -M_PI) d += 2*M_PI;
         }
         q.b[idx] = d/(2*h);
      }
   }
}

struct Topology
{
   vector< vector<int> > nbr;
   map<pair<int,int>, double> order;
};

double bondOrder(const Topology& t, int a, int b)
{
   map<pair<int,int>, double>::const_iterator it =
      t.order.find(pair<int,int>(std::min(a,b), std::max(a,b)));
   return it == t.order.end() ? 1.0 : it->second;
}

//  nu with a combining tilde (the wavenumber symbol): a bare nu is drawn
//  as a v in Cantarell and most UI fonts.
const char *NU = "\xce\xbd\xcc\x83", *DELTA = "\xce\xb4", *RHO = "\xcf\x81",
           *OMEGA = "\xcf\x89", *TAU = "\xcf\x84", *GAMMA = "\xce\xb3";
const char *ENDASH = "\xe2\x80\x93";

//  Do these modes belong to this geometry, in this frame?  A real vibration
//  has no translation or rotation in it; modes printed in another
//  orientation (MOPAC reorients for its force run) fail the rotation part,
//  and labels computed from them would be wrong rather than missing.
//  Returns the largest share of any vibration that lies in the six
//  translations and rotations (mass-weighted).
double rigidShare(const VibInput& in, const vector<double>& mass)
{
   const size_t N = in.elements.size();
   double com[3] = { 0, 0, 0 }, M = 0.0;
   for (size_t a = 0; a < N; a++) {
      M += mass[a];
      for (int k = 0; k < 3; k++) com[k] += mass[a]*in.coords[3*a+k];
   }
   for (int k = 0; k < 3; k++) com[k] /= M;

   vector<Vec> basis;
   for (int k = 0; k < 6; k++) {
      Vec v(3*N, 0.0);
      for (size_t a = 0; a < N; a++) {
         const double r[3] = { in.coords[3*a]-com[0], in.coords[3*a+1]-com[1],
                               in.coords[3*a+2]-com[2] };
         const double s = sqrt(mass[a]);
         for (int c = 0; c < 3; c++) {
            if (k < 3) v[3*a+c] = (c == k) ? s : 0.0;
            else {
               const int i = k - 3;
               double e[3] = { 0, 0, 0 };
               e[i] = 1.0;
               const double cr[3] = { e[1]*r[2]-e[2]*r[1], e[2]*r[0]-e[0]*r[2],
                                      e[0]*r[1]-e[1]*r[0] };
               v[3*a+c] = s*cr[c];
            }
         }
      }
      for (size_t j = 0; j < basis.size(); j++) {
         const double p = dot(v, basis[j]);
         for (size_t i = 0; i < v.size(); i++) v[i] -= p*basis[j][i];
      }
      const double l = sqrt(dot(v, v));
      if (l < 1e-6) continue;       // a linear molecule has no third rotation
      for (size_t i = 0; i < v.size(); i++) v[i] /= l;
      basis.push_back(v);
   }

   double worst = 0.0;
   for (size_t m = 0; m < in.modes.size(); m++) {
      if (fabs(in.freqs[m]) < ZERO_FREQ || in.modes[m].size() != 3*N) continue;
      Vec q(3*N);
      for (size_t a = 0; a < N; a++)
         for (int c = 0; c < 3; c++) q[3*a+c] = sqrt(mass[a])*in.modes[m][3*a+c];
      const double qq = dot(q, q);
      if (qq < 1e-20) continue;
      double inside = 0.0;
      for (size_t j = 0; j < basis.size(); j++) {
         const double p = dot(q, basis[j]);
         inside += p*p;
      }
      worst = std::max(worst, inside/qq);
   }
   return worst;
}

}   // namespace


string VibAnalysis::linearName(const string& irrep, const string& group)
{
   const string g = upperCase(group), i = upperCase(irrep);
   const string sigma = "\xce\xa3", pi = "\xce\xa0";
   const string plus = "+", minus = "\xe2\x88\x92";
   if (g == "D4H") {
      if (i == "A1G") return sigma + "g" + plus;
      if (i == "A2G") return sigma + "g" + minus;
      if (i == "A1U") return sigma + "u" + minus;
      if (i == "A2U") return sigma + "u" + plus;
      if (i == "EG") return pi + "g";
      if (i == "EU") return pi + "u";
   } else if (g == "C4V") {
      if (i == "A1") return sigma + plus;
      if (i == "A2") return sigma + minus;
      if (i == "E") return pi;
   }
   return irrep;
}



//  The six zero-frequency rows are the translations and rotations of the
//  whole molecule.  Each ideal motion (in the symmetry frame, where the
//  axes are the group's) belongs to one irrep, found with the projection
//  operator of the group's own character table.  Which row is which is only
//  known from the displacements: a row that is dominated by one ideal motion
//  gets its name; a row without any displacement (ORCA, ECCE-QM print
//  zeros) gets the names in the order every code lists them, Tx Ty Tz Rx Ry
//  Rz, and says so; a row that is a mixture of them stays unnamed.
static void labelRigid(const VibInput& in, const vector<Vec>& modes,
                const vector<double>& stdCoords, const vector<SymOp>& ops,
                const CharacterTable& table, const vector<string>& shown,
                const vector<int>& classOfOp, bool linear, const string& group,
                VibResult& out)
{
   const size_t N = in.elements.size(), nm = in.modes.size();
   TPerTab pt(true);
   vector<double> mass;
   for (size_t a = 0; a < N; a++)
      mass.push_back(pt.atomicWeight(pt.atomicNumber(in.elements[a])));
   double com[3] = { 0, 0, 0 }, M = 0.0;
   for (size_t a = 0; a < N; a++) {
      if (mass[a] <= 0) return;
      M += mass[a];
      for (int k = 0; k < 3; k++) com[k] += mass[a]*stdCoords[3*a+k];
   }
   for (int k = 0; k < 3; k++) com[k] /= M;

   //  The ideal motions, mass-weighted, normalised; axis k of the frame.
   struct Ideal { string name, kind; Vec v; string irrep; bool rotation; int axis; };
   vector<Ideal> ideals;
   //  The planar-C2v display swap renames B1/B2; the axes follow it, so
   //  that Tx is B1 and Ty is B2 as in the textbooks.
   bool swapped = false;
   for (size_t i = 0; i < shown.size(); i++)
      if (table.irreps()[i] != shown[i]) swapped = true;
   const char *axis = "xyz";
   for (int rot = 0; rot < 2; rot++) {
      for (int k = 0; k < 3; k++) {
         Ideal id;
         id.rotation = rot == 1;
         id.axis = k;
         id.v.assign(3*N, 0.0);
         for (size_t a = 0; a < N; a++) {
            const double r[3] = { stdCoords[3*a]-com[0], stdCoords[3*a+1]-com[1],
                                  stdCoords[3*a+2]-com[2] };
            const double s = sqrt(mass[a]);
            double e[3] = { 0, 0, 0 };
            e[k] = 1.0;
            if (!rot) { for (int c = 0; c < 3; c++) id.v[3*a+c] = s*e[c]; }
            else {
               const double cr[3] = { e[1]*r[2]-e[2]*r[1], e[2]*r[0]-e[0]*r[2],
                                      e[0]*r[1]-e[1]*r[0] };
               for (int c = 0; c < 3; c++) id.v[3*a+c] = s*cr[c];
            }
         }
         const double l = sqrt(dot(id.v, id.v));
         if (l < 1e-6) continue;           // no rotation about a linear molecule's axis
         for (size_t i = 0; i < id.v.size(); i++) id.v[i] /= l;

         //  Irrep: the one whose projection operator leaves the unit vector alone.
         id.irrep.clear();
         for (size_t g = 0; g < table.irreps().size(); g++) {
            const vector<double> *chi = table.characters(table.irreps()[g]);
            if (chi == 0) continue;
            double p[3] = { 0, 0, 0 };
            for (size_t o = 0; o < ops.size(); o++) {
               const double sign = rot ? ops[o].determinant() : 1.0;
               const double w = (*chi)[classOfOp[o]]*sign;
               for (int r = 0; r < 3; r++) p[r] += w*ops[o].m[r][k];
            }
            const double d = table.dimension(table.irreps()[g]);
            double len2 = 0.0;
            for (int r = 0; r < 3; r++) { p[r] *= d/ops.size(); len2 += p[r]*p[r]; }
            if (fabs(len2 - 1.0) < 0.02) {
               id.irrep = linear ? VibAnalysis::linearName(shown[g], group) : shown[g];
               break;
            }
         }
         int shownAxis = k;
         if (swapped && k < 2) shownAxis = 1 - k;
         id.name = string(rot ? "R" : "T") + axis[shownAxis];
         id.kind = string(rot ? "rotation about " : "translation along ") + axis[shownAxis];
         ideals.push_back(id);
      }
   }
   std::sort(ideals.begin(), ideals.end(), [](const Ideal& a, const Ideal& b) {
      return a.name < b.name; });
   //  Tx Ty Tz Rx Ry Rz: "R" sorts after "T" otherwise.
   std::stable_sort(ideals.begin(), ideals.end(), [](const Ideal& a, const Ideal& b) {
      return a.rotation < b.rotation; });

   vector<size_t> rows;
   for (size_t m = 0; m < nm; m++)
      if (fabs(in.freqs[m]) < ZERO_FREQ) rows.push_back(m);
   if (rows.size() != ideals.size()) return;

   vector<bool> hasVector(nm, false);
   bool anyVector = false;
   for (size_t k = 0; k < rows.size(); k++) {
      const Vec& d = modes[rows[k]];
      hasVector[rows[k]] = sqrt(dot(d, d)) > 1e-4;
      anyVector = anyVector || hasVector[rows[k]];
   }

   vector<int> given(ideals.size(), 0);
   auto name = [&](size_t m, size_t which, bool nominal) {
      VibModeInfo& info = out.modes[m];
      info.rigid = true;
      info.label = ideals[which].name;
      info.irrep = ideals[which].irrep;
      info.detail = ideals[which].kind;
      if (nominal)
         info.remark = "The output holds no displacement for this row, so it is named "
                       "by position: translations x y z, then rotations x y z.";
      given[which]++;
   };
   if (!anyVector) {
      for (size_t k = 0; k < rows.size(); k++) name(rows[k], k, true);
      return;
   }
   for (size_t k = 0; k < rows.size(); k++) {
      const size_t m = rows[k];
      if (!hasVector[m]) continue;
      Vec q(3*N);
      for (size_t a = 0; a < N; a++)
         for (int c = 0; c < 3; c++) q[3*a+c] = sqrt(mass[a])*modes[m][3*a+c];
      const double qq = dot(q, q);
      int best = -1;
      double bestShare = 0.0;
      for (size_t w = 0; w < ideals.size(); w++) {
         const double p = dot(q, ideals[w].v);
         const double share = p*p/qq;
         if (share > bestShare) { bestShare = share; best = (int)w; }
      }
      if (best >= 0 && bestShare > 0.9) name(m, (size_t)best, false);
   }
   //  Two rows claiming one motion: neither is trusted.
   for (size_t w = 0; w < ideals.size(); w++) {
      if (given[w] < 2) continue;
      for (size_t m : rows)
         if (out.modes[m].rigid && out.modes[m].label == ideals[w].name) {
            out.modes[m].rigid = false;
            out.modes[m].label.clear();
            out.modes[m].irrep.clear();
            out.modes[m].detail.clear();
         }
   }
}


bool VibAnalysis::symmetry(const VibInput& in, const string& group,
                           const vector<double>& stdCoords,
                           const vector<SymOp>& ops, VibResult& out)
{
   const size_t N = in.elements.size();
   const size_t nm = in.modes.size();
   out.modes.resize(nm);
   for (size_t m = 0; m < nm; m++) {
      out.modes[m].irrep.clear();
      out.modes[m].activityKnown = false;
      out.modes[m].ir = out.modes[m].raman = false;
      out.modes[m].rigid = false;
   }
   if (N == 0 || in.coords.size() != 3*N || stdCoords.size() != 3*N) return false;

   const CharacterTable *table = CharacterTable::lookup(group);
   if (table == 0) { out.note = "no character table for " + group; return false; }

   vector< vector<int> > images;
   if (!SymmetryAnalysis::atomImages(stdCoords, in.elements, ops, 0.05, images)) {
      out.note = "operations do not map the atoms onto themselves";
      return false;
   }
   vector< vector<int> > classes;
   SymmetryAnalysis::conjugacyClasses(ops, classes);
   vector<int> classOfOp;
   if (!SymmetryAnalysis::matchClasses(ops, classes, *table, classOfOp)) {
      out.note = "operations do not match the character table";
      return false;
   }
   const size_t nclass = table->classes().size();

   //  The modes are in the code's frame, the operations in the standard
   //  one: rotate the displacements, which are free vectors.
   SymOp Q;
   double rmsd = 0.0;
   if (!SymmetryAnalysis::alignFrames(in.coords, stdCoords, Q, rmsd) ||
       rmsd > 0.1) {
      out.note = "structure and symmetry frame disagree";
      return false;
   }
   vector<Vec> modes(nm, Vec(3*N));
   for (size_t m = 0; m < nm; m++) {
      if (in.modes[m].size() != 3*N) return false;
      for (size_t a = 0; a < N; a++)
         for (int r = 0; r < 3; r++) {
            double s = 0.0;
            for (int c = 0; c < 3; c++) s += Q.m[r][c]*in.modes[m][3*a+c];
            modes[m][3*a+r] = s;
         }
   }

   //  Activity: the irreps of x,y,z (IR) and of the six products x_i x_j
   //  (Raman), from the operations rather than from a table of functions.
   vector<double> chiVec(nclass, 0.0), chiQuad(nclass, 0.0), nOp(nclass, 0.0);
   for (size_t o = 0; o < ops.size(); o++) {
      const int c = classOfOp[o];
      const SymOp& R = ops[o];
      double tr2 = 0.0;
      for (int i = 0; i < 3; i++)
         for (int j = 0; j < 3; j++) tr2 += R.m[i][j]*R.m[j][i];
      const double t = R.trace();
      chiVec[c] += t;
      chiQuad[c] += 0.5*(t*t + tr2);
      nOp[c] += 1.0;
   }
   for (size_t c = 0; c < nclass; c++)
      if (nOp[c] > 0) { chiVec[c] /= nOp[c]; chiQuad[c] /= nOp[c]; }
   vector<int> multVec, multQuad;
   const bool haveAct = decompose(*table, chiVec, multVec) &&
                        decompose(*table, chiQuad, multQuad);
   //  Displayed names.  For a planar C2v molecule the textbooks (Mulliken)
   //  put the molecule in the yz plane, so out-of-plane motion is B1 and
   //  the in-plane antisymmetric stretch B2; autosym's frame can have it
   //  the other way round, which only renames the two irreps.
   vector<string> irreps = table->irreps();
   if (upperCase(group) == "C2V") {
      for (size_t o = 0; o < ops.size(); o++) {
         if (fabs(ops[o].trace() - 1.0) > 1e-6 || ops[o].determinant() > 0) continue;
         bool fixesAll = true;
         for (size_t a = 0; a < N; a++) if (images[o][a] != (int)a) fixesAll = false;
         if (!fixesAll) continue;
         const vector<double> *b1 = table->characters("B1");
         if (b1 != 0 && (*b1)[classOfOp[o]] > 0) {
            for (size_t i = 0; i < irreps.size(); i++) {
               if (irreps[i] == "B1") irreps[i] = "B2";
               else if (irreps[i] == "B2") irreps[i] = "B1";
            }
         }
         break;
      }
   }

   const bool linear = isLinear(stdCoords) &&
      (upperCase(group) == "D4H" || upperCase(group) == "C4V");

   //  Degenerate sets: neighbours in frequency.
   vector<size_t> order;
   for (size_t m = 0; m < nm; m++)
      if (fabs(in.freqs[m]) >= ZERO_FREQ) order.push_back(m);
   std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
      return in.freqs[a] < in.freqs[b]; });

   //  Irrep of a set of modes, or "" when they are not one irrep.
   auto classify = [&](const vector<size_t>& set, int& irrepIndex) -> bool {
      //  Orthonormal basis of the set.
      vector<Vec> basis;
      for (size_t k = 0; k < set.size(); k++) {
         Vec v = modes[set[k]];
         for (size_t j = 0; j < basis.size(); j++) {
            const double p = dot(v, basis[j]);
            for (size_t i = 0; i < v.size(); i++) v[i] -= p*basis[j][i];
         }
         const double l = sqrt(dot(v, v));
         if (l < 1e-6) return false;
         for (size_t i = 0; i < v.size(); i++) v[i] /= l;
         basis.push_back(v);
      }
      vector<double> chi(nclass, 0.0);
      for (size_t o = 0; o < ops.size(); o++) {
         double tr = 0.0, inside = 0.0;
         for (size_t k = 0; k < basis.size(); k++) {
            const Vec rv = applyOp(ops[o], images[o], basis[k]);
            tr += dot(basis[k], rv);
            for (size_t j = 0; j < basis.size(); j++) {
               const double p = dot(basis[j], rv);
               inside += p*p;
            }
         }
         //  An operation must carry the set into itself.
         if (fabs(inside - basis.size()) > 0.1*basis.size()) return false;
         chi[classOfOp[o]] += tr;
      }
      for (size_t c = 0; c < nclass; c++) if (nOp[c] > 0) chi[c] /= nOp[c];
      vector<int> mult;
      if (!decompose(*table, chi, mult)) return false;
      int found = -1, total = 0;
      for (size_t i = 0; i < mult.size(); i++) {
         total += mult[i];
         if (mult[i] > 0) found = (int)i;
      }
      if (total != 1 || found < 0) return false;
      if (table->dimension(irreps[found]) != (int)set.size()) return false;
      irrepIndex = found;
      return true;
   };

   auto assign = [&](const vector<size_t>& set, int idx) {
      for (size_t k = 0; k < set.size(); k++) {
         VibModeInfo& info = out.modes[set[k]];
         info.irrep = linear ? linearName(irreps[idx], group) : irreps[idx];
         if (haveAct) {
            info.activityKnown = true;
            info.ir = multVec[idx] > 0;
            info.raman = multQuad[idx] > 0;
         }
      }
   };

   size_t i = 0;
   while (i < order.size()) {
      size_t j = i + 1;
      while (j < order.size() &&
             fabs(in.freqs[order[j]] - in.freqs[order[j-1]]) <
                1.0 + 0.002*fabs(in.freqs[order[j]])) j++;
      vector<size_t> set(order.begin() + i, order.begin() + j);
      int idx = -1;
      if (classify(set, idx)) {
         assign(set, idx);
      } else {
         //  Accidentally close but different irreps, or noise: try each
         //  alone, and say "?" for what is still not one irrep.
         for (size_t k = 0; k < set.size(); k++) {
            vector<size_t> one(1, set[k]);
            if (classify(one, idx)) assign(one, idx);
            else out.modes[set[k]].irrep = "?";
         }
      }
      i = j;
   }
   labelRigid(in, modes, stdCoords, ops, *table, irreps, classOfOp, linear, group, out);
   return true;
}


//  ----------------------------------------------------------------------
//  Description
//  ----------------------------------------------------------------------

namespace {

string elementKey(const string& a, const string& b)
{
   return a < b ? a + "-" + b : b + "-" + a;
}

//  H first (terminal atoms in bends), others by atomic number.
bool heavierThan(int za, int zb) { return za < zb; }

string orderedSymbols(const vector<int>& z, TPerTab& pt, bool hydrogenLast)
{
   vector<int> s(z);
   std::sort(s.begin(), s.end(), [&](int a, int b) {
      if (a == 1 || b == 1) {
         if (a == b) return false;
         return hydrogenLast ? b == 1 : a == 1;
      }
      return heavierThan(a, b);
   });
   string out;
   for (size_t i = 0; i < s.size(); i++) out += pt.atomicSymbol(s[i]);
   return out;
}

struct GroupInfo
{
   CoordType type;
   vector<int> members;
   //  For naming
   int centreZ;                 // bend/oop centre, torsion first atom
   vector<int> endZ;            // stretch ends, bend terminals, torsion bond
   int hCount;                  // hydrogen neighbours of the centre
   int otherCount;              // non-hydrogen neighbours of the centre
   double order;                // common bond order, 0 if mixed
   bool rockCandidate;
   string key;
};

struct Part
{
   int group;
   double share;
};

}   // namespace


void VibAnalysis::describe(const VibInput& in, VibResult& out)
{
   const size_t N = in.elements.size();
   const size_t nm = in.modes.size();
   out.modes.resize(nm);
   for (size_t m = 0; m < nm; m++) { out.modes[m].label.clear(); out.modes[m].detail.clear(); }
   if (N < 2 || in.coords.size() != 3*N) return;

   TPerTab pt(true);
   vector<int> Z(N);
   vector<double> mass(N);
   for (size_t a = 0; a < N; a++) {
      Z[a] = pt.atomicNumber(in.elements[a]);
      mass[a] = pt.atomicWeight(Z[a]);
      if (Z[a] <= 0 || mass[a] <= 0) return;
   }

   //  Bonds: the Builder's if given, else by covalent radii (order 1).
   Topology topo;
   topo.nbr.assign(N, vector<int>());
   vector<VibBond> bonds = in.bonds;
   if (bonds.empty()) {
      for (size_t a = 0; a < N; a++)
         for (size_t b = a + 1; b < N; b++) {
            double d[3];
            for (int k = 0; k < 3; k++) d[k] = in.coords[3*a+k]-in.coords[3*b+k];
            const double lim = 1.2*(pt.covalentRadius(Z[a]) + pt.covalentRadius(Z[b]));
            if (norm3(d) < lim) { VibBond vb = { (int)a, (int)b, 1.0 }; bonds.push_back(vb); }
         }
   }
   for (size_t k = 0; k < bonds.size(); k++) {
      const int a = bonds[k].a, b = bonds[k].b;
      if (a < 0 || b < 0 || a >= (int)N || b >= (int)N || a == b) continue;
      if (bonds[k].order > 0 && bonds[k].order < 0.9) continue;   // H bonds, metal contacts
      topo.nbr[a].push_back(b);
      topo.nbr[b].push_back(a);
      topo.order[pair<int,int>(std::min(a,b), std::max(a,b))] = bonds[k].order;
   }
   for (size_t a = 0; a < N; a++) std::sort(topo.nbr[a].begin(), topo.nbr[a].end());

   const Vec& x = in.coords;
   vector<Coord> coords;
   auto add = [&](CoordType t, int a0, int a1, int a2, int a3, int n) {
      Coord q;
      q.type = t; q.at[0] = a0; q.at[1] = a1; q.at[2] = a2; q.at[3] = a3;
      q.n = n; q.linear = false; q.order = 0; q.group = -1;
      return q;
   };

   //  Stretches.
   for (size_t a = 0; a < N; a++)
      for (size_t k = 0; k < topo.nbr[a].size(); k++) {
         const int b = topo.nbr[a][k];
         if (b < (int)a) continue;
         Coord q = add(STRETCH, (int)a, b, -1, -1, 2);
         q.value = evalCoord(q, x);
         q.order = bondOrder(topo, (int)a, b);
         numericB(q, x);
         coords.push_back(q);
      }
   //  Bends; a (nearly) straight angle has two perpendicular components.
   for (size_t c = 0; c < N; c++)
      for (size_t i = 0; i < topo.nbr[c].size(); i++)
         for (size_t k = i + 1; k < topo.nbr[c].size(); k++) {
            const int a = topo.nbr[c][i], b = topo.nbr[c][k];
            Coord q = add(BEND, a, (int)c, b, -1, 3);
            q.value = evalCoord(q, x) * 180.0/M_PI;
            if (q.value > 175.0) {
               double e1[3], e2[3];
               for (int t = 0; t < 3; t++) {
                  e1[t] = x[3*a+t]-x[3*c+t]; e2[t] = x[3*b+t]-x[3*c+t];
               }
               const double r1 = norm3(e1), r2 = norm3(e2);
               double axis[3] = { 1, 0, 0 };
               if (fabs(e1[0]) > 0.9*r1) { axis[0] = 0; axis[1] = 1; }
               double p1[3], p2[3];
               cross(e1, axis, p1);
               const double l1 = norm3(p1);
               for (int t = 0; t < 3; t++) p1[t] /= l1;
               cross(e1, p1, p2);
               const double l2 = norm3(p2);
               for (int t = 0; t < 3; t++) p2[t] /= l2;
               const double *ps[2] = { p1, p2 };
               for (int w = 0; w < 2; w++) {
                  Coord lq = q;
                  lq.linear = true;
                  lq.b.assign(3*N, 0.0);
                  for (int t = 0; t < 3; t++) {
                     lq.b[3*a+t] = ps[w][t]/r1;
                     lq.b[3*b+t] = ps[w][t]/r2;
                     lq.b[3*c+t] = -(ps[w][t]/r1 + ps[w][t]/r2);
                  }
                  coords.push_back(lq);
               }
               continue;
            }
            numericB(q, x);
            coords.push_back(q);
         }
   //  Out-of-plane: a centre with three neighbours that lie in one plane
   //  with it (sp2), measured as the centre's height over their plane.
   for (size_t c = 0; c < N; c++) {
      if (topo.nbr[c].size() != 3) continue;
      const int n0 = topo.nbr[c][0], n1 = topo.nbr[c][1], n2 = topo.nbr[c][2];
      const int nb[3] = { n0, n1, n2 };
      double angSum = 0.0;
      for (int i = 0; i < 3; i++)
         for (int k = i + 1; k < 3; k++) {
            Coord t = add(BEND, nb[i], (int)c, nb[k], -1, 3);
            angSum += evalCoord(t, x)*180.0/M_PI;
         }
      if (angSum < 355.0) continue;
      Coord q = add(OOP, (int)c, n0, n1, n2, 4);
      q.value = 0;
      numericB(q, x);
      coords.push_back(q);
   }
   //  Torsions: one per bond, through the heaviest neighbour at each end,
   //  so the nine H-C-C-H of ethane do not outvote everything else.
   for (size_t j = 0; j < N; j++)
      for (size_t kk = 0; kk < topo.nbr[j].size(); kk++) {
         const int k = topo.nbr[j][kk];
         if (k < (int)j) continue;
         int bi = -1, bl = -1;
         for (size_t t = 0; t < topo.nbr[j].size(); t++) {
            const int a = topo.nbr[j][t];
            if (a != k && (bi < 0 || Z[a] > Z[bi])) bi = a;
         }
         for (size_t t = 0; t < topo.nbr[k].size(); t++) {
            const int a = topo.nbr[k][t];
            if (a != (int)j && a != bi && (bl < 0 || Z[a] > Z[bl])) bl = a;
         }
         if (bi < 0 || bl < 0) continue;
         Coord q = add(TORSION, bi, (int)j, k, bl, 4);
         //  A (nearly) straight bend makes the dihedral undefined.
         Coord t1 = add(BEND, bi, (int)j, k, -1, 3), t2 = add(BEND, (int)j, k, bl, -1, 3);
         if (evalCoord(t1, x)*180/M_PI > 175 || evalCoord(t2, x)*180/M_PI > 175) continue;
         q.value = 0;
         numericB(q, x);
         coords.push_back(q);
      }

   if (coords.empty()) return;

   //  A planar centre with neighbours A, B, B has three bends that sum to
   //  360 degrees, so BXB moving out equals the two AXB moving in together:
   //  one motion, three coordinates.  Credit the in-phase AXB pair to BXB.
   struct PlanarTriple { size_t single, p1, p2; };
   vector<PlanarTriple> triples;
   for (size_t c = 0; c < N; c++) {
      if (topo.nbr[c].size() != 3) continue;
      const int nb[3] = { topo.nbr[c][0], topo.nbr[c][1], topo.nbr[c][2] };
      int x = -1, y = -1, z = -1;
      for (int i = 0; i < 3 && x < 0; i++)
         for (int k = i + 1; k < 3; k++)
            if (in.elements[nb[i]] == in.elements[nb[k]]) {
               x = nb[i]; y = nb[k]; z = nb[3 - i - k];
               break;
            }
      if (x < 0 || in.elements[z] == in.elements[x]) continue;
      auto bendIndex = [&](int a, int b) -> int {
         for (size_t i = 0; i < coords.size(); i++)
            if (coords[i].type == BEND && !coords[i].linear && coords[i].at[1] == (int)c &&
                ((coords[i].at[0] == a && coords[i].at[2] == b) ||
                 (coords[i].at[0] == b && coords[i].at[2] == a))) return (int)i;
         return -1;
      };
      const int sIdx = bendIndex(x, y), p1 = bendIndex(x, z), p2 = bendIndex(y, z);
      if (sIdx < 0 || p1 < 0 || p2 < 0) continue;
      double sum = coords[sIdx].value + coords[p1].value + coords[p2].value;
      if (sum < 355.0) continue;
      PlanarTriple t = { (size_t)sIdx, (size_t)p1, (size_t)p2 };
      triples.push_back(t);
   }

   for (size_t i = 0; i < coords.size(); i++) {
      double g = 0.0;
      for (size_t a = 0; a < N; a++)
         for (int t = 0; t < 3; t++) g += coords[i].b[3*a+t]*coords[i].b[3*a+t]/mass[a];
      coords[i].G = g;
   }

   //  Equivalent coordinates -> groups: same type and elements, and the
   //  same length/angle within a tolerance.
   vector<GroupInfo> groups;
   {
      map<string, vector<int> > bySig;
      for (size_t i = 0; i < coords.size(); i++) {
         const Coord& q = coords[i];
         std::ostringstream sig;
         sig << (int)q.type << ":";
         if (q.type == STRETCH) {
            sig << elementKey(in.elements[q.at[0]], in.elements[q.at[1]]);
         } else if (q.type == BEND) {
            sig << elementKey(in.elements[q.at[0]], in.elements[q.at[2]])
                << "@" << in.elements[q.at[1]] << (q.linear ? "L" : "");
         } else if (q.type == OOP) {
            vector<string> e;
            for (int t = 1; t < 4; t++) e.push_back(in.elements[q.at[t]]);
            std::sort(e.begin(), e.end());
            sig << in.elements[q.at[0]] << "|" << e[0] << e[1] << e[2];
         } else {
            sig << elementKey(in.elements[q.at[1]], in.elements[q.at[2]]);
         }
         bySig[sig.str()].push_back((int)i);
      }
      for (map<string, vector<int> >::iterator it = bySig.begin(); it != bySig.end(); ++it) {
         vector<int> idx = it->second;
         std::sort(idx.begin(), idx.end(), [&](int a, int b) {
            return coords[a].value < coords[b].value; });
         const double tol = coords[idx[0]].type == STRETCH ? 0.04 : 5.0;
         size_t s = 0;
         while (s < idx.size()) {
            size_t e = s + 1;
            while (e < idx.size() &&
                   coords[idx[e]].value - coords[idx[e-1]].value < tol) e++;
            GroupInfo g;
            g.type = coords[idx[s]].type;
            g.members.assign(idx.begin() + s, idx.begin() + e);
            g.key = it->first;
            g.order = coords[idx[s]].order;
            for (size_t t = s; t < e; t++)
               if (fabs(coords[idx[t]].order - g.order) > 1e-6) g.order = 0;
            const Coord& q = coords[idx[s]];
            g.centreZ = 0; g.hCount = 0; g.otherCount = 0; g.rockCandidate = false;
            if (q.type == STRETCH) {
               g.endZ.push_back(Z[q.at[0]]); g.endZ.push_back(Z[q.at[1]]);
            } else if (q.type == BEND) {
               g.centreZ = Z[q.at[1]];
               g.endZ.push_back(Z[q.at[0]]); g.endZ.push_back(Z[q.at[2]]);
               const int c = q.at[1];
               for (size_t t = 0; t < topo.nbr[c].size(); t++)
                  (Z[topo.nbr[c][t]] == 1 ? g.hCount : g.otherCount)++;
               //  CH2/CH3 rock: bends of an H and a non-H terminal about a
               //  centre that carries at least two H.
               const bool hx = (Z[q.at[0]] == 1) != (Z[q.at[2]] == 1);
               bool same = true;
               for (size_t t = s; t < e; t++)
                  if (coords[idx[t]].at[1] != q.at[1]) same = false;
               g.rockCandidate = hx && same && (e - s) >= 2 && g.hCount >= 2 &&
                                 g.otherCount >= 1 && !q.linear;
            } else if (q.type == OOP) {
               g.centreZ = Z[q.at[0]];
               const int c = q.at[0];
               for (size_t t = 0; t < topo.nbr[c].size(); t++)
                  (Z[topo.nbr[c][t]] == 1 ? g.hCount : g.otherCount)++;
            } else {
               g.endZ.push_back(Z[q.at[1]]); g.endZ.push_back(Z[q.at[2]]);
               g.order = bondOrder(topo, q.at[1], q.at[2]);
            }
            groups.push_back(g);
            for (size_t t = s; t < e; t++) coords[idx[t]].group = (int)groups.size() - 1;
            s = e;
         }
      }
   }

   auto centreName = [&](const GroupInfo& g) {
      string s = pt.atomicSymbol(g.centreZ);
      if (g.hCount > 0) {
         s += "H";
         if (g.hCount > 1) { std::ostringstream o; o << g.hCount; s += o.str(); }
      }
      return s;
   };
   auto bondName = [&](const vector<int>& z, double order) {
      const string base = orderedSymbols(z, pt, true);
      if (order > 1.9 && order < 2.1 && z[0] != 1 && z[1] != 1) {
         string a = pt.atomicSymbol(std::min(z[0], z[1])), b = pt.atomicSymbol(std::max(z[0], z[1]));
         return a + "=" + b;
      }
      if (order > 2.9 && order < 3.1 && z[0] != 1 && z[1] != 1) {
         string a = pt.atomicSymbol(std::min(z[0], z[1])), b = pt.atomicSymbol(std::max(z[0], z[1]));
         return a + "\xe2\x89\xa1" + b;
      }
      return base;
   };
   auto spacedBond = [&](const GroupInfo& g) {
      //  Long form: C=O keeps its sign, others get a dash.
      const string b = bondName(g.endZ, g.order);
      if (b.find('=') != string::npos || b.find("\xe2\x89\xa1") != string::npos) return b;
      return pt.atomicSymbol(std::min(g.endZ[0], g.endZ[1]) == 1 ? std::max(g.endZ[0], g.endZ[1]) : std::min(g.endZ[0], g.endZ[1]))
             + string(ENDASH)
             + pt.atomicSymbol(std::min(g.endZ[0], g.endZ[1]) == 1 ? 1 : std::max(g.endZ[0], g.endZ[1]));
   };

   for (size_t m = 0; m < nm; m++) {
      const Vec& d = in.modes[m];
      if (d.size() != 3*N || fabs(in.freqs[m]) < ZERO_FREQ) continue;

      vector<double> S(coords.size()), w(coords.size());
      double total = 0.0;
      for (size_t i = 0; i < coords.size(); i++) {
         S[i] = dot(coords[i].b, d);
         w[i] = coords[i].G > 1e-12 ? S[i]*S[i]/coords[i].G : 0.0;
         total += w[i];
      }
      if (total < 1e-18) continue;
      for (size_t t = 0; t < triples.size(); t++) {
         const double a = S[triples[t].p1], b = S[triples[t].p2];
         if (a*b > 0 && std::min(fabs(a), fabs(b)) >= 0.2*std::max(fabs(a), fabs(b))) {
            w[triples[t].single] += w[triples[t].p1] + w[triples[t].p2];
            w[triples[t].p1] = w[triples[t].p2] = 0.0;
         }
      }

      vector<double> gshare(groups.size(), 0.0);
      for (size_t i = 0; i < coords.size(); i++)
         if (coords[i].group >= 0) gshare[coords[i].group] += w[i]/total;

      //  Phase of a group's members: same sign is in phase.
      auto phase = [&](int g) -> string {
         const GroupInfo& gi = groups[g];
         if (gi.type == OOP || gi.type == TORSION) return "";
         double big = 0.0;
         int linearMembers = 0;
         for (size_t t = 0; t < gi.members.size(); t++) {
            big = std::max(big, fabs(S[gi.members[t]]));
            if (coords[gi.members[t]].linear) linearMembers++;
         }
         if (linearMembers || gi.members.size() < 2 || big <= 0) return "";
         int pos = 0, neg = 0;
         for (size_t t = 0; t < gi.members.size(); t++) {
            const double v = S[gi.members[t]];
            if (fabs(v) < 0.2*big) continue;
            (v > 0 ? pos : neg)++;
         }
         if (pos + neg < 2) return "";
         return (pos == 0 || neg == 0) ? "s" : "as";
      };

      vector<int> idx(groups.size());
      for (size_t g = 0; g < groups.size(); g++) idx[g] = (int)g;
      std::sort(idx.begin(), idx.end(), [&](int a, int b) { return gshare[a] > gshare[b]; });

      auto shortOf = [&](int g) -> string {
         const GroupInfo& gi = groups[g];
         const string ph = phase(g);
         if (gi.type == STRETCH)
            return string(NU) + ph + "(" + bondName(gi.endZ, gi.order) + ")";
         if (gi.type == BEND) {
            if (gi.rockCandidate && ph == "as")
               return string(RHO) + "(" + centreName(gi) + ")";
            vector<int> ends(gi.endZ);
            string t = orderedSymbols(ends, pt, false);
            //  terminal atoms around the centre: H-C-O
            const string a = pt.atomicSymbol(std::min(ends[0], ends[1]) == 1 ? 1 :
                             (heavierThan(ends[0], ends[1]) ? ends[0] : ends[1]));
            const string b = pt.atomicSymbol(std::min(ends[0], ends[1]) == 1 ?
                             std::max(ends[0], ends[1]) :
                             (heavierThan(ends[0], ends[1]) ? ends[1] : ends[0]));
            return string(DELTA) + ph + "(" + a + pt.atomicSymbol(gi.centreZ) + b + ")";
         }
         if (gi.type == OOP) {
            if (gi.hCount == 2) return string(OMEGA) + "(" + centreName(gi) + ")";
            return string(GAMMA) + "(" + centreName(gi) + ")";
         }
         return string(TAU) + "(" + bondName(gi.endZ, gi.order) + ")";
      };
      auto longOf = [&](int g) -> string {
         const GroupInfo& gi = groups[g];
         const string ph = phase(g);
         string kind;
         std::ostringstream pct;
         pct << " " << (int)floor(gshare[g]*100.0 + 0.5) << "%";
         const string phs = ph == "s" ? ", sym." : (ph == "as" ? ", antisym." : "");
         if (gi.type == STRETCH) kind = spacedBond(gi) + " stretch";
         else if (gi.type == BEND) {
            if (gi.rockCandidate && ph == "as") kind = centreName(gi) + " rock";
            else {
               const string a = pt.atomicSymbol(std::min(gi.endZ[0], gi.endZ[1]) == 1 ? 1 :
                                (heavierThan(gi.endZ[0], gi.endZ[1]) ? gi.endZ[0] : gi.endZ[1]));
               const string b = pt.atomicSymbol(std::min(gi.endZ[0], gi.endZ[1]) == 1 ?
                                std::max(gi.endZ[0], gi.endZ[1]) :
                                (heavierThan(gi.endZ[0], gi.endZ[1]) ? gi.endZ[1] : gi.endZ[0]));
               kind = a + ENDASH + pt.atomicSymbol(gi.centreZ) + ENDASH + b + " bend";
            }
         }
         else if (gi.type == OOP) kind = gi.hCount == 2 ? centreName(gi) + " wag" : centreName(gi) + " out-of-plane bend";
         else kind = spacedBond(gi) + " torsion";
         return kind + phs + pct.str();
      };

      const double top = gshare[idx[0]];
      vector<int> significant;
      for (size_t t = 0; t < idx.size(); t++) {
         const double s = gshare[idx[t]];
         if (s >= 0.2 && s >= 0.35*top) significant.push_back(idx[t]);
      }
      if (significant.empty()) significant.push_back(idx[0]);
      string label = shortOf(significant[0]);
      if (significant.size() == 2) label += " + " + shortOf(significant[1]);
      else if (significant.size() > 2) label += " + \xe2\x80\xa6";
      out.modes[m].label = label;

      string detail;
      int shown = 0;
      for (size_t t = 0; t < idx.size() && shown < 4; t++) {
         if (gshare[idx[t]] < 0.05 && shown > 0) break;
         if (!detail.empty()) detail += ", ";
         detail += longOf(idx[t]);
         shown++;
      }
      out.modes[m].detail = detail;
      out.modes[m].parts.clear();
      for (size_t t = 0; t < idx.size() && t < 8; t++) {
         const double s = gshare[idx[t]];
         if (s < 0.05 && t > 0) break;
         string l = longOf(idx[t]);
         const size_t sp = l.rfind(' ');
         out.modes[m].parts.push_back(std::make_pair(l.substr(0, sp), floor(s*100.0 + 0.5)));
      }
   }
}


//  ----------------------------------------------------------------------

//  Mode composition: the kinetic energy distribution of each mode over the
//  chosen internal coordinates (no force constants needed), normalised to
//  100 %.
namespace { vector<int> specKey(const VibCoordSpec& c); }

bool VibAnalysis::composition(const VibInput& in, const vector<VibCoordSpec>& chosen,
                              const vector<int>& centres, VibComposition& out,
                              bool torsions)
{
   out = VibComposition();
   const size_t N = in.elements.size();
   if (N < 2 || in.coords.size() != 3*N) return false;
   TPerTab pt(true);
   vector<int> Z(N);
   vector<double> mass(N);
   for (size_t a = 0; a < N; a++) {
      Z[a] = pt.atomicNumber(in.elements[a]);
      mass[a] = pt.atomicWeight(Z[a]);
      if (Z[a] <= 0 || mass[a] <= 0) return false;
   }
   const Vec& x = in.coords;

   vector<VibCoordSpec> specs = chosen;
   if (specs.empty()) {
      vector<VibBond> bonds = in.bonds;
      if (bonds.empty()) {
         for (size_t a = 0; a < N; a++)
            for (size_t b = a + 1; b < N; b++) {
               double d[3];
               for (int k = 0; k < 3; k++) d[k] = x[3*a+k]-x[3*b+k];
               if (norm3(d) < 1.2*(pt.covalentRadius(Z[a]) + pt.covalentRadius(Z[b]))) {
                  VibBond vb = { (int)a, (int)b, 1.0 };
                  bonds.push_back(vb);
               }
            }
      }
      vector< vector<int> > nbr(N);
      for (size_t k = 0; k < bonds.size(); k++) {
         const int a = bonds[k].a, b = bonds[k].b;
         if (a < 0 || b < 0 || a >= (int)N || b >= (int)N || a == b) continue;
         if (bonds[k].order > 0 && bonds[k].order < 0.9) continue;
         nbr[a].push_back(b); nbr[b].push_back(a);
         VibCoordSpec s = { 2, { std::min(a,b), std::max(a,b), -1, -1 } };
         specs.push_back(s);
      }
      for (size_t c = 0; c < N; c++) {
         bool wanted;
         if (centres.empty()) wanted = Z[c] != 1;
         else wanted = std::find(centres.begin(), centres.end(), (int)c) != centres.end();
         if (!wanted) continue;
         std::sort(nbr[c].begin(), nbr[c].end());
         for (size_t i = 0; i < nbr[c].size(); i++)
            for (size_t k = i + 1; k < nbr[c].size(); k++) {
               //  Bends at carbon that move an H overlap the heavy-atom
               //  bends and stretches and swamp them; give them by hand.
               if (centres.empty() && Z[c] == 6 &&
                   (Z[nbr[c][i]] == 1 || Z[nbr[c][k]] == 1)) continue;
               VibCoordSpec s = { 3, { nbr[c][i], (int)c, nbr[c][k], -1 } };
               specs.push_back(s);
            }
      }
      if (torsions) for (size_t a = 0; a < N; a++) std::sort(nbr[a].begin(), nbr[a].end());
      //  The same torsions describe() uses; the straight-angle check is
      //  made below for every torsion.
      for (size_t j = 0; torsions && j < N; j++)
         for (size_t kk = 0; kk < nbr[j].size(); kk++) {
            const int k = nbr[j][kk];
            if (k < (int)j) continue;
            int bi = -1, bl = -1;
            for (size_t t = 0; t < nbr[j].size(); t++) {
               const int a = nbr[j][t];
               if (a != k && (bi < 0 || Z[a] > Z[bi])) bi = a;
            }
            for (size_t t = 0; t < nbr[k].size(); t++) {
               const int a = nbr[k][t];
               if (a != (int)j && a != bi && (bl < 0 || Z[a] > Z[bl])) bl = a;
            }
            if (bi < 0 || bl < 0) continue;
            VibCoordSpec s = { 4, { bi, (int)j, k, bl } };
            specs.push_back(s);
         }
   }

   vector<Coord> coords;
   std::set< vector<int> > seen;   // a coordinate typed twice counts once
   for (size_t i = 0; i < specs.size(); i++) {
      const VibCoordSpec& s = specs[i];
      if (s.n >= 2 && s.n <= 4 && !seen.insert(specKey(s)).second) continue;
      if (s.n < 2 || s.n > 4) continue;
      bool ok = true;
      for (int t = 0; t < s.n; t++) {
         if (s.at[t] < 0 || s.at[t] >= (int)N) ok = false;
         for (int u = 0; u < t; u++) if (s.at[t] == s.at[u]) ok = false;
      }
      if (!ok) continue;
      Coord q;
      q.type = s.n == 2 ? STRETCH : s.n == 3 ? BEND : TORSION;
      for (int t = 0; t < 4; t++) q.at[t] = t < s.n ? s.at[t] : -1;
      q.n = s.n; q.linear = false; q.order = 0; q.group = -1; q.value = 0;
      //  A straight angle has no direction to bend in, and no dihedral.
      if (q.type == BEND && evalCoord(q, x)*180.0/M_PI > 175.0) continue;
      if (q.type == TORSION) {
         Coord t1 = q, t2 = q;
         t1.type = t2.type = BEND;
         t1.n = t2.n = 3;
         t2.at[0] = q.at[1]; t2.at[1] = q.at[2]; t2.at[2] = q.at[3];
         if (evalCoord(t1, x)*180/M_PI > 175 || evalCoord(t2, x)*180/M_PI > 175) continue;
      }
      numericB(q, x);
      double g = 0.0;
      for (size_t a = 0; a < N; a++)
         for (int t = 0; t < 3; t++) g += q.b[3*a+t]*q.b[3*a+t]/mass[a];
      if (g < 1e-12) continue;
      q.G = g;
      coords.push_back(q);
      out.coords.push_back(s);
      string nm;
      for (int t = 0; t < s.n; t++) {
         if (t) nm += ENDASH;
         std::ostringstream o;
         o << in.elements[s.at[t]] << s.at[t] + 1;
         nm += o.str();
      }
      nm += q.type == STRETCH ? " stretch" : q.type == BEND ? " bend" : " torsion";
      out.names.push_back(nm);
   }
   if (coords.empty()) return false;

   //  G = B M^-1 B^T; the kinetic energy distribution of a mode with internal
   //  displacements s is KED_i = s_i (G^-1 s)_i / (s . G^-1 s).  Unlike the
   //  plain squared cosine it does not double count coordinates that move
   //  together (the C=O stretch and the C-C-N bend of amide I).  G of a
   //  redundant set is singular, so it is damped; s lies in its range.
   const size_t nc = coords.size();
   vector< vector<double> > G(nc, vector<double>(nc, 0.0));
   double diag = 0.0;
   for (size_t i = 0; i < nc; i++)
      for (size_t j = 0; j <= i; j++) {
         double g = 0.0;
         for (size_t a = 0; a < N; a++)
            for (int t = 0; t < 3; t++)
               g += coords[i].b[3*a+t]*coords[j].b[3*a+t]/mass[a];
         G[i][j] = G[j][i] = g;
         if (i == j) diag += g;
      }
   for (size_t i = 0; i < nc; i++) G[i][i] += 1e-3*diag/nc;
   //  Cholesky factor of the damped (positive definite) G.
   vector< vector<double> > Lc(nc, vector<double>(nc, 0.0));
   bool chol = true;
   for (size_t i = 0; i < nc && chol; i++)
      for (size_t j = 0; j <= i; j++) {
         double v = G[i][j];
         for (size_t k = 0; k < j; k++) v -= Lc[i][k]*Lc[j][k];
         if (i == j) { if (v <= 0) { chol = false; break; } Lc[i][i] = sqrt(v); }
         else Lc[i][j] = v/Lc[j][j];
      }
   if (!chol) return false;

   for (size_t m = 0; m < in.modes.size(); m++) {
      vector<double> row(nc, 0.0);
      const Vec& d = in.modes[m];
      if (d.size() == 3*N && m < in.freqs.size() && fabs(in.freqs[m]) >= ZERO_FREQ) {
         vector<double> sv(nc), y(nc);
         for (size_t i = 0; i < nc; i++) sv[i] = dot(coords[i].b, d);
         for (size_t i = 0; i < nc; i++) {          // L z = s
            double v = sv[i];
            for (size_t k = 0; k < i; k++) v -= Lc[i][k]*y[k];
            y[i] = v/Lc[i][i];
         }
         for (size_t i = nc; i-- > 0;) {            // L^T x = z
            double v = y[i];
            for (size_t k = i + 1; k < nc; k++) v -= Lc[k][i]*y[k];
            y[i] = v/Lc[i][i];
         }
         double total = 0.0;
         for (size_t i = 0; i < nc; i++) { row[i] = sv[i]*y[i]; total += row[i]; }
         if (total > 1e-18) {
            //  Negative terms are cancellation between overlapping coordinates.
            double pos = 0.0;
            for (size_t i = 0; i < nc; i++) { if (row[i] < 0) row[i] = 0; pos += row[i]; }
            for (size_t i = 0; i < nc; i++) row[i] *= 100.0/pos;
         } else std::fill(row.begin(), row.end(), 0.0);
      }
      out.percent.push_back(row);
   }
   return true;
}


namespace {
vector<int> specKey(const VibCoordSpec& c)
{
   vector<int> k(c.at, c.at + c.n);
   vector<int> r(k.rbegin(), k.rend());
   if (c.n == 3) { if (k[0] > k[2]) k = r; }
   else if (c.n == 2) std::sort(k.begin(), k.end());
   else if (r < k) k = r;
   k.push_back(c.n);
   return k;
}
}

bool VibAnalysis::selectionShare(const VibInput& in, const vector<int>& atoms,
                                 vector<double>& share, string& label)
{
   share.clear();
   label.clear();
   const size_t N = in.elements.size();
   if (atoms.size() < 2 || N < 2) return false;
   TPerTab pt(true);
   std::set<int> sel(atoms.begin(), atoms.end());
   for (std::set<int>::iterator it = sel.begin(); it != sel.end(); ++it)
      if (*it < 0 || *it >= (int)N) return false;

   vector<VibCoordSpec> group;
   if (sel.size() == 2) {
      VibCoordSpec c = { 2, { *sel.begin(), *sel.rbegin(), -1, -1 } };
      group.push_back(c);
      std::ostringstream o;
      o << in.elements[c.at[0]] << c.at[0] + 1 << ENDASH
        << in.elements[c.at[1]] << c.at[1] + 1 << " stretch";
      label = o.str();
   } else {
      //  Bonds as the automatic set sees them.
      vector<VibBond> bonds = in.bonds;
      if (bonds.empty())
         for (size_t a = 0; a < N; a++)
            for (size_t b = a + 1; b < N; b++) {
               double d[3];
               for (int k = 0; k < 3; k++) d[k] = in.coords[3*a+k]-in.coords[3*b+k];
               if (norm3(d) < 1.2*(pt.covalentRadius(pt.atomicNumber(in.elements[a])) +
                                   pt.covalentRadius(pt.atomicNumber(in.elements[b])))) {
                  VibBond vb = { (int)a, (int)b, 1.0 };
                  bonds.push_back(vb);
               }
            }
      vector< vector<int> > nbr(N);
      for (size_t k = 0; k < bonds.size(); k++) {
         const int a = bonds[k].a, b = bonds[k].b;
         if (a < 0 || b < 0 || a >= (int)N || b >= (int)N || a == b) continue;
         if (bonds[k].order > 0 && bonds[k].order < 0.9) continue;
         if (!sel.count(a) || !sel.count(b)) continue;
         nbr[a].push_back(b); nbr[b].push_back(a);
         VibCoordSpec s = { 2, { std::min(a,b), std::max(a,b), -1, -1 } };
         group.push_back(s);
      }
      for (size_t c = 0; c < N; c++) {
         std::sort(nbr[c].begin(), nbr[c].end());
         for (size_t i = 0; i < nbr[c].size(); i++)
            for (size_t k = i + 1; k < nbr[c].size(); k++) {
               VibCoordSpec s = { 3, { nbr[c][i], (int)c, nbr[c][k], -1 } };
               group.push_back(s);
            }
      }
      for (size_t j = 0; j < N; j++)
         for (size_t t = 0; t < nbr[j].size(); t++) {
            const int k = nbr[j][t];
            if (k < (int)j) continue;
            int a = -1, l = -1;
            for (size_t u = 0; u < nbr[j].size(); u++) if (nbr[j][u] != k) { a = nbr[j][u]; break; }
            for (size_t u = 0; u < nbr[k].size(); u++)
               if (nbr[k][u] != (int)j && nbr[k][u] != a) { l = nbr[k][u]; break; }
            if (a < 0 || l < 0) continue;
            VibCoordSpec s = { 4, { a, (int)j, k, l } };
            group.push_back(s);
         }
      std::ostringstream o;
      o << "selected group (" << sel.size() << " atoms)";
      label = o.str();
   }
   if (group.empty()) return false;

   VibComposition base, all;
   if (!composition(in, vector<VibCoordSpec>(), vector<int>(), base)) return false;
   vector<VibCoordSpec> full = base.coords;
   std::set< vector<int> > have;
   for (size_t i = 0; i < full.size(); i++) have.insert(specKey(full[i]));
   std::set< vector<int> > mine;
   for (size_t i = 0; i < group.size(); i++) {
      mine.insert(specKey(group[i]));
      if (have.insert(specKey(group[i])).second) full.push_back(group[i]);
   }
   if (!composition(in, full, vector<int>(), all)) return false;
   share.assign(all.percent.size(), 0.0);
   for (size_t m = 0; m < all.percent.size(); m++)
      for (size_t i = 0; i < all.coords.size(); i++)
         if (mine.count(specKey(all.coords[i]))) share[m] += all.percent[m][i];
   return true;
}


bool VibAnalysis::analyse(const VibInput& in, VibResult& out)
{
   out = VibResult();
   out.modes.resize(in.modes.size());
   for (size_t m = 0; m < out.modes.size(); m++) {
      out.modes[m].activityKnown = false;
      out.modes[m].ir = out.modes[m].raman = false;
      out.modes[m].rigid = false;
   }

   //  The modes must belong to this geometry before anything is said.
   {
      TPerTab pt(true);
      vector<double> mass;
      for (size_t a = 0; a < in.elements.size(); a++)
         mass.push_back(pt.atomicWeight(pt.atomicNumber(in.elements[a])));
      if (in.coords.size() != 3*in.elements.size() || in.elements.empty()) return false;
      for (size_t a = 0; a < mass.size(); a++) if (mass[a] <= 0) return false;
      if (rigidShare(in, mass) > 0.1) {
         out.note = "the modes are not in the structure's orientation";
         return true;
      }
   }
   describe(in, out);

   Fragment probe;
   const size_t N = in.elements.size();
   for (size_t a = 0; a < N; a++) {
      const double xyz[3] = { in.coords[3*a], in.coords[3*a+1], in.coords[3*a+2] };
      probe.addAtom(in.elements[a], xyz);
   }
   string group;
   try {
      group = SymmetryOps::find(probe, 0.01);
   } catch (...) {
      out.note = "the symmetry search could not be run";
      return true;
   }
   vector<SymOp> ops;
   if (group.empty() || !MoFragments::symmetryOperations(group, ops)) {
      out.note = "no symmetry operations for " + group;
      return true;
   }
   vector<double> stdXyz;
   const double *p = probe.coordinates();
   if (p == 0 || probe.numAtoms() != N) { out.note = "symmetry search changed the atoms"; return true; }
   stdXyz.assign(p, p + 3*N);
   //  A group the search names in any capitalisation.
   VibResult sym;
   sym.modes.resize(in.modes.size());
   if (symmetry(in, group, stdXyz, ops, sym)) {
      out.group = group;
      for (size_t m = 0; m < out.modes.size(); m++) {
         out.modes[m].irrep = sym.modes[m].irrep;
         out.modes[m].activityKnown = sym.modes[m].activityKnown;
         out.modes[m].ir = sym.modes[m].ir;
         out.modes[m].raman = sym.modes[m].raman;
         if (sym.modes[m].rigid) {
            out.modes[m].rigid = true;
            out.modes[m].label = sym.modes[m].label;
            out.modes[m].detail = sym.modes[m].detail;
            out.modes[m].remark = sym.modes[m].remark;
         }
      }
   } else {
      out.note = sym.note;
   }
   return true;
}


bool VibAnalysis::input(Fragment& frag, const vector<double>& freqs,
                        const vector< vector<double> >& modes, VibInput& in)
{
   const size_t N = frag.numAtoms();
   const double *xyz = frag.coordinates();
   if (N == 0 || xyz == 0) return false;
   for (size_t a = 0; a < N; a++) {
      in.elements.push_back(frag.atomRef((int)a)->atomicSymbol());
      for (int k = 0; k < 3; k++) in.coords.push_back(xyz[3*a+k]);
   }
   vector<TBond*> *bonds = frag.bonds();
   if (bonds != 0) {
      for (size_t i = 0; i < bonds->size(); i++) {
         VibBond vb = { (*bonds)[i]->atom1()->index(), (*bonds)[i]->atom2()->index(),
                        (*bonds)[i]->order() };
         in.bonds.push_back(vb);
      }
   }
   in.freqs = freqs;
   in.modes = modes;
   return true;
}


bool VibAnalysis::analyse(Fragment& frag, const vector<double>& freqs,
                          const vector< vector<double> >& modes, VibResult& out)
{
   VibInput in;
   return input(frag, freqs, modes, in) && analyse(in, out);
}

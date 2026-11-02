#include <cmath>
#include <set>

#include "tdat/BondDistort.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"

using std::vector;
using std::string;

namespace {

bool vec(const Fragment& f, int a, int b, double v[3], double& len)
{
  const double *ca = f.atomRef(a)->coordinates();
  const double *cb = f.atomRef(b)->coordinates();
  for (int k = 0; k < 3; k++) v[k] = cb[k] - ca[k];
  len = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
  return len > 1e-12;
}

}

vector<int> BondDistort::neighbours(const Fragment& frag, int centre)
{
  vector<int> out;
  if (centre < 0 || centre >= (int)frag.numAtoms()) return out;
  TAtm *c = frag.atomRef(centre);
  for (TBond *b : c->bondList()) {
    TAtm *o = b->atom1() == c ? b->atom2() : b->atom1();
    out.push_back(o->index());
  }
  return out;
}

bool BondDistort::apply(Fragment& frag, int centre, const vector<int>& nb,
                        const vector<double>& dr)
{
  int n = (int)frag.numAtoms();
  if (centre < 0 || centre >= n || nb.size() != dr.size()) return false;
  vector<vector<double> > shift;
  vector<vector<int> > group;
  std::set<int> nbset(nb.begin(), nb.end());
  for (size_t i = 0; i < nb.size(); i++) {
    if (nb[i] < 0 || nb[i] >= n || nb[i] == centre) return false;
    double v[3], len;
    if (!vec(frag, centre, nb[i], v, len)) return false;
    shift.push_back(vector<double>{dr[i]*v[0]/len, dr[i]*v[1]/len,
                                   dr[i]*v[2]/len});
    // The branch beyond the neighbour, not passing the centre.
    std::set<int> seen{centre, nb[i]};
    vector<int> todo{nb[i]}, br{nb[i]};
    bool loops = false;
    while (!todo.empty() && !loops) {
      int a = todo.back(); todo.pop_back();
      for (int o : neighbours(frag, a)) {
        if (seen.count(o)) continue;
        if (nbset.count(o)) { loops = true; break; }
        seen.insert(o); todo.push_back(o); br.push_back(o);
      }
    }
    if (loops) br.assign(1, nb[i]);
    group.push_back(br);
  }
  // All vectors were taken before anything moved.
  for (size_t i = 0; i < nb.size(); i++)
    for (int a : group[i]) {
      const double *c = frag.atomRef(a)->coordinates();
      frag.atomRef(a)->coordinates(c[0] + shift[i][0], c[1] + shift[i][1],
                                   c[2] + shift[i][2]);
    }
  return true;
}

vector<double> BondDistort::uniform(size_t n, double dr)
{
  return vector<double>(n, dr);
}

bool BondDistort::jahnTeller(const Fragment& frag, int centre,
                             const vector<int>& nb, double delta,
                             const string& axis, vector<double>& dr)
{
  if (nb.size() != 6) return false;
  int ax = axis == "x" ? 0 : axis == "y" ? 1 : axis == "z" ? 2 : -1;
  if (ax < 0 && axis != "longest") return false;
  // Trans pairs: the most nearly antiparallel partner of each neighbour.
  double u[6][3], len[6];
  for (int i = 0; i < 6; i++) {
    if (!vec(frag, centre, nb[i], u[i], len[i])) return false;
    for (int k = 0; k < 3; k++) u[i][k] /= len[i];
  }
  int partner[6];
  for (int i = 0; i < 6; i++) {
    double best = 2.0;
    partner[i] = -1;
    for (int j = 0; j < 6; j++) {
      if (j == i) continue;
      double d = 0;
      for (int k = 0; k < 3; k++) d += u[i][k]*u[j][k];
      if (d < best) { best = d; partner[i] = j; }
    }
  }
  int pick = -1;
  double score = -1;
  for (int i = 0; i < 6; i++) {
    if (partner[i] < i) continue;  // each pair once
    double s = ax >= 0 ? std::fabs(u[i][ax]) : len[i] + len[partner[i]];
    if (s > score + 1e-9) { score = s; pick = i; }
  }
  if (pick < 0) return false;
  dr.assign(6, -0.5 * delta);
  dr[pick] = dr[partner[pick]] = delta;
  return true;
}

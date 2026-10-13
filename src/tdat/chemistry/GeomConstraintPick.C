#include <algorithm>
#include <cmath>
#include <cstdio>

#include "tdat/Fragment.H"
#include "tdat/GeomConstraintPick.H"
#include "tdat/TAtm.H"
#include "tdat/TBond.H"

using std::string;
using std::vector;

std::string constraintAtomName(const Fragment& frag, int index)
{
  char buf[16];
  snprintf(buf, sizeof buf, "%d", index + 1);
  return frag.atomRef(index)->atomicSymbol() + buf;
}

static bool bonded(const Fragment& frag, int a, int b)
{
  const TAtm* atm = frag.atomRef(a);
  const TAtm* other = frag.atomRef(b);
  const vector<TBond*>& bonds = atm->bondList();
  for (size_t i = 0; i < bonds.size(); i++) {
    if (bonds[i]->atom1() == other || bonds[i]->atom2() == other) return true;
  }
  return false;
}

static string join(const Fragment& frag, const vector<int>& atoms)
{
  string s;
  for (size_t i = 0; i < atoms.size(); i++) {
    if (i) s += "\xE2\x80\x93";  // en dash
    s += constraintAtomName(frag, atoms[i]);
  }
  return s;
}

GeomConstraintPick orderConstraintAtoms(const Fragment& frag,
                                        const vector<int>& picked)
{
  GeomConstraintPick r;
  r.atoms = picked;
  const size_t n = picked.size();
  if (n < 2 || n > 4) return r;

  // The chain a-b-c(-d) must be bonded link by link.
  vector<int> chain = picked;
  std::sort(chain.begin(), chain.end());
  vector<vector<int> > valid;   // chains up to reversal, in permutation order
  do {
    if (chain.front() > chain.back()) continue;  // skip the mirror image
    bool ok = true;
    for (size_t i = 0; ok && i + 1 < n; i++)
      ok = bonded(frag, chain[i], chain[i + 1]);
    if (ok) valid.push_back(chain);
  } while (std::next_permutation(chain.begin(), chain.end()));

  const char* kind = n == 2 ? "Bond" : n == 3 ? "Angle" : "Torsion";
  vector<int> rev(picked.rbegin(), picked.rend());

  // The pick order wins when it is already a bonded chain, either way round.
  bool pickOk = true;
  for (size_t i = 0; pickOk && i + 1 < n; i++)
    pickOk = bonded(frag, picked[i], picked[i + 1]);

  if (!pickOk && valid.size() == 1) {
    // Keep the end the student picked first at the front.
    vector<int> c = valid[0];
    size_t pos0 = std::find(c.begin(), c.end(), picked[0]) - c.begin();
    if (pos0 > n / 2) std::reverse(c.begin(), c.end());
    r.atoms = c;
    r.reordered = true;
  } else if (!pickOk) {
    r.needsConfirm = true;
    string names = join(frag, picked);
    if (n == 2)
      r.warning = names + " are not bonded.";
    else if (valid.size() > 1)
      r.warning = names + " can be bonded in more than one order; the order picked is used.";
    else if (n == 3)
      r.warning = names + " do not form an angle: the centre atom (second) must be bonded to both others.";
    else
      r.warning = names + " do not form a torsion: each atom must be bonded to the next.";
  }

  r.description = string(kind) + " " + join(frag, r.atoms);
  if (n == 3)
    r.description += " (centre " + constraintAtomName(frag, r.atoms[1]) + ")";
  return r;
}


std::string dummyAtomPosition(const std::vector<std::vector<double> >& xyz,
                              DummyPlacement how, double distance,
                              double out[3])
{
  const size_t n = xyz.size();
  if (n < 2) return "Pick at least two atoms.";
  double c[3] = {0, 0, 0};
  const size_t from = (how == DUMMY_CENTRE) ? 0 : 1;
  for (size_t i = from; i < n; i++)
    for (int k = 0; k < 3; k++) c[k] += xyz[i][k];
  for (int k = 0; k < 3; k++) c[k] /= double(n - from);
  if (how == DUMMY_CENTRE) {
    for (int k = 0; k < 3; k++) out[k] = c[k];
    return "";
  }
  double v[3], len2 = 0;
  for (int k = 0; k < 3; k++) {
    v[k] = xyz[0][k] - c[k];
    len2 += v[k] * v[k];
  }
  if (len2 < 1e-12)
    return "The first atom lies at the centre of the others, so there is no "
           "axis; pick another atom first.";
  const double s = (how == DUMMY_BEYOND ? distance : -distance) / sqrt(len2);
  for (int k = 0; k < 3; k++) out[k] = xyz[0][k] + s * v[k];
  return "";
}

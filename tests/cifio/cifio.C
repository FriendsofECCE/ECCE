// CIF reader and writer (#358): atom counts from Z x formula, cell, warnings,
// export -> import round trip, malformed input.
// Usage: cifio <fixture dir>
//        cifio --convert in.cif out.cif   (read, write as P1; for the oracle test)
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "util/Ecce.H"
#include "tdat/CifIO.H"
#include "tdat/Fragment.H"
#include "tdat/LatticeDef.H"
#include "tdat/TAtm.H"
#include "util/Transformations.H"

using namespace std;

static int failures = 0;
static void check(bool ok, const string& what)
{
  cout << (ok ? "ok   " : "FAIL ") << what << endl;
  if (!ok) failures++;
}

typedef vector<pair<string, vector<double> > > Atoms;   // element, fractional

// Fractions by solving the 3x3 system with the cell vectors as rows.
static Atoms fractions(Fragment& f)
{
  vector<MPoint> *v = f.getLattice()->toVectors();
  double m[3][3] = {{(*v)[0].x(), (*v)[0].y(), (*v)[0].z()},
                    {(*v)[1].x(), (*v)[1].y(), (*v)[1].z()},
                    {(*v)[2].x(), (*v)[2].y(), (*v)[2].z()}};
  delete v;
  double det = m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1])
             - m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0])
             + m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
  Atoms out;
  for (int i = 0; i < f.numAtoms(); i++) {
    TAtm *a = f.atomRef(i);
    if (a->atomicSymbol() == "Nub") continue;
    const double *x = a->coordinates();
    // x = f0*m0 + f1*m1 + f2*m2  (Cramer's rule on the columns)
    double r[3];
    for (int k = 0; k < 3; k++) {
      double t[3][3];
      for (int i2 = 0; i2 < 3; i2++)
        for (int j = 0; j < 3; j++) t[i2][j] = (j == k) ? x[i2] : m[j][i2];
      r[k] = (t[0][0]*(t[1][1]*t[2][2]-t[1][2]*t[2][1])
            - t[0][1]*(t[1][0]*t[2][2]-t[1][2]*t[2][0])
            + t[0][2]*(t[1][0]*t[2][1]-t[1][1]*t[2][0])) / det;
      r[k] -= floor(r[k] + 1e-9);
    }
    out.push_back(make_pair(a->atomicSymbol(),
                  vector<double>(r, r + 3)));
  }
  return out;
}

static bool less3(const pair<string, vector<double> >& a,
                  const pair<string, vector<double> >& b)
{
  if (a.first != b.first) return a.first < b.first;
  for (int k = 0; k < 3; k++) {
    if (fabs(a.second[k] - b.second[k]) > 1e-4) return a.second[k] < b.second[k];
  }
  return false;
}

static double periodic(double d) { return fabs(d - floor(d + 0.5)); }

static bool sameAtoms(Atoms a, Atoms b, double tol)
{
  if (a.size() != b.size()) return false;
  sort(a.begin(), a.end(), less3);
  sort(b.begin(), b.end(), less3);
  for (size_t i = 0; i < a.size(); i++) {
    if (a[i].first != b[i].first) return false;
    for (int k = 0; k < 3; k++)
      if (periodic(a[i].second[k] - b[i].second[k]) > tol) return false;
  }
  return true;
}

static int count(Fragment& f, const string& el)
{
  int n = 0;
  for (int i = 0; i < f.numAtoms(); i++)
    if (f.atomRef(i)->atomicSymbol() == el) n++;
  return n;
}

struct Case { const char *name; int atoms; double a, b, c; };

int main(int argc, char **argv)
{
  Ecce::initialize();
  if (argc == 4 && string(argv[1]) == "--convert") {
    ifstream in(argv[2]);
    Fragment f;
    if (!CifIO::read(f, in, false)) { cerr << CifIO::lastError() << endl; return 2; }
    ofstream out(argv[3]);
    return f.dumpCIF(out) ? 0 : 3;
  }
  string dir = argc > 1 ? argv[1] : ".";

  // Z x formula: NaCl 4x2, CsCl 1x2, quartz 3x3, graphite 4, LiCoO2 3x4.
  static const Case cases[] = {
    {"nacl", 8, 5.6402, 5.6402, 5.6402}, {"cscl", 2, 4.123, 4.123, 4.123},
    {"quartz", 9, 4.9134, 4.9134, 5.4052}, {"graphite", 4, 2.4612, 2.4612, 6.7079},
    {"licoo2", 12, 2.8156, 2.8156, 14.0542}, {"nacl_nosymops", 8, 5.6402, 5.6402, 5.6402},
    {"nacl_partial", 8, 5.6402, 5.6402, 5.6402}, {"quartz_esd", 9, 4.9134, 4.9134, 5.4052}};
  for (const Case& c : cases) {
    string name = c.name;
    ifstream in((dir + "/" + name + ".cif").c_str());
    Fragment f;
    bool ok = in.good() && CifIO::read(f, in, true);
    vector<string> warn = CifIO::takeWarnings();
    if (!ok && name == "nacl_nosymops" &&
        CifIO::lastError().find("symmetry helper") != string::npos) {
      cout << "SKIP " << name << ": " << CifIO::lastError() << endl;
      continue;
    }
    check(ok, name + ": read" + (ok ? "" : " (" + CifIO::lastError() + ")"));
    if (!ok) continue;
    check(f.numAtoms() == c.atoms, name + ": " + to_string(c.atoms) + " atoms (got "
          + to_string(f.numAtoms()) + ")");
    double a, b, cc, al, be, ga;
    f.getLattice()->getLattice(a, b, cc, al, be, ga);
    check(fabs(a - c.a) < 1e-6 && fabs(b - c.b) < 1e-6 && fabs(cc - c.c) < 1e-6,
          name + ": cell lengths");
    if (name == "nacl_partial") {
      check(count(f, "Na") == 4 && count(f, "K") == 0, name + ": majority species kept");
      check(warn.size() == 1 && warn[0].find("Partially") != string::npos &&
            warn[0].find("Na1") != string::npos, name + ": partial-occupancy warning");
    }
    if (name == "quartz" || name == "quartz_esd")
      check(warn.size() == 1 && warn[0].find("no hydrogen") != string::npos,
            name + ": no-hydrogen warning");
    if (name == "nacl") check(warn.empty(), name + ": no warnings");

    // Export, import again: same atoms and cell.
    ostringstream os;
    check(f.dumpCIF(os), name + ": export");
    istringstream is(os.str());
    Fragment g;
    bool ok2 = CifIO::read(g, is, false);
    CifIO::takeWarnings();
    check(ok2 && g.numAtoms() == f.numAtoms() &&
          sameAtoms(fractions(f), fractions(g), 1e-5), name + ": round trip");
    if (ok2) {
      double a2, b2, c2, al2, be2, ga2;
      g.getLattice()->getLattice(a2, b2, c2, al2, be2, ga2);
      check(fabs(a - a2) + fabs(b - b2) + fabs(cc - c2) + fabs(al - al2) +
            fabs(be - be2) + fabs(ga - ga2) < 1e-4, name + ": round-trip cell");
    }
  }

  // Multiple data blocks: the first is read, with a warning.
  {
    ifstream in((dir + "/nacl.cif").c_str());
    stringstream text;
    text << in.rdbuf();
    string two = text.str() + "\ndata_second\n_cell_length_a 1\n";
    istringstream is(two);
    Fragment f;
    bool ok = CifIO::read(f, is, false);
    vector<string> w = CifIO::takeWarnings();
    check(ok && f.numAtoms() == 8 && !w.empty() && w[0].find("data blocks") != string::npos,
          "two data blocks: first read, warning");
  }

  // Malformed files: a message, no crash.
  const char *bad[] = {"this is not a CIF\n", "data_x\n_cell_length_a 5\n",
    "data_x\n_cell_length_a 5\n_cell_length_b 5\n_cell_length_c 5\n"
    "_cell_angle_alpha 90\n_cell_angle_beta 90\n_cell_angle_gamma 90\n"
    "loop_\n_atom_site_label\n_atom_site_fract_x\n_atom_site_fract_y\n"
    "_atom_site_fract_z\nNa1 0 0\n",
    "data_x\n_cell_length_a 5\n_cell_length_b 5\n_cell_length_c 5\n"
    "_cell_angle_alpha 90\n_cell_angle_beta 90\n_cell_angle_gamma 90\n"
    "_symmetry_space_group_name_H-M 'Q 9 9 9'\nloop_\n_atom_site_label\n"
    "_atom_site_fract_x\n_atom_site_fract_y\n_atom_site_fract_z\nNa1 0 0 0\n"};
  for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
    istringstream is(bad[i]);
    Fragment f;
    bool ok = CifIO::read(f, is, false);
    check(!ok && !CifIO::lastError().empty(), "malformed input " + to_string(i) +
          ": " + CifIO::lastError());
  }
  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}

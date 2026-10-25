// lineprofile: orbitals and electron density along a line through two atoms,
// or averaged over spheres about one atom, from a calculation folder without
// a GUI.  The same LineProfile and RadialDist the Builder's Density Profile
// panel uses.
//
//   lineprofile CALC --atoms A B [--margin M]     along the line A-B
//        [--shift D [--in-plane]]    the line moved sideways by D Angstrom
//   lineprofile CALC --around ATOM [--rmax R]     spherical average about ATOM
//        [--quantity density|psi2|psi|radial|avg]
//        [--show all|totalselected|selected|total]
//        [--mo 1,2,3|occ|homo] [--combine] [--spin alpha|beta|both]
//        [--bohr] [--points N] [--csv FILE]
//
// Atoms are numbered from 1 as the panels label them.  CALC is a
// calculation folder in local data mode.  --show all (the default for the
// density-like quantities) draws the total and each occupied orbital's
// occupation-weighted contribution and prints how well they sum to the total;
// selected draws the --mo orbitals unweighted.  Quantities: along a line
// density (default), psi2, psi; around an atom radial (default), avg.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "util/EcceURL.H"
#include "tdat/LineProfile.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"

using namespace std;

static void usage()
{
  cerr << "usage: lineprofile CALC --atoms A B [--margin M] [--shift D [--in-plane]]\n"
          "       lineprofile CALC --around ATOM [--rmax R]\n"
          "          [--quantity density|psi2|psi|radial|avg]\n"
          "          [--show all|totalselected|selected|total]\n"
          "          [--mo 1,2,3|occ|homo] [--combine] [--spin alpha|beta|both]\n"
          "          [--bohr] [--points N] [--csv FILE]\n";
}

static string absolute(const string& path)
{
  if (!path.empty() && path[0] == '/') return path;
  char buf[4096];
  if (!getcwd(buf, sizeof buf)) return path;
  return string(buf) + "/" + path;
}

int main(int argc, char **argv)
{
  string target, mos, csvFile, quantity, show = "all", spinName = "both";
  int atomA = -1, atomB = -1, around = -1;
  double margin = 2.0, rmax = 0.0, shift = 0.0;
  bool inPlane = false;
  int points = 0;
  bool bohr = false, combine = false;

  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a == "--atoms" && i+2 < argc) { atomA = atoi(argv[i+1]) - 1; atomB = atoi(argv[i+2]) - 1; i += 2; }
    else if (a == "--around" && i+1 < argc) around = atoi(argv[++i]) - 1;
    else if (a == "--margin" && i+1 < argc) margin = atof(argv[++i]);
    else if (a == "--shift" && i+1 < argc) shift = atof(argv[++i]);
    else if (a == "--in-plane") inPlane = true;
    else if (a == "--rmax" && i+1 < argc) rmax = atof(argv[++i]);
    else if (a == "--points" && i+1 < argc) points = atoi(argv[++i]);
    else if (a == "--quantity" && i+1 < argc) quantity = argv[++i];
    else if (a == "--show" && i+1 < argc) show = argv[++i];
    else if (a == "--mo" && i+1 < argc) mos = argv[++i];
    else if (a == "--spin" && i+1 < argc) spinName = argv[++i];
    else if (a == "--combine") combine = true;
    else if (a == "--bohr") bohr = true;
    else if (a == "--csv" && i+1 < argc) csvFile = argv[++i];
    else if (a[0] != '-' && target.empty()) target = a;
    else { usage(); return 2; }
  }
  const bool radialMode = around >= 0;
  if (target.empty() || (!radialMode && (atomA < 0 || atomB < 0))) { usage(); return 2; }

  string url = target.find("://") != string::npos
                   ? target : "file://" + absolute(target);
  EDSIServerCentral central;
  Resource *res = EDSIFactory::getResource(EcceURL(url));
  IPropCalculation *calc = dynamic_cast<IPropCalculation*>(res);
  if (!calc) { cerr << "lineprofile: " << target << " is not a calculation\n"; return 1; }
  DensityPlane::Evaluator ev;
  if (!ev.build(calc)) { cerr << "lineprofile: " << ev.complaint << endl; return 1; }
  const int natoms = (int)ev.symbols.size();
  if ((radialMode && around >= natoms) || (!radialMode && (atomA >= natoms || atomB >= natoms))) {
    cerr << "lineprofile: no such atom\n";
    return 1;
  }

  if (quantity.empty()) quantity = radialMode ? "radial" : "density";
  const bool densityLike = quantity == "density" || quantity == "radial" || quantity == "avg";
  if (radialMode ? !(quantity == "radial" || quantity == "avg")
                 : !(quantity == "density" || quantity == "psi2" || quantity == "psi")) {
    cerr << "lineprofile: quantity " << quantity << " does not apply to this mode\n";
    return 2;
  }
  const RadialDist::Spin spin = spinName == "alpha" ? RadialDist::ALPHA
      : spinName == "beta" ? RadialDist::BETA : RadialDist::BOTH;

  vector<int> orbitals;
  const vector<double>& occ = ev.occ;
  if (show == "all" && densityLike) {
    for (size_t m = 0; m < occ.size(); m++)
      if (occ[m] > 0.0 || (ev.unrestricted && m < ev.occBeta.size() && ev.occBeta[m] > 0.0))
        orbitals.push_back((int)m + 1);
  } else if (mos == "occ") {
    for (size_t m = 0; m < occ.size(); m++) if (occ[m] > 0.0) orbitals.push_back((int)m + 1);
  } else if (mos == "homo" || mos.empty()) {
    orbitals.push_back(ev.homo());
  } else {
    stringstream ss(mos);
    string item;
    while (getline(ss, item, ',')) orbitals.push_back(atoi(item.c_str()));
  }
  const bool weighted = densityLike && show != "selected";
  const vector<string> labels = RadialDist::shellLabels(calc, ev, false);
  vector<RadialDist::Spec> specs;
  if (show != "total" || !densityLike)
    specs = RadialDist::orbitalSpecs(ev, orbitals, spin, weighted,
                                     combine && quantity != "psi", labels);
  if (densityLike && show != "selected") {
    RadialDist::Spec t;
    t.label = "Total density";
    specs.push_back(t);
  }

  vector<RadialDist::Curve> curves;
  vector<double> axis;
  if (radialMode) {
    RadialDist::Options opt;
    opt.bohr = bohr;
    opt.radial = quantity == "radial";
    opt.rMax = rmax > 0 ? rmax : (bohr ? 6.0 / 0.52917721092 : 6.0);
    if (points > 0) opt.points = points | 1;
    const double c[3] = { ev.coordsAng[3*around], ev.coordsAng[3*around+1], ev.coordsAng[3*around+2] };
    RadialDist::compute(ev, c, opt, specs, curves);
    axis = RadialDist::grid(opt);
    for (const RadialDist::Curve& k : curves) {
      printf("%s: integral %.6f", k.spec.label.c_str(), k.integral);
      for (size_t m = 0; m < k.maxR.size(); m++)
        printf("  max %.4f %s (%.5g)", k.maxR[m], bohr ? "bohr" : "A", k.maxP[m]);
      printf("\n");
    }
    if (csvFile.size()) {
      FILE *f = fopen(csvFile.c_str(), "w");
      if (!f) { cerr << "lineprofile: cannot write " << csvFile << endl; return 1; }
      fputs(RadialDist::csv(axis, curves, bohr).c_str(), f);
      fclose(f);
    }
  } else {
    const LineProfile::Line line = LineProfile::throughAtoms(ev, atomA, atomB, margin, shift, !inPlane);
    LineProfile::Result r;
    const LineProfile::Quantity q = quantity == "psi" ? LineProfile::PSI
        : quantity == "psi2" ? LineProfile::PSI2 : LineProfile::DENSITY;
    LineProfile::compute(ev, line, points > 0 ? points : 801, bohr, q, specs, r);
    curves = r.curves;
    for (const LineProfile::Mark& m : r.marks) printf("atom %s at s = %.4f\n", m.label.c_str(), m.s);
    for (const RadialDist::Curve& k : curves) {
      double top = 0, at = 0;
      for (size_t i = 0; i < k.p.size(); i++) if (fabs(k.p[i]) > fabs(top)) { top = k.p[i]; at = r.s[i]; }
      printf("%s: extreme %.6g at s = %.4f\n", k.spec.label.c_str(), top, at);
    }
    if (csvFile.size()) {
      FILE *f = fopen(csvFile.c_str(), "w");
      if (!f) { cerr << "lineprofile: cannot write " << csvFile << endl; return 1; }
      fputs(LineProfile::csv(r, bohr).c_str(), f);
      fclose(f);
    }
  }
  if (weighted && show == "all") {
    const double err = RadialDist::maxSumError(curves);
    double top = 0;
    for (const RadialDist::Curve& k : curves)
      if (k.spec.total) for (double v : k.p) top = max(top, v);
    printf("contributions sum to the total: max deviation %.3e (total peak %.5g)\n", err, top);
  }
  return 0;
}

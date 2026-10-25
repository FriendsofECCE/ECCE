// radialdist: radial distributions of orbitals and of the electron density
// about an atom or a point, from a calculation folder without a GUI.  The
// same RadialDist the Builder's Radial Distribution panel uses.
//
//   radialdist CALC [--centre ATOM | --point X Y Z] [--mo 1,2,3|occ|homo]
//              [--beta] [--total] [--bohr] [--rmax R] [--lebedev N]
//              [--csv FILE]
//
// ATOM is numbered from 1 as the panels label atoms; without a centre the
// only atom, else the atom nearest the centre of mass.  CALC is a
// calculation folder in local data mode.  Prints, per curve, the radii of
// its maxima and its integral (the orbital's population about the centre,
// or the electron count).
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "util/EcceURL.H"
#include "tdat/RadialDist.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"

using namespace std;

static void usage()
{
  cerr << "usage: radialdist CALC [--centre ATOM | --point X Y Z]\n"
          "          [--mo 1,2,3|occ|homo] [--beta] [--total] [--bohr]\n"
          "          [--rmax R] [--lebedev N] [--csv FILE]\n";
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
  string target, mos, csvFile;
  int centreAtom = -1;
  double point[3] = { 0, 0, 0 };
  bool havePoint = false, beta = false, total = false;
  RadialDist::Options opt;

  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    auto num = [&](double& d) { if (i+1 >= argc) return false; d = atof(argv[++i]); return true; };
    if (a == "--mo" && i+1 < argc) mos = argv[++i];
    else if (a == "--centre" && i+1 < argc) centreAtom = atoi(argv[++i]) - 1;
    else if (a == "--point") {
      for (int n = 0; n < 3; n++) if (!num(point[n])) { usage(); return 2; }
      havePoint = true;
    }
    else if (a == "--beta") beta = true;
    else if (a == "--total") total = true;
    else if (a == "--bohr") opt.bohr = true;
    else if (a == "--rmax") { if (!num(opt.rMax)) { usage(); return 2; } }
    else if (a == "--lebedev") { double d; if (!num(d)) { usage(); return 2; } opt.lebedev = (int)d; }
    else if (a == "--csv" && i+1 < argc) csvFile = argv[++i];
    else if (a[0] != '-' && target.empty()) target = a;
    else { usage(); return 2; }
  }
  if (target.empty()) { usage(); return 2; }
  if (opt.bohr && opt.rMax == 6.0) opt.rMax = 6.0 / 0.52917721092;

  string url = target.find("://") != string::npos
                   ? target : "file://" + absolute(target);
  EDSIServerCentral central;
  Resource *res = EDSIFactory::getResource(EcceURL(url));
  IPropCalculation *calc = dynamic_cast<IPropCalculation*>(res);
  if (!calc) { cerr << "radialdist: " << target << " is not a calculation\n"; return 1; }

  DensityPlane::Evaluator ev;
  if (!ev.build(calc)) { cerr << "radialdist: " << ev.complaint << endl; return 1; }

  double centre[3];
  if (havePoint) {
    for (int d = 0; d < 3; d++) centre[d] = point[d];
  } else {
    if (centreAtom < 0) centreAtom = RadialDist::defaultCentre(ev);
    if (centreAtom >= (int)ev.symbols.size()) {
      cerr << "radialdist: no atom " << centreAtom + 1 << endl;
      return 1;
    }
    for (int d = 0; d < 3; d++) centre[d] = ev.coordsAng[3*centreAtom + d];
  }

  vector<int> orbitals;
  const vector<double>& occ = beta ? ev.occBeta : ev.occ;
  if (mos == "occ") {
    for (size_t m = 0; m < occ.size(); m++) if (occ[m] > 0.0) orbitals.push_back((int)m + 1);
  } else if (mos == "homo") {
    orbitals.push_back(ev.homo());
  } else if (!mos.empty()) {
    stringstream ss(mos);
    string item;
    while (getline(ss, item, ',')) orbitals.push_back(atoi(item.c_str()));
  }
  vector<RadialDist::Spec> specs;
  for (int m : orbitals) {
    if (m < 1 || m > ev.orbitals(beta)) {
      cerr << "radialdist: no orbital " << m << " (1.." << ev.orbitals(beta) << ")\n";
      return 1;
    }
    RadialDist::Spec s;
    s.total = false;
    s.mo = m;
    s.beta = beta;
    s.label = "MO " + to_string(m) + (beta ? " beta" : "");
    specs.push_back(s);
  }
  if (total || specs.empty()) {
    RadialDist::Spec s;
    s.label = "Total density";
    specs.push_back(s);
  }

  vector<RadialDist::Curve> curves;
  RadialDist::compute(ev, centre, opt, specs, curves);
  const vector<double> r = RadialDist::grid(opt);
  const char *unit = opt.bohr ? "bohr" : "A";
  for (const RadialDist::Curve& c : curves) {
    printf("%s: integral %.6f", c.spec.label.c_str(), c.integral);
    for (size_t k = 0; k < c.maxR.size(); k++)
      printf("  max %.4f %s (%.5g)", c.maxR[k], unit, c.maxP[k]);
    printf("\n");
  }
  if (!csvFile.empty()) {
    FILE *f = fopen(csvFile.c_str(), "w");
    if (!f) { cerr << "radialdist: cannot write " << csvFile << endl; return 1; }
    const string text = RadialDist::csv(r, curves, opt.bohr);
    fputs(text.c_str(), f);
    fclose(f);
  }
  return 0;
}

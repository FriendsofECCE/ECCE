// densityplane: the electron density or one MO on a plane through a
// molecule, or at points, from a calculation folder without a GUI (#215).
// The same DensityPlane the Builder's Density Map panel uses.
//
//   densityplane CALC [--mo N|homo|lumo [--beta]] [--plane molecular]
//                [--plane atoms A B C] [--plane axial A B]
//                [--step A] [--margin A] [--csv FILE] [--levels]
//   densityplane CALC --point X Y Z [--mo N]          (Angstrom, stored frame)
//   densityplane CALC --points FILE [--mo N]          (x y z lines; values out)
//   densityplane CALC --line X0 Y0 Z0 X1 Y1 Z1 N [--mo N]
//
// Atoms are numbered from 1 as the panel labels them.  CALC is a calculation
// folder in local data mode.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

#include "util/EcceURL.H"
#include "tdat/DensityPlane.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"

using namespace std;
using namespace DensityPlane;

static void usage()
{
  cerr << "usage: densityplane CALC [--mo N|homo|lumo] [--beta]\n"
          "          [--plane molecular | --plane atoms A B C | --plane axial A B]\n"
          "          [--step A] [--margin A] [--csv FILE] [--levels]\n"
          "          [--offset A] [--sum] [--view RX RY RZ UX UY UZ]\n"
          "       densityplane CALC --point X Y Z [--mo N]\n"
          "       densityplane CALC --line X0 Y0 Z0 X1 Y1 Z1 N [--mo N]\n";
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
  string target, mo, csvFile, pointsFile;
  bool beta = false, levels = false;
  PlaneKind kind = MOLECULAR;
  int atoms[3] = { -1, -1, -1 };
  double step = 0.05, margin = 1.5, offset = 0.0;
  bool sum = false, viewPlane = false;
  double viewAxes[6] = { 1, 0, 0, 0, 1, 0 };
  vector<double> pts;     // x y z ... (point or line ends)
  int lineN = 0;

  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    auto num = [&](double& d) { if (i+1 >= argc) return false; d = atof(argv[++i]); return true; };
    if (a == "--mo" && i+1 < argc) mo = argv[++i];
    else if (a == "--beta") beta = true;
    else if (a == "--levels") levels = true;
    else if (a == "--sum") sum = true;
    else if (a == "--offset") { if (!num(offset)) { usage(); return 2; } }
    else if (a == "--view") {
      viewPlane = true;
      for (int n = 0; n < 6; n++) if (!num(viewAxes[n])) { usage(); return 2; }
    }
    else if (a == "--csv" && i+1 < argc) csvFile = argv[++i];
    else if (a == "--step") { if (!num(step)) { usage(); return 2; } }
    else if (a == "--margin") { if (!num(margin)) { usage(); return 2; } }
    else if (a == "--plane" && i+1 < argc) {
      string k = argv[++i];
      int need = 0;
      if (k == "molecular") kind = MOLECULAR;
      else if (k == "atoms") { kind = THREE_ATOMS; need = 3; }
      else if (k == "axial") { kind = AXIAL; need = 2; }
      else { usage(); return 2; }
      for (int n = 0; n < need; n++) {
        if (i+1 >= argc) { usage(); return 2; }
        atoms[n] = atoi(argv[++i]) - 1;
      }
    }
    else if (a == "--point") {
      for (int n = 0; n < 3; n++) { double d; if (!num(d)) { usage(); return 2; } pts.push_back(d); }
    }
    else if (a == "--line") {
      for (int n = 0; n < 6; n++) { double d; if (!num(d)) { usage(); return 2; } pts.push_back(d); }
      double d; if (!num(d)) { usage(); return 2; }
      lineN = (int)d;
    }
    else if (a == "--points" && i+1 < argc) pointsFile = argv[++i];
    else if (a[0] != '-' && target.empty()) target = a;
    else { usage(); return 2; }
  }
  if (target.empty()) { usage(); return 2; }

  string url = target.find("://") != string::npos
                   ? target : "file://" + absolute(target);
  EDSIServerCentral central;
  Resource *res = EDSIFactory::getResource(EcceURL(url));
  IPropCalculation *calc = dynamic_cast<IPropCalculation*>(res);
  if (!calc) { cerr << "densityplane: " << target << " is not a calculation\n"; return 1; }

  Evaluator ev;
  if (!ev.build(calc)) { cerr << "densityplane: " << ev.complaint << endl; return 1; }

  int orbital = 0;
  if (!mo.empty()) {
    orbital = mo == "homo" ? ev.homo() : mo == "lumo" ? ev.homo() + 1 : atoi(mo.c_str());
    if (orbital < 1 || orbital > ev.orbitals(beta)) {
      cerr << "densityplane: no orbital " << mo << " (1.." << ev.orbitals(beta) << ")\n";
      return 1;
    }
  }

  if (!pointsFile.empty()) {
    FILE *f = fopen(pointsFile.c_str(), "r");
    if (!f) { cerr << "densityplane: cannot read " << pointsFile << endl; return 1; }
    double x, y, z;
    while (fscanf(f, "%lf %lf %lf", &x, &y, &z) == 3)
      printf("%.10e\n", orbital ? ev.orbital(orbital, beta, x, y, z)
                                : ev.density(x, y, z));
    fclose(f);
    return 0;
  }

  if (!pts.empty()) {
    auto at = [&](double x, double y, double z) {
      printf("%.6f %.6f %.6f %.10e\n", x, y, z,
             orbital ? ev.orbital(orbital, beta, x, y, z) : ev.density(x, y, z));
    };
    if (lineN == 0) { at(pts[0], pts[1], pts[2]); return 0; }
    for (int k = 0; k < lineN; k++) {
      const double t = lineN > 1 ? double(k)/(lineN - 1) : 0.0;
      at(pts[0] + t*(pts[3]-pts[0]), pts[1] + t*(pts[4]-pts[1]),
         pts[2] + t*(pts[5]-pts[2]));
    }
    return 0;
  }

  Plane plane;
  string why;
  if (viewPlane ? !makeViewPlane(ev.coordsAng, viewAxes, viewAxes + 3, plane, why)
                : !makePlane(kind, ev.coordsAng, ev.atomLabels(), atoms[0],
                             atoms[1], atoms[2], plane, why)) {
    cerr << "densityplane: " << why << endl;
    return 1;
  }
  plane = offsetPlane(plane, offset);
  Map map;
  if (sum && !orbital) { cerr << "densityplane: --sum needs --mo\n"; return 1; }
  if (!ev.map(plane, sum ? ORBITAL_SET : orbital ? ORBITAL : DENSITY, orbital, beta, step, margin, map)) {
    cerr << "densityplane: the map is too large; raise --step\n";
    return 1;
  }
  printf("%s, %s\n%dx%d points at %.3f A, value %.6g .. %.6g\n",
         map.title.c_str(), plane.description.c_str(), map.nu, map.nv,
         map.step, map.zmin, map.zmax);
  for (size_t a = 0; a < map.atoms.size(); a++)
    printf("  %-5s u %7.3f v %7.3f  off-plane %6.3f A\n",
           map.atoms[a].label.c_str(), map.atoms[a].u, map.atoms[a].v,
           map.atoms[a].off);
  if (levels) {
    bool scaled = false;
    const vector<double> l = mapLevels(map, scaled);
    if (scaled) printf("levels scaled to the plane's maximum\n");
    for (size_t k = 0; k < l.size(); k++)
      printf("level %g: %zu segments\n", l[k], contour(map, l[k]).size());
  }
  if (!csvFile.empty()) {
    FILE *f = fopen(csvFile.c_str(), "wb");
    if (!f) { cerr << "densityplane: cannot write " << csvFile << endl; return 1; }
    const string text = csv(map, target);
    fwrite(text.data(), 1, text.size(), f);
    fclose(f);
  }
  return 0;
}

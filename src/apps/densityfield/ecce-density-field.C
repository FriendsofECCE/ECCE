// ecce-density-field: ELF, LOL, the reduced density gradient (NCI) and
// sign(lambda2)rho from a calculation's stored orbitals, written as a Gaussian
// cube file over a box around the molecule (#798, #799, #800).
//
//   ecce-density-field CALC --field elf|lol|rdg|signl2rho|density
//        [--spacing A] [--margin A] [--threads N] -o out.cube
//   ecce-density-field CALC --at X,Y,Z [--at ...]     all fields at points (A)
//
// CALC is a calculation folder in local data mode.  Cube lengths are bohr, as
// the format has it; --spacing and --margin are Angstrom.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#ifndef _WIN32
#include <unistd.h>
#else
#include <direct.h>
#define getcwd _getcwd
#endif

#include "util/EcceURL.H"
#include "tdat/DensityFields.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"

using namespace std;
using DensityPlane::FieldPoint;

static void usage()
{
  cerr << "usage: ecce-density-field CALC --field elf|lol|rdg|signl2rho|density\n"
          "          [--spacing A] [--margin A] [--threads N] -o out.cube\n"
          "       ecce-density-field CALC --at X,Y,Z [--at X,Y,Z ...]   (Angstrom)\n";
}

static string absolute(const string& path)
{
  if (!path.empty() && (path[0] == '/' || (path.size() > 1 && path[1] == ':'))) return path;
  char buf[4096];
  if (!getcwd(buf, sizeof buf)) return path;
  return string(buf) + "/" + path;
}

static const char *SYMBOLS[] = {
  "X","H","He","Li","Be","B","C","N","O","F","Ne","Na","Mg","Al","Si","P","S","Cl","Ar",
  "K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn","Ga","Ge","As","Se","Br","Kr",
  "Rb","Sr","Y","Zr","Nb","Mo","Tc","Ru","Rh","Pd","Ag","Cd","In","Sn","Sb","Te","I","Xe",
  "Cs","Ba","La","Ce","Pr","Nd","Pm","Sm","Eu","Gd","Tb","Dy","Ho","Er","Tm","Yb","Lu",
  "Hf","Ta","W","Re","Os","Ir","Pt","Au","Hg","Tl","Pb","Bi","Po","At","Rn" };

static int atomicNumber(const string& sym)
{
  for (size_t z = 1; z < sizeof SYMBOLS/sizeof *SYMBOLS; z++)
    if (sym == SYMBOLS[z]) return (int)z;
  return 0;
}

static double fieldValue(const string& field, const FieldPoint& p)
{
  if (field == "elf") return DensityFields::elf(p);
  if (field == "lol") return DensityFields::lol(p);
  if (field == "rdg") return DensityFields::rdg(p);
  if (field == "signl2rho") return DensityFields::signL2Rho(p);
  return p.rhoTotal;
}

int main(int argc, char **argv)
{
  string target, field, out;
  double spacing = 0.1, margin = 3.0;
  int threads = (int)thread::hardware_concurrency();
  vector<double> at;

  for (int i = 1; i < argc; i++) {
    string a = argv[i];
    if (a == "--field" && i+1 < argc) field = argv[++i];
    else if (a == "--spacing" && i+1 < argc) spacing = atof(argv[++i]);
    else if (a == "--margin" && i+1 < argc) margin = atof(argv[++i]);
    else if (a == "--threads" && i+1 < argc) threads = atoi(argv[++i]);
    else if (a == "-o" && i+1 < argc) out = argv[++i];
    else if (a == "--at" && i+1 < argc) {
      double x, y, z;
      if (sscanf(argv[++i], "%lf,%lf,%lf", &x, &y, &z) != 3) { usage(); return 2; }
      at.push_back(x); at.push_back(y); at.push_back(z);
    }
    else if (a[0] != '-' && target.empty()) target = a;
    else { usage(); return 2; }
  }
  if (threads < 1) threads = 1;
  const bool grid = at.empty();
  if (target.empty() || (grid && (field.empty() || out.empty())) || spacing <= 0) {
    usage();
    return 2;
  }
  if (grid && field != "elf" && field != "lol" && field != "rdg" &&
      field != "signl2rho" && field != "density") {
    cerr << "ecce-density-field: unknown field " << field << "\n";
    return 2;
  }

  string url = target.find("://") != string::npos
                   ? target : "file://" + absolute(target);
  EDSIServerCentral central;
  Resource *res = EDSIFactory::getResource(EcceURL(url));
  IPropCalculation *calc = dynamic_cast<IPropCalculation*>(res);
  if (!calc) { cerr << "ecce-density-field: " << target << " is not a calculation\n"; return 1; }
  DensityPlane::Evaluator ev;
  if (!ev.build(calc)) { cerr << "ecce-density-field: " << ev.complaint << endl; return 1; }

  if (!grid) {
    printf("# x_A y_A z_A rho gradx grady gradz rho_alpha rho_beta tau_alpha tau_beta"
           " lambda1 lambda2 lambda3 elf lol rdg signl2rho  (atomic units)\n");
    for (size_t k = 0; k < at.size(); k += 3) {
      FieldPoint p;
      ev.fieldsAt(at[k], at[k+1], at[k+2], p);
      printf("%.6f %.6f %.6f %.10e %.10e %.10e %.10e %.10e %.10e %.10e %.10e"
             " %.10e %.10e %.10e %.10e %.10e %.10e %.10e\n",
             at[k], at[k+1], at[k+2], p.rhoTotal, p.gradTotal[0], p.gradTotal[1],
             p.gradTotal[2], p.rho[0], p.rho[1], p.tau[0], p.tau[1], p.eig[0],
             p.eig[1], p.eig[2], DensityFields::elf(p), DensityFields::lol(p),
             DensityFields::rdg(p), DensityFields::signL2Rho(p));
    }
    return 0;
  }

  const double ang = 0.52917721092;
  const size_t natoms = ev.symbols.size();
  double lo[3] = {1e9, 1e9, 1e9}, hi[3] = {-1e9, -1e9, -1e9};
  for (size_t a = 0; a < natoms; a++)
    for (int k = 0; k < 3; k++) {
      lo[k] = min(lo[k], ev.coordsAng[3*a + k]);
      hi[k] = max(hi[k], ev.coordsAng[3*a + k]);
    }
  int n[3];
  for (int k = 0; k < 3; k++) {
    lo[k] -= margin;
    n[k] = (int)ceil((hi[k] + margin - lo[k])/spacing) + 1;
  }
  const size_t total = (size_t)n[0]*n[1]*n[2];
  cerr << "ecce-density-field: " << field << " on " << n[0] << " x " << n[1] << " x "
       << n[2] << " = " << total << " points\n";

  vector<double> values(total);
  // x-slabs are dealt out round-robin; each value is independent.
  auto work = [&](int first) {
    for (int i = first; i < n[0]; i += threads)
      for (int j = 0; j < n[1]; j++)
        for (int k = 0; k < n[2]; k++) {
          FieldPoint p;
          ev.fieldsAt(lo[0] + i*spacing, lo[1] + j*spacing, lo[2] + k*spacing, p);
          values[((size_t)i*n[1] + j)*n[2] + k] = fieldValue(field, p);
        }
  };
  vector<thread> pool;
  for (int t = 1; t < threads; t++) pool.emplace_back(work, t);
  work(0);
  for (auto& t : pool) t.join();

  ofstream f(out.c_str());
  if (!f) { cerr << "ecce-density-field: cannot write " << out << "\n"; return 1; }
  f << "ECCE " << field << " from " << target << "\n"
    << "outer loop x, middle y, inner z; lengths in bohr, values in atomic units\n";
  char buf[200];
  snprintf(buf, sizeof buf, "%5d %12.6f %12.6f %12.6f\n", (int)natoms, lo[0]/ang, lo[1]/ang, lo[2]/ang);
  f << buf;
  for (int k = 0; k < 3; k++) {
    snprintf(buf, sizeof buf, "%5d %12.6f %12.6f %12.6f\n", n[k],
             k == 0 ? spacing/ang : 0.0, k == 1 ? spacing/ang : 0.0, k == 2 ? spacing/ang : 0.0);
    f << buf;
  }
  for (size_t a = 0; a < natoms; a++) {
    const int z = atomicNumber(ev.symbols[a]);
    snprintf(buf, sizeof buf, "%5d %12.6f %12.6f %12.6f %12.6f\n", z, (double)z,
             ev.coordsAng[3*a]/ang, ev.coordsAng[3*a+1]/ang, ev.coordsAng[3*a+2]/ang);
    f << buf;
  }
  for (int i = 0; i < n[0]; i++)
    for (int j = 0; j < n[1]; j++) {
      for (int k = 0; k < n[2]; k++) {
        snprintf(buf, sizeof buf, " %13.5E", values[((size_t)i*n[1] + j)*n[2] + k]);
        f << buf;
        if (k % 6 == 5 || k == n[2] - 1) f << "\n";
      }
    }
  return f.good() ? 0 : 1;
}

// Solvate a structure from the command line (#1118), for scripts and tests:
//   solvate_cli SOLVENT.xyz SOLUTE.xyz|- MARGIN|"a b c" [SCALE]
// SOLUTE is plain XYZ ("-" for none); writes the solvated XYZ to stdout,
// the cell on the comment line.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "tdat/Solvate.H"

using namespace solvate;

int main(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: solvate_cli SOLVENT.xyz SOLUTE.xyz|- MARGIN|\"a b c\" [SCALE]\n");
    return 2;
  }
  SolventBox box;
  std::string err;
  if (!readSolventBox(argv[1], box, &err)) { fprintf(stderr, "%s\n", err.c_str()); return 1; }
  std::vector<std::string> us;
  std::vector<Vec> ux;
  if (std::string(argv[2]) != "-") {
    std::ifstream in(argv[2]);
    int n = 0;
    std::string line;
    std::getline(in, line);
    n = atoi(line.c_str());
    std::getline(in, line);
    for (int i = 0; i < n && std::getline(in, line); ++i) {
      std::istringstream s(line);
      std::string sym;
      Vec p;
      s >> sym >> p[0] >> p[1] >> p[2];
      us.push_back(sym);
      ux.push_back(p);
    }
  }
  Options opt;
  if (argc > 4) opt.scale = atof(argv[4]);
  Cell cell;
  double abc[3];
  if (sscanf(argv[3], "%lf %lf %lf", &abc[0], &abc[1], &abc[2]) == 3) {
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j) cell[i][j] = i == j ? abc[i] : 0.0;
  } else {
    Vec shift;
    marginCell(ux, atof(argv[3]), cell, shift);
    for (Vec& p : ux)
      for (int k = 0; k < 3; ++k) p[k] += shift[k];
  }
  Result res;
  if (!fill(us, ux, cell, box, opt, res, &err)) { fprintf(stderr, "%s\n", err.c_str()); return 1; }
  std::vector<std::string> all = us;
  all.insert(all.end(), res.symbols.begin(), res.symbols.end());
  printf("%d\ncell %.4f %.4f %.4f molecules %d density %.4f\n",
         (int)(us.size() + res.xyz.size()), cell[0][0], cell[1][1], cell[2][2],
         res.molecules, density(all, cell));
  for (size_t i = 0; i < us.size(); ++i)
    printf("%-2s %12.6f %12.6f %12.6f\n", us[i].c_str(), ux[i][0], ux[i][1], ux[i][2]);
  for (size_t i = 0; i < res.xyz.size(); ++i)
    printf("%-2s %12.6f %12.6f %12.6f\n", res.symbols[i].c_str(),
           res.xyz[i][0], res.xyz[i][1], res.xyz[i][2]);
  return 0;
}

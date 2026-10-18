// Command-line front end of VibAnalysis (#367): reads a modes file, prints
// one line per mode.  tests/vibanalysis/run_tests.py writes the file from
// real code output and checks the lines against textbook assignments.
//
//   N                          atoms
//   <element> x y z            N lines, Angstrom
//   bonds B                    optional bond list: 0-based a b order
//   modes M
//   <freq>                     then N lines "dx dy dz", per mode
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "tdat/VibAnalysis.H"

int main(int argc, char **argv)
{
   if (argc < 2) { std::cerr << "usage: vibanalysis_cli <modes-file>\n"; return 2; }
   std::ifstream f(argv[1]);
   VibInput in;
   size_t n = 0;
   f >> n;
   for (size_t i = 0; i < n; i++) {
      std::string el; double x, y, z;
      f >> el >> x >> y >> z;
      in.elements.push_back(el);
      in.coords.push_back(x); in.coords.push_back(y); in.coords.push_back(z);
   }
   std::string word;
   while (f >> word) {
      if (word == "bonds") {
         size_t nb; f >> nb;
         for (size_t i = 0; i < nb; i++) {
            VibBond b; f >> b.a >> b.b >> b.order;
            in.bonds.push_back(b);
         }
      } else if (word == "modes") {
         size_t nm; f >> nm;
         for (size_t m = 0; m < nm; m++) {
            double fr; f >> fr;
            in.freqs.push_back(fr);
            std::vector<double> d(3*n);
            for (size_t k = 0; k < 3*n; k++) f >> d[k];
            in.modes.push_back(d);
         }
      }
   }
   if (!f.eof() && f.fail() && in.modes.empty()) { std::cerr << "bad modes file\n"; return 2; }

   VibResult r;
   if (!VibAnalysis::analyse(in, r)) { std::cerr << "analysis failed\n"; return 1; }
   std::printf("group %s\n", r.group.empty() ? "-" : r.group.c_str());
   if (!r.note.empty()) std::printf("note %s\n", r.note.c_str());
   for (size_t m = 0; m < r.modes.size(); m++) {
      const VibModeInfo& v = r.modes[m];
      std::printf("%zu\t%.2f\t%s\t%s\t%s\t%s\t%s\n", m + 1, in.freqs[m],
                  v.irrep.empty() ? "-" : v.irrep.c_str(),
                  v.activityKnown ? (v.ir ? "IR" : "-") : "?",
                  v.activityKnown ? (v.raman ? "Raman" : "-") : "?",
                  v.label.empty() ? "-" : v.label.c_str(),
                  v.detail.c_str());
   }
   return 0;
}

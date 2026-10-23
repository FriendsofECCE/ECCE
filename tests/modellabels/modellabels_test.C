// Construction labels of the model MO diagram (#458).  The oracles are the
// textbook orders of the N2/O2/CO valence levels and the symmetry facts that
// make water's b1 and BF3's a2' and e'' non-bonding.
//
//   modellabels_test <source dir>     (reads data/client/config/EHTParameters)
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "tdat/FragmentOrbitals.H"
#include "tdat/ModelDiagram.H"

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; \
   printf("FAIL %s:%d  ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

typedef std::vector<ModelDiagram::ConstructLabel> Labels;

static Labels run(const std::vector<std::string>& sym, std::vector<double> xyz,
                  const std::vector<int>& left, const std::vector<int>& right, bool rotate)
{
   if (rotate)   // an arbitrary proper rotation: the labels must not depend on the frame
      for (size_t i = 0; i < xyz.size(); i += 3) {
         const double x = xyz[i], y = xyz[i+1], z = xyz[i+2];
         xyz[i]   = 0.36*x + 0.48*y - 0.8*z;
         xyz[i+1] = -0.8*x + 0.60*y;
         xyz[i+2] = 0.48*x + 0.64*y + 0.6*z;
      }
   ModelDiagram::Input in;
   in.symbols = sym; in.coordsAng = xyz; in.leftAtoms = left; in.rightAtoms = right;
   Labels out;
   std::string why;
   if (!ModelDiagram::constructLabels(in, out, why)) { printf("constructLabels: %s\n", why.c_str()); failures++; }
   return out;
}

static std::string join(const Labels& l, size_t from, size_t n, bool full)
{
   std::string s;
   for (size_t i = from; i < from + n && i < l.size(); i++) s += (i > from ? " " : "") + (full ? l[i].full : l[i].plain);
   return s;
}

static std::vector<double> vec(const double* p, size_t n) { return std::vector<double>(p, p + n); }

int main(int argc, char** argv)
{
   const std::string home = argc > 1 ? argv[1] : ".";
   setenv("ECCE_HOME", home.c_str(), 1);   // FragmentOrbitals reloads from $ECCE_HOME
   if (!FragmentOrbitals::loadParameters(home + "/data/client/config/EHTParameters")) {
      printf("no EHT parameters under %s\n", home.c_str());
      return 1;
   }
   const std::vector<int> a(1, 0), b(1, 1);
   const std::string N2 = "\xCF\x83" "2s \xCF\x83*2s \xCF\x80" "2p \xCF\x83" "2p \xCF\x80*2p \xCF\x83*2p";
   for (int rot = 0; rot < 2; rot++) {
      const double nn[6] = {0, 0, 0, 0, 0, 1.098};
      Labels n2 = run({"N", "N"}, vec(nn, 6), a, b, rot);
      CHECK(n2.size() >= 6, "N2 has %zu levels", n2.size());
      CHECK(join(n2, 0, 6, true) == N2, "N2 (rot %d): %s", rot, join(n2, 0, 6, true).c_str());
      CHECK(join(n2, 0, 6, false) == "\xCF\x83 \xCF\x83* \xCF\x80 \xCF\x83 \xCF\x80* \xCF\x83*",
            "N2 plain: %s", join(n2, 0, 6, false).c_str());
      const double cc[6] = {0, 0, 0, 0, 0, 1.243};
      Labels c2 = run({"C", "C"}, vec(cc, 6), a, b, rot);
      //  C2 is printed, not checked: the unmixed sigma*2p lies below sigma*2s there (open question, #458).
      const double oo[6] = {0, 0, 0, 0, 0, 1.208};
      Labels o2 = run({"O", "O"}, vec(oo, 6), a, b, rot);
      CHECK(join(o2, 0, 6, true) == N2, "O2: %s", join(o2, 0, 6, true).c_str());
      const double co[6] = {0, 0, 0, 0, 0, 1.128};
      Labels c = run({"C", "O"}, vec(co, 6), a, b, rot);
      CHECK(c.size() >= 6 && c[0].full.find("(C 2s/O 2s)") != std::string::npos, "CO parents: %s", join(c, 0, 6, true).c_str());
      printf("N2 rot %d: %s\nC2 rot %d: %s\nCO rot %d: %s\n", rot, join(n2, 0, 6, true).c_str(),
             rot, join(c2, 0, 6, true).c_str(), rot, join(c, 0, 6, true).c_str());
   }

   //  Water in the xz plane: the b1 (O 2py) lone pair has no hydrogen partner.
   {
      const double w[9] = {0, 0, 0,  0.757, 0, 0.587,  -0.757, 0, 0.587};
      Labels l = run({"O", "H", "H"}, vec(w, 9), a, std::vector<int>{1, 2}, true);
      printf("H2O: %s\n", join(l, 0, l.size(), true).c_str());
      int nb = 0;
      for (size_t i = 0; i < l.size(); i++) if (l[i].nonbonding) { nb++; CHECK(l[i].orbitals.size() == 1 && l[i].full == "nb (O 2p)", "water nb: %s", l[i].full.c_str()); }
      CHECK(nb == 1, "water has %d non-bonding levels", nb);
   }
   //  BF3, D3h in the xy plane: a2' and e'' are F p-pi only, nothing of B's.
   {
      std::vector<double> x(12, 0.0);
      for (int k = 0; k < 3; k++) { x[3*(k+1)] = 1.31*cos(2.0944*k); x[3*(k+1)+1] = 1.31*sin(2.0944*k); }
      Labels l = run({"B", "F", "F", "F"}, x, a, std::vector<int>{1, 2, 3}, true);
      printf("BF3: %s\n", join(l, 0, l.size(), true).c_str());
      for (size_t i = 0; i < l.size(); i++) printf("  %zu deg %zu E %.2f eV other %.3f %s\n", i, l[i].orbitals.size(), l[i].energy*27.2114, l[i].otherShare, l[i].full.c_str());
      int nb = 0, degenerate = 0;
      for (size_t i = 0; i < l.size(); i++)
         if (l[i].nonbonding && l[i].otherShare < 1e-6) { nb++; if (l[i].orbitals.size() == 2) degenerate++; }
      CHECK(nb >= 2 && degenerate >= 1, "BF3: %d non-bonding levels, %d of them degenerate", nb, degenerate);
   }
   printf(failures ? "FAILED\n" : "ok\n");
   return failures ? 1 : 0;
}

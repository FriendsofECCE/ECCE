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

//  A bonding level and its antibonding partner are of one sigma/pi type: each
//  type has at least as many bonding as antibonding levels, and none is
//  antibonding alone.
static void partners(const Labels& l, const char* name)
{
   int bond[2] = {0, 0}, anti[2] = {0, 0};
   for (size_t i = 0; i < l.size(); i++) {
      if (l[i].nonbonding || l[i].plain.empty()) continue;
      const int t = l[i].plain.compare(0, 2, "\xCF\x83") == 0 ? 0 : 1;
      (l[i].antibonding ? anti : bond)[t]++;
   }
   for (int t = 0; t < 2; t++)
      CHECK(anti[t] <= bond[t], "%s: %d antibonding vs %d bonding of type %d", name, anti[t], bond[t], t);
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
      CHECK(join(c2, 0, 6, true) == N2, "C2: %s", join(c2, 0, 6, true).c_str());
      const double oo[6] = {0, 0, 0, 0, 0, 1.208};
      Labels o2 = run({"O", "O"}, vec(oo, 6), a, b, rot);
      CHECK(join(o2, 0, 6, true) == N2, "O2: %s", join(o2, 0, 6, true).c_str());
      const double co[6] = {0, 0, 0, 0, 0, 1.128};
      Labels c = run({"C", "O"}, vec(co, 6), a, b, rot);
      CHECK(join(c, 0, 6, true) == "\xCF\x83(C 2s/O 2s) \xCF\x83*(C 2s/O 2s) \xCF\x80(C 2p/O 2p) \xCF\x83(C 2p/O 2p) "
                                   "\xCF\x80*(C 2p/O 2p) \xCF\x83*(C 2p/O 2p)", "CO: %s", join(c, 0, 6, true).c_str());
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
      std::string valence;
      for (size_t i = 0; i < l.size(); i++)
         if (l[i].energy*27.2114 > -30.0 && !l[i].nonbonding) valence += (valence.empty() ? "" : " ") + l[i].full;
      //  4a1', 1a2'', 3e', 2a2''*, 5a1'*, 6e'*.  3e' is pi: its ligand orbitals (e', 82 %
      //  tangential) bond to B 2p only weakly; the sigma e' bond is the F 2s level at -40 eV.
      CHECK(valence == "\xCF\x83(B 2s) \xCF\x80(B 2p) \xCF\x80(B 2p) \xCF\x80*(B 2p) \xCF\x83*(B 2s) \xCF\x83*(B 2p)",
            "BF3 valence: %s", valence.c_str());
      partners(l, "BF3");
      //  3e': a degenerate set that is mostly tangential (a nodal plane through each B-F bond): pi, about 18 % sigma.
      bool found = false;
      for (size_t i = 0; i < l.size(); i++)
         if (l[i].orbitals.size() == 2 && l[i].plain == "\xCF\x80" && l[i].energy*27.2114 > -19.0 && l[i].energy*27.2114 < -18.0) {
            found = true;
            CHECK(l[i].sigmaShare > 0.10 && l[i].sigmaShare < 0.25, "BF3 3e' sigma share %.2f", l[i].sigmaShare);
            CHECK(l[i].split.find("with a nodal plane containing the B\xE2\x80\x93" "F bond") != std::string::npos, "split: %s", l[i].split.c_str());
         }
      CHECK(found, "BF3 3e' not found");
   }
   {
      const double h = 1.012, c = 0.38, r = sqrt(h*h - c*c);
      std::vector<double> x(12, 0.0);
      for (int k = 0; k < 3; k++) { x[3*(k+1)] = r*cos(2.0944*k); x[3*(k+1)+1] = r*sin(2.0944*k); x[3*(k+1)+2] = -c; }
      Labels l = run({"N", "H", "H", "H"}, x, a, std::vector<int>{1, 2, 3}, true);
      printf("NH3: %s\n", join(l, 0, l.size(), true).c_str());
      partners(l, "NH3");
   }
   {
      std::vector<double> x(39, 0.0);
      std::vector<std::string> sym(1, "Cr");
      const double dir[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
      for (int k = 0; k < 6; k++) {
         sym.push_back("C"); sym.push_back("O");
         for (int q = 0; q < 3; q++) { x[3*(1+2*k)+q] = 1.92*dir[k][q]; x[3*(2+2*k)+q] = 3.07*dir[k][q]; }
      }
      std::vector<int> lig;
      for (int k = 1; k < 13; k++) lig.push_back(k);
      Labels l = run(sym, x, a, lig, true);
      printf("Cr(CO)6: %s\n", join(l, 0, l.size(), true).c_str());
      partners(l, "Cr(CO)6");
      //  t2g (3 orbitals) is pi, eg (2) sigma
      int t2g = 0, eg = 0;
      for (size_t i = 0; i < l.size(); i++) {
         if (l[i].full == "\xCF\x80(Cr 3d)" && l[i].orbitals.size() == 3) t2g++;
         if (l[i].full == "\xCF\x83(Cr 3d)" && l[i].orbitals.size() == 2) eg++;
      }
      CHECK(t2g == 1 && eg >= 1, "Cr(CO)6: %d pi t2g, %d sigma eg", t2g, eg);
   }
   printf(failures ? "FAILED\n" : "ok\n");
   return failures ? 1 : 0;
}

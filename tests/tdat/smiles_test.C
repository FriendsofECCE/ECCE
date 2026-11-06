// The SMILES reader and 3-D builder (#867) on a corpus: formula, charge,
// bond lengths, close contacts and ring shape are checked directly.  When
// Open Babel is installed it is used only as an independent oracle: the
// canonical isomeric SMILES of our 3-D structure (read back from an SDF
// with perceived stereo) must equal the canonical form of the input.
#include "tdat/Smiles.H"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unistd.h>

using namespace std;

static int failures = 0, skipped = 0;
static void check(bool ok, const string& what)
{
   if (!ok) { printf("FAIL: %s\n", what.c_str()); failures++; }
}
static double dist(const array<double,3>& a, const array<double,3>& b)
{
   return sqrt((a[0]-b[0])*(a[0]-b[0]) + (a[1]-b[1])*(a[1]-b[1]) + (a[2]-b[2])*(a[2]-b[2]));
}
static map<string,int> parseFormula(const string& f)
{
   map<string,int> m;
   for (size_t i = 0; i < f.size();) {
      string sym(1, f[i++]);
      if (i < f.size() && islower((unsigned char)f[i])) sym += f[i++];
      int n = 0;
      while (i < f.size() && isdigit((unsigned char)f[i])) n = n * 10 + (f[i++] - '0');
      m[sym] += n ? n : 1;
   }
   return m;
}
static string run(const string& cmd)
{
   string out;
   FILE *p = popen(cmd.c_str(), "r");
   if (!p) return out;
   char buf[4096];
   size_t n;
   while ((n = fread(buf, 1, sizeof buf, p)) > 0) out.append(buf, n);
   pclose(p);
   while (!out.empty() && isspace((unsigned char)out.back())) out.pop_back();
   return out;
}
static string canonical(const string& flag, const string& file)
{
   string o = run("obabel " + flag + " " + file + " -ocan 2>/dev/null");
   size_t t = o.find_first_of("\t \n");
   return o.substr(0, t);
}

// V2000 block, turned about an oblique axis so no coordinate plane is
// flat (Open Babel reads a planar file as 2-D and loses E/Z).
static void writeSdf(const char *path, const Smiles::Built& b)
{
   ofstream f(path);
   f << "x\n  test\n\n" << b.el.size() << " " << b.bonds.size() << "\n";
   char line[200];
   snprintf(line, sizeof line, "%3d%3d  0  0  0  0  0  0  0  0999 V2000\n", (int)b.el.size(), (int)b.bonds.size());
   f.seekp(0);
   f << "x\n  test\n\n" << line;
   const double c = cos(0.7), s = sin(0.7), c2 = cos(0.4), s2 = sin(0.4);
   for (size_t i = 0; i < b.el.size(); i++) {
      double x = b.xyz[i][0], y = b.xyz[i][1] * c - b.xyz[i][2] * s, z = b.xyz[i][1] * s + b.xyz[i][2] * c;
      double x2 = x * c2 - z * s2, z2 = x * s2 + z * c2;
      snprintf(line, sizeof line, "%10.4f%10.4f%10.4f %-3s 0  0  0  0  0  0  0  0  0  0  0  0\n",
               x2, y, z2, b.el[i].c_str());
      f << line;
   }
   for (const auto& bd : b.bonds) {
      snprintf(line, sizeof line, "%3d%3d%3d  0\n", bd.a + 1, bd.b + 1, (int)bd.order);
      f << line;
   }
   for (size_t i = 0; i < b.charge.size(); i++)
      if (b.charge[i]) {
         snprintf(line, sizeof line, "M  CHG  1 %3d %3d\n", (int)i + 1, b.charge[i]);
         f << line;
      }
   f << "M  END\n";
}

int main(int argc, char **argv)
{
   const char *corpus = argc > 1 ? argv[1] : "smiles_corpus.txt";
   ifstream in(corpus);
   if (!in) { printf("cannot open %s\n", corpus); return 1; }
   const bool haveOb = system("obabel -V >/dev/null 2>&1") == 0;
   if (!haveOb) printf("note: obabel not found, oracle checks skipped\n");
   char tmp[] = "/tmp/ecce-smiles-test-XXXXXX";
   if (!mkdtemp(tmp)) return 1;
   const string dir = tmp, sdf = dir + "/m.sdf", smi = dir + "/m.smi";

   string line;
   int n = 0, ok = 0;
   while (getline(in, line)) {
      if (line.empty() || line[0] == '#') continue;
      istringstream ls(line);
      string s, formula, flag;
      int q;
      ls >> s >> formula >> q >> flag;
      n++;
      const int before = failures;
      Smiles::Graph g;
      Smiles::Built b;
      string err;
      if (!Smiles::parse(s, g, err) || !Smiles::build3D(g, b, err)) {
         check(false, s + ": " + err);
         continue;
      }
      map<string,int> cnt;
      int charge = 0;
      for (const string& e : b.el) cnt[e]++;
      for (int c : b.charge) charge += c;
      check(cnt == parseFormula(formula), s + ": formula");
      check(charge == q, s + ": charge");
      vector<vector<char> > bonded(b.el.size(), vector<char>(b.el.size(), 0));
      for (const auto& bd : b.bonds) {
         bonded[bd.a][bd.b] = bonded[bd.b][bd.a] = 1;
         double want = Smiles::idealBond(b.el[bd.a], b.el[bd.b], bd.arom ? 1.5 : bd.order);
         double d = dist(b.xyz[bd.a], b.xyz[bd.b]);
         if (fabs(d - want) > 0.1) {
            char m[200];
            snprintf(m, sizeof m, ": bond %d-%d is %.3f, ideal %.3f", bd.a + 1, bd.b + 1, d, want);
            check(false, s + m);
         }
      }
      for (size_t i = 0; i < b.el.size(); i++)
         for (size_t j = i + 1; j < b.el.size(); j++) {
            if (bonded[i][j]) continue;
            double lim = (b.el[i] == "H" && b.el[j] == "H") ? 0.9 : 1.0;
            double d = dist(b.xyz[i], b.xyz[j]);
            if (d < lim) {
               char m[200];
               snprintf(m, sizeof m, ": atoms %d and %d are %.2f apart", (int)i + 1, (int)j + 1, d);
               check(false, s + m);
            }
         }
      if (haveOb && flag != "noracle") {
         writeSdf(sdf.c_str(), b);
         { ofstream(smi) << s << "\n"; }
         string mine = canonical("-isdf", sdf), ref = canonical("-ismi", smi);
         if (mine != ref) check(false, s + ": Open Babel reads our structure as " + mine + ", the input as " + ref);
      } else skipped++;
      if (failures == before) ok++;
   }
   printf("%d corpus entries, %d pass, %d failures counted\n", n, ok, failures);

   // Directly checked stereo and ring shape.
   {
      Smiles::Graph g; Smiles::Built b; string err;
      auto tors = [&](int a, int bb, int c, int d) {
         auto sub = [](const array<double,3>& x, const array<double,3>& y) {
            return array<double,3>{x[0]-y[0], x[1]-y[1], x[2]-y[2]}; };
         auto cr = [](const array<double,3>& x, const array<double,3>& y) {
            return array<double,3>{x[1]*y[2]-x[2]*y[1], x[2]*y[0]-x[0]*y[2], x[0]*y[1]-x[1]*y[0]}; };
         auto dt = [](const array<double,3>& x, const array<double,3>& y) { return x[0]*y[0]+x[1]*y[1]+x[2]*y[2]; };
         auto b1 = sub(b.xyz[bb], b.xyz[a]), b2 = sub(b.xyz[c], b.xyz[bb]), b3 = sub(b.xyz[d], b.xyz[c]);
         auto n1 = cr(b1, b2), n2 = cr(b2, b3);
         double l = sqrt(dt(b2, b2));
         return atan2(dt(cr(n1, n2), b2) / l, dt(n1, n2)) * 180 / M_PI;
      };
      check(Smiles::parse("F/C=C/F", g, err) && Smiles::build3D(g, b, err) &&
            fabs(fabs(tors(0, 1, 2, 3)) - 180) < 15, "F/C=C/F is trans");
      check(Smiles::parse("F/C=C\\F", g, err) && Smiles::build3D(g, b, err) &&
            fabs(tors(0, 1, 2, 3)) < 15, "F/C=C\\F is cis");
      check(Smiles::parse("C1CCCCC1", g, err) && Smiles::build3D(g, b, err), "cyclohexane builds");
      if (b.el.size() == 18) {
         bool chair = true;
         for (int i = 0; i < 6; i++) {
            double t = fabs(tors(i, (i + 1) % 6, (i + 2) % 6, (i + 3) % 6));
            if (t < 40 || t > 70) chair = false;
         }
         check(chair, "cyclohexane ring torsions are chair-like (40-70 degrees)");
      }
      check(Smiles::parse("c1ccccc1", g, err) && Smiles::build3D(g, b, err), "benzene builds");
      bool flat = true;
      for (int i = 0; i < 6 && b.el.size() == 12; i++)
         if (fabs(tors(i, (i + 1) % 6, (i + 2) % 6, (i + 3) % 6)) > 5 &&
             fabs(fabs(tors(i, (i + 1) % 6, (i + 2) % 6, (i + 3) % 6)) - 180) > 5) flat = false;
      check(flat, "benzene is planar");
   }
   // Errors name the column.
   const char *bad[] = {"C(C", "C1CC", "CC)", "C=", "[Xx]", "c1cccc1", "C%1", "[C@@TH1]", "C&C", "C11", 0};
   for (const char **p = bad; *p; p++) {
      Smiles::Graph g; string err;
      bool ok2 = Smiles::parse(*p, g, err);
      check(!ok2 && err.find("column") != string::npos, string("bad SMILES ") + *p + " is refused with a column: " + err);
   }
   run("rm -rf " + dir);
   printf("%s\n", failures ? "FAILED" : "all ok");
   return failures ? 1 : 0;
}

// Straight-path check for TS searches (#468).  Geometries are written out by
// hand: HCN/HNC linear at both ends, a bent HCN->HNC guess, and the SN2
// Cl- + CH3Br -> CH3Cl + Br- end points (Walden inversion).
#include <cstdio>
#include <string>
#include <vector>

#include "tdat/TsPath.H"

static int failures = 0;
#define CHECK(c, msg) do { if (!(c)) { failures++; printf("FAIL %s:%d %s\n", __FILE__, __LINE__, msg); } } while (0)

typedef std::vector<double> V;

int main()
{
   // Order H, C, N; both ends on the z axis, C fixed.
   const V hcn = {0,0,-1.065,  0,0,0,  0,0,1.153};
   const V hnc = {0,0, 2.180,  0,0,0,  0,0,1.170};
   const V bent = {1.4,0,0.5,  0,0,0,  0,0,1.17};
   const std::vector<std::string> el = {"H", "C", "N"};

   TsPathReport r = TsPath::check(hcn, hnc);
   CHECK(r.collinear, "HCN/HNC ends are linear");
   CHECK(r.passes && r.closest < 0.01, "H passes through C");
   std::string w = TsPath::warning(r, el);
   CHECK(w.find("linear") != std::string::npos, "linear text");
   CHECK(w.find("passes H1 through C2") != std::string::npos, "passes text");
   CHECK(w.find("TS Search") != std::string::npos, "points to Guess");

   r = TsPath::check(hcn, hnc, &bent);
   CHECK(!r.collinear, "a bent guess breaks the line");
   CHECK(!r.passes, "path through a bent guess keeps its distance");
   CHECK(TsPath::warning(r, el).empty(), "no warning with the bent guess");

   CHECK(!TsPath::check(hcn, bent).collinear, "one bent end is not collinear");
   CHECK(!TsPath::check(V{0,0,0, 0,0,0.74}, V{0,0,0, 0,0,0.9}).collinear,
         "a diatomic is not warned about");

   // SN2: Cl C H H H Br.  C at the origin; Br leaves along +z, Cl arrives
   // from -z, the three H invert.
   const double s = 0.8660254;
   auto sn2 = [s](double zCl, double zBr, double zH) {
      return V{0,0,zCl,  0,0,0,
               1.03,0,zH,  -1.03*0.5,1.03*s,zH,  -1.03*0.5,-1.03*s,zH,
               0,0,zBr};
   };
   const V rea = sn2(-3.5, 1.95, -0.36), pro = sn2(-1.78, 3.9, 0.36);
   r = TsPath::check(rea, pro);
   CHECK(!r.collinear, "SN2 ends are not linear");
   CHECK(!r.passes, "SN2 straight path keeps every pair apart");
   CHECK(r.closest > 0.9, "SN2 closest approach is a bond length");

   if (failures) { printf("%d failures\n", failures); return 1; }
   printf("tspath ok\n");
   return 0;
}

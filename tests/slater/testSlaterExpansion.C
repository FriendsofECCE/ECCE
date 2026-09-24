#include <cstdio>
#include <vector>
#include "tdat/SlaterExpansion.H"
using namespace std;
struct Case { int n, l; double zeta; const char* label; };
int main() {
  Case cases[] = {
    {1,0,1.26,"H  1s"}, {2,0,1.81,"C  2s"}, {2,1,1.81,"C  2p"},
    {2,0,5.9723090,"O  2s (PM7)"}, {2,1,2.3490170,"O  2p (PM7)"},
    {3,0,2.03,"S  3s"}, {3,1,1.78,"S  3p"}, {3,2,1.00,"S  3d"},
    {4,0,1.50,"K  4s"}, {4,2,1.50,"Fe 3d"}, {5,0,1.60,"Rb 5s"},
    {6,0,1.90,"Cs 6s"}, {6,2,2.00,"Au 5d"},
  };
  int n = sizeof(cases)/sizeof(cases[0]);
  double worst = 1.0; int bad = 0;
  printf("  shell           n l   zeta      overlap with exact\n");
  for (int i = 0; i < n; i++) {
    vector<double> e, c;
    double ov = SlaterExpansion::fit(cases[i].n, cases[i].l, cases[i].zeta, 6, e, c);
    printf("  %-14s  %d %d %7.4f   %.6f%s\n", cases[i].label, cases[i].n,
           cases[i].l, cases[i].zeta, ov, ov < 0.999 ? "   <-- POOR" : "");
    if (ov < worst) worst = ov;
    if (ov < 0.999 || (int)e.size() != 6) bad++;
  }
  printf("\n  worst %.6f over %d shells\n", worst, n);
  // A fit must be rejected, not silently rendered, where it cannot work.
  vector<double> e, c;
  if (SlaterExpansion::fit(2, 5, 1.0, 6, e, c) >= 0.0) { printf("  FAIL l>n-1 accepted\n"); bad++; }
  if (SlaterExpansion::fit(2, 0, -1.0, 6, e, c) >= 0.0) { printf("  FAIL negative zeta accepted\n"); bad++; }
  printf("  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}

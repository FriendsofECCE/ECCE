#include <cstdio>
#include <sstream>
#include <vector>
#include "tdat/SlaterBasisSet.H"
#include "dsm/ICalcUtils.H"
#include "dsm/TGBSConfig.H"
#include "dsm/TGBSGroup.H"
#include "dsm/TGaussianBasisSet.H"
using namespace std;
int main() {
  vector<SlaterBasisSet::Element> els;
  SlaterBasisSet::Element o; o.symbol="O"; o.atomicNumber=8;
  o.zetaS=5.9723090; o.zetaP=2.3490170; o.zetaD=0.0; els.push_back(o);
  SlaterBasisSet::Element h; h.symbol="H"; h.atomicNumber=1;
  h.zetaS=1.2602370; h.zetaP=0.0; h.zetaD=0.0; els.push_back(h);
  SlaterBasisSet::Element s; s.symbol="S"; s.atomicNumber=16;
  s.zetaS=2.0461530; s.zetaP=1.8076780; s.zetaD=3.5103090; els.push_back(s);

  string text;
  if (!SlaterBasisSet::numericalBasis(els, 6, text)) { printf("FAIL: build\n"); return 1; }
  printf("--- generated text, first lines ---\n");
  { istringstream p(text); string l; for (int i=0;i<4 && getline(p,l);i++) printf("%s\n", l.c_str()); }

  istringstream in(text);
  TGBSConfig* cfg = ICalcUtils::importConfig(in);
  if (!cfg) { printf("FAIL: importConfig returned null\n"); return 1; }

  int bad = 0;
  const char* want[] = {"O","H","S"};
  const int   shells[] = {2, 1, 3};
  for (int i = 0; i < 3; i++) {
    vector<const TGaussianBasisSet*> l = cfg->getGBSList(want[i]);
    int ncs = 0;
    if (!l.empty() && l[0]) ncs = l[0]->num_contracted_sets(want[i]);
    printf("  %-2s  basis sets %zu   contracted sets %d  (expected %d)\n",
           want[i], l.size(), ncs, shells[i]);
    if (l.empty() || ncs != shells[i]) bad++;
  }
  printf("  %s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}

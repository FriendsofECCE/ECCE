// The principal quantum number in orbital labels (#827).
#include "tdat/ShellLabel.H"
#include <cstdio>
#include <string>
#include <vector>
using namespace std;

static int failures = 0;
static void check(bool ok, const string& what)
{
  printf("%s %s\n", ok ? "ok   " : "FAIL ", what.c_str());
  if (!ok) failures++;
}
typedef ShellLabel SL;

int main()
{
  check(SL::valenceN("H", 0) == 1 && SL::valenceN("H", 1) == 2, "H 1s, polarisation 2p");
  check(SL::valenceN("C", 0) == 2 && SL::valenceN("C", 1) == 2, "C 2s 2p");
  check(SL::valenceN("C", 2) == 3, "C d is 3d (polarisation)");
  check(SL::valenceN("F", 1) == 2, "F 2p");
  check(SL::valenceN("Na", 0) == 3, "Na 3s");
  check(SL::valenceN("Cl", 0) == 3 && SL::valenceN("Cl", 1) == 3, "Cl 3s 3p");
  check(SL::valenceN("Fe", 2) == 3 && SL::valenceN("Fe", 0) == 4 && SL::valenceN("Fe", 1) == 4, "Fe 3d 4s 4p");
  check(SL::valenceN("Cu", 2) == 3, "Cu 3d");
  check(SL::valenceN("Br", 0) == 4 && SL::valenceN("Br", 1) == 4, "Br 4s 4p");
  check(SL::valenceN("Ce", 3) == 4, "Ce 4f");

  check(SL::coreShells("H").empty() && SL::coreShells("Ne").size() == 1, "H no core; Ne 1s");
  check(SL::coreShells("F").size() == 1 && SL::coreShells("F")[0].first == 1, "F core is 1s");
  check(SL::coreShells("Cl").size() == 3, "Cl core 1s 2s 2p");
  check(SL::coreShells("Fe").size() == 5, "Fe core is [Ar]: 1s 2s 2p 3s 3p");
  check(SL::coreShells("Br").size() == 5, "Br core is [Ar]");
  check(SL::coreCapacity(vector<string>{"F","H"}) == 1, "HF core capacity 1");

  //  HF: MO1 all F s, MO2 F s with a little H, MO3 F p+H s.
  vector<string> el = {"H", "F"};
  vector< vector<SL::Part> > mos = {
    { SL::Part(1,0,1.00) },
    { SL::Part(1,0,0.93), SL::Part(0,0,0.04), SL::Part(1,1,0.03) },
    { SL::Part(1,1,0.70), SL::Part(0,0,0.20), SL::Part(1,0,0.10) } };
  vector<SL::Core> core = SL::coreOrbitals(el, mos);
  vector<string> names = SL::atomNames(el);
  check(names[0] == "H" && names[1] == "F", "unique elements carry no number");
  check(core[0].core && core[0].n == 1, "MO 1 is F 1s core");
  check(!core[1].core && !core[2].core, "MO 2 and 3 are valence");
  check(SL::partName(el, names, mos[0][0], core[0]) == "F 1s", "label F 1s");
  check(SL::partName(el, names, mos[1][0], core[1]) == "F 2s", "label F 2s");
  check(SL::partName(el, names, mos[1][1], core[1]) == "H 1s", "label H 1s");
  check(SL::partName(el, names, mos[2][0], core[2]) == "F 2p", "label F 2p");

  //  Water: two H, one O.
  vector<string> w = {"O", "H", "H"};
  vector<string> wn = SL::atomNames(w);
  check(wn[0] == "O" && wn[1] == "H(1)" && wn[2] == "H(2)", "H(1) H(2), O alone");
  check(SL::partName(w, wn, SL::Part(2,0,0.5), SL::Core()) == "H(2) 1s", "label H(2) 1s");

  //  N2: the 1s sigma-g/sigma-u pair are 50/50 on the two atoms yet core.
  vector<string> n2 = {"N", "N"};
  vector< vector<SL::Part> > nm = {
    { SL::Part(0,0,0.5), SL::Part(1,0,0.5) },
    { SL::Part(0,0,0.5), SL::Part(1,0,0.5) },
    { SL::Part(0,0,0.5), SL::Part(1,0,0.45), SL::Part(0,1,0.05) } };
  vector<SL::Core> nc = SL::coreOrbitals(n2, nm);
  check(nc[0].core && nc[1].core && !nc[2].core, "N2: two core, third valence");

  //  Cl atom-dominated: 1s, 2s, 2p x3, 3s, 3p ...
  vector<string> cl = {"Cl"};
  vector< vector<SL::Part> > cm;
  for (int i = 0; i < 5; i++) cm.push_back({ SL::Part(0, i < 2 ? 0 : 1, 1.0) });
  cm.push_back({ SL::Part(0,0,1.0) });   // 3s
  cm.push_back({ SL::Part(0,1,1.0) });   // 3p
  vector<SL::Core> cc = SL::coreOrbitals(cl, cm);
  check(cc[0].n == 1 && cc[1].n == 2 && cc[2].n == 2 && cc[4].n == 2 && cc[2].l == 1,
        "Cl core 1s 2s 2p");
  check(!cc[5].core && !cc[6].core, "Cl 3s 3p are valence");

  printf(failures ? "FAILED\n" : "PASSED\n");
  return failures ? 1 : 0;
}

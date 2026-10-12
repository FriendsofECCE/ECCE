//  #298: a dummy atom X is not an element -- no basis set, no nuclear charge,
//  not in the formula -- while still counting as an atom (its index is what
//  the constraints and the output tables refer to).
#include <cstdio>
#include <string>
#include <vector>

#include "tdat/Fragment.H"

static int failures = 0;

static void check(bool ok, const std::string& what)
{
  printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
  if (!ok) failures++;
}

int main()
{
  Fragment frag;
  const char* tags[] = {"N", "H", "H", "H", "X"};
  const double xyz[] = {0, 0, 0.1,  0, 0.94, -0.3,  0.81, -0.47, -0.3,
                        -0.81, -0.47, -0.3,  0, 0, 1.1};
  for (int i = 0; i < 5; i++) frag.addAtom(tags[i], xyz + 3 * i);

  check(frag.numAtoms() == 5, "X still counts as an atom (numbering)");
  check(frag.uniqueTagStr() == "H N" || frag.uniqueTagStr() == "N H",
        "basis tags omit X: '" + frag.uniqueTagStr() + "'");
  check(frag.nuclearCharge() == 10, "X adds no nuclear charge");
  check(frag.numElectrons() == 10, "X adds no electrons");
  check(frag.formula().find('X') == std::string::npos,
        "formula omits X: '" + frag.formula() + "'");
  return failures ? 1 : 0;
}

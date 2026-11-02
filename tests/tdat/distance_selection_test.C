#include <cstdio>
#include <stdexcept>
#include "tdat/DistanceSelection.H"
#include "tdat/Fragment.H"
#include "tdat/LatticeDef.H"

int main()
{
  int failures = 0;
  auto check = [&](bool ok, const char *name) {
    printf("%s  %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) ++failures;
  };
  const double xyz[] = {0,0,0, .587,.757,0, .587,-.757,0,
                         3,0,0, 3.587,.757,0, 3.587,-.757,0,
                         8,0,0, 8.587,.757,0, 8.587,-.757,0};
  Fragment water("waters", {"O","H","H","O","H","H","O","H","H"}, xyz);
  for (int i = 0; i < 9; i += 3) {
    water.addBond(i, i+1, 1.0);
    water.addBond(i, i+2, 1.0);
  }
  check(selectWithinDistance(water, {0}, 3.0, false, true) ==
        std::vector<int>({0,1,2,3}), "atoms include the exact radius boundary");
  check(selectWithinDistance(water, {0}, 3.0, true, true) ==
        std::vector<int>({0,1,2,3,4,5}), "whole waters follow bonds");
  check(selectWithinDistance(water, {0}, 3.0, true, false) ==
        std::vector<int>({1,2,3,4,5}), "replace excludes seeds after molecule expansion");
  check(selectWithinDistance(water, {0}, 3.0, false, false) ==
        std::vector<int>({1,2,3}), "replace selects only new atoms");
  check(selectWithinDistance(water, {0,6}, 1.0, false, true) ==
        std::vector<int>({0,1,2,6,7,8}), "distance to any seed, without chaining");
  check(selectWithinDistance(water, {0}, .1, false, false).empty(),
        "no neighbours yields an empty replacement");
  check(selectWithinDistance(water, {0}, .1, true, false) ==
        std::vector<int>({1,2}), "seed fragment expands even with no new atom in radius");
  check(selectWithinDistance(water, {}, 3.0, true, true).empty(), "empty seed set");
  bool rejected = false;
  try { selectWithinDistance(water, {0}, -1, false, true); }
  catch (const std::invalid_argument&) { rejected = true; }
  check(rejected, "invalid radius rejected");

  const double ions[] = {0,0,0, 29.5,0,0};
  Fragment cell("ions", {"Na","Cl"}, ions);
  check(selectWithinDistance(cell, {0}, .6, false, true) == std::vector<int>({0}),
        "no lattice uses Cartesian distances");
  cell.setLattice(new LatticeDef(10,10,10,90,90,90));
  check(selectWithinDistance(cell, {0}, .6, false, true) == std::vector<int>({0,1}),
        "minimum image across multiple cells");
  LatticeDef *skew = new LatticeDef(10,10,10,90,90,90);
  skew->assign(MPoint(10,0,0), MPoint(9,2,0), MPoint(0,0,10));
  cell.setLattice(skew);
  cell.atomRef(1)->coordinates(9.31,.98,0);
  check(selectWithinDistance(cell, {0}, 1.3, false, false) == std::vector<int>({1}),
        "skew cell checks nearest images beyond fractional rounding");
  return failures ? 1 : 0;
}

// Bond-order bookkeeping rule (#439): graphs with explicit neighbours.
#include "tdat/BondOrderRule.H"
#include <cstdio>
#include <string>
#include <vector>
using namespace std;
typedef vector<pair<int,int> > Bonds;
static int fails = 0;

static void check(const char* name, const vector<string>& el, const Bonds& b,
                  const vector<double>& want, size_t unresolved = 0)
{
  BondOrderRule::Result r = BondOrderRule::perceive(el, b);
  bool ok = r.orders == want && r.unresolved.size() == unresolved;
  printf("%s %s", ok ? "ok  " : "FAIL", name);
  for (const string& n : r.notes) printf("  [%s]", n.c_str());
  printf("\n");
  if (!ok) {
    fails++;
    for (double o : r.orders) printf(" %g", o);
    printf("  unresolved=%zu\n", r.unresolved.size());
  }
}

int main()
{
  check("ethane", {"C","C","H","H","H","H","H","H"},
        {{0,1},{0,2},{0,3},{0,4},{1,5},{1,6},{1,7}},
        {1,1,1,1,1,1,1});
  check("ethene", {"C","C","H","H","H","H"},
        {{0,1},{0,2},{0,3},{1,4},{1,5}}, {2,1,1,1,1});
  check("ethyne", {"C","C","H","H"}, {{0,1},{0,2},{1,3}}, {3,1,1});
  check("formaldehyde", {"C","O","H","H"}, {{0,1},{0,2},{0,3}}, {2,1,1});
  check("CO2", {"C","O","O"}, {{0,1},{0,2}}, {2,2});
  check("acetonitrile", {"C","C","N","H","H","H"},
        {{0,1},{1,2},{0,3},{0,4},{0,5}}, {1,3,1,1,1});
  // A Nub counts like a hydrogen.
  check("ethene with nubs", {"C","C","Nub","Nub","Nub","Nub"},
        {{0,1},{0,2},{0,3},{1,4},{1,5}}, {2,1,1,1,1});
  Bonds ring;
  vector<string> el(12, "C");
  for (int i = 0; i < 6; i++) ring.push_back({i, (i + 1) % 6});
  for (int i = 0; i < 6; i++) { el[6 + i] = "H"; ring.push_back({i, 6 + i}); }
  check("benzene", el, ring, {1.5,1.5,1.5,1.5,1.5,1.5,1,1,1,1,1,1});
  // A methyl radical is left single and flagged.
  check("methyl radical", {"C","H","H","H"}, {{0,1},{0,2},{0,3}},
        {1,1,1}, 1);
  check("acetaldehyde", {"C","C","O","H","H","H","H"},
        {{0,1},{1,2},{0,3},{0,4},{0,5},{1,6}}, {1,2,1,1,1,1});
  printf(fails ? "FAILED\n" : "all passed\n");
  return fails != 0;
}

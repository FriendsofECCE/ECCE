// The spin density (alpha - beta) integrates to N(alpha) - N(beta).
//
//   spindensity_test RADIALDIST-FIXTURES ECCEQM-FIXTURES
//
// The expected values come from the multiplicity the calculation was run
// with (doublet H, triplet CH2 UHF, triplet O2 ROKS), not from the stored
// occupations the code under test reads.  Closed-shell Ne must give zero.
#include <cmath>
#include <cstdio>
#include <string>

#include "util/EcceURL.H"
#include "tdat/DensityPlane.H"
#include "dsm/EDSIFactory.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"

using namespace std;

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { ++failures; \
  fprintf(stderr, "FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); } } while (0)

static void one(const string& path, const char *name, double expect, int nElec,
                double step, bool open)
{
  Resource *res = EDSIFactory::getResource(EcceURL("file://" + path));
  IPropCalculation *calc = dynamic_cast<IPropCalculation*>(res);
  DensityPlane::Evaluator ev;
  const bool ok = calc && ev.build(calc);
  CHECK(ok, (string(name) + " builds: " + ev.complaint).c_str());
  if (!ok) return;
  CHECK(ev.openShell() == open, (string(name) + ": openShell").c_str());
  double total, spin;
  ev.integrate(step, 5.0, total, spin);
  fprintf(stderr, "%-12s spin integral %.5f (expect %g), total %.4f (%d electrons)\n",
          name, spin, expect, total, nElec);
  CHECK(fabs(spin - expect) < 2e-3, (string(name) + ": spin integral").c_str());
}

int main(int argc, char **argv)
{
  if (argc < 3) { fprintf(stderr, "usage: spindensity_test RADIALDIST ECCEQM\n"); return 2; }
  const string r = argv[1], q = argv[2];
  one(r + "/ne", "Ne closed", 0.0, 10, 0.2, false);
  one(r + "/h", "H doublet", 1.0, 1, 0.2, true);
  one(r + "/ch2", "CH2 UHF", 2.0, 8, 0.2, true);
  one(q + "/o2-roks-loc", "O2 ROKS", 2.0, 16, 0.2, true);
  fprintf(stderr, failures ? "FAILED\n" : "ok\n");
  return failures ? 1 : 0;
}

// The stalled rule of #769 without the job store: median, floor, few steps,
// resume and out of contact.
#include <cstdio>
#include <string>
#include "util/StalledRule.H"

static int failed = 0;
static void check(bool ok, const char* what)
{
  printf("  %s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) failed++;
}

int main()
{
  StalledRule r(1800, 7200);
  check(r.threshold() == 7200, "no steps: the fixed 2 h");
  r.step(0); r.step(600);
  check(r.threshold() == 7200, "two steps: still the fixed 2 h");
  r.step(1200);
  check(r.threshold() == 1800, "3 steps of 10 min: 3 x median 30 min = the floor");
  check(!r.stalled(1799, false) && r.stalled(1800, false), "stalled at the threshold");

  StalledRule slow(1800, 7200);
  for (int i = 0; i < 5; i++) slow.step(i * 3600.0);
  check(slow.threshold() == 10800, "steps an hour apart: 3 h");
  slow.step(5 * 3600.0 + 100000);      // one outlier does not move the median
  check(slow.threshold() == 10800, "the median ignores one long step");

  StalledRule even(60, 120);
  even.step(0); even.step(100); even.step(300); even.step(600); even.step(1000);
  check(even.threshold() == 3 * 250.0, "even count: mean of the middle two");

  check(!r.stalled(100000, true), "out of contact is never stalled");
  check(!r.stalled(0, false), "output just arrived: not stalled (resume)");
  r.reset();
  check(r.steps() == 0 && r.threshold() == 7200, "reset for a new run");

  check(StalledRule::text(7800) == "no output for 2 h 10 min", "text 2 h 10 min");
  check(StalledRule::text(3600) == "no output for 1 h", "text whole hours");
  check(StalledRule::text(1500) == "no output for 25 min", "text minutes");

  const std::string a = "# seq=3 sent=1.0 idle=0\nscf iter=4 energy=-1\nopt step=2\nopt~ energy=1,2\n";
  const std::string b = "# seq=4 sent=2.0 idle=40\nscf iter=4 energy=-1\nopt step=2\nopt~ energy=1,2,3\n";
  const std::string c = "# seq=5 sent=3.0 idle=0\nscf iter=5 energy=-1\nopt step=2\n";
  check(StalledRule::stepKey(a) == StalledRule::stepKey(b), "history growth is not a step");
  check(StalledRule::stepKey(a) != StalledRule::stepKey(c), "a new iteration is a step");
  check(StalledRule::idleOf(b) == 40 && StalledRule::idleOf(a) == 0, "idle= of the header");
  check(StalledRule::idleOf("# seq=1\nscf idle=9\n") == 0, "idle= only in the header");
  printf(failed ? "FAILED\n" : "all passed\n");
  return failed ? 1 : 0;
}

// JobProgress (#284): the snapshot text the Organizer's Progress box reads,
// and the lines it makes of it.  No wx, no services.
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "util/JobProgress.H"

using namespace std;

static int failures = 0;

static void check(bool ok, const string& what)
{
  cout << (ok ? "ok    " : "FAIL  ") << what << endl;
  if (!ok) failures++;
}

static bool has(const vector<string>& v, const string& s)
{
  for (const string& x : v)
    if (x.find(s) != string::npos) return true;
  return false;
}

int main()
{
  const string running =
      "# seq=7 sent=1760000000.500 at=1760000001\n"
      "scf cycle=12 energy=-76.4089 dE=-3.10e-06/1e-10 grad=2.00e-04/1e-07\n"
      "scf~ dE=-1.2e-01,-3.4e-02,-3.10e-06\n"
      "opt step=4 energy=-76.4089 maxgrad=1.20e-03/3e-04 rmsgrad=5.00e-05/1e-04"
      " dE=-2.00e-06/5e-06\n"
      "opt~ energy=-76.40,-76.405,-76.408,-76.4089\n"
      "freq point=14/54 eta=120\n";

  JobProgress p;
  check(p.parse(running), "a snapshot parses");
  check(p.seq() == 7 && p.at() == 1760000001, "header: seq and the job store's stamp");
  check(p.stages().size() == 3, "three stages");
  const JobProgress::Stage *scf = p.stage("scf");
  check(scf && scf->history.size() == 3 && scf->historyKey == "dE",
        "the SCF history is read");
  check(scf && scf->field("cycle") == "12", "fields keep their text");

  double v, t;
  bool hasT;
  check(JobProgress::split("1.20e-03/3e-04", v, t, hasT) && hasT &&
        fabs(v - 1.2e-3) < 1e-12 && fabs(t - 3e-4) < 1e-12, "value/target split");
  check(JobProgress::split("12", v, t, hasT) && !hasT && v == 12, "a bare value");
  check(!JobProgress::split("yes", v, t, hasT), "not a number");

  vector<JobProgress::Block> blocks = p.running(40);
  check(blocks.size() == 3 && blocks[0].title == "Optimisation" &&
        blocks[1].title == "Frequencies" && blocks[2].title == "SCF",
        "optimisation, frequencies, then SCF");
  const JobProgress::Block& opt = blocks[0];
  int met = 0, unmet = 0;
  for (const auto& c : opt.criteria) (c.met ? met : unmet)++;
  check(opt.criteria.size() == 3 && met == 2 && unmet == 1,
        "criteria against targets: max force not met, RMS force and dE met");
  check(opt.plot.size() == 4 && !opt.plotLog, "an energy plot per step");
  const JobProgress::Block& freq = blocks[1];
  check(freq.bar && fabs(freq.fraction - 14.0 / 54) < 1e-9, "the frequency bar");
  check(has(freq.lines, "point 14 of 54") && has(freq.lines, "about 80 s left"),
        "frequency point and the time left after 40 s: " +
        (freq.lines.empty() ? string() : freq.lines[0]));
  const JobProgress::Block& s = blocks[2];
  check(s.plotLog && s.plot.size() == 3 && fabs(s.plot[2] - log10(3.1e-6)) < 1e-9,
        "log |dE| per SCF cycle");

  const string done =
      "# seq=20 sent=1760000100.000\n"
      "opt step=6 converged=yes\n"
      "opt~ energy=-76.40,-76.4089,-76.40895\n"
      "freq point=54/54 done=yes real=3 imaginary=0 zpe=0.0213456 eta=0\n"
      "scf cycle=9 energy=-76.4 converged=yes\n";
  JobProgress f;
  f.parse(done);
  vector<string> sum = f.summary(true);
  check(sum.size() == 2 && has(sum, "Optimisation converged in 6 steps") &&
        has(sum, "-76.40895000 hartree"), "summary: optimisation and final energy");
  check(has(sum, "Frequencies: 3 real, 0 imaginary") && has(sum, "ZPE 0.021346"),
        "summary: frequencies and ZPE");

  JobProgress e;
  e.parse("# seq=3\nscf cycle=15 energy=-1.1 converged=yes\n");
  vector<string> es = e.summary(true);
  check(es.size() == 1 && has(es, "SCF converged in 15 cycles"), "single point summary");

  JobProgress k;
  k.parse("# seq=3\nopt step=3 energy=-76.4\n");
  check(has(k.summary(false), "Optimisation stopped at step 3"), "a stopped run");

  JobProgress none;
  check(!none.parse("") && none.empty(), "empty text is no progress");
  check(JobProgress::updatedText(3) == "updated 3 s ago" &&
        JobProgress::updatedText(300) == "updated 5 min ago", "updated n s ago");

  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}

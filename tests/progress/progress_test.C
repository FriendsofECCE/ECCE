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

  // A code that gives no mode counts, and a value without a target.
  JobProgress g;
  g.parse("# seq=4\nfreq step=15 done=yes\nopt step=3 grad=0.141\n");
  check(has(g.summary(true), "Frequencies: done"), "summary: done without counts");
  vector<JobProgress::Block> gb = g.running(0);
  check(gb.size() == 2 && gb[0].criteria.empty() &&
        has(gb[0].lines, "gradient 1.4e-01"), "a value without a target is a line");

  JobProgress r;
  r.parse("# seq=5\nmd point=100/500 eta=30\n");
  vector<JobProgress::Block> rb = r.running(0);
  check(rb.size() == 1 && rb[0].title == "Molecular dynamics" && rb[0].bar &&
        has(rb[0].lines, "step 100 of 500"), "a step-counted run (GROMACS)");
  {
    JobProgress md;
    md.parse("# seq=1\nmd point=5/5\nopt converged=no\n");
    vector<string> ms = md.summary(true);
    check(ms.size() == 1 && ms[0].find("Optimisation") == string::npos,
          "an md run is not reported as an unconverged optimisation");
  }
  {
    // A relaxed scan: its point first, then the optimisation; the total is
    // shown only when known and not smaller than the point.
    JobProgress sc;
    sc.parse("# seq=7\nopt step=2 energy=-1.5\nopt~ energy=-1.1,-1.2,-1.3,-1.4,-1.5\n"
             "scan point=4/11\n");
    vector<JobProgress::Block> sb = sc.running(0);
    check(sb.size() == 2 && sb[0].title == "Scan" && has(sb[0].lines, "point 4 of 11") &&
          sb[1].plot.size() == 5, "a scan: point 4 of 11, then the step plot");
    sc.parse("# seq=8\nscan point=4/\nopt step=1\n");
    check(has(sc.running(0)[0].lines, "point 4") && !sc.running(0)[0].bar,
          "a scan without a known total");
    sc.parse("# seq=9\nscan point=12/11\nopt step=1\n");
    check(has(sc.running(0)[0].lines, "point 12") && !sc.running(0)[0].bar,
          "a total smaller than the point (several scanned parameters) is dropped");
  }
  r.parse("# seq=6\nsteep point=4/200 converged=yes\n");
  check(has(r.summary(true), "Minimisation converged in 4 steps"),
        "minimisation summary");
  vector<JobProgress::Block> cb = r.running(0);
  check(cb.size() == 1 && cb[0].bar && cb[0].fraction == 1.0,
        "a converged run's bar is full before the step limit");

  // A GROMACS Prepare: the step under way, then the outcome.
  {
    JobProgress pp;
    pp.parse("# seq=1\nprepare point=3/4 tool=solvate eta=5\n");
    vector<JobProgress::Block> pb = pp.running(0);
    check(pb.size() == 1 && pb[0].title == "Prepare" && pb[0].bar &&
          fabs(pb[0].fraction - 0.5) < 1e-9 &&
          has(pb[0].lines, "step 3 of 4 \xc2\xb7 solvate") &&
          !has(pb[0].lines, "left"), "Prepare: step 3 of 4 under way, no time left");
    check(pp.shortLine() == "Prepare 3/4", "cell: Prepare: " + pp.shortLine());
    check(has(pp.summary(true), "Prepare stopped at step 3 of 4 (solvate)"),
          "a Prepare that stops says where");
    JobProgress pd;
    pd.parse("# seq=2\nprepare point=4/4 tool=ions done=yes atoms=5913 residues=20 "
             "waters=1866 pos=5 neg=6 posname=NA negname=CL charge0=1.000000 "
             "charge1=0 box=dodecahedron a=4.392 b=4.392 c=4.392 alpha=60.00 "
             "beta=60.00 gamma=90.00\n");
    vector<string> ds = pd.summary(true);
    check(ds.size() == 2 &&
          has(ds, "Prepare complete \xc2\xb7 5913 atoms \xc2\xb7 20 protein residues "
                  "\xc2\xb7 1866 waters \xc2\xb7 5 NA, 6 CL"),
          "Prepare summary: the system");
    check(has(ds, "Net charge 1 to 0 \xc2\xb7 rhombic dodecahedron box 4.392 "
                  "\xc3\x97 4.392 \xc3\x97 4.392 nm, angles 60.00/60.00/90.00"),
          "Prepare summary: charge and box");
    vector<JobProgress::Block> db = pd.running(0, true);
    check(db.size() == 1 && db[0].fraction == 1.0, "a finished Prepare's bar is full");
  }

  // The table cell.
  check(p.shortLine() == "Freq 14/54", "cell: frequencies, the last of opt/freq: " + p.shortLine());
  JobProgress c1;
  c1.parse("# seq=1\nscf cycle=7 dE=-4.1e-05/1e-8\nopt step=4 energy=-1\n");
  check(c1.shortLine() == "Opt step 4 \xc2\xb7 SCF 7", "cell: optimisation and its SCF: " + c1.shortLine());
  JobProgress c2;
  c2.parse("# seq=1\nscf cycle=7 energy=-1 dE=-4.1e-05/1e-8\n");
  check(c2.shortLine() == "SCF 7 \xc2\xb7 \xce\x94" "E 4e\xe2\x88\x92" "5", "cell: SCF alone: " + c2.shortLine());
  JobProgress c3;
  c3.parse("# seq=1\nopt step=6 converged=yes\nfreq step=9\n");
  check(c3.shortLine() == "Freq step 9", "cell: freq without a total: " + c3.shortLine());
  JobProgress c4;
  c4.parse("# seq=1\nmd point=100/500\n");
  check(c4.shortLine() == "MD 100/500", "cell: MD run: " + c4.shortLine());
  check(JobProgress().shortLine().empty(), "cell: nothing without progress");

  JobProgress e;
  e.parse("# seq=3\nscf cycle=15 energy=-1.1 converged=yes\n");
  vector<string> es = e.summary(true);
  check(es.size() == 1 && has(es, "SCF converged in 15 cycles"), "single point summary");

  // MOPAC: no SCF cycles, a gradient history that is no energy.
  JobProgress m;
  m.parse("# seq=2\nopt step=5 grad=0.484/1.0 converged=yes\n"
          "opt~ grad=31.9,6.2,0.484\nscf converged=yes\n");
  vector<string> ms = m.summary(true);
  check(has(ms, "Optimisation converged in 5 steps") && !has(ms, "hartree"),
        "MOPAC summary: no gradient shown as an energy");
  JobProgress m1;
  m1.parse("# seq=1\nscf converged=yes\n");
  check(has(m1.summary(true), "SCF converged") &&
        !has(m1.summary(true), "cycles"), "SCF converged without a count");

  JobProgress k;
  k.parse("# seq=3\nopt step=3 energy=-76.4\n");
  check(has(k.summary(false), "Optimisation stopped at step 3"), "a stopped run");

  JobProgress none;
  check(!none.parse("") && none.empty(), "empty text is no progress");
  check(JobProgress::updatedText(3) == "updated 3 s ago" &&
        JobProgress::updatedText(300) == "updated 5 min ago", "updated n s ago");
  // Out of contact (#284): minutes are whole, the time is local.
  check(JobProgress::noContactText(45) == "\xe2\x9a\xa0 no contact for 45 s" &&
        JobProgress::noContactText(17 * 60 + 50) ==
            "\xe2\x9a\xa0 no contact for 17 min",
        "no contact for n min: " + JobProgress::noContactText(17 * 60 + 50));
  const string heard = JobProgress::lastHeardText(1760000000, 1760000000 + 600);
  check(heard.size() == 18 && heard[2] == ':' &&
        heard.substr(5) == " (10 min ago)", "last heard hh:mm (n min ago): " + heard);

  cout << (failures ? "FAILED" : "PASSED") << endl;
  return failures ? 1 : 0;
}

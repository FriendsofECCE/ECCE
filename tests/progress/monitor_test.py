#!/usr/bin/env python3
"""The real eccejobmonitor's [PROGRESS] entries (#284), over a made-up
ECCE-QM output in post mode, with scripts/parsers/ecceqm.desc.

Checks that the monitor
  - sends no jmPROG unless the job store asks (-progress 1);
  - sends at most one a second (post mode reads everything at once, so:
    the first line's snapshot, then the last one a second later);
  - folds lines into stages: a new record clears the stage's other keys,
    a lower one starts it over (a new SCF clears its history);
  - matches a progress line inside another entry's block without taking
    it from that block;
  - keeps the message within one packet however long the histories get;
  - Remember= values reach other entries' {name}, Fortran 1.0D-04 becomes
    1.0e-04, and the lines of jobOutputFile2 (stdout) are matched too.

    tests/progress/monitor_test.py
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
MONITOR = os.path.join(ROOT, "scripts", "eccejobmonitor")
DESC = os.path.join(ROOT, "scripts", "parsers", "ecceqm.desc")
MSG_MAXLEN = 2042

failures = []


def check(ok, what):
    print("%s %s" % ("ok   " if ok else "FAIL ", what))
    if not ok:
        failures.append(what)


def output_text():
    lines = []
    e = -76.0
    for step in range(1, 81):          # long enough to need trimming
        for cycle in range(1, 6):
            lines.append("progress scf cycle=%d energy=%.10f dE=%.2e/1e-10 "
                         "grad=%.2e/1e-07" % (cycle, e, 10.0 ** -cycle, 10.0 ** -cycle))
        e -= 0.001
        lines.append("opt_step %d" % step)
        lines.append("energy_step %.12f" % e)
        lines.append("begin geometry  # index symbol x y z (angstrom)")
        if step == 3:
            # inside another entry's block: matched, and left in the block
            lines.append("progress freq point=0/18")
        lines.append("1 O 0.0 0.0 0.0")
        lines.append("end geometry")
        lines.append("progress opt step=%d energy=%.10f maxgrad=1.00e-03/3e-04 "
                     "rmsgrad=5.00e-05/1e-04%s" % (step, e,
                     " dE=-1.00e-03/5e-06" if step > 1 else ""))
    lines.append("progress opt step=80 converged=yes")
    for k in range(1, 19):
        lines.append("progress freq point=%d/18" % k)
    lines.append("progress freq point=18/18 done=yes real=3 imaginary=0 zpe=0.0213456789")
    lines.append("energy_total -76.0800000000")
    lines.append("end_of_output")
    return "\n".join(lines) + "\n"


def run(work, out, progress):
    results = os.path.join(work, "results-%d.out" % progress)
    cmd = ["perl", MONITOR, "-commType", "stdio", "-monitoringMode", "post",
           "-jobOutputFile", out, "-parseDescriptorFile", DESC,
           "-parseTypes", "ALL", "-outputFile", results, "-calcName", "p",
           "-logFile", os.path.join(work, "monitor.log"),
           "-filesToTransfer", "", "-importDir", work + os.sep]
    if progress:
        cmd += ["-progress", "1"]
    proc = subprocess.run(cmd, cwd=work, stdin=subprocess.DEVNULL,
                          capture_output=True, text=True, timeout=120)
    check(proc.returncode == 0, "monitor ran (progress=%d, rc=%d)" % (progress, proc.returncode))
    raw = open(results, "rb").read().decode("utf-8", "replace")
    msgs, i = [], 0
    while True:
        i = raw.find("\001", i)
        if i < 0:
            break
        seq, length = raw[i + 1], int(raw[i + 2:i + 6])
        payload = raw[i + 6:i + 6 + length]
        i += 6 + length
        msgs.append((seq, payload.replace("\002", "\n")))
    return msgs


def check_remember_and_second_file():
    work = tempfile.mkdtemp(prefix="ecce-progress-")
    try:
        desc = os.path.join(work, "t.desc")
        with open(desc, "w") as h:
            h.write("[PROGRESS]\nBegin=^thr (\\S+)\nRemember=tol\n[END]\n"
                    "[PROGRESS]\nBegin=^iter (\\d+) (\\S+)\n"
                    "Progress=scf cycle=$1 dE=$2/{tol} grad=$2/{nothing}\n[END]\n")
        parse = os.path.join(work, "parse.out")
        stdout = os.path.join(work, "stdout.out")
        with open(parse, "w") as h:
            h.write("nothing here\n")
        with open(stdout, "w") as h:
            h.write("thr 1.0D-04\niter 1 5.0D-03\niter 2 2.5D-05\n")
        cmd = ["perl", MONITOR, "-commType", "stdio", "-monitoringMode", "post",
               "-jobOutputFile", parse, "-jobOutputFile2", stdout,
               "-parseDescriptorFile", desc, "-parseTypes", "ALL",
               "-outputFile", os.path.join(work, "r.out"), "-calcName", "p",
               "-logFile", os.path.join(work, "m.log"), "-filesToTransfer", "",
               "-importDir", work + os.sep, "-progress", "1"]
        subprocess.run(cmd, cwd=work, stdin=subprocess.DEVNULL,
                       capture_output=True, text=True, timeout=60)
        raw = open(os.path.join(work, "r.out"), "rb").read().decode("utf-8", "replace")
        ok = "scf cycle=2 dE=2.5e-05/1.0e-04 grad=2.5e-05/" in raw.replace("\002", "\n")
        check(ok, "Remember, {name} (unknown: empty), D exponents and jobOutputFile2")
    finally:
        shutil.rmtree(work, ignore_errors=True)


def check_qe_md_snapshot():
    """A QE MD output through qe.desc: the md stage counts steps, and the
    estimate (a mean over at least two finished steps) never exceeds the
    run's own length."""
    work = tempfile.mkdtemp(prefix="ecce-progress-")
    try:
        fixture = os.path.join(ROOT, "tests", "parsers", "fixtures", "qe", "si_md.pwout")
        results = os.path.join(work, "r.out")
        cmd = ["perl", MONITOR, "-commType", "stdio", "-monitoringMode", "post",
               "-jobOutputFile", fixture,
               "-parseDescriptorFile", os.path.join(ROOT, "scripts", "parsers", "qe.desc"),
               "-parseTypes", "NONE", "-outputFile", results, "-calcName", "p",
               "-logFile", os.path.join(work, "m.log"), "-filesToTransfer", "",
               "-importDir", work + os.sep, "-progress", "1"]
        subprocess.run(cmd, cwd=work, stdin=subprocess.DEVNULL,
                       capture_output=True, text=True, timeout=60)
        raw = open(results, "rb").read().decode("utf-8", "replace").replace("\002", "\n")
        mds = re.findall(r"^md point=(\d+)/(\d+)(?: eta=(\d+))?", raw, re.M)
        check(bool(mds) and mds[-1][:2] == ("5", "5"), "QE MD: the md stage reaches point=5/5")
        check(all(not e or int(e) < 3600 for _, _, e in mds), "QE MD: no absurd estimate")
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    if not shutil.which("perl"):
        return 77
    work = tempfile.mkdtemp(prefix="ecce-progress-")
    try:
        out = os.path.join(work, "ecceqm.qmout")
        with open(out, "w") as handle:
            handle.write(output_text())

        plain = run(work, out, 0)
        check(not any(p.startswith("jmPROG") for _, p in plain),
              "no jmPROG without -progress 1")

        msgs = run(work, out, 1)
        prog = [(s, p) for s, p in msgs if p.startswith("jmPROG\0")]
        check(len(prog) == 2, "two snapshots: the first line's and the last (%d)" % len(prog))
        check(all(s == "0" for s, _ in prog), "each in a single packet")
        check(all(len(p) <= MSG_MAXLEN for _, p in prog),
              "each within %d bytes (%s)" % (MSG_MAXLEN, [len(p) for _, p in prog]))
        texts = [p.split("\0", 1)[1] for _, p in prog]
        heads = [dict(w.split("=") for w in t.splitlines()[0][2:].split()) for t in texts]
        check([h.get("seq") for h in heads] == ["1", "2"], "numbered 1, 2")
        if len(heads) == 2:
            gap = float(heads[1]["sent"]) - float(heads[0]["sent"])
            check(gap >= 1.0, "a second apart at least (%.3f s)" % gap)
        last = {}
        for line in texts[-1].splitlines()[1:]:
            stage, _, rest = line.partition(" ")
            last[stage] = rest
        print("  last snapshot:\n    " + "\n    ".join(texts[-1].splitlines()))
        check(last.get("opt", "").startswith("step=80 ") and "converged=yes" in last.get("opt", ""),
              "opt: the last step, converged, its criteria kept (same record)")
        check("maxgrad=1.00e-03/3e-04" in last.get("opt", ""), "opt: criteria with targets")
        check(last.get("freq", "").startswith("point=18/18 done=yes") and "zpe=" in last["freq"],
              "freq: done, with real/imaginary/zpe")
        check(last.get("scf", "").startswith("cycle=5 "), "scf: the last cycle")
        hist = last.get("scf~", "")
        check(hist.startswith("dE=") and len(hist[3:].split(",")) == 5,
              "scf history restarted with the last SCF (5 values): %s" % hist)
        ohist = last.get("opt~", "")
        n = len(ohist[7:].split(",")) if ohist.startswith("energy=") else 0
        check(10 <= n <= 60, "opt history kept, trimmed to fit (%d values)" % n)
        geo = [p for _, p in msgs if "\0GEOMTRACE\0" in p and "progress freq point=0/18" in p]
        check(len(geo) == 1, "the progress line inside a geometry block stays in that block")
        check_remember_and_second_file()
        check_qe_md_snapshot()
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: %s" % "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

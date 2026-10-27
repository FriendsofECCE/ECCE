#!/usr/bin/env python3
"""The generated job script ends a run that did not converge as 241 (#614).

MOPAC and Quantum ESPRESSO exit 0 after an optimisation out of cycles or an
SCF that did not converge.  This feeds saved output through the status block
gensub writes (no code, no services, no ECCE state: a temp tree) and checks
the exit status the script would report.

    tests/launch/notconverged_gensub_test.py
"""

import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(REPO, "tests", "queues"))
import run_tests as q   # noqa: E402

PROGRESS = os.path.join(REPO, "tests", "progress", "fixtures")
SYNTH = os.path.join(REPO, "tests", "jobstore", "fixtures", "synthetic")
PARSERS = os.path.join(REPO, "tests", "parsers", "fixtures")
QE = "QuantumESPRESSO: /opt/codes/pw.x\n"

# code, gensub -c name, in, out, config line, fixture, expected status
CASES = [
    ("mopac", "MOPAC", "mopac.mop", "mopac.mopout", "",
     os.path.join(PROGRESS, "mopac", "capped.out"), 241),
    ("quantumespresso", "QuantumESPRESSO", "qe.pwin", "qe.pwout", QE,
     os.path.join(SYNTH, "qe-noconverge.pwout"), 241),
    ("quantumespresso", "QuantumESPRESSO", "qe.pwin", "qe.pwout", QE,
     os.path.join(PROGRESS, "qe", "capped.pwout"), 241),
    # An md run ends on the same "maximum number of steps" line: a result.
    ("quantumespresso", "QuantumESPRESSO", "qe.pwin", "qe.pwout", QE,
     os.path.join(PARSERS, "qe", "si_md.pwout"), 0),
]

failures = []


def check(ok, what):
    print("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)


def statusOf(g, code, name, infile, outfile, sample):
    q.CODES[code] = (name, infile, outfile)
    script = g.script("shell", code, q.PROFILES["basic"], rundir="/qtest/run")
    lines = script.splitlines()
    symbols = [l for l in lines if re.match(r"X_[A-Z_]+=\d+", l)]
    start = next(i for i, l in enumerate(lines) if l.startswith("exitStatus=$?"))
    end = next(i for i, l in enumerate(lines) if l.startswith('echo "Final exit status'))
    run = tempfile.mkdtemp(prefix="notconv-", dir=g.tmp)
    with open(sample, errors="replace") as h, open(os.path.join(run, outfile), "w") as o:
        o.write(h.read())
    body = "\n".join(symbols + ["true"] + lines[start:end + 1] + ['echo "$exitStatus"'])
    body = body.replace("/qtest/run", run)
    res = subprocess.run(["sh", "-c", body], stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, cwd=run)
    return res.stdout.decode().strip().splitlines()[-1]


def main():
    g = q.Gensub()
    try:
        for code, name, infile, outfile, cfg, sample, want in CASES:
            g.config(cfg)
            got = statusOf(g, code, name, infile, outfile, sample)
            check(got == str(want), "%s on %s: status %s (want %s)" %
                  (code, os.path.relpath(sample, REPO), got, want))
    finally:
        g.close()
    print("FAILED: %s" % "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

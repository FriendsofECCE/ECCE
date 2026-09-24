#!/usr/bin/env python3
"""tests/shell -- the local shell connection, for real.

Runs `shellprobe` (a small C++ program linked against the real, compiled
RCommand) once per shell dialect ECCE supports for a machine, over a real
pty, against the real local machine.  Every check corresponds to a bug that
shipped; see tests/shell/README.md.

    tests/shell/run_tests.py
    tests/shell/run_tests.py --shell bash --keep
    tests/shell/run_tests.py --probe /path/to/shellprobe
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

# A csh script, exactly like the one scripts/gensub generates for a real
# submission (#!/bin/csh, hardcoded there), running a job long enough that
# the connection is torn down while it is still going.
#
# The perl sleeper installs its own SIGHUP handler.  That matters: nohup
# protects a job by setting SIGHUP to SIG_IGN, which children inherit -- but
# a code that installs a handler of its own (Gaussian does, which is how
# "Error: hangup" reached it in #69) throws that protection away, so nohup
# alone is not a test of anything.
SUBMIT = r"""#!/bin/csh
perl -e '$SIG{HUP} = sub { open(F, ">>", "%(log)s"); print F "GOT SIGHUP\n"; close(F); exit(129) }; open(F, ">>", "%(log)s"); print F "START\n"; close(F); sleep 8;'
%(mopac)s
echo COMPLETED >> %(log)s
"""

MOPAC_DECK = """PM7 PRECISE
water
 test deck for tests/shell
 O   0.000000  0.000000  0.000000
 H   0.000000  0.000000  0.960000
 H   0.930000  0.000000 -0.240000
"""


def which(name):
    return shutil.which(name)


def find_probe():
    for d in ("build-wt", "build-cmake", "build"):
        cand = os.path.join(ROOT, d, "shellprobe")
        if os.path.exists(cand):
            return cand
    return None


def make_workdir(keep, with_mopac):
    d = tempfile.mkdtemp(prefix="ecce-shelltest-")
    for name in ("globme1.dat", "globme2.dat"):
        open(os.path.join(d, name), "w").write("x\n")
    mopac = ""
    if with_mopac:
        open(os.path.join(d, "mopacjob.mop"), "w").write(MOPAC_DECK)
        mopac = "mopac mopacjob.mop"
    script = os.path.join(d, "victim_submit")
    open(script, "w").write(SUBMIT % {"log": os.path.join(d, "victim.log"),
                                      "mopac": mopac})
    os.chmod(script, 0o755)
    return d


def run_shell(probe, shell, keep, verbose):
    with_mopac = which("mopac") is not None
    d = make_workdir(keep, with_mopac)
    print("== %s (workdir %s%s)" % (shell, d,
                                    ", with mopac" if with_mopac else ""))
    # The ecce-<app> wrapper scripts export this; anything linked against
    # libecceutil asserts on its absence (Ecce::realUser()).
    env = dict(os.environ)
    env.setdefault("ECCE_REALUSER", os.environ.get("USER") or
                   __import__("getpass").getuser())
    proc = subprocess.run([probe, shell, d], stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, timeout=300, env=env)
    out = proc.stdout.decode("utf8", "replace")
    failed = []
    for line in out.splitlines():
        if line.startswith(("PASS ", "FAIL ", "RESULT ")) or verbose:
            print("   " + line)
        if line.startswith("FAIL "):
            failed.append(line[5:].split()[0])
    if proc.returncode != 0 and not failed:
        failed.append("probe exited %d" % proc.returncode)
        if not verbose:
            print(out)

    # The real code, end to end: mopac ran from inside the backgrounded
    # submit script, after the connection that started it was gone.
    if with_mopac:
        outfile = os.path.join(d, "mopacjob.out")
        text = open(outfile).read() if os.path.exists(outfile) else ""
        ok = "== MOPAC DONE ==" in text and "HEAT OF FORMATION" in text
        print("   %s mopac.completed-after-teardown" % ("PASS" if ok else "FAIL"))
        if not ok:
            failed.append("mopac.completed-after-teardown")

    if keep:
        print("   kept %s" % d)
    else:
        shutil.rmtree(d, ignore_errors=True)
    return failed


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--probe", default=None)
    ap.add_argument("--shell", action="append", default=None)
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("-v", "--verbose", action="store_true")
    args = ap.parse_args()

    probe = args.probe or find_probe()
    if not probe or not os.path.exists(probe):
        print("SKIP: shellprobe not built (build the tree first)")
        return 0

    shells = args.shell or ["bash", "csh"]
    failed = {}
    for shell in shells:
        if not which(shell):
            print("== %s: SKIP (not installed)" % shell)
            continue
        bad = run_shell(probe, shell, args.keep, args.verbose)
        if bad:
            failed[shell] = bad

    if failed:
        print("\nFAILED:")
        for shell, bad in failed.items():
            print("  %s: %s" % (shell, ", ".join(bad)))
        return 1
    print("\nall shells OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())

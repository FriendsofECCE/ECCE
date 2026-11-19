#!/usr/bin/env python3
"""Run app tests standalone and hosted (tools as Organizer tabs) and compare.

  hosted_suite.py [--modes normal,hosted] [-j N] [--out DIR] [--timeout S]
                  [--only REGEX] [TEST.py ...]

Without test names: every tests/apps/*_test.py that starts the Builder,
the Calculation Editor or the Launcher through its wrapper and does not
already choose ECCE_ONE_PROCESS itself.  "hosted" runs a test with
ECCE_TEST_HOSTED=1 and tests/apps/hosted on PYTHONPATH (see
hosted/sitecustomize.py); "normal" runs it as it is.  Each run gets its
own HOME and TMPDIR under --out, so parallel runs do not share a session.
Prints one line per test and mode, then the tests whose result differs.

Under tools/offload (`run python3 tests/apps/hosted_suite.py ...`) the
install under test is taken from $BLD, as `apptest` does.
"""
import argparse
import glob
import os
import re
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = re.compile(r'ecce-(builder|calced|launcher)|apps\.run\([^,]+, *"(builder|calced)"')


def startsTool(path, seen=None):
    """Does the file, or a tests/apps module it imports, start a tool?"""
    seen = seen if seen is not None else set()
    if path in seen or not os.path.exists(path):
        return False
    seen.add(path)
    text = open(path, errors="replace").read()
    if TOOLS.search(text):
        return True
    for mod in re.findall(r"^(?:import|from) (\w+)", text, re.M):
        if startsTool(os.path.join(HERE, mod + ".py"), seen):
            return True
    return False


def candidates():
    out = []
    for path in sorted(glob.glob(os.path.join(HERE, "*_test.py"))):
        text = open(path, errors="replace").read()
        if "ECCE_ONE_PROCESS" not in text and startsTool(path):
            out.append(path)
    return out


def tail(text, n=6):
    fails = [l for l in text.splitlines()
             if l.startswith("FAIL") or "Traceback" in l or "Error" in l]
    return " | ".join(fails[:n]) or " | ".join(text.splitlines()[-3:])


def runOne(test, mode, out, timeout):
    name = os.path.basename(test)[:-3]
    home = os.path.join(out, "home", mode + "-" + name)
    tmp = os.path.join(home, "tmp")
    os.makedirs(tmp, exist_ok=True)
    env = dict(os.environ, HOME=home, TMPDIR=tmp)
    bld = os.environ.get("BLD")
    if bld and "ECCE_TEST_HOME" not in os.environ:
        env.update(ECCE_TEST_HOME=os.path.join(bld, "install"),
                   ECCE_TEST_WRAPPERS=os.path.join(bld, "install-bin"),
                   ECCE_TEST_BUILD=os.path.join(bld, "bi"))
    if mode == "hosted":
        env["ECCE_TEST_HOSTED"] = "1"
        env["ECCE_TEST_HOSTED_LOGDIR"] = os.path.join(out, "hosted-" + name)
        os.makedirs(env["ECCE_TEST_HOSTED_LOGDIR"], exist_ok=True)
        env["PYTHONPATH"] = os.pathsep.join(
            [os.path.join(HERE, "hosted")] +
            ([env["PYTHONPATH"]] if env.get("PYTHONPATH") else []))
    args = [sys.executable, test]
    if bld and "--build" in open(test, errors="replace").read():
        args += ["--build", os.path.join(bld, "bi")]
    started = time.time()
    log = os.path.join(out, "%s.%s.log" % (name, mode))
    try:
        r = subprocess.run(args, env=env, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=timeout,
                           start_new_session=True)
        rc, text = r.returncode, r.stdout.decode("utf-8", "replace")
    except subprocess.TimeoutExpired as e:
        rc, text = "TIMEOUT", (e.stdout or b"").decode("utf-8", "replace")
    with open(log, "w") as handle:
        handle.write(text)
    verdict = ("PASS" if rc == 0 else "SKIP" if rc == 77 else
               "TIMEOUT" if rc == "TIMEOUT" else "FAIL(%s)" % rc)
    line = "%-40s %-7s %-10s %5.0fs %s" % (
        name, mode, verdict, time.time() - started,
        "" if verdict in ("PASS", "SKIP") else tail(text))
    print(line, flush=True)
    return name, mode, verdict


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("tests", nargs="*")
    ap.add_argument("--modes", default="normal,hosted")
    ap.add_argument("-j", type=int, default=3)
    ap.add_argument("--out", default=os.path.join(
        os.environ.get("TMPDIR", "/tmp"), "hosted-suite"))
    ap.add_argument("--timeout", type=int, default=900)
    ap.add_argument("--only")
    a = ap.parse_args()
    tests = [os.path.abspath(t) for t in a.tests] or candidates()
    if a.only:
        tests = [t for t in tests if re.search(a.only, os.path.basename(t))]
    os.makedirs(a.out, exist_ok=True)
    jobs = [(t, m) for t in tests for m in a.modes.split(",")]
    print("hosted_suite: %d tests, modes %s, -j %d, logs in %s"
          % (len(tests), a.modes, a.j, a.out), flush=True)
    with ThreadPoolExecutor(a.j) as pool:
        results = list(pool.map(lambda tm: runOne(tm[0], tm[1], a.out,
                                                  a.timeout), jobs))
    table = {}
    for name, mode, verdict in results:
        table.setdefault(name, {})[mode] = verdict
    modes = a.modes.split(",")
    print("\n== summary")
    for m in modes:
        vs = [table[n].get(m) for n in table]
        print("%-7s PASS %d  FAIL %d  SKIP %d  TIMEOUT %d" % (
            m, vs.count("PASS"), sum(1 for v in vs if v and v.startswith("FAIL")),
            vs.count("SKIP"), vs.count("TIMEOUT")))
    print("\n== differences")
    for n in sorted(table):
        if len(set(table[n].get(m) for m in modes)) > 1:
            print("%-40s %s" % (n, "  ".join("%s=%s" % (m, table[n].get(m))
                                             for m in modes)))
    return 0


if __name__ == "__main__":
    sys.exit(main())

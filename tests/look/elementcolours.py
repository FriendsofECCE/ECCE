#!/usr/bin/env python3
"""Element colour schemes: the Jmol table, the classic one, the saved choice.

    tests/look/elementcolours.py [--libdir BUILD]

Compiles elementcolours.C against a build tree and checks, in a private
home: a new user gets Jmol; spot checks of F, Cl, Br, I, S, P and the first
row; "ecce" gives exactly data/client/config/AtomicElements; a saved choice
is kept by a new process, and applying a changed choice recolours the
running tables.  No display.
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
LIBS = ["eccetdat", "ecceutil", "eccexml", "eccetdat", "ecceutil"]


def run(binary, env, *args):
    r = subprocess.run([binary] + list(args), env=env, capture_output=True,
                       text=True, timeout=60)
    if r.returncode:
        print(r.stdout[-1000:], r.stderr[-1000:])
        sys.exit(1)
    table = {}
    scheme = {}
    for line in r.stdout.splitlines():
        w = line.split()
        if w[0] == "scheme":
            scheme[w[1]] = w[2]
        else:
            table.setdefault(w[0], {})[w[1]] = w[2]
    return table, scheme


def classic():
    got = {}
    for line in open(os.path.join(ROOT, "data/client/config/AtomicElements")):
        if line.startswith("#") or ":" not in line:
            continue
        sym, rest = line.split(":", 1)
        got[sym.strip()] = rest.split(",")[5].strip().lower()
    return got


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--libdir", default=os.path.join(ROOT, "build-cmake"))
    o = ap.parse_args()
    work = tempfile.mkdtemp(prefix="elemcol")
    failures = []

    def check(ok, what):
        print("  %s %s" % ("ok  " if ok else "FAIL", what))
        if not ok:
            failures.append(what)

    try:
        binary = os.path.join(work, "elementcolours")
        cxx = subprocess.run(
            ["nice", "-n", "19", "g++", "-std=c++17", "-o", binary,
             os.path.join(HERE, "elementcolours.C"),
             "-I" + os.path.join(ROOT, "include"),
             "-L" + o.libdir, "-Wl,-rpath," + o.libdir,
             "-Wl,--start-group"] + ["-l" + l for l in LIBS] +
            ["-Wl,--end-group", "-lxerces-c"],
            capture_output=True, text=True)
        if cxx.returncode:
            print(cxx.stderr[-3000:])
            return 1
        home = os.path.join(work, "home")
        os.makedirs(os.path.join(home, ".ECCE"))
        env = dict(os.environ, HOME=home, ECCE_REALUSERHOME=home,
                   ECCE_HOME=ROOT)

        t, s = run(binary, env, "show")
        jmol = t["saved"]
        check(s["saved"] == "jmol", "a new user gets Jmol (%s)" % s["saved"])
        for sym, want in (("F", "#90e050"), ("Cl", "#1ff01f"),
                          ("Br", "#a62929"), ("I", "#940094"),
                          ("S", "#ffff30"), ("P", "#ff8000"),
                          ("H", "#ffffff"), ("C", "#909090"),
                          ("N", "#3050f8"), ("O", "#ff0d0d")):
            check(jmol[sym] == want, "Jmol %s %s (%s)" % (sym, want, jmol[sym]))

        t, s = run(binary, env, "switch", "ecce")
        check(s["before"] == "jmol" and s["after"] == "ecce",
              "switch applied: %s" % s)
        check(t["before"] == jmol, "before the switch the tables are Jmol")
        ref = classic()
        same = all(t["after"][k] == v for k, v in ref.items()
                   if k in t["after"])
        check(same and len(t["after"]) >= 100,
              "ecce: every element has its AtomicElements colour")
        check(t["after"]["F"] == "#9b30ff", "ecce F is the classic purple")

        t, s = run(binary, env, "show")
        check(s["saved"] == "ecce" and t["saved"] == classic_or(t, ref),
              "a new process keeps the saved choice")
        t, s = run(binary, env, "switch", "jmol")
        check(t["after"] == jmol, "back to Jmol")
        check(t["after"]["X"] == ref["X"], "the ghost atom keeps its colour")
    finally:
        shutil.rmtree(work, True)
    return 1 if failures else 0


def classic_or(t, ref):
    return t["saved"] if all(t["saved"][k] == v for k, v in ref.items()
                             if k in t["saved"]) else None


if __name__ == "__main__":
    sys.exit(main())
